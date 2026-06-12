#include "stage_controller.h"
#include "mt_api_bridge.h"
#include <QDebug>

using namespace MtAxisConfig;

StageController::StageController(QObject *parent)
    : DeviceBase(parent)
    , m_cardOpen(false)
    , m_axisRunning{false, false}
    , m_axisPending{false, false}
    , m_axisObservedRunning{false, false}
    , m_axisPos{0, 0}
    , m_speedPPS(DEFAULT_SPEED_PPS)
    , m_axisSpeedPPS{DEFAULT_SPEED_PPS, DEFAULT_SPEED_PPS}
    , m_pollTimer(new QTimer(this))
    , m_pendingAction(PendingAction::None)
{
    setDeviceName("MT_API 运动控制卡双轴位移台");

    // 兼容旧查询接口：MT_API 下 pulsePerUnit 表示 pulse/μm
    m_info1.pulsePerUnit = PULSES_PER_UM;
    m_info1.valid = true;
    m_info2.pulsePerUnit = PULSES_PER_UM;
    m_info2.valid = true;

    QObject::connect(m_pollTimer, &QTimer::timeout, this, &StageController::onPollTimer);
}

StageController::~StageController()
{
    disconnect();
}

void StageController::setPortName(const QString &port)
{
    m_portName = port;
}

QString StageController::portName() const
{
    return m_portName;
}

bool StageController::connect()
{
    if (m_cardOpen) return true;

    MtApiBridge &api = MtApiBridge::instance();
    if (!api.isLoaded()) {
        setError("MT_API.dll 未加载，请先在 initDevices() 中加载 DLL");
        return false;
    }

    if (!checkResult(api.MT_Init(), "MT_Init 初始化")) return false;

    // USB 即插即用：连接前先关闭已有 UART/USB 句柄，避免重复打开导致失败
    if (api.MT_Close_UART) api.MT_Close_UART();
    if (api.MT_Close_USB) api.MT_Close_USB();

    if (!checkResult(api.MT_Open_USB(), "MT_Open_USB 打开USB设备")) {
        api.MT_DeInit();
        return false;
    }

    if (!checkResult(api.MT_Check(), "MT_Check 检测控制卡")) {
        api.MT_Close_USB();
        api.MT_DeInit();
        return false;
    }

    int axisCount = 0;
    if (!checkResult(api.MT_Get_Axis_Num(&axisCount), "MT_Get_Axis_Num 获取轴数")) {
        api.MT_Close_USB();
        api.MT_DeInit();
        return false;
    }
    if (axisCount < REQUIRED_AXIS_COUNT) {
        setError(QString("运动控制卡轴数不足：当前%1轴，至少需要%2轴").arg(axisCount).arg(REQUIRED_AXIS_COUNT));
        api.MT_Close_USB();
        api.MT_DeInit();
        return false;
    }

    if (!configureAxis(AXIS_1) || !configureAxis(AXIS_2)) {
        api.MT_Close_USB();
        api.MT_DeInit();
        return false;
    }

    m_cardOpen = true;
    setStatus(DeviceStatus::Connected);
    emit connected();
    emit stage1Connected();
    emit stage2Connected();
    emit deviceInfoReceived(getDeviceInfo());
    m_pollTimer->start(POLL_INTERVAL_MS);
    qDebug() << "MT_API 运动控制卡(USB)连接成功, 轴数:" << axisCount;
    return true;
}

void StageController::disconnect()
{
    if (m_pollTimer) m_pollTimer->stop();
    if (m_cardOpen && MtApiBridge::instance().isLoaded()) {
        MtApiBridge &api = MtApiBridge::instance();
        if (api.MT_Set_Axis_Halt_All) api.MT_Set_Axis_Halt_All();
        if (api.MT_Close_USB) api.MT_Close_USB();
        if (api.MT_DeInit) api.MT_DeInit();
    }
    const bool wasOpen = m_cardOpen;
    resetRuntimeState();
    setStatus(DeviceStatus::Disconnected);
    if (wasOpen) {
        emit disconnected();
        emit stage1Disconnected();
        emit stage2Disconnected();
    }
}

bool StageController::isConnected() const
{
    return m_cardOpen;
}

QString StageController::getDeviceInfo() const
{
    return QString("MT_API 运动控制卡 [串口=%1, 轴0=%2 pulse/%3, 轴1=%4 pulse/%5, 速度=%6 pulse/s]")
        .arg(m_portName)
        .arg(m_axisPos[0]).arg(QString::number(positionUm1(), 'f', 2) + "μm")
        .arg(m_axisPos[1]).arg(QString::number(positionUm2(), 'f', 2) + "μm")
        .arg(m_speedPPS);
}

// ========== 旧双串口接口兼容壳 ==========

bool StageController::openPort1(const QString &portName, qint32 baudRate,
                                QSerialPort::DataBits dataBits,
                                QSerialPort::Parity parity,
                                QSerialPort::StopBits stopBits)
{
    Q_UNUSED(baudRate)
    Q_UNUSED(dataBits)
    Q_UNUSED(parity)
    Q_UNUSED(stopBits)

    // 业务用途：旧 UI 的 Stage1 连接按钮仍调用 openPort1，这里转为单控制卡连接
    setPortName(portName);
    return connect();
}

bool StageController::openPort2(const QString &portName, qint32 baudRate,
                                QSerialPort::DataBits dataBits,
                                QSerialPort::Parity parity,
                                QSerialPort::StopBits stopBits)
{
    Q_UNUSED(baudRate)
    Q_UNUSED(dataBits)
    Q_UNUSED(parity)
    Q_UNUSED(stopBits)

    // 业务用途：旧 UI 的 Stage2 连接按钮保留。若已连接则直接成功，否则也按所选串口连接整卡。
    if (m_cardOpen) return true;
    setPortName(portName);
    return connect();
}

void StageController::closePort1()
{
    disconnect();
}

void StageController::closePort2()
{
    disconnect();
}

bool StageController::isConnected1() const
{
    return m_cardOpen;
}

bool StageController::isConnected2() const
{
    return m_cardOpen;
}

// ========== MT_API 辅助方法 ==========

bool StageController::checkResult(int result, const QString &action)
{
    if (result == 0) return true;
    setError(QString("%1失败，MT_API返回码=%2").arg(action).arg(result));
    qDebug() << getLastError();
    return false;
}

bool StageController::configureAxis(unsigned short axis)
{
    MtApiBridge &api = MtApiBridge::instance();

    // 业务用途：按 Demo 验证流程优先使用普通位置模式；Position_Open 保留在桥接层备用。
    if (!checkResult(api.MT_Set_Axis_Mode_Position(axis), QString("轴%1 设置位置模式").arg(axis))) return false;
    if (!checkResult(api.MT_Set_Axis_Acc(axis, DEFAULT_ACC_PPS2), QString("轴%1 设置加速度").arg(axis))) return false;
    if (!checkResult(api.MT_Set_Axis_Dec(axis, DEFAULT_DEC_PPS2), QString("轴%1 设置减速度").arg(axis))) return false;
    if (!checkResult(api.MT_Set_Axis_Position_V_Max(axis, m_speedPPS), QString("轴%1 设置最大速度").arg(axis))) return false;
    if (!checkResult(api.MT_Set_Axis_Software_Limit_Neg_Value(axis, SOFT_LIMIT_NEG), QString("轴%1 设置负软限位").arg(axis))) return false;
    if (!checkResult(api.MT_Set_Axis_Software_Limit_Pos_Value(axis, SOFT_LIMIT_POS), QString("轴%1 设置正软限位").arg(axis))) return false;
    if (!checkResult(api.MT_Set_Axis_Software_Limit_Enable(axis), QString("轴%1 启用软限位").arg(axis))) return false;

    qDebug() << "轴配置完成:" << axis;
    return true;
}

void StageController::resetRuntimeState()
{
    m_cardOpen = false;
    m_axisRunning[0] = false;
    m_axisRunning[1] = false;
    m_axisPending[0] = false;
    m_axisPending[1] = false;
    m_axisObservedRunning[0] = false;
    m_axisObservedRunning[1] = false;
    m_pendingAction = PendingAction::None;
}


// ========== 双轴控制接口实现 ==========

bool StageController::readDeviceInfoDual()
{
    if (!isConnected()) { setError("运动控制卡未连接"); return false; }

    int pos1 = 0;
    int pos2 = 0;
    MtApiBridge &api = MtApiBridge::instance();
    bool ok = true;
    ok &= checkResult(api.MT_Get_Axis_Software_P_Now(AXIS_1, &pos1), "读取轴0当前位置");
    ok &= checkResult(api.MT_Get_Axis_Software_P_Now(AXIS_2, &pos2), "读取轴1当前位置");
    if (!ok) return false;

    m_axisPos[0] = pos1;
    m_axisPos[1] = pos2;
    emit positionChanged1(m_axisPos[0]);
    emit positionChanged2(m_axisPos[1]);
    emit deviceInfoReceived(getDeviceInfo());
    return true;
}

bool StageController::setMaxSpeedDual(qint32 speedPulsesPerSec)
{
    if (speedPulsesPerSec <= 0) {
        setError("运动速度必须为正数（pulse/s）");
        return false;
    }

    m_speedPPS = speedPulsesPerSec;
    if (!isConnected()) return true;

    MtApiBridge &api = MtApiBridge::instance();
    bool ok = true;
    ok &= checkResult(api.MT_Set_Axis_Position_V_Max(AXIS_1, m_speedPPS), "设置轴0最大速度");
    ok &= checkResult(api.MT_Set_Axis_Position_V_Max(AXIS_2, m_speedPPS), "设置轴1最大速度");
    return ok;
}

bool StageController::setMaxSpeedUmPerSec(double umPerSec)
{
    if (umPerSec <= 0) {
        setError("运动速度必须为正数（μm/s）");
        return false;
    }
    return setMaxSpeedDual(umToPulses(umPerSec));
}

bool StageController::moveAbsoluteDual(double displacementUm)
{
    if (!isConnected()) { setError("运动控制卡未连接"); return false; }

    int pulses = umToPulses(displacementUm);
    if (!isWithinSoftLimit(pulses)) {
        setError(QString("目标位置越过软件限位：%1 μm → %2 pulse").arg(displacementUm).arg(pulses));
        return false;
    }

    MtApiBridge &api = MtApiBridge::instance();
    bool ok = true;
    ok &= checkResult(api.MT_Set_Axis_Position_V_Max(AXIS_1, m_speedPPS), "设置轴0运动速度");
    ok &= checkResult(api.MT_Set_Axis_Position_V_Max(AXIS_2, m_speedPPS), "设置轴1运动速度");
    if (!ok) return false;

    m_axisPending[0] = true;
    m_axisPending[1] = true;
    m_axisObservedRunning[0] = false;
    m_axisObservedRunning[1] = false;
    m_pendingAction = PendingAction::Move;

    ok &= checkResult(api.MT_Set_Axis_Position_P_Target_Abs(AXIS_1, pulses), "轴0绝对定位");
    ok &= checkResult(api.MT_Set_Axis_Position_P_Target_Abs(AXIS_2, pulses), "轴1绝对定位");
    if (!ok) {
        m_axisPending[0] = false;
        m_axisPending[1] = false;
        m_axisObservedRunning[0] = false;
        m_axisObservedRunning[1] = false;
        m_pendingAction = PendingAction::None;
    }

    qDebug() << QString("双轴绝对位移: %1 μm → %2 pulse, speed=%3 pulse/s")
                .arg(displacementUm, 0, 'f', 3).arg(pulses).arg(m_speedPPS);
    return ok;
}


bool StageController::queryStatusDual()
{
    if (!isConnected()) { setError("运动控制卡未连接"); return false; }

    int run1 = 0;
    int run2 = 0;
    MtApiBridge &api = MtApiBridge::instance();
    bool ok = true;
    ok &= checkResult(api.MT_Get_Axis_Status_Run(AXIS_1, &run1), "查询轴0运行状态");
    ok &= checkResult(api.MT_Get_Axis_Status_Run(AXIS_2, &run2), "查询轴1运行状态");
    if (ok) {
        m_axisRunning[0] = (run1 != 0);
        m_axisRunning[1] = (run2 != 0);
    }
    return ok;
}

bool StageController::queryPositionDual()
{
    return readDeviceInfoDual();
}

bool StageController::stopDual()
{
    if (!isConnected()) { setError("运动控制卡未连接"); return false; }

    MtApiBridge &api = MtApiBridge::instance();
    bool ok = true;
    ok &= checkResult(api.MT_Set_Axis_Position_Stop(AXIS_1), "停止轴0定位运动");
    ok &= checkResult(api.MT_Set_Axis_Position_Stop(AXIS_2), "停止轴1定位运动");
    m_axisPending[0] = false;
    m_axisPending[1] = false;
    m_axisObservedRunning[0] = false;
    m_axisObservedRunning[1] = false;
    m_pendingAction = PendingAction::None;
    return ok;
}

bool StageController::haltDual()
{
    if (!isConnected()) { setError("运动控制卡未连接"); return false; }

    bool ok = checkResult(MtApiBridge::instance().MT_Set_Axis_Halt_All(), "双轴急停");
    m_axisPending[0] = false;
    m_axisPending[1] = false;
    m_axisObservedRunning[0] = false;
    m_axisObservedRunning[1] = false;
    m_pendingAction = PendingAction::None;
    return ok;
}

bool StageController::homeDual(quint8 direction)
{
    Q_UNUSED(direction)
    if (!isConnected()) { setError("运动控制卡未连接"); return false; }

    MtApiBridge &api = MtApiBridge::instance();
    const int homeSpeed = (HOME_DIRECTION >= 0) ? HOME_SPEED_PPS : -HOME_SPEED_PPS;
    bool ok = true;
    ok &= checkResult(api.MT_Set_Axis_Mode_Home_Home_Switch(AXIS_1), "轴0设置回零模式");
    ok &= checkResult(api.MT_Set_Axis_Home_V(AXIS_1, homeSpeed), "轴0设置回零速度");
    ok &= checkResult(api.MT_Set_Axis_Mode_Home_Home_Switch(AXIS_2), "轴1设置回零模式");
    ok &= checkResult(api.MT_Set_Axis_Home_V(AXIS_2, homeSpeed), "轴1设置回零速度");
    if (!ok) return false;

    m_axisPending[0] = true;
    m_axisPending[1] = true;
    m_axisObservedRunning[0] = false;
    m_axisObservedRunning[1] = false;
    m_pendingAction = PendingAction::Home;
    return true;
}

// ========== 单轴调试接口 ==========

bool StageController::moveAbsoluteAxis(unsigned short axis, double displacementUm)
{
    if (!isConnected()) { setError("运动控制卡未连接"); return false; }
    if (axis > 1) { setError("非法轴号（仅支持 0/1）"); return false; }

    int pulses = umToPulses(displacementUm);
    if (!isWithinSoftLimit(pulses)) {
        setError(QString("轴%1目标位置越过软件限位：%2 μm → %3 pulse").arg(axis).arg(displacementUm).arg(pulses));
        return false;
    }

    // 单轴独立控制：使用该轴自己的速度（m_axisSpeedPPS），不影响另一轴与双轴预设流程。
    MtApiBridge &api = MtApiBridge::instance();
    if (!checkResult(api.MT_Set_Axis_Position_V_Max(axis, m_axisSpeedPPS[axis]), QString("设置轴%1速度").arg(axis))) return false;
    return checkResult(api.MT_Set_Axis_Position_P_Target_Abs(axis, pulses), QString("轴%1绝对定位").arg(axis));
}

bool StageController::setAxisSpeedUmPerSec(unsigned short axis, double umPerSec)
{
    if (axis > 1) { setError("非法轴号（仅支持 0/1）"); return false; }
    if (umPerSec <= 0) {
        setError("运动速度必须为正数（μm/s）");
        return false;
    }

    // 入参 μm/s → pulse/s，记录到该轴独立速度。未连接时仅缓存，连接后下发也生效。
    m_axisSpeedPPS[axis] = umToPulses(umPerSec);
    if (!isConnected()) return true;

    return checkResult(MtApiBridge::instance().MT_Set_Axis_Position_V_Max(axis, m_axisSpeedPPS[axis]),
                       QString("设置轴%1运动速度").arg(axis));
}

bool StageController::stopAxis(unsigned short axis)
{
    if (axis > 1) { setError("非法轴号（仅支持 0/1）"); return false; }
    if (!isConnected()) { setError("运动控制卡未连接"); return false; }

    return checkResult(MtApiBridge::instance().MT_Set_Axis_Position_Stop(axis),
                       QString("停止轴%1运动").arg(axis));
}

bool StageController::moveAbsolute1(double displacementUm)
{
    return moveAbsoluteAxis(AXIS_1, displacementUm);
}

bool StageController::moveAbsolute2(double displacementUm)
{
    return moveAbsoluteAxis(AXIS_2, displacementUm);
}


void StageController::onPollTimer()
{
    if (!m_cardOpen || !MtApiBridge::instance().isLoaded()) return;

    MtApiBridge &api = MtApiBridge::instance();
    const unsigned short axes[2] = { AXIS_1, AXIS_2 };
    bool allAxesFinished = (m_pendingAction != PendingAction::None);

    for (int i = 0; i < 2; ++i) {
        int run = 0;
        int pos = m_axisPos[i];

        if (api.MT_Get_Axis_Status_Run(axes[i], &run) != 0) {
            allAxesFinished = false;
            continue;
        }
        if (api.MT_Get_Axis_Software_P_Now(axes[i], &pos) != 0) {
            allAxesFinished = false;
            continue;
        }

        const bool running = (run != 0);
        m_axisRunning[i] = running;

        if (pos != m_axisPos[i]) {
            m_axisPos[i] = pos;
            if (i == 0) emit positionChanged1(pos);
            else emit positionChanged2(pos);
        }

        // 防误完成：任务刚下发时 run 可能还未变为1，必须先观察到 running=true。
        if (m_axisPending[i] && running) {
            m_axisObservedRunning[i] = true;
        }

        if (!m_axisPending[i]) {
            allAxesFinished = false;
        } else if (!m_axisObservedRunning[i]) {
            allAxesFinished = false;
        } else if (running) {
            allAxesFinished = false;
        }
    }

    // 业务用途：两个轴都经历“运行中→停止”后，才认为本次 Move/Home 完成，只发一次完成信号。
    if (allAxesFinished) {
        PendingAction finishedAction = m_pendingAction;
        m_axisPending[0] = false;
        m_axisPending[1] = false;
        m_axisObservedRunning[0] = false;
        m_axisObservedRunning[1] = false;
        m_pendingAction = PendingAction::None;

        if (finishedAction == PendingAction::Home) {
            // 回零完成后切回位置模式，保证后续绝对定位仍可执行。
            configureAxis(AXIS_1);
            configureAxis(AXIS_2);
            m_axisPos[0] = 0;
            m_axisPos[1] = 0;
            emit positionChanged1(0);
            emit positionChanged2(0);
        }

        emit moveCompletedDual();
    }
}




#include "stage_controller.h"
#include <QDebug>
#include <QThread>

// ===================================================================
// Thorlabs ELLx 双台位移台控制器（双串口聚合）实现
// ===================================================================

StageController::StageController(QObject *parent)
    : DeviceBase(parent)
    , m_serial1(new SerialPortBase(this))
    , m_serial2(new SerialPortBase(this))
    , m_baudRate1(9600)
    , m_baudRate2(9600)
    , m_pendingMove1(false)
    , m_pendingMove2(false)
{
    setDeviceName("Thorlabs ELLx 双台位移台（双串口）");

    // ----- 串口1信号 → 槽 -----
    QObject::connect(m_serial1, &SerialPortBase::dataReceived,
                     this, &StageController::onSerial1DataReceived);
    QObject::connect(m_serial1, &SerialPortBase::connected,
                     this, &StageController::onSerial1Connected);
    QObject::connect(m_serial1, &SerialPortBase::disconnected,
                     this, &StageController::onSerial1Disconnected);
    QObject::connect(m_serial1, &SerialPortBase::errorOccurred,
                     this, &StageController::onSerial1Error);

    // ----- 串口2信号 → 槽 -----
    QObject::connect(m_serial2, &SerialPortBase::dataReceived,
                     this, &StageController::onSerial2DataReceived);
    QObject::connect(m_serial2, &SerialPortBase::connected,
                     this, &StageController::onSerial2Connected);
    QObject::connect(m_serial2, &SerialPortBase::disconnected,
                     this, &StageController::onSerial2Disconnected);
    QObject::connect(m_serial2, &SerialPortBase::errorOccurred,
                     this, &StageController::onSerial2Error);
}

StageController::~StageController()
{
    disconnect();
}

// ========== 基类接口 ==========

bool StageController::connect()
{
    // 适用于"先调用 openPort1/2 设置好端口，再调用基类 connect()"的场景
    bool ok = true;
    if (!m_portName1.isEmpty() && !m_serial1->isOpen()) {
        ok &= openPort1(m_portName1, m_baudRate1);
    }
    if (!m_portName2.isEmpty() && !m_serial2->isOpen()) {
        ok &= openPort2(m_portName2, m_baudRate2);
    }
    return ok;
}

void StageController::disconnect()
{
    closePort1();
    closePort2();
    setStatus(DeviceStatus::Disconnected);
}

bool StageController::isConnected() const
{
    return isConnected1() && isConnected2();
}

QString StageController::getDeviceInfo() const
{
    return QString("Thorlabs ELLx 双台 [台1: 串口=%1, 波特率=%2, ppu=%3 | "
                   "台2: 串口=%4, 波特率=%5, ppu=%6]")
           .arg(m_portName1)
           .arg(m_baudRate1)
           .arg(m_info1.pulsePerUnit, 0, 'f', 2)
           .arg(m_portName2)
           .arg(m_baudRate2)
           .arg(m_info2.pulsePerUnit, 0, 'f', 2);
}

// ========== 双串口连接 ==========

bool StageController::openPort1(const QString &portName, qint32 baudRate,
                                QSerialPort::DataBits dataBits,
                                QSerialPort::Parity parity,
                                QSerialPort::StopBits stopBits)
{
    m_portName1 = portName;
    m_baudRate1 = baudRate;

    const int MAX_RETRY = 3;
    for (int i = 0; i < MAX_RETRY; i++) {
        if (m_serial1->openPort(portName, baudRate, dataBits, parity, stopBits)) {
            qDebug() << "位移台1 串口打开成功:" << portName;
            return true;
        }
        if (i < MAX_RETRY - 1) {
            qDebug() << "位移台1 串口打开失败，重试..." << (i + 1);
            QThread::msleep(500);
        }
    }
    setError(QString("无法打开位移台1串口 %1").arg(portName));
    return false;
}

bool StageController::openPort2(const QString &portName, qint32 baudRate,
                                QSerialPort::DataBits dataBits,
                                QSerialPort::Parity parity,
                                QSerialPort::StopBits stopBits)
{
    m_portName2 = portName;
    m_baudRate2 = baudRate;

    const int MAX_RETRY = 3;
    for (int i = 0; i < MAX_RETRY; i++) {
        if (m_serial2->openPort(portName, baudRate, dataBits, parity, stopBits)) {
            qDebug() << "位移台2 串口打开成功:" << portName;
            return true;
        }
        if (i < MAX_RETRY - 1) {
            qDebug() << "位移台2 串口打开失败，重试..." << (i + 1);
            QThread::msleep(500);
        }
    }
    setError(QString("无法打开位移台2串口 %1").arg(portName));
    return false;
}

void StageController::closePort1()
{
    if (m_serial1->isOpen()) m_serial1->closePort();
}

void StageController::closePort2()
{
    if (m_serial2->isOpen()) m_serial2->closePort();
}

bool StageController::isConnected1() const { return m_serial1->isOpen(); }
bool StageController::isConnected2() const { return m_serial2->isOpen(); }

// ========== 串口事件槽 ==========

void StageController::onSerial1DataReceived(const QByteArray &data)
{
    m_recvBuf1.append(data);
    while (m_recvBuf1.contains('\n')) {
        int idx = m_recvBuf1.indexOf('\n');
        QByteArray line = m_recvBuf1.left(idx + 1);
        m_recvBuf1.remove(0, idx + 1);

        QString resp = QString::fromLatin1(line).trimmed();
        if (!resp.isEmpty()) {
            qDebug() << "位移台1 收到:" << resp;
            parseLine(1, resp);
        }
    }
    emit dataReady(data);
}

void StageController::onSerial2DataReceived(const QByteArray &data)
{
    m_recvBuf2.append(data);
    while (m_recvBuf2.contains('\n')) {
        int idx = m_recvBuf2.indexOf('\n');
        QByteArray line = m_recvBuf2.left(idx + 1);
        m_recvBuf2.remove(0, idx + 1);

        QString resp = QString::fromLatin1(line).trimmed();
        if (!resp.isEmpty()) {
            qDebug() << "位移台2 收到:" << resp;
            parseLine(2, resp);
        }
    }
    emit dataReady(data);
}

void StageController::onSerial1Connected()
{
    emit stage1Connected();
    if (isConnected()) setStatus(DeviceStatus::Connected);
    qDebug() << "位移台1 已连接";
}

void StageController::onSerial1Disconnected()
{
    emit stage1Disconnected();
    if (!isConnected1() && !isConnected2()) {
        setStatus(DeviceStatus::Disconnected);
    }
    qDebug() << "位移台1 已断开";
}

void StageController::onSerial1Error(const QString &error)
{
    setError(QString("位移台1 错误: %1").arg(error));
    qDebug() << "位移台1 错误:" << error;
}

void StageController::onSerial2Connected()
{
    emit stage2Connected();
    if (isConnected()) setStatus(DeviceStatus::Connected);
    qDebug() << "位移台2 已连接";
}

void StageController::onSerial2Disconnected()
{
    emit stage2Disconnected();
    if (!isConnected1() && !isConnected2()) {
        setStatus(DeviceStatus::Disconnected);
    }
    qDebug() << "位移台2 已断开";
}

void StageController::onSerial2Error(const QString &error)
{
    setError(QString("位移台2 错误: %1").arg(error));
    qDebug() << "位移台2 错误:" << error;
}

// ========== 帧发送 ==========

bool StageController::sendFrame1(const QByteArray &frame)
{
    qint64 written = m_serial1->writeData(frame);
    if (written != frame.size()) {
        setError("位移台1 发送指令失败");
        return false;
    }
    qDebug() << "位移台1 发送:" << QString::fromLatin1(frame).trimmed();
    return true;
}

bool StageController::sendFrame2(const QByteArray &frame)
{
    qint64 written = m_serial2->writeData(frame);
    if (written != frame.size()) {
        setError("位移台2 发送指令失败");
        return false;
    }
    qDebug() << "位移台2 发送:" << QString::fromLatin1(frame).trimmed();
    return true;
}

// ========== 响应解析 ==========

void StageController::parseLine(int stageIndex, const QString &line)
{
    if (line.length() < 3) return;

    // 单台协议：响应行首字符为地址（固定 '0'），第1-2字符为命令类型
    QString cmdType = line.mid(1, 2).toUpper();

    if (cmdType == STAGE_RESP_INFO) {
        parseInfoLine(stageIndex, line);
    } else if (cmdType == STAGE_RESP_STATUS) {
        parseStatusLine(stageIndex, line);
    } else if (cmdType == STAGE_RESP_POSITION) {
        parsePositionLine(stageIndex, line);
    } else {
        qDebug() << QString("位移台%1 未知响应:").arg(stageIndex) << cmdType;
    }
}

void StageController::parseInfoLine(int stageIndex, const QString &resp)
{
    // 0IN<type(2)><serial(8)><year(4)><fw(2)><hw(2)><travel(4)><pulsePerUnit(8)>
    if (resp.length() < 31) {
        qDebug() << "IN 响应长度不足:" << resp;
        return;
    }
    bool ok;
    qint32 ppu = resp.mid(23, 8).toLong(&ok, 16);
    if (!ok || ppu <= 0) {
        qDebug() << "IN 响应 pulsePerUnit 解析失败，使用默认值 2048";
        ppu = 2048;
    }

    if (stageIndex == 1) {
        m_info1.pulsePerUnit = static_cast<double>(ppu);
        m_info1.valid = true;
        qDebug() << "台1 pulsePerUnit =" << m_info1.pulsePerUnit;
    } else {
        m_info2.pulsePerUnit = static_cast<double>(ppu);
        m_info2.valid = true;
        qDebug() << "台2 pulsePerUnit =" << m_info2.pulsePerUnit;
    }
    emit deviceInfoReceived(resp);
}

void StageController::parseStatusLine(int stageIndex, const QString &resp)
{
    // GS 响应：0GS<status(2)>
    if (resp.length() < 5) return;
    QString statusCode = resp.mid(3, 2);
    qDebug() << QString("台%1 状态码: %2").arg(stageIndex).arg(statusCode);
    if (statusCode != "00") {
        setError(QString("台%1 设备错误: %2").arg(stageIndex).arg(statusCode));
    }
}

void StageController::parsePositionLine(int stageIndex, const QString &resp)
{
    // PO 响应：0PO<pos(8)>
    if (resp.length() < 11) return;
    bool ok;
    quint32 raw = resp.mid(3, 8).toULong(&ok, 16);
    if (!ok) return;
    qint32 pulses = static_cast<qint32>(raw);

    // 仅在本次 PO 是 moveAbsoluteDual 的回应时（pending 为 true）才参与
    // moveCompletedDual 的判定，避免普通 queryPositionDual 误触发。
    bool wasPending = false;
    if (stageIndex == 1) {
        wasPending = m_pendingMove1;
        m_status1.positionPulses = pulses;
        qDebug() << "台1 位置(脉冲):" << pulses;
        emit positionChanged1(pulses);
        if (m_pendingMove1) m_pendingMove1 = false;
    } else {
        wasPending = m_pendingMove2;
        m_status2.positionPulses = pulses;
        qDebug() << "台2 位置(脉冲):" << pulses;
        emit positionChanged2(pulses);
        if (m_pendingMove2) m_pendingMove2 = false;
    }

    // 本次 PO 来源于一次 dual 移动，且两台都已就绪，则发射完成
    if (wasPending && !m_pendingMove1 && !m_pendingMove2) {
        emit moveCompletedDual();
    }
}

// ========== 双台同步控制接口实现 ==========

bool StageController::readDeviceInfoDual()
{
    if (!isConnected()) { setError("位移台未全部连接"); return false; }

    bool ok = true;
    ok &= sendFrame1(ellxBuildFrame('0', STAGE_CMD_GET_INFO));
    QThread::msleep(50);
    ok &= sendFrame2(ellxBuildFrame('0', STAGE_CMD_GET_INFO));
    return ok;
}

bool StageController::setMaxSpeedDual(qint32 speedPulses)
{
    if (!isConnected()) { setError("位移台未全部连接"); return false; }

    QString param = ellxPulsesToHex(speedPulses);
    bool ok = true;
    ok &= sendFrame1(ellxBuildFrame('0', STAGE_CMD_SET_SPEED, param));
    QThread::msleep(50);
    ok &= sendFrame2(ellxBuildFrame('0', STAGE_CMD_SET_SPEED, param));
    qDebug() << "双台设置速度:" << speedPulses << "脉冲/s";
    return ok;
}

bool StageController::moveAbsoluteDual(double positionMm)
{
    if (!isConnected()) { setError("位移台未全部连接"); return false; }

    // 台1 / 台2 各按自己的 pulsePerUnit 换算
    double ppu1 = m_info1.valid ? m_info1.pulsePerUnit : 2048.0;
    double ppu2 = m_info2.valid ? m_info2.pulsePerUnit : 2048.0;
    qint32 pulses1 = ellxMmToPulses(positionMm, ppu1);
    qint32 pulses2 = ellxMmToPulses(positionMm, ppu2);
    QString param1 = ellxPulsesToHex(pulses1);
    QString param2 = ellxPulsesToHex(pulses2);

    m_pendingMove1 = true;
    m_pendingMove2 = true;

    bool ok = true;
    ok &= sendFrame1(ellxBuildFrame('0', STAGE_CMD_MOVE_ABS, param1));
    QThread::msleep(50);
    ok &= sendFrame2(ellxBuildFrame('0', STAGE_CMD_MOVE_ABS, param2));

    qDebug() << QString("双台绝对位移: %1 mm → 台1=%2脉冲, 台2=%3脉冲")
                .arg(positionMm, 0, 'f', 3)
                .arg(pulses1).arg(pulses2);

    if (!ok) {
        // 发送失败时清除挂起标志，避免误触发 moveCompletedDual
        m_pendingMove1 = false;
        m_pendingMove2 = false;
    }
    return ok;
}

bool StageController::queryStatusDual()
{
    if (!isConnected()) { setError("位移台未全部连接"); return false; }
    bool ok = true;
    ok &= sendFrame1(ellxBuildFrame('0', STAGE_CMD_GET_STATUS));
    QThread::msleep(50);
    ok &= sendFrame2(ellxBuildFrame('0', STAGE_CMD_GET_STATUS));
    return ok;
}

bool StageController::queryPositionDual()
{
    if (!isConnected()) { setError("位移台未全部连接"); return false; }
    bool ok = true;
    ok &= sendFrame1(ellxBuildFrame('0', STAGE_CMD_GET_POS));
    QThread::msleep(50);
    ok &= sendFrame2(ellxBuildFrame('0', STAGE_CMD_GET_POS));
    return ok;
}

bool StageController::stopDual()
{
    if (!isConnected()) { setError("位移台未全部连接"); return false; }
    bool ok = true;
    ok &= sendFrame1(ellxBuildFrame('0', STAGE_CMD_STOP));
    QThread::msleep(50);
    ok &= sendFrame2(ellxBuildFrame('0', STAGE_CMD_STOP));
    qDebug() << "双台停止";
    // 停止时清除挂起标志
    m_pendingMove1 = false;
    m_pendingMove2 = false;
    return ok;
}

bool StageController::homeDual(quint8 direction)
{
    if (!isConnected()) { setError("位移台未全部连接"); return false; }
    QString dirStr = QString::number(direction);
    bool ok = true;
    ok &= sendFrame1(ellxBuildFrame('0', STAGE_CMD_HOME, dirStr));
    QThread::msleep(50);
    ok &= sendFrame2(ellxBuildFrame('0', STAGE_CMD_HOME, dirStr));
    qDebug() << "双台回零, 方向:" << direction;
    return ok;
}

// ========== 单台调试接口 ==========

bool StageController::moveAbsolute1(double positionMm)
{
    if (!isConnected1()) { setError("位移台1 未连接"); return false; }
    double ppu = m_info1.valid ? m_info1.pulsePerUnit : 2048.0;
    qint32 pulses = ellxMmToPulses(positionMm, ppu);
    QString param = ellxPulsesToHex(pulses);
    return sendFrame1(ellxBuildFrame('0', STAGE_CMD_MOVE_ABS, param));
}

bool StageController::moveAbsolute2(double positionMm)
{
    if (!isConnected2()) { setError("位移台2 未连接"); return false; }
    double ppu = m_info2.valid ? m_info2.pulsePerUnit : 2048.0;
    qint32 pulses = ellxMmToPulses(positionMm, ppu);
    QString param = ellxPulsesToHex(pulses);
    return sendFrame2(ellxBuildFrame('0', STAGE_CMD_MOVE_ABS, param));
}



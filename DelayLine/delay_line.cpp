#include "delay_line.h"
#include <QDebug>
#include <QThread>

DelayLine::DelayLine(QObject *parent)
    : DeviceBase(parent)
    , m_serialPort(new SerialPortBase(this))
    , m_baudRate(9600)
    , m_deviceId(0x01)
    , m_pollTimer(new QTimer(this))
    , m_commTimeoutTimer(new QTimer(this))
    , m_lastPosition(0.0f)
    , m_stableCount(0)
{
    setDeviceName("电动光纤延时线");

    // 串口信号连接
    QObject::connect(m_serialPort, &SerialPortBase::dataReceived,
                     this, &DelayLine::onDataReceived);
    QObject::connect(m_serialPort, &SerialPortBase::connected,
                     this, &DelayLine::onSerialConnected);
    QObject::connect(m_serialPort, &SerialPortBase::disconnected,
                     this, &DelayLine::onSerialDisconnected);
    QObject::connect(m_serialPort, &SerialPortBase::errorOccurred,
                     this, &DelayLine::onSerialError);

    // 轮询定时器（默认未启动）
    QObject::connect(m_pollTimer, &QTimer::timeout,
                     this, &DelayLine::onPollTimer);

    // 通信超时定时器：500ms 单次
    m_commTimeoutTimer->setSingleShot(true);
    QObject::connect(m_commTimeoutTimer, &QTimer::timeout,
                     this, &DelayLine::onCommTimeoutCheck);
}

DelayLine::~DelayLine()
{
    stopPolling();
    disconnect();
}

bool DelayLine::connect()
{
    if (m_portName.isEmpty()) {
        setError("串口名称未设置");
        return false;
    }

    return openPort(m_portName, m_baudRate);
}

void DelayLine::disconnect()
{
    stopPolling();
    if (m_serialPort->isOpen()) {
        m_serialPort->closePort();
        setStatus(DeviceStatus::Disconnected);
    }
}

bool DelayLine::isConnected() const
{
    return m_serialPort->isOpen();
}

QString DelayLine::getDeviceInfo() const
{
    return QString("%1 (串口: %2, 波特率: %3, 设备ID: 0x%4)")
           .arg(m_deviceName)
           .arg(m_portName)
           .arg(m_baudRate)
           .arg(m_deviceId, 2, 16, QChar('0'));
}

bool DelayLine::openPort(const QString &portName, qint32 baudRate,
                        QSerialPort::DataBits dataBits,
                        QSerialPort::Parity parity,
                        QSerialPort::StopBits stopBits)
{
    m_portName = portName;
    m_baudRate = baudRate;

    const int MAX_RETRY = 3;
    for (int i = 0; i < MAX_RETRY; i++) {
        if (m_serialPort->openPort(portName, baudRate, dataBits, parity, stopBits)) {
            qDebug() << "延时线连接成功:" << portName;
            return true;
        }

        if (i < MAX_RETRY - 1) {
            qDebug() << "延时线连接失败，重试中..." << (i + 1);
            QThread::msleep(1000);
        }
    }

    setError(QString("延时线连接失败: 无法打开串口 %1").arg(portName));
    return false;
}

// ========== 延时线控制函数 ==========

bool DelayLine::setDelay(float delayPS)
{
    if (!isConnected()) {
        setError("设备未连接");
        return false;
    }

    // 功能码 0x04：设置时间延迟，PS×1000→3字节大端
    QByteArray data = delayToBytes(delayPS);
    QByteArray frame = buildFrame(DELAY_CMD_SET_DELAY, data);

    qint64 written = m_serialPort->writeData(frame);
    if (written != frame.size()) {
        setError("发送设置延迟命令失败");
        return false;
    }

    qDebug() << "延时线[ID=" << m_deviceId << "] 设置延迟:" << delayPS
             << "PS, 帧:" << frame.toHex(' ');
    m_currentStatus.currentDelay = delayPS;
    return true;
}

bool DelayLine::home()
{
    if (!isConnected()) {
        setError("设备未连接");
        return false;
    }

    // 功能码 0x07：归零，参数全0
    QByteArray data(3, 0x00);
    QByteArray frame = buildFrame(DELAY_CMD_HOME, data);

    qint64 written = m_serialPort->writeData(frame);
    if (written != frame.size()) {
        setError("发送归零命令失败");
        return false;
    }

    qDebug() << "延时线[ID=" << m_deviceId << "] 归零, 帧:" << frame.toHex(' ');
    m_currentStatus.currentDelay = 0.0f;
    m_currentStatus.isHomed = true;
    return true;
}

bool DelayLine::stop()
{
    if (!isConnected()) {
        setError("设备未连接");
        return false;
    }

    // 功能码 0x0B：停止运动，参数全0
    QByteArray data(3, 0x00);
    QByteArray frame = buildFrame(DELAY_CMD_STOP, data);

    qint64 written = m_serialPort->writeData(frame);
    if (written != frame.size()) {
        setError("发送停止命令失败");
        return false;
    }

    qDebug() << "延时线[ID=" << m_deviceId << "] 停止, 帧:" << frame.toHex(' ');
    m_currentStatus.isMoving = false;
    return true;
}

bool DelayLine::queryPosition()
{
    if (!isConnected()) {
        setError("设备未连接");
        return false;
    }

    // 功能码 0x0E：查询当前位置，参数全0
    QByteArray data(3, 0x00);
    QByteArray frame = buildFrame(DELAY_CMD_QUERY_POS, data);

    qint64 written = m_serialPort->writeData(frame);
    if (written != frame.size()) {
        setError("发送查询位置命令失败");
        return false;
    }

    qDebug() << "延时线[ID=" << m_deviceId << "] 查询位置, 帧:" << frame.toHex(' ');
    return true;
}

bool DelayLine::setSpeed(quint8 speed)
{
    if (!isConnected()) {
        setError("设备未连接");
        return false;
    }

    // 功能码 0x02：设置速度，数据域 00 00 SPD（speed 越小越快，0x12最快~0xFF最慢）
    QByteArray data;
    data.append(static_cast<char>(0x00));
    data.append(static_cast<char>(0x00));
    data.append(static_cast<char>(speed));
    QByteArray frame = buildFrame(DELAY_CMD_SET_SPEED, data);

    qint64 written = m_serialPort->writeData(frame);
    if (written != frame.size()) {
        setError("发送设置速度命令失败");
        return false;
    }

    qDebug() << "延时线[ID=" << m_deviceId << "] 设置速度:" << speed
             << ", 帧:" << frame.toHex(' ');
    return true;
}

bool DelayLine::saveEprom()
{
    if (!isConnected()) {
        setError("设备未连接");
        return false;
    }

    // 功能码 0x0F：保存配置到EPROM，参数全0
    QByteArray data(3, 0x00);
    QByteArray frame = buildFrame(DELAY_CMD_SAVE_EPROM, data);

    qint64 written = m_serialPort->writeData(frame);
    if (written != frame.size()) {
        setError("发送保存EPROM命令失败");
        return false;
    }

    qDebug() << "延时线[ID=" << m_deviceId << "] 保存EPROM, 帧:" << frame.toHex(' ');
    return true;
}

bool DelayLine::enableRealtime(bool enable)
{
    if (!isConnected()) {
        setError("设备未连接");
        return false;
    }

    // 功能码 0x39：开/关实时上报，数据域 05 06 01(开)/00(关)
    QByteArray data;
    data.append(static_cast<char>(DELAY_REALTIME_PREFIX_D0));
    data.append(static_cast<char>(DELAY_REALTIME_PREFIX_D1));
    data.append(static_cast<char>(enable ? 0x01 : 0x00));
    QByteArray frame = buildFrame(DELAY_CMD_REALTIME, data);

    qint64 written = m_serialPort->writeData(frame);
    if (written != frame.size()) {
        setError("发送实时上报命令失败");
        return false;
    }

    qDebug() << "延时线[ID=" << m_deviceId << "] 实时上报:" << (enable ? "开" : "关")
             << ", 帧:" << frame.toHex(' ');
    return true;
}

float DelayLine::getCurrentDelay() const
{
    return m_currentStatus.currentDelay;
}

bool DelayLine::isMoving() const
{
    return m_currentStatus.isMoving;
}

// ========== 实时位置轮询控制 ==========

void DelayLine::startPolling(int intervalMs)
{
    if (!isConnected()) {
        setError("设备未连接，无法启动轮询");
        return;
    }
    // 重置运动状态判断的中间变量
    m_lastPosition = m_currentStatus.currentDelay;
    m_stableCount = 0;
    m_pollTimer->start(intervalMs);
    qDebug() << "延时线[ID=" << m_deviceId << "] 启动位置轮询, 间隔:" << intervalMs << "ms";
}

void DelayLine::stopPolling()
{
    if (m_pollTimer->isActive()) {
        m_pollTimer->stop();
    }
    if (m_commTimeoutTimer->isActive()) {
        m_commTimeoutTimer->stop();
    }
}

bool DelayLine::isPolling() const
{
    return m_pollTimer->isActive();
}

// ========== 槽函数 ==========

void DelayLine::onPollTimer()
{
    // 每个 tick 发一次位置查询；发出后启动 500ms 超时检测
    if (!isConnected()) {
        stopPolling();
        return;
    }
    if (queryPosition()) {
        // 重启超时计时：若 500ms 内未收到 0xAA 应答则判定通信超时
        m_commTimeoutTimer->start(500);
    }
}

void DelayLine::onCommTimeoutCheck()
{
    // 500ms 未收到位置应答，判定通信超时
    qDebug() << "延时线[ID=" << m_deviceId << "] 通信超时";
    emit commTimeout();
}

void DelayLine::onDataReceived(const QByteArray &data)
{
    qDebug() << "延时线接收数据:" << data.toHex(' ');

    // 累积到接收缓冲区后做帧对齐解析（解决粘包/分包）
    m_recvBuffer.append(data);
    processBuffer();
}

void DelayLine::onSerialConnected()
{
    setStatus(DeviceStatus::Connected);
    emit connected();
    qDebug() << "延时线已连接";
}

void DelayLine::onSerialDisconnected()
{
    setStatus(DeviceStatus::Disconnected);
    emit disconnected();
    qDebug() << "延时线已断开";
}

void DelayLine::onSerialError(const QString &error)
{
    setError(error);
    qDebug() << "延时线错误:" << error;
}

// ========== 协议相关函数 ==========

QByteArray DelayLine::buildFrame(quint8 cmd, const QByteArray &data)
{
    QByteArray frame;
    
    // 起始码
    frame.append(DELAY_FRAME_START);
    
    // 设备ID
    frame.append(m_deviceId);
    
    // 命令
    frame.append(cmd);
    
    // 数据（必须是3字节）
    if (data.size() >= 3) {
        frame.append(data.mid(0, 3));
    } else {
        // 不足3字节，补0
        frame.append(data);
        for (int i = data.size(); i < 3; ++i) {
            frame.append((char)0x00);
        }
    }
    
    // 结束码
    frame.append(DELAY_FRAME_END);
    
    qDebug() << "延时线发送帧:" << frame.toHex(' ');
    
    return frame;
}

bool DelayLine::parseSingleFrame(const QByteArray &frame7)
{
    // 入参为已对齐的 7 字节帧：FC ID CMD D2 D1 D0 FE
    if (frame7.size() != DELAY_FRAME_LENGTH) {
        return false;
    }

    // 检查设备ID（双延时线共串口场景下过滤非本机帧）
    if ((quint8)frame7[1] != m_deviceId) {
        qDebug() << "延时线设备ID不匹配, 期望:" << m_deviceId
                 << "实际:" << (quint8)frame7[1];
        return false;
    }

    // 提取命令和数据
    quint8 cmd = (quint8)frame7[2];
    QByteArray data = frame7.mid(3, 3);

    // 处理位置查询响应（CMD=0xAA）
    if (cmd == DELAY_RESPONSE_POS) {
        // 收到位置应答，取消通信超时计时
        if (m_commTimeoutTimer->isActive()) {
            m_commTimeoutTimer->stop();
        }

        float delay = bytesToDelay(data);
        m_currentStatus.currentDelay = delay;

        // 运动状态自动判断：位置连续多次不变 → 停止，否则 → 运动中
        if (qFuzzyCompare(delay + 1.0f, m_lastPosition + 1.0f)) {
            m_stableCount++;
            if (m_stableCount >= STABLE_THRESHOLD && m_currentStatus.isMoving) {
                m_currentStatus.isMoving = false;
                emit moveCompleted();  // 运动完成
            }
        } else {
            m_stableCount = 0;
            m_currentStatus.isMoving = true;  // 位置变化中 → 运动中
        }
        m_lastPosition = delay;

        qDebug() << "延时线[ID=" << m_deviceId << "] 当前位置:" << delay
                 << "PS, 运动:" << m_currentStatus.isMoving;
        emit positionUpdated(m_deviceId, delay);
        emit delayChanged(delay);  // 兼容旧信号
        return true;
    }

    qDebug() << "延时线收到命令:" << QString::number(cmd, 16) << "数据:" << data.toHex(' ');
    return true;
}

void DelayLine::processBuffer()
{
    // 帧对齐：buffer 长度 ≥ 7 时尝试取帧，校验头尾，失败丢 1 字节重新找 FC
    while (m_recvBuffer.size() >= DELAY_FRAME_LENGTH) {
        // 找到起始码 0xFC
        if ((quint8)m_recvBuffer[0] != DELAY_FRAME_START) {
            m_recvBuffer.remove(0, 1);  // 丢弃 1 字节，重新对齐
            continue;
        }

        // 校验结束码 0xFE（第 7 字节）
        if ((quint8)m_recvBuffer[DELAY_FRAME_LENGTH - 1] != DELAY_FRAME_END) {
            m_recvBuffer.remove(0, 1);  // 帧尾不匹配，丢 1 字节继续找
            continue;
        }

        // 取出完整 7 字节帧并解析
        QByteArray frame = m_recvBuffer.left(DELAY_FRAME_LENGTH);
        if (parseSingleFrame(frame)) {
            emit dataReady(frame);
        }

        // 移除已处理的 7 字节，继续检查后续帧
        m_recvBuffer.remove(0, DELAY_FRAME_LENGTH);
    }

    // 防止异常数据无限堆积：buffer 过大时清空（保留最后 6 字节可能的半帧）
    if (m_recvBuffer.size() > 64) {
        qDebug() << "延时线[ID=" << m_deviceId << "] 接收缓冲异常，清理";
        m_recvBuffer = m_recvBuffer.right(DELAY_FRAME_LENGTH - 1);
    }
}

#include "delay_line.h"
#include <QDebug>
#include <QThread>

DelayLine::DelayLine(QObject *parent)
    : DeviceBase(parent)
    , m_serialPort(new SerialPortBase(this))
    , m_baudRate(9600)
    , m_deviceId(0x01)
{
    setDeviceName("电动光纤延时线");
    
    QObject::connect(m_serialPort, &SerialPortBase::dataReceived,
                     this, &DelayLine::onDataReceived);
    QObject::connect(m_serialPort, &SerialPortBase::connected,
                     this, &DelayLine::onSerialConnected);
    QObject::connect(m_serialPort, &SerialPortBase::disconnected,
                     this, &DelayLine::onSerialDisconnected);
    QObject::connect(m_serialPort, &SerialPortBase::errorOccurred,
                     this, &DelayLine::onSerialError);
}

DelayLine::~DelayLine()
{
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

float DelayLine::getCurrentDelay() const
{
    return m_currentStatus.currentDelay;
}

bool DelayLine::isMoving() const
{
    return m_currentStatus.isMoving;
}

// ========== 槽函数 ==========

void DelayLine::onDataReceived(const QByteArray &data)
{
    qDebug() << "延时线接收数据:" << data.toHex(' ');
    
    // 解析响应帧
    if (parseFrame(data)) {
        emit dataReady(data);
    }
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

bool DelayLine::parseFrame(const QByteArray &frame)
{
    // 最小帧长度：FC + ID + CMD + DATA(3) + FE = 7
    if (frame.size() < 7) {
        qDebug() << "延时线帧长度不足";
        return false;
    }
    
    // 检查起始码和结束码
    if ((quint8)frame[0] != DELAY_FRAME_START || (quint8)frame[frame.size() - 1] != DELAY_FRAME_END) {
        qDebug() << "延时线帧标识错误";
        return false;
    }
    
    // 检查设备ID
    if ((quint8)frame[1] != m_deviceId) {
        qDebug() << "延时线设备ID不匹配";
        return false;
    }
    
    // 提取命令和数据
    quint8 cmd = (quint8)frame[2];
    QByteArray data = frame.mid(3, 3);
    
    // 处理位置查询响应
    if (cmd == DELAY_RESPONSE_POS) {
        float delay = bytesToDelay(data);
        m_currentStatus.currentDelay = delay;
        qDebug() << "延时线当前位置:" << delay << "PS";
        emit delayChanged(delay);
        return true;
    }
    
    qDebug() << "延时线收到命令:" << QString::number(cmd, 16) << "数据:" << data.toHex(' ');
    
    return true;
}

#include "laser_driver.h"
#include <QDebug>
#include <QThread>

LaserDriver::LaserDriver(LaserType type, QObject *parent)
    : DeviceBase(parent)
    , m_serialPort(new SerialPortBase(this))
    , m_baudRate(9600)
    , m_deviceId(0x01)
    , m_laserType(type)
{
    // 根据激光器类型设置设备名称
    switch (type) {
        case LaserType::SeedSource:
            setDeviceName("种子源激光器");
            break;
        case LaserType::FOPO:
            setDeviceName("FOPO激光器");
            break;
        case LaserType::Stokes:
            setDeviceName("Stokes激光器");
            break;
    }
    
    // 连接串口信号
    QObject::connect(m_serialPort, &SerialPortBase::dataReceived,
                     this, &LaserDriver::onDataReceived);
    QObject::connect(m_serialPort, &SerialPortBase::connected,
                     this, &LaserDriver::onSerialConnected);
    QObject::connect(m_serialPort, &SerialPortBase::disconnected,
                     this, &LaserDriver::onSerialDisconnected);
    QObject::connect(m_serialPort, &SerialPortBase::errorOccurred,
                     this, &LaserDriver::onSerialError);
}

LaserDriver::~LaserDriver()
{
    disconnect();
}

bool LaserDriver::connect()
{
    if (m_portName.isEmpty()) {
        setError("串口名称未设置");
        return false;
    }
    
    return openPort(m_portName, m_baudRate);
}

void LaserDriver::disconnect()
{
    if (m_serialPort->isOpen()) {
        m_serialPort->closePort();
        setStatus(DeviceStatus::Disconnected);
    }
}

bool LaserDriver::isConnected() const
{
    return m_serialPort->isOpen();
}

QString LaserDriver::getDeviceInfo() const
{
    return QString("%1 (串口: %2, 波特率: %3, 设备ID: 0x%4)")
           .arg(m_deviceName)
           .arg(m_portName)
           .arg(m_baudRate)
           .arg(m_deviceId, 2, 16, QChar('0'));
}

bool LaserDriver::openPort(const QString &portName, qint32 baudRate,
                          QSerialPort::DataBits dataBits,
                          QSerialPort::Parity parity,
                          QSerialPort::StopBits stopBits)
{
    m_portName = portName;
    m_baudRate = baudRate;
    
    // 尝试打开串口，最多重试3次
    const int MAX_RETRY = 3;
    for (int i = 0; i < MAX_RETRY; i++) {
        if (m_serialPort->openPort(portName, baudRate, dataBits, parity, stopBits)) {
            qDebug() << "激光器连接成功:" << portName;
            return true;
        }
        
        if (i < MAX_RETRY - 1) {
            qDebug() << "激光器连接失败，重试中..." << (i + 1);
            QThread::msleep(1000);  // 等待1秒后重试
        }
    }
    
    setError(QString("激光器连接失败: 无法打开串口 %1").arg(portName));
    return false;
}

void LaserDriver::onDataReceived(const QByteArray &data)
{
    qDebug() << "激光器接收数据:" << data.toHex(' ');
    // TODO: 解析数据，更新状态
    emit dataReady(data);
}

void LaserDriver::onSerialConnected()
{
    setStatus(DeviceStatus::Connected);
    emit connected();
    qDebug() << "激光器已连接";
}

void LaserDriver::onSerialDisconnected()
{
    setStatus(DeviceStatus::Disconnected);
    emit disconnected();
    qDebug() << "激光器已断开";
}

void LaserDriver::onSerialError(const QString &error)
{
    setError(error);
    qDebug() << "激光器错误:" << error;
}

// 协议相关函数实现
QByteArray LaserDriver::buildFrame(quint8 controlCode, const QByteArray &data)
{
    QByteArray frame;
    
    // 起始码
    frame.append(LASER_FRAME_START);
    
    // 设备ID
    frame.append(m_deviceId);
    
    // 控制字
    frame.append(controlCode);
    
    // 数据长度
    frame.append((quint8)data.size());
    
    // 数据域
    frame.append(data);
    
    // 计算校验和（从设备ID到数据域的累加和）
    quint8 checksum = calculateChecksum(frame.mid(1));
    frame.append(checksum);
    
    // 结束码
    frame.append(LASER_FRAME_END);
    
    return frame;
}

bool LaserDriver::parseFrame(const QByteArray &frame, LaserStatus &status)
{
    // 最小帧长度：起始码(1) + 设备ID(1) + 控制字(1) + 长度(1) + 校验和(1) + 结束码(1) = 6
    if (frame.size() < 6) {
        qDebug() << "帧长度不足";
        return false;
    }
    
    // 检查起始码和结束码
    if ((quint8)frame[0] != LASER_FRAME_START || (quint8)frame[frame.size() - 1] != LASER_FRAME_END) {
        qDebug() << "帧标识错误";
        return false;
    }
    
    // 提取数据长度
    quint8 dataLen = (quint8)frame[3];
    
    // 检查帧长度是否匹配
    if (frame.size() != 6 + dataLen) {
        qDebug() << "帧长度不匹配";
        return false;
    }
    
    // 验证校验和
    QByteArray checksumData = frame.mid(1, 3 + dataLen);
    quint8 expectedChecksum = calculateChecksum(checksumData);
    quint8 actualChecksum = (quint8)frame[4 + dataLen];
    
    if (expectedChecksum != actualChecksum) {
        qDebug() << "校验和错误";
        return false;
    }
    
    // 解析数据域（根据实际协议定义）
    // TODO: 根据控制字解析不同的状态数据
    Q_UNUSED(status);
    
    return true;
}

quint8 LaserDriver::calculateChecksum(const QByteArray &data)
{
    quint32 sum = 0;
    for (int i = 0; i < data.size(); ++i) {
        sum += (quint8)data[i];
    }
    return sum & 0xFF;  // 取低8位
}

bool LaserDriver::setCurrent(float current)
{
    if (!isConnected()) {
        setError("激光器未连接");
        return false;
    }
    
    // 根据激光器类型验证电流范围
    QString unit;
    float maxCurrent;
    
    switch (m_laserType) {
        case LaserType::SeedSource:
            unit = "mA";
            maxCurrent = 5000.0f;  // 假设最大5000mA
            break;
        case LaserType::FOPO:
            unit = "A";
            maxCurrent = 50.0f;  // 假设最大50A
            break;
        case LaserType::Stokes:
            unit = "mA";
            maxCurrent = 5000.0f;  // 假设最大5000mA
            break;
        default:
            setError("未知的激光器类型");
            return false;
    }
    
    if (current < 0 || current > maxCurrent) {
        setError(QString("电流值超出范围 (0-%1%2)").arg(maxCurrent).arg(unit));
        return false;
    }
    
    // 构建设置电流命令
    QByteArray data;
    // 将电流值转换为字节（根据实际协议调整）
    // 这里假设使用4字节浮点数
    union {
        float f;
        quint8 bytes[4];
    } currentUnion;
    currentUnion.f = current;
    
    for (int i = 0; i < 4; ++i) {
        data.append(currentUnion.bytes[i]);
    }
    
    QByteArray frame = buildFrame(LASER_CMD_SET_CURRENT, data);
    
    // 发送命令
    if (m_serialPort->writeData(frame)) {
        qDebug() << QString("%1设置电流: %2%3").arg(m_deviceName).arg(current).arg(unit);
        return true;
    } else {
        setError("发送设置电流命令失败");
        return false;
    }
}

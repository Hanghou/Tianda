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
    
    // 解析数据帧
    LaserStatus status;
    if (parseFrame(data, status)) {
        m_currentStatus = status;
        emit statusUpdated(status);
        qDebug() << QString("%1状态更新 - 运行:%2, 电流:%3, 温度:%4℃")
                    .arg(m_deviceName)
                    .arg(status.isRunning ? "是" : "否")
                    .arg(status.setCurrent)
                    .arg(status.setTemperature);
    }
    
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
    
    // 设备ID（4字节，全为0x00）
    frame.append((char)0x00);
    frame.append((char)0x00);
    frame.append((char)0x00);
    frame.append((char)0x00);
    
    // 控制字
    frame.append(controlCode);
    
    // 数据长度（2字节，高字节在前）
    quint16 dataLen = data.size();
    frame.append((char)((dataLen >> 8) & 0xFF));  // 高字节
    frame.append((char)(dataLen & 0xFF));         // 低字节
    
    // 数据域
    frame.append(data);
    
    // 计算校验和（从设备ID到数据域的累加和取反）
    quint8 checksum = calculateChecksum(frame.mid(1));
    frame.append(checksum);
    
    // 结束码
    frame.append(LASER_FRAME_END);
    
    return frame;
}

bool LaserDriver::parseFrame(const QByteArray &frame, LaserStatus &status)
{
    // 最小帧长度：起始码(1) + 设备ID(4) + 控制字(1) + 长度(2) + 校验和(1) + 结束码(1) = 10
    if (frame.size() < 10) {
        qDebug() << "帧长度不足";
        return false;
    }
    
    // 检查起始码和结束码
    if ((quint8)frame[0] != LASER_FRAME_START || (quint8)frame[frame.size() - 1] != LASER_FRAME_END) {
        qDebug() << "帧标识错误";
        return false;
    }
    
    // 提取控制字
    quint8 controlCode = (quint8)frame[5];
    
    // 提取数据长度（2字节，高字节在前）
    quint16 dataLen = ((quint8)frame[6] << 8) | (quint8)frame[7];
    
    // 检查帧长度是否匹配：起始码(1) + 设备ID(4) + 控制字(1) + 长度(2) + 数据域(dataLen) + 校验和(1) + 结束码(1)
    if (frame.size() != 10 + dataLen) {
        qDebug() << "帧长度不匹配，期望:" << (10 + dataLen) << "实际:" << frame.size();
        return false;
    }
    
    // 验证校验和
    QByteArray checksumData = frame.mid(1, 7 + dataLen);  // 从设备ID到数据域
    quint8 expectedChecksum = calculateChecksum(checksumData);
    quint8 actualChecksum = (quint8)frame[8 + dataLen];
    
    if (expectedChecksum != actualChecksum) {
        qDebug() << "校验和错误，期望:" << QString::number(expectedChecksum, 16) 
                 << "实际:" << QString::number(actualChecksum, 16);
        return false;
    }
    
    // 解析数据域（根据控制字）
    if (controlCode == LASER_CMD_QUERY_STATUS && dataLen == 18) {
        // 解析18字节状态数据
        const char* data = frame.constData() + 8;  // 数据域起始位置
        
        // 第1字节：目标检测
        status.targetDetected = (data[0] == 0x01);
        
        // 第2字节：目标温度状态
        quint8 tempStatus = (quint8)data[1];
        status.targetTemperatureOk = (tempStatus == 0x01);
        
        // 第3字节：运行状态
        status.isRunning = (data[2] == 0x01);
        
        // 第4字节：环境温度状态
        status.ambientTemperatureOk = (data[3] == 0x00);
        
        // 第5字节：背光电流单位
        status.currentUnit = (quint8)data[4];
        
        // 第6字节：背光电流清空状态
        status.currentClear = (data[5] == 0x01);
        
        // 第7-8字节：驱动电流设定值（实际值*10）
        quint16 currentRaw = ((quint8)data[6] << 8) | (quint8)data[7];
        status.setCurrent = currentRaw / 10.0f;
        
        // 第9-10字节：最大电流值
        status.maxCurrent = ((quint8)data[8] << 8) | (quint8)data[9];
        
        // 第11-14字节：温度设定值（浮点数）
        union {
            float f;
            quint8 bytes[4];
        } tempUnion;
        tempUnion.bytes[0] = (quint8)data[10];
        tempUnion.bytes[1] = (quint8)data[11];
        tempUnion.bytes[2] = (quint8)data[12];
        tempUnion.bytes[3] = (quint8)data[13];
        status.setTemperature = tempUnion.f;
        
        // 第15-18字节：背光电流（浮点数）
        union {
            float f;
            quint8 bytes[4];
        } currentBgUnion;
        currentBgUnion.bytes[0] = (quint8)data[14];
        currentBgUnion.bytes[1] = (quint8)data[15];
        currentBgUnion.bytes[2] = (quint8)data[16];
        currentBgUnion.bytes[3] = (quint8)data[17];
        status.backgroundCurrent = currentBgUnion.f;
        
        return true;
    }
    
    return true;
}

quint8 LaserDriver::calculateChecksum(const QByteArray &data)
{
    quint8 sum = 0;
    for (int i = 0; i < data.size(); ++i) {
        sum += (quint8)data[i];
    }
    return ~sum;  // 累加和取反
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
    // 数据域：4字节（实际只用2字节，实际值*10）
    QByteArray data;
    quint16 currentValue = (quint16)(current * 10);  // 实际值*10
    data.append((char)((currentValue >> 8) & 0xFF));  // 高字节
    data.append((char)(currentValue & 0xFF));         // 低字节
    data.append((char)0x00);  // 填充字节
    data.append((char)0x00);  // 填充字节
    
    QByteArray frame = buildFrame(LASER_CMD_SET_CURRENT, data);
    
    // 发送命令
    if (m_serialPort->writeData(frame)) {
        qDebug() << QString("%1设置电流: %2%3").arg(m_deviceName).arg(current).arg(unit);
        qDebug() << "发送帧:" << frame.toHex(' ');
        return true;
    } else {
        setError("发送设置电流命令失败");
        return false;
    }
}

bool LaserDriver::setMaxCurrent(float maxCurrent)
{
    if (!isConnected()) {
        setError("激光器未连接");
        return false;
    }
    
    // 构建设置最大电流命令
    // 数据域：4字节（实际只用2字节，注意：这里不需要*10，直接使用实际值）
    QByteArray data;
    quint16 maxCurrentValue = (quint16)maxCurrent;  // 直接使用实际值，不*10
    data.append((char)((maxCurrentValue >> 8) & 0xFF));  // 高字节
    data.append((char)(maxCurrentValue & 0xFF));         // 低字节
    data.append((char)0x00);  // 填充字节
    data.append((char)0x00);  // 填充字节
    
    QByteArray frame = buildFrame(LASER_CMD_SET_MAX_CURRENT, data);
    
    // 发送命令
    if (m_serialPort->writeData(frame)) {
        qDebug() << QString("%1设置最大电流: %2").arg(m_deviceName).arg(maxCurrent);
        qDebug() << "发送帧:" << frame.toHex(' ');
        return true;
    } else {
        setError("发送设置最大电流命令失败");
        return false;
    }
}

bool LaserDriver::setTemperature(float temperature)
{
    if (!isConnected()) {
        setError("激光器未连接");
        return false;
    }
    
    // 构建设置温度命令
    // 数据域：4字节（浮点数）
    QByteArray data;
    union {
        float f;
        quint8 bytes[4];
    } tempUnion;
    tempUnion.f = temperature;
    
    for (int i = 0; i < 4; ++i) {
        data.append(tempUnion.bytes[i]);
    }
    
    QByteArray frame = buildFrame(LASER_CMD_SET_TEMPERATURE, data);
    
    // 发送命令
    if (m_serialPort->writeData(frame)) {
        qDebug() << QString("%1设置温度: %2℃").arg(m_deviceName).arg(temperature);
        qDebug() << "发送帧:" << frame.toHex(' ');
        return true;
    } else {
        setError("发送设置温度命令失败");
        return false;
    }
}

bool LaserDriver::turnOn()
{
    if (!isConnected()) {
        setError("激光器未连接");
        return false;
    }
    
    // 构建开启命令
    // 数据域：4字节（全为0x00）
    QByteArray data;
    data.append((char)0x00);
    data.append((char)0x00);
    data.append((char)0x00);
    data.append((char)0x00);
    
    QByteArray frame = buildFrame(LASER_CMD_TURN_ON, data);
    
    // 发送命令
    if (m_serialPort->writeData(frame)) {
        qDebug() << QString("%1开启激光器").arg(m_deviceName);
        qDebug() << "发送帧:" << frame.toHex(' ');
        return true;
    } else {
        setError("发送开启命令失败");
        return false;
    }
}

bool LaserDriver::turnOff()
{
    if (!isConnected()) {
        setError("激光器未连接");
        return false;
    }
    
    // 构建关闭命令
    // 数据域：4字节（全为0x00）
    QByteArray data;
    data.append((char)0x00);
    data.append((char)0x00);
    data.append((char)0x00);
    data.append((char)0x00);
    
    QByteArray frame = buildFrame(LASER_CMD_TURN_OFF, data);
    
    // 发送命令
    if (m_serialPort->writeData(frame)) {
        qDebug() << QString("%1关闭激光器").arg(m_deviceName);
        qDebug() << "发送帧:" << frame.toHex(' ');
        return true;
    } else {
        setError("发送关闭命令失败");
        return false;
    }
}

bool LaserDriver::queryStatus()
{
    if (!isConnected()) {
        setError("激光器未连接");
        return false;
    }
    
    // 构建查询状态命令
    // 数据域：4字节（全为0x00）
    QByteArray data;
    data.append((char)0x00);
    data.append((char)0x00);
    data.append((char)0x00);
    data.append((char)0x00);
    
    QByteArray frame = buildFrame(LASER_CMD_QUERY_STATUS, data);
    
    // 发送命令
    if (m_serialPort->writeData(frame)) {
        qDebug() << QString("%1查询状态").arg(m_deviceName);
        qDebug() << "发送帧:" << frame.toHex(' ');
        return true;
    } else {
        setError("发送查询状态命令失败");
        return false;
    }
}

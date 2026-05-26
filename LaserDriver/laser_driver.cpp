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
    qDebug() << "激光器接收数据:" << data.toHex(' ').toUpper();
    
    // 将接收到的数据追加到缓冲区
    m_receiveBuffer.append(data);
    
    // 解析数据帧
    if (parseFrame(m_receiveBuffer)) {
        // 解析成功,清空缓冲区
        m_receiveBuffer.clear();
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

// ========== 协议相关函数实现 ==========

/**
 * @brief 构建命令帧
 * @param commandCode 命令字
 * @param data 数据域
 * @return 完整的命令帧
 */
QByteArray LaserDriver::buildFrame(quint8 commandCode, const QByteArray &data)
{
    QByteArray frame;
    
    // 信息头（上位机下发：0xAA 0x55）
    frame.append(LASER_FRAME_START_1);
    frame.append(LASER_FRAME_START_2);
    
    // 命令字
    frame.append(commandCode);
    
    // 数据长度（1字节）
    quint8 dataLen = data.size();
    frame.append(dataLen);
    
    // 数据域
    frame.append(data);
    
    // 计算校验和（信息头+命令字+数据长度+数据，所有字节相加取低16位）
    QByteArray checksumData = frame;  // 包含所有字节
    QByteArray checksum = calculateChecksum(checksumData);
    frame.append(checksum);
    
    return frame;
}

/**
 * @brief 计算校验和
 * @param data 需要计算校验和的数据
 * @return 校验和（2字节，低字节在前，高字节在后）
 */
QByteArray LaserDriver::calculateChecksum(const QByteArray &data)
{
    quint32 sum = 0;
    for (quint8 byte : data) {
        sum += byte;
    }
    
    // 取低16位
    quint16 low16 = sum & 0xFFFF;
    
    // 拆分低字节和高字节
    quint8 lowByte = low16 & 0xFF;
    quint8 highByte = (low16 >> 8) & 0xFF;
    
    QByteArray checksum;
    checksum.append(lowByte);   // 低字节在前
    checksum.append(highByte);  // 高字节在后
    
    return checksum;
}

/**
 * @brief 解析接收到的数据帧
 * @param frame 接收到的数据帧
 * @return 解析成功返回true
 */
bool LaserDriver::parseFrame(const QByteArray &frame)
{
    // 最小帧长度：信息头(2) + 命令字(1) + 数据长度(1) + 数据(至少1) + 校验和(2) = 7
    if (frame.size() < 7) {
        qDebug() << "帧长度不足";
        return false;
    }
    
    // 检查信息头（下位机上传：0x55 0xAA）
    if ((quint8)frame[0] != LASER_FRAME_REPLY_1 || (quint8)frame[1] != LASER_FRAME_REPLY_2) {
        qDebug() << "信息头错误";
        return false;
    }
    
    // 提取命令字
    quint8 commandCode = (quint8)frame[2];
    
    // 提取数据长度
    quint8 dataLen = (quint8)frame[3];
    
    // 检查帧长度是否匹配：信息头(2) + 命令字(1) + 数据长度(1) + 数据(dataLen) + 校验和(2)
    if (frame.size() != 6 + dataLen) {
        qDebug() << "帧长度不匹配，期望:" << (6 + dataLen) << "实际:" << frame.size();
        return false;
    }
    
    // 验证校验和
    QByteArray checksumData = frame.mid(0, 4 + dataLen);  // 信息头+命令字+数据长度+数据
    QByteArray expectedChecksum = calculateChecksum(checksumData);
    quint8 actualChecksumLow = (quint8)frame[4 + dataLen];
    quint8 actualChecksumHigh = (quint8)frame[5 + dataLen];
    
    if ((quint8)expectedChecksum[0] != actualChecksumLow || 
        (quint8)expectedChecksum[1] != actualChecksumHigh) {
        qDebug() << "校验和错误";
        return false;
    }
    
    // 根据命令字解析数据
    switch (commandCode) {
        case LASER_CMD_READ_BASIC_INFO:
            return parseBasicInfo(frame);
            
        case LASER_CMD_SET_POWER:
        case LASER_CMD_LIGHT_CONTROL:
            return parseSetCommandReply(frame, commandCode);
            
        default:
            qDebug() << "未知命令字:" << QString::number(commandCode, 16);
            return false;
    }
}

/**
 * @brief 解析基本信息应答
 * @param frame 应答帧
 * @return 解析成功返回true
 */
bool LaserDriver::parseBasicInfo(const QByteArray &frame)
{
    // 基本信息报文至少需要25字节数据
    quint8 dataLen = (quint8)frame[3];
    if (dataLen < 25) {
        qDebug() << "基本信息数据长度不足";
        return false;
    }
    
    // 提取第25字节（索引24）的功率设置方式
    m_basicInfo.powerSetMode = (quint8)frame[4 + 24];  // 数据从索引4开始
    
    qDebug() << QString("%1基本信息 - 功率设置方式:%2 (%3)")
                .arg(m_deviceName)
                .arg(m_basicInfo.powerSetMode)
                .arg(m_basicInfo.powerSetMode == POWER_SET_MODE_CURRENT ? "设置电流" : "设置功率");
    
    return true;
}

/**
 * @brief 解析设置命令应答
 * @param frame 应答帧
 * @param expectedCmd 期望的命令字
 * @return 解析成功返回true
 */
bool LaserDriver::parseSetCommandReply(const QByteArray &frame, quint8 expectedCmd)
{
    quint8 dataLen = (quint8)frame[3];
    
    // 设置命令应答数据长度为1字节（命令执行说明）
    if (dataLen != 1) {
        qDebug() << "设置命令应答数据长度错误";
        return false;
    }
    
    // 提取命令执行说明
    quint8 execResult = (quint8)frame[4];
    
    // 解析执行结果
    if (execResult == EXEC_SUCCESS) {
        qDebug() << QString("%1命令执行成功 (0x%2)")
                    .arg(m_deviceName)
                    .arg(expectedCmd, 2, 16, QChar('0'));
        
        // 更新状态
        if (expectedCmd == LASER_CMD_LIGHT_CONTROL) {
            // 0xC1命令的状态需要根据发送的数据来判断
            // 这里暂时不更新，等待实时信息报文更新
        }
        
        emit statusUpdated(m_currentStatus);
        return true;
    } else {
        // 解析错误信息
        QStringList errors;
        if (execResult & EXEC_BELOW_MIN) errors << "超过最小值";
        if (execResult & EXEC_ABOVE_MAX) errors << "超过最大值";
        if (execResult & EXEC_INVALID_STATE) errors << "设备当前状态无法执行";
        if (execResult & EXEC_NO_FUNCTION) errors << "设备无此功能";
        if (execResult & EXEC_SAVE_ERROR) errors << "保存数据出错";
        
        QString errorMsg = QString("%1命令执行失败: %2")
                          .arg(m_deviceName)
                          .arg(errors.join(", "));
        qDebug() << errorMsg;
        setError(errorMsg);
        return false;
    }
}

// ========== 命令函数实现 ==========

/**
 * @brief 读取基本信息（0xD1命令）
 * @return 成功返回true
 */
bool LaserDriver::readBasicInfo()
{
    if (!isConnected()) {
        setError("激光器未连接");
        return false;
    }
    
    // 构建读取基本信息命令（无数据）
    QByteArray data;
    QByteArray frame = buildFrame(LASER_CMD_READ_BASIC_INFO, data);
    
    // 发送命令
    if (m_serialPort->writeData(frame)) {
        qDebug() << QString("%1读取基本信息").arg(m_deviceName);
        qDebug() << "发送帧:" << frame.toHex(' ').toUpper();
        return true;
    } else {
        setError("发送读取基本信息命令失败");
        return false;
    }
}

/**
 * @brief 设置泵浦电流（0xC3命令）
 * @param current 电流值（mA）
 * @return 成功返回true
 */
bool LaserDriver::setPumpCurrent(quint16 current)
{
    if (!isConnected()) {
        setError("激光器未连接");
        return false;
    }
    
    // 检查功率设置方式（仅警告，不阻止执行）
    if (m_basicInfo.powerSetMode != 0 && m_basicInfo.powerSetMode != POWER_SET_MODE_CURRENT) {
        qDebug() << QString("%1警告: 功率设置方式为%2，不是电流模式(2)，但仍尝试发送0xC3命令")
                    .arg(m_deviceName)
                    .arg(m_basicInfo.powerSetMode);
    }
    
    // 构建设置电流命令
    // 数据域：2字节（16位电流值，单位mA，低字节在前，高字节在后）
    QByteArray data;
    quint8 lowByte = current & 0xFF;
    quint8 highByte = (current >> 8) & 0xFF;
    data.append(lowByte);   // 低字节在前
    data.append(highByte);  // 高字节在后
    
    QByteArray frame = buildFrame(LASER_CMD_SET_POWER, data);
    
    // 发送命令
    if (m_serialPort->writeData(frame)) {
        qDebug() << QString("%1设置泵浦电流: %2mA").arg(m_deviceName).arg(current);
        qDebug() << "发送帧:" << frame.toHex(' ').toUpper();
        
        // 更新状态
        m_currentStatus.setCurrent = current;
        
        return true;
    } else {
        setError("发送设置泵浦电流命令失败");
        return false;
    }
}

/**
 * @brief 设置电流（兼容旧接口）
 * @param current 电流值（实际值，单位根据激光器类型）
 * @return 成功返回true
 */
bool LaserDriver::setCurrent(float current)
{
    // 转换为mA并调用setPumpCurrent
    quint16 currentMa = (quint16)current;
    
    // 根据激光器类型验证电流范围
    QString unit;
    float maxCurrent;
    
    switch (m_laserType) {
        case LaserType::SeedSource:
            unit = "mA";
            maxCurrent = 5000.0f;
            break;
        case LaserType::FOPO:
            unit = "A";
            maxCurrent = 50.0f;
            // FOPO使用A为单位，需要转换为mA
            currentMa = (quint16)(current * 1000);
            break;
        case LaserType::Stokes:
            unit = "mA";
            maxCurrent = 5000.0f;
            break;
        default:
            setError("未知的激光器类型");
            return false;
    }
    
    if (current < 0 || current > maxCurrent) {
        setError(QString("电流值超出范围 (0-%1%2)").arg(maxCurrent).arg(unit));
        return false;
    }
    
    return setPumpCurrent(currentMa);
}

/**
 * @brief 设置最大电流（已废弃，新协议不支持）
 * @param maxCurrent 最大电流值
 * @return 始终返回false
 */
bool LaserDriver::setMaxCurrent(float maxCurrent)
{
    Q_UNUSED(maxCurrent);
    setError("新协议不支持设置最大电流");
    qDebug() << QString("%1: 新协议不支持设置最大电流").arg(m_deviceName);
    return false;
}

/**
 * @brief 设置温度（已废弃，新协议不支持）
 * @param temperature 温度值
 * @return 始终返回false
 */
bool LaserDriver::setTemperature(float temperature)
{
    Q_UNUSED(temperature);
    setError("新协议不支持设置温度");
    qDebug() << QString("%1: 新协议不支持设置温度").arg(m_deviceName);
    return false;
}

/**
 * @brief 开启激光器（0xC1命令，数据=1）
 * @return 成功返回true
 */
bool LaserDriver::turnOn()
{
    if (!isConnected()) {
        setError("激光器未连接");
        return false;
    }
    
    // 构建开启命令（数据=1表示开启）
    QByteArray data;
    data.append((char)0x01);  // 1=开启
    QByteArray frame = buildFrame(LASER_CMD_LIGHT_CONTROL, data);
    
    // 发送命令
    if (m_serialPort->writeData(frame)) {
        qDebug() << QString("%1开启激光器").arg(m_deviceName);
        qDebug() << "发送帧:" << frame.toHex(' ').toUpper();
        return true;
    } else {
        setError("发送开启命令失败");
        return false;
    }
}

/**
 * @brief 关闭激光器（0xC1命令，数据=0）
 * @return 成功返回true
 */
bool LaserDriver::turnOff()
{
    if (!isConnected()) {
        setError("激光器未连接");
        return false;
    }
    
    // 构建关闭命令（数据=0表示关闭）
    QByteArray data;
    data.append((char)0x00);  // 0=关闭
    QByteArray frame = buildFrame(LASER_CMD_LIGHT_CONTROL, data);
    
    // 发送命令
    if (m_serialPort->writeData(frame)) {
        qDebug() << QString("%1关闭激光器").arg(m_deviceName);
        qDebug() << "发送帧:" << frame.toHex(' ').toUpper();
        return true;
    } else {
        setError("发送关闭命令失败");
        return false;
    }
}

/**
 * @brief 查询状态（已废弃，新协议不支持独立的查询状态命令）
 * @return 始终返回false
 */
bool LaserDriver::queryStatus()
{
    setError("新协议不支持独立的查询状态命令，请使用readBasicInfo()");
    qDebug() << QString("%1: 新协议不支持独立的查询状态命令").arg(m_deviceName);
    return false;
}

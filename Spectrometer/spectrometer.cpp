#include "spectrometer.h"
#include <QDebug>
#include <QThread>

Spectrometer::Spectrometer(QObject *parent)
    : DeviceBase(parent)
    , m_serialPort(new SerialPortBase(this))
    , m_baudRate(115200)            // 默认波特率
    , m_dataBits(QSerialPort::Data8)
    , m_stopBits(QSerialPort::OneStop)
    , m_parity(QSerialPort::NoParity)
    , m_pixelLength(0)
    , m_integrationTime(0)
    , m_maxIntegrationTime(0)
    , m_minIntegrationTime(0)
{
    setDeviceName("奥诺天成光谱仪");
}

Spectrometer::~Spectrometer()
{
    disconnect();
}

bool Spectrometer::connect()
{
    if (m_portName.isEmpty()) {
        setError("串口名称未设置");
        return false;
    }
    
    // 使用用户配置的串口参数
    QSerialPort::DataBits dataBits = static_cast<QSerialPort::DataBits>(m_dataBits);
    QSerialPort::StopBits stopBits = static_cast<QSerialPort::StopBits>(m_stopBits);
    QSerialPort::Parity parity = static_cast<QSerialPort::Parity>(m_parity);
    
    if (!m_serialPort->openPort(m_portName, m_baudRate, dataBits, parity, stopBits)) {
        setError("无法打开串口: " + m_portName);
        return false;
    }
    
    // 等待设备稳定
    QThread::msleep(100);
    
    // 获取设备信息
    m_pixelLength = getPixelLength();
    if (m_pixelLength <= 0) {
        setError("无法获取设备像素数，请检查设备连接");
        m_serialPort->closePort();
        return false;
    }
    
    // 获取积分时间范围
    m_minIntegrationTime = getMinIntegrationTime();
    m_maxIntegrationTime = getMaxIntegrationTime();
    m_integrationTime = getIntegrationTime();
    
    // 获取设备序列号和型号
    m_serialNumber = getSerialNumber();
    m_productNumber = getProductNumber();
    
    setStatus(DeviceStatus::Connected);
    emit connected();
    
    qDebug() << "光谱仪已连接:";
    qDebug() << "  串口:" << m_portName << "波特率:" << m_baudRate;
    qDebug() << "  像素数:" << m_pixelLength;
    qDebug() << "  积分时间范围:" << m_minIntegrationTime << "~" << m_maxIntegrationTime << "us";
    qDebug() << "  序列号:" << m_serialNumber;
    qDebug() << "  型号:" << m_productNumber;
    
    return true;
}

void Spectrometer::disconnect()
{
    if (m_serialPort && m_serialPort->isOpen()) {
        m_serialPort->closePort();
        setStatus(DeviceStatus::Disconnected);
        emit disconnected();
        qDebug() << "光谱仪已断开";
    }
}

bool Spectrometer::isConnected() const
{
    return m_serialPort && m_serialPort->isOpen();
}

QString Spectrometer::getDeviceInfo() const
{
    return QString("%1 (串口: %2, 像素: %3)")
           .arg(m_deviceName)
           .arg(m_portName)
           .arg(m_pixelLength);
}

bool Spectrometer::setIntegrationTime(int timeMicros)
{
    if (!isConnected()) {
        setError("设备未连接");
        return false;
    }
    
    // 构建数据：4字节，大端序
    QByteArray data;
    data.append((timeMicros >> 24) & 0xFF);
    data.append((timeMicros >> 16) & 0xFF);
    data.append((timeMicros >> 8) & 0xFF);
    data.append(timeMicros & 0xFF);
    
    if (!sendCommand(SpectrometerCmd::SET_INTEGRATION_TIME, data)) {
        return false;
    }
    
    SpectrometerFrame response;
    if (!receiveResponse(response, 1000)) {
        setError("设置积分时间超时");
        return false;
    }
    
    // 检查返回值：0x00表示成功
    if (response.data.size() > 0 && (quint8)response.data[0] == 0x00) {
        m_integrationTime = timeMicros;
        qDebug() << "积分时间已设置为:" << timeMicros << "us";
        return true;
    }
    
    setError("设置积分时间失败");
    return false;
}

int Spectrometer::getIntegrationTime()
{
    if (!isConnected()) {
        return -1;
    }
    
    if (!sendCommand(SpectrometerCmd::GET_INTEGRATION_TIME)) {
        return -1;
    }
    
    SpectrometerFrame response;
    if (!receiveResponse(response, 1000)) {
        return -1;
    }
    
    // 返回4字节，大端序
    if (response.data.size() >= 4) {
        int time = ((quint8)response.data[0] << 24) |
                   ((quint8)response.data[1] << 16) |
                   ((quint8)response.data[2] << 8) |
                   (quint8)response.data[3];
        return time;
    }
    
    return -1;
}

int Spectrometer::getMaxIntegrationTime()
{
    if (!isConnected()) {
        return -1;
    }
    
    if (!sendCommand(SpectrometerCmd::GET_MAX_INTEGRATION_TIME)) {
        return -1;
    }
    
    SpectrometerFrame response;
    if (!receiveResponse(response, 1000)) {
        return -1;
    }
    
    // 返回4字节，大端序
    if (response.data.size() >= 4) {
        int time = ((quint8)response.data[0] << 24) |
                   ((quint8)response.data[1] << 16) |
                   ((quint8)response.data[2] << 8) |
                   (quint8)response.data[3];
        return time;
    }
    
    return -1;
}

int Spectrometer::getMinIntegrationTime()
{
    if (!isConnected()) {
        return -1;
    }
    
    if (!sendCommand(SpectrometerCmd::GET_MIN_INTEGRATION_TIME)) {
        return -1;
    }
    
    SpectrometerFrame response;
    if (!receiveResponse(response, 1000)) {
        return -1;
    }
    
    // 返回4字节，大端序
    if (response.data.size() >= 4) {
        int time = ((quint8)response.data[0] << 24) |
                   ((quint8)response.data[1] << 16) |
                   ((quint8)response.data[2] << 8) |
                   (quint8)response.data[3];
        return time;
    }
    
    return -1;
}

bool Spectrometer::startScan()
{
    if (!isConnected()) {
        setError("设备未连接");
        return false;
    }
    
    // 使用命令0x1E进行同步采集（软件触发）
    // 需要先发送积分时间作为参数
    QByteArray data;
    data.append((m_integrationTime >> 24) & 0xFF);
    data.append((m_integrationTime >> 16) & 0xFF);
    data.append((m_integrationTime >> 8) & 0xFF);
    data.append(m_integrationTime & 0xFF);
    
    if (!sendCommand(0x1E, data)) {  // 命令0x1E：同步采集
        return false;
    }
    
    SpectrometerFrame response;
    // 同步采集会直接返回光谱数据，需要更长的超时时间
    int timeout = m_integrationTime / 1000 + 2000;  // 积分时间 + 2秒余量
    if (!receiveResponse(response, timeout)) {
        setError("采集光谱数据超时");
        return false;
    }
    
    // 检查状态位：第一个字节0x00表示成功
    if (response.data.size() > 0 && (quint8)response.data[0] == 0x00) {
        // 解析光谱数据（从第2个字节开始，每个像素2字节，大端序）
        QVector<int> spectrum;
        int pixelCount = (response.data.size() - 1) / 2;
        spectrum.reserve(pixelCount);
        
        for (int i = 0; i < pixelCount; ++i) {
            int value = ((quint8)response.data[i * 2 + 1] << 8) |
                        (quint8)response.data[i * 2 + 2];
            spectrum.append(value);
        }
        
        qDebug() << "光谱采集成功，像素数:" << pixelCount;
        emit spectrumDataReady(spectrum);
        return true;
    }
    
    setError("采集光谱数据失败");
    return false;
}

bool Spectrometer::startContinuousScan(int intervalMicros)
{
    if (!isConnected()) {
        setError("设备未连接");
        return false;
    }
    
    // 构建命令0x19的数据：开始连续采集
    QByteArray data;
    data.append((char)0x00);  // 0x00 = 开始采集
    
    // 采集时间间隔（4字节，大端序，单位：微秒）
    data.append((intervalMicros >> 24) & 0xFF);
    data.append((intervalMicros >> 16) & 0xFF);
    data.append((intervalMicros >> 8) & 0xFF);
    data.append(intervalMicros & 0xFF);
    
    if (!sendCommand(0x19, data)) {
        return false;
    }
    
    SpectrometerFrame response;
    if (!receiveResponse(response, 1000)) {
        setError("启动连续采集超时");
        return false;
    }
    
    // 检查返回值：0x00表示成功
    if (response.data.size() > 0 && (quint8)response.data[0] == 0x00) {
        qDebug() << "连续采集已启动，间隔:" << intervalMicros << "us";
        return true;
    }
    
    setError("启动连续采集失败");
    return false;
}

bool Spectrometer::stopContinuousScan()
{
    if (!isConnected()) {
        setError("设备未连接");
        return false;
    }
    
    // 构建命令0x19的数据：停止连续采集
    QByteArray data;
    data.append((char)0x01);  // 0x01 = 停止采集
    
    // 填充4字节（间隔参数，停止时无意义）
    data.append((char)0x00);
    data.append((char)0x00);
    data.append((char)0x00);
    data.append((char)0x00);
    
    if (!sendCommand(0x19, data)) {
        return false;
    }
    
    SpectrometerFrame response;
    if (!receiveResponse(response, 1000)) {
        setError("停止连续采集超时");
        return false;
    }
    
    // 检查返回值：0x00表示成功
    if (response.data.size() > 0 && (quint8)response.data[0] == 0x00) {
        qDebug() << "连续采集已停止";
        return true;
    }
    
    setError("停止连续采集失败");
    return false;
}

bool Spectrometer::setAverageTimes(int times)
{
    if (!isConnected()) {
        setError("设备未连接");
        return false;
    }
    
    // 检查范围：1-1024次
    if (times < 1 || times > 1024) {
        setError("平均次数应在1-1024范围内");
        return false;
    }
    
    // 构建数据：2字节，大端序
    QByteArray data;
    data.append((times >> 8) & 0xFF);
    data.append(times & 0xFF);
    
    if (!sendCommand(0x28, data)) {  // 命令0x28：设置平均次数
        return false;
    }
    
    SpectrometerFrame response;
    if (!receiveResponse(response, 1000)) {
        setError("设置平均次数超时");
        return false;
    }
    
    // 检查返回值：0x00表示成功
    if (response.data.size() > 0 && (quint8)response.data[0] == 0x00) {
        qDebug() << "平均次数已设置为:" << times;
        return true;
    }
    
    setError("设置平均次数失败");
    return false;
}

bool Spectrometer::getWavelengthCoefficients(QVector<float> &coefficients)
{
    if (!isConnected()) {
        setError("设备未连接");
        return false;
    }
    
    if (!sendCommand(0x55)) {  // 命令0x55：获取波长标定系数
        return false;
    }
    
    SpectrometerFrame response;
    if (!receiveResponse(response, 1000)) {
        setError("获取波长标定系数超时");
        return false;
    }
    
    // 标定系数从第17个字节开始，每4个字节为一个系数（单精度浮点型），共4个系数
    if (response.data.size() >= 32) {
        coefficients.clear();
        for (int i = 0; i < 4; ++i) {
            // 提取4字节浮点数（大端序）
            quint32 bits = ((quint8)response.data[16 + i * 4] << 24) |
                           ((quint8)response.data[17 + i * 4] << 16) |
                           ((quint8)response.data[18 + i * 4] << 8) |
                           (quint8)response.data[19 + i * 4];
            
            float value;
            memcpy(&value, &bits, sizeof(float));
            coefficients.append(value);
        }
        
        qDebug() << "波长标定系数:" << coefficients;
        return true;
    }
    
    setError("波长标定系数数据不完整");
    return false;
}

int Spectrometer::getPixelLength()
{
    if (!isConnected()) {
        return -1;
    }
    
    if (!sendCommand(SpectrometerCmd::GET_PIXEL_LENGTH)) {
        return -1;
    }
    
    SpectrometerFrame response;
    if (!receiveResponse(response, 1000)) {
        return -1;
    }
    
    // 返回2字节，大端序
    if (response.data.size() >= 2) {
        int length = ((quint8)response.data[0] << 8) | (quint8)response.data[1];
        return length;
    }
    
    return -1;
}

QString Spectrometer::getSerialNumber()
{
    if (!isConnected()) {
        return QString();
    }
    
    if (!sendCommand(SpectrometerCmd::GET_SN)) {
        return QString();
    }
    
    SpectrometerFrame response;
    if (!receiveResponse(response, 1000)) {
        return QString();
    }
    
    // 返回字符串
    return QString::fromLatin1(response.data);
}

QString Spectrometer::getProductNumber()
{
    if (!isConnected()) {
        return QString();
    }
    
    if (!sendCommand(SpectrometerCmd::GET_PN)) {
        return QString();
    }
    
    SpectrometerFrame response;
    if (!receiveResponse(response, 1000)) {
        return QString();
    }
    
    // 返回字符串
    return QString::fromLatin1(response.data);
}

bool Spectrometer::sendCommand(quint8 cmd, const QByteArray &data)
{
    if (!m_serialPort || !m_serialPort->isOpen()) {
        setError("串口未打开");
        return false;
    }
    
    // 创建命令帧
    SpectrometerFrame frame = SpectrometerFrame::createCommand(cmd, data);
    QByteArray packet = frame.toByteArray();
    
    // 发送数据
    qint64 written = m_serialPort->writeData(packet);
    if (written != packet.size()) {
        setError("发送命令失败");
        return false;
    }
    
    return true;
}

bool Spectrometer::receiveResponse(SpectrometerFrame &frame, int timeout)
{
    if (!m_serialPort || !m_serialPort->isOpen()) {
        setError("串口未打开");
        return false;
    }
    
    QByteArray buffer;
    QElapsedTimer timer;
    timer.start();
    
    while (timer.elapsed() < timeout) {
        // 等待数据
        QThread::msleep(10);
        
        // 读取数据
        QByteArray newData = m_serialPort->readAll();
        if (!newData.isEmpty()) {
            buffer.append(newData);
            
            // 尝试解析
            if (buffer.size() >= 6) {  // 最小帧长度
                if (SpectrometerFrame::parse(buffer, frame)) {
                    return true;
                }
            }
        }
        
        // 检查是否超时
        if (timer.elapsed() >= timeout) {
            break;
        }
    }
    
    setError("接收响应超时");
    return false;
}

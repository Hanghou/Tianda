#include "stage_controller.h"
#include <QDebug>
#include <QThread>

StageController::StageController(QObject *parent)
    : DeviceBase(parent)
    , m_serialPort(new SerialPortBase(this))
    , m_baudRate(9600)
    , m_address(0)
    , m_currentPosition(0)
{
    setDeviceName("Thorlabs位移台");
    
    QObject::connect(m_serialPort, &SerialPortBase::dataReceived,
                     this, &StageController::onDataReceived);
    QObject::connect(m_serialPort, &SerialPortBase::connected,
                     this, &StageController::onSerialConnected);
    QObject::connect(m_serialPort, &SerialPortBase::disconnected,
                     this, &StageController::onSerialDisconnected);
    QObject::connect(m_serialPort, &SerialPortBase::errorOccurred,
                     this, &StageController::onSerialError);
}

StageController::~StageController()
{
    disconnect();
}

bool StageController::connect()
{
    if (m_portName.isEmpty()) {
        setError("串口名称未设置");
        return false;
    }
    
    return openPort(m_portName, m_baudRate);
}

void StageController::disconnect()
{
    if (m_serialPort->isOpen()) {
        m_serialPort->closePort();
        setStatus(DeviceStatus::Disconnected);
    }
}

bool StageController::isConnected() const
{
    return m_serialPort->isOpen();
}

QString StageController::getDeviceInfo() const
{
    return QString("%1 (串口: %2, 波特率: %3, 地址: %4)")
           .arg(m_deviceName)
           .arg(m_portName)
           .arg(m_baudRate)
           .arg(m_address);
}

bool StageController::openPort(const QString &portName, qint32 baudRate,
                              QSerialPort::DataBits dataBits,
                              QSerialPort::Parity parity,
                              QSerialPort::StopBits stopBits)
{
    m_portName = portName;
    m_baudRate = baudRate;
    
    const int MAX_RETRY = 3;
    for (int i = 0; i < MAX_RETRY; i++) {
        if (m_serialPort->openPort(portName, baudRate, dataBits, parity, stopBits)) {
            qDebug() << "位移台连接成功:" << portName;
            return true;
        }
        
        if (i < MAX_RETRY - 1) {
            qDebug() << "位移台连接失败，重试中..." << (i + 1);
            QThread::msleep(1000);
        }
    }
    
    setError(QString("位移台连接失败: 无法打开串口 %1").arg(portName));
    return false;
}

void StageController::onDataReceived(const QByteArray &data)
{
    qDebug() << "位移台接收数据:" << data.toHex(' ');
    
    // 添加到接收缓冲区
    m_receiveBuffer.append(data);
    
    // 检查是否包含完整的响应（以\r\n结尾）
    if (m_receiveBuffer.contains('\n')) {
        parseResponse(m_receiveBuffer);
        m_receiveBuffer.clear();
    }
    
    emit dataReady(data);
}

void StageController::onSerialConnected()
{
    setStatus(DeviceStatus::Connected);
    emit connected();
    qDebug() << "位移台已连接";
}

void StageController::onSerialDisconnected()
{
    setStatus(DeviceStatus::Disconnected);
    emit disconnected();
    qDebug() << "位移台已断开";
}

void StageController::onSerialError(const QString &error)
{
    setError(error);
    qDebug() << "位移台错误:" << error;
}

QByteArray StageController::buildCommand(const QString &cmd, const QString &param)
{
    // Thorlabs Elliptec协议格式
    // 格式: 地址(0-F) + 命令(2字符) + 参数(可选)
    // 例如: "0ho0" (地址0, home命令, 顺时针)
    
    QString addressStr = QString::number(m_address, 16).toUpper();
    if (m_address > 9) {
        addressStr = QString(QChar('A' + m_address - 10));
    }
    
    QString command = addressStr + cmd;
    if (!param.isEmpty()) {
        command += param;
    }
    
    qDebug() << "发送命令:" << command;
    return command.toLatin1();
}

bool StageController::parseResponse(const QByteArray &response)
{
    // Thorlabs Elliptec响应格式
    // GS: 地址 + "GS" + 状态码(2字符)
    // PO: 地址 + "PO" + 位置(8字符十六进制)
    // IN: 地址 + "IN" + 设备信息
    
    if (response.isEmpty()) {
        return false;
    }
    
    QString resp = QString::fromLatin1(response).trimmed();
    qDebug() << "解析响应:" << resp;
    
    if (resp.length() < 3) {
        return false;
    }
    
    // 提取命令类型（第2-3个字符）
    QString cmdType = resp.mid(1, 2);
    
    if (cmdType == "GS") {
        // 状态响应
        if (resp.length() >= 5) {
            QString statusCode = resp.mid(3, 2);
            qDebug() << "状态码:" << statusCode;
            
            if (statusCode == "00") {
                qDebug() << "设备状态: OK";
                return true;
            } else {
                qDebug() << "设备错误码:" << statusCode;
                setError(QString("设备错误: %1").arg(statusCode));
                return false;
            }
        }
    } else if (cmdType == "PO") {
        // 位置响应
        if (resp.length() >= 11) {
            QString posHex = resp.mid(3, 8);
            bool ok;
            m_currentPosition = posHex.toLong(&ok, 16);
            if (ok) {
                qDebug() << "当前位置(脉冲):" << m_currentPosition;
                emit positionChanged(m_currentPosition);
                emit moveCompleted();
                return true;
            }
        }
    } else if (cmdType == "IN") {
        // 设备信息响应
        qDebug() << "设备信息:" << resp;
        emit deviceInfoReceived(resp);
        return true;
    }
    
    return false;
}

// ========== 位置转换函数 ==========

qint32 StageController::positionToPulses(float position, bool isRotation)
{
    if (isRotation) {
        // 旋转台: ELL14 = 262144 pulses/revolution
        // 角度转脉冲: pulses = (angle / 360.0) * 262144
        return static_cast<qint32>((position / 360.0) * 262144.0);
    } else {
        // 直线台: ELL17/ELL20 = 2048 pulses/mm
        // 距离转脉冲: pulses = distance * 2048
        return static_cast<qint32>(position * 2048.0);
    }
}

float StageController::pulsesToPosition(qint32 pulses, bool isRotation)
{
    if (isRotation) {
        // 脉冲转角度: angle = (pulses / 262144.0) * 360.0
        return (pulses / 262144.0) * 360.0;
    } else {
        // 脉冲转距离: distance = pulses / 2048.0
        return pulses / 2048.0;
    }
}

// ========== 控制命令实现 ==========

bool StageController::home(quint8 direction)
{
    if (!isConnected()) {
        setError("设备未连接");
        return false;
    }
    
    // 构建home命令: 地址 + "ho" + 方向(0=CW, 1=CCW)
    QString dirStr = QString::number(direction);
    QByteArray cmd = buildCommand("ho", dirStr);
    
    if (m_serialPort->writeData(cmd)) {
        qDebug() << "回零命令已发送";
        return true;
    } else {
        setError("发送回零命令失败");
        return false;
    }
}

bool StageController::moveAbsolute(float position, bool isRotation)
{
    if (!isConnected()) {
        setError("设备未连接");
        return false;
    }
    
    // 转换位置为脉冲数
    qint32 pulses = positionToPulses(position, isRotation);
    
    // 构建moveAbsolute命令: 地址 + "ma" + 位置(8位十六进制)
    QString posHex = QString("%1").arg(pulses, 8, 16, QChar('0')).toUpper();
    QByteArray cmd = buildCommand("ma", posHex);
    
    if (m_serialPort->writeData(cmd)) {
        qDebug() << "移动到绝对位置:" << position 
                 << (isRotation ? "度" : "mm")
                 << "(" << pulses << "脉冲)";
        return true;
    } else {
        setError("发送移动命令失败");
        return false;
    }
}

bool StageController::moveRelative(float distance, bool isRotation)
{
    if (!isConnected()) {
        setError("设备未连接");
        return false;
    }
    
    // 转换距离为脉冲数
    qint32 pulses = positionToPulses(distance, isRotation);
    
    // 构建moveRelative命令: 地址 + "mr" + 距离(8位十六进制)
    QString posHex = QString("%1").arg(pulses, 8, 16, QChar('0')).toUpper();
    QByteArray cmd = buildCommand("mr", posHex);
    
    if (m_serialPort->writeData(cmd)) {
        qDebug() << "相对移动:" << distance 
                 << (isRotation ? "度" : "mm")
                 << "(" << pulses << "脉冲)";
        return true;
    } else {
        setError("发送移动命令失败");
        return false;
    }
}

bool StageController::stop()
{
    if (!isConnected()) {
        setError("设备未连接");
        return false;
    }
    
    // 构建stop命令: 地址 + "st"
    QByteArray cmd = buildCommand("st");
    
    if (m_serialPort->writeData(cmd)) {
        qDebug() << "停止命令已发送";
        return true;
    } else {
        setError("发送停止命令失败");
        return false;
    }
}

bool StageController::requestDeviceInfo()
{
    if (!isConnected()) {
        setError("设备未连接");
        return false;
    }
    
    // 构建info命令: 地址 + "in"
    QByteArray cmd = buildCommand("in");
    
    if (m_serialPort->writeData(cmd)) {
        qDebug() << "设备信息请求已发送";
        return true;
    } else {
        setError("发送设备信息请求失败");
        return false;
    }
}

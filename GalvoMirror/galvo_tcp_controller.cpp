#include "galvo_tcp_controller.h"
#include <QDebug>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

GalvoTcpController::GalvoTcpController(QObject *parent)
    : DeviceBase(parent)
    , m_socket(new QTcpSocket(this))
    , m_ipAddress("172.18.34.227")
    , m_port(2000)  // 默认端口，可能需要根据实际情况调整
    , m_isConnected(false)
{
    setDeviceName("大族思特MEMS振镜(TCP)");
    
    // 连接信号
    QObject::connect(m_socket, &QTcpSocket::connected,
                    this, &GalvoTcpController::onConnected);
    QObject::connect(m_socket, &QTcpSocket::disconnected,
                    this, &GalvoTcpController::onDisconnected);
    QObject::connect(m_socket, &QTcpSocket::readyRead,
                    this, &GalvoTcpController::onReadyRead);
    QObject::connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error),
                    this, &GalvoTcpController::onError);
}

GalvoTcpController::~GalvoTcpController()
{
    disconnect();
}

bool GalvoTcpController::connect()
{
    if (m_isConnected) {
        qDebug() << "振镜已经连接";
        return true;
    }
    
    qDebug() << "正在连接振镜控制卡:" << m_ipAddress << ":" << m_port;
    
    setStatus(DeviceStatus::Connecting);
    m_socket->connectToHost(m_ipAddress, m_port);
    
    // 等待连接（最多5秒）
    if (!m_socket->waitForConnected(5000)) {
        setError(QString("连接失败: %1").arg(m_socket->errorString()));
        setStatus(DeviceStatus::Error);
        return false;
    }
    
    return true;  // onConnected会被自动调用
}

void GalvoTcpController::disconnect()
{
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->disconnectFromHost();
        if (m_socket->state() != QAbstractSocket::UnconnectedState) {
            m_socket->waitForDisconnected(1000);
        }
    }
    
    m_isConnected = false;
    setStatus(DeviceStatus::Disconnected);
}

bool GalvoTcpController::isConnected() const
{
    return m_isConnected && m_socket->state() == QAbstractSocket::ConnectedState;
}

QString GalvoTcpController::getDeviceInfo() const
{
    return QString("%1 (IP: %2:%3)")
           .arg(m_deviceName)
           .arg(m_ipAddress)
           .arg(m_port);
}

bool GalvoTcpController::setAngle(float angle, float radius)
{
    if (!m_isConnected) {
        setError("振镜未连接");
        return false;
    }
    
    // 将角度转换为弧度
    float angleRad = angle * M_PI / 180.0f;
    
    // 将极坐标转换为笛卡尔坐标
    float x = radius * std::cos(angleRad);
    float y = radius * std::sin(angleRad);
    
    return scannerJump(x, y);
}

bool GalvoTcpController::moveToZero()
{
    return scannerJump(0.0f, 0.0f);
}

bool GalvoTcpController::scannerJump(float x, float y)
{
    if (!m_isConnected) {
        setError("振镜未连接");
        return false;
    }
    
    // 注意：这里的命令格式是假设的，需要根据实际协议调整
    // 由于厂家没有公开TCP协议细节，这里只是一个框架
    // 实际使用时可能需要抓包分析DLL的通信协议
    
    qDebug() << "振镜跳转到坐标:(" << x << "," << y << ")";
    
    // TODO: 构造实际的命令包
    // 这需要逆向工程或者厂家提供协议文档
    QByteArray command;
    // command = constructJumpCommand(x, y);
    // return sendCommand(command);
    
    // 暂时返回true，表示命令已发送（实际上还未实现）
    qWarning() << "警告：TCP直接通信协议未完全实现，需要厂家协议文档";
    return true;
}

bool GalvoTcpController::sendCommand(const QByteArray &command)
{
    if (!m_isConnected) {
        return false;
    }
    
    qint64 bytesWritten = m_socket->write(command);
    if (bytesWritten == -1) {
        setError(QString("发送命令失败: %1").arg(m_socket->errorString()));
        return false;
    }
    
    m_socket->flush();
    return true;
}

void GalvoTcpController::onConnected()
{
    m_isConnected = true;
    setStatus(DeviceStatus::Connected);
    emit connected();
    qDebug() << "振镜TCP连接成功:" << m_ipAddress << ":" << m_port;
}

void GalvoTcpController::onDisconnected()
{
    m_isConnected = false;
    setStatus(DeviceStatus::Disconnected);
    emit disconnected();
    qDebug() << "振镜TCP连接已断开";
}

void GalvoTcpController::onReadyRead()
{
    QByteArray data = m_socket->readAll();
    qDebug() << "收到振镜数据:" << data.toHex();
    
    // TODO: 解析响应数据
}

void GalvoTcpController::onError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError);
    QString errorMsg = m_socket->errorString();
    setError(errorMsg);
    qWarning() << "振镜TCP错误:" << errorMsg;
}

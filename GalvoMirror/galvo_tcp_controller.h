#ifndef GALVO_TCP_CONTROLLER_H
#define GALVO_TCP_CONTROLLER_H

#include <QObject>
#include <QTcpSocket>
#include <QString>
#include "../Communication/device_base.h"

/**
 * @brief 振镜TCP直接控制类
 * 不使用DLL，直接通过TCP Socket与控制卡通信
 * 这是一个简化版本，只实现基本的连接和角度控制功能
 */
class GalvoTcpController : public DeviceBase
{
    Q_OBJECT

public:
    explicit GalvoTcpController(QObject *parent = nullptr);
    ~GalvoTcpController();

    // 实现基类接口
    bool connect() override;
    void disconnect() override;
    bool isConnected() const override;
    QString getDeviceInfo() const override;
    
    /**
     * @brief 设置IP地址
     * @param ip IP地址（默认172.18.34.227）
     */
    void setIPAddress(const QString &ip) { m_ipAddress = ip; }
    
    /**
     * @brief 设置端口
     * @param port 端口号（默认2000）
     */
    void setPort(quint16 port) { m_port = port; }
    
    /**
     * @brief 设置振镜角度
     * @param angle 角度值（度）
     * @param radius 半径（mm），默认10mm
     * @return 成功返回true
     */
    bool setAngle(float angle, float radius = 10.0f);
    
    /**
     * @brief 振镜回零位
     * @return 成功返回true
     */
    bool moveToZero();
    
    /**
     * @brief 振镜跳转到指定XY坐标
     * @param x X坐标（mm）
     * @param y Y坐标（mm）
     * @return 成功返回true
     */
    bool scannerJump(float x, float y);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onError(QAbstractSocket::SocketError socketError);

private:
    bool sendCommand(const QByteArray &command);
    
private:
    QTcpSocket *m_socket;
    QString m_ipAddress;
    quint16 m_port;
    bool m_isConnected;
};

#endif // GALVO_TCP_CONTROLLER_H

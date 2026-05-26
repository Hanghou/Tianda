#ifndef GALVO_MIRROR_H
#define GALVO_MIRROR_H

#include <QObject>
#include <QString>
#include <QTimer>
#include <QHostAddress>
#include "../Communication/device_base.h"
#include "galvo_protocol_v2.h"

QT_BEGIN_NAMESPACE
class QTcpSocket;
class QUdpSocket;
QT_END_NAMESPACE

/**
 * @brief 振镜控制卡类（按文档协议直连）
 *
 * 通信架构：
 *   UDP:5998 心跳检测（确认控制卡在线）
 *   TCP:6002 图形参数 / 寄存器读写
 *   TCP:6003 控制指令（开始/停止/暂停/继续打标）
 */
class GalvoMirror : public DeviceBase
{
    Q_OBJECT

public:
    explicit GalvoMirror(QObject *parent = nullptr);
    ~GalvoMirror() override;

    // ========== DeviceBase 接口实现 ==========
    bool connect() override;
    void disconnect() override;
    bool isConnected() const override;
    QString getDeviceInfo() const override;

    // ========== 振镜控制卡接口（TCP/UDP 协议） ==========
    bool connectByIP(const QString &ipAddress);   // 连接指定 IP（先心跳，再 TCP）
    QString getDeviceIP() const { return m_cardIP; }
    bool isHeartbeatOk() const { return m_heartbeatOk; }

    // 控制指令（端口 6003）
    bool startMark();           // 发送图形参数 + 开始('e')
    bool stopMark();            // 发送停止('r')
    bool pauseMark();           // 暂停('p')
    bool continueMark();        // 继续('c')

    // 图形参数构造与发送（端口 6002）
    void setLaserPara(const GalvoLaserPara &p)   { m_laserPara = p; }
    void setMarkSetting(const GalvoMarkSetting &m) { m_markSetting = m; }
    void setShape(const GalvoShapeInfo &s)       { m_shape = s; }
    void setIOControl(const GalvoIOControl &io)  { m_ioControl = io; }
    void setPointArray(const GalvoPointPara &pa) { m_pointPara = pa; }
    bool sendShapeFrame();      // 仅发送当前图形参数（不启动打标）

    // 振镜跳转（兼容预设执行业务）：内部转换为单点打标
    bool scannerJump(float x, float y, float z);

    // 寄存器读写（端口 6002）
    bool readRegister(uint32_t addr);
    bool writeRegister(uint32_t addr, int32_t value);

signals:
    void heartbeatChanged(bool online);              // 心跳状态变化
    void actionDataReceived(const QByteArray &data); // 6003 接收数据
    void shapeDataReceived(const QByteArray &data);  // 6002 接收数据
    void messageLog(const QString &msg);             // 文本日志（用于 textReceive）

private slots:
    void onUdpReadyRead();
    void onHeartbeatTimeout();
    void onActionReadyRead();
    void onShapeReadyRead();

private:
    // 网络
    QString      m_cardIP;
    QUdpSocket  *m_udpSocket;
    QTcpSocket  *m_shapeSocket;   // 6002
    QTcpSocket  *m_actionSocket;  // 6003
    QTimer      *m_heartbeatTimer;
    quint32      m_ticks;
    quint32      m_lastTicksWhenSent;
    bool         m_heartbeatOk;

    // 图形参数缓存
    GalvoLaserPara   m_laserPara;
    GalvoMarkSetting m_markSetting;
    GalvoShapeInfo   m_shape;
    GalvoIOControl   m_ioControl;
    GalvoPointPara   m_pointPara;

    // 内部辅助
    void initializeParameters();
    bool sendOnSocket(QTcpSocket *sock, const void *data, int size);
    GalvoMarkParameter buildMarkParameter() const;
};

#endif // GALVO_MIRROR_H

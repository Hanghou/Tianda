#include "galvo_mirror.h"

#include <QTcpSocket>
#include <QUdpSocket>
#include <QNetworkInterface>
#include <QHostAddress>
#include <QDebug>
#include <QThread>

GalvoMirror::GalvoMirror(QObject *parent)
    : DeviceBase(parent)
    , m_cardIP(GalvoProto::kDefaultIP)
    , m_udpSocket(nullptr)
    , m_shapeSocket(nullptr)
    , m_actionSocket(nullptr)
    , m_heartbeatTimer(nullptr)
    , m_ticks(0)
    , m_lastTicksWhenSent(0)
    , m_heartbeatOk(false)
{
    initializeParameters();
}

GalvoMirror::~GalvoMirror()
{
    disconnect();
    if (m_udpSocket) { m_udpSocket->close(); m_udpSocket->deleteLater(); m_udpSocket = nullptr; }
}

// ========== 默认参数初始化 ==========
void GalvoMirror::initializeParameters()
{
    std::memset(&m_laserPara,   0, sizeof(m_laserPara));
    std::memset(&m_markSetting, 0, sizeof(m_markSetting));
    std::memset(&m_shape,       0, sizeof(m_shape));
    std::memset(&m_ioControl,   0, sizeof(m_ioControl));
    std::memset(&m_pointPara,   0, sizeof(m_pointPara));

    m_laserPara.nLaserOnDelay   = 110;
    m_laserPara.nLaserOffDelay  = 120;
    m_laserPara.nFPSDelay       = 10;
    m_laserPara.nFPSLength      = 20;
    m_laserPara.nQDelay         = 5;
    m_laserPara.DutyCycle       = 0.5f;
    m_laserPara.Frequency       = 50.0f;
    m_laserPara.StandbyDutyCycle = 0.2f;
    m_laserPara.StandbyFrequency = 10.0f;
    m_laserPara.nLaserPower     = 50.0f;

    m_markSetting.nMarkV          = 100;
    m_markSetting.nMark2MarkDelay = 0;
    m_markSetting.nJumpDelay      = 0;
    m_markSetting.nMarkDelay      = 0;
    m_markSetting.nJumpV          = 1000;
    m_markSetting.ScanTimes       = 1;

    m_shape.Shape             = static_cast<float>(GALVO_SHAPE_POINT);
    m_shape.PointX            = 0.0f;
    m_shape.PointY            = 0.0f;
    m_shape.Point_LaseronTime = 1000.0f;
    m_shape.Line_StartX       = -10.0f;
    m_shape.Line_StartY       = 10.0f;
    m_shape.Line_EndX         = 10.0f;
    m_shape.Line_EndY         = -10.0f;
    m_shape.CircleX           = 0.0f;
    m_shape.CircleY           = 0.0f;
    m_shape.Circle_Radius     = 5.0f;

    m_ioControl.nRedLightEnable  = 0;
    m_ioControl.nReadyDownEanble = 1;
    m_ioControl.nLightLevel      = 0;
}

// ========== DeviceBase 接口实现 ==========
bool GalvoMirror::connect()
{
    return connectByIP(m_cardIP);
}

void GalvoMirror::disconnect()
{
    if (m_actionSocket) {
        if (m_actionSocket->state() == QAbstractSocket::ConnectedState)
            m_actionSocket->disconnectFromHost();
        m_actionSocket->deleteLater();
        m_actionSocket = nullptr;
    }
    if (m_shapeSocket) {
        if (m_shapeSocket->state() == QAbstractSocket::ConnectedState)
            m_shapeSocket->disconnectFromHost();
        m_shapeSocket->deleteLater();
        m_shapeSocket = nullptr;
    }
    if (m_heartbeatTimer) {
        m_heartbeatTimer->stop();
        m_heartbeatTimer->deleteLater();
        m_heartbeatTimer = nullptr;
    }
    m_heartbeatOk = false;
    setStatus(DeviceStatus::Disconnected);
    emit messageLog(QStringLiteral("已断开振镜控制卡连接"));
}

bool GalvoMirror::isConnected() const
{
    return m_shapeSocket && m_shapeSocket->state() == QAbstractSocket::ConnectedState
        && m_actionSocket && m_actionSocket->state() == QAbstractSocket::ConnectedState;
}

QString GalvoMirror::getDeviceInfo() const
{
    return QString("GalvoMirror @ %1 (TCP %2/%3, UDP %4)")
            .arg(m_cardIP)
            .arg(GalvoProto::kPortShape)
            .arg(GalvoProto::kPortAction)
            .arg(GalvoProto::kPortHeart);
}


// ========== 心跳检测 (UDP 5998) ==========
bool GalvoMirror::connectByIP(const QString &ipAddress)
{
    m_cardIP = ipAddress;
    setStatus(DeviceStatus::Connecting);
    emit messageLog(QStringLiteral("正在连接振镜控制卡 %1 ...").arg(ipAddress));

    // 1. 启动心跳（如果还没启动）
    if (!m_udpSocket) {
        m_udpSocket = new QUdpSocket(this);
        // 绑定本机 172.18.34.x 网段地址到 5998 端口
        const QList<QHostAddress> addrList = QNetworkInterface::allAddresses();
        bool bound = false;
        for (const QHostAddress &addr : addrList) {
            if (addr.toString().startsWith(QStringLiteral("172.18.34"))) {
                if (m_udpSocket->bind(addr, GalvoProto::kPortHeart)) {
                    bound = true;
                    emit messageLog(QStringLiteral("UDP 心跳已绑定 %1:%2")
                                    .arg(addr.toString())
                                    .arg(GalvoProto::kPortHeart));
                    break;
                }
            }
        }
        if (!bound) {
            // 找不到 172.18.34.x 网段时绑定 AnyIPv4，避免完全失败
            m_udpSocket->bind(QHostAddress::AnyIPv4, GalvoProto::kPortHeart);
            emit messageLog(QStringLiteral("UDP 心跳已绑定 AnyIPv4:%1（未找到 172.18.34.x 网段）")
                            .arg(GalvoProto::kPortHeart));
        }
        QObject::connect(m_udpSocket, &QUdpSocket::readyRead,
                         this, &GalvoMirror::onUdpReadyRead);

        m_heartbeatTimer = new QTimer(this);
        QObject::connect(m_heartbeatTimer, &QTimer::timeout,
                         this, &GalvoMirror::onHeartbeatTimeout);
        m_heartbeatTimer->start(1000);
    }

    // 2. 创建 TCP 6002 / 6003 连接
    if (!m_shapeSocket) {
        m_shapeSocket = new QTcpSocket(this);
        QObject::connect(m_shapeSocket, &QTcpSocket::readyRead,
                         this, &GalvoMirror::onShapeReadyRead);
    }
    if (!m_actionSocket) {
        m_actionSocket = new QTcpSocket(this);
        QObject::connect(m_actionSocket, &QTcpSocket::readyRead,
                         this, &GalvoMirror::onActionReadyRead);
    }

    m_shapeSocket->connectToHost(m_cardIP, GalvoProto::kPortShape);
    if (!m_shapeSocket->waitForConnected(3000)) {
        setError(QStringLiteral("6002 端口连接失败：%1").arg(m_shapeSocket->errorString()));
        setStatus(DeviceStatus::Error);
        emit messageLog(QStringLiteral("6002 端口连接失败：%1").arg(m_shapeSocket->errorString()));
        return false;
    }

    m_actionSocket->connectToHost(m_cardIP, GalvoProto::kPortAction);
    if (!m_actionSocket->waitForConnected(3000)) {
        setError(QStringLiteral("6003 端口连接失败：%1").arg(m_actionSocket->errorString()));
        setStatus(DeviceStatus::Error);
        emit messageLog(QStringLiteral("6003 端口连接失败：%1").arg(m_actionSocket->errorString()));
        return false;
    }

    setStatus(DeviceStatus::Connected);
    emit messageLog(QStringLiteral("振镜控制卡连接成功"));
    return true;
}

void GalvoMirror::onUdpReadyRead()
{
    while (m_udpSocket && m_udpSocket->hasPendingDatagrams()) {
        QByteArray buf;
        buf.resize(static_cast<int>(m_udpSocket->pendingDatagramSize()));
        m_udpSocket->readDatagram(buf.data(), buf.size());
        ++m_ticks;
    }
}

void GalvoMirror::onHeartbeatTimeout()
{
    if (!m_udpSocket) return;
    const quint32 before = m_ticks;
    m_lastTicksWhenSent = before;
    // 向控制卡发送任意数据
    m_udpSocket->writeDatagram("0", 1,
                               QHostAddress(m_cardIP),
                               GalvoProto::kPortHeart);
    QThread::msleep(100);
    const bool ok = (m_ticks != before);
    if (ok != m_heartbeatOk) {
        m_heartbeatOk = ok;
        emit heartbeatChanged(ok);
        emit messageLog(ok ? QStringLiteral("心跳检测：在线")
                           : QStringLiteral("心跳检测：离线"));
    }
}

// ========== TCP 接收 ==========
void GalvoMirror::onActionReadyRead()
{
    if (!m_actionSocket) return;
    QByteArray data = m_actionSocket->readAll();
    if (data.isEmpty()) return;
    emit actionDataReceived(data);
    if (data.at(0) == 'r')
        emit messageLog(QStringLiteral("[6003] 开始运行中..."));
    else if (data.at(0) == 'f')
        emit messageLog(QStringLiteral("[6003] 打标完成"));
}

void GalvoMirror::onShapeReadyRead()
{
    if (!m_shapeSocket) return;
    QByteArray data = m_shapeSocket->readAll();
    if (data.isEmpty()) return;
    emit shapeDataReceived(data);
    if (data.at(0) == 'r')
        emit messageLog(QStringLiteral("[6002] 图形参数已接收"));
    else if (data.at(0) == 'f')
        emit messageLog(QStringLiteral("[6002] 图形参数处理完成"));
}


// ========== 通用 TCP 发送 ==========
bool GalvoMirror::sendOnSocket(QTcpSocket *sock, const void *data, int size)
{
    if (!sock || sock->state() != QAbstractSocket::ConnectedState) {
        emit messageLog(QStringLiteral("发送失败：Socket 未连接"));
        return false;
    }
    qint64 sent = sock->write(reinterpret_cast<const char*>(data), size);
    sock->flush();
    return sent == size;
}

// ========== 构造打标参数总帧 ==========
GalvoMarkParameter GalvoMirror::buildMarkParameter() const
{
    GalvoMarkParameter param;
    std::memset(&param, 0, sizeof(param));

    param.m_nCmdType = 0x08;     // 固定：图形参数命令
    param.cSysCmd    = 0x14;     // 固定：系统命令 20
    param.cStatus    = 1;
    param.uReserved  = 0;

    GalvoCommFrame &comm = param.stUnitFrame;
    comm.nHeader   = GalvoProto::kFrameHeader;  // 0x5A5AA5A5
    comm.LaserPara = m_laserPara;
    comm.stmark    = m_markSetting;
    comm.ShpaeInfo = m_shape;
    comm.IOControl = m_ioControl;
    comm.PointPara = m_pointPara;
    return param;
}

// ========== 仅发送图形参数到 6002 ==========
bool GalvoMirror::sendShapeFrame()
{
    if (!isConnected()) {
        emit messageLog(QStringLiteral("发送图形参数失败：未连接"));
        return false;
    }
    GalvoMarkParameter param = buildMarkParameter();
    bool ok = sendOnSocket(m_shapeSocket, &param, sizeof(param));
    if (ok)
        emit messageLog(QStringLiteral("已发送图形参数 (%1 字节)").arg(sizeof(param)));
    else
        emit messageLog(QStringLiteral("图形参数发送失败"));
    return ok;
}

// ========== 控制指令 (TCP 6003) ==========
bool GalvoMirror::startMark()
{
    if (!isConnected()) {
        emit messageLog(QStringLiteral("开始打标失败：未连接"));
        return false;
    }
    // 1. 先发图形参数到 6002
    if (!sendShapeFrame()) return false;

    // 2. 再发开始指令到 6003
    GalvoControlAction action;
    std::memset(&action, 0, sizeof(action));
    action.nDebugType = 114;                 // 固定 'r'
    action.nOperation = GALVO_CMD_START;     // 'e'
    bool ok = sendOnSocket(m_actionSocket, &action, sizeof(action));
    emit messageLog(ok ? QStringLiteral("已发送开始打标指令")
                       : QStringLiteral("开始打标指令发送失败"));
    return ok;
}

bool GalvoMirror::stopMark()
{
    if (!isConnected()) return false;
    GalvoControlAction action;
    std::memset(&action, 0, sizeof(action));
    action.nDebugType = 114;
    action.nOperation = GALVO_CMD_STOP;       // 'r'
    bool ok = sendOnSocket(m_actionSocket, &action, sizeof(action));
    emit messageLog(ok ? QStringLiteral("已发送停止/复位指令")
                       : QStringLiteral("停止指令发送失败"));
    return ok;
}

bool GalvoMirror::pauseMark()
{
    if (!isConnected()) return false;
    GalvoControlAction action;
    std::memset(&action, 0, sizeof(action));
    action.nDebugType = 114;
    action.nOperation = GALVO_CMD_PAUSE;      // 'p'
    bool ok = sendOnSocket(m_actionSocket, &action, sizeof(action));
    emit messageLog(ok ? QStringLiteral("已发送暂停指令")
                       : QStringLiteral("暂停指令发送失败"));
    return ok;
}

bool GalvoMirror::continueMark()
{
    if (!isConnected()) return false;
    GalvoControlAction action;
    std::memset(&action, 0, sizeof(action));
    action.nDebugType = 114;
    action.nOperation = GALVO_CMD_CONTINUE;   // 'c'
    bool ok = sendOnSocket(m_actionSocket, &action, sizeof(action));
    emit messageLog(ok ? QStringLiteral("已发送继续指令")
                       : QStringLiteral("继续指令发送失败"));
    return ok;
}

// ========== 振镜跳转：兼容业务接口（转换为单点打标） ==========
bool GalvoMirror::scannerJump(float x, float y, float /*z*/)
{
    if (!isConnected()) return false;
    // 写入点图形参数
    m_shape.Shape  = static_cast<float>(GALVO_SHAPE_POINT);
    m_shape.PointX = x;
    m_shape.PointY = y;
    if (m_shape.Point_LaseronTime <= 0.0f)
        m_shape.Point_LaseronTime = 1.0f; // 默认极短脉冲，避免长时间出光
    return startMark();
}

// ========== 寄存器读写 (TCP 6002) ==========
bool GalvoMirror::readRegister(uint32_t addr)
{
    if (!isConnected()) return false;
    GalvoRegCommandFrame frame;
    std::memset(&frame, 0, sizeof(frame));
    frame.m_nCmdType = 1;
    frame.m_nCmdCount = 1;
    frame.RegCommand.cCmd      = 0x03; // 读
    frame.RegCommand.cDataType = 0x00; // INT
    frame.RegCommand.cStatus   = 0x01;
    frame.RegCommand.uAddr     = addr;
    frame.RegCommand.udData    = 0;
    return sendOnSocket(m_shapeSocket, &frame, sizeof(frame));
}

bool GalvoMirror::writeRegister(uint32_t addr, int32_t value)
{
    if (!isConnected()) return false;
    GalvoRegCommandFrame frame;
    std::memset(&frame, 0, sizeof(frame));
    frame.m_nCmdType = 1;
    frame.m_nCmdCount = 1;
    frame.RegCommand.cCmd      = 0x06; // 写
    frame.RegCommand.cDataType = 0x00;
    frame.RegCommand.cStatus   = 0x01;
    frame.RegCommand.uAddr     = addr;
    frame.RegCommand.udData    = value;
    return sendOnSocket(m_shapeSocket, &frame, sizeof(frame));
}

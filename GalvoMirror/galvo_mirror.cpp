#include "galvo_mirror.h"
#include <QDebug>
#include <QTimer>
#include <QEventLoop>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

GalvoMirror::GalvoMirror(QObject *parent)
    : DeviceBase(parent)
    , m_dllWrapper(new GalvoDLLWrapper())
    , m_ipAddress("172.18.34.227")
    , m_isConnected(false)
{
    setDeviceName("大族思特MEMS振镜");
}

GalvoMirror::~GalvoMirror()
{
    disconnect();
    delete m_dllWrapper;
}

bool GalvoMirror::connect()
{
    if (m_dllPath.isEmpty()) {
        setError("DLL路径未设置");
        return false;
    }
    
    if (m_ipAddress.isEmpty()) {
        setError("IP地址未设置");
        return false;
    }
    
    qDebug() << "=== 开始连接振镜 ===";
    qDebug() << "DLL路径:" << m_dllPath;
    qDebug() << "IP地址:" << m_ipAddress;
    
    // 加载DLL（如果还没加载）
    if (!m_dllWrapper->isLoaded()) {
        qDebug() << "步骤1: 加载DLL...";
        if (!m_dllWrapper->loadDLL(m_dllPath)) {
            setError(m_dllWrapper->getLastError());
            qDebug() << "加载DLL失败";
            return false;
        }
        qDebug() << "加载DLL成功";
    } else {
        qDebug() << "DLL已加载，跳过加载步骤";
    }
    
    // 初始化控制卡通讯
    qDebug() << "步骤2: 初始化控制卡通讯...";
    if (!m_dllWrapper->initBoard(nullptr)) {
        setError(QString("初始化失败: %1").arg(m_dllWrapper->getLastError()));
        qDebug() << "初始化失败";
        // 不卸载DLL，避免崩溃
        qDebug() << "保持DLL加载状态";
        return false;
    }
    qDebug() << "初始化成功";
    
    // 等待一下让控制卡初始化
    qDebug() << "步骤3: 等待500ms...";
    QEventLoop loop;
    QTimer::singleShot(500, &loop, &QEventLoop::quit);
    loop.exec();
    qDebug() << "等待完成";
    
    // 获取设备数量
    qDebug() << "步骤4: 获取设备数量...";
    int deviceCount = m_dllWrapper->getDeviceCount();
    qDebug() << "检测到" << deviceCount << "个控制卡";
    
    // 如果没有检测到设备，直接返回失败
    if (deviceCount == 0) {
        setError(QString("未检测到控制卡\n\n可能原因:\n1. 控制卡未上电\n2. 网络未配置\n3. IP地址错误\n4. 无法ping通设备"));
        qDebug() << "未检测到设备";
        // 不卸载DLL，避免崩溃
        qDebug() << "保持DLL加载状态，避免崩溃";
        qDebug() << "=== 连接失败（无设备）===";
        return false;
    }
    
    // 连接到指定IP的控制卡
    qDebug() << "步骤5: 连接到指定IP...";
    if (!m_dllWrapper->connectByIpStr(m_ipAddress)) {
        setError(QString("连接失败: %1\n\n可能原因:\n1. 控制卡未上电\n2. 网络未配置\n3. IP地址错误\n4. 无法ping通设备")
                 .arg(m_dllWrapper->getLastError()));
        qDebug() << "连接IP失败";
        // 不卸载DLL，避免崩溃
        qDebug() << "保持DLL加载状态，避免崩溃";
        qDebug() << "=== 连接失败（IP错误）===";
        return false;
    }
    qDebug() << "连接IP成功";
    
    // 验证连接状态
    qDebug() << "步骤6: 验证连接状态...";
    int status = m_dllWrapper->getConnectStatus(0);
    qDebug() << "设备状态:" << status;
    
    if (status != HM_DEV_Connect && status != HM_DEV_Ready) {
        setError(QString("设备状态异常，状态码: %1\n\n0=已连接, 1=就绪, 2=不可用").arg(status));
        qDebug() << "设备状态异常";
        // 不卸载DLL，避免崩溃
        qDebug() << "保持DLL加载状态，避免崩溃";
        qDebug() << "=== 连接失败（状态异常）===";
        return false;
    }
    
    m_isConnected = true;
    setStatus(DeviceStatus::Connected);
    emit connected();
    qDebug() << "=== 振镜连接成功 ===" << "IP:" << m_ipAddress << "状态:" << status;
    
    return true;
}

void GalvoMirror::disconnect()
{
    if (m_isConnected) {
        qDebug() << "=== 开始断开振镜 ===";
        
        // 调用DLL的断开函数
        qDebug() << "调用disconnectDevice...";
        m_dllWrapper->disconnectDevice(0);
        
        qDebug() << "卸载DLL...";
        m_dllWrapper->unloadDLL();
        
        m_isConnected = false;
        setStatus(DeviceStatus::Disconnected);
        emit disconnected();
        qDebug() << "=== 振镜已断开 ===";
    }
}

bool GalvoMirror::isConnected() const
{
    return m_isConnected;
}

QString GalvoMirror::getDeviceInfo() const
{
    return QString("%1 (IP: %2, DLL: %3)")
           .arg(m_deviceName)
           .arg(m_ipAddress)
           .arg(m_dllPath);
}

bool GalvoMirror::setAngle(float angle, float radius)
{
    if (!m_isConnected) {
        setError("振镜未连接");
        return false;
    }
    
    // 将角度转换为弧度
    float angleRad = angle * M_PI / 180.0f;
    
    // 将极坐标转换为笛卡尔坐标
    // X = radius * cos(angle)
    // Y = radius * sin(angle)
    float x = radius * std::cos(angleRad);
    float y = radius * std::sin(angleRad);
    
    qDebug() << "设置振镜角度:" << angle << "度 -> XY坐标:(" << x << "," << y << ")";
    
    // 调用DLL的振镜跳转函数
    if (!m_dllWrapper->scannerJump(0, x, y, 0)) {
        setError(QString("设置角度失败: %1").arg(m_dllWrapper->getLastError()));
        return false;
    }
    
    qDebug() << "振镜角度设置成功";
    return true;
}

bool GalvoMirror::setPosition(float xAngle, float yAngle, float radius)
{
    if (!m_isConnected) {
        setError("振镜未连接");
        return false;
    }
    
    // 将角度转换为弧度
    float xAngleRad = xAngle * M_PI / 180.0f;
    float yAngleRad = yAngle * M_PI / 180.0f;
    
    // 计算XY坐标
    float x = radius * std::cos(xAngleRad);
    float y = radius * std::sin(yAngleRad);
    
    // 调用DLL的振镜跳转函数
    if (!m_dllWrapper->scannerJump(0, x, y, 0)) {
        setError(QString("设置位置失败: %1").arg(m_dllWrapper->getLastError()));
        return false;
    }
    
    return true;
}

bool GalvoMirror::moveToZero()
{
    if (!m_isConnected) {
        setError("振镜未连接");
        return false;
    }
    
    qDebug() << "振镜回零...";
    
    // 调用DLL的振镜跳转函数，回到原点
    if (!m_dllWrapper->scannerJump(0, 0, 0, 0)) {
        setError(QString("回零失败: %1").arg(m_dllWrapper->getLastError()));
        return false;
    }
    
    qDebug() << "振镜回零成功";
    return true;
}

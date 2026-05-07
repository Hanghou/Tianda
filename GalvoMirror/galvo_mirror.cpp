#include "galvo_mirror.h"
#include <QDebug>
#include <QWidget>

// 解决 byte 类型冲突：在包含 windows.h 之前定义
#ifndef BYTE_DEFINED
#define BYTE_DEFINED
typedef unsigned char byte;
#endif

#include <windows.h>  // Windows 头文件以支持 HWND 类型
#include "library/HM_HashuScan.h"

/**
 * @brief 构造函数
 */
GalvoMirror::GalvoMirror(QObject *parent)
    : DeviceBase(parent)
    , m_ipIndex(0)
    , m_ipAddress("172.18.34.227")  // 默认IP地址
    , m_isConnected(false)
    , m_deviceCount(0)
    , m_markProgress(0)
    , m_progressTimer(new QTimer(this))
{
    initializeParameters();
    
    // 设置进度查询定时器
    QObject::connect(m_progressTimer, &QTimer::timeout, this, &GalvoMirror::onProgressTimer);
}

/**
 * @brief 析构函数
 */
GalvoMirror::~GalvoMirror()
{
    disconnect();
}

// ========== 基础设备接口实现 ==========

/**
 * @brief 初始化设备
 */
bool GalvoMirror::initialize()
{
    qDebug() << "GalvoMirror: 初始化振镜控制卡";
    
    // 调用 HM_InitBoard 初始化通信
    // 注意：需要窗口句柄，这里传入 nullptr，DLL 会创建隐藏窗口
    int result = HM_InitBoard(nullptr);
    
    if (result == HM_OK) {
        qDebug() << "GalvoMirror: 初始化成功";
        setStatus(DeviceStatus::Ready);
        return true;
    } else {
        qDebug() << "GalvoMirror: 初始化失败，错误码:" << result;
        setStatus(DeviceStatus::Error);
        setError("初始化失败");
        return false;
    }
}

/**
 * @brief 连接设备
 */
bool GalvoMirror::connect()
{
    qDebug() << "GalvoMirror: 连接振镜控制卡";
    
    // 先搜索设备
    int deviceCount = 0;
    int result = HM_GetDeviceCount(&deviceCount);
    
    if (result != HM_OK || deviceCount == 0) {
        qDebug() << "GalvoMirror: 未找到设备";
        setError("未找到振镜控制卡设备");
        setStatus(DeviceStatus::Error);
        return false;
    }
    
    m_deviceCount = deviceCount;
    qDebug() << "GalvoMirror: 找到" << deviceCount << "个设备";
    
    // 默认连接第一个设备
    if (m_ipIndex < 0 || m_ipIndex >= deviceCount) {
        m_ipIndex = 0;
    }
    
    result = HM_ConnectTo(m_ipIndex);
    
    if (result == HM_OK) {
        m_isConnected = true;
        setStatus(DeviceStatus::Connected);
        emit connected();
        qDebug() << "GalvoMirror: 连接成功，设备索引:" << m_ipIndex;
        return true;
    } else {
        m_isConnected = false;
        setStatus(DeviceStatus::Error);
        setError(QString("连接失败，错误码: %1").arg(result));
        qDebug() << "GalvoMirror: 连接失败，错误码:" << result;
        return false;
    }
}

/**
 * @brief 断开连接
 */
void GalvoMirror::disconnect()
{
    if (!m_isConnected) {
        return;
    }
    
    qDebug() << "GalvoMirror: 断开振镜控制卡连接";
    
    // 停止进度定时器
    if (m_progressTimer->isActive()) {
        m_progressTimer->stop();
    }
    
    // 调用 HM_DisconnectTo 断开连接
    int result = HM_DisconnectTo(m_ipIndex);
    
    if (result == HM_OK) {
        qDebug() << "GalvoMirror: 断开连接成功";
    } else {
        qDebug() << "GalvoMirror: 断开连接失败，错误码:" << result;
    }
    
    m_isConnected = false;
    setStatus(DeviceStatus::Disconnected);
    emit disconnected();
}

/**
 * @brief 检查是否已连接
 */
bool GalvoMirror::isConnected() const
{
    return m_isConnected;
}

/**
 * @brief 获取设备信息
 */
QString GalvoMirror::getDeviceInfo() const
{
    return QString("思特GMC振镜控制卡 - IP: %1, 索引: %2")
        .arg(m_ipAddress)
        .arg(m_ipIndex);
}

// ========== 设备连接与管理 ==========

/**
 * @brief 搜索控制卡设备
 */
bool GalvoMirror::searchDevices()
{
    qDebug() << "GalvoMirror: 搜索控制卡设备";
    
    // 调用 HM_GetDeviceCount 获取设备数量
    int deviceCount = 0;
    int result = HM_GetDeviceCount(&deviceCount);
    
    if (result == HM_OK) {
        m_deviceCount = deviceCount;
        qDebug() << "GalvoMirror: 找到" << deviceCount << "个设备";
        return true;
    } else {
        qDebug() << "GalvoMirror: 搜索设备失败，错误码:" << result;
        setError(QString("搜索设备失败，错误码: %1").arg(result));
        return false;
    }
}

/**
 * @brief 获取设备数量
 */
int GalvoMirror::getDeviceCount() const
{
    // TODO: 返回实际设备数量
    return m_deviceCount;
}

/**
 * @brief 通过IP地址连接
 */
bool GalvoMirror::connectByIP(const QString& ipAddress)
{
    qDebug() << "GalvoMirror: 通过IP连接:" << ipAddress;
    
    m_ipAddress = ipAddress;
    
    // 将QString转换为char*
    QByteArray ipBytes = ipAddress.toLocal8Bit();
    char* ipStr = ipBytes.data();
    
    // 调用 HM_ConnectByIpStr
    int result = HM_ConnectByIpStr(ipStr);
    
    if (result == HM_OK) {
        m_isConnected = true;
        setStatus(DeviceStatus::Connected);
        emit connected();
        qDebug() << "GalvoMirror: 通过IP连接成功";
        return true;
    } else {
        m_isConnected = false;
        setStatus(DeviceStatus::Error);
        setError(QString("通过IP连接失败，错误码: %1").arg(result));
        qDebug() << "GalvoMirror: 通过IP连接失败，错误码:" << result;
        return false;
    }
}

/**
 * @brief 通过索引连接
 */
bool GalvoMirror::connectByIndex(int ipIndex)
{
    qDebug() << "GalvoMirror: 通过索引连接:" << ipIndex;
    
    m_ipIndex = ipIndex;
    
    // TODO: 调用 HM_ConnectTo
    
    return true;
}

/**
 * @brief 获取连接状态
 */
int GalvoMirror::getConnectionStatus() const
{
    // 调用 HM_GetConnectStatus
    int status = HM_GetConnectStatus(m_ipIndex);
    return status;
}

/**
 * @brief 获取设备IP地址
 */
QString GalvoMirror::getDeviceIP() const
{
    return m_ipAddress;
}

// ========== 打标文件管理 ==========

/**
 * @brief 下载打标文件（异步）
 */
bool GalvoMirror::downloadMarkFile(const QString& filePath)
{
    qDebug() << "GalvoMirror: 下载打标文件:" << filePath;
    
    // TODO: 调用 HM_DownloadMarkFile
    
    return true;
}

/**
 * @brief 下载打标文件（同步）
 */
bool GalvoMirror::downloadMarkFileSync(const QString& filePath)
{
    qDebug() << "GalvoMirror: 同步下载打标文件:" << filePath;
    
    // TODO: 调用 HM_DownloadMarkFileSyn
    
    emit downloadFinished();
    return true;
}

/**
 * @brief 开始打标
 */
bool GalvoMirror::startMark()
{
    qDebug() << "GalvoMirror: 开始打标";
    
    // TODO: 调用 HM_StartMark
    
    // 启动进度查询定时器
    m_progressTimer->start(100);  // 每100ms查询一次
    
    return true;
}

/**
 * @brief 停止打标
 */
bool GalvoMirror::stopMark()
{
    qDebug() << "GalvoMirror: 停止打标";
    
    // TODO: 调用 HM_StopMark
    
    m_progressTimer->stop();
    
    return true;
}

/**
 * @brief 暂停打标
 */
bool GalvoMirror::pauseMark()
{
    qDebug() << "GalvoMirror: 暂停打标";
    
    // TODO: 调用 HM_PauseMark
    
    return true;
}

/**
 * @brief 继续打标
 */
bool GalvoMirror::continueMark()
{
    qDebug() << "GalvoMirror: 继续打标";
    
    // TODO: 调用 HM_ContinueMark
    
    return true;
}

/**
 * @brief 获取打标进度
 */
int GalvoMirror::getMarkProgress() const
{
    return m_markProgress;
}

// ========== 参数设置 ==========

/**
 * @brief 设置打标偏移
 */
bool GalvoMirror::setOffset(float offsetX, float offsetY, float offsetZ)
{
    qDebug() << "GalvoMirror: 设置偏移:" << offsetX << offsetY << offsetZ;
    
    if (!m_isConnected) {
        qDebug() << "GalvoMirror: 设备未连接";
        setError("设备未连接");
        return false;
    }
    
    // 调用 HM_SetOffset
    int result = HM_SetOffset(m_ipIndex, offsetX, offsetY, offsetZ);
    
    if (result == HM_OK) {
        qDebug() << "GalvoMirror: 设置偏移成功";
        return true;
    } else {
        qDebug() << "GalvoMirror: 设置偏移失败，错误码:" << result;
        setError(QString("设置偏移失败，错误码: %1").arg(result));
        return false;
    }
}

/**
 * @brief 设置旋转
 */
bool GalvoMirror::setRotation(float angle, float centerX, float centerY)
{
    qDebug() << "GalvoMirror: 设置旋转:" << angle << centerX << centerY;
    
    // TODO: 调用 HM_SetRotates
    
    return true;
}

/**
 * @brief 设置坐标系
 */
bool GalvoMirror::setCoordinate(int coordinate)
{
    qDebug() << "GalvoMirror: 设置坐标系:" << coordinate;
    
    // TODO: 调用 HM_SetCoordinate
    
    return true;
}

/**
 * @brief 设置打标范围
 */
bool GalvoMirror::setMarkRegion(int region)
{
    qDebug() << "GalvoMirror: 设置打标范围:" << region;
    
    // TODO: 调用 HM_SetMarkRegion
    
    return true;
}

/**
 * @brief 获取打标范围
 */
int GalvoMirror::getMarkRegion() const
{
    // TODO: 调用 HM_GetMarkRegion
    return 0;
}

// ========== 振镜控制 ==========

/**
 * @brief 振镜跳转到指定位置
 */
bool GalvoMirror::scannerJump(float x, float y, float z)
{
    qDebug() << "GalvoMirror: 振镜跳转:" << x << y << z;
    
    if (!m_isConnected) {
        qDebug() << "GalvoMirror: 设备未连接";
        setError("设备未连接");
        return false;
    }
    
    // 调用 HM_ScannerJump
    int result = HM_ScannerJump(m_ipIndex, x, y, z);
    
    if (result == HM_OK) {
        qDebug() << "GalvoMirror: 振镜跳转成功";
        return true;
    } else {
        qDebug() << "GalvoMirror: 振镜跳转失败，错误码:" << result;
        setError(QString("振镜跳转失败，错误码: %1").arg(result));
        return false;
    }
}

/**
 * @brief 设置红光预览
 */
bool GalvoMirror::setGuideLaser(bool enable)
{
    qDebug() << "GalvoMirror: 红光预览:" << enable;
    
    if (!m_isConnected) {
        qDebug() << "GalvoMirror: 设备未连接";
        setError("设备未连接");
        return false;
    }
    
    // 调用 HM_SetGuidLaser
    int result = HM_SetGuidLaser(m_ipIndex, enable);
    
    if (result == HM_OK) {
        qDebug() << "GalvoMirror: 红光预览设置成功";
        return true;
    } else {
        qDebug() << "GalvoMirror: 红光预览设置失败，错误码:" << result;
        setError(QString("红光预览设置失败，错误码: %1").arg(result));
        return false;
    }
}

// ========== 校正表管理 ==========

/**
 * @brief 下载校正表
 */
bool GalvoMirror::downloadCorrection(const QString& filePath)
{
    qDebug() << "GalvoMirror: 下载校正表:" << filePath;
    
    // TODO: 调用 HM_DownloadCorrection
    
    return true;
}

/**
 * @brief 固化校正表
 */
bool GalvoMirror::burnCorrection(const QString& filePath)
{
    qDebug() << "GalvoMirror: 固化校正表:" << filePath;
    
    // TODO: 调用 HM_BurnCorrection
    
    return true;
}

/**
 * @brief 选择校正表
 */
bool GalvoMirror::selectCorrection(int crtIndex)
{
    qDebug() << "GalvoMirror: 选择校正表:" << crtIndex;
    
    // TODO: 调用 HM_SelectCorrection
    
    return true;
}

// ========== IO控制 ==========

/**
 * @brief 设置输出高电平
 */
bool GalvoMirror::setOutputOn(int outIndex)
{
    qDebug() << "GalvoMirror: 设置输出" << outIndex << "为高电平";
    
    // TODO: 调用 HM_SetOutputOn_GMC4
    
    return true;
}

/**
 * @brief 设置输出低电平
 */
bool GalvoMirror::setOutputOff(int outIndex)
{
    qDebug() << "GalvoMirror: 设置输出" << outIndex << "为低电平";
    
    // TODO: 调用 HM_SetOutputOff_GMC4
    
    return true;
}

/**
 * @brief 获取输入状态
 */
int GalvoMirror::getInputStatus() const
{
    // TODO: 调用 HM_GetInput_GMC4
    return 0;
}

/**
 * @brief 获取激光器报警状态
 */
int GalvoMirror::getLaserInputStatus() const
{
    // TODO: 调用 HM_GetLaserInput
    return 0;
}

/**
 * @brief 设置模拟量输出
 */
bool GalvoMirror::setAnalog(float voutA, float voutB)
{
    qDebug() << "GalvoMirror: 设置模拟量:" << voutA << voutB;
    
    // TODO: 调用 HM_SetAnalog
    
    return true;
}

// ========== 脱机打标 ==========

/**
 * @brief 设置脱机模式
 */
bool GalvoMirror::setBurnMode(int mode)
{
    qDebug() << "GalvoMirror: 设置脱机模式:" << mode;
    
    // TODO: 调用 HM_SetBurnMode
    
    return true;
}

/**
 * @brief 设置脱机文档索引
 */
bool GalvoMirror::setBurnIndex(int udmIndex)
{
    qDebug() << "GalvoMirror: 设置脱机文档索引:" << udmIndex;
    
    // TODO: 调用 HM_SetBurnIndex
    
    return true;
}

/**
 * @brief 设置开始脱机标志
 */
bool GalvoMirror::setStartBurnFlag()
{
    qDebug() << "GalvoMirror: 设置开始脱机标志";
    
    // TODO: 调用 HM_SetStartBurnFlag
    
    return true;
}

/**
 * @brief 固化/清除脱机文件
 */
bool GalvoMirror::burnMarkFile(bool enable)
{
    qDebug() << "GalvoMirror: 固化脱机文件:" << enable;
    
    // TODO: 调用 HM_BurnMarkFile
    
    return true;
}

/**
 * @brief 判断是否固化完成
 */
bool GalvoMirror::getBurnOverFlag() const
{
    // TODO: 调用 HM_GetBurnOverFlag
    return true;
}

/**
 * @brief 获取脱机文档个数
 */
int GalvoMirror::getBurnFileNum() const
{
    // TODO: 调用 HM_GetBurnFileNum
    return 0;
}

/**
 * @brief 判断是否有SD卡
 */
bool GalvoMirror::hasSDCard() const
{
    // TODO: 调用 HM_GetSDCardFlag
    return false;
}

// ========== 状态查询 ==========

/**
 * @brief 获取工作状态
 */
int GalvoMirror::getWorkStatus() const
{
    if (!m_isConnected) {
        return 0;
    }
    
    // 调用 HM_GetWorkStatus
    // 返回值：1=ready, 2=run, 3=alarm
    int status = HM_GetWorkStatus(m_ipIndex);
    return status;
}

/**
 * @brief 获取XY位置反馈
 */
bool GalvoMirror::getFeedbackPosXY(short* fbX, short* fbY)
{
    // TODO: 调用 HM_GetFeedbackPosXY
    return true;
}

/**
 * @brief 获取XY位置指令
 */
bool GalvoMirror::getCmdPosXY(short* cmdX, short* cmdY)
{
    // TODO: 调用 HM_GetCmdPosXY
    return true;
}

/**
 * @brief 获取超范围报警
 */
bool GalvoMirror::getOverrangeInfo() const
{
    // TODO: 调用 HM_GetOverangeInfo
    return false;
}

/**
 * @brief 清除闭环报警
 */
bool GalvoMirror::clearCloseLoopAlarm()
{
    qDebug() << "GalvoMirror: 清除闭环报警";
    
    // TODO: 调用 HM_ClearCloseLoopAlarm
    
    return true;
}

/**
 * @brief 获取振镜状态信息
 */
int GalvoMirror::getGalvoStatusInfo(int galvoType)
{
    // TODO: 调用 HM_GetGalvoStatusInfo
    return 0;
}

// ========== 私有槽函数 ==========

/**
 * @brief 进度查询定时器槽函数
 */
void GalvoMirror::onProgressTimer()
{
    // TODO: 调用 HM_ExecuteProgress 查询打标进度
    // 进度会通过消息回调返回
}

// ========== 私有辅助方法 ==========

/**
 * @brief 初始化参数
 */
void GalvoMirror::initializeParameters()
{
    m_ipIndex = 0;
    m_ipAddress = "172.18.34.227";  // 默认IP
    m_isConnected = false;
    m_deviceCount = 0;
    m_markProgress = 0;
}

/**
 * @brief 加载DLL库
 */
bool GalvoMirror::loadLibraries()
{
    // TODO: 动态加载 HM_Comm.dll 和 HM_HashuScan.dll
    return true;
}

/**
 * @brief 设置消息处理
 */
void GalvoMirror::setupMessageHandling()
{
    // TODO: 在Qt中处理Windows消息
    // 需要特殊处理窗口句柄和消息循环
}

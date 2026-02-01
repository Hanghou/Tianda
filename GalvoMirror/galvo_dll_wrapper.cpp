#include "galvo_dll_wrapper.h"
#include <QtCore/QLibrary>
#include <QtCore/QDebug>

GalvoDLLWrapper::GalvoDLLWrapper()
    : m_library(nullptr)
    , m_initBoard(nullptr)
    , m_getDeviceCount(nullptr)
    , m_connectByIpStr(nullptr)
    , m_disconnectTo(nullptr)
    , m_getConnectStatus(nullptr)
    , m_scannerJump(nullptr)
{
}

GalvoDLLWrapper::~GalvoDLLWrapper()
{
    unloadDLL();
}

bool GalvoDLLWrapper::loadDLL(const QString &dllPath)
{
    if (m_library) {
        unloadDLL();
    }
    
    m_library = new QLibrary(dllPath);
    
    if (!m_library->load()) {
        m_lastError = QString("无法加载DLL: %1").arg(m_library->errorString());
        qDebug() << m_lastError;
        delete m_library;
        m_library = nullptr;
        return false;
    }
    
    if (!resolveFunctions()) {
        m_lastError = "无法解析DLL函数";
        unloadDLL();
        return false;
    }
    
    qDebug() << "振镜DLL加载成功:" << dllPath;
    return true;
}

void GalvoDLLWrapper::unloadDLL()
{
    if (m_library) {
        m_library->unload();
        delete m_library;
        m_library = nullptr;
    }
    
    // 清空函数指针
    m_initBoard = nullptr;
    m_getDeviceCount = nullptr;
    m_connectByIpStr = nullptr;
    m_disconnectTo = nullptr;
    m_getConnectStatus = nullptr;
    m_scannerJump = nullptr;
}

bool GalvoDLLWrapper::isLoaded() const
{
    return m_library && m_library->isLoaded();
}

bool GalvoDLLWrapper::resolveFunctions()
{
    if (!m_library) {
        m_lastError = "DLL库未加载";
        return false;
    }
    
    qDebug() << "=== 开始解析 DLL 函数 ===";
    
    // 解析DLL函数地址
    m_initBoard = (HM_InitBoardFunc)m_library->resolve("HM_InitBoard");
    if (!m_initBoard) {
        qDebug() << "警告：无法解析HM_InitBoard函数";
    } else {
        qDebug() << "✓ HM_InitBoard 解析成功";
    }
    
    m_getDeviceCount = (HM_GetDeviceCountFunc)m_library->resolve("HM_GetDeviceCount");
    if (!m_getDeviceCount) {
        qDebug() << "警告：无法解析HM_GetDeviceCount函数";
    } else {
        qDebug() << "✓ HM_GetDeviceCount 解析成功";
    }
    
    m_connectByIpStr = (HM_ConnectByIpStrFunc)m_library->resolve("HM_ConnectByIpStr");
    if (!m_connectByIpStr) {
        qDebug() << "错误：无法解析HM_ConnectByIpStr函数";
        // 不立即返回，继续尝试其他函数
    } else {
        qDebug() << "✓ HM_ConnectByIpStr 解析成功";
    }
    
    m_disconnectTo = (HM_DisconnectToFunc)m_library->resolve("HM_DisconnectTo");
    if (!m_disconnectTo) {
        qDebug() << "警告：无法解析HM_DisconnectTo函数";
    } else {
        qDebug() << "✓ HM_DisconnectTo 解析成功";
    }
    
    m_getConnectStatus = (HM_GetConnectStatusFunc)m_library->resolve("HM_GetConnectStatus");
    if (!m_getConnectStatus) {
        qDebug() << "错误：无法解析HM_GetConnectStatus函数";
        // 不立即返回，继续尝试其他函数
    } else {
        qDebug() << "✓ HM_GetConnectStatus 解析成功";
    }
    
    m_scannerJump = (HM_ScannerJumpFunc)m_library->resolve("HM_ScannerJump");
    if (!m_scannerJump) {
        qDebug() << "警告：无法解析HM_ScannerJump函数";
    } else {
        qDebug() << "✓ HM_ScannerJump 解析成功";
    }
    
    // 检查关键函数是否都解析成功
    if (!m_connectByIpStr && !m_getConnectStatus) {
        m_lastError = "无法解析关键DLL函数（HM_ConnectByIpStr 和 HM_GetConnectStatus）\n\n";
        m_lastError += "这可能是因为：\n";
        m_lastError += "1. DLL 版本不正确（需要厂家提供的正确版本）\n";
        m_lastError += "2. DLL 文件损坏\n";
        m_lastError += "3. 函数名称已更改（需要查看 DLL 文档）\n\n";
        m_lastError += "建议：\n";
        m_lastError += "• 联系厂家获取正确版本的 DLL\n";
        m_lastError += "• 或使用 TCP 方式连接";
        qDebug() << "=== DLL 函数解析失败 ===";
        qDebug() << m_lastError;
        return false;
    }
    
    qDebug() << "=== DLL 关键函数解析成功 ===";
    return true;
}

QString GalvoDLLWrapper::getLastError() const
{
    return m_lastError;
}

bool GalvoDLLWrapper::initBoard(HWND hWnd)
{
    if (!m_initBoard) {
        m_lastError = "HM_InitBoard函数未加载";
        return false;
    }
    
    int result = m_initBoard(hWnd);
    if (result != HM_OK) {
        m_lastError = QString("初始化控制卡失败，错误码: %1").arg(result);
        qDebug() << m_lastError;
        return false;
    }
    
    qDebug() << "控制卡通讯初始化成功";
    return true;
}

int GalvoDLLWrapper::getDeviceCount()
{
    if (!m_getDeviceCount) {
        m_lastError = "HM_GetDeviceCount函数未加载";
        return 0;
    }
    
    int count = 0;
    int result = m_getDeviceCount(&count);
    if (result != HM_OK) {
        m_lastError = QString("获取设备数量失败，错误码: %1").arg(result);
        qDebug() << m_lastError;
        return 0;
    }
    
    qDebug() << "找到" << count << "个控制卡";
    return count;
}

bool GalvoDLLWrapper::connectByIpStr(const QString &ipAddress)
{
    if (!m_connectByIpStr) {
        m_lastError = "HM_ConnectByIpStr函数未加载";
        qDebug() << m_lastError;
        return false;
    }
    
    if (!isLoaded()) {
        m_lastError = "DLL未加载";
        qDebug() << m_lastError;
        return false;
    }
    
    // 转换QString到char*
    QByteArray ipBytes = ipAddress.toLatin1();
    char* ipStr = ipBytes.data();
    
    qDebug() << "正在连接控制卡:" << ipAddress;
    
    int result = m_connectByIpStr(ipStr);
    if (result != HM_OK) {
        m_lastError = QString("连接控制卡失败，错误码: %1").arg(result);
        qDebug() << m_lastError;
        return false;
    }
    
    qDebug() << "控制卡连接成功:" << ipAddress;
    return true;
}

bool GalvoDLLWrapper::disconnectDevice(int ipIndex)
{
    if (!m_disconnectTo) {
        m_lastError = "HM_DisconnectTo函数未加载";
        qDebug() << m_lastError;
        return false;
    }
    
    if (!isLoaded()) {
        qDebug() << "DLL未加载，跳过断开连接";
        return true;  // DLL都没加载，不算错误
    }
    
    qDebug() << "正在断开控制卡连接...";
    
    int result = m_disconnectTo(ipIndex);
    if (result != HM_OK) {
        m_lastError = QString("断开连接失败，错误码: %1").arg(result);
        qDebug() << m_lastError;
        return false;
    }
    
    qDebug() << "控制卡已断开";
    return true;
}

int GalvoDLLWrapper::getConnectStatus(int ipIndex)
{
    if (!m_getConnectStatus) {
        m_lastError = "HM_GetConnectStatus函数未加载";
        qDebug() << m_lastError;
        return HM_DEV_NotAvailable;
    }
    
    if (!isLoaded()) {
        qDebug() << "DLL未加载，返回不可用状态";
        return HM_DEV_NotAvailable;
    }
    
    int status = m_getConnectStatus(ipIndex);
    qDebug() << "控制卡状态:" << status << "(0=已连接, 1=就绪, 2=不可用)";
    return status;
}

bool GalvoDLLWrapper::scannerJump(int ipIndex, float x, float y, float z)
{
    if (!m_scannerJump) {
        m_lastError = "HM_ScannerJump函数未加载";
        return false;
    }
    
    int result = m_scannerJump(ipIndex, x, y, z);
    if (result != HM_OK) {
        m_lastError = QString("振镜跳转失败，错误码: %1").arg(result);
        qDebug() << m_lastError;
        return false;
    }
    
    qDebug() << "振镜跳转成功: (" << x << "," << y << "," << z << ")";
    return true;
}

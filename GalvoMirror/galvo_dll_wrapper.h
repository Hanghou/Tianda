#ifndef GALVO_DLL_WRAPPER_H
#define GALVO_DLL_WRAPPER_H

#include <QtCore/QString>
#include <QtCore/QLibrary>
#include <windows.h>

// DLL函数返回值定义
#define HM_OK           0x00000000  // 成功
#define HM_FAILED       0x00000001  // 失败
#define HM_UNKNOWN      0xFFFFFFFF  // 未知

// 设备连接状态定义
#define HM_DEV_Connect      0x00000000  // 连接状态
#define HM_DEV_Ready        0x00000001  // Ready状态
#define HM_DEV_NotAvailable 0x00000002  // 离线状态

/**
 * @brief 振镜DLL封装类
 * 封装厂家提供的HM_HashuScan.dll
 */
class GalvoDLLWrapper
{
public:
    GalvoDLLWrapper();
    ~GalvoDLLWrapper();

    /**
     * @brief 加载DLL
     * @param dllPath DLL文件路径
     * @return 成功返回true
     */
    bool loadDLL(const QString &dllPath);
    
    /**
     * @brief 卸载DLL
     */
    void unloadDLL();
    
    /**
     * @brief 检查DLL是否已加载
     * @return 已加载返回true
     */
    bool isLoaded() const;
    
    /**
     * @brief 获取最后的错误信息
     * @return 错误信息
     */
    QString getLastError() const;
    
    /**
     * @brief 初始化控制卡通讯
     * @param hWnd 消息接收窗口句柄
     * @return 成功返回true
     */
    bool initBoard(HWND hWnd = nullptr);
    
    /**
     * @brief 获取控制卡个数
     * @return 控制卡个数
     */
    int getDeviceCount();
    
    /**
     * @brief 通过IP字符串连接控制卡
     * @param ipAddress IP地址字符串
     * @return 成功返回true
     */
    bool connectByIpStr(const QString &ipAddress);
    
    /**
     * @brief 断开设备连接
     * @param ipIndex IP索引（默认0）
     * @return 成功返回true
     */
    bool disconnectDevice(int ipIndex = 0);
    
    /**
     * @brief 获取连接状态
     * @param ipIndex IP索引（默认0）
     * @return 连接状态
     */
    int getConnectStatus(int ipIndex = 0);
    
    /**
     * @brief 振镜跳转到指定位置
     * @param ipIndex IP索引
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @return 成功返回true
     */
    bool scannerJump(int ipIndex, float x, float y, float z);

private:
    bool resolveFunctions();
    
private:
    QLibrary *m_library;
    QString m_lastError;
    
    // DLL函数指针定义
    typedef int (*HM_InitBoardFunc)(HWND);
    typedef int (*HM_GetDeviceCountFunc)(int*);
    typedef int (*HM_ConnectByIpStrFunc)(char*);
    typedef int (*HM_DisconnectToFunc)(int);
    typedef int (*HM_GetConnectStatusFunc)(int);
    typedef int (*HM_ScannerJumpFunc)(int, float, float, float);
    
    HM_InitBoardFunc m_initBoard;
    HM_GetDeviceCountFunc m_getDeviceCount;
    HM_ConnectByIpStrFunc m_connectByIpStr;
    HM_DisconnectToFunc m_disconnectTo;
    HM_GetConnectStatusFunc m_getConnectStatus;
    HM_ScannerJumpFunc m_scannerJump;
};

#endif // GALVO_DLL_WRAPPER_H

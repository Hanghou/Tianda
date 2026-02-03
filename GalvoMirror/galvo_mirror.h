#ifndef GALVO_MIRROR_H
#define GALVO_MIRROR_H

#include <QObject>
#include <QString>
#include <QTimer>
#include "../Communication/device_base.h"

// 前向声明
struct MarkParameter;
struct structUdmPos;

/**
 * @brief 振镜控制卡类
 * 
 * 基于思特GMC控制卡的振镜控制实现
 * 支持2D/3D打标、在线/脱机打标等功能
 */
class GalvoMirror : public DeviceBase
{
    Q_OBJECT

public:
    explicit GalvoMirror(QObject *parent = nullptr);
    ~GalvoMirror() override;

    // ========== 基础设备接口实现 ==========
    bool initialize();  // 初始化设备（不是override，DeviceBase没有此方法）
    bool connect() override;
    void disconnect() override;  // 修改返回类型为void，匹配基类
    bool isConnected() const override;
    QString getDeviceInfo() const override;

    // ========== 振镜控制卡专用接口 ==========
    
    // 设备连接与管理
    bool searchDevices();                           // 搜索控制卡
    int getDeviceCount() const;                     // 获取设备数量
    bool connectByIP(const QString& ipAddress);     // 通过IP连接
    bool connectByIndex(int ipIndex);               // 通过索引连接
    int getConnectionStatus() const;                // 获取连接状态
    QString getDeviceIP() const;                    // 获取设备IP
    
    // 打标文件管理
    bool downloadMarkFile(const QString& filePath); // 下载打标文件
    bool downloadMarkFileSync(const QString& filePath); // 同步下载
    bool startMark();                               // 开始打标
    bool stopMark();                                // 停止打标
    bool pauseMark();                               // 暂停打标
    bool continueMark();                            // 继续打标
    int getMarkProgress() const;                    // 获取打标进度
    
    // 参数设置
    bool setOffset(float offsetX, float offsetY, float offsetZ);  // 设置偏移
    bool setRotation(float angle, float centerX, float centerY);  // 设置旋转
    bool setCoordinate(int coordinate);             // 设置坐标系
    bool setMarkRegion(int region);                 // 设置打标范围
    int getMarkRegion() const;                      // 获取打标范围
    
    // 振镜控制
    bool scannerJump(float x, float y, float z);    // 振镜跳转
    bool setGuideLaser(bool enable);                // 红光预览
    
    // 校正表管理
    bool downloadCorrection(const QString& filePath); // 下载校正表
    bool burnCorrection(const QString& filePath);     // 固化校正表
    bool selectCorrection(int crtIndex);              // 选择校正表(0或1)
    
    // IO控制
    bool setOutputOn(int outIndex);                 // 设置输出高电平
    bool setOutputOff(int outIndex);                // 设置输出低电平
    int getInputStatus() const;                     // 获取输入状态
    int getLaserInputStatus() const;                // 获取激光器报警状态
    bool setAnalog(float voutA, float voutB);       // 设置模拟量输出
    
    // 脱机打标
    bool setBurnMode(int mode);                     // 设置脱机模式(1:单文档, 2:多文档)
    bool setBurnIndex(int udmIndex);                // 设置脱机文档索引
    bool setStartBurnFlag();                        // 设置开始脱机标志
    bool burnMarkFile(bool enable);                 // 固化/清除脱机文件
    bool getBurnOverFlag() const;                   // 判断是否固化完成
    int getBurnFileNum() const;                     // 获取脱机文档个数
    bool hasSDCard() const;                         // 判断是否有SD卡
    
    // 状态查询
    int getWorkStatus() const;                      // 获取工作状态(1:ready, 2:run, 3:alarm)
    bool getFeedbackPosXY(short* fbX, short* fbY);  // 获取XY位置反馈
    bool getCmdPosXY(short* cmdX, short* cmdY);     // 获取XY位置指令
    bool getOverrangeInfo() const;                  // 获取超范围报警
    bool clearCloseLoopAlarm();                     // 清除闭环报警
    int getGalvoStatusInfo(int galvoType);          // 获取振镜状态信息

signals:
    void deviceFound(int ipIndex, QString ipAddress);  // 发现设备
    void downloadProgress(int progress);               // 下载进度
    void downloadFinished();                           // 下载完成
    void markProgress(int progress);                   // 打标进度
    void markFinished();                               // 打标完成
    void deviceStatusChanged(int status);              // 设备状态变化

private slots:
    void onProgressTimer();                         // 进度查询定时器

private:
    // ========== 内部成员变量 ==========
    int m_ipIndex;                  // 控制卡IP索引
    QString m_ipAddress;            // 控制卡IP地址
    bool m_isConnected;             // 连接状态
    int m_deviceCount;              // 设备数量
    int m_markProgress;             // 打标进度
    QTimer* m_progressTimer;        // 进度查询定时器
    
    // ========== 内部辅助方法 ==========
    void initializeParameters();    // 初始化参数
    bool loadLibraries();           // 加载DLL库
    void setupMessageHandling();    // 设置消息处理
};

#endif // GALVO_MIRROR_H

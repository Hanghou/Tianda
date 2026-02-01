#ifndef GALVO_MIRROR_H
#define GALVO_MIRROR_H

#include "../Communication/device_base.h"
#include "galvo_dll_wrapper.h"

/**
 * @brief 振镜控制类
 * 使用DLL方式控制振镜
 */
class GalvoMirror : public DeviceBase
{
    Q_OBJECT

public:
    explicit GalvoMirror(QObject *parent = nullptr);
    ~GalvoMirror();

    // 实现基类接口
    bool connect() override;
    void disconnect() override;
    bool isConnected() const override;
    QString getDeviceInfo() const override;
    
    /**
     * @brief 设置DLL路径
     * @param path DLL文件路径
     */
    void setDLLPath(const QString &path) { m_dllPath = path; }
    
    /**
     * @brief 设置IP地址
     * @param ip IP地址
     */
    void setIPAddress(const QString &ip) { m_ipAddress = ip; }
    
    /**
     * @brief 设置振镜角度
     * @param angle 角度值（度）
     * @param radius 半径（mm），默认10mm
     * @return 成功返回true
     */
    bool setAngle(float angle, float radius = 10.0f);
    
    /**
     * @brief 设置振镜二维位置（用于扫描）
     * @param xAngle X方向角度（度）
     * @param yAngle Y方向角度（度）
     * @param radius 半径（mm），默认10mm
     * @return 成功返回true
     */
    bool setPosition(float xAngle, float yAngle, float radius = 10.0f);
    
    /**
     * @brief 振镜回零位
     * @return 成功返回true
     */
    bool moveToZero();
    
    // TODO: 后续实现的功能
    // bool setAngleRange(float startAngle, float endAngle);
    // bool startMark();
    // bool stopMark();

signals:
    void markCompleted();

private:
    GalvoDLLWrapper *m_dllWrapper;
    QString m_dllPath;
    QString m_ipAddress;
    bool m_isConnected;
};

#endif // GALVO_MIRROR_H

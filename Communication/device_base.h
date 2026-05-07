#ifndef DEVICE_BASE_H
#define DEVICE_BASE_H

#include <QObject>
#include <QString>

/**
 * @brief 设备状态枚举
 */
enum class DeviceStatus {
    Disconnected,   // 未连接
    Connecting,     // 连接中
    Connected,      // 已连接
    Ready,          // 就绪
    Busy,           // 忙碌
    Error           // 错误
};

/**
 * @brief 设备基类
 * 所有设备类的基类，定义统一接口
 */
class DeviceBase : public QObject
{
    Q_OBJECT

public:
    explicit DeviceBase(QObject *parent = nullptr);
    virtual ~DeviceBase();

    /**
     * @brief 连接设备（纯虚函数，子类必须实现）
     * @return 成功返回true，失败返回false
     */
    virtual bool connect() = 0;
    
    /**
     * @brief 断开设备（纯虚函数，子类必须实现）
     */
    virtual void disconnect() = 0;
    
    /**
     * @brief 检查是否已连接（纯虚函数，子类必须实现）
     * @return 已连接返回true，否则返回false
     */
    virtual bool isConnected() const = 0;
    
    /**
     * @brief 获取设备信息（纯虚函数，子类必须实现）
     * @return 设备信息字符串
     */
    virtual QString getDeviceInfo() const = 0;
    
    /**
     * @brief 获取设备状态
     * @return 设备状态
     */
    DeviceStatus getStatus() const { return m_status; }
    
    /**
     * @brief 获取最后的错误信息
     * @return 错误信息字符串
     */
    QString getLastError() const { return m_lastError; }
    
    /**
     * @brief 获取设备名称
     * @return 设备名称
     */
    QString getDeviceName() const { return m_deviceName; }

signals:
    /**
     * @brief 状态改变信号
     * @param status 新的状态
     */
    void statusChanged(DeviceStatus status);
    
    /**
     * @brief 错误发生信号
     * @param error 错误信息
     */
    void errorOccurred(const QString &error);
    
    /**
     * @brief 数据就绪信号
     * @param data 数据
     */
    void dataReady(const QByteArray &data);
    
    /**
     * @brief 连接成功信号
     */
    void connected();
    
    /**
     * @brief 断开连接信号
     */
    void disconnected();

protected:
    /**
     * @brief 设置设备状态
     * @param status 新状态
     */
    void setStatus(DeviceStatus status);
    
    /**
     * @brief 设置错误信息
     * @param error 错误信息
     */
    void setError(const QString &error);
    
    /**
     * @brief 设置设备名称
     * @param name 设备名称
     */
    void setDeviceName(const QString &name);

protected:
    DeviceStatus m_status;      // 设备状态
    QString m_lastError;        // 最后的错误信息
    QString m_deviceName;       // 设备名称
};

#endif // DEVICE_BASE_H

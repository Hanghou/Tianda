#ifndef LASER_DRIVER_H
#define LASER_DRIVER_H

#include "../Communication/device_base.h"
#include "../Communication/serial_port_base.h"
#include "laser_protocol.h"

/**
 * @brief 激光器类型枚举
 */
enum class LaserType {
    SeedSource,  // 种子源激光器
    FOPO,        // FOPO激光器
    Stokes       // Stokes激光器
};

/**
 * @brief 激光器驱动类
 * 负责激光器的连接、断开和控制
 */
class LaserDriver : public DeviceBase
{
    Q_OBJECT

public:
    explicit LaserDriver(LaserType type, QObject *parent = nullptr);
    ~LaserDriver();

    // 实现基类接口
    bool connect() override;
    void disconnect() override;
    bool isConnected() const override;
    QString getDeviceInfo() const override;
    
    /**
     * @brief 打开串口
     * @param portName 串口名称
     * @param baudRate 波特率（默认9600）
     * @param dataBits 数据位（默认8位）
     * @param parity 校验位（默认无校验）
     * @param stopBits 停止位（默认1位）
     * @return 成功返回true
     */
    bool openPort(const QString &portName, 
                  qint32 baudRate = 9600,
                  QSerialPort::DataBits dataBits = QSerialPort::Data8,
                  QSerialPort::Parity parity = QSerialPort::NoParity,
                  QSerialPort::StopBits stopBits = QSerialPort::OneStop);
    
    /**
     * @brief 设置设备ID
     * @param id 设备ID
     */
    void setDeviceId(quint8 id) { m_deviceId = id; }
    
    /**
     * @brief 获取设备ID
     * @return 设备ID
     */
    quint8 getDeviceId() const { return m_deviceId; }
    
    /**
     * @brief 获取激光器类型
     * @return 激光器类型
     */
    LaserType getLaserType() const { return m_laserType; }
    
    /**
     * @brief 设置电流
     * @param current 电流值（种子源和Stokes单位为mA，FOPO单位为A）
     * @return 成功返回true
     */
    bool setCurrent(float current);
    
    // TODO: 后续实现的功能
    // bool turnOn();
    // bool turnOff();
    // bool setTemperature(float temperature);
    // bool queryStatus();
    // LaserStatus getStatus() const;

signals:
    void statusUpdated(const LaserStatus &status);

private slots:
    void onDataReceived(const QByteArray &data);
    void onSerialConnected();
    void onSerialDisconnected();
    void onSerialError(const QString &error);

private:
    // 协议相关（后续实现）
    QByteArray buildFrame(quint8 controlCode, const QByteArray &data);
    bool parseFrame(const QByteArray &frame, LaserStatus &status);
    quint8 calculateChecksum(const QByteArray &data);
    
private:
    SerialPortBase *m_serialPort;   // 串口对象
    QString m_portName;             // 串口名称
    qint32 m_baudRate;              // 波特率
    quint8 m_deviceId;              // 设备ID
    LaserType m_laserType;          // 激光器类型
    LaserStatus m_currentStatus;    // 当前状态
};

#endif // LASER_DRIVER_H

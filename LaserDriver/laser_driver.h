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
     * @brief 读取基本信息（0xD1命令）
     * @return 成功返回true
     */
    bool readBasicInfo();
    
    /**
     * @brief 获取基本信息
     * @return 基本信息结构
     */
    LaserBasicInfo getBasicInfo() const { return m_basicInfo; }
    
    /**
     * @brief 设置泵浦电流（仅当功率设置方式为2时有效）
     * @param current 电流值（mA）
     * @return 成功返回true
     */
    bool setPumpCurrent(quint16 current);
    
    /**
     * @brief 设置电流
     * @param current 电流值（实际值，单位根据激光器类型：种子源和Stokes为mA，FOPO为A）
     * @return 成功返回true
     */
    bool setCurrent(float current);
    
    /**
     * @brief 设置电流最大值
     * @param maxCurrent 最大电流值（单位与setCurrent相同）
     * @return 成功返回true
     */
    bool setMaxCurrent(float maxCurrent);
    
    /**
     * @brief 设置温度
     * @param temperature 温度值（摄氏度）
     * @return 成功返回true
     */
    bool setTemperature(float temperature);
    
    /**
     * @brief 开启激光器
     * @return 成功返回true
     */
    bool turnOn();
    
    /**
     * @brief 关闭激光器
     * @return 成功返回true
     */
    bool turnOff();
    
    /**
     * @brief 查询激光器状态
     * @return 成功返回true
     */
    bool queryStatus();
    
    /**
     * @brief 获取当前状态
     * @return 激光器状态
     */
    LaserStatus getStatus() const { return m_currentStatus; }

signals:
    void statusUpdated(const LaserStatus &status);

private slots:
    void onDataReceived(const QByteArray &data);
    void onSerialConnected();
    void onSerialDisconnected();
    void onSerialError(const QString &error);

private:
    // 协议相关
    QByteArray buildFrame(quint8 commandCode, const QByteArray &data);
    bool parseFrame(const QByteArray &frame);
    QByteArray calculateChecksum(const QByteArray &data);
    bool parseBasicInfo(const QByteArray &frame);
    bool parseSetCommandReply(const QByteArray &frame, quint8 expectedCmd);
    
private:
    SerialPortBase *m_serialPort;   // 串口对象
    QString m_portName;             // 串口名称
    qint32 m_baudRate;              // 波特率
    quint8 m_deviceId;              // 设备ID
    LaserType m_laserType;          // 激光器类型
    LaserStatus m_currentStatus;    // 当前状态
    LaserBasicInfo m_basicInfo;     // 基本信息
    QByteArray m_receiveBuffer;     // 接收缓冲区
};

#endif // LASER_DRIVER_H

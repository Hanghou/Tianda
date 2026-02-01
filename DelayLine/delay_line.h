#ifndef DELAY_LINE_H
#define DELAY_LINE_H

#include "../Communication/device_base.h"
#include "../Communication/serial_port_base.h"
#include "delay_protocol.h"

/**
 * @brief 延时线控制类
 * 负责延时线的连接、断开和控制
 */
class DelayLine : public DeviceBase
{
    Q_OBJECT

public:
    explicit DelayLine(QObject *parent = nullptr);
    ~DelayLine();

    // 实现基类接口
    bool connect() override;
    void disconnect() override;
    bool isConnected() const override;
    QString getDeviceInfo() const override;
    
    /**
     * @brief 打开串口
     */
    bool openPort(const QString &portName, 
                  qint32 baudRate = 9600,
                  QSerialPort::DataBits dataBits = QSerialPort::Data8,
                  QSerialPort::Parity parity = QSerialPort::NoParity,
                  QSerialPort::StopBits stopBits = QSerialPort::OneStop);
    
    /**
     * @brief 设置设备ID
     */
    void setDeviceId(quint8 id) { m_deviceId = id; }
    
    // ========== 延时线控制功能 ==========
    
    /**
     * @brief 设置延迟值（绝对位置）
     * @param delayPS 延迟值（PS，可以有小数，如100.5）
     * @return 成功返回true
     */
    bool setDelay(float delayPS);
    
    /**
     * @brief 归零
     * @return 成功返回true
     */
    bool home();
    
    /**
     * @brief 停止运动
     * @return 成功返回true
     */
    bool stop();
    
    /**
     * @brief 查询当前位置
     * @return 成功返回true
     */
    bool queryPosition();
    
    /**
     * @brief 保存配置到EEPROM
     * @return 成功返回true
     */
    bool saveToEEPROM();
    
    /**
     * @brief 获取当前延迟值
     * @return 当前延迟值（PS）
     */
    float getCurrentDelay() const;
    
    /**
     * @brief 是否在运动
     * @return 运动中返回true
     */
    bool isMoving() const;

signals:
    void delayChanged(float delayPS);
    void moveCompleted();

private slots:
    void onDataReceived(const QByteArray &data);
    void onSerialConnected();
    void onSerialDisconnected();
    void onSerialError(const QString &error);

private:
    QByteArray buildFrame(quint8 cmd, const QByteArray &data);
    bool parseFrame(const QByteArray &frame);
    
private:
    SerialPortBase *m_serialPort;
    QString m_portName;
    qint32 m_baudRate;
    quint8 m_deviceId;
    DelayLineStatus m_currentStatus;
};

#endif // DELAY_LINE_H

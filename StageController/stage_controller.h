#ifndef STAGE_CONTROLLER_H
#define STAGE_CONTROLLER_H

#include "../Communication/device_base.h"
#include "../Communication/serial_port_base.h"
#include "stage_protocol.h"

/**
 * @brief 位移台控制器类
 * 负责位移台的连接、断开和控制
 */
class StageController : public DeviceBase
{
    Q_OBJECT

public:
    explicit StageController(QObject *parent = nullptr);
    ~StageController();

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
     * @brief 设置设备地址
     * @param address 设备地址
     */
    void setAddress(quint8 address) { m_address = address; }
    
    /**
     * @brief 回零操作
     * @param direction 旋转方向（0=顺时针，1=逆时针），仅对旋转台有效
     * @return 成功返回true
     */
    bool home(quint8 direction = 0);
    
    /**
     * @brief 移动到绝对位置
     * @param position 目标位置（角度或距离）
     * @param isRotation true=旋转台（度），false=直线台（mm）
     * @return 成功返回true
     */
    bool moveAbsolute(float position, bool isRotation = false);
    
    /**
     * @brief 移动相对位置
     * @param distance 相对距离（角度或距离）
     * @param isRotation true=旋转台（度），false=直线台（mm）
     * @return 成功返回true
     */
    bool moveRelative(float distance, bool isRotation = false);
    
    /**
     * @brief 停止运动
     * @return 成功返回true
     */
    bool stop();
    
    /**
     * @brief 获取当前位置
     * @return 当前位置（脉冲数）
     */
    qint32 getPositionPulses() const { return m_currentPosition; }
    
    /**
     * @brief 获取设备信息
     * @return 成功返回true
     */
    bool requestDeviceInfo();

signals:
    void positionChanged(qint32 positionPulses);
    void moveCompleted();
    void deviceInfoReceived(QString info);

private slots:
    void onDataReceived(const QByteArray &data);
    void onSerialConnected();
    void onSerialDisconnected();
    void onSerialError(const QString &error);

private:
    QByteArray buildCommand(const QString &cmd, const QString &param = "");
    bool parseResponse(const QByteArray &response);
    qint32 positionToPulses(float position, bool isRotation);
    float pulsesToPosition(qint32 pulses, bool isRotation);
    
private:
    SerialPortBase *m_serialPort;
    QString m_portName;
    qint32 m_baudRate;
    quint8 m_address;
    StageStatus m_currentStatus;
    qint32 m_currentPosition;
    QByteArray m_receiveBuffer;
};

#endif // STAGE_CONTROLLER_H

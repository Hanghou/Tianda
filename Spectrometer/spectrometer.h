#ifndef SPECTROMETER_H
#define SPECTROMETER_H

#include "../Communication/serial_port_base.h"
#include "../Communication/device_base.h"
#include "spectrometer_protocol.h"
#include <QVector>
#include <QElapsedTimer>

/**
 * @brief 光谱仪控制类
 * 使用串口方式控制光谱仪（OPTOSKY ATP系列）
 * 串口参数：115200, 8N1
 */
class Spectrometer : public DeviceBase
{
    Q_OBJECT

public:
    explicit Spectrometer(QObject *parent = nullptr);
    ~Spectrometer();

    // 实现基类接口
    bool connect() override;
    void disconnect() override;
    bool isConnected() const override;
    QString getDeviceInfo() const override;
    
    /**
     * @brief 设置串口名称
     * @param portName 串口名称（如"COM5"）
     */
    void setPortName(const QString &portName) { m_portName = portName; }
    
    /**
     * @brief 获取串口名称
     * @return 串口名称
     */
    QString getPortName() const { return m_portName; }
    
    /**
     * @brief 设置波特率
     * @param baudRate 波特率
     */
    void setBaudRate(qint32 baudRate) { m_baudRate = baudRate; }
    
    /**
     * @brief 设置数据位
     * @param dataBits 数据位
     */
    void setDataBits(int dataBits) { m_dataBits = dataBits; }
    
    /**
     * @brief 设置停止位
     * @param stopBits 停止位
     */
    void setStopBits(int stopBits) { m_stopBits = stopBits; }
    
    /**
     * @brief 设置校验位
     * @param parity 校验位
     */
    void setParity(int parity) { m_parity = parity; }
    
    /**
     * @brief 设置积分时间
     * @param timeMicros 积分时间（微秒）
     * @return 成功返回true
     */
    bool setIntegrationTime(int timeMicros);
    
    /**
     * @brief 获取当前积分时间
     * @return 积分时间（微秒），失败返回-1
     */
    int getIntegrationTime();
    
    /**
     * @brief 获取最大积分时间
     * @return 最大积分时间（微秒），失败返回-1
     */
    int getMaxIntegrationTime();
    
    /**
     * @brief 获取最小积分时间
     * @return 最小积分时间（微秒），失败返回-1
     */
    int getMinIntegrationTime();
    
    /**
     * @brief 开始采集光谱（同步方式）
     * 使用命令0x1E进行软件触发采集，直接返回光谱数据
     * @return 成功返回true，数据通过spectrumDataReady信号发送
     */
    bool startScan();
    
    /**
     * @brief 设置采集平均次数
     * @param times 平均次数（1-1024）
     * @return 成功返回true
     */
    bool setAverageTimes(int times);
    
    /**
     * @brief 获取波长标定系数
     * @param coefficients 输出的4个标定系数
     * @return 成功返回true
     */
    bool getWavelengthCoefficients(QVector<float> &coefficients);
    
    /**
     * @brief 获取设备像素数
     * @return 像素数，失败返回-1
     */
    int getPixelLength();
    
    /**
     * @brief 获取序列号
     * @return 序列号字符串
     */
    QString getSerialNumber();
    
    /**
     * @brief 获取产品型号
     * @return 产品型号字符串
     */
    QString getProductNumber();

signals:
    void spectrumDataReady(const QVector<int> &intensity);

private:
    /**
     * @brief 发送命令
     * @param cmd 命令ID
     * @param data 数据（可选）
     * @return 成功返回true
     */
    bool sendCommand(quint8 cmd, const QByteArray &data = QByteArray());
    
    /**
     * @brief 接收响应
     * @param frame 输出的帧结构
     * @param timeout 超时时间（毫秒）
     * @return 成功返回true
     */
    bool receiveResponse(SpectrometerFrame &frame, int timeout = 1000);

private:
    SerialPortBase *m_serialPort;   // 串口对象
    QString m_portName;             // 串口名称
    qint32 m_baudRate;              // 波特率
    int m_dataBits;                 // 数据位
    int m_stopBits;                 // 停止位
    int m_parity;                   // 校验位
    int m_pixelLength;              // 像素数
    int m_integrationTime;          // 当前积分时间（微秒）
    int m_maxIntegrationTime;       // 最大积分时间（微秒）
    int m_minIntegrationTime;       // 最小积分时间（微秒）
    QString m_serialNumber;         // 序列号
    QString m_productNumber;        // 产品型号
};

#endif // SPECTROMETER_H

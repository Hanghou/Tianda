#ifndef SERIAL_PORT_BASE_H
#define SERIAL_PORT_BASE_H

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include "../Utils/constants.h"

/**
 * @brief 串口配置结构体
 */
struct SerialConfig {
    QString portName;
    qint32 baudRate;
    QSerialPort::DataBits dataBits;
    QSerialPort::Parity parity;
    QSerialPort::StopBits stopBits;
    
    SerialConfig()
        : baudRate(9600)
        , dataBits(QSerialPort::Data8)
        , parity(QSerialPort::NoParity)
        , stopBits(QSerialPort::OneStop)
    {}
};

/**
 * @brief 串口通信基类
 * 封装Qt串口通信功能，提供统一的串口操作接口
 */
class SerialPortBase : public QObject
{
    Q_OBJECT

public:
    explicit SerialPortBase(QObject *parent = nullptr);
    virtual ~SerialPortBase();

    /**
     * @brief 打开串口（使用配置结构体）
     * @param config 串口配置
     * @param maxRetry 最大重试次数
     * @return 成功返回true，失败返回false
     */
    bool openPort(const SerialConfig &config, int maxRetry = Constants::Serial::MAX_RETRY);
    
    /**
     * @brief 打开串口（传统方式，保持兼容）
     */
    bool openPort(const QString &portName, 
                  qint32 baudRate = 9600,
                  QSerialPort::DataBits dataBits = QSerialPort::Data8,
                  QSerialPort::Parity parity = QSerialPort::NoParity,
                  QSerialPort::StopBits stopBits = QSerialPort::OneStop);
    
    /**
     * @brief 关闭串口
     */
    void closePort();
    
    /**
     * @brief 检查串口是否打开
     */
    bool isOpen() const;
    
    /**
     * @brief 写入数据
     */
    qint64 writeData(const QByteArray &data);
    
    /**
     * @brief 读取数据
     */
    QByteArray readData();
    
    /**
     * @brief 读取所有可用数据
     */
    QByteArray readAll();
    
    /**
     * @brief 获取串口名称
     */
    QString portName() const;
    
    /**
     * @brief 获取当前配置
     */
    SerialConfig getConfig() const;
    
    /**
     * @brief 获取可用串口列表
     */
    static QStringList availablePorts();
    
    /**
     * @brief 设置读取超时时间
     */
    void setReadTimeout(int msec);
    
    /**
     * @brief 设置写入超时时间
     */
    void setWriteTimeout(int msec);

signals:
    void dataReceived(const QByteArray &data);
    void errorOccurred(const QString &error);
    void connected();
    void disconnected();

protected slots:
    void onReadyRead();
    void onErrorOccurred(QSerialPort::SerialPortError error);

protected:
    QSerialPort *m_serialPort;
    SerialConfig m_config;
    int m_readTimeout;
    int m_writeTimeout;
};

#endif // SERIAL_PORT_BASE_H

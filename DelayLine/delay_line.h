#ifndef DELAY_LINE_H
#define DELAY_LINE_H

#include "../Communication/device_base.h"
#include "../Communication/serial_port_base.h"
#include "delay_protocol.h"
#include <QTimer>

/**
 * @brief 延时线控制类
 * 负责延时线的连接、断开和控制
 * 支持接收缓冲帧对齐、实时位置轮询、运动状态自动判断
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
    quint8 deviceId() const { return m_deviceId; }

    // ========== 延时线控制功能（协议：FC ID FUNC P1 P2 P3 FE）==========

    /**
     * @brief 设置延迟值（绝对位置）
     * @param delayPS 目标延迟值（PS），换算规则：PS×1000→3字节大端
     * @return 成功返回true
     */
    bool setDelay(float delayPS);

    /**
     * @brief 归零（功能码 0x07）
     * @return 成功返回true
     */
    bool home();

    /**
     * @brief 停止运动（功能码 0x0B）
     * @return 成功返回true
     */
    bool stop();

    /**
     * @brief 查询当前位置（功能码 0x0E）
     * @return 成功返回true
     */
    bool queryPosition();

    /**
     * @brief 设置速度（功能码 0x02）
     * @param speed 速度值（0x12最快 ~ 0xFF最慢）
     * @return 成功返回true
     */
    bool setSpeed(quint8 speed);

    /**
     * @brief 保存配置到EPROM（功能码 0x0F）
     * @return 成功返回true
     */
    bool saveEprom();

    /**
     * @brief 开/关实时位置上报（功能码 0x39）
     * @param enable true=开启上报, false=关闭上报
     * @return 成功返回true
     */
    bool enableRealtime(bool enable);

    /**
     * @brief 获取当前延迟值（本地缓存）
     * @return 当前延迟值（PS）
     */
    float getCurrentDelay() const;

    /**
     * @brief 是否在运动
     */
    bool isMoving() const;

    // ========== 实时位置轮询控制 ==========

    /**
     * @brief 启动位置轮询
     * @param intervalMs 轮询间隔（默认100ms）
     */
    void startPolling(int intervalMs = 100);

    /**
     * @brief 停止位置轮询
     */
    void stopPolling();

    /**
     * @brief 是否正在轮询
     */
    bool isPolling() const;

signals:
    /** @brief 位置更新信号（每次收到 0xAA 应答时发射） */
    void positionUpdated(quint8 id, float delayPS);
    /** @brief 延迟值改变信号（兼容旧代码） */
    void delayChanged(float delayPS);
    /** @brief 运动完成信号（位置不再变化时发射） */
    void moveCompleted();
    /** @brief 通信超时信号（500ms 无应答） */
    void commTimeout();

private slots:
    void onDataReceived(const QByteArray &data);
    void onSerialConnected();
    void onSerialDisconnected();
    void onSerialError(const QString &error);
    void onPollTimer();          // 轮询定时器触发
    void onCommTimeoutCheck();   // 通信超时检测

private:
    QByteArray buildFrame(quint8 cmd, const QByteArray &data);
    void processBuffer();        // 缓冲区帧对齐与解析
    bool parseSingleFrame(const QByteArray &frame7);  // 解析单帧

private:
    SerialPortBase *m_serialPort;
    QString m_portName;
    qint32 m_baudRate;
    quint8 m_deviceId;
    DelayLineStatus m_currentStatus;

    // 接收缓冲区（帧对齐）
    QByteArray m_recvBuffer;

    // 实时位置轮询
    QTimer *m_pollTimer;         // 100ms 轮询定时器
    QTimer *m_commTimeoutTimer;  // 500ms 通信超时检测

    // 运动状态判断：连续多次位置不变 → 停止
    float m_lastPosition;        // 上一次位置
    int m_stableCount;           // 位置连续不变计数
    static const int STABLE_THRESHOLD = 3;  // 连续3次不变判定为停止
};

#endif // DELAY_LINE_H

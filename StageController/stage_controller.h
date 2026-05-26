#ifndef STAGE_CONTROLLER_H
#define STAGE_CONTROLLER_H

#include "../Communication/device_base.h"
#include "../Communication/serial_port_base.h"
#include "stage_protocol.h"

/**
 * @brief Thorlabs ELLx 双台位移台控制器（双串口聚合）
 *
 * 物理拓扑：两台同型号 ELLx 位移台分别接在两根独立串口上：
 *   台1 → 串口1（如 COM3）
 *   台2 → 串口2（如 COM4）
 * 每台都按 ELLx 单台协议通信（地址固定使用 '0'）。
 *
 * 业务接口以 *Dual() 命名：内部依次对两个串口下发同一指令，
 * 让两台同步执行同一动作（典型用法：双台同步绝对位移）。
 */
class StageController : public DeviceBase
{
    Q_OBJECT

public:
    explicit StageController(QObject *parent = nullptr);
    ~StageController();

    // ========== 基类接口（语义：两台都连接才算 Connected） ==========
    bool connect() override;
    void disconnect() override;
    bool isConnected() const override;
    QString getDeviceInfo() const override;

    // ========== 双串口连接接口 ==========
    bool openPort1(const QString &portName,
                   qint32 baudRate = 9600,
                   QSerialPort::DataBits dataBits = QSerialPort::Data8,
                   QSerialPort::Parity parity = QSerialPort::NoParity,
                   QSerialPort::StopBits stopBits = QSerialPort::OneStop);
    bool openPort2(const QString &portName,
                   qint32 baudRate = 9600,
                   QSerialPort::DataBits dataBits = QSerialPort::Data8,
                   QSerialPort::Parity parity = QSerialPort::NoParity,
                   QSerialPort::StopBits stopBits = QSerialPort::OneStop);
    void closePort1();
    void closePort2();

    bool isConnected1() const;
    bool isConnected2() const;

    // ========== 双台同步控制接口 ==========

    /** 读取双台换算参数（0in），必须在 moveAbsoluteDual 之前调用 */
    bool readDeviceInfoDual();

    /** 设置双台最大速度（sv 指令） */
    bool setMaxSpeedDual(qint32 speedPulses);

    /** 双台同步绝对位移（ma 指令）：按各自的 pulsePerUnit 各算一次脉冲数 */
    bool moveAbsoluteDual(double positionMm);

    bool queryStatusDual();
    bool queryPositionDual();
    bool stopDual();
    /** 双台回零（ho），direction：0=顺时针，1=逆时针 */
    bool homeDual(quint8 direction = 0);

    // ========== 单台调试接口（可选） ==========
    bool moveAbsolute1(double positionMm);
    bool moveAbsolute2(double positionMm);

    const EllxDeviceInfo &deviceInfo1() const { return m_info1; }
    const EllxDeviceInfo &deviceInfo2() const { return m_info2; }
    qint32 positionPulses1() const { return m_status1.positionPulses; }
    qint32 positionPulses2() const { return m_status2.positionPulses; }

signals:
    // 单台位置变化
    void positionChanged1(qint32 positionPulses);
    void positionChanged2(qint32 positionPulses);
    // 双台都完成才发射（用于"双台同步移动完成"）
    void moveCompletedDual();
    // 单台连接/断开（供 UI 各自显示状态指示器）
    void stage1Connected();
    void stage1Disconnected();
    void stage2Connected();
    void stage2Disconnected();
    void deviceInfoReceived(QString info);

private slots:
    void onSerial1DataReceived(const QByteArray &data);
    void onSerial2DataReceived(const QByteArray &data);
    void onSerial1Connected();
    void onSerial1Disconnected();
    void onSerial1Error(const QString &error);
    void onSerial2Connected();
    void onSerial2Disconnected();
    void onSerial2Error(const QString &error);

private:
    bool sendFrame1(const QByteArray &frame);
    bool sendFrame2(const QByteArray &frame);
    // stageIndex: 1=台1, 2=台2
    void parseLine(int stageIndex, const QString &line);
    void parseInfoLine(int stageIndex, const QString &resp);
    void parseStatusLine(int stageIndex, const QString &resp);
    void parsePositionLine(int stageIndex, const QString &resp);

private:
    SerialPortBase  *m_serial1;
    SerialPortBase  *m_serial2;
    QString          m_portName1;
    qint32           m_baudRate1;
    QString          m_portName2;
    qint32           m_baudRate2;
    QByteArray       m_recvBuf1;
    QByteArray       m_recvBuf2;

    EllxDeviceInfo   m_info1;
    EllxDeviceInfo   m_info2;
    StageStatus      m_status1;
    StageStatus      m_status2;

    // moveAbsoluteDual 后跟踪两台 PO 响应，凑齐才发 moveCompletedDual
    bool             m_pendingMove1;
    bool             m_pendingMove2;
};

#endif // STAGE_CONTROLLER_H

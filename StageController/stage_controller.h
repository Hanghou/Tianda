#ifndef STAGE_CONTROLLER_H
#define STAGE_CONTROLLER_H

#include "../Communication/device_base.h"
#include "stage_protocol.h"      // 保留旧结构体 EllxDeviceInfo/StageStatus，兼容现有外部读取接口
#include "mt_axis_config.h"
#include <QSerialPort>
#include <QTimer>

/**
 * @brief MT_API 运动控制卡双轴位移台控制器
 *
 * 业务用途：替换旧 Thorlabs ELLx 双串口 ASCII 协议，改为一张 MT_API 控制卡 + 两个轴。
 * 兼容策略：保留 openPort1/openPort2/closePort1/closePort2 等旧接口，避免 UI 层立即大改。
 * 单位约定：moveAbsoluteDual() 新语义为 μm；setMaxSpeedUmPerSec() 接收 μm/s。
 */
class StageController : public DeviceBase
{
    Q_OBJECT

public:
    explicit StageController(QObject *parent = nullptr);
    ~StageController();

    // ========== 基类接口（一张控制卡连接成功即 Stage1/Stage2 均可用） ==========
    bool connect() override;
    void disconnect() override;
    bool isConnected() const override;
    QString getDeviceInfo() const override;

    // ========== 新版单串口控制卡接口 ==========
    void setPortName(const QString &port);
    QString portName() const;

    // ========== 旧双串口接口兼容壳（供现有 UI/integration.cpp 调用） ==========
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

    // ========== 双轴同步控制接口 ==========
    bool readDeviceInfoDual();
    bool setMaxSpeedDual(qint32 speedPulsesPerSec);       // 兼容旧接口：直接接收 pulse/s
    bool setMaxSpeedUmPerSec(double umPerSec);            // 新接口：接收 μm/s，内部换算 pulse/s
    bool moveAbsoluteDual(double displacementUm);         // 新语义：接收 μm，内部换算 pulse
    bool queryStatusDual();
    bool queryPositionDual();
    bool stopDual();
    bool haltDual();
    bool homeDual(quint8 direction = 0);                  // 兼容旧签名，实际按配置回零方向执行

    // ========== 单轴调试接口（兼容旧接口，参数新语义为 μm） ==========
    bool moveAbsolute1(double displacementUm);
    bool moveAbsolute2(double displacementUm);

    // ========== 单轴独立控制接口（双轴可分别设速度/位移/停止） ==========
    // axis: 0=轴1, 1=轴2。speed 单位 μm/s，位移单位 μm。
    bool setAxisSpeedUmPerSec(unsigned short axis, double umPerSec);  // 设置指定轴运动速度
    bool stopAxis(unsigned short axis);                              // 停止指定轴（减速停止）

    // ========== 查询接口 ==========
    const EllxDeviceInfo &deviceInfo1() const { return m_info1; }
    const EllxDeviceInfo &deviceInfo2() const { return m_info2; }
    qint32 positionPulses1() const { return m_axisPos[0]; }
    qint32 positionPulses2() const { return m_axisPos[1]; }
    double positionUm1() const { return MtAxisConfig::pulsesToUm(m_axisPos[0]); }
    double positionUm2() const { return MtAxisConfig::pulsesToUm(m_axisPos[1]); }
    bool isMoving1() const { return m_axisRunning[0]; }
    bool isMoving2() const { return m_axisRunning[1]; }

signals:
    void positionChanged1(qint32 positionPulses);
    void positionChanged2(qint32 positionPulses);
    void moveCompletedDual();
    void stage1Connected();
    void stage1Disconnected();
    void stage2Connected();
    void stage2Disconnected();
    void deviceInfoReceived(QString info);

private slots:
    void onPollTimer();

private:
    enum class PendingAction { None, Move, Home };

    bool configureAxis(unsigned short axis);
    bool checkResult(int result, const QString &action);
    bool moveAbsoluteAxis(unsigned short axis, double displacementUm);
    void resetRuntimeState();

private:
    QString m_portName;
    bool m_cardOpen;
    bool m_axisRunning[2];
    bool m_axisPending[2];
    bool m_axisObservedRunning[2];   // 防误完成：本次任务中是否已经观察到轴进入运行状态
    qint32 m_axisPos[2];
    int m_speedPPS;
    int m_axisSpeedPPS[2];           // 每轴独立运动速度（pulse/s），供单轴独立控制使用
    QTimer *m_pollTimer;
    PendingAction m_pendingAction;

    // 兼容旧 deviceInfo1()/deviceInfo2() 返回结构。MT_API 下 pulsePerUnit 用 pulse/μm 表示。
    EllxDeviceInfo m_info1;
    EllxDeviceInfo m_info2;
};

#endif // STAGE_CONTROLLER_H

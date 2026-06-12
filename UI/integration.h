#ifndef INTEGRATION_H
#define INTEGRATION_H

// 禁用 Windows min/max 宏，避免与 Qt Charts 冲突
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <QMainWindow>
#include <QSerialPortInfo>
#include <QTimer>
#include <QTableWidget>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include "../LaserDriver/ohld_protocol.h"
#include "../Spectrometer/spectrometer.h"
#include "../StageController/stage_controller.h"
#include "../DelayLine/delay_line.h"
#include "../GalvoMirror/galvo_mirror.h"
#include "../Utils/config_manager.h"
#include "../Utils/data_manager.h"
#include "../Utils/csv_exporter.h"
#include "../Utils/image_saver.h"
#include "../Utils/preset_manager.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Integration; }
QT_END_NAMESPACE

/**
 * @brief 预设页面类型枚举
 */
enum class PresetPageType {
    GalvoPage,    // 振镜页
    StagePage     // 位移台页
};

/**
 * @brief 主窗口类
 * 整合所有设备模块
 */
class Integration : public QMainWindow
{
    Q_OBJECT

public:
    Integration(QWidget *parent = nullptr);
    ~Integration();

private slots:
    // 连接/断开槽函数
    void on_btnConnectSpectrometerFOPO_clicked();
    void on_btnDisconnectSpectrometerFOPO_clicked();
    void on_btnConnectSpectrometerStokes_clicked();
    void on_btnDisconnectSpectrometerStokes_clicked();
    void on_btnConnectStage1_clicked();
    void on_btnDisconnectStage1_clicked();
    void on_btnConnectGalvo_clicked();
    void on_btnDisconnectGalvo_clicked();
    void on_btnConnectDelay_clicked();
    void on_btnDisconnectDelay_clicked();

    // OHLD 四泵连接/断开槽函数
    void on_btnConnectPump1_clicked();
    void on_btnDisconnectPump1_clicked();
    void on_btnConnectPump2_clicked();
    void on_btnDisconnectPump2_clicked();
    void on_btnConnectPump3_clicked();
    void on_btnDisconnectPump3_clicked();
    void on_btnConnectPump4_clicked();
    void on_btnDisconnectPump4_clicked();

    // FOPO路光谱仪测量控制槽函数
    void on_btnSingleMeasureFOPO_clicked();
    void on_btnContinuousMeasureFOPO_clicked();
    void on_btnStopMeasureFOPO_clicked();
    void onMeasureTimeoutFOPO();

    // Stokes路光谱仪测量控制槽函数
    void on_btnSingleMeasureStokes_clicked();
    void on_btnContinuousMeasureStokes_clicked();
    void on_btnStopMeasureStokes_clicked();
    void onMeasureTimeoutStokes();

    // FOPO路图表控制槽函数
    void on_btnSavePlotFOPO_clicked();
    void on_btnResetViewFOPO_clicked();
    void on_btnClearPlotFOPO_clicked();

    // Stokes路图表控制槽函数
    void on_btnSavePlotStokes_clicked();
    void on_btnResetViewStokes_clicked();
    void on_btnClearPlotStokes_clicked();

    // FOPO路峰值检测槽函数
    void on_btnShowPeaksFOPO_clicked();  // 显示峰值检测窗口

    // Stokes路峰值检测槽函数
    void on_btnShowPeaksStokes_clicked();  // 显示峰值检测窗口

    // 位移台控制槽函数
    void on_btnStageMoveAbsolute_clicked();        // 轴1 绝对移动
    void on_btnStageMoveAbsolute2_clicked();       // 轴2 绝对移动
    void on_btnStageStop1_clicked();               // 轴1 停止
    void on_btnStageStop2_clicked();               // 轴2 停止
    void on_btnConfirmStageAngle_clicked();        // 旧槽（保留兼容，转发）
    void on_btnConfirmStagePosition_clicked();     // 旧槽（保留兼容，转发）
    void on_btnConfirmStageTimeDelay_clicked();    // 旧单延迟线槽（保留兼容）
    void on_btnConfirmStageDelayLine1_clicked();   // 设置延迟线1（位移台页）
    void on_btnConfirmStageDelayLine2_clicked();   // 设置延迟线2（位移台页）
    void on_btnStageDelayLine1Home_clicked();      // 归零延迟线1（位移台页）
    void on_btnStageDelayLine2Home_clicked();      // 归零延迟线2（位移台页）
    void onDelayLine1PositionUpdated(quint8 id, float delayPS);  // 延迟线1实时位置更新
    void onDelayLine2PositionUpdated(quint8 id, float delayPS);  // 延迟线2实时位置更新
    void onStage1PositionChanged(qint32 positionPulses);
    void onStage2PositionChanged(qint32 positionPulses);
    void onStageMoveCompletedDual();

    // 延时线控制槽函数
    void on_btnConfirmTimeDelay_clicked();         // 设置延迟值（振镜版）
    void onDelayChanged(float delayPS);            // 延迟值改变（延迟线1）
    void onDelay2Changed(float delayPS);           // 延迟值改变（延迟线2）

    // 振镜角度控制槽函数（UI 仅保留角度输入 + 确定按钮）
    void on_btnGalvoStart_clicked();                  // 跳转到目标角度
    void on_btnGalvoStop_clicked();                   // 停止/复位
    void onGalvoHeartbeatChanged(bool online);        // 心跳状态变化

    // 泵功率设置槽函数 - 振镜页（OHLD 三泵：种子源 / FOPO预放 / Stokes，pumpIndex=0/1/3）
    // 注：振镜页不再控制 pump路主级泵（pumpIndex=2），该泵仅在位移台页使用
    void on_btnConfirmGalvoPump1_clicked();   // 种子源泵 (OHLD pumpIndex=0)
    void on_btnConfirmGalvoPump2_clicked();   // pump路预放泵 (OHLD pumpIndex=1)
    void on_btnConfirmGalvoPump4_clicked();   // Stokes路泵 (OHLD pumpIndex=3)

    // 泵功率设置槽函数 - 位移台页（OHLD 四泵）
    void on_btnConfirmStagePump1_clicked();   // 种子源泵
    void on_btnConfirmStagePump2_clicked();   // pump路预放泵
    void on_btnConfirmStagePump3_clicked();   // pump路主级泵
    void on_btnConfirmStagePump4_clicked();   // Stokes路泵

    // 预设管理槽函数 - 振镜页（光源功率预设）
    void on_btnStartPowerExecution_clicked();    // 开始执行功率预设
    void on_btnStopPowerExecution_clicked();     // 停止执行功率预设
    void onPowerPresetDelayTimeout();            // 功率预设间隔定时器触发
    void on_btnConfirmDelayPreset_clicked();     // 确定按钮（振镜页底部执行按钮）

    // 预设管理槽函数 - 振镜页（延迟线预设）
    void on_btnStartDelayExecutionGalvo_clicked();    // 开始执行延迟预设
    void on_btnStopDelayExecutionGalvo_clicked();     // 停止执行延迟预设
    void onDelayPresetDelayTimeoutGalvo();            // 延迟预设间隔定时器触发

    // 预设管理槽函数 - 位移台页（波长调谐，整合原电控与功率预设 + 延迟线预设）
    void on_btnStartPowerExecutionStage_clicked();    // 开始执行波长调谐预设
    void on_btnStopPowerExecutionStage_clicked();     // 停止执行波长调谐预设
    void onPowerPresetDelayTimeoutStage();            // 波长调谐预设间隔定时器触发

    // 位移台页扫描槽函数（暂留空，待实现）
    void on_btnStageScanSetWavelength_clicked();      // 单点波长设置
    void on_btnStageScanStart_clicked();              // 开始扫描
    void on_btnStageScanStop_clicked();               // 停止扫描

    // 预设管理槽函数 - 位移台页（延迟线预设 - 已整合到波长调谐）
    void on_btnStartDelayExecution_clicked();    // 开始执行延迟预设
    void on_btnStopDelayExecution_clicked();     // 停止执行延迟预设
    void onDelayPresetDelayTimeout();            // 延迟预设间隔定时器触发

    // 设备状态更新槽函数
    void onSpectrometerFOPOStatusChanged(DeviceStatus status);
    void onSpectrometerStokesStatusChanged(DeviceStatus status);
    void onStageStatusChanged(DeviceStatus status);
    void onGalvoStatusChanged(DeviceStatus status);
    void onDelayStatusChanged(DeviceStatus status);

    // 光谱仪数据接收槽函数
    void onSpectrumDataReady(const QVector<int> &intensity);  // FOPO路
    void onSpectrumDataReadyStokes(const QVector<int> &intensity);  // Stokes路

    // 错误处理槽函数
    void onDeviceError(const QString &error);

private:
    // 串口配置结构体（移到Communication/serial_port_base.h）
    // struct SerialConfig 已在serial_port_base.h中定义

    // 初始化函数
    void initDevices();
    void initUI();
    void initConnections();
    void loadConfiguration();
    void saveConfiguration();

    // UI初始化辅助函数
    void initSerialPortCombos();
    void refreshSerialPorts();
    void populateSerialPortCombo(class QComboBox *combo);
    void populateBaudRateCombo(class QComboBox *combo);
    void populateDataBitsCombo(class QComboBox *combo);
    void populateStopBitsCombo(class QComboBox *combo);
    void populateParityCombo(class QComboBox *combo);

    // UI更新函数
    void updateConnectionStatus();
    void updateStatusBar(const QString &message);
    void initStatusIndicators();  // 初始化所有状态指示器
    void updateStatusIndicator(class QLabel *indicator, DeviceStatus status);

    // 串口配置获取函数
    SerialConfig getSpectrometerFOPOSerialConfig();
    SerialConfig getSpectrometerStokesSerialConfig();
    SerialConfig getDelayLineSerialConfig();
    SerialConfig getDelayLineSerialConfig2();   // 延迟线2独立串口配置
    SerialConfig getStageSerialConfig1();   // 位移台1串口配置
    SerialConfig getStageSerialConfig2();   // 位移台2串口配置

    // 振镜网络配置获取函数
    QString getGalvoIPAddress();

    // 光谱测量相关函数
    void startMeasurement();
    void stopMeasurement();
    void saveSpectrum();

    // 图表相关函数
    void initSpectrumChart();
    void updateSpectrum(const QVector<int> &intensity);  // FOPO路
    void updateSpectrumStokes(const QVector<int> &intensity);  // Stokes路
    void showChartMaximized();  // 显示最大化图表窗口（FOPO路）
    void showChartMaximizedStokes();  // 显示最大化图表窗口（Stokes路）

    // 峰值检测相关函数（已废弃，改用弹窗方式）
    // void detectPeaks();
    // void updatePeaksTable();
    // void exportPeaksToCSV();
    double calculateFWHM(int peakIndex, const QVector<int> &data);

    // 配置保存/加载辅助函数
    void saveSerialConfig(const QString &device, const QString &port,
                         qint32 baudRate, int dataBits, int stopBits, int parity);
    void loadSerialConfig(const QString &device, QString &port,
                         qint32 &baudRate, int &dataBits, int &stopBits, int &parity);

    // 预设管理辅助函数
    void initPresetTables();                     // 初始化预设表格

    QList<PowerPreset> loadPowerPresetsFromTable();  // 从表格加载功率预设
    QList<StagePowerPreset> loadPowerPresetsFromTableStage();  // 从表格加载功率预设（位移台页 - 旧，保留兼容）
    QList<WavelengthTuningPreset> loadWavelengthTuningPresetsFromTableStage();  // 从波长调谐表加载预设
    void executePowerPreset(int index);          // 执行单个功率预设
    void executePowerPresetStage(int index);     // 执行单个功率预设（位移台页）
    void executeCombinedPresetGalvo(int index);  // 执行组合预设（振镜页：功率+延迟线）
    void executeCombinedPresetStage(int index);  // 执行组合预设（位移台页：电控+功率+延迟线）
    void stopPowerPresetExecution();             // 停止功率预设执行
    void stopPowerPresetExecutionStage();        // 停止功率预设执行（位移台页）

    // 统一的延迟预设方法
    QList<DelayPreset> loadDelayPresetsFromTable(PresetPageType pageType);
    void executeDelayPreset(PresetPageType pageType, int index);
    void stopDelayPresetExecution(PresetPageType pageType);

protected:
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;  // 事件过滤器

private:
    Ui::Integration *ui;

    // 设备模块
    Spectrometer *m_spectrometerFOPO;    // FOPO路光谱仪
    Spectrometer *m_spectrometerStokes;  // Stokes路光谱仪
    StageController *m_stageController;
    GalvoMirror *m_galvoMirror;        // 振镜控制卡
    DelayLine *m_delayLine;            // 延迟线1（设备ID=0x01，振镜页 + 位移台页）
    DelayLine *m_delayLine2;           // 延迟线2（设备ID=0x02，位移台页独立串口）

    // OHLD 四泵：独立串口实例（位移台页泵功率控制）
    SerialPortBase *m_ohldPumps[4];    // 四泵独立串口，索引0~3对应泵1~4
    OhldPumpStatus  m_ohldStatus[4];   // 四泵状态缓存
    QByteArray m_kntRealtimeMeaning[4];  // 科乃特D2实时信息含义缓存，用于解析D3电流/温度字段
    QTimer *m_kntPollTimer[4];          // 科乃特四泵实时电流轮询定时器（连接后持续D3查询）
    bool m_kntBusy[4];                  // 串口忙标志：避免轮询D3与设置功率C3并发串包


    // 工具模块
    ConfigManager *m_configManager;
    DataManager *m_dataManager;
    CSVExporter *m_csvExporter;
    ImageSaver *m_imageSaver;
    PresetManager *m_presetManager;

    // FOPO路光谱仪测量状态
    bool m_isContinuousMeasuringFOPO;
    bool m_isMeasuringFOPO;
    QTimer *m_measureTimerFOPO;

    // Stokes路光谱仪测量状态
    bool m_isContinuousMeasuringStokes;
    bool m_isMeasuringStokes;
    QTimer *m_measureTimerStokes;

    // FOPO路图表相关
    QChartView *m_chartViewFOPO;
    QChart *m_chartFOPO;
    QLineSeries *m_seriesFOPO;
    QValueAxis *m_axisXFOPO;
    QValueAxis *m_axisYFOPO;
    QDialog *m_chartMaximizedDialogFOPO;

    // Stokes路图表相关
    QChartView *m_chartViewStokes;
    QChart *m_chartStokes;
    QLineSeries *m_seriesStokes;
    QValueAxis *m_axisXStokes;
    QValueAxis *m_axisYStokes;
    QDialog *m_chartMaximizedDialogStokes;

    // 位移台页独立图表（_S 后缀，与振镜页同步显示）
    QChartView *m_chartViewFOPO_S;
    QChart *m_chartFOPO_S;
    QLineSeries *m_seriesFOPO_S;
    QChartView *m_chartViewStokes_S;
    QChart *m_chartStokes_S;
    QLineSeries *m_seriesStokes_S;

    // 峰值检测相关
    struct PeakData {
        double wavelength;  // 波长
        int intensity;      // 强度
        double fwhm;        // 半高全宽
        int pixelIndex;     // 像素索引
    };
    // FOPO路峰值检测相关
    QVector<PeakData> m_peaksFOPO;
    QVector<int> m_lastSpectrumDataFOPO;

    // Stokes路峰值检测相关
    QVector<PeakData> m_peaksStokes;
    QVector<int> m_lastSpectrumDataStokes;

    // 预设执行相关 - 功率预设（振镜页）
    class QTableWidget *m_powerPresetTable;      // 功率预设表格
    QTimer *m_powerPresetDelayTimer;             // 功率预设间隔定时器
    QList<PowerPreset> m_currentPowerPresets;    // 当前执行的功率预设列表
    int m_currentPowerPresetIndex;               // 当前执行的预设索引
    bool m_isPowerPresetExecuting;               // 是否正在执行功率预设
    int m_powerPresetTimeInterval;               // 功率预设时间间隔（秒）

    // 预设执行相关 - 延迟预设（统一管理）
    class QTableWidget *m_delayPresetTableGalvo;      // 延迟预设表格（振镜页）
    class QTableWidget *m_delayPresetTable;           // 延迟预设表格（位移台页）
    QTimer *m_delayPresetDelayTimerGalvo;             // 延迟预设间隔定时器（振镜页）
    QTimer *m_delayPresetDelayTimer;                  // 延迟预设间隔定时器（位移台页）
    QList<DelayPreset> m_currentDelayPresetsGalvo;    // 当前执行的延迟预设列表（振镜页）
    QList<DelayPreset> m_currentDelayPresets;         // 当前执行的延迟预设列表（位移台页）
    int m_currentDelayPresetIndexGalvo;               // 当前执行的预设索引（振镜页）
    int m_currentDelayPresetIndex;                    // 当前执行的预设索引（位移台页）
    bool m_isDelayPresetExecutingGalvo;               // 是否正在执行延迟预设（振镜页）
    bool m_isDelayPresetExecuting;                    // 是否正在执行延迟预设（位移台页）
    int m_delayPresetTimeIntervalGalvo;               // 延迟预设时间间隔（秒）（振镜页）
    int m_delayPresetTimeInterval;                    // 延迟预设时间间隔（秒）（位移台页）

    // 预设执行相关 - 功率预设（位移台页）/ 波长调谐
    class QTableWidget *m_powerPresetTableStage;      // 功率预设表格（位移台页 - 旧，保留兼容）
    class QTableWidget *m_wavelengthTuningTable;      // 波长调谐表格（位移台页 - 新统一表）
    QTimer *m_powerPresetDelayTimerStage;             // 功率预设间隔定时器（位移台页）
    QList<StagePowerPreset> m_currentPowerPresetsStage;  // 当前执行的功率预设列表（位移台页）
    QList<WavelengthTuningPreset> m_currentWavelengthTuningPresets;  // 当前执行的波长调谐预设列表
    int m_currentPowerPresetIndexStage;               // 当前执行的预设索引（位移台页）
    bool m_isPowerPresetExecutingStage;               // 是否正在执行功率预设（位移台页）
    int m_powerPresetTimeIntervalStage;               // 功率预设时间间隔（秒）（位移台页）

    // 辅助方法：根据页面类型获取对应的成员变量
    QTableWidget* getDelayPresetTable(PresetPageType pageType);
    QTimer* getDelayPresetTimer(PresetPageType pageType);
    QList<DelayPreset>& getCurrentDelayPresets(PresetPageType pageType);
    int& getCurrentDelayPresetIndex(PresetPageType pageType);
    bool& isDelayPresetExecuting(PresetPageType pageType);
    int& getDelayPresetTimeInterval(PresetPageType pageType);

    // 科乃特四泵统一控制方法（连接后开光源；确定按钮设置输出功率mW→读取实时信息→刷新驱动电流）
    void setOhldPumpCurrent(int pumpIndex, float powerMW);
    // 科乃特四泵静默控制：仅设置输出功率，用于预设执行链路批量下发。
    bool setOhldPumpCurrentSilent(int pumpIndex, float powerMW);
    // OHLD 四泵统一连接/断开
    void connectOhldPump(int pumpIndex);
    void disconnectOhldPump(int pumpIndex);
    // 科乃特四泵实时电流轮询：连接后持续发送D3查询并刷新对应驱动电流显示
    void pollOhldPumpCurrent(int pumpIndex);
    // 刷新指定泵的驱动电流显示框（lineEditPumpXDrive）
    void updatePumpDriveCurrentDisplay(int pumpIndex, float driveCurrentMA);

    // OHLD 四泵 UI 辅助查找
    class QComboBox *ohldPumpPortCombo(int pumpIndex) const;
    class QComboBox *ohldPumpBaudCombo(int pumpIndex) const;
    class QLabel    *ohldPumpStatusLabel(int pumpIndex) const;

    /**
     * @brief 振镜角度到坐标映射（占位实现，需根据实际标定替换）
     * @param angleDeg 角度（度）
     * @param[out] x 振镜 X 坐标
     * @param[out] y 振镜 Y 坐标
     * @param[out] z 振镜 Z 坐标
     */
    void galvoAngleToCoord(float angleDeg, float &x, float &y, float &z) const;

    // 统一的延迟线设置方法
    void setDelayTime(class QLineEdit *inputField, class QLineEdit *syncField = nullptr);

    // 单轴绝对移动公共处理：读取速度/位移输入框，设速度并下发该轴绝对移动。axis: 0=轴1, 1=轴2。
    void moveStageAxisFromUi(unsigned short axis, class QLineEdit *speedEdit, class QLineEdit *displaceEdit);
    // 双延迟线统一控制方法（归零→设置→查询）
    void setDelayLineValue(DelayLine *device, float delayPS, const QString &label);
    // 仅设置延迟方法（设置→查询，不归零），用于确定按钮使命令独立
    void setDelayLineOnly(DelayLine *device, float delayPS, const QString &label);

    // 编辑按钮槽函数
    void on_btnEditPowerPresets_clicked();         // 编辑光源功率预设
    void on_btnEditDelayPresetsGalvo_clicked();    // 编辑延迟线预设（振镜页）
    void on_btnEditPowerPresetsStage_clicked();    // 编辑电控平台与功率预设（位移台页 - 旧，保留兼容）
    void on_btnEditDelayPresets_clicked();         // 编辑延迟线预设（位移台页 - 旧，保留兼容）
    void on_btnEditWavelengthTuningStage_clicked();  // 编辑波长调谐预设（位移台页 - 新统一入口）

    // 编辑弹窗辅助方法
    void showPowerPresetEditDialog();              // 显示功率预设编辑弹窗
    void showDelayPresetEditDialogGalvo();         // 显示延迟预设编辑弹窗（振镜页）
    void showPowerPresetEditDialogStage();         // 显示功率预设编辑弹窗（位移台页 - 旧，保留兼容）
    void showDelayPresetEditDialog();              // 显示延迟预设编辑弹窗（位移台页 - 旧，保留兼容）
    void showWavelengthTuningEditDialogStage();    // 显示波长调谐编辑弹窗（位移台页 - 新统一弹窗）
};

#endif // INTEGRATION_H

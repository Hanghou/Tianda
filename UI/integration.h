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
#include "../LaserDriver/laser_driver.h"
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
 * @brief 激光器设备类型枚举（用于代码复用）
 * 注意：三个激光器是完全独立的设备，各自有独立的 LaserDriver 实例
 */
enum class LaserDeviceType {
    Seed,     // 种子源激光器
    FOPO,     // FOPO激光器
    Stokes    // Stokes激光器
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
    void on_btnConnectSeedLaser_clicked();
    void on_btnDisconnectSeedLaser_clicked();
    void on_btnConnectFOPOLaser_clicked();
    void on_btnDisconnectFOPOLaser_clicked();
    void on_btnConnectStokesLaser_clicked();
    void on_btnDisconnectStokesLaser_clicked();
    void on_btnConnectSpectrometer_clicked();
    void on_btnDisconnectSpectrometer_clicked();
    void on_btnConnectStage_clicked();
    void on_btnDisconnectStage_clicked();
    void on_btnConnectGalvo_clicked();
    void on_btnDisconnectGalvo_clicked();
    void on_btnConnectDelay_clicked();
    void on_btnDisconnectDelay_clicked();
    
    // 光谱仪测量控制槽函数
    void on_btnSingleMeasure_clicked();
    void on_btnContinuousMeasure_clicked();
    void on_btnStopMeasure_clicked();
    void onMeasureTimeout();  // 测量定时器触发
    
    // 图表控制槽函数
    void on_btnSavePlot_clicked();
    void on_btnResetView_clicked();
    void on_btnClearPlot_clicked();
    
    // 峰值检测槽函数
    void on_btnDetectPeaks_clicked();
    void on_btnExportPeaks_clicked();
    void on_btnClearPeaks_clicked();
    
    // 位移台控制槽函数
    void on_btnConfirmStageAngle_clicked();     // 设置旋转台角度
    void on_btnConfirmStagePosition_clicked();  // 设置直线台位置
    void on_btnConfirmStageTimeDelay_clicked(); // 设置延时线延迟（位移台版）
    void onStagePositionChanged(qint32 positionPulses);  // 位置改变
    void onStageMoveCompleted();                // 移动完成
    
    // 延时线控制槽函数
    void on_btnConfirmTimeDelay_clicked();  // 设置延迟值（振镜版）
    void onDelayChanged(float delayPS);     // 延迟值改变
    
    // 振镜角度控制槽函数
    void on_btnConfirmGalvoAngle_clicked(); // 设置振镜角度
    
    // 泵功率设置槽函数 - 振镜页
    void on_btnConfirmSeedPump_clicked();   // 确认种子源泵设置
    void on_btnConfirmFOPOPump_clicked();   // 确认FOPO泵设置
    void on_btnConfirmStokesPump_clicked(); // 确认Stokes泵设置
    
    // 泵功率设置槽函数 - 位移台页
    void on_btnConfirmStageSeedPump_clicked();   // 确认种子源泵设置（位移台页）
    void on_btnConfirmStageFOPOPump_clicked();   // 确认FOPO泵设置（位移台页）
    void on_btnConfirmStageStokesPump_clicked(); // 确认Stokes泵设置（位移台页）
    
    // 预设管理槽函数 - 振镜页（光源功率预设）
    void on_btnLightPowerPreset_clicked();   // 光源功率预设置入口按钮（切换显示/隐藏）
    void on_btnAddPowerPresetRow_clicked();      // 添加功率预设行
    void on_btnClearPowerPresets_clicked();      // 清空所有功率预设
    void on_btnMoveUpPowerPreset_clicked();      // 上移功率预设行
    void on_btnMoveDownPowerPreset_clicked();    // 下移功率预设行
    void on_btnDeletePowerPreset_clicked();      // 删除功率预设行
    void on_btnStartPowerExecution_clicked();    // 开始执行功率预设
    void on_btnStopPowerExecution_clicked();     // 停止执行功率预设
    void onPowerPresetDelayTimeout();            // 功率预设间隔定时器触发
    void on_btnConfirmDelayPreset_clicked();     // 确定按钮（振镜页底部执行按钮）
    
    // 预设管理槽函数 - 振镜页（延迟线预设）
    void on_btnDelayLinePreset_clicked();        // 延迟线预设置入口按钮（切换显示/隐藏）
    void on_btnAddDelayPresetRowGalvo_clicked();      // 添加延迟预设行
    void on_btnClearDelayPresetsGalvo_clicked();      // 清空所有延迟预设
    void on_btnMoveUpDelayPresetGalvo_clicked();      // 上移延迟预设行
    void on_btnMoveDownDelayPresetGalvo_clicked();    // 下移延迟预设行
    void on_btnDeleteDelayPresetGalvo_clicked();      // 删除延迟预设行
    void on_btnStartDelayExecutionGalvo_clicked();    // 开始执行延迟预设
    void on_btnStopDelayExecutionGalvo_clicked();     // 停止执行延迟预设
    void onDelayPresetDelayTimeoutGalvo();            // 延迟预设间隔定时器触发
    
    // 预设管理槽函数 - 位移台页（电控与功率预设）
    void on_btnStagePowerPreset_clicked();       // 电控平台与功率预设置入口按钮（切换显示/隐藏）
    void on_btnAddPowerPresetRowStage_clicked();      // 添加功率预设行
    void on_btnClearPowerPresetsStage_clicked();      // 清空所有功率预设
    void on_btnMoveUpPowerPresetStage_clicked();      // 上移功率预设行
    void on_btnMoveDownPowerPresetStage_clicked();    // 下移功率预设行
    void on_btnDeletePowerPresetStage_clicked();      // 删除功率预设行
    void on_btnStartPowerExecutionStage_clicked();    // 开始执行功率预设
    void on_btnStopPowerExecutionStage_clicked();     // 停止执行功率预设
    void onPowerPresetDelayTimeoutStage();            // 功率预设间隔定时器触发
    void on_btnConfirmStagePreset_clicked();          // 确定按钮（位移台页底部执行按钮）
    
    // 预设管理槽函数 - 位移台页（延迟线预设）
    void on_btnStageDelayPreset_clicked();       // 延迟线预设置入口按钮（切换显示/隐藏）
    void on_btnAddDelayPresetRow_clicked();      // 添加延迟预设行
    void on_btnClearDelayPresets_clicked();      // 清空所有延迟预设
    void on_btnMoveUpDelayPreset_clicked();      // 上移延迟预设行
    void on_btnMoveDownDelayPreset_clicked();    // 下移延迟预设行
    void on_btnDeleteDelayPreset_clicked();      // 删除延迟预设行
    void on_btnStartDelayExecution_clicked();    // 开始执行延迟预设
    void on_btnStopDelayExecution_clicked();     // 停止执行延迟预设
    void onDelayPresetDelayTimeout();            // 延迟预设间隔定时器触发
    
    // 设备状态更新槽函数
    void onSeedLaserStatusChanged(DeviceStatus status);
    void onFOPOLaserStatusChanged(DeviceStatus status);
    void onStokesLaserStatusChanged(DeviceStatus status);
    void onSpectrometerStatusChanged(DeviceStatus status);
    void onStageStatusChanged(DeviceStatus status);
    void onGalvoStatusChanged(DeviceStatus status);
    void onDelayStatusChanged(DeviceStatus status);
    
    // 光谱仪数据接收槽函数
    void onSpectrumDataReady(const QVector<int> &intensity);
    
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
    SerialConfig getSeedLaserSerialConfig();
    SerialConfig getFOPOLaserSerialConfig();
    SerialConfig getStokesLaserSerialConfig();
    SerialConfig getSpectrometerSerialConfig();
    SerialConfig getDelayLineSerialConfig();
    SerialConfig getStageSerialConfig();
    
    // 振镜网络配置获取函数
    QString getGalvoIPAddress();
    
    // 光谱测量相关函数
    void startMeasurement();
    void stopMeasurement();
    void saveSpectrum();
    
    // 图表相关函数
    void initSpectrumChart();
    void updateSpectrum(const QVector<int> &intensity);
    void showChartMaximized();  // 显示最大化图表窗口
    
    // 峰值检测相关函数
    void detectPeaks();
    void updatePeaksTable();
    void exportPeaksToCSV();
    double calculateFWHM(int peakIndex, const QVector<int> &data);
    
    // 配置保存/加载辅助函数
    void saveSerialConfig(const QString &device, const QString &port, 
                         qint32 baudRate, int dataBits, int stopBits, int parity);
    void loadSerialConfig(const QString &device, QString &port, 
                         qint32 &baudRate, int &dataBits, int &stopBits, int &parity);
    
    // 预设管理辅助函数
    void initPresetTables();                     // 初始化预设表格
    void addPowerPresetRow();                    // 添加功率预设行
    void addPowerPresetRowStage();               // 添加功率预设行（位移台页）
    void removePowerPresetRow(int row);          // 删除功率预设行
    void removePowerPresetRowStage(int row);     // 删除功率预设行（位移台页）
    
    // 表格操作辅助函数
    void moveRowUp(QTableWidget *table, int row);      // 上移行
    void moveRowDown(QTableWidget *table, int row);    // 下移行
    void deleteRow(QTableWidget *table, int row);      // 删除行
    void swapRows(QTableWidget *table, int row1, int row2);  // 交换两行
    void updateRowNumbers(QTableWidget *table);        // 更新序号
    
    // 预设置显示控制辅助函数
    void updateGalvoConfirmButtonVisibility();         // 更新振镜页底部按钮显示状态
    void updateStageConfirmButtonVisibility();         // 更新位移台页底部按钮显示状态
    QList<PowerPreset> loadPowerPresetsFromTable();  // 从表格加载功率预设
    QList<StagePowerPreset> loadPowerPresetsFromTableStage();  // 从表格加载功率预设（位移台页）
    void executePowerPreset(int index);          // 执行单个功率预设
    void executePowerPresetStage(int index);     // 执行单个功率预设（位移台页）
    void executeCombinedPresetGalvo(int index);  // 执行组合预设（振镜页：功率+延迟线）
    void executeCombinedPresetStage(int index);  // 执行组合预设（位移台页：电控+功率+延迟线）
    void stopPowerPresetExecution();             // 停止功率预设执行
    void stopPowerPresetExecutionStage();        // 停止功率预设执行（位移台页）
    
    // 统一的延迟预设方法
    void addDelayPresetRow(PresetPageType pageType);
    void removeDelayPresetRow(PresetPageType pageType, int row);
    QList<DelayPreset> loadDelayPresetsFromTable(PresetPageType pageType);
    void executeDelayPreset(PresetPageType pageType, int index);
    void stopDelayPresetExecution(PresetPageType pageType);

protected:
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;  // 事件过滤器

private:
    Ui::Integration *ui;
    
    // 设备模块
    LaserDriver *m_seedLaserDriver;    // 种子源激光器
    LaserDriver *m_fopoLaserDriver;    // FOPO激光器
    LaserDriver *m_stokesLaserDriver;  // Stokes激光器
    Spectrometer *m_spectrometer;
    StageController *m_stageController;
    GalvoMirror *m_galvoMirror;        // 振镜控制卡
    DelayLine *m_delayLine;
    
    // 工具模块
    ConfigManager *m_configManager;
    DataManager *m_dataManager;
    CSVExporter *m_csvExporter;
    ImageSaver *m_imageSaver;
    PresetManager *m_presetManager;
    
    // 光谱仪测量状态
    bool m_isContinuousMeasuring;
    bool m_isMeasuring;  // 是否正在测量
    QTimer *m_measureTimer;
    
    // 图表相关
    QChartView *m_chartView;
    QChart *m_chart;
    QLineSeries *m_series;
    QValueAxis *m_axisX;
    QValueAxis *m_axisY;
    class QDialog *m_chartMaximizedDialog;  // 图表最大化窗口
    
    // 峰值检测相关
    struct PeakData {
        double wavelength;  // 波长
        int intensity;      // 强度
        double fwhm;        // 半高全宽
        int pixelIndex;     // 像素索引
    };
    QVector<PeakData> m_peaks;
    QVector<int> m_lastSpectrumData;  // 保存最后一次的光谱数据
    
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
    
    // 预设执行相关 - 功率预设（位移台页）
    class QTableWidget *m_powerPresetTableStage;      // 功率预设表格（位移台页）
    QTimer *m_powerPresetDelayTimerStage;             // 功率预设间隔定时器（位移台页）
    QList<StagePowerPreset> m_currentPowerPresetsStage;  // 当前执行的功率预设列表（位移台页）
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
    
    // 统一的激光器连接/断开方法（三个激光器是独立设备）
    void connectLaser(LaserDeviceType type);
    void disconnectLaser(LaserDeviceType type);
    
    // 统一的泵功率设置方法（每个泵只控制对应的激光器）
    void setPumpCurrent(LaserDeviceType type, class QLineEdit *inputField, class QLineEdit *syncField = nullptr);
    
    // 统一的延迟线设置方法
    void setDelayTime(class QLineEdit *inputField, class QLineEdit *syncField = nullptr);
};

#endif // INTEGRATION_H

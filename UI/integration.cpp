#include "integration.h"
#include "ui_integration.h"
#include "Communication/serial_port_base.h"
#include "../LaserDriver/ohld_protocol.h"
#include <QMessageBox>
#include <QDebug>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QComboBox>
#include <QLabel>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>
#include <QFileDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialog>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QLineEdit>
#include <QPixmap>
#include <QLayoutItem>
#include <QTextStream>
#include <QDoubleValidator>
#include <QEvent>
#include <QMouseEvent>
#include <QThread>
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QtCharts/QChartView>

Integration::Integration(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Integration)
    , m_spectrometerFOPO(nullptr)
    , m_spectrometerStokes(nullptr)
    , m_stageController(nullptr)
    , m_galvoMirror(nullptr)
    , m_delayLine(nullptr)
    , m_delayLine2(nullptr)
    , m_configManager(nullptr)
    , m_dataManager(nullptr)
    , m_csvExporter(nullptr)
    , m_imageSaver(nullptr)
    , m_presetManager(nullptr)
    , m_isContinuousMeasuringFOPO(false)
    , m_isMeasuringFOPO(false)
    , m_measureTimerFOPO(nullptr)
    , m_isContinuousMeasuringStokes(false)
    , m_isMeasuringStokes(false)
    , m_measureTimerStokes(nullptr)
    , m_chartViewFOPO(nullptr)
    , m_chartFOPO(nullptr)
    , m_seriesFOPO(nullptr)
    , m_axisXFOPO(nullptr)
    , m_axisYFOPO(nullptr)
    , m_chartMaximizedDialogFOPO(nullptr)
    , m_chartViewStokes(nullptr)
    , m_chartStokes(nullptr)
    , m_seriesStokes(nullptr)
    , m_axisXStokes(nullptr)
    , m_axisYStokes(nullptr)
    , m_chartMaximizedDialogStokes(nullptr)
    , m_chartViewFOPO_S(nullptr)
    , m_chartFOPO_S(nullptr)
    , m_seriesFOPO_S(nullptr)
    , m_chartViewStokes_S(nullptr)
    , m_chartStokes_S(nullptr)
    , m_seriesStokes_S(nullptr)
    , m_powerPresetTable(nullptr)
    , m_powerPresetDelayTimer(nullptr)
    , m_currentPowerPresetIndex(0)
    , m_isPowerPresetExecuting(false)
    , m_powerPresetTimeInterval(1)
    , m_delayPresetTableGalvo(nullptr)
    , m_delayPresetTable(nullptr)
    , m_delayPresetDelayTimerGalvo(nullptr)
    , m_delayPresetDelayTimer(nullptr)
    , m_currentDelayPresetIndexGalvo(0)
    , m_currentDelayPresetIndex(0)
    , m_isDelayPresetExecutingGalvo(false)
    , m_isDelayPresetExecuting(false)
    , m_delayPresetTimeIntervalGalvo(1)
    , m_delayPresetTimeInterval(1)
    , m_powerPresetTableStage(nullptr)
    , m_wavelengthTuningTable(nullptr)
    , m_powerPresetDelayTimerStage(nullptr)
    , m_currentPowerPresetIndexStage(0)
    , m_isPowerPresetExecutingStage(false)
    , m_powerPresetTimeIntervalStage(1)
{
    // 初始化四泵串口指针和状态
    for (int i = 0; i < 4; ++i) {
        m_ohldPumps[i] = nullptr;
    }

    ui->setupUi(this);

    // 初始化测量定时器
    m_measureTimerFOPO = new QTimer(this);
    m_measureTimerStokes = new QTimer(this);

    // 初始化预设定时器
    m_powerPresetDelayTimer = new QTimer(this);
    m_delayPresetDelayTimerGalvo = new QTimer(this);
    m_powerPresetDelayTimerStage = new QTimer(this);
    m_delayPresetDelayTimer = new QTimer(this);

    // 初始化
    initDevices();
    initUI();
    initConnections();
    loadConfiguration();

    updateStatusBar("程序已启动");

    // 窗口启动时自动最大化（使用延迟确保在事件循环开始后执行）
    QTimer::singleShot(0, this, [this]() {
        showMaximized();
    });
}

Integration::~Integration()
{
    // 清理设备
    delete m_spectrometerFOPO;
    delete m_spectrometerStokes;
    delete m_stageController;
    delete m_galvoMirror;
    delete m_delayLine;

    // 清理工具
    delete m_configManager;
    delete m_dataManager;
    delete m_csvExporter;
    delete m_imageSaver;
    delete m_presetManager;

    delete ui;
}

// ========== 初始化函数 ==========

void Integration::initDevices()
{
    // 创建光谱仪实例（两个独立的光谱仪）
    m_spectrometerFOPO = new Spectrometer(this);
    m_spectrometerStokes = new Spectrometer(this);

    // 创建其他设备实例
    m_stageController = new StageController(this);
    m_galvoMirror = new GalvoMirror(this);  // 创建振镜控制卡实例
    m_delayLine = new DelayLine(this);
    m_delayLine->setDeviceId(0x01);   // 延迟线1，设备ID=1
    m_delayLine2 = new DelayLine(this);
    m_delayLine2->setDeviceId(0x02);  // 延迟线2，设备ID=2，独立串口

    // 创建 OHLD 四泵独立串口实例
    for (int i = 0; i < 4; ++i) {
        m_ohldPumps[i] = new SerialPortBase(this);
    }

    // 创建工具实例
    m_configManager = new ConfigManager();
    m_dataManager = new DataManager(this);
    m_csvExporter = new CSVExporter(this);
    m_imageSaver = new ImageSaver(this);
    m_presetManager = new PresetManager(this);
}

void Integration::initUI()
{
    // 设置全局按钮样式（灰色背景，提升视觉效果）
    QString buttonStyle =
        "QPushButton {"
        "    background-color: #E0E0E0;"  // 浅灰色背景
        "    border: 1px solid #A0A0A0;"  // 深灰色边框
        "    border-radius: 3px;"         // 圆角
        "    padding: 5px 10px;"          // 内边距
        "    color: #000000;"             // 黑色文字
        "}"
        "QPushButton:hover {"
        "    background-color: #D0D0D0;"  // 鼠标悬停时稍深
        "}"
        "QPushButton:pressed {"
        "    background-color: #C0C0C0;"  // 按下时更深
        "}"
        "QPushButton:disabled {"
        "    background-color: #F0F0F0;"  // 禁用时更浅
        "    color: #A0A0A0;"             // 禁用时文字变灰
        "}";

    // 应用样式到所有按钮
    QList<QPushButton*> buttons = this->findChildren<QPushButton*>();
    for (QPushButton *btn : buttons) {
        btn->setStyleSheet(buttonStyle);
    }

    // 初始化串口下拉框
    initSerialPortCombos();

    // 初始化光谱图表
    initSpectrumChart();

    // 初始化预设表格
    initPresetTables();

    // 初始化预设置控件的显示状态（振镜页两表常显，位移台页波长调谐表常显）

    // 初始化所有状态指示器的大小和样式（统一为20x20的圆形指示灯）
    initStatusIndicators();

    // 设置所有输入框居中对齐
    // 振镜页输入框
    ui->lineEditTimeDelay->setAlignment(Qt::AlignCenter);
    ui->lineEditGalvoPump1->setAlignment(Qt::AlignCenter);
    ui->lineEditGalvoPump2->setAlignment(Qt::AlignCenter);
    ui->lineEditGalvoPump3->setAlignment(Qt::AlignCenter);
    ui->lineEditGalvoPump4->setAlignment(Qt::AlignCenter);

    // 振镜页打标控制输入框（按文档协议）
    ui->lineEditGalvoLaserPower->setAlignment(Qt::AlignCenter);
    ui->lineEditGalvoFrequency->setAlignment(Qt::AlignCenter);
    ui->lineEditGalvoDutyCycle->setAlignment(Qt::AlignCenter);
    ui->lineEditGalvoMarkV->setAlignment(Qt::AlignCenter);
    ui->lineEditGalvoJumpV->setAlignment(Qt::AlignCenter);
    ui->lineEditGalvoScanTimes->setAlignment(Qt::AlignCenter);
    ui->lineEditGalvoPointX->setAlignment(Qt::AlignCenter);
    ui->lineEditGalvoPointY->setAlignment(Qt::AlignCenter);
    ui->lineEditGalvoPointTime->setAlignment(Qt::AlignCenter);
    ui->lineEditGalvoLineStartX->setAlignment(Qt::AlignCenter);
    ui->lineEditGalvoLineStartY->setAlignment(Qt::AlignCenter);
    ui->lineEditGalvoLineEndX->setAlignment(Qt::AlignCenter);
    ui->lineEditGalvoLineEndY->setAlignment(Qt::AlignCenter);
    ui->lineEditGalvoCircleX->setAlignment(Qt::AlignCenter);
    ui->lineEditGalvoCircleY->setAlignment(Qt::AlignCenter);
    ui->lineEditGalvoCircleR->setAlignment(Qt::AlignCenter);

    // 位移台页输入框
    ui->lineEditStageSpeed->setAlignment(Qt::AlignCenter);
    ui->lineEditStageDisplace->setAlignment(Qt::AlignCenter);
    ui->lineEditStageDelayLine1->setAlignment(Qt::AlignCenter);
    ui->lineEditStageDelayLine2->setAlignment(Qt::AlignCenter);
    ui->lineEditStagePump1->setAlignment(Qt::AlignCenter);
    ui->lineEditStagePump2->setAlignment(Qt::AlignCenter);
    ui->lineEditStagePump3->setAlignment(Qt::AlignCenter);
    ui->lineEditStagePump4->setAlignment(Qt::AlignCenter);

    // 位移台页扫描输入框
    ui->lineEditStageScanWavelength->setAlignment(Qt::AlignCenter);
    ui->lineEditStageScanStart->setAlignment(Qt::AlignCenter);
    ui->lineEditStageScanEnd->setAlignment(Qt::AlignCenter);
    ui->lineEditStageScanStep->setAlignment(Qt::AlignCenter);
    ui->lineEditStageScanInterval->setAlignment(Qt::AlignCenter);

    // 更新连接状态
    updateConnectionStatus();
}

void Integration::initConnections()
{
    // 连接光谱仪信号 - FOPO路
    connect(m_spectrometerFOPO, &Spectrometer::statusChanged,
            this, &Integration::onSpectrometerFOPOStatusChanged);
    connect(m_spectrometerFOPO, &Spectrometer::errorOccurred,
            this, &Integration::onDeviceError);
    connect(m_spectrometerFOPO, &Spectrometer::spectrumDataReady,
            this, &Integration::onSpectrumDataReady);

    // 连接光谱仪信号 - Stokes路
    connect(m_spectrometerStokes, &Spectrometer::statusChanged,
            this, &Integration::onSpectrometerStokesStatusChanged);
    connect(m_spectrometerStokes, &Spectrometer::errorOccurred,
            this, &Integration::onDeviceError);
    connect(m_spectrometerStokes, &Spectrometer::spectrumDataReady,
            this, &Integration::onSpectrumDataReadyStokes);

    // 连接位移台信号（双串口聚合：分别处理两台位置变化与连接状态）
    connect(m_stageController, &StageController::errorOccurred,
            this, &Integration::onDeviceError);
    connect(m_stageController, &StageController::positionChanged1,
            this, &Integration::onStage1PositionChanged);
    connect(m_stageController, &StageController::positionChanged2,
            this, &Integration::onStage2PositionChanged);
    connect(m_stageController, &StageController::moveCompletedDual,
            this, &Integration::onStageMoveCompletedDual);
    connect(m_stageController, &StageController::stage1Connected, this, [this]() {
        updateStatusIndicator(ui->labelStage1Status, DeviceStatus::Connected);
    });
    connect(m_stageController, &StageController::stage1Disconnected, this, [this]() {
        updateStatusIndicator(ui->labelStage1Status, DeviceStatus::Disconnected);
    });
    connect(m_stageController, &StageController::stage2Connected, this, [this]() {
        updateStatusIndicator(ui->labelStage2Status, DeviceStatus::Connected);
    });
    connect(m_stageController, &StageController::stage2Disconnected, this, [this]() {
        updateStatusIndicator(ui->labelStage2Status, DeviceStatus::Disconnected);
    });

    // 连接振镜控制卡信号
    connect(m_galvoMirror, &GalvoMirror::statusChanged,
            this, &Integration::onGalvoStatusChanged);
    connect(m_galvoMirror, &GalvoMirror::errorOccurred,
            this, &Integration::onDeviceError);
    connect(m_galvoMirror, &GalvoMirror::messageLog,
            this, &Integration::onGalvoMessageLog);
    connect(m_galvoMirror, &GalvoMirror::heartbeatChanged,
            this, &Integration::onGalvoHeartbeatChanged);

    // 连接延时线1信号
    connect(m_delayLine, &DelayLine::statusChanged,
            this, &Integration::onDelayStatusChanged);
    connect(m_delayLine, &DelayLine::errorOccurred,
            this, &Integration::onDeviceError);
    connect(m_delayLine, &DelayLine::delayChanged,
            this, &Integration::onDelayChanged);

    // 连接延时线2信号
    connect(m_delayLine2, &DelayLine::statusChanged,
            this, &Integration::onDelayStatusChanged);
    connect(m_delayLine2, &DelayLine::errorOccurred,
            this, &Integration::onDeviceError);
    connect(m_delayLine2, &DelayLine::delayChanged,
            this, &Integration::onDelay2Changed);

    // 连接测量定时器
    connect(m_measureTimerFOPO, &QTimer::timeout,
            this, &Integration::onMeasureTimeoutFOPO);
    connect(m_measureTimerStokes, &QTimer::timeout,
            this, &Integration::onMeasureTimeoutStokes);

    // 连接预设定时器
    connect(m_powerPresetDelayTimer, &QTimer::timeout,
            this, &Integration::onPowerPresetDelayTimeout);
    connect(m_delayPresetDelayTimerGalvo, &QTimer::timeout,
            this, &Integration::onDelayPresetDelayTimeoutGalvo);
    connect(m_powerPresetDelayTimerStage, &QTimer::timeout,
            this, &Integration::onPowerPresetDelayTimeoutStage);
    connect(m_delayPresetDelayTimer, &QTimer::timeout,
            this, &Integration::onDelayPresetDelayTimeout);

    // 连接编辑按钮信号槽
    connect(ui->btnEditPowerPresets, &QPushButton::clicked,
            this, &Integration::on_btnEditPowerPresets_clicked);
    connect(ui->btnEditDelayPresetsGalvo, &QPushButton::clicked,
            this, &Integration::on_btnEditDelayPresetsGalvo_clicked);
    connect(ui->btnEditWavelengthTuningStage, &QPushButton::clicked,
            this, &Integration::on_btnEditWavelengthTuningStage_clicked);

    // 连接位移台页 _S 光谱按钮到同一业务槽
    connect(ui->btnSingleMeasureFOPO_S,     &QPushButton::clicked, this, &Integration::on_btnSingleMeasureFOPO_clicked);
    connect(ui->btnContinuousMeasureFOPO_S, &QPushButton::clicked, this, &Integration::on_btnContinuousMeasureFOPO_clicked);
    connect(ui->btnStopMeasureFOPO_S,       &QPushButton::clicked, this, &Integration::on_btnStopMeasureFOPO_clicked);
    connect(ui->btnSavePlotFOPO_S,          &QPushButton::clicked, this, &Integration::on_btnSavePlotFOPO_clicked);
    connect(ui->btnResetViewFOPO_S,         &QPushButton::clicked, this, &Integration::on_btnResetViewFOPO_clicked);
    connect(ui->btnClearPlotFOPO_S,         &QPushButton::clicked, this, &Integration::on_btnClearPlotFOPO_clicked);
    connect(ui->btnShowPeaksFOPO_S,         &QPushButton::clicked, this, &Integration::on_btnShowPeaksFOPO_clicked);

    connect(ui->btnSingleMeasureStokes_S,     &QPushButton::clicked, this, &Integration::on_btnSingleMeasureStokes_clicked);
    connect(ui->btnContinuousMeasureStokes_S, &QPushButton::clicked, this, &Integration::on_btnContinuousMeasureStokes_clicked);
    connect(ui->btnStopMeasureStokes_S,       &QPushButton::clicked, this, &Integration::on_btnStopMeasureStokes_clicked);
    connect(ui->btnSavePlotStokes_S,          &QPushButton::clicked, this, &Integration::on_btnSavePlotStokes_clicked);
    connect(ui->btnResetViewStokes_S,         &QPushButton::clicked, this, &Integration::on_btnResetViewStokes_clicked);
    connect(ui->btnClearPlotStokes_S,         &QPushButton::clicked, this, &Integration::on_btnClearPlotStokes_clicked);
    connect(ui->btnShowPeaksStokes_S,         &QPushButton::clicked, this, &Integration::on_btnShowPeaksStokes_clicked);
}

void Integration::loadConfiguration()
{
    // TODO: 从配置文件加载设置
    qDebug() << "加载配置...";
}

void Integration::saveConfiguration()
{
    // TODO: 保存设置到配置文件
    qDebug() << "保存配置...";
}

void Integration::closeEvent(QCloseEvent *event)
{
    saveConfiguration();
    QMainWindow::closeEvent(event);
}

bool Integration::eventFilter(QObject *obj, QEvent *event)
{
    // 检测FOPO路图表视图的双击事件（振镜页 + 位移台页 _S 镜像）
    if ((obj == m_chartViewFOPO || obj == m_chartViewFOPO_S)
        && event->type() == QEvent::MouseButtonDblClick) {
        showChartMaximized();
        return true;
    }

    // 检测Stokes路图表视图的双击事件（振镜页 + 位移台页 _S 镜像）
    if ((obj == m_chartViewStokes || obj == m_chartViewStokes_S)
        && event->type() == QEvent::MouseButtonDblClick) {
        showChartMaximizedStokes();
        return true;
    }

    // 传递给基类处理其他事件
    return QMainWindow::eventFilter(obj, event);
}


// ========== UI初始化辅助函数 ==========

void Integration::initSerialPortCombos()
{
    // 初始化所有串口下拉框
    populateSerialPortCombo(ui->comboBoxSpectrometerFOPOPort);
    populateSerialPortCombo(ui->comboBoxSpectrometerStokesPort);
    populateSerialPortCombo(ui->comboBoxStage1Port);
    populateSerialPortCombo(ui->comboBoxStage2Port);
    populateSerialPortCombo(ui->comboBoxDelayPort);
    populateSerialPortCombo(ui->comboBoxDelay2Port);
    populateSerialPortCombo(ui->comboBoxPump1Port);
    populateSerialPortCombo(ui->comboBoxPump2Port);
    populateSerialPortCombo(ui->comboBoxPump3Port);
    populateSerialPortCombo(ui->comboBoxPump4Port);

    // 初始化波特率下拉框
    populateBaudRateCombo(ui->comboBoxSpectrometerFOPOBaudRate);
    populateBaudRateCombo(ui->comboBoxSpectrometerStokesBaudRate);
    populateBaudRateCombo(ui->comboBoxStage1BaudRate);
    populateBaudRateCombo(ui->comboBoxStage2BaudRate);
    populateBaudRateCombo(ui->comboBoxDelayBaudRate);
    populateBaudRateCombo(ui->comboBoxDelay2BaudRate);
    populateBaudRateCombo(ui->comboBoxPump1BaudRate);
    populateBaudRateCombo(ui->comboBoxPump2BaudRate);
    populateBaudRateCombo(ui->comboBoxPump3BaudRate);
    populateBaudRateCombo(ui->comboBoxPump4BaudRate);

    // 设置默认波特率
    ui->comboBoxSpectrometerFOPOBaudRate->setCurrentText("115200");
    ui->comboBoxSpectrometerStokesBaudRate->setCurrentText("115200");
    ui->comboBoxStage1BaudRate->setCurrentText("9600");
    ui->comboBoxStage2BaudRate->setCurrentText("9600");
    ui->comboBoxDelayBaudRate->setCurrentText("9600");
    ui->comboBoxDelay2BaudRate->setCurrentText("9600");
    ui->comboBoxPump1BaudRate->setCurrentText("9600");
    ui->comboBoxPump2BaudRate->setCurrentText("9600");
    ui->comboBoxPump3BaudRate->setCurrentText("9600");
    ui->comboBoxPump4BaudRate->setCurrentText("9600");
}

void Integration::refreshSerialPorts()
{
    // TODO: 刷新可用串口列表
    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    qDebug() << "可用串口数量:" << ports.size();
}

void Integration::populateSerialPortCombo(QComboBox *combo)
{
    if (!combo) return;

    combo->clear();
    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : ports) {
        combo->addItem(info.portName());
    }
}

void Integration::populateBaudRateCombo(QComboBox *combo)
{
    if (!combo) return;

    combo->clear();
    combo->addItem("9600", 9600);
    combo->addItem("19200", 19200);
    combo->addItem("38400", 38400);
    combo->addItem("57600", 57600);
    combo->addItem("115200", 115200);
}

void Integration::populateDataBitsCombo(QComboBox *combo)
{
    if (!combo) return;

    combo->clear();
    combo->addItem("5", QSerialPort::Data5);
    combo->addItem("6", QSerialPort::Data6);
    combo->addItem("7", QSerialPort::Data7);
    combo->addItem("8", QSerialPort::Data8);
}

void Integration::populateStopBitsCombo(QComboBox *combo)
{
    if (!combo) return;

    combo->clear();
    combo->addItem("1", QSerialPort::OneStop);
    combo->addItem("1.5", QSerialPort::OneAndHalfStop);
    combo->addItem("2", QSerialPort::TwoStop);
}

void Integration::populateParityCombo(QComboBox *combo)
{
    if (!combo) return;

    combo->clear();
    combo->addItem("None", QSerialPort::NoParity);
    combo->addItem("Even", QSerialPort::EvenParity);
    combo->addItem("Odd", QSerialPort::OddParity);
    combo->addItem("Space", QSerialPort::SpaceParity);
    combo->addItem("Mark", QSerialPort::MarkParity);
}

void Integration::updateConnectionStatus()
{
    // TODO: 更新所有设备的连接状态指示器
}

void Integration::updateStatusBar(const QString &message)
{
    statusBar()->showMessage(message, 3000);
}

void Integration::initStatusIndicators()
{
    // 初始化所有状态指示器为统一的大小和默认状态（灰色-未连接）
    // 这样确保程序启动时所有指示灯都是统一的20x20大小
    updateStatusIndicator(ui->labelSpectrometerFOPOStatus, DeviceStatus::Disconnected);
    updateStatusIndicator(ui->labelSpectrometerStokesStatus, DeviceStatus::Disconnected);
    updateStatusIndicator(ui->labelStage1Status, DeviceStatus::Disconnected);
    updateStatusIndicator(ui->labelStage2Status, DeviceStatus::Disconnected);
    updateStatusIndicator(ui->labelGalvoStatus, DeviceStatus::Disconnected);
    updateStatusIndicator(ui->labelDelayStatus, DeviceStatus::Disconnected);

    qDebug() << "状态指示器初始化完成";
}

void Integration::updateStatusIndicator(QLabel *indicator, DeviceStatus status)
{
    if (!indicator) return;

    // 设置固定大小（圆形指示灯）
    indicator->setFixedSize(20, 20);
    indicator->setText("");  // 不显示文字

    switch (status) {
        case DeviceStatus::Connected:
            // 绿色 - 已连接
            indicator->setStyleSheet("QLabel { background-color: #00FF00; border-radius: 10px; border: 1px solid #333; }");
            indicator->setToolTip("已连接");
            break;
        case DeviceStatus::Disconnected:
            // 灰色 - 未连接
            indicator->setStyleSheet("QLabel { background-color: #808080; border-radius: 10px; border: 1px solid #333; }");
            indicator->setToolTip("未连接");
            break;
        case DeviceStatus::Error:
            // 红色 - 错误/断开
            indicator->setStyleSheet("QLabel { background-color: #FF0000; border-radius: 10px; border: 1px solid #333; }");
            indicator->setToolTip("错误");
            break;
        case DeviceStatus::Busy:
            // 橙色 - 忙碌
            indicator->setStyleSheet("QLabel { background-color: #FFA500; border-radius: 10px; border: 1px solid #333; }");
            indicator->setToolTip("忙碌");
            break;
        case DeviceStatus::Connecting:
            // 黄色 - 连接中
            indicator->setStyleSheet("QLabel { background-color: #FFFF00; border-radius: 10px; border: 1px solid #333; }");
            indicator->setToolTip("连接中");
            break;
        case DeviceStatus::Ready:
            // 浅绿色 - 就绪
            indicator->setStyleSheet("QLabel { background-color: #90EE90; border-radius: 10px; border: 1px solid #333; }");
            indicator->setToolTip("就绪");
            break;
    }
}


// ========== 光谱仪连接/断开槽函数 ==========

void Integration::on_btnConnectSpectrometerFOPO_clicked()
{
    SerialConfig config = getSpectrometerFOPOSerialConfig();

    // 检查是否选择了串口
    if (config.portName.isEmpty()) {
        QMessageBox::warning(this, "连接失败",
            "光谱仪：请先选择串口设备！\n\n请在串口下拉框中选择一个可用的串口。");
        return;
    }

    // 显示连接中状态
    updateStatusIndicator(ui->labelSpectrometerFOPOStatus, DeviceStatus::Connecting);
    updateStatusBar("光谱仪正在连接...");

    // 处理 UI 事件
    QCoreApplication::processEvents();

    // 使用 QTimer 异步执行连接操作
    QTimer::singleShot(50, this, [this, config]() {
        // 设置串口参数
        m_spectrometerFOPO->setPortName(config.portName);
        m_spectrometerFOPO->setBaudRate(config.baudRate);
        m_spectrometerFOPO->setDataBits(static_cast<int>(config.dataBits));
        m_spectrometerFOPO->setStopBits(static_cast<int>(config.stopBits));
        m_spectrometerFOPO->setParity(static_cast<int>(config.parity));

        // 处理 UI 事件
        QCoreApplication::processEvents();

        if (m_spectrometerFOPO->connect()) {
            updateStatusBar("光谱仪连接成功");
            updateStatusIndicator(ui->labelSpectrometerFOPOStatus, DeviceStatus::Connected);

            QString info = QString("像素数: %1, 序列号: %2")
                           .arg(m_spectrometerFOPO->getPixelLength())
                           .arg(m_spectrometerFOPO->getSerialNumber());
            qDebug() << "光谱仪信息:" << info;
        } else {
            updateStatusBar("光谱仪连接失败");
            updateStatusIndicator(ui->labelSpectrometerFOPOStatus, DeviceStatus::Error);
            QMessageBox::warning(this, "连接失败", m_spectrometerFOPO->getLastError());
        }
    });
}

void Integration::on_btnDisconnectSpectrometerFOPO_clicked()
{
    m_spectrometerFOPO->disconnect();
    updateStatusIndicator(ui->labelSpectrometerFOPOStatus, DeviceStatus::Disconnected);
    updateStatusBar("光谱仪已断开");
}

// ========== 位移台连接/断开槽函数（双串口） ==========

void Integration::on_btnConnectStage1_clicked()
{
    SerialConfig config = getStageSerialConfig1();

    if (config.portName.isEmpty()) {
        QMessageBox::warning(this, "连接失败",
            "位移台1：请先选择串口设备！\n\n请在串口下拉框中选择一个可用的串口。");
        return;
    }

    updateStatusIndicator(ui->labelStage1Status, DeviceStatus::Connecting);
    updateStatusBar("位移台1 正在连接...");
    QCoreApplication::processEvents();

    QTimer::singleShot(50, this, [this, config]() {
        bool success = m_stageController->openPort1(
            config.portName,
            config.baudRate,
            config.dataBits,
            config.parity,
            config.stopBits
        );
        QCoreApplication::processEvents();

        if (success) {
            updateStatusBar("位移台1 连接成功");
            updateStatusIndicator(ui->labelStage1Status, DeviceStatus::Connected);
            qDebug() << "位移台1 连接成功";
        } else {
            updateStatusBar("位移台1 连接失败");
            updateStatusIndicator(ui->labelStage1Status, DeviceStatus::Error);
            qDebug() << "位移台1 连接失败：" << m_stageController->getLastError();
            QMessageBox::warning(this, "连接失败",
                "位移台1 连接失败：" + m_stageController->getLastError());
        }
    });
}

void Integration::on_btnDisconnectStage1_clicked()
{
    m_stageController->closePort1();
    updateStatusIndicator(ui->labelStage1Status, DeviceStatus::Disconnected);
    updateStatusBar("位移台1 已断开");
}

void Integration::on_btnConnectStage2_clicked()
{
    SerialConfig config = getStageSerialConfig2();

    if (config.portName.isEmpty()) {
        QMessageBox::warning(this, "连接失败",
            "位移台2：请先选择串口设备！\n\n请在串口下拉框中选择一个可用的串口。");
        return;
    }

    updateStatusIndicator(ui->labelStage2Status, DeviceStatus::Connecting);
    updateStatusBar("位移台2 正在连接...");
    QCoreApplication::processEvents();

    QTimer::singleShot(50, this, [this, config]() {
        bool success = m_stageController->openPort2(
            config.portName,
            config.baudRate,
            config.dataBits,
            config.parity,
            config.stopBits
        );
        QCoreApplication::processEvents();

        if (success) {
            updateStatusBar("位移台2 连接成功");
            updateStatusIndicator(ui->labelStage2Status, DeviceStatus::Connected);
            qDebug() << "位移台2 连接成功";
        } else {
            updateStatusBar("位移台2 连接失败");
            updateStatusIndicator(ui->labelStage2Status, DeviceStatus::Error);
            qDebug() << "位移台2 连接失败：" << m_stageController->getLastError();
            QMessageBox::warning(this, "连接失败",
                "位移台2 连接失败：" + m_stageController->getLastError());
        }
    });
}

void Integration::on_btnDisconnectStage2_clicked()
{
    m_stageController->closePort2();
    updateStatusIndicator(ui->labelStage2Status, DeviceStatus::Disconnected);
    updateStatusBar("位移台2 已断开");
}

// ========== 振镜连接/断开槽函数 ==========

void Integration::on_btnConnectGalvo_clicked()
{
    // 获取IP地址配置
    QString ipAddress = getGalvoIPAddress();

    // 检查IP地址是否有效
    if (ipAddress.isEmpty()) {
        QMessageBox::warning(this, "连接失败",
            "振镜控制卡：请先输入IP地址！\n\n默认IP地址为：172.18.34.227");
        return;
    }

    // 显示连接中状态
    updateStatusIndicator(ui->labelGalvoStatus, DeviceStatus::Connecting);
    updateStatusBar("振镜控制卡正在连接...");

    // 处理 UI 事件
    QCoreApplication::processEvents();

    // 使用 QTimer 异步执行连接操作
    QTimer::singleShot(50, this, [this, ipAddress]() {
        // 通过IP地址连接（内部启动 UDP 心跳并连接 TCP 6002/6003）
        bool success = m_galvoMirror->connectByIP(ipAddress);

        // 处理 UI 事件
        QCoreApplication::processEvents();

        if (success && m_galvoMirror->isConnected()) {
            updateStatusBar("振镜控制卡连接成功");
            updateStatusIndicator(ui->labelGalvoStatus, DeviceStatus::Connected);
            qDebug() << "振镜控制卡连接成功，IP:" << ipAddress;
        } else {
            updateStatusBar("振镜控制卡连接失败");
            updateStatusIndicator(ui->labelGalvoStatus, DeviceStatus::Error);
            qDebug() << "振镜控制卡连接失败";
            QMessageBox::warning(this, "连接失败",
                QString("振镜控制卡连接失败\nIP地址：%1\n\n请检查：\n"
                        "1. 控制卡是否上电\n"
                        "2. 网线是否连接\n"
                        "3. 电脑IP是否在172.18.34.2~123范围内\n"
                        "4. IP地址是否正确").arg(ipAddress));
        }
    });
}

void Integration::on_btnDisconnectGalvo_clicked()
{
    m_galvoMirror->disconnect();
    updateStatusIndicator(ui->labelGalvoStatus, DeviceStatus::Disconnected);
    updateStatusBar("振镜控制卡已断开");
}

// ========== 延时线连接/断开槽函数 ==========

void Integration::on_btnConnectDelay_clicked()
{
    SerialConfig config = getDelayLineSerialConfig();

    // 检查是否选择了串口
    if (config.portName.isEmpty()) {
        QMessageBox::warning(this, "连接失败",
            "延时线：请先选择串口设备！\n\n请在串口下拉框中选择一个可用的串口。");
        return;
    }

    // 显示连接中状态
    updateStatusIndicator(ui->labelDelayStatus, DeviceStatus::Connecting);
    updateStatusBar("延时线正在连接...");

    // 处理 UI 事件
    QCoreApplication::processEvents();

    // 使用 QTimer 异步执行连接操作
    QTimer::singleShot(50, this, [this, config]() {
        bool success = m_delayLine->openPort(
            config.portName,
            config.baudRate,
            config.dataBits,
            config.parity,
            config.stopBits
        );

        // 处理 UI 事件
        QCoreApplication::processEvents();

        if (success) {
            // openPort 已成功打开串口；DeviceBase::isConnected() 会基于
            // SerialPortBase::isOpen() 返回 true，无需再调用 connect() 重开。
            updateStatusBar("延时线连接成功");
            updateStatusIndicator(ui->labelDelayStatus, DeviceStatus::Connected);
            qDebug() << "延时线连接成功";
        } else {
            updateStatusBar("延时线连接失败");
            updateStatusIndicator(ui->labelDelayStatus, DeviceStatus::Error);
            qDebug() << "延时线连接失败：" << m_delayLine->getLastError();
            QMessageBox::warning(this, "连接失败", "延时线连接失败：" + m_delayLine->getLastError());
        }
    });
}

void Integration::on_btnDisconnectDelay_clicked()
{
    m_delayLine->disconnect();
    updateStatusIndicator(ui->labelDelayStatus, DeviceStatus::Disconnected);
    updateStatusBar("延时线1已断开");
}

void Integration::on_btnConnectDelay2_clicked()
{
    SerialConfig config = getDelayLineSerialConfig2();

    if (config.portName.isEmpty()) {
        QMessageBox::warning(this, "连接失败",
            "延时线2：请先选择串口设备！");
        return;
    }

    updateStatusIndicator(ui->labelDelay2Status, DeviceStatus::Connecting);
    updateStatusBar("延时线2正在连接...");
    QCoreApplication::processEvents();

    QTimer::singleShot(50, this, [this, config]() {
        bool success = m_delayLine2->openPort(
            config.portName, config.baudRate,
            config.dataBits, config.parity, config.stopBits
        );
        QCoreApplication::processEvents();

        if (success) {
            updateStatusBar("延时线2连接成功");
            updateStatusIndicator(ui->labelDelay2Status, DeviceStatus::Connected);
            qDebug() << "延时线2连接成功";
        } else {
            updateStatusBar("延时线2连接失败");
            updateStatusIndicator(ui->labelDelay2Status, DeviceStatus::Error);
            QMessageBox::warning(this, "连接失败", "延时线2连接失败：" + m_delayLine2->getLastError());
        }
    });
}

void Integration::on_btnDisconnectDelay2_clicked()
{
    m_delayLine2->disconnect();
    updateStatusIndicator(ui->labelDelay2Status, DeviceStatus::Disconnected);
    updateStatusBar("延时线2已断开");
}

// ========== OHLD 四泵连接/断开槽函数 ==========

// ========== OHLD 四泵 UI 辅助查找（去掉 static lambda 捕获 this 的隐患）==========

QComboBox *Integration::ohldPumpPortCombo(int pumpIndex) const
{
    switch (pumpIndex) {
        case 0: return ui->comboBoxPump1Port;
        case 1: return ui->comboBoxPump2Port;
        case 2: return ui->comboBoxPump3Port;
        case 3: return ui->comboBoxPump4Port;
        default: return nullptr;
    }
}

QComboBox *Integration::ohldPumpBaudCombo(int pumpIndex) const
{
    switch (pumpIndex) {
        case 0: return ui->comboBoxPump1BaudRate;
        case 1: return ui->comboBoxPump2BaudRate;
        case 2: return ui->comboBoxPump3BaudRate;
        case 3: return ui->comboBoxPump4BaudRate;
        default: return nullptr;
    }
}

QLabel *Integration::ohldPumpStatusLabel(int pumpIndex) const
{
    switch (pumpIndex) {
        case 0: return ui->labelPump1ConnStatus;
        case 1: return ui->labelPump2ConnStatus;
        case 2: return ui->labelPump3ConnStatus;
        case 3: return ui->labelPump4ConnStatus;
        default: return nullptr;
    }
}

void Integration::galvoAngleToCoord(float angleDeg, float &x, float &y, float &z) const
{
    // TODO: 替换为实际标定公式或查表。当前为线性占位映射。
    x = angleDeg * 10.0f;
    y = 0.0f;
    z = 0.0f;
}

void Integration::connectOhldPump(int pumpIndex)
{
    if (pumpIndex < 0 || pumpIndex >= 4) return;

    static const char *pumpNames[4] = {
        "种子源泵", "pump路预放泵", "pump路主级泵", "Stokes路泵"
    };

    QComboBox *portCombo = ohldPumpPortCombo(pumpIndex);
    QComboBox *baudCombo = ohldPumpBaudCombo(pumpIndex);
    QLabel    *statusLabel = ohldPumpStatusLabel(pumpIndex);
    if (!portCombo || !baudCombo || !statusLabel) return;

    QString portName = portCombo->currentText();
    if (portName.isEmpty()) {
        QMessageBox::warning(this, "连接失败",
            QString::fromUtf8(pumpNames[pumpIndex]) + "：请先选择串口设备！");
        return;
    }
    qint32 baudRate = baudCombo->currentData().toInt();
    if (baudRate <= 0) baudRate = 9600;

    updateStatusIndicator(statusLabel, DeviceStatus::Connecting);
    updateStatusBar(QString::fromUtf8(pumpNames[pumpIndex]) + "正在连接...");
    QCoreApplication::processEvents();

    QTimer::singleShot(50, this, [this, pumpIndex, portName, baudRate, statusLabel]() {
        if (!m_ohldPumps[pumpIndex]) return;
        bool ok = m_ohldPumps[pumpIndex]->openPort(
            portName, baudRate,
            QSerialPort::Data8, QSerialPort::NoParity, QSerialPort::OneStop
        );
        static const char *names[4] = {
            "种子源泵", "pump路预放泵", "pump路主级泵", "Stokes路泵"
        };
        if (ok) {
            updateStatusIndicator(statusLabel, DeviceStatus::Connected);
            updateStatusBar(QString::fromUtf8(names[pumpIndex]) + "连接成功");
            qDebug() << QString::fromUtf8(names[pumpIndex]) << "连接成功:" << portName;
        } else {
            updateStatusIndicator(statusLabel, DeviceStatus::Error);
            updateStatusBar(QString::fromUtf8(names[pumpIndex]) + "连接失败");
            QMessageBox::warning(this, "连接失败",
                QString::fromUtf8(names[pumpIndex]) + "连接失败：无法打开串口 " + portName);
        }
    });
}

void Integration::disconnectOhldPump(int pumpIndex)
{
    if (pumpIndex < 0 || pumpIndex >= 4) return;

    static const char *pumpNames[4] = {
        "种子源泵", "pump路预放泵", "pump路主级泵", "Stokes路泵"
    };

    QLabel *statusLabel = ohldPumpStatusLabel(pumpIndex);

    if (m_ohldPumps[pumpIndex] && m_ohldPumps[pumpIndex]->isOpen()) {
        // 先发送关闭指令
        m_ohldPumps[pumpIndex]->writeData(ohldBuildFrame(OHLD_CMD_CLOSE));
        QThread::msleep(100);
        m_ohldPumps[pumpIndex]->closePort();
    }
    if (statusLabel) {
        updateStatusIndicator(statusLabel, DeviceStatus::Disconnected);
    }
    updateStatusBar(QString::fromUtf8(pumpNames[pumpIndex]) + "已断开");
    qDebug() << QString::fromUtf8(pumpNames[pumpIndex]) << "已断开";
}

void Integration::on_btnConnectPump1_clicked()    { connectOhldPump(0); }
void Integration::on_btnDisconnectPump1_clicked() { disconnectOhldPump(0); }
void Integration::on_btnConnectPump2_clicked()    { connectOhldPump(1); }
void Integration::on_btnDisconnectPump2_clicked() { disconnectOhldPump(1); }
void Integration::on_btnConnectPump3_clicked()    { connectOhldPump(2); }
void Integration::on_btnDisconnectPump3_clicked() { disconnectOhldPump(2); }
void Integration::on_btnConnectPump4_clicked()    { connectOhldPump(3); }
void Integration::on_btnDisconnectPump4_clicked() { disconnectOhldPump(3); }

// ========== 光谱测量控制槽函数 ==========

void Integration::on_btnSingleMeasureFOPO_clicked()
{
    if (!m_spectrometerFOPO->isConnected()) {
        QMessageBox::warning(this, "错误", "请先连接光谱仪");
        return;
    }

    startMeasurement();
}

void Integration::on_btnContinuousMeasureFOPO_clicked()
{
    if (!m_spectrometerFOPO->isConnected()) {
        QMessageBox::warning(this, "错误", "请先连接光谱仪");
        return;
    }

    // 获取积分时间并检查有效性
    int integrationTime = m_spectrometerFOPO->getIntegrationTime();
    if (integrationTime <= 0) {
        QMessageBox::warning(this, "错误", "无法获取积分时间，请检查光谱仪连接");
        qDebug() << "获取积分时间失败，返回值:" << integrationTime;
        return;
    }

    // 启动连续测量定时器
    // 默认间隔：积分时间 + 100ms 余量
    int intervalMs = integrationTime / 1000 + 100;

    // 确保间隔至少为100ms
    if (intervalMs < 100) {
        intervalMs = 100;
        qDebug() << "测量间隔过短，调整为最小值: 100ms";
    }

    m_measureTimerFOPO->start(intervalMs);
    m_isContinuousMeasuringFOPO = true;
    m_isMeasuringFOPO = true;

    // 立即执行一次测量
    startMeasurement();

    updateStatusBar(QString("开始持续测量（间隔: %1 ms）").arg(intervalMs));
    qDebug() << "开始持续测量，间隔:" << intervalMs << "ms";
}

void Integration::on_btnStopMeasureFOPO_clicked()
{
    // 停止连续测量定时器
    if (m_isContinuousMeasuringFOPO) {
        m_measureTimerFOPO->stop();
        m_isContinuousMeasuringFOPO = false;
        updateStatusBar("持续测量已停止");
        qDebug() << "持续测量已停止";
    }

    stopMeasurement();
}

void Integration::on_btnSavePlotFOPO_clicked()
{
    saveSpectrum();
}

void Integration::on_btnResetViewFOPO_clicked()
{
    if (!m_chartFOPO) {
        QMessageBox::warning(this, "错误", "图表未初始化");
        return;
    }

    // 重置坐标轴范围
    if (m_axisXFOPO) {
        m_axisXFOPO->setRange(200, 1100);  // 波长范围 200-1100 nm
    }
    if (m_axisYFOPO) {
        m_axisYFOPO->setRange(0, 65535);  // 强度范围
    }

    // 重置缩放
    m_chartFOPO->zoomReset();

    updateStatusBar("视图已重置");
    qDebug() << "视图已重置";
}

void Integration::on_btnClearPlotFOPO_clicked()
{
    if (!m_seriesFOPO) {
        QMessageBox::warning(this, "错误", "数据系列未初始化");
        return;
    }

    // 清除数据
    m_seriesFOPO->clear();
    m_lastSpectrumDataFOPO.clear();

    updateStatusBar("光谱数据已清除");
    qDebug() << "光谱数据已清除";
}

// ========== 峰值检测弹窗函数 ==========

void Integration::on_btnShowPeaksFOPO_clicked()
{
    // 创建峰值检测对话框
    QDialog *peakDialog = new QDialog(this);
    peakDialog->setWindowTitle("FOPO路峰值检测");
    peakDialog->resize(600, 400);

    // 创建布局
    QVBoxLayout *mainLayout = new QVBoxLayout(peakDialog);

    // 创建按钮行
    QHBoxLayout *buttonLayout = new QHBoxLayout();

    QPushButton *btnDetect = new QPushButton("检测峰值", peakDialog);
    QPushButton *btnExport = new QPushButton("导出峰值", peakDialog);
    QPushButton *btnClear = new QPushButton("清除峰值", peakDialog);

    buttonLayout->addWidget(btnDetect);
    buttonLayout->addWidget(btnExport);
    buttonLayout->addWidget(btnClear);
    buttonLayout->addStretch();

    mainLayout->addLayout(buttonLayout);

    // 创建峰值表格
    QTableWidget *peakTable = new QTableWidget(peakDialog);
    peakTable->setColumnCount(3);
    peakTable->setHorizontalHeaderLabels({"序号", "像素索引", "强度"});
    peakTable->horizontalHeader()->setStretchLastSection(true);

    mainLayout->addWidget(peakTable);

    // 连接按钮信号
    connect(btnDetect, &QPushButton::clicked, [this, peakTable]() {
        if (m_lastSpectrumDataFOPO.isEmpty()) {
            QMessageBox::warning(this, "错误", "没有可用的光谱数据，请先进行测量");
            return;
        }

        // 执行峰值检测（内联实现）
        m_peaksFOPO.clear();

        int intensityThreshold = 1000;  // 强度阈值
        int peakWidth = 5;  // 峰宽度
        int totalPixels = m_lastSpectrumDataFOPO.size();

        // 峰值检测算法：寻找局部最大值
        for (int i = peakWidth; i < totalPixels - peakWidth; ++i) {
            int currentIntensity = m_lastSpectrumDataFOPO[i];

            if (currentIntensity < intensityThreshold) {
                continue;
            }

            bool isPeak = true;
            for (int j = i - peakWidth; j <= i + peakWidth; ++j) {
                if (j != i && m_lastSpectrumDataFOPO[j] >= currentIntensity) {
                    isPeak = false;
                    break;
                }
            }

            if (isPeak) {
                PeakData peak;
                peak.pixelIndex = i;
                peak.wavelength = 200.0 + (900.0 * i / (totalPixels - 1));
                peak.intensity = currentIntensity;
                peak.fwhm = calculateFWHM(i, m_lastSpectrumDataFOPO);
                m_peaksFOPO.append(peak);
                i += peakWidth;
            }
        }

        // 更新表格
        peakTable->setRowCount(m_peaksFOPO.size());
        for (int i = 0; i < m_peaksFOPO.size(); i++) {
            peakTable->setItem(i, 0, new QTableWidgetItem(QString::number(i + 1)));
            peakTable->setItem(i, 1, new QTableWidgetItem(QString::number(m_peaksFOPO[i].pixelIndex)));
            peakTable->setItem(i, 2, new QTableWidgetItem(QString::number(m_peaksFOPO[i].intensity)));
        }

        updateStatusBar(QString("检测到 %1 个峰值").arg(m_peaksFOPO.size()));
        qDebug() << "FOPO路峰值检测完成，共检测到" << m_peaksFOPO.size() << "个峰值";
    });

    connect(btnExport, &QPushButton::clicked, [this]() {
        if (m_peaksFOPO.isEmpty()) {
            QMessageBox::warning(this, "错误", "没有峰值数据可导出，请先检测峰值");
            return;
        }

        // 导出峰值数据（内联实现）
        QString fileName = QFileDialog::getSaveFileName(
            this,
            "导出FOPO路峰值数据",
            QCoreApplication::applicationDirPath() + "/peaks_fopo.csv",
            "CSV文件 (*.csv);;所有文件 (*)"
        );

        if (fileName.isEmpty()) {
            return;
        }

        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::warning(this, "错误", "无法创建文件：" + fileName);
            return;
        }

        QTextStream out(&file);
        out.setEncoding(QStringConverter::Utf8);
        out << "\xEF\xBB\xBF";  // UTF-8 BOM
        out << "序号,像素索引,波长[nm],强度[counts],FWHM[nm]\n";

        for (int i = 0; i < m_peaksFOPO.size(); i++) {
            const PeakData &peak = m_peaksFOPO[i];
            out << (i + 1) << ","
                << peak.pixelIndex << ","
                << QString::number(peak.wavelength, 'f', 2) << ","
                << peak.intensity << ","
                << QString::number(peak.fwhm, 'f', 2) << "\n";
        }

        file.close();

        updateStatusBar("FOPO路峰值数据已导出: " + fileName);
        QMessageBox::information(this, "导出成功",
            QString("峰值数据已导出到:\n%1\n\n共导出 %2 个峰值")
            .arg(fileName).arg(m_peaksFOPO.size()));
    });

    connect(btnClear, &QPushButton::clicked, [this, peakTable]() {
        m_peaksFOPO.clear();
        peakTable->setRowCount(0);
        updateStatusBar("FOPO路峰值数据已清除");
        qDebug() << "FOPO路峰值数据已清除";
    });

    // 显示对话框
    peakDialog->exec();
    peakDialog->deleteLater();
}

void Integration::on_btnShowPeaksStokes_clicked()
{
    // 创建峰值检测对话框
    QDialog *peakDialog = new QDialog(this);
    peakDialog->setWindowTitle("Stokes路峰值检测");
    peakDialog->resize(600, 400);

    // 创建布局
    QVBoxLayout *mainLayout = new QVBoxLayout(peakDialog);

    // 创建按钮行
    QHBoxLayout *buttonLayout = new QHBoxLayout();

    QPushButton *btnDetect = new QPushButton("检测峰值", peakDialog);
    QPushButton *btnExport = new QPushButton("导出峰值", peakDialog);
    QPushButton *btnClear = new QPushButton("清除峰值", peakDialog);

    buttonLayout->addWidget(btnDetect);
    buttonLayout->addWidget(btnExport);
    buttonLayout->addWidget(btnClear);
    buttonLayout->addStretch();

    mainLayout->addLayout(buttonLayout);

    // 创建峰值表格
    QTableWidget *peakTable = new QTableWidget(peakDialog);
    peakTable->setColumnCount(3);
    peakTable->setHorizontalHeaderLabels({"序号", "像素索引", "强度"});
    peakTable->horizontalHeader()->setStretchLastSection(true);

    mainLayout->addWidget(peakTable);

    // 连接按钮信号
    connect(btnDetect, &QPushButton::clicked, [this, peakTable]() {
        if (m_lastSpectrumDataStokes.isEmpty()) {
            QMessageBox::warning(this, "错误", "没有可用的Stokes路光谱数据，请先进行测量");
            return;
        }

        // 执行峰值检测
        m_peaksStokes.clear();

        int windowSize = 5;
        for (int i = windowSize; i < m_lastSpectrumDataStokes.size() - windowSize; i++) {
            bool isPeak = true;
            int currentValue = m_lastSpectrumDataStokes[i];

            for (int j = -windowSize; j <= windowSize; j++) {
                if (j != 0 && m_lastSpectrumDataStokes[i + j] >= currentValue) {
                    isPeak = false;
                    break;
                }
            }

            if (isPeak && currentValue > 1000) {
                PeakData peak;
                peak.wavelength = i;
                peak.intensity = currentValue;
                peak.pixelIndex = i;
                peak.fwhm = 0.0;
                m_peaksStokes.append(peak);
            }
        }

        // 更新表格
        peakTable->setRowCount(m_peaksStokes.size());
        for (int i = 0; i < m_peaksStokes.size(); i++) {
            peakTable->setItem(i, 0, new QTableWidgetItem(QString::number(i + 1)));
            peakTable->setItem(i, 1, new QTableWidgetItem(QString::number(m_peaksStokes[i].pixelIndex)));
            peakTable->setItem(i, 2, new QTableWidgetItem(QString::number(m_peaksStokes[i].intensity)));
        }

        updateStatusBar(QString("Stokes路检测到 %1 个峰值").arg(m_peaksStokes.size()));
        qDebug() << "Stokes路峰值检测完成，共检测到" << m_peaksStokes.size() << "个峰值";
    });

    connect(btnExport, &QPushButton::clicked, [this]() {
        if (m_peaksStokes.isEmpty()) {
            QMessageBox::warning(this, "错误", "没有Stokes路峰值数据可导出，请先检测峰值");
            return;
        }

        QString fileName = QFileDialog::getSaveFileName(this, "导出Stokes路峰值数据",
                                                        "", "CSV文件 (*.csv)");
        if (fileName.isEmpty()) {
            return;
        }

        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::warning(this, "错误", "无法创建文件");
            return;
        }

        QTextStream out(&file);
        out << "Peak Number,Pixel Index,Intensity (counts)\n";

        for (int i = 0; i < m_peaksStokes.size(); i++) {
            out << (i + 1) << ","
                << m_peaksStokes[i].pixelIndex << ","
                << m_peaksStokes[i].intensity << "\n";
        }

        file.close();

        updateStatusBar("Stokes路峰值数据已导出");
        qDebug() << "Stokes路峰值数据已导出到:" << fileName;
    });

    connect(btnClear, &QPushButton::clicked, [this, peakTable]() {
        m_peaksStokes.clear();
        peakTable->setRowCount(0);
        updateStatusBar("Stokes路峰值数据已清除");
        qDebug() << "Stokes路峰值数据已清除";
    });

    // 显示对话框
    peakDialog->exec();
    peakDialog->deleteLater();
}

// ========== 峰值检测辅助函数 ==========

double Integration::calculateFWHM(int peakIndex, const QVector<int> &data)
{
    if (peakIndex < 0 || peakIndex >= data.size()) {
        return 0.0;
    }

    int peakIntensity = data[peakIndex];
    int halfMaxIntensity = peakIntensity / 2;

    // 向左查找半高点
    int leftIndex = peakIndex;
    while (leftIndex > 0 && data[leftIndex] > halfMaxIntensity) {
        leftIndex--;
    }

    // 向右查找半高点
    int rightIndex = peakIndex;
    while (rightIndex < data.size() - 1 && data[rightIndex] > halfMaxIntensity) {
        rightIndex++;
    }

    // 计算 FWHM（像素宽度转换为波长宽度）
    int pixelWidth = rightIndex - leftIndex;
    int totalPixels = data.size();
    double wavelengthWidth = 900.0 * pixelWidth / (totalPixels - 1);

    return wavelengthWidth;
}

void Integration::startMeasurement()
{
    if (m_spectrometerFOPO->startScan()) {
        m_isMeasuringFOPO = true;
        updateStatusBar("开始光谱测量");
        qDebug() << "开始光谱测量";
    } else {
        QMessageBox::warning(this, "错误", "启动测量失败：" + m_spectrometerFOPO->getLastError());
    }
}

void Integration::stopMeasurement()
{
    m_isMeasuringFOPO = false;
    updateStatusBar("停止光谱测量");
    qDebug() << "停止光谱测量";
}

void Integration::onMeasureTimeoutFOPO()
{
    // 定时器触发，执行一次测量
    if (m_isContinuousMeasuringFOPO && m_spectrometerFOPO->isConnected()) {
        startMeasurement();
    }
}

void Integration::saveSpectrum()
{
    QString fileName = QFileDialog::getSaveFileName(
        this,
        "保存光谱图片",
        QCoreApplication::applicationDirPath() + "/spectrum.png",
        "PNG图片 (*.png);;JPEG图片 (*.jpg *.jpeg);;所有文件 (*)"
    );

    if (fileName.isEmpty()) {
        return;
    }

    // 保存图表为图片
    if (m_chartViewFOPO) {
        QPixmap pixmap = m_chartViewFOPO->grab();
        if (pixmap.save(fileName)) {
            updateStatusBar("光谱图片已保存: " + fileName);
            qDebug() << "光谱图片已保存:" << fileName;
            QMessageBox::information(this, "保存成功", "光谱图片已保存到:\n" + fileName);
        } else {
            QMessageBox::warning(this, "保存失败", "无法保存光谱图片");
        }
    } else {
        QMessageBox::warning(this, "错误", "图表视图未初始化");
    }
}

// ========== 位移台控制槽函数 ==========

void Integration::on_btnStageMoveAbsolute_clicked()
{
    // 读取速度
    bool ok;
    qint32 speed = ui->lineEditStageSpeed->text().toInt(&ok);
    if (!ok || speed <= 0) {
        QMessageBox::warning(this, "错误", "请输入有效的运动速度（正整数，pulse/s）");
        return;
    }

    // 读取位移量
    double displace = ui->lineEditStageDisplace->text().toDouble(&ok);
    if (!ok) {
        QMessageBox::warning(this, "错误", "请输入有效的位移量（mm）");
        return;
    }

    if (!m_stageController->isConnected()) {
        QMessageBox::warning(this, "错误", "位移台未连接");
        return;
    }

    // 标准流程：读取双台参数 → 设置速度 → 双台同步绝对位移
    m_stageController->readDeviceInfoDual();
    QThread::msleep(200);  // 等待 IN 响应

    if (!m_stageController->setMaxSpeedDual(speed)) {
        QMessageBox::warning(this, "错误", "设置速度失败：" + m_stageController->getLastError());
        return;
    }
    QThread::msleep(100);

    if (m_stageController->moveAbsoluteDual(displace)) {
        updateStatusBar(QString("双台绝对位移: %1 mm，速度: %2 pulse/s").arg(displace).arg(speed));
        qDebug() << "双台绝对位移成功:" << displace << "mm";
    } else {
        QMessageBox::warning(this, "错误", "双台绝对位移失败：" + m_stageController->getLastError());
    }
}

void Integration::on_btnConfirmStageAngle_clicked()
{
    // 旧槽保留兼容，转发到新统一绝对位移槽
    on_btnStageMoveAbsolute_clicked();
}

void Integration::on_btnConfirmStagePosition_clicked()
{
    // 旧槽保留兼容，转发到新统一绝对位移槽
    on_btnStageMoveAbsolute_clicked();
}

void Integration::on_btnConfirmStageTimeDelay_clicked()
{
    // 旧单延迟线槽，保留兼容，转发到延迟线1
    on_btnConfirmStageDelayLine1_clicked();
}

void Integration::on_btnConfirmStageDelayLine1_clicked()
{
    bool ok;
    float delayPS = ui->lineEditStageDelayLine1->text().toFloat(&ok);
    if (!ok || delayPS < 0) {
        QMessageBox::warning(this, "错误", "请输入有效的延迟值（PS）");
        return;
    }
    setDelayLineValue(m_delayLine, delayPS, "延迟线1");
}

void Integration::on_btnConfirmStageDelayLine2_clicked()
{
    bool ok;
    float delayPS = ui->lineEditStageDelayLine2->text().toFloat(&ok);
    if (!ok || delayPS < 0) {
        QMessageBox::warning(this, "错误", "请输入有效的延迟值（PS）");
        return;
    }
    setDelayLineValue(m_delayLine2, delayPS, "延迟线2");
}

void Integration::onStage1PositionChanged(qint32 positionPulses)
{
    qDebug() << "位移台1 位置改变:" << positionPulses;
}

void Integration::onStage2PositionChanged(qint32 positionPulses)
{
    qDebug() << "位移台2 位置改变:" << positionPulses;
}

void Integration::onStageMoveCompletedDual()
{
    updateStatusBar("双位移台移动完成");
    qDebug() << "双位移台移动完成";
}

// ========== 延时线控制槽函数 ==========

void Integration::on_btnConfirmTimeDelay_clicked()
{
    setDelayTime(ui->lineEditTimeDelay, nullptr);
}

void Integration::onDelayChanged(float delayPS)
{
    qDebug() << "延时线1延迟改变:" << delayPS << "PS";
}

void Integration::onDelay2Changed(float delayPS)
{
    qDebug() << "延时线2延迟改变:" << delayPS << "PS";
}

// ========== 振镜打标控制槽函数（按 TCP/UDP 协议直连） ==========

// 帮助函数：从 UI 收集激光/振镜参数并下发到 GalvoMirror
static void collectGalvoParam(GalvoMirror *gm, Ui::Integration *ui)
{
    GalvoLaserPara laser;
    std::memset(&laser, 0, sizeof(laser));
    laser.nLaserOnDelay   = 110;
    laser.nLaserOffDelay  = 120;
    laser.nFPSDelay       = 10;
    laser.nFPSLength      = 20;
    laser.nQDelay         = 5;
    laser.DutyCycle        = ui->lineEditGalvoDutyCycle->text().toFloat();
    laser.Frequency        = ui->lineEditGalvoFrequency->text().toFloat();
    laser.StandbyDutyCycle = 0.2f;
    laser.StandbyFrequency = 10.0f;
    laser.nLaserPower      = ui->lineEditGalvoLaserPower->text().toFloat();
    gm->setLaserPara(laser);

    GalvoMarkSetting mark;
    std::memset(&mark, 0, sizeof(mark));
    mark.nMarkV          = ui->lineEditGalvoMarkV->text().toInt();
    mark.nMark2MarkDelay = 0;
    mark.nJumpDelay      = 0;
    mark.nMarkDelay      = 0;
    mark.nJumpV          = ui->lineEditGalvoJumpV->text().toInt();
    mark.ScanTimes       = ui->lineEditGalvoScanTimes->text().toInt();
    if (mark.ScanTimes < 1) mark.ScanTimes = 1;
    gm->setMarkSetting(mark);

    GalvoShapeInfo shape;
    std::memset(&shape, 0, sizeof(shape));
    if (ui->rbGalvoShapeLine->isChecked())
        shape.Shape = static_cast<float>(GALVO_SHAPE_LINE);
    else if (ui->rbGalvoShapeCircle->isChecked())
        shape.Shape = static_cast<float>(GALVO_SHAPE_CIRCLE);
    else if (ui->rbGalvoShapeArray->isChecked())
        shape.Shape = static_cast<float>(GALVO_SHAPE_ARRAY);
    else
        shape.Shape = static_cast<float>(GALVO_SHAPE_POINT);

    shape.PointX            = ui->lineEditGalvoPointX->text().toFloat();
    shape.PointY            = ui->lineEditGalvoPointY->text().toFloat();
    shape.Point_LaseronTime = ui->lineEditGalvoPointTime->text().toFloat();
    shape.Line_StartX       = ui->lineEditGalvoLineStartX->text().toFloat();
    shape.Line_StartY       = ui->lineEditGalvoLineStartY->text().toFloat();
    shape.Line_EndX         = ui->lineEditGalvoLineEndX->text().toFloat();
    shape.Line_EndY         = ui->lineEditGalvoLineEndY->text().toFloat();
    shape.CircleX           = ui->lineEditGalvoCircleX->text().toFloat();
    shape.CircleY           = ui->lineEditGalvoCircleY->text().toFloat();
    shape.Circle_Radius     = ui->lineEditGalvoCircleR->text().toFloat();
    gm->setShape(shape);

    GalvoIOControl io;
    std::memset(&io, 0, sizeof(io));
    io.nRedLightEnable  = 0;
    io.nReadyDownEanble = 1;
    io.nLightLevel      = 0;
    gm->setIOControl(io);
}

void Integration::on_rbGalvoShapePoint_toggled(bool checked)
{
    if (checked) qDebug() << "振镜图形切换：点";
}
void Integration::on_rbGalvoShapeLine_toggled(bool checked)
{
    if (checked) qDebug() << "振镜图形切换：线";
}
void Integration::on_rbGalvoShapeCircle_toggled(bool checked)
{
    if (checked) qDebug() << "振镜图形切换：圆";
}
void Integration::on_rbGalvoShapeArray_toggled(bool checked)
{
    if (checked) qDebug() << "振镜图形切换：多点阵列";
}

void Integration::on_btnGalvoSetParam_clicked()
{
    if (!m_galvoMirror->isConnected()) {
        QMessageBox::warning(this, "错误", "振镜控制卡未连接！\n请先连接振镜控制卡。");
        return;
    }
    collectGalvoParam(m_galvoMirror, ui);
    if (m_galvoMirror->sendShapeFrame())
        updateStatusBar("已下发振镜图形参数");
    else
        updateStatusBar("振镜图形参数下发失败");
}

void Integration::on_btnGalvoStart_clicked()
{
    if (!m_galvoMirror->isConnected()) {
        QMessageBox::warning(this, "错误", "振镜控制卡未连接！\n请先连接振镜控制卡。");
        return;
    }
    collectGalvoParam(m_galvoMirror, ui);
    if (m_galvoMirror->startMark())
        updateStatusBar("振镜：开始打标");
    else
        updateStatusBar("振镜：开始打标失败");
}

void Integration::on_btnGalvoStop_clicked()
{
    if (m_galvoMirror->stopMark())
        updateStatusBar("振镜：已停止");
}

void Integration::on_btnGalvoPause_clicked()
{
    if (m_galvoMirror->pauseMark())
        updateStatusBar("振镜：已暂停");
}

void Integration::on_btnGalvoContinue_clicked()
{
    if (m_galvoMirror->continueMark())
        updateStatusBar("振镜：已继续");
}

void Integration::on_btnGalvoClearLog_clicked()
{
    ui->textEditGalvoLog->clear();
}

void Integration::onGalvoMessageLog(const QString &msg)
{
    ui->textEditGalvoLog->append(msg);
}

void Integration::onGalvoHeartbeatChanged(bool online)
{
    qDebug() << "振镜心跳：" << (online ? "在线" : "离线");
}

// ========== 泵功率设置槽函数 - 振镜页 ==========

// ========== 泵功率设置槽函数 - 位移台页（OHLD 四泵）==========

void Integration::on_btnConfirmStagePump1_clicked()
{
    bool ok;
    float cur = ui->lineEditStagePump1->text().toFloat(&ok);
    if (!ok) { QMessageBox::warning(this, "错误", "请输入有效的电流值"); return; }
    setOhldPumpCurrent(0, cur);
}

void Integration::on_btnConfirmStagePump2_clicked()
{
    bool ok;
    float cur = ui->lineEditStagePump2->text().toFloat(&ok);
    if (!ok) { QMessageBox::warning(this, "错误", "请输入有效的电流值"); return; }
    setOhldPumpCurrent(1, cur);
}

void Integration::on_btnConfirmStagePump3_clicked()
{
    bool ok;
    float cur = ui->lineEditStagePump3->text().toFloat(&ok);
    if (!ok) { QMessageBox::warning(this, "错误", "请输入有效的电流值"); return; }
    setOhldPumpCurrent(2, cur);
}

void Integration::on_btnConfirmStagePump4_clicked()
{
    bool ok;
    float cur = ui->lineEditStagePump4->text().toFloat(&ok);
    if (!ok) { QMessageBox::warning(this, "错误", "请输入有效的电流值"); return; }
    setOhldPumpCurrent(3, cur);
}

// ========== 泵功率设置槽函数 - 振镜页（OHLD 四泵，与位移台页统一） ==========

void Integration::on_btnConfirmGalvoPump1_clicked()
{
    bool ok;
    float cur = ui->lineEditGalvoPump1->text().toFloat(&ok);
    if (!ok) { QMessageBox::warning(this, "错误", "请输入有效的电流值"); return; }
    setOhldPumpCurrent(0, cur);
}

void Integration::on_btnConfirmGalvoPump2_clicked()
{
    bool ok;
    float cur = ui->lineEditGalvoPump2->text().toFloat(&ok);
    if (!ok) { QMessageBox::warning(this, "错误", "请输入有效的电流值"); return; }
    setOhldPumpCurrent(1, cur);
}

void Integration::on_btnConfirmGalvoPump3_clicked()
{
    bool ok;
    float cur = ui->lineEditGalvoPump3->text().toFloat(&ok);
    if (!ok) { QMessageBox::warning(this, "错误", "请输入有效的电流值"); return; }
    setOhldPumpCurrent(2, cur);
}

void Integration::on_btnConfirmGalvoPump4_clicked()
{
    bool ok;
    float cur = ui->lineEditGalvoPump4->text().toFloat(&ok);
    if (!ok) { QMessageBox::warning(this, "错误", "请输入有效的电流值"); return; }
    setOhldPumpCurrent(3, cur);
}

void Integration::setOhldPumpCurrent(int pumpIndex, float currentMA)
{
    // pumpIndex: 0=种子源泵, 1=pump路预放泵, 2=pump路主级泵, 3=Stokes路泵
    if (pumpIndex < 0 || pumpIndex >= 4) return;

    SerialPortBase *port = m_ohldPumps[pumpIndex];
    if (!port || !port->isOpen()) {
        QMessageBox::warning(this, "错误",
            QString("泵%1 串口未连接，请先在连接页配置并连接").arg(pumpIndex + 1));
        return;
    }

    static const QString pumpNames[4] = {
        "种子源泵", "pump路预放泵（小泵）", "pump路主级泵（大泵）", "Stokes路泵（小泵）"
    };
    const QString &name = pumpNames[pumpIndex];

    // 1. 开启激光器（0x05）
    QByteArray openFrame = ohldBuildFrame(OHLD_CMD_OPEN);
    if (port->writeData(openFrame) != openFrame.size()) {
        QMessageBox::warning(this, "错误", name + " 开启指令发送失败");
        return;
    }
    QThread::msleep(100);

    // 2. 设置驱动电流（0x02，电流×10，4字节大端）
    QByteArray setFrame = ohldBuildSetCurrentFrame(currentMA);
    if (port->writeData(setFrame) != setFrame.size()) {
        QMessageBox::warning(this, "错误", name + " 设置电流指令发送失败");
        return;
    }
    QThread::msleep(100);

    // 3. 查询状态（0x01），等待响应后解析并刷新显示框
    QByteArray queryFrame = ohldBuildFrame(OHLD_CMD_QUERY);
    if (port->writeData(queryFrame) != queryFrame.size()) {
        QMessageBox::warning(this, "错误", name + " 查询指令发送失败");
        return;
    }
    QThread::msleep(200);

    // 读取响应（已等待 200ms，直接读取所有可用数据）
    QByteArray resp = port->readAll();
    OhldPumpStatus status;
    if (ohldParseQueryResponse(resp, status)) {
        m_ohldStatus[pumpIndex] = status;

        // 刷新对应泵的显示框
        QLineEdit *tempEdit   = nullptr;
        QLineEdit *driveEdit  = nullptr;
        QLineEdit *maxEdit    = nullptr;
        QLineEdit *backEdit   = nullptr;

        switch (pumpIndex) {
            case 0:
                tempEdit  = ui->lineEditPump1Temp;
                driveEdit = ui->lineEditPump1Drive;
                maxEdit   = ui->lineEditPump1Max;
                backEdit  = ui->lineEditPump1Back;
                break;
            case 1:
                tempEdit  = ui->lineEditPump2Temp;
                driveEdit = ui->lineEditPump2Drive;
                maxEdit   = ui->lineEditPump2Max;
                backEdit  = ui->lineEditPump2Back;
                break;
            case 2:
                tempEdit  = ui->lineEditPump3Temp;
                driveEdit = ui->lineEditPump3Drive;
                maxEdit   = ui->lineEditPump3Max;
                backEdit  = ui->lineEditPump3Back;
                break;
            case 3:
                tempEdit  = ui->lineEditPump4Temp;
                driveEdit = ui->lineEditPump4Drive;
                maxEdit   = ui->lineEditPump4Max;
                backEdit  = ui->lineEditPump4Back;
                break;
        }

        if (tempEdit)  tempEdit->setText(QString::number(status.temperature,  'f', 1) + " ℃");
        if (driveEdit) driveEdit->setText(QString::number(status.driveCurrent, 'f', 1) + " mA");
        if (maxEdit)   maxEdit->setText(QString::number(status.maxCurrent,    'f', 1) + " mA");
        if (backEdit)  backEdit->setText(QString::number(status.backCurrent,  'f', 1) + " mA");

        updateStatusBar(QString("%1 电流设置成功: %2 mA，温度: %3 ℃")
                        .arg(name).arg(currentMA, 0, 'f', 1).arg(status.temperature, 0, 'f', 1));

        // 温度异常保护（>60℃ 自动关闭）
        if (status.temperature > 60.0f) {
            QByteArray closeFrame = ohldBuildFrame(OHLD_CMD_CLOSE);
            port->writeData(closeFrame);
            QMessageBox::warning(this, "温度异常",
                name + QString(" 温度过高（%1℃），已自动关闭！").arg(status.temperature, 0, 'f', 1));
        }
    } else {
        updateStatusBar(name + " 查询响应解析失败，请检查连接");
        qDebug() << name << "OHLD 查询响应解析失败，原始数据:" << resp.toHex(' ');
    }
}

bool Integration::setOhldPumpCurrentSilent(int pumpIndex, float currentMA)
{
    // 静默版：仅顺序下发 开启(0x05) → 设置电流(0x02)，不弹窗、不查询、不刷新显示。
    // 用于功率/位移台/延迟预设执行链路。返回 true 表示两条命令均完整写入串口。
    if (pumpIndex < 0 || pumpIndex >= 4) return false;
    SerialPortBase *port = m_ohldPumps[pumpIndex];
    if (!port || !port->isOpen()) {
        return false;
    }

    QByteArray openFrame = ohldBuildFrame(OHLD_CMD_OPEN);
    if (port->writeData(openFrame) != openFrame.size()) {
        return false;
    }
    QThread::msleep(50);

    QByteArray setFrame = ohldBuildSetCurrentFrame(currentMA);
    if (port->writeData(setFrame) != setFrame.size()) {
        return false;
    }
    QThread::msleep(50);

    return true;
}


void Integration::setDelayTime(QLineEdit *inputField, QLineEdit *syncField)
{
    // 振镜页延迟线1设置入口，复用 setDelayLineValue
    if (!inputField) return;

    bool ok;
    float delayPS = inputField->text().toFloat(&ok);
    if (!ok) {
        QMessageBox::warning(this, "错误", "请输入有效的延迟值");
        return;
    }

    setDelayLineValue(m_delayLine, delayPS, "延迟线1");

    if (syncField) {
        syncField->setText(inputField->text());
    }
}

void Integration::setDelayLineValue(DelayLine *device, float delayPS, const QString &label)
{
    // 统一双延迟线控制方法：归零 → 设置延迟 → 查询位置
    if (!device) return;

    if (!device->isConnected()) {
        QMessageBox::warning(this, "错误", label + " 未连接");
        return;
    }

    // 1. 归零（功能码 0x07）
    if (!device->home()) {
        QMessageBox::warning(this, "错误", label + " 归零失败：" + device->getLastError());
        return;
    }
    qDebug() << label << "归零成功";

    // 2. 设置延迟（功能码 0x04，PS×1000→3字节大端）
    if (device->setDelay(delayPS)) {
        updateStatusBar(label + " 延迟设置成功: " + QString::number(delayPS) + " PS");
        qDebug() << label << "延迟设置成功:" << delayPS << "PS";
    } else {
        QMessageBox::warning(this, "错误", label + " 延迟设置失败：" + device->getLastError());
        return;
    }

    // 3. 查询当前位置（功能码 0x0E）
    device->queryPosition();
}

// ========== 设备状态更新槽函数 ==========

void Integration::onSpectrometerFOPOStatusChanged(DeviceStatus status)
{
    updateStatusIndicator(ui->labelSpectrometerFOPOStatus, status);
    qDebug() << "光谱仪状态改变:" << static_cast<int>(status);
}

void Integration::onStageStatusChanged(DeviceStatus status)
{
    // 双串口聚合：把整体状态同步到两个指示灯
    updateStatusIndicator(ui->labelStage1Status, status);
    updateStatusIndicator(ui->labelStage2Status, status);
    qDebug() << "双位移台整体状态改变:" << static_cast<int>(status);
}

void Integration::onGalvoStatusChanged(DeviceStatus status)
{
    updateStatusIndicator(ui->labelGalvoStatus, status);
    qDebug() << "振镜控制卡状态变化:" << static_cast<int>(status);
}

void Integration::onDelayStatusChanged(DeviceStatus status)
{
    updateStatusIndicator(ui->labelDelayStatus, status);
    qDebug() << "延时线状态改变:" << static_cast<int>(status);
}

// ========== 光谱仪数据接收槽函数 ==========

void Integration::onSpectrumDataReady(const QVector<int> &intensity)
{
    qDebug() << "收到FOPO路光谱数据，像素数:" << intensity.size();
    updateSpectrum(intensity);
}


// ========== Stokes路光谱仪连接与测量函数 ==========

void Integration::on_btnConnectSpectrometerStokes_clicked()
{
    SerialConfig config = getSpectrometerStokesSerialConfig();

    // 检查是否选择了串口
    if (config.portName.isEmpty()) {
        QMessageBox::warning(this, "连接失败",
            "Stokes路光谱仪：请先选择串口设备！\n\n请在串口下拉框中选择一个可用的串口。");
        return;
    }

    // 显示连接中状态
    updateStatusIndicator(ui->labelSpectrometerStokesStatus, DeviceStatus::Connecting);
    updateStatusBar("Stokes路光谱仪正在连接...");

    // 处理 UI 事件
    QCoreApplication::processEvents();

    // 使用 QTimer 异步执行连接操作
    QTimer::singleShot(50, this, [this, config]() {
        // 设置串口参数
        m_spectrometerStokes->setPortName(config.portName);
        m_spectrometerStokes->setBaudRate(config.baudRate);
        m_spectrometerStokes->setDataBits(static_cast<int>(config.dataBits));
        m_spectrometerStokes->setStopBits(static_cast<int>(config.stopBits));
        m_spectrometerStokes->setParity(static_cast<int>(config.parity));

        // 处理 UI 事件
        QCoreApplication::processEvents();

        if (m_spectrometerStokes->connect()) {
            updateStatusBar("Stokes路光谱仪连接成功");
            updateStatusIndicator(ui->labelSpectrometerStokesStatus, DeviceStatus::Connected);

            QString info = QString("像素数: %1, 序列号: %2")
                           .arg(m_spectrometerStokes->getPixelLength())
                           .arg(m_spectrometerStokes->getSerialNumber());
            qDebug() << "Stokes路光谱仪信息:" << info;
        } else {
            updateStatusBar("Stokes路光谱仪连接失败");
            updateStatusIndicator(ui->labelSpectrometerStokesStatus, DeviceStatus::Error);
            QMessageBox::warning(this, "连接失败", m_spectrometerStokes->getLastError());
        }
    });
}

void Integration::on_btnDisconnectSpectrometerStokes_clicked()
{
    m_spectrometerStokes->disconnect();
    updateStatusIndicator(ui->labelSpectrometerStokesStatus, DeviceStatus::Disconnected);
    updateStatusBar("Stokes路光谱仪已断开");
}

void Integration::on_btnSingleMeasureStokes_clicked()
{
    if (!m_spectrometerStokes->isConnected()) {
        QMessageBox::warning(this, "错误", "请先连接Stokes路光谱仪");
        return;
    }

    // 单次测量
    if (m_spectrometerStokes->startScan()) {
        m_isMeasuringStokes = true;
        updateStatusBar("Stokes路开始单次测量");
        qDebug() << "Stokes路开始单次测量";
    } else {
        QMessageBox::warning(this, "错误", "Stokes路启动测量失败：" + m_spectrometerStokes->getLastError());
        qDebug() << "Stokes路启动测量失败：" << m_spectrometerStokes->getLastError();
    }
}

void Integration::on_btnContinuousMeasureStokes_clicked()
{
    if (!m_spectrometerStokes->isConnected()) {
        QMessageBox::warning(this, "错误", "请先连接Stokes路光谱仪");
        return;
    }

    // 获取积分时间并检查有效性
    int integrationTime = m_spectrometerStokes->getIntegrationTime();
    if (integrationTime <= 0) {
        QMessageBox::warning(this, "错误", "无法获取积分时间，请检查光谱仪连接");
        qDebug() << "Stokes路获取积分时间失败，返回值:" << integrationTime;
        return;
    }

    // 启动连续测量定时器
    // 默认间隔：积分时间 + 100ms 余量
    int intervalMs = integrationTime / 1000 + 100;

    // 确保间隔至少为100ms
    if (intervalMs < 100) {
        intervalMs = 100;
        qDebug() << "Stokes路测量间隔过短，调整为最小值: 100ms";
    }

    m_measureTimerStokes->start(intervalMs);
    m_isContinuousMeasuringStokes = true;
    m_isMeasuringStokes = true;

    // 立即执行一次测量
    if (m_spectrometerStokes->startScan()) {
        updateStatusBar(QString("Stokes路开始持续测量（间隔: %1 ms）").arg(intervalMs));
        qDebug() << "Stokes路开始持续测量，间隔:" << intervalMs << "ms";
    } else {
        m_measureTimerStokes->stop();
        m_isContinuousMeasuringStokes = false;
        m_isMeasuringStokes = false;
        QMessageBox::warning(this, "错误", "Stokes路启动测量失败");
    }
}

void Integration::on_btnStopMeasureStokes_clicked()
{
    // 停止连续测量定时器
    if (m_isContinuousMeasuringStokes) {
        m_measureTimerStokes->stop();
        m_isContinuousMeasuringStokes = false;
        updateStatusBar("Stokes路持续测量已停止");
        qDebug() << "Stokes路持续测量已停止";
    }

    m_isMeasuringStokes = false;
}

void Integration::on_btnSavePlotStokes_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(
        this,
        "保存Stokes路光谱图片",
        QCoreApplication::applicationDirPath() + "/spectrum_stokes.png",
        "PNG图片 (*.png);;JPEG图片 (*.jpg *.jpeg);;所有文件 (*)"
    );

    if (fileName.isEmpty()) {
        return;
    }

    // 保存图表为图片
    if (m_chartViewStokes) {
        QPixmap pixmap = m_chartViewStokes->grab();
        if (pixmap.save(fileName)) {
            updateStatusBar("Stokes路光谱图片已保存: " + fileName);
            qDebug() << "Stokes路光谱图片已保存:" << fileName;
            QMessageBox::information(this, "保存成功", "Stokes路光谱图片已保存到:\n" + fileName);
        } else {
            QMessageBox::warning(this, "保存失败", "无法保存Stokes路光谱图片");
        }
    } else {
        QMessageBox::warning(this, "错误", "Stokes路图表视图未初始化");
    }
}

void Integration::on_btnResetViewStokes_clicked()
{
    if (!m_chartStokes) {
        QMessageBox::warning(this, "错误", "图表未初始化");
        return;
    }

    // 重置坐标轴范围
    if (m_axisXStokes) {
        m_axisXStokes->setRange(200, 1100);  // 波长范围 200-1100 nm
    }
    if (m_axisYStokes) {
        m_axisYStokes->setRange(0, 65535);  // 强度范围
    }

    // 重置缩放
    m_chartStokes->zoomReset();

    updateStatusBar("Stokes路视图已重置");
    qDebug() << "Stokes路视图已重置";
}

void Integration::on_btnClearPlotStokes_clicked()
{
    if (!m_seriesStokes) {
        QMessageBox::warning(this, "错误", "数据系列未初始化");
        return;
    }

    // 清除数据
    m_seriesStokes->clear();
    m_lastSpectrumDataStokes.clear();

    updateStatusBar("Stokes路光谱数据已清除");
    qDebug() << "Stokes路光谱数据已清除";
}

void Integration::onMeasureTimeoutStokes()
{
    // 定时器触发，执行一次测量
    if (m_isContinuousMeasuringStokes && m_spectrometerStokes->isConnected()) {
        m_spectrometerStokes->startScan();
    }
}

void Integration::onSpectrometerStokesStatusChanged(DeviceStatus status)
{
    updateStatusIndicator(ui->labelSpectrometerStokesStatus, status);
    qDebug() << "Stokes路光谱仪状态改变:" << static_cast<int>(status);
}

void Integration::onSpectrumDataReadyStokes(const QVector<int> &intensity)
{
    // 保存最新的光谱数据
    m_lastSpectrumDataStokes = intensity;

    // 更新图表显示
    updateSpectrumStokes(intensity);

    qDebug() << "Stokes路光谱数据已接收，数据点数量:" << intensity.size();
}

// ========== 错误处理槽函数 ==========

void Integration::onDeviceError(const QString &error)
{
    qDebug() << "设备错误:" << error;
    updateStatusBar("设备错误: " + error);
}

// ========== 光谱图表相关函数 ==========

void Integration::initSpectrumChart()
{
    // ========== 初始化FOPO路光谱仪图表 ==========
    // 创建图表
    m_chartFOPO = new QChart();
    m_chartFOPO->setTitle("FOPO路光谱数据");
    m_chartFOPO->setAnimationOptions(QChart::NoAnimation);  // 禁用动画，提高性能

    // 创建数据系列
    m_seriesFOPO = new QLineSeries();
    m_seriesFOPO->setName("光谱强度");
    m_seriesFOPO->setUseOpenGL(true);  // 启用OpenGL加速，大幅提升性能
    m_chartFOPO->addSeries(m_seriesFOPO);

    // 创建坐标轴
    m_axisXFOPO = new QValueAxis();
    m_axisXFOPO->setTitleText("波长 [nm]");
    m_axisXFOPO->setRange(200, 1100);
    m_axisXFOPO->setGridLineVisible(true);
    m_axisXFOPO->setMinorGridLineVisible(true);
    m_axisXFOPO->setTickCount(10);
    m_axisXFOPO->setMinorTickCount(4);
    m_chartFOPO->addAxis(m_axisXFOPO, Qt::AlignBottom);
    m_seriesFOPO->attachAxis(m_axisXFOPO);

    m_axisYFOPO = new QValueAxis();
    m_axisYFOPO->setTitleText("强度 [counts]");
    m_axisYFOPO->setRange(0, 65535);
    m_axisYFOPO->setGridLineVisible(true);
    m_axisYFOPO->setMinorGridLineVisible(true);
    m_axisYFOPO->setTickCount(10);
    m_axisYFOPO->setMinorTickCount(4);
    m_chartFOPO->addAxis(m_axisYFOPO, Qt::AlignLeft);
    m_seriesFOPO->attachAxis(m_axisYFOPO);

    // 隐藏图例（节省空间）
    m_chartFOPO->legend()->setVisible(false);

    // 创建标准图表视图
    m_chartViewFOPO = new QChartView(m_chartFOPO);
    m_chartViewFOPO->setRenderHint(QPainter::Antialiasing);
    m_chartViewFOPO->setOptimizationFlag(QGraphicsView::DontAdjustForAntialiasing, true);
    m_chartViewFOPO->setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);

    // 安装事件过滤器以处理双击事件
    m_chartViewFOPO->installEventFilter(this);

    // 将图表视图添加到UI布局中
    QWidget *plotWidgetFOPO = ui->chartWidgetFOPO;
    if (plotWidgetFOPO) {
        if (plotWidgetFOPO->layout()) {
            QLayoutItem *item;
            while ((item = plotWidgetFOPO->layout()->takeAt(0)) != nullptr) {
                delete item->widget();
                delete item;
            }
            delete plotWidgetFOPO->layout();
        }

        QVBoxLayout *layout = new QVBoxLayout(plotWidgetFOPO);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(m_chartViewFOPO);
        plotWidgetFOPO->setLayout(layout);
    }

    // ========== 初始化Stokes路光谱仪图表 ==========
    // 创建图表
    m_chartStokes = new QChart();
    m_chartStokes->setTitle("Stokes路光谱数据");
    m_chartStokes->setAnimationOptions(QChart::NoAnimation);

    // 创建数据系列
    m_seriesStokes = new QLineSeries();
    m_seriesStokes->setName("光谱强度");
    m_seriesStokes->setUseOpenGL(true);
    m_chartStokes->addSeries(m_seriesStokes);

    // 创建坐标轴
    m_axisXStokes = new QValueAxis();
    m_axisXStokes->setTitleText("波长 [nm]");
    m_axisXStokes->setRange(200, 1100);
    m_axisXStokes->setGridLineVisible(true);
    m_axisXStokes->setMinorGridLineVisible(true);
    m_axisXStokes->setTickCount(10);
    m_axisXStokes->setMinorTickCount(4);
    m_chartStokes->addAxis(m_axisXStokes, Qt::AlignBottom);
    m_seriesStokes->attachAxis(m_axisXStokes);

    m_axisYStokes = new QValueAxis();
    m_axisYStokes->setTitleText("强度 [counts]");
    m_axisYStokes->setRange(0, 65535);
    m_axisYStokes->setGridLineVisible(true);
    m_axisYStokes->setMinorGridLineVisible(true);
    m_axisYStokes->setTickCount(10);
    m_axisYStokes->setMinorTickCount(4);
    m_chartStokes->addAxis(m_axisYStokes, Qt::AlignLeft);
    m_seriesStokes->attachAxis(m_axisYStokes);

    // 隐藏图例（节省空间）
    m_chartStokes->legend()->setVisible(false);

    // 创建标准图表视图
    m_chartViewStokes = new QChartView(m_chartStokes);
    m_chartViewStokes->setRenderHint(QPainter::Antialiasing);
    m_chartViewStokes->setOptimizationFlag(QGraphicsView::DontAdjustForAntialiasing, true);
    m_chartViewStokes->setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);

    // 安装事件过滤器以处理双击事件
    m_chartViewStokes->installEventFilter(this);

    // 将图表视图添加到UI布局中
    QWidget *plotWidgetStokes = ui->chartWidgetStokes;
    if (plotWidgetStokes) {
        if (plotWidgetStokes->layout()) {
            QLayoutItem *item;
            while ((item = plotWidgetStokes->layout()->takeAt(0)) != nullptr) {
                delete item->widget();
                delete item;
            }
            delete plotWidgetStokes->layout();
        }

        QVBoxLayout *layout = new QVBoxLayout(plotWidgetStokes);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(m_chartViewStokes);
        plotWidgetStokes->setLayout(layout);
    }

    qDebug() << "双光谱图表初始化完成 - OpenGL加速已启用，支持高性能实时绘图";

    // ========== 初始化位移台页独立图表（_S，与振镜页同步）==========
    auto initMirrorChart = [this](QChart *&chart, QLineSeries *&series,
                                   QChartView *&view, QWidget *container,
                                   const QString &title) {
        chart = new QChart();
        chart->setTitle(title);
        chart->setAnimationOptions(QChart::NoAnimation);
        chart->legend()->setVisible(false);
        series = new QLineSeries();
        series->setUseOpenGL(true);
        chart->addSeries(series);
        auto *axX = new QValueAxis(); axX->setRange(200, 1100); chart->addAxis(axX, Qt::AlignBottom); series->attachAxis(axX);
        auto *axY = new QValueAxis(); axY->setRange(0, 65535);  chart->addAxis(axY, Qt::AlignLeft);   series->attachAxis(axY);
        view = new QChartView(chart);
        view->setRenderHint(QPainter::Antialiasing);
        // 安装事件过滤器以支持双击放大（与振镜页一致）
        view->installEventFilter(this);
        if (container) {
            // 清理 .ui 中预设的空 layout，避免 setLayout 冲突
            if (container->layout()) {
                QLayoutItem *item;
                while ((item = container->layout()->takeAt(0)) != nullptr) {
                    delete item->widget();
                    delete item;
                }
                delete container->layout();
            }
            QVBoxLayout *l = new QVBoxLayout(container);
            l->setContentsMargins(0, 0, 0, 0);
            l->addWidget(view);
            container->setLayout(l);
        }
    };
    initMirrorChart(m_chartFOPO_S,   m_seriesFOPO_S,   m_chartViewFOPO_S,
                    ui->chartWidgetFOPO_S,   "FOPO路光谱数据");
    initMirrorChart(m_chartStokes_S, m_seriesStokes_S, m_chartViewStokes_S,
                    ui->chartWidgetStokes_S, "Stokes路光谱数据");
}

void Integration::updateSpectrum(const QVector<int> &intensity)
{
    if (!m_seriesFOPO) return;

    int totalPixels = intensity.size();

    // 保存光谱数据
    m_lastSpectrumDataFOPO = intensity;

    // 使用 QVector<QPointF> 批量更新数据，避免多次重绘
    QVector<QPointF> points;
    points.reserve(totalPixels);  // 预分配内存，提高性能

    // 将像素转换为波长并添加数据点
    for (int i = 0; i < totalPixels; ++i) {
        // 波长范围：200-1100 nm
        double wavelength = 200.0 + (900.0 * i / (totalPixels - 1));
        points.append(QPointF(wavelength, intensity[i]));
    }

    // 批量替换数据（只触发一次重绘）
    m_seriesFOPO->replace(points);

    // 自动调整Y轴范围
    if (!intensity.isEmpty()) {
        int maxValue = *std::max_element(intensity.begin(), intensity.end());
        m_axisYFOPO->setRange(0, maxValue * 1.1);
    }

    // 同步更新位移台页 _S 图表
    if (m_seriesFOPO_S) {
        m_seriesFOPO_S->replace(points);
    }

    qDebug() << "FOPO路光谱数据已更新，数据点数量:" << totalPixels;
}

void Integration::updateSpectrumStokes(const QVector<int> &intensity)
{
    if (!m_seriesStokes) return;

    int totalPixels = intensity.size();

    // 保存光谱数据
    m_lastSpectrumDataStokes = intensity;

    // 使用 QVector<QPointF> 批量更新数据，避免多次重绘
    QVector<QPointF> points;
    points.reserve(totalPixels);

    // 将像素转换为波长并添加数据点
    for (int i = 0; i < totalPixels; ++i) {
        // 波长范围：200-1100 nm
        double wavelength = 200.0 + (900.0 * i / (totalPixels - 1));
        points.append(QPointF(wavelength, intensity[i]));
    }

    // 批量替换数据（只触发一次重绘）
    m_seriesStokes->replace(points);

    // 自动调整Y轴范围
    if (!intensity.isEmpty()) {
        int maxValue = *std::max_element(intensity.begin(), intensity.end());
        m_axisYStokes->setRange(0, maxValue * 1.1);
    }

    // 同步更新位移台页 _S 图表
    if (m_seriesStokes_S) {
        m_seriesStokes_S->replace(points);
    }

    qDebug() << "Stokes路光谱数据已更新，数据点数量:" << totalPixels;
}

void Integration::showChartMaximized()
{
    // 如果最大化窗口已存在，直接显示
    if (m_chartMaximizedDialogFOPO) {
        m_chartMaximizedDialogFOPO->show();
        m_chartMaximizedDialogFOPO->raise();
        m_chartMaximizedDialogFOPO->activateWindow();
        return;
    }

    // 创建最大化窗口
    m_chartMaximizedDialogFOPO = new QDialog(this);
    m_chartMaximizedDialogFOPO->setWindowTitle("FOPO路光谱数据 - 最大化视图");
    m_chartMaximizedDialogFOPO->setWindowFlags(Qt::Window | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint);
    m_chartMaximizedDialogFOPO->resize(1200, 800);

    // 创建新的图表和数据系列（复制当前数据）
    QChart *maximizedChart = new QChart();
    maximizedChart->setTitle("FOPO路光谱数据");
    maximizedChart->setAnimationOptions(QChart::NoAnimation);

    // 创建新的数据系列并复制数据
    QLineSeries *maximizedSeries = new QLineSeries();
    maximizedSeries->setName("光谱强度");
    maximizedSeries->setUseOpenGL(true);

    // 如果有数据则复制，否则创建空系列
    if (m_seriesFOPO && m_seriesFOPO->count() > 0) {
        maximizedSeries->replace(m_seriesFOPO->points());
    }

    maximizedChart->addSeries(maximizedSeries);

    // 创建坐标轴（复制当前坐标轴设置）
    QValueAxis *axisX = new QValueAxis();
    axisX->setTitleText("波长 [nm]");
    if (m_axisXFOPO) {
        axisX->setRange(m_axisXFOPO->min(), m_axisXFOPO->max());
    } else {
        axisX->setRange(200, 1100);
    }
    axisX->setGridLineVisible(true);
    axisX->setMinorGridLineVisible(true);
    axisX->setTickCount(10);
    axisX->setMinorTickCount(4);
    maximizedChart->addAxis(axisX, Qt::AlignBottom);
    maximizedSeries->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis();
    axisY->setTitleText("强度 [counts]");
    if (m_axisYFOPO) {
        axisY->setRange(m_axisYFOPO->min(), m_axisYFOPO->max());
    } else {
        axisY->setRange(0, 65535);
    }
    axisY->setGridLineVisible(true);
    axisY->setMinorGridLineVisible(true);
    axisY->setTickCount(10);
    axisY->setMinorTickCount(4);
    maximizedChart->addAxis(axisY, Qt::AlignLeft);
    maximizedSeries->attachAxis(axisY);

    // 显示图例
    maximizedChart->legend()->setVisible(true);
    maximizedChart->legend()->setAlignment(Qt::AlignTop);

    // 创建图表视图
    QChartView *maximizedChartView = new QChartView(maximizedChart, m_chartMaximizedDialogFOPO);
    maximizedChartView->setRenderHint(QPainter::Antialiasing);
    maximizedChartView->setOptimizationFlag(QGraphicsView::DontAdjustForAntialiasing, true);
    maximizedChartView->setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);

    // 设置布局
    QVBoxLayout *layout = new QVBoxLayout(m_chartMaximizedDialogFOPO);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(maximizedChartView);
    m_chartMaximizedDialogFOPO->setLayout(layout);

    // 当窗口关闭时清理资源
    connect(m_chartMaximizedDialogFOPO, &QDialog::finished, this, [this]() {
        // 删除最大化窗口（图表会自动随窗口删除）
        if (m_chartMaximizedDialogFOPO) {
            m_chartMaximizedDialogFOPO->deleteLater();
            m_chartMaximizedDialogFOPO = nullptr;
        }

        qDebug() << "图表最大化窗口已关闭";
    });

    // 显示最大化窗口
    m_chartMaximizedDialogFOPO->showMaximized();

    qDebug() << "图表已最大化显示";
}

// ========== 串口配置获取函数 ==========

SerialConfig Integration::getSpectrometerFOPOSerialConfig()
{
    SerialConfig config;
    config.portName = ui->comboBoxSpectrometerFOPOPort->currentText();
    config.baudRate = ui->comboBoxSpectrometerFOPOBaudRate->currentData().toInt();
    config.dataBits = QSerialPort::Data8;
    config.stopBits = QSerialPort::OneStop;
    config.parity = QSerialPort::NoParity;
    return config;
}

SerialConfig Integration::getDelayLineSerialConfig()
{
    SerialConfig config;
    config.portName = ui->comboBoxDelayPort->currentText();
    config.baudRate = ui->comboBoxDelayBaudRate->currentData().toInt();
    config.dataBits = QSerialPort::Data8;
    config.stopBits = QSerialPort::OneStop;
    config.parity = QSerialPort::NoParity;
    return config;
}

SerialConfig Integration::getDelayLineSerialConfig2()
{
    SerialConfig config;
    config.portName = ui->comboBoxDelay2Port->currentText();
    config.baudRate = ui->comboBoxDelay2BaudRate->currentData().toInt();
    config.dataBits = QSerialPort::Data8;
    config.stopBits = QSerialPort::OneStop;
    config.parity = QSerialPort::NoParity;
    return config;
}

SerialConfig Integration::getStageSerialConfig1()
{
    SerialConfig config;
    config.portName = ui->comboBoxStage1Port->currentText();
    config.baudRate = ui->comboBoxStage1BaudRate->currentData().toInt();
    config.dataBits = QSerialPort::Data8;
    config.stopBits = QSerialPort::OneStop;
    config.parity = QSerialPort::NoParity;
    return config;
}

SerialConfig Integration::getStageSerialConfig2()
{
    SerialConfig config;
    config.portName = ui->comboBoxStage2Port->currentText();
    config.baudRate = ui->comboBoxStage2BaudRate->currentData().toInt();
    config.dataBits = QSerialPort::Data8;
    config.stopBits = QSerialPort::OneStop;
    config.parity = QSerialPort::NoParity;
    return config;
}

QString Integration::getGalvoIPAddress()
{
    return ui->lineEditGalvoIP->text();
}

// ========== 配置保存/加载辅助函数 ==========

void Integration::saveSerialConfig(const QString &device, const QString &port,
                                   qint32 baudRate, int dataBits, int stopBits, int parity)
{
    // TODO: 保存到配置文件
    m_configManager->saveDeviceConfig(device, "portName", port);
    m_configManager->saveDeviceConfig(device, "baudRate", baudRate);
    m_configManager->saveDeviceConfig(device, "dataBits", dataBits);
    m_configManager->saveDeviceConfig(device, "stopBits", stopBits);
    m_configManager->saveDeviceConfig(device, "parity", parity);
    qDebug() << "保存配置:" << device << port << baudRate;
}

void Integration::loadSerialConfig(const QString &device, QString &port,
                                   qint32 &baudRate, int &dataBits, int &stopBits, int &parity)
{
    // TODO: 从配置文件加载
    port = m_configManager->loadDeviceConfig(device, "portName", "COM1").toString();
    baudRate = m_configManager->loadDeviceConfig(device, "baudRate", 9600).toInt();
    dataBits = m_configManager->loadDeviceConfig(device, "dataBits", 8).toInt();
    stopBits = m_configManager->loadDeviceConfig(device, "stopBits", 1).toInt();
    parity = m_configManager->loadDeviceConfig(device, "parity", 0).toInt();
    qDebug() << "加载配置:" << device;
}

// ========== 预设管理函数 ==========

void Integration::initPresetTables()
{
    // 初始化振镜页 - 光源功率预设表格
    m_powerPresetTable = ui->tableWidgetPowerPresets;
    m_powerPresetTable->setColumnCount(6);
    m_powerPresetTable->setHorizontalHeaderLabels({"振镜起始(deg)", "振镜结束(deg)",
                                                    "种子源泵(mA)", "预放泵(mA)", "主级泵(mA)", "Stokes泵(mA)"});
    m_powerPresetTable->horizontalHeader()->setStretchLastSection(false);
    m_powerPresetTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_powerPresetTable->setSelectionMode(QAbstractItemView::SingleSelection);

    // 设置表头字体大小
    QFont headerFont = m_powerPresetTable->horizontalHeader()->font();
    headerFont.setPointSize(9);
    m_powerPresetTable->horizontalHeader()->setFont(headerFont);

    // 设置列宽
    m_powerPresetTable->setColumnWidth(0, 90);
    m_powerPresetTable->setColumnWidth(1, 90);
    m_powerPresetTable->setColumnWidth(2, 90);
    m_powerPresetTable->setColumnWidth(3, 70);
    m_powerPresetTable->setColumnWidth(4, 90);

    // 添加默认6行数据
    const QStringList galvoPowerData[6] = {
        {"0", "0.2", "100", "7", "400"},
        {"0.2", "0.4", "110", "7", "450"},
        {"0.4", "0.6", "90", "7", "400"},
        {"0.6", "0.8", "105", "7", "350"},
        {"0.8", "1.0", "110", "9", "300"},
        {"1.0", "1.2", "100", "10", "370"}
    };

    for (int row = 0; row < 6; ++row) {
        m_powerPresetTable->insertRow(row);
        for (int col = 0; col < 5; ++col) {
            QTableWidgetItem *item = new QTableWidgetItem(galvoPowerData[row][col]);
            item->setTextAlignment(Qt::AlignCenter);
            m_powerPresetTable->setItem(row, col, item);
        }
    }

    // 初始化振镜页 - 延迟线预设表格
    m_delayPresetTableGalvo = ui->tableWidgetDelayPresetsGalvo;
    m_delayPresetTableGalvo->setColumnCount(2);  // 删除序号列,只保留2列数据
    m_delayPresetTableGalvo->setHorizontalHeaderLabels({"振镜角度(deg)", "延迟时间(PS)"});
    m_delayPresetTableGalvo->horizontalHeader()->setStretchLastSection(false);
    m_delayPresetTableGalvo->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_delayPresetTableGalvo->setSelectionMode(QAbstractItemView::SingleSelection);

    // 设置表头字体大小
    headerFont = m_delayPresetTableGalvo->horizontalHeader()->font();
    headerFont.setPointSize(9);
    m_delayPresetTableGalvo->horizontalHeader()->setFont(headerFont);

    // 设置列宽
    m_delayPresetTableGalvo->setColumnWidth(0, 120);
    m_delayPresetTableGalvo->setColumnWidth(1, 120);

    // 添加默认6行数据
    const QStringList galvoDelayData[6] = {
        {"0.1", "3"},
        {"0.2", "4"},
        {"0.3", "5"},
        {"0.4", "6"},
        {"0.5", "7"},
        {"1.2", "150"}
    };

    for (int row = 0; row < 6; ++row) {
        m_delayPresetTableGalvo->insertRow(row);
        for (int col = 0; col < 2; ++col) {
            QTableWidgetItem *item = new QTableWidgetItem(galvoDelayData[row][col]);
            item->setTextAlignment(Qt::AlignCenter);
            m_delayPresetTableGalvo->setItem(row, col, item);
        }
    }

    // 初始化位移台页 - 波长调谐统一表格（整合原电控与功率预设 + 延迟线预设）
    m_wavelengthTuningTable = ui->tableWidgetWavelengthTuningStage;
    m_wavelengthTuningTable->setColumnCount(8);
    m_wavelengthTuningTable->setHorizontalHeaderLabels({
        "波长（mm）", "两个旋转平台位置", "延迟线一", "延迟线二",
        "pump路主级泵功率（mW）", "种子源泵功率（mW）",
        "Stokes路泵功率（mW）", "pump路预放泵功率（mW）"
    });
    m_wavelengthTuningTable->horizontalHeader()->setStretchLastSection(false);
    m_wavelengthTuningTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_wavelengthTuningTable->setSelectionMode(QAbstractItemView::SingleSelection);

    // 设置表头字体大小
    headerFont = m_wavelengthTuningTable->horizontalHeader()->font();
    headerFont.setPointSize(9);
    m_wavelengthTuningTable->horizontalHeader()->setFont(headerFont);

    // 设置列宽
    for (int col = 0; col < 8; ++col) {
        m_wavelengthTuningTable->setColumnWidth(col, 130);
    }

    // 添加默认6行数据
    const QStringList wavelengthData[6] = {
        {"1064", "0", "0", "0", "0", "0", "0", "0"},
        {"1070", "5", "100", "100", "500", "100", "400", "200"},
        {"1080", "10", "200", "200", "1000", "110", "400", "200"},
        {"1090", "15", "300", "300", "1500", "120", "400", "200"},
        {"1100", "20", "400", "400", "2000", "130", "400", "200"},
        {"1110", "25", "500", "500", "2000", "135", "400", "200"}
    };

    for (int row = 0; row < 6; ++row) {
        m_wavelengthTuningTable->insertRow(row);
        for (int col = 0; col < 8; ++col) {
            QTableWidgetItem *item = new QTableWidgetItem(wavelengthData[row][col]);
            item->setTextAlignment(Qt::AlignCenter);
            m_wavelengthTuningTable->setItem(row, col, item);
        }
    }

    // 设置所有表格的最小和最大高度
    m_powerPresetTable->setMinimumHeight(100);
    m_powerPresetTable->setMaximumHeight(250);
    m_delayPresetTableGalvo->setMinimumHeight(100);
    m_delayPresetTableGalvo->setMaximumHeight(250);
    m_wavelengthTuningTable->setMinimumHeight(150);
    m_wavelengthTuningTable->setMaximumHeight(300);

    // 位移台页功率预设表格复用波长调谐统一表格
    m_powerPresetTableStage = m_wavelengthTuningTable;

    qDebug() << "预设表格初始化完成";
}

// ========== 预设管理槽函数 - 振镜页（光源功率预设） ==========

void Integration::on_btnStartPowerExecution_clicked()
{
    // 加载两个预设
    m_currentPowerPresets = loadPowerPresetsFromTable();
    m_currentDelayPresetsGalvo = loadDelayPresetsFromTable(PresetPageType::GalvoPage);

    // 检查是否至少有一个预设
    if (m_currentPowerPresets.isEmpty() && m_currentDelayPresetsGalvo.isEmpty()) {
        QMessageBox::warning(this, "错误", "没有可执行的预设");
        return;
    }

    // 智能检查：分析预设中实际使用的设备
    bool needsGalvo = false;
    bool needsSeed = false;
    bool needsFOPO = false;
    bool needsMain = false;
    bool needsStokes = false;
    bool needsDelay = false;

    // 检查功率预设
    for (const PowerPreset &preset : m_currentPowerPresets) {
        if (preset.galvoAngleStart != 0 || preset.galvoAngleEnd != 0) {
            needsGalvo = true;
        }
        if (preset.seedPumpCurrent != 0) {
            needsSeed = true;
        }
        if (preset.fopoPumpCurrent != 0) {
            needsFOPO = true;
        }
        if (preset.mainPumpCurrent != 0) {
            needsMain = true;
        }
        if (preset.stokesPumpCurrent != 0) {
            needsStokes = true;
        }
    }

    // 检查延迟线预设
    for (const DelayPreset &preset : m_currentDelayPresetsGalvo) {
        if (preset.galvoAngle != 0) {
            needsGalvo = true;
        }
        if (preset.delayTime != 0) {
            needsDelay = true;
        }
    }

    // 检查需要的设备是否连接
    QStringList missingDevices;
    if (needsGalvo && !m_galvoMirror->isConnected()) {
        missingDevices << "振镜控制卡";
    }
    if (needsSeed && (!m_ohldPumps[0] || !m_ohldPumps[0]->isOpen())) {
        missingDevices << "种子源泵（OHLD-1）";
    }
    if (needsFOPO && (!m_ohldPumps[1] || !m_ohldPumps[1]->isOpen())) {
        missingDevices << "pump路预放泵（OHLD-2）";
    }
    if (needsMain && (!m_ohldPumps[2] || !m_ohldPumps[2]->isOpen())) {
        missingDevices << "pump路主级泵（OHLD-3）";
    }
    if (needsStokes && (!m_ohldPumps[3] || !m_ohldPumps[3]->isOpen())) {
        missingDevices << "Stokes泵（OHLD-4）";
    }
    if (needsDelay && !m_delayLine->isConnected()) {
        missingDevices << "延时线";
    }

    // 如果有设备未连接，询问用户是否继续
    if (!missingDevices.isEmpty()) {
        QString message = "以下设备未连接，将跳过相关命令：\n\n";
        message += missingDevices.join("\n");
        message += "\n\n是否继续执行？";

        QMessageBox::StandardButton reply = QMessageBox::question(
            this, "设备未连接", message,
            QMessageBox::Yes | QMessageBox::No);

        if (reply != QMessageBox::Yes) {
            return;
        }
    }

    // 开始执行
    m_currentPowerPresetIndex = 0;
    m_currentDelayPresetIndexGalvo = 0;
    m_isPowerPresetExecuting = true;
    m_isDelayPresetExecutingGalvo = true;

    // 执行第一组
    executeCombinedPresetGalvo(0);

    // 启动定时器
    m_powerPresetDelayTimer->start(m_powerPresetTimeInterval * 1000);

    // 更新状态
    int maxSize = qMax(m_currentPowerPresets.size(), m_currentDelayPresetsGalvo.size());
    updateStatusBar(QString("开始执行预设（共 %1 组）").arg(maxSize));
    qDebug() << "开始执行组合预设，功率预设:" << m_currentPowerPresets.size()
             << "组，延迟线预设:" << m_currentDelayPresetsGalvo.size() << "组";
}

void Integration::on_btnStopPowerExecution_clicked()
{
    stopPowerPresetExecution();
}



void Integration::on_btnConfirmDelayPreset_clicked()
{
    // 获取时间间隔
    m_powerPresetTimeInterval = ui->spinBoxGalvoTimeInterval->value();

    // 调用执行函数
    on_btnStartPowerExecution_clicked();
}

void Integration::onPowerPresetDelayTimeout()
{
    if (!m_isPowerPresetExecuting && !m_isDelayPresetExecutingGalvo) {
        m_powerPresetDelayTimer->stop();
        return;
    }

    // 检查关键设备连接状态（如果预设中使用了该设备）
    bool hasDisconnectedDevice = false;
    QStringList disconnectedDevices;

    if (m_currentPowerPresetIndex < m_currentPowerPresets.size()) {
        const PowerPreset &preset = m_currentPowerPresets[m_currentPowerPresetIndex];
        if (preset.seedPumpCurrent != 0 && (!m_ohldPumps[0] || !m_ohldPumps[0]->isOpen())) {
            disconnectedDevices << "种子源泵（OHLD-1）";
            hasDisconnectedDevice = true;
        }
        if (preset.fopoPumpCurrent != 0 && (!m_ohldPumps[1] || !m_ohldPumps[1]->isOpen())) {
            disconnectedDevices << "pump路预放泵（OHLD-2）";
            hasDisconnectedDevice = true;
        }
        if (preset.mainPumpCurrent != 0 && (!m_ohldPumps[2] || !m_ohldPumps[2]->isOpen())) {
            disconnectedDevices << "pump路主级泵（OHLD-3）";
            hasDisconnectedDevice = true;
        }
        if (preset.stokesPumpCurrent != 0 && (!m_ohldPumps[3] || !m_ohldPumps[3]->isOpen())) {
            disconnectedDevices << "Stokes泵（OHLD-4）";
            hasDisconnectedDevice = true;
        }
    }

    if (m_currentDelayPresetIndexGalvo < m_currentDelayPresetsGalvo.size()) {
        const DelayPreset &preset = m_currentDelayPresetsGalvo[m_currentDelayPresetIndexGalvo];
        if (preset.delayTime != 0 && !m_delayLine->isConnected()) {
            disconnectedDevices << "延时线";
            hasDisconnectedDevice = true;
        }
    }

    // 如果有设备断开，停止执行并提示
    if (hasDisconnectedDevice) {
        stopPowerPresetExecution();
        QMessageBox::warning(this, "设备断开",
            QString("检测到以下设备断开连接，预设执行已停止：\n\n%1")
            .arg(disconnectedDevices.join("\n")));
        return;
    }

    // 计算最大组数
    int maxSize = qMax(m_currentPowerPresets.size(), m_currentDelayPresetsGalvo.size());

    m_currentPowerPresetIndex++;
    m_currentDelayPresetIndexGalvo++;

    if (m_currentPowerPresetIndex >= maxSize) {
        // 所有预设执行完成
        stopPowerPresetExecution();
        QMessageBox::information(this, "完成",
            QString("所有预设执行完成\n功率预设: %1 组\n延迟线预设: %2 组")
            .arg(m_currentPowerPresets.size())
            .arg(m_currentDelayPresetsGalvo.size()));
        return;
    }

    // 执行下一组预设
    executeCombinedPresetGalvo(m_currentPowerPresetIndex);
}



QList<PowerPreset> Integration::loadPowerPresetsFromTable()
{
    QList<PowerPreset> presets;

    for (int row = 0; row < m_powerPresetTable->rowCount(); row++) {
        PowerPreset preset;

        // 读取振镜角度起（第0列）
        QTableWidgetItem *itemStart = m_powerPresetTable->item(row, 0);
        if (itemStart) {
            preset.galvoAngleStart = itemStart->text().toFloat();
        }

        // 读取振镜角度止（第1列）
        QTableWidgetItem *itemEnd = m_powerPresetTable->item(row, 1);
        if (itemEnd) {
            preset.galvoAngleEnd = itemEnd->text().toFloat();
        }

        // 读取种子源泵电流（第2列）
        QTableWidgetItem *itemSeed = m_powerPresetTable->item(row, 2);
        if (itemSeed) {
            preset.seedPumpCurrent = itemSeed->text().toFloat();
        }

        // 读取FOPO/预放泵电流（第3列）
        QTableWidgetItem *itemFOPO = m_powerPresetTable->item(row, 3);
        if (itemFOPO) {
            preset.fopoPumpCurrent = itemFOPO->text().toFloat();
        }

        // 读取主级泵电流（第4列）
        QTableWidgetItem *itemMain = m_powerPresetTable->item(row, 4);
        if (itemMain) {
            preset.mainPumpCurrent = itemMain->text().toFloat();
        }

        // 读取Stokes泵电流（第5列）
        QTableWidgetItem *itemStokes = m_powerPresetTable->item(row, 5);
        if (itemStokes) {
            preset.stokesPumpCurrent = itemStokes->text().toFloat();
        }

        presets.append(preset);
    }

    qDebug() << "从表格加载功率预设，共" << presets.size() << "个";
    return presets;
}

void Integration::executePowerPreset(int index)
{
    if (index < 0 || index >= m_currentPowerPresets.size()) {
        return;
    }

    const PowerPreset &preset = m_currentPowerPresets[index];

    qDebug() << "执行功率预设" << (index + 1) << "/" << m_currentPowerPresets.size();

    QStringList successList;
    QStringList failedList;
    QStringList skippedList;

    // 1. 设置振镜角度（使用起始和结束角度的平均值）
    float galvoAngle = (preset.galvoAngleStart + preset.galvoAngleEnd) / 2.0f;
    if (galvoAngle != 0) {
        if (m_galvoMirror->isConnected()) {
            float x, y, z;
            galvoAngleToCoord(galvoAngle, x, y, z);

            if (m_galvoMirror->scannerJump(x, y, z)) {
                successList << QString("振镜角度: %1 度").arg(galvoAngle, 0, 'f', 1);
            } else {
                failedList << QString("振镜角度: %1 度").arg(galvoAngle, 0, 'f', 1);
            }
        } else {
            skippedList << QString("振镜角度: %1 度（设备未连接）").arg(galvoAngle, 0, 'f', 1);
        }
    }

    // 2. 设置种子源泵功率（OHLD pumpIndex=0）
    if (preset.seedPumpCurrent != 0) {
        if (m_ohldPumps[0] && m_ohldPumps[0]->isOpen()) {
            if (setOhldPumpCurrentSilent(0, preset.seedPumpCurrent)) {
                successList << QString("种子源泵: %1 mA").arg(preset.seedPumpCurrent, 0, 'f', 1);
            } else {
                failedList << QString("种子源泵: %1 mA").arg(preset.seedPumpCurrent, 0, 'f', 1);
            }
        } else {
            skippedList << QString("种子源泵: %1 mA（OHLD串口未连接）").arg(preset.seedPumpCurrent, 0, 'f', 1);
        }
    }

    // 3. 设置FOPO/pump路预放泵功率（OHLD pumpIndex=1）
    if (preset.fopoPumpCurrent != 0) {
        if (m_ohldPumps[1] && m_ohldPumps[1]->isOpen()) {
            if (setOhldPumpCurrentSilent(1, preset.fopoPumpCurrent)) {
                successList << QString("FOPO泵: %1 mA").arg(preset.fopoPumpCurrent, 0, 'f', 1);
            } else {
                failedList << QString("FOPO泵: %1 mA").arg(preset.fopoPumpCurrent, 0, 'f', 1);
            }
        } else {
            skippedList << QString("FOPO泵: %1 mA（OHLD串口未连接）").arg(preset.fopoPumpCurrent, 0, 'f', 1);
        }
    }

    // 4. 设置pump路主级泵功率（OHLD pumpIndex=2）
    if (preset.mainPumpCurrent != 0) {
        if (m_ohldPumps[2] && m_ohldPumps[2]->isOpen()) {
            if (setOhldPumpCurrentSilent(2, preset.mainPumpCurrent)) {
                successList << QString("主级泵: %1 mA").arg(preset.mainPumpCurrent, 0, 'f', 1);
            } else {
                failedList << QString("主级泵: %1 mA").arg(preset.mainPumpCurrent, 0, 'f', 1);
            }
        } else {
            skippedList << QString("主级泵: %1 mA（OHLD串口未连接）").arg(preset.mainPumpCurrent, 0, 'f', 1);
        }
    }

    // 5. 设置Stokes泵功率（OHLD pumpIndex=3）
    if (preset.stokesPumpCurrent != 0) {
        if (m_ohldPumps[3] && m_ohldPumps[3]->isOpen()) {
            if (setOhldPumpCurrentSilent(3, preset.stokesPumpCurrent)) {
                successList << QString("Stokes泵: %1 mA").arg(preset.stokesPumpCurrent, 0, 'f', 1);
            } else {
                failedList << QString("Stokes泵: %1 mA").arg(preset.stokesPumpCurrent, 0, 'f', 1);
            }
        } else {
            skippedList << QString("Stokes泵: %1 mA（OHLD串口未连接）").arg(preset.stokesPumpCurrent, 0, 'f', 1);
        }
    }

    // 输出执行结果
    if (!successList.isEmpty()) {
        qDebug() << "  成功:" << successList.join(", ");
    }
    if (!skippedList.isEmpty()) {
        qDebug() << "  跳过:" << skippedList.join(", ");
    }
    if (!failedList.isEmpty()) {
        qDebug() << "  失败:" << failedList.join(", ");
    }
}

void Integration::executeCombinedPresetGalvo(int index)
{
    // 计算实际使用的索引（如果超出范围，使用最后一个）
    int powerIndex = qMin(index, m_currentPowerPresets.size() - 1);
    int delayIndex = qMin(index, m_currentDelayPresetsGalvo.size() - 1);

    qDebug() << "执行组合预设" << (index + 1) << "功率索引:" << powerIndex << "延迟索引:" << delayIndex;

    // 执行功率预设
    if (powerIndex >= 0 && !m_currentPowerPresets.isEmpty()) {
        executePowerPreset(powerIndex);
    }

    // 执行延迟线预设
    if (delayIndex >= 0 && !m_currentDelayPresetsGalvo.isEmpty()) {
        executeDelayPreset(PresetPageType::GalvoPage, delayIndex);
    }
}

void Integration::stopPowerPresetExecution()
{
    m_isPowerPresetExecuting = false;
    m_isDelayPresetExecutingGalvo = false;
    m_powerPresetDelayTimer->stop();
    updateStatusBar("预设执行已停止");
    qDebug() << "预设执行已停止";
}


// ========== 预设管理槽函数 - 振镜页（延迟线预设） ==========

void Integration::on_btnStartDelayExecutionGalvo_clicked()
{
    // 加载预设
    m_currentDelayPresetsGalvo = loadDelayPresetsFromTable(PresetPageType::GalvoPage);

    if (m_currentDelayPresetsGalvo.isEmpty()) {
        QMessageBox::warning(this, "错误", "没有可执行的预设");
        return;
    }

    // 智能检查
    bool needsGalvo = false;
    bool needsDelay = false;

    for (const DelayPreset &preset : m_currentDelayPresetsGalvo) {
        if (preset.galvoAngle != 0) {
            needsGalvo = true;
        }
        if (preset.delayTime != 0) {
            needsDelay = true;
        }
    }

    QStringList missingDevices;
    if (needsGalvo && !m_galvoMirror->isConnected()) {
        missingDevices << "振镜控制卡";
    }
    if (needsDelay && !m_delayLine->isConnected()) {
        missingDevices << "延时线";
    }

    if (!missingDevices.isEmpty()) {
        QString message = "以下设备未连接，将跳过相关命令：\n\n";
        message += missingDevices.join("\n");
        message += "\n\n是否继续执行？";

        QMessageBox::StandardButton reply = QMessageBox::question(
            this, "设备未连接", message,
            QMessageBox::Yes | QMessageBox::No);

        if (reply != QMessageBox::Yes) {
            return;
        }
    }

    // 开始执行
    m_currentDelayPresetIndexGalvo = 0;
    m_isDelayPresetExecutingGalvo = true;

    executeDelayPreset(PresetPageType::GalvoPage, 0);
    m_delayPresetDelayTimerGalvo->start(m_delayPresetTimeIntervalGalvo * 1000);

    updateStatusBar("开始执行延迟预设（振镜页）");
    qDebug() << "开始执行延迟预设（振镜页），共" << m_currentDelayPresetsGalvo.size() << "个";
}

void Integration::on_btnStopDelayExecutionGalvo_clicked()
{
    stopDelayPresetExecution(PresetPageType::GalvoPage);
}



void Integration::onDelayPresetDelayTimeoutGalvo()
{
    if (!m_isDelayPresetExecutingGalvo) {
        m_delayPresetDelayTimerGalvo->stop();
        return;
    }

    // 检查关键设备连接状态（如果预设中使用了该设备）
    bool hasDisconnectedDevice = false;
    QStringList disconnectedDevices;

    if (m_currentDelayPresetIndexGalvo < m_currentDelayPresetsGalvo.size()) {
        const DelayPreset &preset = m_currentDelayPresetsGalvo[m_currentDelayPresetIndexGalvo];
        if (preset.delayTime != 0 && !m_delayLine->isConnected()) {
            disconnectedDevices << "延时线";
            hasDisconnectedDevice = true;
        }
        // 振镜暂时不可用，不检查
    }

    // 如果有设备断开，停止执行并提示
    if (hasDisconnectedDevice) {
        stopDelayPresetExecution(PresetPageType::GalvoPage);
        QMessageBox::warning(this, "设备断开",
            QString("检测到以下设备断开连接，预设执行已停止：\n\n%1")
            .arg(disconnectedDevices.join("\n")));
        return;
    }

    m_currentDelayPresetIndexGalvo++;

    if (m_currentDelayPresetIndexGalvo >= m_currentDelayPresetsGalvo.size()) {
        stopDelayPresetExecution(PresetPageType::GalvoPage);
        QMessageBox::information(this, "完成", "所有延迟预设执行完成（振镜页）");
        return;
    }

    executeDelayPreset(PresetPageType::GalvoPage, m_currentDelayPresetIndexGalvo);
}

// ========== 预设管理槽函数 - 位移台页（电控与功率预设） ==========

void Integration::on_btnStartPowerExecutionStage_clicked()
{
    // 加载预设
    m_currentPowerPresetsStage = loadPowerPresetsFromTableStage();

    if (m_currentPowerPresetsStage.isEmpty()) {
        QMessageBox::warning(this, "错误", "没有可执行的预设");
        return;
    }

    // 智能检查
    bool needsStage = false;
    bool needsSeed = false;
    bool needsFOPO = false;
    bool needsMain = false;
    bool needsStokes = false;

    for (const StagePowerPreset &preset : m_currentPowerPresetsStage) {
        if (preset.stageAngle != 0 || preset.stagePosition != 0) {
            needsStage = true;
        }
        if (preset.seedPumpCurrent != 0) {
            needsSeed = true;
        }
        if (preset.fopoPumpCurrent != 0) {
            needsFOPO = true;
        }
        if (preset.mainPumpCurrent != 0) {
            needsMain = true;
        }
        if (preset.stokesPumpCurrent != 0) {
            needsStokes = true;
        }
    }

    QStringList missingDevices;
    if (needsStage && !m_stageController->isConnected()) {
        missingDevices << "位移台";
    }
    if (needsSeed && (!m_ohldPumps[0] || !m_ohldPumps[0]->isOpen())) {
        missingDevices << "种子源泵（OHLD-1）";
    }
    if (needsFOPO && (!m_ohldPumps[1] || !m_ohldPumps[1]->isOpen())) {
        missingDevices << "pump路预放泵（OHLD-2）";
    }
    if (needsMain && (!m_ohldPumps[2] || !m_ohldPumps[2]->isOpen())) {
        missingDevices << "pump路主级泵（OHLD-3）";
    }
    if (needsStokes && (!m_ohldPumps[3] || !m_ohldPumps[3]->isOpen())) {
        missingDevices << "Stokes泵（OHLD-4）";
    }

    if (!missingDevices.isEmpty()) {
        QString message = "以下设备未连接，将跳过相关命令：\n\n";
        message += missingDevices.join("\n");
        message += "\n\n是否继续执行？";

        QMessageBox::StandardButton reply = QMessageBox::question(
            this, "设备未连接", message,
            QMessageBox::Yes | QMessageBox::No);

        if (reply != QMessageBox::Yes) {
            return;
        }
    }

    // 开始执行
    m_currentPowerPresetIndexStage = 0;
    m_isPowerPresetExecutingStage = true;

    executePowerPresetStage(0);
    m_powerPresetDelayTimerStage->start(m_powerPresetTimeIntervalStage * 1000);

    updateStatusBar("开始执行功率预设（位移台页）");
    qDebug() << "开始执行功率预设（位移台页），共" << m_currentPowerPresetsStage.size() << "个";
}

void Integration::on_btnStopPowerExecutionStage_clicked()
{
    stopPowerPresetExecutionStage();
}



void Integration::onPowerPresetDelayTimeoutStage()
{
    if (!m_isPowerPresetExecutingStage) {
        m_powerPresetDelayTimerStage->stop();
        return;
    }

    // 检查关键设备连接状态（如果预设中使用了该设备）
    bool hasDisconnectedDevice = false;
    QStringList disconnectedDevices;

    if (m_currentPowerPresetIndexStage < m_currentPowerPresetsStage.size()) {
        const StagePowerPreset &preset = m_currentPowerPresetsStage[m_currentPowerPresetIndexStage];
        if (preset.stageAngle != 0 && !m_stageController->isConnected()) {
            disconnectedDevices << "旋转台";
            hasDisconnectedDevice = true;
        }
        if (preset.stagePosition != 0 && !m_stageController->isConnected()) {
            disconnectedDevices << "直线台";
            hasDisconnectedDevice = true;
        }
        if (preset.seedPumpCurrent != 0 && (!m_ohldPumps[0] || !m_ohldPumps[0]->isOpen())) {
            disconnectedDevices << "种子源泵（OHLD-1）";
            hasDisconnectedDevice = true;
        }
        if (preset.fopoPumpCurrent != 0 && (!m_ohldPumps[1] || !m_ohldPumps[1]->isOpen())) {
            disconnectedDevices << "pump路预放泵（OHLD-2）";
            hasDisconnectedDevice = true;
        }
        if (preset.mainPumpCurrent != 0 && (!m_ohldPumps[2] || !m_ohldPumps[2]->isOpen())) {
            disconnectedDevices << "pump路主级泵（OHLD-3）";
            hasDisconnectedDevice = true;
        }
        if (preset.stokesPumpCurrent != 0 && (!m_ohldPumps[3] || !m_ohldPumps[3]->isOpen())) {
            disconnectedDevices << "Stokes泵（OHLD-4）";
            hasDisconnectedDevice = true;
        }
    }

    // 如果有设备断开，停止执行并提示
    if (hasDisconnectedDevice) {
        stopPowerPresetExecutionStage();
        QMessageBox::warning(this, "设备断开",
            QString("检测到以下设备断开连接，预设执行已停止：\n\n%1")
            .arg(disconnectedDevices.join("\n")));
        return;
    }

    m_currentPowerPresetIndexStage++;

    if (m_currentPowerPresetIndexStage >= m_currentPowerPresetsStage.size()) {
        stopPowerPresetExecutionStage();
        QMessageBox::information(this, "完成", "所有功率预设执行完成（位移台页）");
        return;
    }

    executePowerPresetStage(m_currentPowerPresetIndexStage);
}

// ========== 位移台页扫描槽函数（待实现） ==========

void Integration::on_btnStageScanSetWavelength_clicked()
{
    // TODO: 单点波长设置 - 读取 lineEditStageScanWavelength 并执行设置
}

void Integration::on_btnStageScanStart_clicked()
{
    // TODO: 开始扫描 - 读取起始/终止/步进/时间间隔参数并启动扫描流程
}

void Integration::on_btnStageScanStop_clicked()
{
    // TODO: 停止扫描 - 中断当前扫描流程
}



QList<StagePowerPreset> Integration::loadPowerPresetsFromTableStage()
{
    QList<StagePowerPreset> presets;

    for (int row = 0; row < m_powerPresetTableStage->rowCount(); row++) {
        StagePowerPreset preset;

        // 波长调谐统一表格列布局：
        // 0=波长, 1=旋转平台位置, 2=延迟线一, 3=延迟线二,
        // 4=pump路主级泵, 5=种子源泵, 6=Stokes路泵, 7=pump路预放泵

        // 读取旋转平台位置（列1）→ stagePosition
        QTableWidgetItem *itemPos = m_powerPresetTableStage->item(row, 1);
        if (itemPos) {
            preset.stagePosition = itemPos->text().toFloat();
        }

        // 读取种子源泵电流（列5）
        QTableWidgetItem *itemSeed = m_powerPresetTableStage->item(row, 5);
        if (itemSeed) {
            preset.seedPumpCurrent = itemSeed->text().toFloat();
        }

        // 读取pump路预放泵电流（列7）
        QTableWidgetItem *itemFOPO = m_powerPresetTableStage->item(row, 7);
        if (itemFOPO) {
            preset.fopoPumpCurrent = itemFOPO->text().toFloat();
        }

        // 读取pump路主级泵电流（列4）
        QTableWidgetItem *itemMain = m_powerPresetTableStage->item(row, 4);
        if (itemMain) {
            preset.mainPumpCurrent = itemMain->text().toFloat();
        }

        // 读取Stokes泵电流（列6）
        QTableWidgetItem *itemStokes = m_powerPresetTableStage->item(row, 6);
        if (itemStokes) {
            preset.stokesPumpCurrent = itemStokes->text().toFloat();
        }

        presets.append(preset);
    }

    qDebug() << "从表格加载功率预设（位移台页），共" << presets.size() << "个";
    return presets;
}

void Integration::executePowerPresetStage(int index)
{
    if (index < 0 || index >= m_currentPowerPresetsStage.size()) {
        return;
    }

    const StagePowerPreset &preset = m_currentPowerPresetsStage[index];

    qDebug() << "执行功率预设（位移台页）" << (index + 1) << "/" << m_currentPowerPresetsStage.size();

    QStringList successList;
    QStringList failedList;
    QStringList skippedList;

    // 1. 双台同步绝对位移（使用 stageAngle 字段作为位移量，单位 mm）
    double stageMm = (preset.stageAngle != 0) ? preset.stageAngle
                   : (preset.stagePosition != 0) ? preset.stagePosition : 0.0;
    if (stageMm != 0) {
        if (m_stageController->isConnected()) {
            if (m_stageController->moveAbsoluteDual(stageMm)) {
                successList << QString("双台绝对位移: %1 mm").arg(stageMm, 0, 'f', 3);
            } else {
                failedList << QString("双台绝对位移: %1 mm").arg(stageMm, 0, 'f', 3);
            }
        } else {
            skippedList << QString("双台绝对位移: %1 mm（设备未连接）").arg(stageMm, 0, 'f', 3);
        }
    }

    // 3. 设置种子源泵功率（OHLD pumpIndex=0）
    if (preset.seedPumpCurrent != 0) {
        if (m_ohldPumps[0] && m_ohldPumps[0]->isOpen()) {
            if (setOhldPumpCurrentSilent(0, preset.seedPumpCurrent)) {
                successList << QString("种子源泵: %1 mA").arg(preset.seedPumpCurrent, 0, 'f', 1);
            } else {
                failedList << QString("种子源泵: %1 mA").arg(preset.seedPumpCurrent, 0, 'f', 1);
            }
        } else {
            skippedList << QString("种子源泵: %1 mA（OHLD串口未连接）").arg(preset.seedPumpCurrent, 0, 'f', 1);
        }
    }

    // 4. 设置FOPO/pump路预放泵功率（OHLD pumpIndex=1）
    if (preset.fopoPumpCurrent != 0) {
        if (m_ohldPumps[1] && m_ohldPumps[1]->isOpen()) {
            if (setOhldPumpCurrentSilent(1, preset.fopoPumpCurrent)) {
                successList << QString("FOPO泵: %1 mA").arg(preset.fopoPumpCurrent, 0, 'f', 1);
            } else {
                failedList << QString("FOPO泵: %1 mA").arg(preset.fopoPumpCurrent, 0, 'f', 1);
            }
        } else {
            skippedList << QString("FOPO泵: %1 mA（OHLD串口未连接）").arg(preset.fopoPumpCurrent, 0, 'f', 1);
        }
    }

    // 5. 设置pump路主级泵功率（OHLD pumpIndex=2）
    if (preset.mainPumpCurrent != 0) {
        if (m_ohldPumps[2] && m_ohldPumps[2]->isOpen()) {
            if (setOhldPumpCurrentSilent(2, preset.mainPumpCurrent)) {
                successList << QString("主级泵: %1 mA").arg(preset.mainPumpCurrent, 0, 'f', 1);
            } else {
                failedList << QString("主级泵: %1 mA").arg(preset.mainPumpCurrent, 0, 'f', 1);
            }
        } else {
            skippedList << QString("主级泵: %1 mA（OHLD串口未连接）").arg(preset.mainPumpCurrent, 0, 'f', 1);
        }
    }

    // 6. 设置Stokes泵功率（OHLD pumpIndex=3）
    if (preset.stokesPumpCurrent != 0) {
        if (m_ohldPumps[3] && m_ohldPumps[3]->isOpen()) {
            if (setOhldPumpCurrentSilent(3, preset.stokesPumpCurrent)) {
                successList << QString("Stokes泵: %1 mA").arg(preset.stokesPumpCurrent, 0, 'f', 1);
            } else {
                failedList << QString("Stokes泵: %1 mA").arg(preset.stokesPumpCurrent, 0, 'f', 1);
            }
        } else {
            skippedList << QString("Stokes泵: %1 mA（OHLD串口未连接）").arg(preset.stokesPumpCurrent, 0, 'f', 1);
        }
    }

    // 输出执行结果
    if (!successList.isEmpty()) {
        qDebug() << "  成功:" << successList.join(", ");
    }
    if (!skippedList.isEmpty()) {
        qDebug() << "  跳过:" << skippedList.join(", ");
    }
    if (!failedList.isEmpty()) {
        qDebug() << "  失败:" << failedList.join(", ");
    }
}

void Integration::stopPowerPresetExecutionStage()
{
    m_isPowerPresetExecutingStage = false;
    m_powerPresetDelayTimerStage->stop();
    updateStatusBar("功率预设执行已停止（位移台页）");
    qDebug() << "功率预设执行已停止（位移台页）";
}

// ========== 预设管理槽函数 - 位移台页（延迟线预设） ==========

void Integration::on_btnStartDelayExecution_clicked()
{
    // 加载预设
    m_currentDelayPresets = loadDelayPresetsFromTable(PresetPageType::StagePage);

    if (m_currentDelayPresets.isEmpty()) {
        QMessageBox::warning(this, "错误", "没有可执行的预设");
        return;
    }

    // 智能检查：分析预设中实际使用的设备
    bool needsStage = false;
    bool needsDelay = false;

    for (const DelayPreset &preset : m_currentDelayPresets) {
        if (preset.stagePosition != 0) {
            needsStage = true;
        }
        if (preset.delayTime != 0) {
            needsDelay = true;
        }
    }

    QStringList missingDevices;
    if (needsStage && !m_stageController->isConnected()) {
        missingDevices << "旋转台";
    }
    if (needsDelay && !m_delayLine->isConnected()) {
        missingDevices << "延时线";
    }

    if (!missingDevices.isEmpty()) {
        QString message = "以下设备未连接，将跳过相关命令：\n\n";
        message += missingDevices.join("\n");
        message += "\n\n是否继续执行？";

        QMessageBox::StandardButton reply = QMessageBox::question(
            this, "设备未连接", message,
            QMessageBox::Yes | QMessageBox::No);

        if (reply != QMessageBox::Yes) {
            return;
        }
    }

    // 开始执行
    m_currentDelayPresetIndex = 0;
    m_isDelayPresetExecuting = true;

    executeDelayPreset(PresetPageType::StagePage, 0);
    m_delayPresetDelayTimer->start(m_delayPresetTimeInterval * 1000);

    updateStatusBar("开始执行延迟预设（位移台页）");
    qDebug() << "开始执行延迟预设（位移台页），共" << m_currentDelayPresets.size() << "个";
}

void Integration::on_btnStopDelayExecution_clicked()
{
    stopDelayPresetExecution(PresetPageType::StagePage);
}



void Integration::onDelayPresetDelayTimeout()
{
    if (!m_isDelayPresetExecuting) {
        m_delayPresetDelayTimer->stop();
        return;
    }

    // 检查关键设备连接状态（如果预设中使用了该设备）
    bool hasDisconnectedDevice = false;
    QStringList disconnectedDevices;

    if (m_currentDelayPresetIndex < m_currentDelayPresets.size()) {
        const DelayPreset &preset = m_currentDelayPresets[m_currentDelayPresetIndex];
        if (preset.delayTime != 0 && !m_delayLine->isConnected()) {
            disconnectedDevices << "延时线";
            hasDisconnectedDevice = true;
        }
        // 振镜暂时不可用，不检查
    }

    // 如果有设备断开，停止执行并提示
    if (hasDisconnectedDevice) {
        stopDelayPresetExecution(PresetPageType::StagePage);
        QMessageBox::warning(this, "设备断开",
            QString("检测到以下设备断开连接，预设执行已停止：\n\n%1")
            .arg(disconnectedDevices.join("\n")));
        return;
    }

    m_currentDelayPresetIndex++;

    if (m_currentDelayPresetIndex >= m_currentDelayPresets.size()) {
        stopDelayPresetExecution(PresetPageType::StagePage);
        QMessageBox::information(this, "完成", "所有延迟预设执行完成（位移台页）");
        return;
    }

    executeDelayPreset(PresetPageType::StagePage, m_currentDelayPresetIndex);
}


// ========== 统一的延迟预设方法 ==========



QList<DelayPreset> Integration::loadDelayPresetsFromTable(PresetPageType pageType)
{
    QList<DelayPreset> presets;
    QTableWidget *table = getDelayPresetTable(pageType);

    if (!table) {
        qDebug() << "错误：无法获取延迟预设表格";
        return presets;
    }

    for (int row = 0; row < table->rowCount(); row++) {
        DelayPreset preset;

        if (pageType == PresetPageType::GalvoPage) {
            // 振镜页：读取振镜角度（第0列）
            QTableWidgetItem *itemAngle = table->item(row, 0);
            if (itemAngle) {
                preset.galvoAngle = itemAngle->text().toFloat();
            }
        } else {
            // 位移台页：读取旋转台角度（第0列）
            QTableWidgetItem *itemAngle = table->item(row, 0);
            if (itemAngle) {
                preset.stagePosition = itemAngle->text().toFloat();
            }
        }

        // 读取延迟时间（第1列）
        QTableWidgetItem *itemDelay = table->item(row, 1);
        if (itemDelay) {
            preset.delayTime = itemDelay->text().toFloat();
        }

        presets.append(preset);
    }

    QString pageName = (pageType == PresetPageType::GalvoPage) ? "振镜页" : "位移台页";
    qDebug() << "从表格加载延迟预设（" << pageName << "），共" << presets.size() << "个";
    return presets;
}

void Integration::executeDelayPreset(PresetPageType pageType, int index)
{
    QList<DelayPreset> &presets = getCurrentDelayPresets(pageType);

    if (index < 0 || index >= presets.size()) {
        return;
    }

    const DelayPreset &preset = presets[index];
    QString pageName = (pageType == PresetPageType::GalvoPage) ? "振镜页" : "位移台页";

    qDebug() << "执行延迟预设（" << pageName << "）" << (index + 1) << "/" << presets.size();

    QStringList successList;
    QStringList failedList;
    QStringList skippedList;

    if (pageType == PresetPageType::GalvoPage) {
        // 振镜页：设置振镜角度
        if (preset.galvoAngle != 0) {
            if (m_galvoMirror->isConnected()) {
                float x, y, z;
                galvoAngleToCoord(preset.galvoAngle, x, y, z);

                if (m_galvoMirror->scannerJump(x, y, z)) {
                    successList << QString("振镜角度: %1 度").arg(preset.galvoAngle, 0, 'f', 1);
                } else {
                    failedList << QString("振镜角度: %1 度").arg(preset.galvoAngle, 0, 'f', 1);
                }
            } else {
                skippedList << QString("振镜角度: %1 度（设备未连接）").arg(preset.galvoAngle, 0, 'f', 1);
            }
        }
    } else {
        // 位移台页：双台同步绝对位移
        if (preset.stagePosition != 0) {
            if (m_stageController->isConnected()) {
                if (m_stageController->moveAbsoluteDual(preset.stagePosition)) {
                    successList << QString("双台绝对位移: %1 mm").arg(preset.stagePosition, 0, 'f', 3);
                } else {
                    failedList << QString("双台绝对位移: %1 mm").arg(preset.stagePosition, 0, 'f', 3);
                }
            } else {
                skippedList << QString("双台绝对位移: %1 mm（设备未连接）").arg(preset.stagePosition, 0, 'f', 3);
            }
        }
    }

    // 2. 设置延时线延迟（两个页面都需要）
    if (preset.delayTime != 0) {
        if (m_delayLine->isConnected()) {
            if (m_delayLine->setDelay(preset.delayTime)) {
                successList << QString("延时线: %1 PS").arg(preset.delayTime, 0, 'f', 1);
            } else {
                failedList << QString("延时线: %1 PS").arg(preset.delayTime, 0, 'f', 1);
            }
        } else {
            skippedList << QString("延时线: %1 PS（设备未连接）").arg(preset.delayTime, 0, 'f', 1);
        }
    }

    // 输出执行结果
    if (!successList.isEmpty()) {
        qDebug() << "  成功:" << successList.join(", ");
    }
    if (!skippedList.isEmpty()) {
        qDebug() << "  跳过:" << skippedList.join(", ");
    }
    if (!failedList.isEmpty()) {
        qDebug() << "  失败:" << failedList.join(", ");
    }
}

void Integration::stopDelayPresetExecution(PresetPageType pageType)
{
    bool &isExecuting = isDelayPresetExecuting(pageType);
    QTimer *timer = getDelayPresetTimer(pageType);
    QString pageName = (pageType == PresetPageType::GalvoPage) ? "振镜页" : "位移台页";

    isExecuting = false;
    if (timer) {
        timer->stop();
    }

    updateStatusBar("延迟预设执行已停止（" + pageName + "）");
    qDebug() << "延迟预设执行已停止（" << pageName << "）";
}

// ========== 辅助方法：根据页面类型获取对应的成员变量 ==========

QTableWidget* Integration::getDelayPresetTable(PresetPageType pageType)
{
    return (pageType == PresetPageType::GalvoPage) ? m_delayPresetTableGalvo : m_delayPresetTable;
}

QTimer* Integration::getDelayPresetTimer(PresetPageType pageType)
{
    return (pageType == PresetPageType::GalvoPage) ? m_delayPresetDelayTimerGalvo : m_delayPresetDelayTimer;
}

QList<DelayPreset>& Integration::getCurrentDelayPresets(PresetPageType pageType)
{
    return (pageType == PresetPageType::GalvoPage) ? m_currentDelayPresetsGalvo : m_currentDelayPresets;
}

int& Integration::getCurrentDelayPresetIndex(PresetPageType pageType)
{
    return (pageType == PresetPageType::GalvoPage) ? m_currentDelayPresetIndexGalvo : m_currentDelayPresetIndex;
}

bool& Integration::isDelayPresetExecuting(PresetPageType pageType)
{
    return (pageType == PresetPageType::GalvoPage) ? m_isDelayPresetExecutingGalvo : m_isDelayPresetExecuting;
}

int& Integration::getDelayPresetTimeInterval(PresetPageType pageType)
{
    return (pageType == PresetPageType::GalvoPage) ? m_delayPresetTimeIntervalGalvo : m_delayPresetTimeInterval;
}





void Integration::showChartMaximizedStokes()
{
    // 如果最大化窗口已存在，直接显示
    if (m_chartMaximizedDialogStokes) {
        m_chartMaximizedDialogStokes->show();
        m_chartMaximizedDialogStokes->raise();
        m_chartMaximizedDialogStokes->activateWindow();
        return;
    }

    // 创建最大化窗口
    m_chartMaximizedDialogStokes = new QDialog(this);
    m_chartMaximizedDialogStokes->setWindowTitle("Stokes路光谱数据 - 最大化视图");
    m_chartMaximizedDialogStokes->setWindowFlags(Qt::Window | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint);
    m_chartMaximizedDialogStokes->resize(1200, 800);

    // 创建新的图表和数据系列（复制当前数据）
    QChart *maximizedChart = new QChart();
    maximizedChart->setTitle("Stokes路光谱数据");
    maximizedChart->setAnimationOptions(QChart::NoAnimation);

    // 创建新的数据系列并复制数据
    QLineSeries *maximizedSeries = new QLineSeries();
    maximizedSeries->setName("光谱强度");
    maximizedSeries->setUseOpenGL(true);

    // 如果有数据则复制，否则创建空系列
    if (m_seriesStokes && m_seriesStokes->count() > 0) {
        maximizedSeries->replace(m_seriesStokes->points());
    }

    maximizedChart->addSeries(maximizedSeries);

    // 创建坐标轴（复制当前坐标轴设置）
    QValueAxis *axisX = new QValueAxis();
    axisX->setTitleText("波长 [nm]");
    if (m_axisXStokes) {
        axisX->setRange(m_axisXStokes->min(), m_axisXStokes->max());
    } else {
        axisX->setRange(200, 1100);
    }
    axisX->setGridLineVisible(true);
    axisX->setMinorGridLineVisible(true);
    axisX->setTickCount(10);
    axisX->setMinorTickCount(4);
    maximizedChart->addAxis(axisX, Qt::AlignBottom);
    maximizedSeries->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis();
    axisY->setTitleText("强度 [counts]");
    if (m_axisYStokes) {
        axisY->setRange(m_axisYStokes->min(), m_axisYStokes->max());
    } else {
        axisY->setRange(0, 65535);
    }
    axisY->setGridLineVisible(true);
    axisY->setMinorGridLineVisible(true);
    axisY->setTickCount(10);
    axisY->setMinorTickCount(4);
    maximizedChart->addAxis(axisY, Qt::AlignLeft);
    maximizedSeries->attachAxis(axisY);

    // 显示图例
    maximizedChart->legend()->setVisible(true);
    maximizedChart->legend()->setAlignment(Qt::AlignTop);

    // 创建图表视图
    QChartView *maximizedChartView = new QChartView(maximizedChart, m_chartMaximizedDialogStokes);
    maximizedChartView->setRenderHint(QPainter::Antialiasing);
    maximizedChartView->setOptimizationFlag(QGraphicsView::DontAdjustForAntialiasing, true);
    maximizedChartView->setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);

    // 设置布局
    QVBoxLayout *layout = new QVBoxLayout(m_chartMaximizedDialogStokes);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(maximizedChartView);
    m_chartMaximizedDialogStokes->setLayout(layout);

    // 当窗口关闭时清理资源
    connect(m_chartMaximizedDialogStokes, &QDialog::finished, this, [this]() {
        // 删除最大化窗口（图表会自动随窗口删除）
        if (m_chartMaximizedDialogStokes) {
            m_chartMaximizedDialogStokes->deleteLater();
            m_chartMaximizedDialogStokes = nullptr;
        }

        qDebug() << "Stokes路图表最大化窗口已关闭";
    });

    // 显示最大化窗口
    m_chartMaximizedDialogStokes->showMaximized();

    qDebug() << "Stokes路图表已最大化显示";
}

SerialConfig Integration::getSpectrometerStokesSerialConfig()
{
    SerialConfig config;
    config.portName = ui->comboBoxSpectrometerStokesPort->currentText();
    config.baudRate = ui->comboBoxSpectrometerStokesBaudRate->currentData().toInt();
    config.dataBits = QSerialPort::Data8;
    config.stopBits = QSerialPort::OneStop;
    config.parity = QSerialPort::NoParity;
    return config;
}

// 编辑按钮槽函数实现

void Integration::on_btnEditPowerPresets_clicked()
{
    showPowerPresetEditDialog();
}

void Integration::on_btnEditDelayPresetsGalvo_clicked()
{
    showDelayPresetEditDialogGalvo();
}

void Integration::on_btnEditPowerPresetsStage_clicked()
{
    showPowerPresetEditDialogStage();
}

void Integration::on_btnEditDelayPresets_clicked()
{
    showDelayPresetEditDialog();
}

void Integration::on_btnEditWavelengthTuningStage_clicked()
{
    showWavelengthTuningEditDialogStage();
}

// 编辑弹窗辅助方法实现

void Integration::showPowerPresetEditDialog()
{
    // 创建编辑弹窗
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle("编辑光源功率预设");
    dialog->resize(700, 400);

    // 创建垂直布局
    QVBoxLayout *mainLayout = new QVBoxLayout(dialog);

    // 创建表格
    QTableWidget *tableWidget = new QTableWidget(dialog);
    tableWidget->setColumnCount(6);
    tableWidget->setHorizontalHeaderLabels({"振镜起始(deg)", "振镜结束(deg)", "种子源泵(mA)", "预放泵(mA)", "主级泵(mA)", "Stokes泵(mA)"});
    tableWidget->setEditTriggers(QAbstractItemView::AllEditTriggers);
    tableWidget->setAlternatingRowColors(true);

    // 从原表格加载数据
    QTableWidget *sourceTable = ui->tableWidgetPowerPresets;
    for (int row = 0; row < sourceTable->rowCount(); ++row) {
        tableWidget->insertRow(row);
        for (int col = 0; col < sourceTable->columnCount(); ++col) {
            QTableWidgetItem *item = sourceTable->item(row, col);
            if (item) {
                tableWidget->setItem(row, col, new QTableWidgetItem(item->text()));
            }
        }
    }

    // 创建按钮布局
    QHBoxLayout *buttonLayout = new QHBoxLayout();

    // 添加按钮
    QPushButton *btnAdd = new QPushButton("+", dialog);
    QPushButton *btnDelete = new QPushButton("-", dialog);
    QPushButton *btnSave = new QPushButton("保存", dialog);
    QPushButton *btnCancel = new QPushButton("取消", dialog);

    // 添加按钮到布局
    buttonLayout->addWidget(btnAdd);
    buttonLayout->addWidget(btnDelete);
    buttonLayout->addStretch();
    buttonLayout->addWidget(btnSave);
    buttonLayout->addWidget(btnCancel);

    // 添加表格和按钮布局到主布局
    mainLayout->addWidget(tableWidget);
    mainLayout->addLayout(buttonLayout);

    // 连接信号槽
    connect(btnAdd, &QPushButton::clicked, [=]() {
        int row = tableWidget->rowCount();
        tableWidget->insertRow(row);
        tableWidget->setItem(row, 0, new QTableWidgetItem(QString::number(row + 1)));
        tableWidget->setItem(row, 1, new QTableWidgetItem("0"));
        tableWidget->setItem(row, 2, new QTableWidgetItem("0"));
        tableWidget->setItem(row, 3, new QTableWidgetItem("0"));
        tableWidget->setItem(row, 4, new QTableWidgetItem("0"));
        tableWidget->setItem(row, 5, new QTableWidgetItem("0"));
    });

    connect(btnDelete, &QPushButton::clicked, [=]() {
        int currentRow = tableWidget->currentRow();
        if (currentRow >= 0) {
            tableWidget->removeRow(currentRow);
            // 更新序号
            for (int i = 0; i < tableWidget->rowCount(); ++i) {
                tableWidget->setItem(i, 0, new QTableWidgetItem(QString::number(i + 1)));
            }
        }
    });

    connect(btnSave, &QPushButton::clicked, [=]() {
        // 保存数据到原表格
        QTableWidget *sourceTable = ui->tableWidgetPowerPresets;
        sourceTable->setRowCount(0);

        for (int row = 0; row < tableWidget->rowCount(); ++row) {
            sourceTable->insertRow(row);
            for (int col = 0; col < tableWidget->columnCount(); ++col) {
                QTableWidgetItem *item = tableWidget->item(row, col);
                if (item) {
                    QTableWidgetItem *newItem = new QTableWidgetItem(item->text());
                    newItem->setTextAlignment(Qt::AlignCenter);
                    sourceTable->setItem(row, col, newItem);
                }
            }
        }

        dialog->accept();
    });

    connect(btnCancel, &QPushButton::clicked, dialog, &QDialog::reject);

    // 显示弹窗
    dialog->exec();

    // 清理资源
    delete dialog;
}

void Integration::showDelayPresetEditDialogGalvo()
{
    // 创建编辑弹窗
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle("编辑延迟线预设");
    dialog->resize(400, 400);

    // 创建垂直布局
    QVBoxLayout *mainLayout = new QVBoxLayout(dialog);

    // 创建表格
    QTableWidget *tableWidget = new QTableWidget(dialog);
    tableWidget->setColumnCount(2);
    tableWidget->setHorizontalHeaderLabels({"振镜角度(deg)", "延迟时间(PS)"});
    tableWidget->setEditTriggers(QAbstractItemView::AllEditTriggers);
    tableWidget->setAlternatingRowColors(true);

    // 从原表格加载数据
    QTableWidget *sourceTable = ui->tableWidgetDelayPresetsGalvo;
    for (int row = 0; row < sourceTable->rowCount(); ++row) {
        tableWidget->insertRow(row);
        for (int col = 0; col < sourceTable->columnCount(); ++col) {
            QTableWidgetItem *item = sourceTable->item(row, col);
            if (item) {
                tableWidget->setItem(row, col, new QTableWidgetItem(item->text()));
            }
        }
    }

    // 创建按钮布局
    QHBoxLayout *buttonLayout = new QHBoxLayout();

    // 添加按钮
    QPushButton *btnAdd = new QPushButton("+", dialog);
    QPushButton *btnDelete = new QPushButton("-", dialog);
    QPushButton *btnSave = new QPushButton("保存", dialog);
    QPushButton *btnCancel = new QPushButton("取消", dialog);

    // 添加按钮到布局
    buttonLayout->addWidget(btnAdd);
    buttonLayout->addWidget(btnDelete);
    buttonLayout->addStretch();
    buttonLayout->addWidget(btnSave);
    buttonLayout->addWidget(btnCancel);

    // 添加表格和按钮布局到主布局
    mainLayout->addWidget(tableWidget);
    mainLayout->addLayout(buttonLayout);

    // 连接信号槽
    connect(btnAdd, &QPushButton::clicked, [=]() {
        int row = tableWidget->rowCount();
        tableWidget->insertRow(row);
        tableWidget->setItem(row, 0, new QTableWidgetItem("0"));
        tableWidget->setItem(row, 1, new QTableWidgetItem("0"));
    });

    connect(btnDelete, &QPushButton::clicked, [=]() {
        int currentRow = tableWidget->currentRow();
        if (currentRow >= 0) {
            tableWidget->removeRow(currentRow);
            // 更新序号
            for (int i = 0; i < tableWidget->rowCount(); ++i) {
                tableWidget->setItem(i, 0, new QTableWidgetItem(QString::number(i + 1)));
            }
        }
    });

    connect(btnSave, &QPushButton::clicked, [=]() {
        // 保存数据到原表格
        QTableWidget *sourceTable = ui->tableWidgetDelayPresetsGalvo;
        sourceTable->setRowCount(0);

        for (int row = 0; row < tableWidget->rowCount(); ++row) {
            sourceTable->insertRow(row);
            for (int col = 0; col < tableWidget->columnCount(); ++col) {
                QTableWidgetItem *item = tableWidget->item(row, col);
                if (item) {
                    QTableWidgetItem *newItem = new QTableWidgetItem(item->text());
                    newItem->setTextAlignment(Qt::AlignCenter);
                    sourceTable->setItem(row, col, newItem);
                }
            }
        }

        dialog->accept();
    });

    connect(btnCancel, &QPushButton::clicked, dialog, &QDialog::reject);

    // 显示弹窗
    dialog->exec();

    // 清理资源
    delete dialog;
}

void Integration::showPowerPresetEditDialogStage()
{
    // 已整合到波长调谐统一弹窗，直接转发
    showWavelengthTuningEditDialogStage();
}

// showDelayPresetEditDialog 同样转发到统一弹窗
void Integration::showDelayPresetEditDialog()
{
    showWavelengthTuningEditDialogStage();
}


// ========== 波长调谐统一表 - 新函数 ==========

QList<WavelengthTuningPreset> Integration::loadWavelengthTuningPresetsFromTableStage()
{
    QList<WavelengthTuningPreset> presets;
    if (!m_wavelengthTuningTable) return presets;

    for (int row = 0; row < m_wavelengthTuningTable->rowCount(); ++row) {
        WavelengthTuningPreset preset;
        auto getText = [&](int col) -> float {
            QTableWidgetItem *item = m_wavelengthTuningTable->item(row, col);
            return item ? item->text().toFloat() : 0.0f;
        };
        preset.wavelength      = getText(0);
        preset.stagePositions  = getText(1);
        preset.delayLine1      = getText(2);
        preset.delayLine2      = getText(3);
        preset.mainPumpPower   = getText(4);
        preset.seedPumpPower   = getText(5);
        preset.stokesPumpPower = getText(6);
        preset.preampPumpPower = getText(7);
        presets.append(preset);
    }
    return presets;
}

void Integration::showWavelengthTuningEditDialogStage()
{
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle("编辑波长调谐预设");
    dialog->resize(1100, 450);

    QVBoxLayout *mainLayout = new QVBoxLayout(dialog);

    // 创建编辑表格（8列，与主表一致）
    QTableWidget *tableWidget = new QTableWidget(dialog);
    tableWidget->setColumnCount(8);
    tableWidget->setHorizontalHeaderLabels({
        "波长（mm）", "两个旋转平台位置", "延迟线一", "延迟线二",
        "pump路主级泵功率（mW）", "种子源泵功率（mW）",
        "Stokes路泵功率（mW）", "pump路预放泵功率（mW）"
    });
    tableWidget->setEditTriggers(QAbstractItemView::AllEditTriggers);
    tableWidget->setAlternatingRowColors(true);
    tableWidget->horizontalHeader()->setStretchLastSection(false);
    for (int col = 0; col < 8; ++col) {
        tableWidget->setColumnWidth(col, 120);
    }

    // 从主表加载数据
    QTableWidget *sourceTable = ui->tableWidgetWavelengthTuningStage;
    for (int row = 0; row < sourceTable->rowCount(); ++row) {
        tableWidget->insertRow(row);
        for (int col = 0; col < sourceTable->columnCount(); ++col) {
            QTableWidgetItem *item = sourceTable->item(row, col);
            tableWidget->setItem(row, col, new QTableWidgetItem(item ? item->text() : "0"));
        }
    }

    // 按钮区
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *btnAdd    = new QPushButton("添加行", dialog);
    QPushButton *btnDelete = new QPushButton("删除行", dialog);
    QPushButton *btnImport = new QPushButton("读取excel文件", dialog);
    btnImport->setObjectName("btnImportExcelWavelengthTuning");
    QPushButton *btnSave   = new QPushButton("保存", dialog);
    QPushButton *btnCancel = new QPushButton("取消", dialog);

    buttonLayout->addWidget(btnAdd);
    buttonLayout->addWidget(btnDelete);
    buttonLayout->addWidget(btnImport);
    buttonLayout->addStretch();
    buttonLayout->addWidget(btnSave);
    buttonLayout->addWidget(btnCancel);

    mainLayout->addWidget(tableWidget);
    mainLayout->addLayout(buttonLayout);

    // 添加行
    connect(btnAdd, &QPushButton::clicked, [=]() {
        int row = tableWidget->rowCount();
        tableWidget->insertRow(row);
        for (int col = 0; col < 8; ++col) {
            QTableWidgetItem *item = new QTableWidgetItem("0");
            item->setTextAlignment(Qt::AlignCenter);
            tableWidget->setItem(row, col, item);
        }
    });

    // 删除行
    connect(btnDelete, &QPushButton::clicked, [=]() {
        int row = tableWidget->currentRow();
        if (row >= 0) tableWidget->removeRow(row);
    });

    // 读取excel文件（预留接口：可接入 QXlsx 或 CSV 解析）
    connect(btnImport, &QPushButton::clicked, [=]() {
        QString filePath = QFileDialog::getOpenFileName(
            dialog, "选择Excel/CSV文件", "",
            "Excel文件 (*.xlsx *.xls);;CSV文件 (*.csv);;所有文件 (*)"
        );
        if (filePath.isEmpty()) return;

        // TODO: 接入 Excel/CSV 解析库后在此实现导入逻辑
        // 当前仅支持 CSV 格式导入
        if (filePath.endsWith(".csv", Qt::CaseInsensitive)) {
            QFile file(filePath);
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QMessageBox::warning(dialog, "错误", "无法打开文件：" + filePath);
                return;
            }
            QTextStream in(&file);
            tableWidget->setRowCount(0);
            int row = 0;
            while (!in.atEnd()) {
                QString line = in.readLine().trimmed();
                if (line.isEmpty()) continue;
                QStringList cols = line.split(',');
                tableWidget->insertRow(row);
                for (int col = 0; col < qMin(cols.size(), 8); ++col) {
                    QTableWidgetItem *item = new QTableWidgetItem(cols[col].trimmed());
                    item->setTextAlignment(Qt::AlignCenter);
                    tableWidget->setItem(row, col, item);
                }
                ++row;
            }
            file.close();
            QMessageBox::information(dialog, "导入完成", QString("已导入 %1 行数据").arg(row));
        } else {
            QMessageBox::information(dialog, "提示", "暂不支持 .xlsx 格式，请将文件另存为 CSV 后导入。");
        }
    });

    // 保存到主表
    connect(btnSave, &QPushButton::clicked, [=]() {
        QTableWidget *target = ui->tableWidgetWavelengthTuningStage;
        target->setRowCount(0);
        for (int row = 0; row < tableWidget->rowCount(); ++row) {
            target->insertRow(row);
            for (int col = 0; col < 8; ++col) {
                QTableWidgetItem *item = tableWidget->item(row, col);
                QTableWidgetItem *newItem = new QTableWidgetItem(item ? item->text() : "0");
                newItem->setTextAlignment(Qt::AlignCenter);
                target->setItem(row, col, newItem);
            }
        }
        dialog->accept();
    });

    connect(btnCancel, &QPushButton::clicked, dialog, &QDialog::reject);

    dialog->exec();
    delete dialog;
}

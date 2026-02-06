#include "integration.h"
#include "ui_integration.h"
#include "Communication/serial_port_base.h"
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
    , m_connectionManagerWidget(nullptr)
    , m_seedLaserDriver(nullptr)
    , m_fopoLaserDriver(nullptr)
    , m_stokesLaserDriver(nullptr)
    , m_spectrometerFOPO(nullptr)
    , m_spectrometerStokes(nullptr)
    , m_stageController(nullptr)
    , m_galvoMirror(nullptr)
    , m_delayLine(nullptr)
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
    , m_powerPresetDelayTimerStage(nullptr)
    , m_currentPowerPresetIndexStage(0)
    , m_isPowerPresetExecutingStage(false)
    , m_powerPresetTimeIntervalStage(1)
{
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
    // 清理连接管理窗口
    if (m_connectionManagerWidget) {
        delete m_connectionManagerWidget;
        m_connectionManagerWidget = nullptr;
    }
    
    // 清理设备
    delete m_seedLaserDriver;
    delete m_fopoLaserDriver;
    delete m_stokesLaserDriver;
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
    // 创建激光器实例（三个独立的激光器）
    m_seedLaserDriver = new LaserDriver(LaserType::SeedSource, this);
    m_fopoLaserDriver = new LaserDriver(LaserType::FOPO, this);
    m_stokesLaserDriver = new LaserDriver(LaserType::Stokes, this);
    
    // 创建光谱仪实例（两个独立的光谱仪）
    m_spectrometerFOPO = new Spectrometer(this);
    m_spectrometerStokes = new Spectrometer(this);
    
    // 创建其他设备实例
    m_stageController = new StageController(this);
    m_galvoMirror = new GalvoMirror(this);  // 创建振镜控制卡实例
    m_delayLine = new DelayLine(this);
    
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
    
    // 初始化预设置控件的显示状态（默认隐藏）
    ui->groupBoxPowerPresetsNew->setVisible(false);       // 振镜页-光源功率预设置
    ui->groupBoxDelayPresetsGalvo->setVisible(false);     // 振镜页-延迟线预设置
    ui->groupBoxPowerPresetsStage->setVisible(false);     // 位移台页-电控平台与功率预设置
    ui->groupBoxDelayPresetsNew->setVisible(false);       // 位移台页-延迟线预设置
    
    // 初始化底部控制按钮的显示状态（默认隐藏）
    ui->widgetGalvoConfirmButton->setVisible(false);      // 振镜页-底部控制按钮
    ui->widgetStageConfirmButton->setVisible(false);      // 位移台页-底部控制按钮
    
    // 初始化所有状态指示器的大小和样式（统一为20x20的圆形指示灯）
    initStatusIndicators();
    
    // 设置所有输入框居中对齐
    // 振镜页输入框
    ui->lineEditGalvoAngle->setAlignment(Qt::AlignCenter);
    ui->lineEditTimeDelay->setAlignment(Qt::AlignCenter);
    ui->lineEditSeedPump->setAlignment(Qt::AlignCenter);
    ui->lineEditFOPOPump->setAlignment(Qt::AlignCenter);
    ui->lineEditStokesPump->setAlignment(Qt::AlignCenter);
    
    // 位移台页输入框
    ui->lineEditStageAngle->setAlignment(Qt::AlignCenter);
    ui->lineEditStagePosition->setAlignment(Qt::AlignCenter);
    ui->lineEditStageTimeDelay->setAlignment(Qt::AlignCenter);
    ui->lineEditStageSeedPump->setAlignment(Qt::AlignCenter);
    ui->lineEditStageFOPOPump->setAlignment(Qt::AlignCenter);
    ui->lineEditStageStokesPump->setAlignment(Qt::AlignCenter);
    
    // 更新连接状态
    updateConnectionStatus();
    
    // ========== 连接管理配置 ==========
    // 创建独立的连接管理窗口
    m_connectionManagerWidget = new QWidget(nullptr, Qt::Window);
    m_connectionManagerWidget->setWindowTitle("设备连接管理");
    m_connectionManagerWidget->resize(700, 800);
    m_connectionManagerWidget->setAttribute(Qt::WA_DeleteOnClose, false);  // 关闭时不删除
    
    // 获取连接页widget(索引0)
    QWidget *connectionTab = ui->tabWidget->widget(0);
    if (connectionTab) {
        // 将连接页从TabWidget中移除
        ui->tabWidget->removeTab(0);
        
        // 将连接页设置为独立窗口的内容
        QVBoxLayout *layout = new QVBoxLayout(m_connectionManagerWidget);
        layout->setContentsMargins(10, 10, 10, 10);
        connectionTab->setParent(m_connectionManagerWidget);
        connectionTab->show();  // 确保子widget可见
        layout->addWidget(connectionTab);
        m_connectionManagerWidget->setLayout(layout);
    }
    
    // 创建菜单栏
    QMenuBar *menuBar = new QMenuBar(this);
    setMenuBar(menuBar);
    
    // 创建连接管理菜单
    QMenu *connectionMenu = menuBar->addMenu("连接管理");
    QAction *openConnectionAction = connectionMenu->addAction("打开连接管理窗口");
    
    // 使用lambda直接连接信号槽
    connect(openConnectionAction, &QAction::triggered, this, [this]() {
        if (m_connectionManagerWidget) {
            if (m_connectionManagerWidget->isVisible()) {
                m_connectionManagerWidget->hide();
            } else {
                m_connectionManagerWidget->show();
                m_connectionManagerWidget->raise();
                m_connectionManagerWidget->activateWindow();
            }
        }
    });
}

void Integration::initConnections()
{
    // 连接种子源激光器信号
    connect(m_seedLaserDriver, &LaserDriver::statusChanged,
            this, &Integration::onSeedLaserStatusChanged);
    connect(m_seedLaserDriver, &LaserDriver::errorOccurred,
            this, &Integration::onDeviceError);
    
    // 连接FOPO激光器信号
    connect(m_fopoLaserDriver, &LaserDriver::statusChanged,
            this, &Integration::onFOPOLaserStatusChanged);
    connect(m_fopoLaserDriver, &LaserDriver::errorOccurred,
            this, &Integration::onDeviceError);
    
    // 连接Stokes激光器信号
    connect(m_stokesLaserDriver, &LaserDriver::statusChanged,
            this, &Integration::onStokesLaserStatusChanged);
    connect(m_stokesLaserDriver, &LaserDriver::errorOccurred,
            this, &Integration::onDeviceError);
    
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
    
    // 连接位移台信号
    connect(m_stageController, &StageController::statusChanged,
            this, &Integration::onStageStatusChanged);
    connect(m_stageController, &StageController::errorOccurred,
            this, &Integration::onDeviceError);
    connect(m_stageController, &StageController::positionChanged,
            this, &Integration::onStagePositionChanged);
    connect(m_stageController, &StageController::moveCompleted,
            this, &Integration::onStageMoveCompleted);
    
    // 连接振镜控制卡信号
    connect(m_galvoMirror, &GalvoMirror::statusChanged,
            this, &Integration::onGalvoStatusChanged);
    connect(m_galvoMirror, &GalvoMirror::errorOccurred,
            this, &Integration::onDeviceError);
    
    // 连接延时线信号
    connect(m_delayLine, &DelayLine::statusChanged,
            this, &Integration::onDelayStatusChanged);
    connect(m_delayLine, &DelayLine::errorOccurred,
            this, &Integration::onDeviceError);
    connect(m_delayLine, &DelayLine::delayChanged,
            this, &Integration::onDelayChanged);
    
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
    // 检测FOPO路图表视图的双击事件
    if (obj == m_chartViewFOPO && event->type() == QEvent::MouseButtonDblClick) {
        showChartMaximized();
        return true;
    }
    
    // 检测Stokes路图表视图的双击事件
    if (obj == m_chartViewStokes && event->type() == QEvent::MouseButtonDblClick) {
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
    populateSerialPortCombo(ui->comboBoxSeedLaserPort);
    populateSerialPortCombo(ui->comboBoxFOPOLaserPort);
    populateSerialPortCombo(ui->comboBoxStokesLaserPort);
    populateSerialPortCombo(ui->comboBoxSpectrometerFOPOPort);
    populateSerialPortCombo(ui->comboBoxSpectrometerStokesPort);
    populateSerialPortCombo(ui->comboBoxStagePort);
    populateSerialPortCombo(ui->comboBoxDelayPort);
    
    // 初始化波特率下拉框
    populateBaudRateCombo(ui->comboBoxSeedLaserBaudRate);
    populateBaudRateCombo(ui->comboBoxFOPOLaserBaudRate);
    populateBaudRateCombo(ui->comboBoxStokesLaserBaudRate);
    populateBaudRateCombo(ui->comboBoxSpectrometerFOPOBaudRate);
    populateBaudRateCombo(ui->comboBoxSpectrometerStokesBaudRate);  // 添加 Stokes 光谱仪波特率
    populateBaudRateCombo(ui->comboBoxStageBaudRate);
    populateBaudRateCombo(ui->comboBoxDelayBaudRate);
    
    // 设置默认波特率
    ui->comboBoxSeedLaserBaudRate->setCurrentText("9600");
    ui->comboBoxFOPOLaserBaudRate->setCurrentText("9600");
    ui->comboBoxStokesLaserBaudRate->setCurrentText("9600");
    ui->comboBoxSpectrometerFOPOBaudRate->setCurrentText("115200");
    ui->comboBoxSpectrometerStokesBaudRate->setCurrentText("115200");  // 设置 Stokes 光谱仪默认波特率
    ui->comboBoxStageBaudRate->setCurrentText("9600");
    ui->comboBoxDelayBaudRate->setCurrentText("9600");
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
    
    updateStatusIndicator(ui->labelSeedLaserStatus, DeviceStatus::Disconnected);
    updateStatusIndicator(ui->labelFOPOLaserStatus, DeviceStatus::Disconnected);
    updateStatusIndicator(ui->labelStokesLaserStatus, DeviceStatus::Disconnected);
    updateStatusIndicator(ui->labelSpectrometerFOPOStatus, DeviceStatus::Disconnected);
    updateStatusIndicator(ui->labelSpectrometerStokesStatus, DeviceStatus::Disconnected);
    updateStatusIndicator(ui->labelStageStatus, DeviceStatus::Disconnected);
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


// ========== 激光器连接/断开槽函数 ==========

void Integration::on_btnConnectSeedLaser_clicked()
{
    connectLaser(LaserDeviceType::Seed);
}

void Integration::on_btnDisconnectSeedLaser_clicked()
{
    disconnectLaser(LaserDeviceType::Seed);
}

void Integration::on_btnConnectFOPOLaser_clicked()
{
    connectLaser(LaserDeviceType::FOPO);
}

void Integration::on_btnDisconnectFOPOLaser_clicked()
{
    disconnectLaser(LaserDeviceType::FOPO);
}

void Integration::on_btnConnectStokesLaser_clicked()
{
    connectLaser(LaserDeviceType::Stokes);
}

void Integration::on_btnDisconnectStokesLaser_clicked()
{
    disconnectLaser(LaserDeviceType::Stokes);
}

void Integration::connectLaser(LaserDeviceType type)
{
    LaserDriver *driver = nullptr;
    SerialConfig config;
    QString deviceName;
    QLabel *statusIndicator = nullptr;
    
    switch (type) {
        case LaserDeviceType::Seed:
            driver = m_seedLaserDriver;
            config = getSeedLaserSerialConfig();
            deviceName = "种子源激光器";
            statusIndicator = ui->labelSeedLaserStatus;
            break;
        case LaserDeviceType::FOPO:
            driver = m_fopoLaserDriver;
            config = getFOPOLaserSerialConfig();
            deviceName = "FOPO激光器";
            statusIndicator = ui->labelFOPOLaserStatus;
            break;
        case LaserDeviceType::Stokes:
            driver = m_stokesLaserDriver;
            config = getStokesLaserSerialConfig();
            deviceName = "Stokes激光器";
            statusIndicator = ui->labelStokesLaserStatus;
            break;
    }
    
    if (!driver) return;
    
    // 检查是否选择了串口
    if (config.portName.isEmpty()) {
        QMessageBox::warning(this, "连接失败", 
            deviceName + "：请先选择串口设备！\n\n请在串口下拉框中选择一个可用的串口。");
        return;
    }
    
    // 显示连接中状态
    if (statusIndicator) {
        updateStatusIndicator(statusIndicator, DeviceStatus::Connecting);
    }
    updateStatusBar(deviceName + "正在连接...");
    
    // 处理 UI 事件，确保状态更新显示
    QCoreApplication::processEvents();
    
    // 使用 QTimer 异步执行连接操作，避免阻塞主线程
    QTimer::singleShot(50, this, [this, driver, config, deviceName, statusIndicator]() {
        // 设置串口参数
        bool success = driver->openPort(
            config.portName,
            config.baudRate,
            config.dataBits,
            config.parity,
            config.stopBits
        );
        
        // 处理 UI 事件
        QCoreApplication::processEvents();
        
        if (success && driver->connect()) {
            updateStatusBar(deviceName + "连接成功");
            qDebug() << deviceName << "连接成功";
            if (statusIndicator) {
                updateStatusIndicator(statusIndicator, DeviceStatus::Connected);
            }
        } else {
            updateStatusBar(deviceName + "连接失败");
            qDebug() << deviceName << "连接失败：" << driver->getLastError();
            if (statusIndicator) {
                updateStatusIndicator(statusIndicator, DeviceStatus::Error);
            }
            QMessageBox::warning(this, "连接失败", deviceName + "连接失败：" + driver->getLastError());
        }
    });
}

void Integration::disconnectLaser(LaserDeviceType type)
{
    LaserDriver *driver = nullptr;
    QString deviceName;
    QLabel *statusIndicator = nullptr;
    
    switch (type) {
        case LaserDeviceType::Seed:
            driver = m_seedLaserDriver;
            deviceName = "种子源激光器";
            statusIndicator = ui->labelSeedLaserStatus;
            break;
        case LaserDeviceType::FOPO:
            driver = m_fopoLaserDriver;
            deviceName = "FOPO激光器";
            statusIndicator = ui->labelFOPOLaserStatus;
            break;
        case LaserDeviceType::Stokes:
            driver = m_stokesLaserDriver;
            deviceName = "Stokes激光器";
            statusIndicator = ui->labelStokesLaserStatus;
            break;
    }
    
    if (!driver) return;
    
    driver->disconnect();
    if (statusIndicator) {
        updateStatusIndicator(statusIndicator, DeviceStatus::Disconnected);
    }
    updateStatusBar(deviceName + "已断开");
    qDebug() << deviceName << "已断开";
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

// ========== 位移台连接/断开槽函数 ==========

void Integration::on_btnConnectStage_clicked()
{
    SerialConfig config = getStageSerialConfig();
    
    // 检查是否选择了串口
    if (config.portName.isEmpty()) {
        QMessageBox::warning(this, "连接失败", 
            "位移台：请先选择串口设备！\n\n请在串口下拉框中选择一个可用的串口。");
        return;
    }
    
    // 显示连接中状态
    updateStatusIndicator(ui->labelStageStatus, DeviceStatus::Connecting);
    updateStatusBar("位移台正在连接...");
    
    // 处理 UI 事件
    QCoreApplication::processEvents();
    
    // 使用 QTimer 异步执行连接操作
    QTimer::singleShot(50, this, [this, config]() {
        bool success = m_stageController->openPort(
            config.portName,
            config.baudRate,
            config.dataBits,
            config.parity,
            config.stopBits
        );
        
        // 处理 UI 事件
        QCoreApplication::processEvents();
        
        if (success && m_stageController->connect()) {
            updateStatusBar("位移台连接成功");
            updateStatusIndicator(ui->labelStageStatus, DeviceStatus::Connected);
            qDebug() << "位移台连接成功";
        } else {
            updateStatusBar("位移台连接失败");
            updateStatusIndicator(ui->labelStageStatus, DeviceStatus::Error);
            qDebug() << "位移台连接失败：" << m_stageController->getLastError();
            QMessageBox::warning(this, "连接失败", "位移台连接失败：" + m_stageController->getLastError());
        }
    });
}

void Integration::on_btnDisconnectStage_clicked()
{
    m_stageController->disconnect();
    updateStatusIndicator(ui->labelStageStatus, DeviceStatus::Disconnected);
    updateStatusBar("位移台已断开");
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
        // 初始化控制卡
        if (!m_galvoMirror->initialize()) {
            updateStatusBar("振镜控制卡初始化失败");
            updateStatusIndicator(ui->labelGalvoStatus, DeviceStatus::Error);
            QMessageBox::warning(this, "连接失败", "振镜控制卡初始化失败");
            return;
        }
        
        // 通过IP地址连接
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
        
        if (success && m_delayLine->connect()) {
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
    updateStatusBar("延时线已断开");
}


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

void Integration::on_btnConfirmStageAngle_clicked()
{
    bool ok;
    float angle = ui->lineEditStageAngle->text().toFloat(&ok);
    
    if (!ok) {
        QMessageBox::warning(this, "错误", "请输入有效的角度值");
        return;
    }
    
    if (!m_stageController->isConnected()) {
        QMessageBox::warning(this, "错误", "位移台未连接");
        return;
    }
    
    if (m_stageController->moveAbsolute(angle, true)) {
        updateStatusBar("旋转台角度设置成功: " + QString::number(angle) + "度");
        qDebug() << "旋转台角度设置成功:" << angle << "度";
    } else {
        QMessageBox::warning(this, "错误", "旋转台角度设置失败：" + m_stageController->getLastError());
    }
}

void Integration::on_btnConfirmStagePosition_clicked()
{
    bool ok;
    float position = ui->lineEditStagePosition->text().toFloat(&ok);
    
    if (!ok) {
        QMessageBox::warning(this, "错误", "请输入有效的位置值");
        return;
    }
    
    if (!m_stageController->isConnected()) {
        QMessageBox::warning(this, "错误", "位移台未连接");
        return;
    }
    
    if (m_stageController->moveAbsolute(position, false)) {
        updateStatusBar("直线台位置设置成功: " + QString::number(position) + " mm");
        qDebug() << "直线台位置设置成功:" << position << "mm";
    } else {
        QMessageBox::warning(this, "错误", "直线台位置设置失败：" + m_stageController->getLastError());
    }
}

void Integration::on_btnConfirmStageTimeDelay_clicked()
{
    setDelayTime(ui->lineEditStageTimeDelay, ui->lineEditTimeDelay);
}

void Integration::onStagePositionChanged(qint32 positionPulses)
{
    qDebug() << "位移台位置改变:" << positionPulses;
}

void Integration::onStageMoveCompleted()
{
    updateStatusBar("位移台移动完成");
    qDebug() << "位移台移动完成";
}

// ========== 延时线控制槽函数 ==========

void Integration::on_btnConfirmTimeDelay_clicked()
{
    setDelayTime(ui->lineEditTimeDelay, ui->lineEditStageTimeDelay);
}

void Integration::onDelayChanged(float delayPS)
{
    qDebug() << "延时线延迟改变:" << delayPS << "PS";
}

// ========== 振镜角度控制槽函数 ==========

void Integration::on_btnConfirmGalvoAngle_clicked()
{
    // 检查振镜是否连接
    if (!m_galvoMirror->isConnected()) {
        QMessageBox::warning(this, "错误", "振镜控制卡未连接！\n请先连接振镜控制卡。");
        return;
    }
    
    // 获取振镜角度输入
    bool ok;
    float angle = ui->lineEditGalvoAngle->text().toFloat(&ok);
    
    if (!ok) {
        QMessageBox::warning(this, "输入错误", "请输入有效的振镜角度值！");
        return;
    }
    
    // 设置振镜角度（通过振镜跳转功能实现）
    // 假设振镜角度对应XY坐标的转换关系
    // 这里需要根据实际的振镜-角度映射关系进行转换
    // 暂时使用简单的线性映射：angle -> (x, y, 0)
    float x = angle * 10.0f;  // 示例映射，需要根据实际情况调整
    float y = 0.0f;
    float z = 0.0f;
    
    if (m_galvoMirror->scannerJump(x, y, z)) {
        updateStatusBar(QString("振镜角度已设置为: %1°").arg(angle));
        qDebug() << "振镜角度设置成功:" << angle << "度";
    } else {
        QMessageBox::warning(this, "设置失败", "振镜角度设置失败！");
        qDebug() << "振镜角度设置失败";
    }
}

// ========== 泵功率设置槽函数 - 振镜页 ==========

void Integration::on_btnConfirmSeedPump_clicked()
{
    setPumpCurrent(LaserDeviceType::Seed, ui->lineEditSeedPump, ui->lineEditStageSeedPump);
}

void Integration::on_btnConfirmFOPOPump_clicked()
{
    setPumpCurrent(LaserDeviceType::FOPO, ui->lineEditFOPOPump, ui->lineEditStageFOPOPump);
}

void Integration::on_btnConfirmStokesPump_clicked()
{
    setPumpCurrent(LaserDeviceType::Stokes, ui->lineEditStokesPump, ui->lineEditStageStokesPump);
}

// ========== 泵功率设置槽函数 - 位移台页 ==========

void Integration::on_btnConfirmStageSeedPump_clicked()
{
    setPumpCurrent(LaserDeviceType::Seed, ui->lineEditStageSeedPump, ui->lineEditSeedPump);
}

void Integration::on_btnConfirmStageFOPOPump_clicked()
{
    setPumpCurrent(LaserDeviceType::FOPO, ui->lineEditStageFOPOPump, ui->lineEditFOPOPump);
}

void Integration::on_btnConfirmStageStokesPump_clicked()
{
    setPumpCurrent(LaserDeviceType::Stokes, ui->lineEditStageStokesPump, ui->lineEditStokesPump);
}

void Integration::setPumpCurrent(LaserDeviceType type, QLineEdit *inputField, QLineEdit *syncField)
{
    if (!inputField) return;
    
    bool ok;
    float current = inputField->text().toFloat(&ok);
    
    if (!ok) {
        QMessageBox::warning(this, "错误", "请输入有效的电流值");
        return;
    }
    
    LaserDriver *driver = nullptr;
    QString deviceName;
    
    switch (type) {
        case LaserDeviceType::Seed:
            driver = m_seedLaserDriver;
            deviceName = "种子源激光器";
            break;
        case LaserDeviceType::FOPO:
            driver = m_fopoLaserDriver;
            deviceName = "FOPO激光器";
            break;
        case LaserDeviceType::Stokes:
            driver = m_stokesLaserDriver;
            deviceName = "Stokes激光器";
            break;
    }
    
    if (!driver) return;
    
    if (!driver->isConnected()) {
        QMessageBox::warning(this, "错误", deviceName + "未连接");
        return;
    }
    
    if (driver->setCurrent(current)) {
        updateStatusBar(deviceName + "泵功率设置成功: " + QString::number(current));
        qDebug() << deviceName << "泵功率设置成功:" << current;
        
        // 同步到另一个输入框
        if (syncField) {
            syncField->setText(inputField->text());
        }
    } else {
        QMessageBox::warning(this, "错误", deviceName + "泵功率设置失败：" + driver->getLastError());
    }
}

void Integration::setDelayTime(QLineEdit *inputField, QLineEdit *syncField)
{
    if (!inputField) return;
    
    bool ok;
    float delayPS = inputField->text().toFloat(&ok);
    
    if (!ok) {
        QMessageBox::warning(this, "错误", "请输入有效的延迟值");
        return;
    }
    
    if (!m_delayLine->isConnected()) {
        QMessageBox::warning(this, "错误", "延时线未连接");
        return;
    }
    
    if (m_delayLine->setDelay(delayPS)) {
        updateStatusBar("延时线延迟设置成功: " + QString::number(delayPS) + " PS");
        qDebug() << "延时线延迟设置成功:" << delayPS << "PS";
        
        // 同步到另一个输入框
        if (syncField) {
            syncField->setText(inputField->text());
        }
    } else {
        QMessageBox::warning(this, "错误", "延时线延迟设置失败：" + m_delayLine->getLastError());
    }
}

// ========== 设备状态更新槽函数 ==========

void Integration::onSeedLaserStatusChanged(DeviceStatus status)
{
    updateStatusIndicator(ui->labelSeedLaserStatus, status);
    qDebug() << "种子源激光器状态改变:" << static_cast<int>(status);
}

void Integration::onFOPOLaserStatusChanged(DeviceStatus status)
{
    updateStatusIndicator(ui->labelFOPOLaserStatus, status);
    qDebug() << "FOPO激光器状态改变:" << static_cast<int>(status);
}

void Integration::onStokesLaserStatusChanged(DeviceStatus status)
{
    updateStatusIndicator(ui->labelStokesLaserStatus, status);
    qDebug() << "Stokes激光器状态改变:" << static_cast<int>(status);
}

void Integration::onSpectrometerFOPOStatusChanged(DeviceStatus status)
{
    updateStatusIndicator(ui->labelSpectrometerFOPOStatus, status);
    qDebug() << "光谱仪状态改变:" << static_cast<int>(status);
}

void Integration::onStageStatusChanged(DeviceStatus status)
{
    updateStatusIndicator(ui->labelStageStatus, status);
    qDebug() << "位移台状态改变:" << static_cast<int>(status);
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

SerialConfig Integration::getSeedLaserSerialConfig()
{
    SerialConfig config;
    config.portName = ui->comboBoxSeedLaserPort->currentText();
    config.baudRate = ui->comboBoxSeedLaserBaudRate->currentData().toInt();
    config.dataBits = QSerialPort::Data8;
    config.stopBits = QSerialPort::OneStop;
    config.parity = QSerialPort::NoParity;
    return config;
}

SerialConfig Integration::getFOPOLaserSerialConfig()
{
    SerialConfig config;
    config.portName = ui->comboBoxFOPOLaserPort->currentText();
    config.baudRate = ui->comboBoxFOPOLaserBaudRate->currentData().toInt();
    config.dataBits = QSerialPort::Data8;
    config.stopBits = QSerialPort::OneStop;
    config.parity = QSerialPort::NoParity;
    return config;
}

SerialConfig Integration::getStokesLaserSerialConfig()
{
    SerialConfig config;
    config.portName = ui->comboBoxStokesLaserPort->currentText();
    config.baudRate = ui->comboBoxStokesLaserBaudRate->currentData().toInt();
    config.dataBits = QSerialPort::Data8;
    config.stopBits = QSerialPort::OneStop;
    config.parity = QSerialPort::NoParity;
    return config;
}

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

SerialConfig Integration::getStageSerialConfig()
{
    SerialConfig config;
    config.portName = ui->comboBoxStagePort->currentText();
    config.baudRate = ui->comboBoxStageBaudRate->currentData().toInt();
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
    m_powerPresetTable->setColumnCount(5);  // 删除序号列,只保留5列数据
    m_powerPresetTable->setHorizontalHeaderLabels({"振镜起始(deg)", "振镜结束(deg)", 
                                                    "种子源泵(mA)", "FOPO泵(A)", "Stokes泵(mA)"});
    m_powerPresetTable->horizontalHeader()->setStretchLastSection(false);
    m_powerPresetTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_powerPresetTable->setSelectionMode(QAbstractItemView::SingleSelection);
    
    // 设置表头字体大小
    QFont headerFont = m_powerPresetTable->horizontalHeader()->font();
    headerFont.setPointSize(9);
    m_powerPresetTable->horizontalHeader()->setFont(headerFont);
    
    // 设置列宽
    m_powerPresetTable->setColumnWidth(0, 110);
    m_powerPresetTable->setColumnWidth(1, 110);
    m_powerPresetTable->setColumnWidth(2, 110);
    m_powerPresetTable->setColumnWidth(3, 90);
    m_powerPresetTable->setColumnWidth(4, 110);
    
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
    m_delayPresetTableGalvo->setColumnWidth(0, 150);
    m_delayPresetTableGalvo->setColumnWidth(1, 150);
    
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
    
    // 初始化位移台页 - 电控与功率预设表格
    m_powerPresetTableStage = ui->tableWidgetPowerPresetsStage;
    m_powerPresetTableStage->setColumnCount(5);  // 删除序号列,只保留5列数据
    m_powerPresetTableStage->setHorizontalHeaderLabels({"旋转台角度(deg)", "位移台位置(mm)", 
                                                         "种子源泵(mA)", "FOPO泵(A)", "Stokes泵(mA)"});
    m_powerPresetTableStage->horizontalHeader()->setStretchLastSection(false);
    m_powerPresetTableStage->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_powerPresetTableStage->setSelectionMode(QAbstractItemView::SingleSelection);
    
    // 设置表头字体大小
    headerFont = m_powerPresetTableStage->horizontalHeader()->font();
    headerFont.setPointSize(9);
    m_powerPresetTableStage->horizontalHeader()->setFont(headerFont);
    
    // 设置列宽
    m_powerPresetTableStage->setColumnWidth(0, 120);
    m_powerPresetTableStage->setColumnWidth(1, 120);
    m_powerPresetTableStage->setColumnWidth(2, 110);
    m_powerPresetTableStage->setColumnWidth(3, 90);
    m_powerPresetTableStage->setColumnWidth(4, 110);
    
    // 添加默认6行数据
    const QStringList stagePowerData[6] = {
        {"0", "3", "100", "7", "400"},
        {"3", "6", "110", "7", "450"},
        {"6", "9", "90", "7", "400"},
        {"9", "12", "105", "7", "350"},
        {"12", "15", "110", "9", "300"},
        {"15", "18", "100", "10", "370"}
    };
    
    for (int row = 0; row < 6; ++row) {
        m_powerPresetTableStage->insertRow(row);
        for (int col = 0; col < 5; ++col) {
            QTableWidgetItem *item = new QTableWidgetItem(stagePowerData[row][col]);
            item->setTextAlignment(Qt::AlignCenter);
            m_powerPresetTableStage->setItem(row, col, item);
        }
    }
    
    // 初始化位移台页 - 延迟线预设表格
    m_delayPresetTable = ui->tableWidgetDelayPresets;
    m_delayPresetTable->setColumnCount(2);  // 删除序号列,只保留2列数据
    m_delayPresetTable->setHorizontalHeaderLabels({"旋转台角度(deg)", "延迟时间(PS)"});
    m_delayPresetTable->horizontalHeader()->setStretchLastSection(false);
    m_delayPresetTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_delayPresetTable->setSelectionMode(QAbstractItemView::SingleSelection);
    
    // 设置表头字体大小
    headerFont = m_delayPresetTable->horizontalHeader()->font();
    headerFont.setPointSize(9);
    m_delayPresetTable->horizontalHeader()->setFont(headerFont);
    
    // 设置列宽
    m_delayPresetTable->setColumnWidth(0, 150);
    m_delayPresetTable->setColumnWidth(1, 150);
    
    // 添加默认6行数据(与振镜页延迟线相同)
    for (int row = 0; row < 6; ++row) {
        m_delayPresetTable->insertRow(row);
        for (int col = 0; col < 2; ++col) {
            QTableWidgetItem *item = new QTableWidgetItem(galvoDelayData[row][col]);
            item->setTextAlignment(Qt::AlignCenter);
            m_delayPresetTable->setItem(row, col, item);
        }
    }
    
    // 设置所有表格的最小和最大高度
    m_powerPresetTable->setMinimumHeight(100);
    m_powerPresetTable->setMaximumHeight(250);
    m_delayPresetTableGalvo->setMinimumHeight(100);
    m_delayPresetTableGalvo->setMaximumHeight(250);
    m_powerPresetTableStage->setMinimumHeight(100);
    m_powerPresetTableStage->setMaximumHeight(250);
    m_delayPresetTable->setMinimumHeight(100);
    m_delayPresetTable->setMaximumHeight(250);
    
    qDebug() << "预设表格初始化完成";
}

// ========== 预设管理槽函数 - 振镜页（光源功率预设） ==========

void Integration::on_btnLightPowerPreset_clicked()
{
    // 切换光源功率预设置的显示/隐藏状态
    bool isVisible = ui->groupBoxPowerPresetsNew->isVisible();
    ui->groupBoxPowerPresetsNew->setVisible(!isVisible);
    
    // 更新底部控制按钮的显示状态
    updateGalvoConfirmButtonVisibility();
    
    // 更新按钮文字提示
    if (!isVisible) {
        qDebug() << "显示光源功率预设置";
    } else {
        qDebug() << "隐藏光源功率预设置";
    }
}

void Integration::on_btnAddPowerPresetRow_clicked()
{
    addPowerPresetRow();
}

void Integration::on_btnClearPowerPresets_clicked()
{
    m_powerPresetTable->setRowCount(0);
    qDebug() << "清空功率预设";
}

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
    if (needsSeed && !m_seedLaserDriver->isConnected()) {
        missingDevices << "种子源激光器";
    }
    if (needsFOPO && !m_fopoLaserDriver->isConnected()) {
        missingDevices << "FOPO激光器";
    }
    if (needsStokes && !m_stokesLaserDriver->isConnected()) {
        missingDevices << "Stokes激光器";
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

void Integration::on_btnMoveUpPowerPreset_clicked()
{
    int currentRow = m_powerPresetTable->currentRow();
    if (currentRow < 0) {
        QMessageBox::warning(this, "提示", "请先选择要操作的行");
        return;
    }
    moveRowUp(m_powerPresetTable, currentRow);
}

void Integration::on_btnMoveDownPowerPreset_clicked()
{
    int currentRow = m_powerPresetTable->currentRow();
    if (currentRow < 0) {
        QMessageBox::warning(this, "提示", "请先选择要操作的行");
        return;
    }
    moveRowDown(m_powerPresetTable, currentRow);
}

void Integration::on_btnDeletePowerPreset_clicked()
{
    int currentRow = m_powerPresetTable->currentRow();
    if (currentRow < 0) {
        QMessageBox::warning(this, "提示", "请先选择要操作的行");
        return;
    }
    deleteRow(m_powerPresetTable, currentRow);
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
        if (preset.seedPumpCurrent != 0 && !m_seedLaserDriver->isConnected()) {
            disconnectedDevices << "种子源激光器";
            hasDisconnectedDevice = true;
        }
        if (preset.fopoPumpCurrent != 0 && !m_fopoLaserDriver->isConnected()) {
            disconnectedDevices << "FOPO激光器";
            hasDisconnectedDevice = true;
        }
        if (preset.stokesPumpCurrent != 0 && !m_stokesLaserDriver->isConnected()) {
            disconnectedDevices << "Stokes激光器";
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

void Integration::addPowerPresetRow()
{
    int row = m_powerPresetTable->rowCount();
    m_powerPresetTable->insertRow(row);
    
    // 添加5列数据(删除了序号列)
    for (int col = 0; col < 5; col++) {
        QTableWidgetItem *item = new QTableWidgetItem("");
        item->setTextAlignment(Qt::AlignCenter);
        m_powerPresetTable->setItem(row, col, item);
    }
    
    qDebug() << "添加功率预设行，当前行数:" << m_powerPresetTable->rowCount();
}

void Integration::removePowerPresetRow(int row)
{
    // TODO: 删除功率预设表格的指定行
    qDebug() << "删除功率预设行:" << row;
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
        
        // 读取FOPO泵电流（第3列）
        QTableWidgetItem *itemFOPO = m_powerPresetTable->item(row, 3);
        if (itemFOPO) {
            preset.fopoPumpCurrent = itemFOPO->text().toFloat();
        }
        
        // 读取Stokes泵电流（第4列）
        QTableWidgetItem *itemStokes = m_powerPresetTable->item(row, 4);
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
            // 将角度转换为振镜坐标
            float x = galvoAngle * 10.0f;  // 示例映射，需要根据实际情况调整
            float y = 0.0f;
            float z = 0.0f;
            
            if (m_galvoMirror->scannerJump(x, y, z)) {
                successList << QString("振镜角度: %.1f度").arg(galvoAngle);
            } else {
                failedList << QString("振镜角度: %.1f度").arg(galvoAngle);
            }
        } else {
            skippedList << QString("振镜角度: %.1f度（设备未连接）").arg(galvoAngle);
        }
    }
    
    // 2. 设置种子源泵功率
    if (preset.seedPumpCurrent != 0) {
        if (m_seedLaserDriver->isConnected()) {
            if (m_seedLaserDriver->setCurrent(preset.seedPumpCurrent)) {
                successList << QString("种子源泵: %.1f mA").arg(preset.seedPumpCurrent);
            } else {
                failedList << QString("种子源泵: %.1f mA").arg(preset.seedPumpCurrent);
            }
        } else {
            skippedList << QString("种子源泵: %.1f mA（设备未连接）").arg(preset.seedPumpCurrent);
        }
    }
    
    // 3. 设置FOPO泵功率
    if (preset.fopoPumpCurrent != 0) {
        if (m_fopoLaserDriver->isConnected()) {
            if (m_fopoLaserDriver->setCurrent(preset.fopoPumpCurrent)) {
                successList << QString("FOPO泵: %.1f A").arg(preset.fopoPumpCurrent);
            } else {
                failedList << QString("FOPO泵: %.1f A").arg(preset.fopoPumpCurrent);
            }
        } else {
            skippedList << QString("FOPO泵: %.1f A（设备未连接）").arg(preset.fopoPumpCurrent);
        }
    }
    
    // 4. 设置Stokes泵功率
    if (preset.stokesPumpCurrent != 0) {
        if (m_stokesLaserDriver->isConnected()) {
            if (m_stokesLaserDriver->setCurrent(preset.stokesPumpCurrent)) {
                successList << QString("Stokes泵: %.1f mA").arg(preset.stokesPumpCurrent);
            } else {
                failedList << QString("Stokes泵: %.1f mA").arg(preset.stokesPumpCurrent);
            }
        } else {
            skippedList << QString("Stokes泵: %.1f mA（设备未连接）").arg(preset.stokesPumpCurrent);
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

void Integration::on_btnDelayLinePreset_clicked()
{
    // 切换延迟线预设置的显示/隐藏状态
    bool isVisible = ui->groupBoxDelayPresetsGalvo->isVisible();
    ui->groupBoxDelayPresetsGalvo->setVisible(!isVisible);
    
    // 更新底部控制按钮的显示状态
    updateGalvoConfirmButtonVisibility();
    
    // 更新按钮文字提示
    if (!isVisible) {
        qDebug() << "显示延迟线预设置（振镜页）";
    } else {
        qDebug() << "隐藏延迟线预设置（振镜页）";
    }
}

void Integration::on_btnAddDelayPresetRowGalvo_clicked()
{
    addDelayPresetRow(PresetPageType::GalvoPage);
}

void Integration::on_btnClearDelayPresetsGalvo_clicked()
{
    m_delayPresetTableGalvo->setRowCount(0);
    qDebug() << "清空延迟预设（振镜页）";
}

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

void Integration::on_btnMoveUpDelayPresetGalvo_clicked()
{
    int currentRow = m_delayPresetTableGalvo->currentRow();
    if (currentRow < 0) {
        QMessageBox::warning(this, "提示", "请先选择要操作的行");
        return;
    }
    moveRowUp(m_delayPresetTableGalvo, currentRow);
}

void Integration::on_btnMoveDownDelayPresetGalvo_clicked()
{
    int currentRow = m_delayPresetTableGalvo->currentRow();
    if (currentRow < 0) {
        QMessageBox::warning(this, "提示", "请先选择要操作的行");
        return;
    }
    moveRowDown(m_delayPresetTableGalvo, currentRow);
}

void Integration::on_btnDeleteDelayPresetGalvo_clicked()
{
    int currentRow = m_delayPresetTableGalvo->currentRow();
    if (currentRow < 0) {
        QMessageBox::warning(this, "提示", "请先选择要操作的行");
        return;
    }
    deleteRow(m_delayPresetTableGalvo, currentRow);
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

void Integration::on_btnStagePowerPreset_clicked()
{
    // 切换电控平台与功率预设置的显示/隐藏状态
    bool isVisible = ui->groupBoxPowerPresetsStage->isVisible();
    ui->groupBoxPowerPresetsStage->setVisible(!isVisible);
    
    // 更新底部控制按钮的显示状态
    updateStageConfirmButtonVisibility();
    
    // 更新按钮文字提示
    if (!isVisible) {
        qDebug() << "显示电控平台与功率预设置";
    } else {
        qDebug() << "隐藏电控平台与功率预设置";
    }
}

void Integration::on_btnAddPowerPresetRowStage_clicked()
{
    addPowerPresetRowStage();
}

void Integration::on_btnClearPowerPresetsStage_clicked()
{
    m_powerPresetTableStage->setRowCount(0);
    qDebug() << "清空功率预设（位移台页）";
}

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
        if (preset.stokesPumpCurrent != 0) {
            needsStokes = true;
        }
    }
    
    QStringList missingDevices;
    if (needsStage && !m_stageController->isConnected()) {
        missingDevices << "位移台";
    }
    if (needsSeed && !m_seedLaserDriver->isConnected()) {
        missingDevices << "种子源激光器";
    }
    if (needsFOPO && !m_fopoLaserDriver->isConnected()) {
        missingDevices << "FOPO激光器";
    }
    if (needsStokes && !m_stokesLaserDriver->isConnected()) {
        missingDevices << "Stokes激光器";
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

void Integration::on_btnMoveUpPowerPresetStage_clicked()
{
    int currentRow = m_powerPresetTableStage->currentRow();
    if (currentRow < 0) {
        QMessageBox::warning(this, "提示", "请先选择要操作的行");
        return;
    }
    moveRowUp(m_powerPresetTableStage, currentRow);
}

void Integration::on_btnMoveDownPowerPresetStage_clicked()
{
    int currentRow = m_powerPresetTableStage->currentRow();
    if (currentRow < 0) {
        QMessageBox::warning(this, "提示", "请先选择要操作的行");
        return;
    }
    moveRowDown(m_powerPresetTableStage, currentRow);
}

void Integration::on_btnDeletePowerPresetStage_clicked()
{
    int currentRow = m_powerPresetTableStage->currentRow();
    if (currentRow < 0) {
        QMessageBox::warning(this, "提示", "请先选择要操作的行");
        return;
    }
    deleteRow(m_powerPresetTableStage, currentRow);
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
        if (preset.seedPumpCurrent != 0 && !m_seedLaserDriver->isConnected()) {
            disconnectedDevices << "种子源激光器";
            hasDisconnectedDevice = true;
        }
        if (preset.fopoPumpCurrent != 0 && !m_fopoLaserDriver->isConnected()) {
            disconnectedDevices << "FOPO激光器";
            hasDisconnectedDevice = true;
        }
        if (preset.stokesPumpCurrent != 0 && !m_stokesLaserDriver->isConnected()) {
            disconnectedDevices << "Stokes激光器";
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

void Integration::on_btnConfirmStagePreset_clicked()
{
    // 获取时间间隔
    m_powerPresetTimeIntervalStage = ui->spinBoxStageTimeInterval->value();
    
    // 调用执行函数
    on_btnStartPowerExecutionStage_clicked();
}

void Integration::addPowerPresetRowStage()
{
    int row = m_powerPresetTableStage->rowCount();
    m_powerPresetTableStage->insertRow(row);
    
    // 添加所有列的item
    for (int col = 0; col < 6; col++) {
        QTableWidgetItem *item = new QTableWidgetItem();
        item->setTextAlignment(Qt::AlignCenter);  // 居中对齐
        
        if (col == 0) {
            // 序号列：自动编号且不可编辑
            item->setText(QString::number(row + 1));
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        } else {
            // 数据列：默认为空,可编辑
            item->setText("");
        }
        
        m_powerPresetTableStage->setItem(row, col, item);
    }
    
    qDebug() << "添加功率预设行（位移台页），当前行数:" << m_powerPresetTableStage->rowCount();
}

void Integration::removePowerPresetRowStage(int row)
{
    // TODO: 删除功率预设表格的指定行（位移台页）
    qDebug() << "删除功率预设行（位移台页）:" << row;
}

QList<StagePowerPreset> Integration::loadPowerPresetsFromTableStage()
{
    QList<StagePowerPreset> presets;
    
    for (int row = 0; row < m_powerPresetTableStage->rowCount(); row++) {
        StagePowerPreset preset;
        
        // 读取旋转台角度（第0列）
        QTableWidgetItem *itemAngle = m_powerPresetTableStage->item(row, 0);
        if (itemAngle) {
            preset.stageAngle = itemAngle->text().toFloat();
        }
        
        // 读取直线台位置（第1列）
        QTableWidgetItem *itemPos = m_powerPresetTableStage->item(row, 1);
        if (itemPos) {
            preset.stagePosition = itemPos->text().toFloat();
        }
        
        // 读取种子源泵电流（第2列）
        QTableWidgetItem *itemSeed = m_powerPresetTableStage->item(row, 2);
        if (itemSeed) {
            preset.seedPumpCurrent = itemSeed->text().toFloat();
        }
        
        // 读取FOPO泵电流（第3列）
        QTableWidgetItem *itemFOPO = m_powerPresetTableStage->item(row, 3);
        if (itemFOPO) {
            preset.fopoPumpCurrent = itemFOPO->text().toFloat();
        }
        
        // 读取Stokes泵电流（第4列）
        QTableWidgetItem *itemStokes = m_powerPresetTableStage->item(row, 4);
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
    
    // 1. 设置旋转台角度
    if (preset.stageAngle != 0) {
        if (m_stageController->isConnected()) {
            if (m_stageController->moveAbsolute(preset.stageAngle, true)) {
                successList << QString("旋转台角度: %.1f度").arg(preset.stageAngle);
            } else {
                failedList << QString("旋转台角度: %.1f度").arg(preset.stageAngle);
            }
        } else {
            skippedList << QString("旋转台角度: %.1f度（设备未连接）").arg(preset.stageAngle);
        }
    }
    
    // 2. 设置直线台位置
    if (preset.stagePosition != 0) {
        if (m_stageController->isConnected()) {
            if (m_stageController->moveAbsolute(preset.stagePosition, false)) {
                successList << QString("直线台位置: %.1f mm").arg(preset.stagePosition);
            } else {
                failedList << QString("直线台位置: %.1f mm").arg(preset.stagePosition);
            }
        } else {
            skippedList << QString("直线台位置: %.1f mm（设备未连接）").arg(preset.stagePosition);
        }
    }
    
    // 3. 设置种子源泵功率
    if (preset.seedPumpCurrent != 0) {
        if (m_seedLaserDriver->isConnected()) {
            if (m_seedLaserDriver->setCurrent(preset.seedPumpCurrent)) {
                successList << QString("种子源泵: %.1f mA").arg(preset.seedPumpCurrent);
            } else {
                failedList << QString("种子源泵: %.1f mA").arg(preset.seedPumpCurrent);
            }
        } else {
            skippedList << QString("种子源泵: %.1f mA（设备未连接）").arg(preset.seedPumpCurrent);
        }
    }
    
    // 4. 设置FOPO泵功率
    if (preset.fopoPumpCurrent != 0) {
        if (m_fopoLaserDriver->isConnected()) {
            if (m_fopoLaserDriver->setCurrent(preset.fopoPumpCurrent)) {
                successList << QString("FOPO泵: %.1f A").arg(preset.fopoPumpCurrent);
            } else {
                failedList << QString("FOPO泵: %.1f A").arg(preset.fopoPumpCurrent);
            }
        } else {
            skippedList << QString("FOPO泵: %.1f A（设备未连接）").arg(preset.fopoPumpCurrent);
        }
    }
    
    // 5. 设置Stokes泵功率
    if (preset.stokesPumpCurrent != 0) {
        if (m_stokesLaserDriver->isConnected()) {
            if (m_stokesLaserDriver->setCurrent(preset.stokesPumpCurrent)) {
                successList << QString("Stokes泵: %.1f mA").arg(preset.stokesPumpCurrent);
            } else {
                failedList << QString("Stokes泵: %.1f mA").arg(preset.stokesPumpCurrent);
            }
        } else {
            skippedList << QString("Stokes泵: %.1f mA（设备未连接）").arg(preset.stokesPumpCurrent);
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

void Integration::on_btnStageDelayPreset_clicked()
{
    // 切换延迟线预设置的显示/隐藏状态
    bool isVisible = ui->groupBoxDelayPresetsNew->isVisible();
    ui->groupBoxDelayPresetsNew->setVisible(!isVisible);
    
    // 更新底部控制按钮的显示状态
    updateStageConfirmButtonVisibility();
    
    // 更新按钮文字提示
    if (!isVisible) {
        qDebug() << "显示延迟线预设置（位移台页）";
    } else {
        qDebug() << "隐藏延迟线预设置（位移台页）";
    }
}

void Integration::on_btnAddDelayPresetRow_clicked()
{
    addDelayPresetRow(PresetPageType::StagePage);
}

void Integration::on_btnClearDelayPresets_clicked()
{
    m_delayPresetTable->setRowCount(0);
    qDebug() << "清空延迟预设（位移台页）";
}

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

void Integration::on_btnMoveUpDelayPreset_clicked()
{
    int currentRow = m_delayPresetTable->currentRow();
    if (currentRow < 0) {
        QMessageBox::warning(this, "提示", "请先选择要操作的行");
        return;
    }
    moveRowUp(m_delayPresetTable, currentRow);
}

void Integration::on_btnMoveDownDelayPreset_clicked()
{
    int currentRow = m_delayPresetTable->currentRow();
    if (currentRow < 0) {
        QMessageBox::warning(this, "提示", "请先选择要操作的行");
        return;
    }
    moveRowDown(m_delayPresetTable, currentRow);
}

void Integration::on_btnDeleteDelayPreset_clicked()
{
    int currentRow = m_delayPresetTable->currentRow();
    if (currentRow < 0) {
        QMessageBox::warning(this, "提示", "请先选择要操作的行");
        return;
    }
    deleteRow(m_delayPresetTable, currentRow);
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

void Integration::addDelayPresetRow(PresetPageType pageType)
{
    QTableWidget *table = getDelayPresetTable(pageType);
    if (!table) return;
    
    int row = table->rowCount();
    table->insertRow(row);
    
    // 添加所有列的item
    for (int col = 0; col < 3; col++) {
        QTableWidgetItem *item = new QTableWidgetItem();
        item->setTextAlignment(Qt::AlignCenter);  // 居中对齐
        
        if (col == 0) {
            // 序号列：自动编号且不可编辑
            item->setText(QString::number(row + 1));
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        } else {
            // 数据列：默认为空,可编辑
            item->setText("");
        }
        
        table->setItem(row, col, item);
    }
    
    QString pageName = (pageType == PresetPageType::GalvoPage) ? "振镜页" : "位移台页";
    qDebug() << "添加延迟预设行（" << pageName << "），当前行数:" << table->rowCount();
}

void Integration::removeDelayPresetRow(PresetPageType pageType, int row)
{
    // TODO: 根据页面类型删除延迟预设行
    QString pageName = (pageType == PresetPageType::GalvoPage) ? "振镜页" : "位移台页";
    qDebug() << "删除延迟预设行（" << pageName << "）:" << row;
}

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
                // 将角度转换为振镜坐标
                float x = preset.galvoAngle * 10.0f;  // 示例映射，需要根据实际情况调整
                float y = 0.0f;
                float z = 0.0f;
                
                if (m_galvoMirror->scannerJump(x, y, z)) {
                    successList << QString("振镜角度: %.1f度").arg(preset.galvoAngle);
                } else {
                    failedList << QString("振镜角度: %.1f度").arg(preset.galvoAngle);
                }
            } else {
                skippedList << QString("振镜角度: %.1f度（设备未连接）").arg(preset.galvoAngle);
            }
        }
    } else {
        // 位移台页：设置旋转台角度
        if (preset.stagePosition != 0) {
            if (m_stageController->isConnected()) {
                // 使用旋转台模式
                bool isRotation = true;
                
                if (m_stageController->moveAbsolute(preset.stagePosition, isRotation)) {
                    successList << QString("旋转台角度: %.1f度").arg(preset.stagePosition);
                } else {
                    failedList << QString("旋转台角度: %.1f度").arg(preset.stagePosition);
                }
            } else {
                skippedList << QString("旋转台角度: %.1f度（设备未连接）").arg(preset.stagePosition);
            }
        }
    }
    
    // 2. 设置延时线延迟（两个页面都需要）
    if (preset.delayTime != 0) {
        if (m_delayLine->isConnected()) {
            if (m_delayLine->setDelay(preset.delayTime)) {
                successList << QString("延时线: %.1f PS").arg(preset.delayTime);
            } else {
                failedList << QString("延时线: %.1f PS").arg(preset.delayTime);
            }
        } else {
            skippedList << QString("延时线: %.1f PS（设备未连接）").arg(preset.delayTime);
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

// ========== 表格操作辅助函数 ==========

void Integration::moveRowUp(QTableWidget *table, int row)
{
    if (row <= 0) return;
    swapRows(table, row, row - 1);
    updateRowNumbers(table);
}

void Integration::moveRowDown(QTableWidget *table, int row)
{
    if (row >= table->rowCount() - 1) return;
    swapRows(table, row, row + 1);
    updateRowNumbers(table);
}

void Integration::deleteRow(QTableWidget *table, int row)
{
    table->removeRow(row);
    updateRowNumbers(table);
}

void Integration::swapRows(QTableWidget *table, int row1, int row2)
{
    int colCount = table->columnCount();
    
    // 跳过序号列（第0列），交换所有数据列
    for (int col = 1; col < colCount; col++) {
        QWidget *widget1 = table->cellWidget(row1, col);
        QWidget *widget2 = table->cellWidget(row2, col);
        
        if (widget1 && widget2) {
            QLineEdit *edit1 = qobject_cast<QLineEdit*>(widget1);
            QLineEdit *edit2 = qobject_cast<QLineEdit*>(widget2);
            
            if (edit1 && edit2) {
                QString temp = edit1->text();
                edit1->setText(edit2->text());
                edit2->setText(temp);
            }
        }
    }
}

void Integration::updateRowNumbers(QTableWidget *table)
{
    for (int row = 0; row < table->rowCount(); row++) {
        QTableWidgetItem *item = table->item(row, 0);
        if (item) {
            item->setText(QString::number(row + 1));
        }
    }
}

// ========== 预设置显示控制辅助函数 ==========

void Integration::updateGalvoConfirmButtonVisibility()
{
    // 检查振镜页是否有任意一个预设置显示
    bool anyVisible = ui->groupBoxPowerPresetsNew->isVisible() || 
                      ui->groupBoxDelayPresetsGalvo->isVisible();
    
    // 更新底部控制按钮的显示状态
    ui->widgetGalvoConfirmButton->setVisible(anyVisible);
    
    if (anyVisible) {
        qDebug() << "振镜页：显示底部控制按钮";
    } else {
        qDebug() << "振镜页：隐藏底部控制按钮";
    }
}

void Integration::updateStageConfirmButtonVisibility()
{
    // 检查位移台页是否有任意一个预设置显示
    bool anyVisible = ui->groupBoxPowerPresetsStage->isVisible() || 
                      ui->groupBoxDelayPresetsNew->isVisible();
    
    // 更新底部控制按钮的显示状态
    ui->widgetStageConfirmButton->setVisible(anyVisible);
    
    if (anyVisible) {
        qDebug() << "位移台页：显示底部控制按钮";
    } else {
        qDebug() << "位移台页：隐藏底部控制按钮";
    }
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

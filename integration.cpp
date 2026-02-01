#include "integration.h"
#include "ui_integration.h"
#include "Communication/serial_port_base.h"
#include "GalvoMirror/galvo_tcp_controller.h"
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
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QChartView>

Integration::Integration(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Integration)
    , m_seedLaserDriver(nullptr)
    , m_fopoLaserDriver(nullptr)
    , m_stokesLaserDriver(nullptr)
    , m_spectrometer(nullptr)
    , m_stageController(nullptr)
    , m_galvoTcp(nullptr)
    , m_delayLine(nullptr)
    , m_configManager(nullptr)
    , m_dataManager(nullptr)
    , m_csvExporter(nullptr)
    , m_imageSaver(nullptr)
    , m_presetManager(nullptr)
    , m_isContinuousMeasuring(false)
    , m_isMeasuring(false)
    , m_measureTimer(nullptr)
    , m_chartView(nullptr)
    , m_chart(nullptr)
    , m_series(nullptr)
    , m_axisX(nullptr)
    , m_axisY(nullptr)
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
}

Integration::~Integration()
{
    // 清理设备
    delete m_seedLaserDriver;
    delete m_fopoLaserDriver;
    delete m_stokesLaserDriver;
    delete m_spectrometer;
    delete m_stageController;
    delete m_galvoTcp;
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
    
    // 创建光谱仪实例（单个）
    m_spectrometer = new Spectrometer(this);
    
    // 创建其他设备实例
    m_stageController = new StageController(this);
    m_galvoTcp = new GalvoTcpController(this);  // 创建 TCP 控制器
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
    // 初始化串口下拉框
    initSerialPortCombos();
    
    // 初始化光谱图表
    initSpectrumChart();
    
    // 初始化预设表格
    initPresetTables();
    
    // 更新连接状态
    updateConnectionStatus();
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
    
    // 连接光谱仪信号
    connect(m_spectrometer, &Spectrometer::statusChanged,
            this, &Integration::onSpectrometerStatusChanged);
    connect(m_spectrometer, &Spectrometer::errorOccurred,
            this, &Integration::onDeviceError);
    connect(m_spectrometer, &Spectrometer::spectrumDataReady,
            this, &Integration::onSpectrumDataReady);
    
    // 连接位移台信号
    connect(m_stageController, &StageController::statusChanged,
            this, &Integration::onStageStatusChanged);
    connect(m_stageController, &StageController::errorOccurred,
            this, &Integration::onDeviceError);
    connect(m_stageController, &StageController::positionChanged,
            this, &Integration::onStagePositionChanged);
    connect(m_stageController, &StageController::moveCompleted,
            this, &Integration::onStageMoveCompleted);
    
    // 连接振镜信号（TCP方式）
    connect(m_galvoTcp, &GalvoTcpController::statusChanged,
            this, &Integration::onGalvoStatusChanged);
    connect(m_galvoTcp, &GalvoTcpController::errorOccurred,
            this, &Integration::onDeviceError);
    
    // 连接延时线信号
    connect(m_delayLine, &DelayLine::statusChanged,
            this, &Integration::onDelayStatusChanged);
    connect(m_delayLine, &DelayLine::errorOccurred,
            this, &Integration::onDeviceError);
    connect(m_delayLine, &DelayLine::delayChanged,
            this, &Integration::onDelayChanged);
    
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


// ========== UI初始化辅助函数 ==========

void Integration::initSerialPortCombos()
{
    // 初始化所有串口下拉框
    populateSerialPortCombo(ui->comboBoxSeedLaserPort);
    populateSerialPortCombo(ui->comboBoxFOPOLaserPort);
    populateSerialPortCombo(ui->comboBoxStokesLaserPort);
    populateSerialPortCombo(ui->comboBoxSpectrometerPort);
    populateSerialPortCombo(ui->comboBoxStagePort);
    populateSerialPortCombo(ui->comboBoxDelayPort);
    
    // 初始化波特率下拉框
    populateBaudRateCombo(ui->comboBoxSeedLaserBaudRate);
    populateBaudRateCombo(ui->comboBoxFOPOLaserBaudRate);
    populateBaudRateCombo(ui->comboBoxStokesLaserBaudRate);
    populateBaudRateCombo(ui->comboBoxSpectrometerBaudRate);
    populateBaudRateCombo(ui->comboBoxStageBaudRate);
    populateBaudRateCombo(ui->comboBoxDelayBaudRate);
    
    // 设置默认波特率
    ui->comboBoxSeedLaserBaudRate->setCurrentText("9600");
    ui->comboBoxFOPOLaserBaudRate->setCurrentText("9600");
    ui->comboBoxStokesLaserBaudRate->setCurrentText("9600");
    ui->comboBoxSpectrometerBaudRate->setCurrentText("115200");
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

void Integration::on_btnConnectSpectrometer_clicked()
{
    SerialConfig config = getSpectrometerSerialConfig();
    
    // 检查是否选择了串口
    if (config.portName.isEmpty()) {
        QMessageBox::warning(this, "连接失败", 
            "光谱仪：请先选择串口设备！\n\n请在串口下拉框中选择一个可用的串口。");
        return;
    }
    
    // 显示连接中状态
    updateStatusIndicator(ui->labelSpectrometerStatus, DeviceStatus::Connecting);
    updateStatusBar("光谱仪正在连接...");
    
    // 处理 UI 事件
    QCoreApplication::processEvents();
    
    // 使用 QTimer 异步执行连接操作
    QTimer::singleShot(50, this, [this, config]() {
        // 设置串口参数
        m_spectrometer->setPortName(config.portName);
        m_spectrometer->setBaudRate(config.baudRate);
        m_spectrometer->setDataBits(static_cast<int>(config.dataBits));
        m_spectrometer->setStopBits(static_cast<int>(config.stopBits));
        m_spectrometer->setParity(static_cast<int>(config.parity));
        
        // 处理 UI 事件
        QCoreApplication::processEvents();
        
        if (m_spectrometer->connect()) {
            updateStatusBar("光谱仪连接成功");
            updateStatusIndicator(ui->labelSpectrometerStatus, DeviceStatus::Connected);
            
            QString info = QString("像素数: %1, 序列号: %2")
                           .arg(m_spectrometer->getPixelLength())
                           .arg(m_spectrometer->getSerialNumber());
            qDebug() << "光谱仪信息:" << info;
        } else {
            updateStatusBar("光谱仪连接失败");
            updateStatusIndicator(ui->labelSpectrometerStatus, DeviceStatus::Error);
            QMessageBox::warning(this, "连接失败", m_spectrometer->getLastError());
        }
    });
}

void Integration::on_btnDisconnectSpectrometer_clicked()
{
    m_spectrometer->disconnect();
    updateStatusIndicator(ui->labelSpectrometerStatus, DeviceStatus::Disconnected);
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
    QString ipAddress = getGalvoIPAddress();
    
    // 检查是否输入了 IP 地址
    if (ipAddress.isEmpty()) {
        QMessageBox::warning(this, "连接失败", 
            "振镜：请先输入 IP 地址！\n\n请在 IP 地址输入框中输入振镜的 IP 地址。");
        return;
    }
    
    // 显示连接中状态
    updateStatusIndicator(ui->labelGalvoStatus, DeviceStatus::Connecting);
    updateStatusBar("振镜正在连接（TCP 方式）...");
    
    // 处理 UI 事件
    QCoreApplication::processEvents();
    
    // 使用 QTimer 异步执行连接操作
    QTimer::singleShot(50, this, [this, ipAddress]() {
        m_galvoTcp->setIPAddress(ipAddress);
        m_galvoTcp->setPort(2000);  // 默认端口，可能需要调整
        
        // 处理 UI 事件
        QCoreApplication::processEvents();
        
        if (m_galvoTcp->connect()) {
            updateStatusBar("振镜连接成功（TCP 方式）");
            updateStatusIndicator(ui->labelGalvoStatus, DeviceStatus::Connected);
            qDebug() << "振镜连接成功（TCP 方式）, IP:" << ipAddress;
        } else {
            updateStatusBar("振镜连接失败");
            updateStatusIndicator(ui->labelGalvoStatus, DeviceStatus::Error);
            QString errorMsg = m_galvoTcp->getLastError();
            qDebug() << "振镜连接失败：" << errorMsg;
            
            QString userMsg = "振镜连接失败（TCP 方式）\n\n";
            userMsg += "错误信息：" + errorMsg + "\n\n";
            userMsg += "可能原因：\n";
            userMsg += "1. IP 地址错误（当前：" + ipAddress + "）\n";
            userMsg += "2. 设备未上电或网络未连接\n";
            userMsg += "3. 端口号不正确（当前使用：2000）\n";
            userMsg += "4. 防火墙阻止连接\n";
            userMsg += "5. 设备不在同一网段\n\n";
            userMsg += "解决方法：\n";
            userMsg += "• 检查设备是否上电\n";
            userMsg += "• 确认 IP 地址正确\n";
            userMsg += "• 尝试 ping " + ipAddress + "\n";
            userMsg += "• 检查网络连接";
            
            QMessageBox::warning(this, "连接失败", userMsg);
        }
    });
}

void Integration::on_btnDisconnectGalvo_clicked()
{
    m_galvoTcp->disconnect();
    updateStatusIndicator(ui->labelGalvoStatus, DeviceStatus::Disconnected);
    updateStatusBar("振镜已断开");
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

void Integration::on_btnSingleMeasure_clicked()
{
    if (!m_spectrometer->isConnected()) {
        QMessageBox::warning(this, "错误", "请先连接光谱仪");
        return;
    }
    
    startMeasurement();
}

void Integration::on_btnContinuousMeasure_clicked()
{
    if (!m_spectrometer->isConnected()) {
        QMessageBox::warning(this, "错误", "请先连接光谱仪");
        return;
    }
    
    // TODO: 实现连续测量模式
    startMeasurement();
}

void Integration::on_btnStopMeasure_clicked()
{
    stopMeasurement();
}

void Integration::on_btnSavePlot_clicked()
{
    saveSpectrum();
}

void Integration::on_btnResetView_clicked()
{
    if (!m_chart) {
        QMessageBox::warning(this, "错误", "图表未初始化");
        return;
    }
    
    // 重置坐标轴范围
    if (m_axisX) {
        m_axisX->setRange(200, 1100);  // 波长范围 200-1100 nm
    }
    if (m_axisY) {
        m_axisY->setRange(0, 65535);  // 强度范围
    }
    
    // 重置缩放
    m_chart->zoomReset();
    
    updateStatusBar("视图已重置");
    qDebug() << "视图已重置";
}

void Integration::on_btnClearPlot_clicked()
{
    if (!m_series) {
        QMessageBox::warning(this, "错误", "数据系列未初始化");
        return;
    }
    
    // 清除数据
    m_series->clear();
    m_lastSpectrumData.clear();
    
    updateStatusBar("光谱数据已清除");
    qDebug() << "光谱数据已清除";
}

// ========== 峰值检测槽函数 ==========

void Integration::on_btnDetectPeaks_clicked()
{
    if (m_lastSpectrumData.isEmpty()) {
        QMessageBox::warning(this, "错误", "没有可用的光谱数据，请先进行测量");
        return;
    }
    
    detectPeaks();
    updatePeaksTable();
    
    updateStatusBar(QString("检测到 %1 个峰值").arg(m_peaks.size()));
    qDebug() << "峰值检测完成，共检测到" << m_peaks.size() << "个峰值";
}

void Integration::on_btnExportPeaks_clicked()
{
    if (m_peaks.isEmpty()) {
        QMessageBox::warning(this, "错误", "没有峰值数据可导出，请先检测峰值");
        return;
    }
    
    exportPeaksToCSV();
}

void Integration::on_btnClearPeaks_clicked()
{
    m_peaks.clear();
    ui->tableWidgetPeaks->setRowCount(0);
    
    updateStatusBar("峰值数据已清除");
    qDebug() << "峰值数据已清除";
}

// ========== 峰值检测辅助函数 ==========

void Integration::detectPeaks()
{
    m_peaks.clear();
    
    if (m_lastSpectrumData.isEmpty()) {
        return;
    }
    
    // 获取检测参数
    int intensityThreshold = ui->spinBoxIntensityThreshold->value();
    int peakWidth = ui->spinBoxPeakWidth->value();
    
    int totalPixels = m_lastSpectrumData.size();
    
    // 峰值检测算法：寻找局部最大值
    for (int i = peakWidth; i < totalPixels - peakWidth; ++i) {
        int currentIntensity = m_lastSpectrumData[i];
        
        // 检查是否超过阈值
        if (currentIntensity < intensityThreshold) {
            continue;
        }
        
        // 检查是否为局部最大值
        bool isPeak = true;
        for (int j = i - peakWidth; j <= i + peakWidth; ++j) {
            if (j != i && m_lastSpectrumData[j] >= currentIntensity) {
                isPeak = false;
                break;
            }
        }
        
        if (isPeak) {
            PeakData peak;
            peak.pixelIndex = i;
            // 将像素索引转换为波长（200-1100 nm）
            peak.wavelength = 200.0 + (900.0 * i / (totalPixels - 1));
            peak.intensity = currentIntensity;
            peak.fwhm = calculateFWHM(i, m_lastSpectrumData);
            
            m_peaks.append(peak);
            
            // 跳过峰值附近的点，避免重复检测
            i += peakWidth;
        }
    }
    
    qDebug() << "峰值检测完成，检测到" << m_peaks.size() << "个峰值";
}

void Integration::updatePeaksTable()
{
    ui->tableWidgetPeaks->setRowCount(0);
    
    for (int i = 0; i < m_peaks.size(); ++i) {
        const PeakData &peak = m_peaks[i];
        
        int row = ui->tableWidgetPeaks->rowCount();
        ui->tableWidgetPeaks->insertRow(row);
        
        // 波长
        QTableWidgetItem *wavelengthItem = new QTableWidgetItem(
            QString::number(peak.wavelength, 'f', 2));
        wavelengthItem->setTextAlignment(Qt::AlignCenter);
        ui->tableWidgetPeaks->setItem(row, 0, wavelengthItem);
        
        // 强度
        QTableWidgetItem *intensityItem = new QTableWidgetItem(
            QString::number(peak.intensity));
        intensityItem->setTextAlignment(Qt::AlignCenter);
        ui->tableWidgetPeaks->setItem(row, 1, intensityItem);
        
        // FWHM
        QTableWidgetItem *fwhmItem = new QTableWidgetItem(
            QString::number(peak.fwhm, 'f', 2));
        fwhmItem->setTextAlignment(Qt::AlignCenter);
        ui->tableWidgetPeaks->setItem(row, 2, fwhmItem);
    }
    
    qDebug() << "峰值表格已更新，共" << m_peaks.size() << "行";
}

void Integration::exportPeaksToCSV()
{
    QString fileName = QFileDialog::getSaveFileName(
        this,
        "导出峰值数据",
        QCoreApplication::applicationDirPath() + "/peaks_data.csv",
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
    
    // 写入 UTF-8 BOM（Excel 识别中文）
    out.setCodec("UTF-8");
    out << "\xEF\xBB\xBF";
    
    // 写入表头
    out << "波长 [nm],强度 [counts],FWHM [nm]\n";
    
    // 写入数据
    for (const PeakData &peak : m_peaks) {
        out << QString::number(peak.wavelength, 'f', 2) << ","
            << peak.intensity << ","
            << QString::number(peak.fwhm, 'f', 2) << "\n";
    }
    
    file.close();
    
    updateStatusBar("峰值数据已导出: " + fileName);
    qDebug() << "峰值数据已导出:" << fileName;
    QMessageBox::information(this, "导出成功", 
        QString("峰值数据已导出到:\n%1\n\n共导出 %2 个峰值")
        .arg(fileName).arg(m_peaks.size()));
}

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
    if (m_spectrometer->startScan()) {
        m_isMeasuring = true;
        updateStatusBar("开始光谱测量");
        qDebug() << "开始光谱测量";
    } else {
        QMessageBox::warning(this, "错误", "启动测量失败：" + m_spectrometer->getLastError());
    }
}

void Integration::stopMeasurement()
{
    m_isMeasuring = false;
    updateStatusBar("停止光谱测量");
    qDebug() << "停止光谱测量";
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
    if (m_chartView) {
        QPixmap pixmap = m_chartView->grab();
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
    bool ok;
    float angle = ui->lineEditGalvoAngle->text().toFloat(&ok);
    
    if (!ok) {
        QMessageBox::warning(this, "错误", "请输入有效的角度值");
        return;
    }
    
    // 检查连接状态
    if (!m_galvoTcp->isConnected()) {
        QMessageBox::warning(this, "错误", "振镜未连接");
        return;
    }
    
    // 设置角度
    if (m_galvoTcp->setAngle(angle, 10.0f)) {
        updateStatusBar("振镜角度设置成功: " + QString::number(angle) + "度");
        qDebug() << "振镜角度设置成功:" << angle << "度";
    } else {
        QMessageBox::warning(this, "错误", "振镜角度设置失败：" + m_galvoTcp->getLastError());
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

void Integration::onSpectrometerStatusChanged(DeviceStatus status)
{
    updateStatusIndicator(ui->labelSpectrometerStatus, status);
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
    qDebug() << "振镜状态改变:" << static_cast<int>(status);
}

void Integration::onDelayStatusChanged(DeviceStatus status)
{
    updateStatusIndicator(ui->labelDelayStatus, status);
    qDebug() << "延时线状态改变:" << static_cast<int>(status);
}

// ========== 光谱仪数据接收槽函数 ==========

void Integration::onSpectrumDataReady(const QVector<int> &intensity)
{
    qDebug() << "收到光谱数据，像素数:" << intensity.size();
    updateSpectrum(intensity);
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
    // 创建图表
    m_chart = new QChart();
    m_chart->setTitle("光谱数据");
    m_chart->setAnimationOptions(QChart::NoAnimation);
    
    // 创建数据系列
    m_series = new QLineSeries();
    m_series->setName("光谱强度");
    m_chart->addSeries(m_series);
    
    // 创建坐标轴
    m_axisX = new QValueAxis();
    m_axisX->setTitleText("波长 [nm]");
    m_axisX->setRange(200, 1100);
    m_axisX->setGridLineVisible(true);  // 显示主网格线
    m_axisX->setMinorGridLineVisible(true);  // 显示次网格线
    m_axisX->setTickCount(10);  // 设置主刻度数量
    m_axisX->setMinorTickCount(4);  // 设置次刻度数量
    m_chart->addAxis(m_axisX, Qt::AlignBottom);
    m_series->attachAxis(m_axisX);
    
    m_axisY = new QValueAxis();
    m_axisY->setTitleText("强度 [counts]");
    m_axisY->setRange(0, 65535);
    m_axisY->setGridLineVisible(true);  // 显示主网格线
    m_axisY->setMinorGridLineVisible(true);  // 显示次网格线
    m_axisY->setTickCount(10);  // 设置主刻度数量
    m_axisY->setMinorTickCount(4);  // 设置次刻度数量
    m_chart->addAxis(m_axisY, Qt::AlignLeft);
    m_series->attachAxis(m_axisY);
    
    // 显示图例
    m_chart->legend()->setVisible(true);
    m_chart->legend()->setAlignment(Qt::AlignTop);
    
    // 创建图表视图
    m_chartView = new QChartView(m_chart);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    
    // 将图表视图添加到UI布局中
    QWidget *plotWidget = ui->widgetSpectrumPlot;
    if (plotWidget) {
        // 清除现有布局
        if (plotWidget->layout()) {
            QLayoutItem *item;
            while ((item = plotWidget->layout()->takeAt(0)) != nullptr) {
                delete item->widget();
                delete item;
            }
            delete plotWidget->layout();
        }
        
        // 创建新布局并添加图表
        QVBoxLayout *layout = new QVBoxLayout(plotWidget);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(m_chartView);
        plotWidget->setLayout(layout);
    }
    
    qDebug() << "光谱图表初始化完成 - 网格线和图例已启用";
}

void Integration::updateSpectrum(const QVector<int> &intensity)
{
    if (!m_series) return;
    
    m_series->clear();
    
    int totalPixels = intensity.size();
    
    // 将像素转换为波长并添加数据点
    for (int i = 0; i < totalPixels; ++i) {
        // 波长范围：200-1100 nm
        double wavelength = 200.0 + (900.0 * i / (totalPixels - 1));
        m_series->append(wavelength, intensity[i]);
    }
    
    // 保存光谱数据
    m_lastSpectrumData = intensity;
    
    // 自动调整Y轴范围
    if (!intensity.isEmpty()) {
        int maxValue = *std::max_element(intensity.begin(), intensity.end());
        m_axisY->setRange(0, maxValue * 1.1);
    }
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

SerialConfig Integration::getSpectrometerSerialConfig()
{
    SerialConfig config;
    config.portName = ui->comboBoxSpectrometerPort->currentText();
    config.baudRate = ui->comboBoxSpectrometerBaudRate->currentData().toInt();
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
    m_powerPresetTable->setColumnCount(5);
    m_powerPresetTable->setHorizontalHeaderLabels({"振镜角度起(度)", "振镜角度止(度)", 
                                                    "种子源泵(mA)", "FOPO泵(A)", "Stokes泵(mA)"});
    m_powerPresetTable->horizontalHeader()->setStretchLastSection(true);
    
    // 初始化振镜页 - 延迟线预设表格
    m_delayPresetTableGalvo = ui->tableWidgetDelayPresetsGalvo;
    m_delayPresetTableGalvo->setColumnCount(2);
    m_delayPresetTableGalvo->setHorizontalHeaderLabels({"振镜角度(度)", "延迟时间(PS)"});
    m_delayPresetTableGalvo->horizontalHeader()->setStretchLastSection(true);
    
    // 初始化位移台页 - 电控与功率预设表格
    m_powerPresetTableStage = ui->tableWidgetPowerPresetsStage;
    m_powerPresetTableStage->setColumnCount(5);
    m_powerPresetTableStage->setHorizontalHeaderLabels({"旋转台角度(度)", "直线台位置(mm)", 
                                                         "种子源泵(mA)", "FOPO泵(A)", "Stokes泵(mA)"});
    m_powerPresetTableStage->horizontalHeader()->setStretchLastSection(true);
    
    // 初始化位移台页 - 延迟线预设表格
    m_delayPresetTable = ui->tableWidgetDelayPresets;
    m_delayPresetTable->setColumnCount(2);
    m_delayPresetTable->setHorizontalHeaderLabels({"振镜角度(度)", "延迟时间(PS)"});
    m_delayPresetTable->horizontalHeader()->setStretchLastSection(true);
    
    qDebug() << "预设表格初始化完成";
}

// ========== 预设管理槽函数 - 振镜页（光源功率预设） ==========

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
    // 加载预设
    m_currentPowerPresets = loadPowerPresetsFromTable();
    
    if (m_currentPowerPresets.isEmpty()) {
        QMessageBox::warning(this, "错误", "没有可执行的预设");
        return;
    }
    
    // 智能检查：分析预设中实际使用的设备
    bool needsGalvo = false;
    bool needsSeed = false;
    bool needsFOPO = false;
    bool needsStokes = false;
    
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
    
    // 检查需要的设备是否连接
    QStringList missingDevices;
    if (needsGalvo && !m_galvoTcp->isConnected()) {
        missingDevices << "振镜";
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
    m_isPowerPresetExecuting = true;
    
    // 执行第一个预设
    executePowerPreset(0);
    
    // 启动定时器
    m_powerPresetDelayTimer->start(m_powerPresetTimeInterval * 1000);
    
    updateStatusBar("开始执行功率预设");
    qDebug() << "开始执行功率预设，共" << m_currentPowerPresets.size() << "个";
}

void Integration::on_btnStopPowerExecution_clicked()
{
    stopPowerPresetExecution();
}

void Integration::onPowerPresetDelayTimeout()
{
    if (!m_isPowerPresetExecuting) {
        m_powerPresetDelayTimer->stop();
        return;
    }
    
    m_currentPowerPresetIndex++;
    
    if (m_currentPowerPresetIndex >= m_currentPowerPresets.size()) {
        // 所有预设执行完成
        stopPowerPresetExecution();
        QMessageBox::information(this, "完成", "所有功率预设执行完成");
        return;
    }
    
    // 执行下一个预设
    executePowerPreset(m_currentPowerPresetIndex);
}

void Integration::addPowerPresetRow()
{
    int row = m_powerPresetTable->rowCount();
    m_powerPresetTable->insertRow(row);
    
    // 添加输入框
    for (int col = 0; col < 5; col++) {
        QLineEdit *lineEdit = new QLineEdit();
        lineEdit->setText("0");
        lineEdit->setAlignment(Qt::AlignCenter);
        m_powerPresetTable->setCellWidget(row, col, lineEdit);
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
        
        // 读取振镜角度起
        QLineEdit *lineEditStart = qobject_cast<QLineEdit*>(m_powerPresetTable->cellWidget(row, 0));
        if (lineEditStart) {
            preset.galvoAngleStart = lineEditStart->text().toFloat();
        }
        
        // 读取振镜角度止
        QLineEdit *lineEditEnd = qobject_cast<QLineEdit*>(m_powerPresetTable->cellWidget(row, 1));
        if (lineEditEnd) {
            preset.galvoAngleEnd = lineEditEnd->text().toFloat();
        }
        
        // 读取种子源泵电流
        QLineEdit *lineEditSeed = qobject_cast<QLineEdit*>(m_powerPresetTable->cellWidget(row, 2));
        if (lineEditSeed) {
            preset.seedPumpCurrent = lineEditSeed->text().toFloat();
        }
        
        // 读取FOPO泵电流
        QLineEdit *lineEditFOPO = qobject_cast<QLineEdit*>(m_powerPresetTable->cellWidget(row, 3));
        if (lineEditFOPO) {
            preset.fopoPumpCurrent = lineEditFOPO->text().toFloat();
        }
        
        // 读取Stokes泵电流
        QLineEdit *lineEditStokes = qobject_cast<QLineEdit*>(m_powerPresetTable->cellWidget(row, 4));
        if (lineEditStokes) {
            preset.stokesPumpCurrent = lineEditStokes->text().toFloat();
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
    
    // 1. 设置振镜角度（使用平均值）
    float galvoAngle = (preset.galvoAngleStart + preset.galvoAngleEnd) / 2.0f;
    if (galvoAngle != 0) {
        if (m_galvoTcp->isConnected()) {
            if (m_galvoTcp->setAngle(galvoAngle, 10.0f)) {
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

void Integration::stopPowerPresetExecution()
{
    m_isPowerPresetExecuting = false;
    m_powerPresetDelayTimer->stop();
    updateStatusBar("功率预设执行已停止");
    qDebug() << "功率预设执行已停止";
}


// ========== 预设管理槽函数 - 振镜页（延迟线预设） ==========

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
    if (needsGalvo && !m_galvoTcp->isConnected()) {
        missingDevices << "振镜";
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
    
    m_currentDelayPresetIndexGalvo++;
    
    if (m_currentDelayPresetIndexGalvo >= m_currentDelayPresetsGalvo.size()) {
        stopDelayPresetExecution(PresetPageType::GalvoPage);
        QMessageBox::information(this, "完成", "所有延迟预设执行完成（振镜页）");
        return;
    }
    
    executeDelayPreset(PresetPageType::GalvoPage, m_currentDelayPresetIndexGalvo);
}

// ========== 预设管理槽函数 - 位移台页（电控与功率预设） ==========

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

void Integration::onPowerPresetDelayTimeoutStage()
{
    if (!m_isPowerPresetExecutingStage) {
        m_powerPresetDelayTimerStage->stop();
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

void Integration::addPowerPresetRowStage()
{
    int row = m_powerPresetTableStage->rowCount();
    m_powerPresetTableStage->insertRow(row);
    
    // 添加输入框
    for (int col = 0; col < 5; col++) {
        QLineEdit *lineEdit = new QLineEdit();
        lineEdit->setText("0");
        lineEdit->setAlignment(Qt::AlignCenter);
        m_powerPresetTableStage->setCellWidget(row, col, lineEdit);
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
        
        // 读取旋转台角度
        QLineEdit *lineEditAngle = qobject_cast<QLineEdit*>(m_powerPresetTableStage->cellWidget(row, 0));
        if (lineEditAngle) {
            preset.stageAngle = lineEditAngle->text().toFloat();
        }
        
        // 读取直线台位置
        QLineEdit *lineEditPos = qobject_cast<QLineEdit*>(m_powerPresetTableStage->cellWidget(row, 1));
        if (lineEditPos) {
            preset.stagePosition = lineEditPos->text().toFloat();
        }
        
        // 读取种子源泵电流
        QLineEdit *lineEditSeed = qobject_cast<QLineEdit*>(m_powerPresetTableStage->cellWidget(row, 2));
        if (lineEditSeed) {
            preset.seedPumpCurrent = lineEditSeed->text().toFloat();
        }
        
        // 读取FOPO泵电流
        QLineEdit *lineEditFOPO = qobject_cast<QLineEdit*>(m_powerPresetTableStage->cellWidget(row, 3));
        if (lineEditFOPO) {
            preset.fopoPumpCurrent = lineEditFOPO->text().toFloat();
        }
        
        // 读取Stokes泵电流
        QLineEdit *lineEditStokes = qobject_cast<QLineEdit*>(m_powerPresetTableStage->cellWidget(row, 4));
        if (lineEditStokes) {
            preset.stokesPumpCurrent = lineEditStokes->text().toFloat();
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
    
    // 智能检查
    bool needsGalvo = false;
    bool needsDelay = false;
    
    for (const DelayPreset &preset : m_currentDelayPresets) {
        if (preset.galvoAngle != 0) {
            needsGalvo = true;
        }
        if (preset.delayTime != 0) {
            needsDelay = true;
        }
    }
    
    QStringList missingDevices;
    if (needsGalvo && !m_galvoTcp->isConnected()) {
        missingDevices << "振镜";
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
    
    // 添加输入框
    for (int col = 0; col < 2; col++) {
        QLineEdit *lineEdit = new QLineEdit();
        lineEdit->setText("0");
        lineEdit->setAlignment(Qt::AlignCenter);
        table->setCellWidget(row, col, lineEdit);
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
        
        // 读取振镜角度
        QLineEdit *lineEditAngle = qobject_cast<QLineEdit*>(table->cellWidget(row, 0));
        if (lineEditAngle) {
            preset.galvoAngle = lineEditAngle->text().toFloat();
        }
        
        // 读取延迟时间
        QLineEdit *lineEditDelay = qobject_cast<QLineEdit*>(table->cellWidget(row, 1));
        if (lineEditDelay) {
            preset.delayTime = lineEditDelay->text().toFloat();
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
    
    // 1. 设置振镜角度
    if (preset.galvoAngle != 0) {
        if (m_galvoTcp->isConnected()) {
            if (m_galvoTcp->setAngle(preset.galvoAngle, 10.0f)) {
                successList << QString("振镜角度: %.1f度").arg(preset.galvoAngle);
            } else {
                failedList << QString("振镜角度: %.1f度").arg(preset.galvoAngle);
            }
        } else {
            skippedList << QString("振镜角度: %.1f度（设备未连接）").arg(preset.galvoAngle);
        }
    }
    
    // 2. 设置延时线延迟
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

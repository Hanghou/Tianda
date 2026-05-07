#include "galvo_gmc_controller.h"
#include <QDebug>
#include <QThread>
#include <cmath>
#include <cstring>  // for memset

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

GalvoGMCController::GalvoGMCController(QObject *parent)
    : DeviceBase(parent)
    , m_ipAddress("172.18.34.227")
    , m_ipIndex(-1)
    , m_isConnected(false)
    , m_isInitialized(false)
    , m_hWnd(nullptr)
{
    setDeviceName("思特GMC振镜控制卡");
    
    // 初始化默认打标参数
    memset(&m_defaultParam, 0, sizeof(MarkParameter));
    m_defaultParam.MarkSpeed = 1000;        // 打标速度 1000mm/s
    m_defaultParam.JumpSpeed = 3000;        // 跳转速度 3000mm/s
    m_defaultParam.MarkDelay = 100;         // 打标延时 100us
    m_defaultParam.JumpDelay = 100;         // 跳转延时 100us
    m_defaultParam.PolygonDelay = 50;       // 转弯延时 50us
    m_defaultParam.MarkCount = 1;           // 打标次数 1次
    m_defaultParam.LaserOnDelay = 0;        // 开激光延时 0us
    m_defaultParam.LaserOffDelay = 0;       // 关激光延时 0us
    m_defaultParam.FPKDelay = 0;            // 首脉冲抑制延时 0us
    m_defaultParam.FPKLength = 0;           // 首脉冲抑制长度 0us
    m_defaultParam.QDelay = 0;              // Q频率延时 0us
    m_defaultParam.DutyCycle = 0.5f;        // 占空比 50%
    m_defaultParam.Frequency = 20.0f;       // 频率 20kHz
    m_defaultParam.StandbyFrequency = 20.0f;// 待机频率 20kHz
    m_defaultParam.StandbyDutyCycle = 0.5f; // 待机占空比 50%
    m_defaultParam.LaserPower = 50.0f;      // 激光功率 50%
    m_defaultParam.AnalogMode = 0;          // 不使用模拟量
    m_defaultParam.Waveform = 0;            // SPI波形号 0
    m_defaultParam.PulseWidthMode = 0;      // 不使用脉宽模式
    m_defaultParam.PulseWidth = 0;          // 脉宽 0ns
}

GalvoGMCController::~GalvoGMCController()
{
    disconnect();
}

bool GalvoGMCController::initialize(HWND hWnd)
{
    if (m_isInitialized) {
        qDebug() << "振镜控制卡已经初始化";
        return true;
    }
    
    m_hWnd = hWnd;
    
    // 初始化通讯
    int ret = HM_InitBoard(hWnd);
    if (ret != HM_OK) {
        setError("初始化通讯失败");
        return false;
    }
    
    m_isInitialized = true;
    qDebug() << "振镜控制卡初始化成功";
    return true;
}

bool GalvoGMCController::connect()
{
    if (!m_isInitialized) {
        setError("请先调用initialize()初始化");
        return false;
    }
    
    if (m_isConnected) {
        qDebug() << "振镜已经连接";
        return true;
    }
    
    qDebug() << "正在连接振镜控制卡:" << m_ipAddress;
    
    setStatus(DeviceStatus::Connecting);
    
    // 通过IP地址连接
    QByteArray ipBytes = m_ipAddress.toUtf8();
    int ret = HM_ConnectByIpStr(ipBytes.data());
    
    if (ret != HM_OK) {
        setError(QString("连接失败: IP=%1").arg(m_ipAddress));
        setStatus(DeviceStatus::Error);
        return false;
    }
    
    // 获取IP索引
    m_ipIndex = HM_GetIndexByIpAddr(ipBytes.data());
    if (m_ipIndex < 0) {
        setError("获取IP索引失败");
        setStatus(DeviceStatus::Error);
        return false;
    }
    
    // 等待连接状态
    QThread::msleep(500);
    
    // 检查连接状态
    int status = HM_GetConnectStatus(m_ipIndex);
    if (status == HM_DEV_Connect) {
        m_isConnected = true;
        setStatus(DeviceStatus::Connected);
        emit connected();
        qDebug() << "振镜连接成功, IP索引:" << m_ipIndex;
        return true;
    } else {
        setError("连接状态异常");
        setStatus(DeviceStatus::Error);
        return false;
    }
}

void GalvoGMCController::disconnect()
{
    if (m_isConnected && m_ipIndex >= 0) {
        HM_DisconnectTo(m_ipIndex);
        m_isConnected = false;
        setStatus(DeviceStatus::Disconnected);
        emit disconnected();
        qDebug() << "振镜已断开连接";
    }
}

bool GalvoGMCController::isConnected() const
{
    return m_isConnected;
}

QString GalvoGMCController::getDeviceInfo() const
{
    return QString("%1 (IP: %2, 索引: %3)")
           .arg(m_deviceName)
           .arg(m_ipAddress)
           .arg(m_ipIndex);
}

bool GalvoGMCController::setAngle(float angle, float radius)
{
    if (!m_isConnected) {
        setError("振镜未连接");
        return false;
    }
    
    qDebug() << "设置振镜角度:" << angle << "度, 半径:" << radius << "mm";
    
    // 生成UDM文件
    if (!generateAngleUDM(angle, radius)) {
        return false;
    }
    
    // 下载并执行
    return downloadAndExecute("D:/galvo_angle.bin");
}

bool GalvoGMCController::moveToZero()
{
    return scannerJump(0.0f, 0.0f);
}

bool GalvoGMCController::scannerJump(float x, float y)
{
    if (!m_isConnected) {
        setError("振镜未连接");
        return false;
    }
    
    qDebug() << "振镜跳转到坐标:(" << x << "," << y << ")";
    
    // 使用在线跳转命令（不需要生成UDM文件）
    int ret = HM_ScannerJump(m_ipIndex, x, y, 0.0f);
    if (ret != HM_OK) {
        setError("振镜跳转失败");
        return false;
    }
    
    return true;
}

bool GalvoGMCController::setGuidLaser(bool enable)
{
    if (!m_isConnected) {
        setError("振镜未连接");
        return false;
    }
    
    int ret = HM_SetGuidLaser(m_ipIndex, enable);
    if (ret != HM_OK) {
        setError(enable ? "开启红光失败" : "关闭红光失败");
        return false;
    }
    
    qDebug() << (enable ? "红光已开启" : "红光已关闭");
    return true;
}

bool GalvoGMCController::generateAngleUDM(float angle, float radius)
{
    // 创建新的UDM文件
    if (UDM_NewFile() != HM_OK) {
        setError("创建UDM文件失败");
        return false;
    }
    
    // 开始主函数
    UDM_Main();
    
    // 设置协议（XY2-100，2D打标）
    UDM_SetProtocol(1, 0);
    
    // 设置图层参数
    UDM_SetLayersPara(&m_defaultParam, 1);
    
    // 将角度转换为XY坐标
    float angleRad = angle * M_PI / 180.0f;
    float x = radius * std::cos(angleRad);
    float y = radius * std::sin(angleRad);
    
    // 创建轨迹：从原点到目标点的直线
    structUdmPos points[2];
    points[0].x = 0.0f;
    points[0].y = 0.0f;
    points[0].z = 0.0f;
    points[0].a = 0.0f;
    
    points[1].x = x;
    points[1].y = y;
    points[1].z = 0.0f;
    points[1].a = 0.0f;
    
    // 添加轨迹
    UDM_AddPolyline2D(points, 2, 0);
    
    // 跳转回零点
    UDM_Jump(0.0f, 0.0f, 0.0f);
    
    // 结束主函数
    UDM_EndMain();
    
    // 保存文件
    if (UDM_SaveToFile("D:/galvo_angle.bin") != HM_OK) {
        setError("保存UDM文件失败");
        return false;
    }
    
    qDebug() << "UDM文件生成成功: 角度=" << angle << "度, 坐标=(" << x << "," << y << ")";
    return true;
}

bool GalvoGMCController::generateJumpUDM(float x, float y)
{
    // 创建新的UDM文件
    if (UDM_NewFile() != HM_OK) {
        setError("创建UDM文件失败");
        return false;
    }
    
    // 开始主函数
    UDM_Main();
    
    // 设置协议
    UDM_SetProtocol(1, 0);
    
    // 设置图层参数
    UDM_SetLayersPara(&m_defaultParam, 1);
    
    // 跳转到目标位置
    UDM_Jump(x, y, 0.0f);
    
    // 结束主函数
    UDM_EndMain();
    
    // 保存文件
    if (UDM_SaveToFile("D:/galvo_jump.bin") != HM_OK) {
        setError("保存UDM文件失败");
        return false;
    }
    
    return true;
}

bool GalvoGMCController::downloadAndExecute(const QString &filePath)
{
    QByteArray pathBytes = filePath.toUtf8();
    
    // 同步下载文件
    int ret = HM_DownloadMarkFileSyn(m_ipIndex, pathBytes.data(), m_hWnd);
    if (ret != HM_OK) {
        setError("下载UDM文件失败");
        return false;
    }
    
    qDebug() << "UDM文件下载成功";
    
    // 开始打标
    ret = HM_StartMark(m_ipIndex);
    if (ret != HM_OK) {
        setError("启动打标失败");
        return false;
    }
    
    qDebug() << "打标已启动";
    return true;
}

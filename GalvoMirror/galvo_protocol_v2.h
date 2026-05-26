#ifndef GALVO_PROTOCOL_V2_H
#define GALVO_PROTOCOL_V2_H

// 大族思特 MEMS 振镜控制卡 TCP/UDP 协议结构定义（C++ Qt 二次开发）
// IP:172.18.34.227 默认；UDP:5998 心跳；TCP:6002 图形参数；TCP:6003 控制指令
// 全部结构体 1 字节对齐

#include <cstdint>
#include <cstring>

#pragma pack(push, 1)

// ========== 控制操作枚举（端口 6003） ==========
enum GalvoControlMode : uint32_t {
    GALVO_CMD_START    = 'e',  // 101 - 开始打标
    GALVO_CMD_STOP     = 'r',  // 114 - 停止/复位
    GALVO_CMD_PAUSE    = 'p',  // 112 - 暂停
    GALVO_CMD_CONTINUE = 'c',  // 99  - 继续
};

// 控制指令帧 - 通过 TCP:6003 发送（20 字节）
struct GalvoControlAction {
    uint32_t nDebugType;   // 固定值 114 ('r')
    uint32_t nOperation;   // 操作码，见 GalvoControlMode
    uint32_t uParam;       // 保留，填0
    uint32_t uStatus;      // 保留，填0
    uint32_t uReserved;    // 保留，填0
};

// ========== 图形参数（端口 6002） ==========

// 图形类型
enum GalvoShapeType : uint32_t {
    GALVO_SHAPE_POINT  = 0x01,  // 点
    GALVO_SHAPE_LINE   = 0x02,  // 线
    GALVO_SHAPE_CIRCLE = 0x03,  // 圆
    GALVO_SHAPE_ARRAY  = 0x08,  // 多点阵列
};

// 激光参数
struct GalvoLaserPara {
    uint32_t nLaserOnDelay;     // 开激光延时(μs)
    uint32_t nLaserOffDelay;    // 关激光延时(μs)
    uint32_t nFPSDelay;         // 首脉冲压缩延时(μs)
    uint32_t nFPSLength;        // 首脉冲压缩长度(μs)
    uint32_t nQDelay;           // Q延时(μs)
    float    DutyCycle;         // 出光占空比(0~1)
    float    Frequency;         // 出光频率(kHz)
    float    StandbyDutyCycle;  // 待机占空比(0~1)
    float    StandbyFrequency;  // 待机频率(kHz)
    float    nLaserPower;       // 激光功率(0~100)
};

// 振镜运动参数
struct GalvoMarkSetting {
    int32_t nMarkV;            // 打标速度(mm/s)
    int32_t nMark2MarkDelay;   // 拐弯延时(μs)
    int32_t nJumpDelay;        // 跳转延时(μs)
    int32_t nMarkDelay;        // 打标延时(μs)
    int32_t nJumpV;            // 跳转速度(mm/s)
    int32_t ScanTimes;         // 扫描次数
};

// 图形信息
struct GalvoShapeInfo {
    float Shape;              // 图形类型: 0x01=点, 0x02=线, 0x03=圆, 0x08=多点阵列
    float PointX;             // 点_X坐标
    float PointY;             // 点_Y坐标
    float Point_LaseronTime;  // 点出光时间(ms)
    float Line_StartX;        // 线_起点X
    float Line_StartY;        // 线_起点Y
    float Line_EndX;          // 线_终点X
    float Line_EndY;          // 线_终点Y
    float CircleX;            // 圆_中心X
    float CircleY;            // 圆_中心Y
    float Circle_Radius;      // 圆_半径R
};

// IO控制
struct GalvoIOControl {
    uint32_t nRedLightEnable;   // 红光预览使能 (0=关, 1=开)
    uint32_t nReadyDownEanble;  // 准备按钮按下 (0=未按, 1=按下)
    uint32_t nLightLevel;       // 光照等级
};

// 多点阵列参数（最大 128 点）
struct GalvoPointPara {
    int32_t PointCount;
    float   PointData_xPos[128];
    float   PointData_yPos[128];
};

// 通讯数据帧
struct GalvoCommFrame {
    int32_t          nHeader;     // 帧头标识，固定 0x5A5AA5A5
    GalvoLaserPara   LaserPara;
    GalvoMarkSetting stmark;
    GalvoShapeInfo   ShpaeInfo;
    GalvoIOControl   IOControl;
    GalvoPointPara   PointPara;
};

// 打标参数总帧 - 通过 TCP:6002 发送
struct GalvoMarkParameter {
    int32_t        m_nCmdType;   // 固定 0x08
    char           cSysCmd;      // 固定 0x14 (20)
    char           cStatus;      // 固定 1
    int16_t        uReserved;    // 保留
    GalvoCommFrame stUnitFrame;
};

// ========== 寄存器读写（端口 6002） ==========

struct GalvoRegCommand {
    char     cCmd;        // 0x03=读, 0x06=写
    char     cDataType;   // 0=INT, 1=UINT
    char     cStatus;
    char     cReserved;
    uint32_t uAddr;       // 寄存器地址
    int32_t  udData;      // 数据值
};

struct GalvoRegCommandFrame {
    uint32_t        m_nCmdType;   // 固定 1
    uint32_t        m_nCmdCount;  // 命令数量
    GalvoRegCommand RegCommand;
};

#pragma pack(pop)

// 协议常量
namespace GalvoProto {
    constexpr const char *kDefaultIP   = "172.18.34.227";
    constexpr quint16     kPortHeart   = 5998;   // UDP 心跳
    constexpr quint16     kPortShape   = 6002;   // TCP 图形参数
    constexpr quint16     kPortAction  = 6003;   // TCP 控制指令
    constexpr int32_t     kFrameHeader = 0x5A5AA5A5;
}

#endif // GALVO_PROTOCOL_V2_H

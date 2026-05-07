#ifndef GALVO_PROTOCOL_H
#define GALVO_PROTOCOL_H

/**
 * @file galvo_protocol.h
 * @brief 思特GMC振镜控制卡协议定义
 * 
 * 包含控制卡的协议类型、状态定义、消息定义等
 */

// ========== 协议类型定义 ==========
enum GalvoProtocol {
    PROTOCOL_SPI = 0,           // SPI协议
    PROTOCOL_XY2_100 = 1,       // XY2-100协议（常用）
    PROTOCOL_SL2 = 2            // SL2协议
};

// ========== 打标维度定义 ==========
enum MarkDimension {
    DIMENSION_2D = 0,           // 2D平面打标
    DIMENSION_3D = 1            // 3D打标
};

// ========== 设备连接状态定义 ==========
enum DeviceStatus {
    DEV_CONNECT = 0,            // 已连接
    DEV_READY = 1,              // 就绪状态
    DEV_NOT_AVAILABLE = 2       // 不可用
};

// ========== 工作状态定义 ==========
enum WorkStatus {
    WORK_READY = 1,             // 就绪状态
    WORK_RUN = 2,               // 运行状态
    WORK_ALARM = 3              // 报警状态
};

// ========== 消息定义 ==========
#define HM_MSG_DeviceStatusUpdate   5991    // 设备状态更新消息
#define HM_MSG_StreamEnd            6012    // 打标文件下载完成消息
#define HM_MSG_UDMHalt              6035    // 打标完成消息
#define HM_MSG_ExecProcess          6037    // 打标进度消息

// ========== 坐标系定义 ==========
enum CoordinateSystem {
    COORD_XY = 0,               // XY
    COORD_X_Y = 1,              // X-Y
    COORD_NEG_XY = 2,           // -XY
    COORD_NEG_X_Y = 3,          // -X-Y
    COORD_YX = 4,               // YX
    COORD_Y_X = 5,              // Y-X
    COORD_NEG_YX = 6,           // -YX
    COORD_NEG_Y_X = 7           // -Y-X
};

// ========== 脱机模式定义 ==========
enum BurnMode {
    BURN_SINGLE = 1,            // 单文档脱机
    BURN_MULTI = 2              // 多文档脱机
};

// ========== 振镜类型定义 ==========
enum GalvoType {
    GALVO_ANALOG = 0,           // 模拟方头或只带位置反馈的方头
    GALVO_DIGITAL_PHOTO = 1,    // 数字光电方头
    GALVO_DIGITAL_GRATING = 2   // 数字光栅方头
};

// ========== 打标参数结构体 ==========
struct MarkParameter {
    unsigned int MarkSpeed;         // 打标速度(mm/s)
    unsigned int JumpSpeed;         // 跳转速度(mm/s)
    unsigned int MarkDelay;         // 打标延时(us)
    unsigned int JumpDelay;         // 跳转延时(us)
    unsigned int PolygonDelay;      // 转弯延时(us)
    unsigned int MarkCount;         // 打标次数
    float LaserOnDelay;             // 开激光延时(us)
    float LaserOffDelay;            // 关激光延时(us)
    float FPKDelay;                 // 首脉冲抑制延时(us)
    float FPKLength;                // 首脉冲抑制长度(us)
    float QDelay;                   // 出光Q频率延时(us)
    float DutyCycle;                // 出光时占空比(0~1)
    float Frequency;                // 出光时频率(kHz)
    float StandbyFrequency;         // 不出光Q频率(kHz)
    float StandbyDutyCycle;         // 不出光Q占空比(0~1)
    float LaserPower;               // 激光能量百分比(0~100)
    unsigned int AnalogMode;        // 1:使用模拟量输出控制能量(0~10V)
    unsigned int Waveform;          // SPI激光器波形号(0~63)
    unsigned int PulseWidthMode;    // 0:不开启MOPA脉宽使能, 1:开启
    unsigned int PulseWidth;        // MOPA激光器脉宽值(ns)
    
    // 默认构造函数
    MarkParameter() 
        : MarkSpeed(1000)
        , JumpSpeed(5000)
        , MarkDelay(100)
        , JumpDelay(100)
        , PolygonDelay(50)
        , MarkCount(1)
        , LaserOnDelay(50.0f)
        , LaserOffDelay(50.0f)
        , FPKDelay(0.0f)
        , FPKLength(0.0f)
        , QDelay(0.0f)
        , DutyCycle(0.5f)
        , Frequency(20.0f)
        , StandbyFrequency(20.0f)
        , StandbyDutyCycle(0.5f)
        , LaserPower(50.0f)
        , AnalogMode(0)
        , Waveform(0)
        , PulseWidthMode(0)
        , PulseWidth(0)
    {}
};

// ========== 打标位置点结构体 ==========
struct structUdmPos {
    float x;    // x坐标
    float y;    // y坐标
    float z;    // z坐标
    float a;    // 预留
    
    // 默认构造函数
    structUdmPos() : x(0.0f), y(0.0f), z(0.0f), a(0.0f) {}
    
    // 带参数构造函数
    structUdmPos(float _x, float _y, float _z = 0.0f, float _a = 0.0f)
        : x(_x), y(_y), z(_z), a(_a) {}
};

// ========== IO端口定义 ==========
// 输入端口 IN0~IN14
#define IO_IN0      0
#define IO_IN1      1
#define IO_IN2      2
#define IO_IN3      3
#define IO_IN4      4
#define IO_IN5      5
#define IO_IN6      6
#define IO_IN7      7
#define IO_IN8      8
#define IO_IN9      9
#define IO_IN10     10
#define IO_IN11     11
#define IO_IN12     12
#define IO_IN13     13
#define IO_IN14     14

// 输出端口 OUT0~OUT15
#define IO_OUT0     0
#define IO_OUT1     1
#define IO_OUT2     2
#define IO_OUT3     3
#define IO_OUT4     4
#define IO_OUT5     5
#define IO_OUT6     6
#define IO_OUT7     7
#define IO_OUT8     8
#define IO_OUT9     9
#define IO_OUT10    10
#define IO_OUT11    11
#define IO_OUT12    12
#define IO_OUT13    13
#define IO_OUT14    14
#define IO_OUT15    15

// ========== 激光器控制引脚定义 ==========
// J11端口引脚定义
#define LASER_D0_D7         // 8位数字量输出(能量控制)
#define LASER_HLATCH    9   // 能量锁存信号
#define LASER_PWM_EN    10  // 脉宽使能信号
#define LASER_ALARM1    11  // 激光器报警1
#define LASER_ALARM2    12  // 激光器报警2
#define LASER_FPK       13  // 首脉冲抑制信号
#define LASER_GND       14  // 参考地
#define LASER_GND2      15  // 参考地
#define LASER_ALARM3    16  // 激光器报警3
#define LASER_5V        17  // +5V电压输出
#define LASER_ENABLE    18  // 激光使能输出
#define LASER_ON        19  // 出激光信号
#define LASER_Q         20  // 频率输出
#define LASER_ALARM4    21  // 激光器报警4
#define LASER_RED       22  // 红光输出/脉宽使能
#define LASER_5V_2      23  // +5V高电平输出
#define LASER_VOUTA     24  // 模拟量输出A(0~10V)
#define LASER_VOUTB     25  // 模拟量输出B(0~10V)

// ========== 默认IP配置 ==========
#define DEFAULT_IP          "172.18.34.227"     // 控制卡默认IP
#define DEFAULT_SUBNET      "255.255.255.0"     // 默认子网掩码
#define IP_RANGE_START      "172.18.34.2"       // IP范围起始
#define IP_RANGE_END        "172.18.34.123"     // IP范围结束
#define INIT_IP             "172.18.34.226"     // 初始化临时IP

// ========== 打标范围限制 ==========
#define Z_AXIS_MIN          -4.0f               // Z轴最小值(mm)
#define Z_AXIS_MAX          4.0f                // Z轴最大值(mm)
#define MAX_MARK_REGION     300                 // 最大打标范围(mm)

// ========== 文件大小限制 ==========
#define MAX_SINGLE_FILE_SIZE    (2 * 1024 * 1024)  // 单文档脱机最大2MB
#define MAX_MULTI_FILE_COUNT    16                  // 多文档脱机最多16个文件

#endif // GALVO_PROTOCOL_H

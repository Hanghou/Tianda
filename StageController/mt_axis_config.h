#ifndef MT_AXIS_CONFIG_H
#define MT_AXIS_CONFIG_H

#include <cmath>

/**
 * @brief MT_API 运动控制卡双轴位移台硬件参数配置
 *
 * 业务用途：集中管理“微米 ↔ 脉冲”的换算、默认速度/加减速、回零速度、软件限位。
 * 当前依据规格书和演示软件 sys.ini：脉冲当量 = 0.1 μm/pulse，即 10 pulse/μm。
 * 后续若更换丝杆/细分/减速比，只需优先校准本文件中的 PULSES_PER_UM。
 */
namespace MtAxisConfig {

// ========== 轴号配置 ==========
constexpr unsigned short AXIS_1 = 0;       // 控制卡第1轴，对应原 Stage1
constexpr unsigned short AXIS_2 = 1;       // 控制卡第2轴，对应原 Stage2
constexpr int REQUIRED_AXIS_COUNT = 2;     // 本项目双位移台至少需要2轴

// ========== 单位换算配置 ==========
constexpr double PULSES_PER_UM = 10.0;     // 脉冲/微米：速度 μm/s → pulse/s、位置 μm → pulse
constexpr double UM_PER_PULSE  = 0.1;      // 微米/脉冲：位置 pulse → μm

// ========== 默认运动参数（单位均为脉冲制） ==========
constexpr int DEFAULT_SPEED_PPS = 10000;   // 默认定位速度：pulse/s
constexpr int DEFAULT_ACC_PPS2  = 100000;  // 默认加速度：pulse/s²
constexpr int DEFAULT_DEC_PPS2  = 100000;  // 默认减速度：pulse/s²
constexpr int HOME_SPEED_PPS    = 2000;    // 回零速度：pulse/s
constexpr int HOME_DIRECTION    = 1;       // 回零方向：1=正向，-1=负向（如方向相反仅改此项）

// ========== 软件限位（留足余量，防止无边界目标值误下发） ==========
constexpr int SOFT_LIMIT_NEG = -99999999;  // 负向软限位，单位 pulse
constexpr int SOFT_LIMIT_POS =  99999999;  // 正向软限位，单位 pulse

// ========== 状态轮询配置 ==========
constexpr int POLL_INTERVAL_MS = 100;      // 运动状态/位置轮询间隔：ms

/**
 * @brief 微米转换为脉冲
 * @param um 目标位移或速度，单位 μm 或 μm/s
 * @return 四舍五入后的脉冲值，单位 pulse 或 pulse/s
 */
inline int umToPulses(double um)
{
    return static_cast<int>(std::lround(um * PULSES_PER_UM));
}

/**
 * @brief 脉冲转换为微米
 * @param pulses 当前位置脉冲数
 * @return 微米值
 */
inline double pulsesToUm(int pulses)
{
    return static_cast<double>(pulses) * UM_PER_PULSE;
}

/**
 * @brief 判断目标脉冲是否位于软件限位范围内
 * @param pulses 目标脉冲位置
 * @return 在范围内返回 true，越界返回 false
 */
inline bool isWithinSoftLimit(int pulses)
{
    return pulses >= SOFT_LIMIT_NEG && pulses <= SOFT_LIMIT_POS;
}

} // namespace MtAxisConfig

#endif // MT_AXIS_CONFIG_H

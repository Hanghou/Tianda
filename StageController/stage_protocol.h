#ifndef STAGE_PROTOCOL_H
#define STAGE_PROTOCOL_H

#include <QString>
#include <QByteArray>
#include <cmath>

/**
 * @brief Thorlabs ELLx 位移台通信协议定义
 *
 * 指令格式：ASCII 文本，末尾 CRLF（\r\n）
 * 格式：<地址><命令>[参数]\r\n
 *   地址：0 = 台1，1 = 台2（单字符十六进制）
 *
 * 支持指令：
 *   0in / 1in  读取换算参数（返回 IN 帧，含 pulsePerUnit）
 *   sv<hex8>   设置最大速度（32位有符号大端十六进制，8字符）
 *   ma<hex8>   绝对位移（32位有符号大端十六进制脉冲数，8字符）
 *   gs         查询状态（返回 GS 帧）
 *   gp         查询当前位置（返回 PO 帧）
 *   st         停止
 *   ho<dir>    回零（dir: 0=顺时针，1=逆时针）
 *
 * 换算规则：
 *   位移量(mm) × pulsePerUnit → qint32 脉冲数
 *   脉冲数 → 8字符大端有符号十六进制字符串（负数用补码）
 *
 * 示例：
 *   读台1参数：  "0in\r\n"
 *   设台1速度：  "0sv00100000\r\n"
 *   台1绝对移动："0ma00200000\r\n"
 *   查台2状态：  "1gs\r\n"
 */

// 命令字符串
const QString STAGE_CMD_GET_INFO  = "in";   // 读取换算参数
const QString STAGE_CMD_SET_SPEED = "sv";   // 设置最大速度
const QString STAGE_CMD_MOVE_ABS  = "ma";   // 绝对位移
const QString STAGE_CMD_GET_STATUS= "gs";   // 查询状态
const QString STAGE_CMD_GET_POS   = "gp";   // 查询当前位置
const QString STAGE_CMD_STOP      = "st";   // 停止
const QString STAGE_CMD_HOME      = "ho";   // 回零

// 响应命令类型（响应帧第1-2字符）
const QString STAGE_RESP_INFO     = "IN";   // 换算参数响应
const QString STAGE_RESP_STATUS   = "GS";   // 状态响应
const QString STAGE_RESP_POSITION = "PO";   // 位置响应

/**
 * @brief ELLx 设备换算参数（由 0in/1in 响应解析）
 */
struct EllxDeviceInfo {
    double pulsePerUnit;    // 脉冲数/mm（由 IN 响应中 pulse_per_unit 字段解析）
    bool   valid;           // 是否已成功读取

    EllxDeviceInfo() : pulsePerUnit(2048.0), valid(false) {}
};

/**
 * @brief 位移台状态结构
 */
struct StageStatus {
    qint32 positionPulses;  // 当前位置（脉冲数）
    bool   isMoving;
    bool   isHomed;

    StageStatus() : positionPulses(0), isMoving(false), isHomed(false) {}
};

/**
 * @brief 构建 ELLx ASCII 指令帧（含 CRLF）
 * @param address 设备地址字符（'0' 或 '1'）
 * @param cmd     命令字符串（如 "ma"）
 * @param param   参数字符串（可为空）
 */
inline QByteArray ellxBuildFrame(char address, const QString &cmd, const QString &param = "")
{
    QString frame = QString(address) + cmd + param + "\r\n";
    return frame.toLatin1();
}

/**
 * @brief 将 32 位有符号脉冲数转为 8 字符大端十六进制字符串（补码）
 */
inline QString ellxPulsesToHex(qint32 pulses)
{
    // 用 quint32 保证补码表示
    return QString("%1").arg(static_cast<quint32>(pulses), 8, 16, QChar('0')).toUpper();
}

/**
 * @brief 将位移量(mm)按换算参数转为脉冲数（对负数也正确四舍五入）
 */
inline qint32 ellxMmToPulses(double mm, double pulsePerUnit)
{
    return static_cast<qint32>(std::lround(mm * pulsePerUnit));
}

#endif // STAGE_PROTOCOL_H

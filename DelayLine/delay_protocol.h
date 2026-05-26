#ifndef DELAY_PROTOCOL_H
#define DELAY_PROTOCOL_H

#include <QtGlobal>
#include <QByteArray>

/**
 * @brief 延时线通信协议定义
 * 二进制协议
 * 协议格式: FC + ID + FUNC + P1 + P2 + P3 + FE （共7字节）
 *
 * 支持指令（按用户文档）：
 *   设置时间延迟  功能码 0x04：FC ID 04 XX XX XX FE
 *   查询当前位置  功能码 0x0E：FC ID 0E 00 00 00 FE
 *   归零          功能码 0x07：FC ID 07 00 00 00 FE
 *   停止运动      功能码 0x0B：FC ID 0B 00 00 00 FE
 *
 * 数值换算：目标延迟（PS） × 1000 → 3字节大端十六进制
 * 例：100 PS → 100000 → 0x0186A0 → [0x01, 0x86, 0xA0]
 */

// 帧标识
const quint8 DELAY_FRAME_START = 0xFC;  // 起始码
const quint8 DELAY_FRAME_END   = 0xFE;  // 结束码

// 功能码定义（严格按用户文档协议）
const quint8 DELAY_CMD_SET_DELAY   = 0x04;  // 设置时间延迟（绝对位置，PS×1000→3字节）
const quint8 DELAY_CMD_HOME        = 0x07;  // 归零
const quint8 DELAY_CMD_STOP        = 0x0B;  // 停止运动
const quint8 DELAY_CMD_QUERY_POS   = 0x0E;  // 查询当前位置

// 响应标识
const quint8 DELAY_RESPONSE_POS = 0xAA;     // 位置查询响应

/**
 * @brief 延时线状态结构
 */
struct DelayLineStatus {
    float currentDelay;     // 当前延迟值(PS)
    bool isMoving;          // 是否在运动
    bool isHomed;           // 是否已归零
    quint8 errorCode;       // 错误码
    
    DelayLineStatus() 
        : currentDelay(0.0f)
        , isMoving(false)
        , isHomed(false)
        , errorCode(0)
    {}
};

/**
 * @brief 延迟值（PS）转换为3字节大端数据
 * @param delayPS 延迟值（PS）
 * @return 3字节数据（高位、中位、低位）
 *
 * 换算规则：延迟值（PS） × 1000 → quint32 → 3字节大端十六进制
 * 例：100 PS → 100×1000=100000 → 0x0186A0 → [0x01, 0x86, 0xA0]
 */
inline QByteArray delayToBytes(float delayPS) {
    // 按协议：PS × 1000，取整，转3字节大端
    quint32 value = static_cast<quint32>(delayPS * 1000.0f + 0.5f);  // 四舍五入

    QByteArray data;
    data.append(static_cast<char>((value >> 16) & 0xFF));  // 高位
    data.append(static_cast<char>((value >> 8)  & 0xFF));  // 中位
    data.append(static_cast<char>(value         & 0xFF));  // 低位

    return data;
}

/**
 * @brief 3字节大端数据转换为延迟值（PS）
 * @param data 3字节数据
 * @return 延迟值（PS）
 */
inline float bytesToDelay(const QByteArray &data) {
    if (data.size() < 3) {
        return 0.0f;
    }

    quint32 value = (static_cast<quint32>(static_cast<quint8>(data[0])) << 16) |
                    (static_cast<quint32>(static_cast<quint8>(data[1])) << 8)  |
                     static_cast<quint32>(static_cast<quint8>(data[2]));

    return value / 1000.0f;  // 转回 PS
}

#endif // DELAY_PROTOCOL_H

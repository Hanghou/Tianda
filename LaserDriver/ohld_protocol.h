#ifndef OHLD_PROTOCOL_H
#define OHLD_PROTOCOL_H

#include <QtGlobal>
#include <QByteArray>

/**
 * @brief OHLD 激光器（泵）通信协议
 *
 * 帧格式：68 + 设备ID(4字节,固定00000000H) + 控制字(1字节) + 数据长度(2字节大端)
 *         + 数据域(N字节) + 校验码(1字节) + 16
 *
 * 校验码：对 设备ID ~ 数据域 所有字节累加，取低8位后按位取反
 *
 * 串口参数：9600 / 8 / 1 / 无校验 / 无流控
 */

// 帧边界
const quint8 OHLD_FRAME_START = 0x68;
const quint8 OHLD_FRAME_END   = 0x16;

// 控制字
const quint8 OHLD_CMD_QUERY   = 0x01;  // 查询状态/电流/温度
const quint8 OHLD_CMD_SET_CUR = 0x02;  // 设置驱动电流（电流值×10，4字节大端）
const quint8 OHLD_CMD_OPEN    = 0x05;  // 开启激光器
const quint8 OHLD_CMD_CLOSE   = 0x06;  // 关闭激光器

/**
 * @brief OHLD 泵状态（由 0x01 查询响应解析）
 */
struct OhldPumpStatus {
    bool    isRunning;       // 启停状态
    float   driveCurrent;    // 驱动电流（mA）
    float   maxCurrent;      // 最大电流（mA）
    float   temperature;     // 温度（℃）
    float   backCurrent;     // 背光电流（mA）

    OhldPumpStatus()
        : isRunning(false), driveCurrent(0), maxCurrent(0)
        , temperature(0), backCurrent(0) {}
};

/**
 * @brief 构建 OHLD 帧
 * @param cmd      控制字
 * @param data     数据域（可为空）
 * @return 完整帧字节数组
 */
inline QByteArray ohldBuildFrame(quint8 cmd, const QByteArray &data = QByteArray())
{
    QByteArray frame;
    frame.append(static_cast<char>(OHLD_FRAME_START));

    // 设备ID：4字节全0
    frame.append(4, 0x00);

    // 控制字
    frame.append(static_cast<char>(cmd));

    // 数据长度：2字节大端
    quint16 len = static_cast<quint16>(data.size());
    frame.append(static_cast<char>((len >> 8) & 0xFF));
    frame.append(static_cast<char>(len & 0xFF));

    // 数据域
    frame.append(data);

    // 校验码：设备ID(4) + 控制字(1) + 数据长度(2) + 数据域 累加，取低8位取反
    quint8 sum = 0;
    for (int i = 1; i < frame.size(); ++i) {  // 从设备ID开始（跳过0x68）
        sum += static_cast<quint8>(frame.at(i));
    }
    frame.append(static_cast<char>(~sum & 0xFF));

    frame.append(static_cast<char>(OHLD_FRAME_END));
    return frame;
}

/**
 * @brief 构建设置电流帧（电流值×10，4字节大端）
 * @param currentMA 电流值（mA）
 */
inline QByteArray ohldBuildSetCurrentFrame(float currentMA)
{
    quint32 val = static_cast<quint32>(currentMA * 10.0f + 0.5f);
    QByteArray data;
    data.append(static_cast<char>((val >> 24) & 0xFF));
    data.append(static_cast<char>((val >> 16) & 0xFF));
    data.append(static_cast<char>((val >> 8)  & 0xFF));
    data.append(static_cast<char>(val         & 0xFF));
    return ohldBuildFrame(OHLD_CMD_SET_CUR, data);
}

/**
 * @brief 解析 OHLD 查询响应帧（0x01 回复）
 * @param frame  完整响应帧
 * @param status 输出解析结果
 * @return 解析成功返回 true
 *
 * 响应数据域布局（参考 OHLD 文档）：
 *   [0]     启停状态（0=关，1=开）
 *   [1-2]   驱动电流（大端，单位0.1mA）
 *   [3-4]   最大电流（大端，单位0.1mA）
 *   [5-6]   温度（大端，单位0.1℃）
 *   [7-8]   背光电流（大端，单位0.1mA）
 */
inline bool ohldParseQueryResponse(const QByteArray &frame, OhldPumpStatus &status)
{
    // 最小帧长：1(68)+4(ID)+1(cmd)+2(len)+9(data)+1(chk)+1(16) = 19
    if (frame.size() < 19) return false;
    if (static_cast<quint8>(frame.at(0)) != OHLD_FRAME_START) return false;
    if (static_cast<quint8>(frame.at(frame.size()-1)) != OHLD_FRAME_END) return false;

    // 数据域起始偏移 = 1(68)+4(ID)+1(cmd)+2(len) = 8
    const int dataOffset = 8;
    if (frame.size() < dataOffset + 9 + 2) return false;

    auto u8  = [&](int i) { return static_cast<quint8>(frame.at(i)); };
    auto u16 = [&](int i) { return static_cast<quint16>((u8(i) << 8) | u8(i+1)); };

    status.isRunning    = (u8(dataOffset) != 0);
    status.driveCurrent = u16(dataOffset + 1) / 10.0f;
    status.maxCurrent   = u16(dataOffset + 3) / 10.0f;
    status.temperature  = u16(dataOffset + 5) / 10.0f;
    status.backCurrent  = u16(dataOffset + 7) / 10.0f;
    return true;
}

#endif // OHLD_PROTOCOL_H

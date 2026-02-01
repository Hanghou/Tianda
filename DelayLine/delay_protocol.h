#ifndef DELAY_PROTOCOL_H
#define DELAY_PROTOCOL_H

#include <QtGlobal>
#include <QByteArray>

/**
 * @brief 延时线通信协议定义
 * 四川梓冠电动光纤延时线
 * 协议格式: FC + ID + CMD + DATA(3字节) + FE
 */

// 帧标识
const quint8 DELAY_FRAME_START = 0xFC;  // 起始码
const quint8 DELAY_FRAME_END = 0xFE;    // 结束码

// 命令码定义
const quint8 DELAY_CMD_SET_BAUDRATE = 0x01;      // 设置波特率
const quint8 DELAY_CMD_SET_SPEED = 0x02;         // 设置速度
const quint8 DELAY_CMD_SET_MAX_RANGE = 0x03;     // 设置最大量程
const quint8 DELAY_CMD_SET_DELAY = 0x04;         // 设置延迟值（绝对位置）
const quint8 DELAY_CMD_INCREASE = 0x05;          // 增加延迟
const quint8 DELAY_CMD_DECREASE = 0x06;          // 减小延迟
const quint8 DELAY_CMD_HOME = 0x07;              // 归零
const quint8 DELAY_CMD_LOOP_START = 0x08;        // 循环起点
const quint8 DELAY_CMD_LOOP_END = 0x09;          // 循环终点
const quint8 DELAY_CMD_LOOP_DELAY = 0x0A;        // 循环停留时间
const quint8 DELAY_CMD_STOP = 0x0B;              // 停止
const quint8 DELAY_CMD_QUERY_POS = 0x0E;         // 查询当前位置
const quint8 DELAY_CMD_SAVE = 0x0F;              // 保存到EEPROM
const quint8 DELAY_CMD_REALTIME_SAVE = 0x34;     // 实时位置保存开关
const quint8 DELAY_CMD_SET_ID = 0x38;            // 设置ID号
const quint8 DELAY_CMD_REALTIME_DATA = 0x39;     // 实时位置数据开关
const quint8 DELAY_CMD_FORMAT = 0x3A;            // 格式化
const quint8 DELAY_CMD_SAVE_ALT = 0x3C;          // 保存（备用命令）

// 响应标识
const quint8 DELAY_RESPONSE_POS = 0xAA;          // 位置查询响应

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
 * @brief 延迟值转换为3字节数据
 * @param delayPS 延迟值（PS，可以有小数）
 * @return 3字节数据（高位、中位、低位）
 * 
 * 转换规则：延迟值 × 1000 转为整数，然后转为3字节十六进制
 * 例如：100.5 PS → 100500 → 0x018894 → [0x01, 0x88, 0x94]
 */
inline QByteArray delayToBytes(float delayPS) {
    // 延迟值 × 1000，转为整数
    quint32 value = static_cast<quint32>(delayPS * 1000.0f);
    
    QByteArray data;
    data.append((value >> 16) & 0xFF);  // 高位
    data.append((value >> 8) & 0xFF);   // 中位
    data.append(value & 0xFF);          // 低位
    
    return data;
}

/**
 * @brief 3字节数据转换为延迟值
 * @param data 3字节数据
 * @return 延迟值（PS）
 */
inline float bytesToDelay(const QByteArray &data) {
    if (data.size() < 3) {
        return 0.0f;
    }
    
    quint32 value = ((quint8)data[0] << 16) | 
                    ((quint8)data[1] << 8) | 
                    (quint8)data[2];
    
    return value / 1000.0f;  // 转回PS
}

#endif // DELAY_PROTOCOL_H

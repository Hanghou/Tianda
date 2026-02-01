#ifndef LASER_PROTOCOL_H
#define LASER_PROTOCOL_H

#include <QtGlobal>

/**
 * @brief 激光器通信协议定义
 * 协议格式: 68H + 设备ID + 控制字 + 数据长度 + 数据域 + 校验码 + 16H
 */

// 帧标识
const quint8 LASER_FRAME_START = 0x68;  // 起始码
const quint8 LASER_FRAME_END = 0x16;    // 结束码

// 控制字定义
const quint8 LASER_CMD_QUERY_STATUS = 0x01;     // 查询状态
const quint8 LASER_CMD_TURN_ON = 0x02;          // 开启
const quint8 LASER_CMD_TURN_OFF = 0x03;         // 关闭
const quint8 LASER_CMD_SET_CURRENT = 0x04;      // 设置电流
const quint8 LASER_CMD_SET_TEMPERATURE = 0x05;  // 设置温度

/**
 * @brief 激光器状态结构
 */
struct LaserStatus {
    bool isOn;              // 是否开启
    float current;          // 当前电流(mA)
    float temperature;      // 当前温度(℃)
    quint8 errorCode;       // 错误码
    
    LaserStatus() 
        : isOn(false)
        , current(0.0f)
        , temperature(0.0f)
        , errorCode(0)
    {}
};

#endif // LASER_PROTOCOL_H

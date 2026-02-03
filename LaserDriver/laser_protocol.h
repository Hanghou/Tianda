#ifndef LASER_PROTOCOL_H
#define LASER_PROTOCOL_H

#include <QtGlobal>

/**
 * @brief 激光器通信协议定义 - OHLD-1000
 * 协议格式: 68H + 4字节设备ID + 控制字 + 2字节数据长度 + 数据域 + 校验码 + 16H
 * 
 * 串口设置：
 * - 波特率：9600
 * - 数据位：8
 * - 停止位：1
 * - 校验位：None
 * - 流控：None
 */

// 帧标识
const quint8 LASER_FRAME_START = 0x68;  // 起始码
const quint8 LASER_FRAME_END = 0x16;    // 结束码

// 设备ID（4字节，全为0）
const quint32 LASER_DEVICE_ID = 0x00000000;

// 控制字定义（根据 OHLD 协议）
const quint8 LASER_CMD_QUERY_STATUS = 0x01;     // 查询状态
const quint8 LASER_CMD_SET_CURRENT = 0x02;      // 设置电流
const quint8 LASER_CMD_SET_MAX_CURRENT = 0x03;  // 设置电流最大值
const quint8 LASER_CMD_SET_TEMPERATURE = 0x04;  // 设置温度
const quint8 LASER_CMD_TURN_ON = 0x05;          // 开启激光器
const quint8 LASER_CMD_TURN_OFF = 0x06;         // 关闭激光器
const quint8 LASER_CMD_SET_POWER_ON_STATE = 0x09; // 设置上电状态

/**
 * @brief 激光器状态结构（根据查询命令的回复）
 */
struct LaserStatus {
    bool targetDetected;        // 目标检测（第1字节）
    bool targetTemperatureOk;   // 目标温度状态（第2字节）
    bool isRunning;             // 运行状态（第3字节）
    bool ambientTemperatureOk;  // 环境温度状态（第4字节）
    quint8 currentUnit;         // 背光电流单位（第5字节：0=nA, 1=mA, 2=uA）
    bool currentClear;          // 背光电流清空状态（第6字节）
    float setCurrent;           // 驱动电流设定值（第7-8字节，实际值*10）
    quint16 maxCurrent;         // 最大电流值（第9-10字节）
    float setTemperature;       // 温度设定值（第11-14字节，浮点数）
    float backgroundCurrent;    // 背光电流（第15-18字节，浮点数）
    
    LaserStatus() 
        : targetDetected(false)
        , targetTemperatureOk(false)
        , isRunning(false)
        , ambientTemperatureOk(true)
        , currentUnit(1)  // 默认 mA
        , currentClear(false)
        , setCurrent(0.0f)
        , maxCurrent(0)
        , setTemperature(25.0f)
        , backgroundCurrent(0.0f)
    {}
};

#endif // LASER_PROTOCOL_H

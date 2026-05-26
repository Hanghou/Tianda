#ifndef LASER_PROTOCOL_H
#define LASER_PROTOCOL_H

#include <QtGlobal>

/**
 * @brief 激光器和放大器通信协议定义
 * 协议格式: 0xAA 0x55 + 命令字 + 数据长度 + 数据 + 校验和(2字节)
 * 
 * 串口设置：
 * - 波特率：9600
 * - 数据位：8
 * - 停止位：1
 * - 校验位：None
 * - 流控：None
 */

// 帧标识
const quint8 LASER_FRAME_START_1 = 0xAA;  // 起始码1（上位机下发）
const quint8 LASER_FRAME_START_2 = 0x55;  // 起始码2（上位机下发）
const quint8 LASER_FRAME_REPLY_1 = 0x55;  // 应答起始码1（下位机上传）
const quint8 LASER_FRAME_REPLY_2 = 0xAA;  // 应答起始码2（下位机上传）

// 命令字定义
const quint8 LASER_CMD_READ_BASIC_INFO = 0xD1;      // 读取基本信息
const quint8 LASER_CMD_READ_FIRMWARE_INFO = 0xD2;   // 读取固件实时信息的含义
const quint8 LASER_CMD_READ_REALTIME_INFO = 0xD3;   // 读取实时信息
const quint8 LASER_CMD_LIGHT_CONTROL = 0xC1;        // 开启、关闭光源 (1字节: 1=开, 0=关)
const quint8 LASER_CMD_SET_POWER = 0xC3;            // 设置输出功率/电流（取决于功率设置方式，2字节）
const quint8 LASER_CMD_MULTI_POWER = 0xC4;          // 多路功率设置命令
const quint8 LASER_CMD_SET_TEMPERATURE = 0xC7;      // 设置种子激光器温度
const quint8 LASER_CMD_SET_WAVELENGTH = 0xC8;       // 设置波长
const quint8 LASER_CMD_SAVE_FLASH = 0xA1;           // 保存flash命令
const quint8 LASER_CMD_APC = 0xC5;                  // APC使用命令
const quint8 LASER_CMD_FACTORY_RESET = 0xC2;        // 返回出厂设置
const quint8 LASER_CMD_RED_LIGHT = 0xCB;            // 开启关闭红光
const quint8 LASER_CMD_RIN_CONTROL = 0xC6;          // 开启RIN抑制

// 功率设置方式定义（基本信息报文第25字节）
const quint8 POWER_SET_MODE_POWER = 1;              // 1-设置功率
const quint8 POWER_SET_MODE_CURRENT = 2;            // 2-设置电流
const quint8 POWER_SET_MODE_DA = 3;                 // 3-设置DA
const quint8 POWER_SET_MODE_POTENTIOMETER = 4;      // 4-电位器
const quint8 POWER_SET_MODE_FIXED_POWER = 5;        // 5-固定功率
const quint8 POWER_SET_MODE_VOLTAGE = 6;            // 6-通过电压设置
const quint8 POWER_SET_MODE_PUMP_POWER = 7;         // 7-设置泵浦功率
const quint8 POWER_SET_MODE_PEAK_POWER = 8;         // 8-设置峰值功率
const quint8 POWER_SET_MODE_GAIN = 9;               // 9-设置增益
const quint8 POWER_SET_MODE_MONITOR_POWER = 10;     // 10-设置监测功率

// 命令执行说明（应答报文数据字段）
const quint8 EXEC_SUCCESS = 0x00;           // 设置成功
const quint8 EXEC_BELOW_MIN = 0x01;         // 超过设置最小值
const quint8 EXEC_ABOVE_MAX = 0x02;         // 超过设置最大值
const quint8 EXEC_INVALID_STATE = 0x04;     // 设备当前状态无法执行
const quint8 EXEC_NO_FUNCTION = 0x08;       // 设备无此功能
const quint8 EXEC_SAVE_ERROR = 0x10;        // 保存数据出错

/**
 * @brief 激光器基本信息结构（0xD1命令回复）
 */
struct LaserBasicInfo {
    quint8 powerSetMode;        // 功率设置方式（第25字节：1=设置功率，2=设置电流）
    quint16 minCurrent;         // 最小电流值（mA）
    quint16 maxCurrent;         // 最大电流值（mA）
    
    LaserBasicInfo()
        : powerSetMode(0)
        , minCurrent(0)
        , maxCurrent(0)
    {}
};

/**
 * @brief 激光器状态结构
 */
struct LaserStatus {
    bool isRunning;             // 运行状态
    quint16 setCurrent;         // 设置的电流值（mA）
    quint16 actualCurrent;      // 实际电流值（mA）
    quint8 powerSetMode;        // 功率设置方式（1=功率，2=电流）
    
    LaserStatus() 
        : isRunning(false)
        , setCurrent(0)
        , actualCurrent(0)
        , powerSetMode(0)
    {}
};

#endif // LASER_PROTOCOL_H

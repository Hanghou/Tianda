#ifndef STAGE_PROTOCOL_H
#define STAGE_PROTOCOL_H

#include <QString>

/**
 * @brief 位移台通信协议定义
 * 协议格式: ASCII命令格式 (地址 + 命令 + 参数)
 * 示例: "1ho" (地址1, home命令)
 */

// 常用命令
const QString STAGE_CMD_HOME = "ho";        // 归零
const QString STAGE_CMD_MOVE_ABS = "pa";    // 绝对移动
const QString STAGE_CMD_MOVE_REL = "pr";    // 相对移动
const QString STAGE_CMD_STOP = "st";        // 停止
const QString STAGE_CMD_GET_POS = "tp";     // 获取位置

/**
 * @brief 位移台状态结构
 */
struct StageStatus {
    float position;         // 当前位置(mm)
    bool isMoving;          // 是否在运动
    bool isHomed;           // 是否已归零
    quint8 errorCode;       // 错误码
    
    StageStatus()
        : position(0.0f)
        , isMoving(false)
        , isHomed(false)
        , errorCode(0)
    {}
};

#endif // STAGE_PROTOCOL_H

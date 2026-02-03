# 位移台控制模块

## 功能概述
控制旋转台和直线台，实现精确的位置控制。

## 通信协议
- 串口通信
- 波特率：9600 bps
- 数据位：8
- 停止位：1
- 校验位：无

## 主要功能
- ✅ 旋转台角度控制
- ✅ 直线台位置控制
- ✅ 位置反馈
- ✅ 移动状态监控
- ✅ 实时位置更新

## 文件说明
- `stage_controller.h/cpp` - 位移台控制类实现
- `stage_protocol.h` - 协议定义

## 使用示例

### 连接位移台
```cpp
StageController *stage = new StageController();
stage->openPort("COM4", 9600);
```

### 设置旋转台角度
```cpp
stage->setRotationAngle(45.0);  // 45°
```

### 设置直线台位置
```cpp
stage->setLinearPosition(100.0);  // 100 mm
```

### 监听位置变化
```cpp
connect(stage, &StageController::positionChanged,
        this, &Integration::onStagePositionChanged);
```

### 监听移动完成
```cpp
connect(stage, &StageController::moveCompleted,
        this, &Integration::onStageMoveCompleted);
```

## 控制精度
- 旋转台：0.01°
- 直线台：0.01 mm

## 参考文档
- 协议定义：`stage_protocol.h`

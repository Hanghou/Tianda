# 激光器驱动模块

## 功能概述
控制三个独立的激光器：
- **种子源激光器**（Seed Source）- 单位：mA
- **FOPO激光器** - 单位：A
- **Stokes激光器** - 单位：mA

## 通信协议
**OHLD-1000 激光器驱动控制协议**
- 串口通信：9600 bps, 8N1
- 帧格式：`68H + 4字节设备ID + 控制字 + 2字节数据长度 + 数据域 + 校验码 + 16H`
- 校验算法：累加和取反

## 主要功能
- ✅ 开启/关闭激光器
- ✅ 设置电流（实际值×10）
- ✅ 设置最大电流
- ✅ 设置温度（IEEE 754浮点数）
- ✅ 查询状态（18字节状态数据）

## 文件说明
- `laser_driver.h/cpp` - 激光器驱动类实现
- `laser_protocol.h` - 协议定义和帧结构
- `LASER_TEST_GUIDE.md` - 测试指南
- `LASER_CONTROL_ARCHITECTURE.md` - 架构说明
- `OHLD激光器驱动器控制协议兼容版本（等长）.txt` - 官方协议文档

## 使用示例

### 连接激光器
```cpp
LaserDriver *seedLaser = new LaserDriver(LaserType::SeedSource);
seedLaser->openPort("COM3", 9600);
```

### 设置电流
```cpp
// 种子源：300 mA
seedLaser->setCurrent(300.0f);

// FOPO：3 A
fopoLaser->setCurrent(3.0f);
```

### 开启/关闭
```cpp
seedLaser->turnOn();   // 开启
seedLaser->turnOff();  // 关闭
```

### 查询状态
```cpp
seedLaser->queryStatus();
LaserStatus status = seedLaser->getStatus();
```

## 重要提示
⚠️ **三个激光器完全独立**
- 每个激光器有独立的 `LaserDriver` 实例
- 每个激光器连接到不同的串口
- 命令不会混淆或串扰

⚠️ **电流单位不同**
- 种子源和 Stokes：mA（毫安）
- FOPO：A（安培）

## 协议验证
所有命令已通过官方协议文档验证：
- ✅ 开启命令：`68 00 00 00 00 05 00 04 00 00 00 00 F6 16`
- ✅ 关闭命令：`68 00 00 00 00 06 00 04 00 00 00 00 F5 16`
- ✅ 设置电流（300mA）：`68 00 00 00 00 02 00 04 0B B8 00 00 36 16`
- ✅ 查询状态：`68 00 00 00 00 01 00 04 00 00 00 00 FA 16`

## 参考文档
- [测试指南](LASER_TEST_GUIDE.md)
- [架构说明](LASER_CONTROL_ARCHITECTURE.md)
- [协议文档](OHLD激光器驱动器控制协议兼容版本（等长）.txt)

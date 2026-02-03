# 光谱仪模块

## 功能概述
控制 OPTOSKY ATP 系列光纤光谱仪，实现光谱数据采集和分析。

## 通信协议
**OPTOSKY ATP 系列通讯协议 V2.3**
- USB 通信（虚拟串口）
- 波长范围：200-1100 nm
- 像素数：2048/3648（取决于型号）
- 积分时间：1-65535 ms

## 主要功能
- ✅ 单次测量（命令 0x1E - 同步方式）
- ✅ 持续测量（QTimer + 单次测量）
- ✅ 积分时间控制
- ✅ 波长标定系数获取
- ✅ 设备信息查询
- ✅ 峰值检测
- ✅ 数据导出（CSV）
- ✅ 图表显示和交互

## 文件说明
- `spectrometer.h/cpp` - 光谱仪驱动类实现
- `spectrometer_protocol.h` - 协议定义
- `Driver.dll/lib` - 官方驱动库
- `DriverType.h` - 驱动类型定义
- `Driver_app.h` - 驱动应用接口
- `SPECTROMETER_TEST_GUIDE.md` - 测试指南
- `SPECTROMETER_IMPLEMENTATION_STATUS.md` - 实现状态
- `OPTOSKY_ATP系列光纤光谱仪_通讯协议_中文版_V2.3_20240124.txt` - 官方协议文档

## 使用示例

### 连接光谱仪
```cpp
Spectrometer *spectrometer = new Spectrometer();
spectrometer->openPort("COM5", 115200);
```

### 设置积分时间
```cpp
spectrometer->setIntegrationTime(100);  // 100 ms
```

### 单次测量
```cpp
spectrometer->startSingleScan();
// 等待 spectrumDataReady 信号
```

### 持续测量
```cpp
spectrometer->startContinuousScan();  // 开始
spectrometer->stopContinuousScan();   // 停止
```

### 获取光谱数据
```cpp
connect(spectrometer, &Spectrometer::spectrumDataReady,
        this, &Integration::onSpectrumDataReady);
```

## 测量模式

### 单次测量
- 使用命令 0x1E（同步方式）
- 适合精确测量
- 手动触发

### 持续测量
- 使用 QTimer 定时触发单次测量
- 采集间隔 = 积分时间 + 100ms
- 采集频率约 5-10 Hz
- 实时显示

## 性能指标
- 采集速度：5-10 Hz（取决于积分时间）
- 波长精度：±0.5 nm
- 动态范围：16 bit
- 信噪比：>250:1

## 参考文档
- [测试指南](SPECTROMETER_TEST_GUIDE.md)
- [实现状态](SPECTROMETER_IMPLEMENTATION_STATUS.md)
- [协议文档](OPTOSKY_ATP系列光纤光谱仪_通讯协议_中文版_V2.3_20240124.txt)

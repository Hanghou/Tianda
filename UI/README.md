# UI 模块

## 功能概述
提供图形用户界面，整合所有设备模块的控制和显示功能。

## 文件说明
- `integration.h/cpp/ui` - 主窗口类

## 主要功能

### Integration（主窗口）
- ✅ 设备连接管理（激光器、光谱仪、振镜、位移台、延迟线）
- ✅ 光谱测量控制（单次/持续）
- ✅ 图表显示
- ✅ 峰值检测和分析
- ✅ 预设管理（功率预设、延迟预设）
- ✅ 数据导出（CSV、图像）
- ✅ 配置保存/加载

## 使用示例

### 主窗口初始化
```cpp
Integration *mainWindow = new Integration();
mainWindow->show();
```

## UI 布局

### 主窗口标签页
1. **光谱仪页** - 光谱测量和图表显示
2. **振镜页** - 振镜控制和预设管理
3. **位移台页** - 位移台控制和预设管理

### 设备连接区域
- 串口配置（端口、波特率、数据位、停止位、校验位）
- 连接/断开按钮
- 状态指示器

### 控制区域
- 参数设置输入框
- 确认按钮
- 实时反馈显示

### 预设管理区域
- 预设表格（可添加、删除、上移、下移）
- 执行控制（开始、停止）
- 时间间隔设置

## 信号槽机制

### 设备状态信号
```cpp
void onSeedLaserStatusChanged(DeviceStatus status);
void onSpectrometerStatusChanged(DeviceStatus status);
// ... 其他设备
```

### 数据接收信号
```cpp
void onSpectrumDataReady(const QVector<int> &intensity);
void onDelayChanged(float delayPS);
void onStagePositionChanged(qint32 positionPulses);
```

### 错误处理信号
```cpp
void onDeviceError(const QString &error);
```

## 参考文档
- Qt Widgets 文档
- Qt Charts 文档
- 项目主 README.md

# Integration - 光学系统集成控制软件

## 项目简介
这是一个用于控制多个光学设备的集成软件，包括激光器、光谱仪、振镜、位移台和延迟线。基于 Qt 框架开发，提供友好的图形界面和强大的数据处理功能。

## 系统架构

### 设备模块
- **LaserDriver** - 激光器驱动（3个独立激光器：种子源、FOPO、Stokes）
- **Spectrometer** - 光谱仪控制（OPTOSKY ATP 系列）
- **GalvoMirror** - 振镜控制（思特 GMC 系列）
- **StageController** - 位移台控制（旋转台 + 直线台）
- **DelayLine** - 延迟线控制

### 核心模块
- **Communication** - 通信基类（串口、设备基类）
- **Utils** - 工具类（配置、数据管理、导出、预设）

### UI 模块
- **integration** - 主窗口（UI/）
- **chart_dialog** - 图表对话框（UI/）
- **custom_chart_view** - 自定义图表视图（UI/）

## 目录结构

```
Integration/
├── Communication/      # 通信基类
│   ├── device_base.*
│   ├── serial_port_base.*
│   ├── error_codes.h
│   └── README.md
├── Utils/              # 工具类
│   ├── config_manager.*
│   ├── data_manager.*
│   ├── csv_exporter.*
│   ├── image_saver.*
│   ├── preset_manager.*
│   ├── constants.h
│   └── README.md
├── LaserDriver/        # 激光器模块
│   ├── laser_driver.*
│   ├── laser_protocol.h
│   └── README.md
├── Spectrometer/       # 光谱仪模块
│   ├── spectrometer.*
│   ├── spectrometer_protocol.h
│   ├── Driver.dll/lib
│   └── README.md
├── GalvoMirror/        # 振镜模块
│   ├── galvo_gmc_controller.*
│   ├── library/        # DLL库文件
│   └── README.md
├── StageController/    # 位移台模块
│   ├── stage_controller.*
│   ├── stage_protocol.h
│   └── README.md
├── DelayLine/          # 延迟线模块
│   ├── delay_line.*
│   ├── delay_protocol.h
│   └── README.md
├── UI/                 # UI 模块
│   ├── integration.*   # 主窗口
│   ├── chart_dialog.*  # 图表对话框
│   ├── custom_chart_view.* # 自定义图表
│   └── README.md
├── docs/               # 项目文档
│   ├── guides/         # 使用指南
│   ├── README.md
│   └── SKILL.md
├── main.cpp            # 程序入口
└── Integration.pro     # Qt 项目文件
```

## 编译要求

### 基本要求
- Qt 6.x
- C++11 或更高
- Windows 10/11

### 编译器选择
- **MinGW 64-bit** - 用于激光器、光谱仪、位移台、延迟线
- **MSVC 2022 64-bit** - 用于振镜控制（必需）

⚠️ **重要提示**：振镜模块使用的 DLL 库只支持 MSVC 编译器，如果需要使用振镜功能，必须使用 MSVC 编译整个项目。

### 依赖库
- Qt Widgets
- Qt SerialPort
- Qt Charts
- Qt Network

## 快速开始

### 1. 克隆项目
```bash
git clone <repository-url>
cd Integration
```

### 2. 打开项目
使用 Qt Creator 打开 `Integration.pro`

### 3. 选择编译器
- 如果不使用振镜：选择 MinGW 64-bit
- 如果使用振镜：选择 MSVC 2022 64-bit

### 4. 编译运行
```
Build → Rebuild All
Build → Run
```

## 功能特性

### 激光器控制
- 三个独立激光器：种子源、FOPO、Stokes
- 电流控制、温度控制
- 开关控制
- 状态查询
- 单位：种子源和 Stokes 为 mA，FOPO 为 A

### 光谱仪控制
- 单次测量
- 持续测量（实时，5-10 Hz）
- 积分时间控制（1-65535 ms）
- 峰值检测和 FWHM 计算
- 数据导出（CSV）
- 图表显示和交互

### 振镜控制
- 角度控制（X/Y 轴）
- 扫描控制
- 导引激光控制
- 实时状态监控

### 位移台控制
- 旋转台角度控制（精度 0.01°）
- 直线台位置控制（精度 0.01 mm）
- 位置反馈
- 移动状态监控

### 延迟线控制
- 延迟时间设置（皮秒级）
- 实时反馈
- 精度：1 ps

### 预设管理
- 光源功率预设（振镜页和位移台页）
- 延迟线预设（振镜页和位移台页）
- 电控平台预设（位移台页）
- 批量执行
- 动态控制

## 文档

### 模块文档
- [激光器模块](LaserDriver/README.md)
- [光谱仪模块](Spectrometer/README.md)
- [振镜模块](GalvoMirror/README.md)
- [位移台模块](StageController/README.md)
- [延迟线模块](DelayLine/README.md)
- [通信基类](Communication/README.md)
- [工具类](Utils/README.md)

### 使用指南
- [振镜启用指南](docs/guides/ENABLE_GALVO_GUIDE.md)
- [振镜功能总结](docs/guides/GALVO_ENABLED_SUMMARY.md)
- [光谱仪完成总结](docs/guides/SPECTROMETER_COMPLETION_SUMMARY.md)
- [Git 推送指南](docs/guides/GIT_PUSH_GUIDE.txt)

### 开发文档
- [项目重构计划](docs/PROJECT_RESTRUCTURE_PLAN.md)
- [快速重构指南](docs/QUICK_RESTRUCTURE_GUIDE.md)
- [技能文档](docs/SKILL.md)

## 配置 MSVC 编译器（振镜功能）

### 1. 安装 Visual Studio
- 下载并安装 Visual Studio 2022
- 选择"使用 C++ 的桌面开发"工作负载

### 2. 配置 Qt Creator
1. 打开 Qt Creator
2. Tools → Options → Kits
3. 添加新的 Kit：
   - Compiler: MSVC 2022 64-bit
   - Qt version: Qt 6.x MSVC 版本
   - Debugger: CDB (Windows Debugger)
4. 应用并关闭

### 3. 切换项目 Kit
1. 在项目视图中选择 MSVC 64-bit Kit
2. 重新编译项目

## 常见问题

### Q: MinGW 编译振镜模块失败？
A: 振镜 DLL 只支持 MSVC 编译器，必须切换到 MSVC Kit。

### Q: 找不到 DLL 文件？
A: 确保 `GalvoMirror/library/` 目录下的 DLL 文件已复制到输出目录。

### Q: 三个激光器会混淆吗？
A: 不会。三个激光器是完全独立的设备，各自有独立的 `LaserDriver` 实例和串口连接。

### Q: 光谱仪持续测量频率？
A: 约 5-10 Hz，取决于积分时间。采集间隔 = 积分时间 + 100ms。

## 许可证
[待定]

## 联系方式
[待定]

## 更新日志

### 2024-02-03
- ✅ 完成激光器驱动实现（OHLD 协议）
- ✅ 完成光谱仪持续测量功能
- ✅ 完成振镜 GMC 控制器集成
- ✅ 完成项目文档结构重构
- ✅ 为所有模块创建 README.md

### 2024-01-XX
- ✅ 初始项目搭建
- ✅ 基础设备模块实现
- ✅ UI 界面设计

# 快速重构指南（最小改动方案）

## 目标
在不破坏现有代码的前提下，快速统一项目结构和命名规范。

## 当前问题
1. ❌ 文档文件散落在根目录
2. ❌ 模块命名不统一（LaserDriver vs DelayLine）
3. ❌ 主窗口文件在根目录

## 最小改动方案

### 步骤 1：整理文档（不影响编译）

```bash
# 创建文档目录
mkdir docs
mkdir docs/guides
mkdir docs/modules

# 移动根目录的文档
mv README.md docs/
mv SKILL.md docs/
mv ENABLE_GALVO_GUIDE.md docs/guides/
mv GALVO_ENABLED_SUMMARY.md docs/guides/
mv GIT_PUSH_GUIDE.txt docs/guides/
mv SPECTROMETER_COMPLETION_SUMMARY.md docs/guides/
mv PROJECT_RESTRUCTURE_PLAN.md docs/

# 整理模块文档（不移动代码文件）
# 这些文档已经在各自的模块目录中，只需要统一命名
```

### 步骤 2：统一模块文档结构

每个模块目录应该包含：
```
ModuleName/
├── module_name.h           # 头文件
├── module_name.cpp         # 实现文件
├── module_protocol.h       # 协议定义（如果有）
├── README.md               # 模块说明
└── docs/                   # 模块文档（可选）
    ├── usage_guide.md
    ├── test_guide.md
    └── protocol.txt
```

### 步骤 3：创建统一的 README 文件

为每个模块创建 README.md：

#### LaserDriver/README.md
```markdown
# 激光器驱动模块

## 功能
控制三个独立的激光器：种子源、FOPO、Stokes

## 协议
OHLD-1000 激光器驱动控制协议

## 文件
- `laser_driver.h/cpp` - 激光器驱动类
- `laser_protocol.h` - 协议定义
- `docs/` - 详细文档

## 使用
参见 `docs/LASER_TEST_GUIDE.md`
```

#### Spectrometer/README.md
```markdown
# 光谱仪模块

## 功能
控制 OPTOSKY ATP 系列光纤光谱仪

## 协议
OPTOSKY ATP 系列通讯协议 V2.3

## 文件
- `spectrometer.h/cpp` - 光谱仪驱动类
- `spectrometer_protocol.h` - 协议定义
- `library/` - DLL 库文件
- `docs/` - 详细文档

## 使用
参见 `docs/SPECTROMETER_TEST_GUIDE.md`
```

#### GalvoMirror/README.md
```markdown
# 振镜控制模块

## 功能
控制思特 GMC 系列振镜控制卡

## 要求
- MSVC 编译器（MinGW 不支持）
- Visual Studio 2022 或更高版本

## 文件
- `galvo_gmc_controller.h/cpp` - 振镜控制类
- `library/` - DLL 库文件
- `docs/` - 详细文档

## 使用
参见 `docs/GMC_USAGE.md`
```

#### StageController/README.md
```markdown
# 位移台控制模块

## 功能
控制旋转台和直线台

## 文件
- `stage_controller.h/cpp` - 位移台控制类
- `stage_protocol.h` - 协议定义

## 使用
通过串口通信控制位移台
```

#### DelayLine/README.md
```markdown
# 延迟线控制模块

## 功能
控制光学延迟线

## 文件
- `delay_line.h/cpp` - 延迟线控制类
- `delay_protocol.h` - 协议定义

## 使用
通过串口通信控制延迟线
```

### 步骤 4：更新主 README.md

```markdown
# Integration - 光学系统集成控制软件

## 项目简介
这是一个用于控制多个光学设备的集成软件，包括激光器、光谱仪、振镜、位移台和延迟线。

## 系统架构

### 设备模块
- **LaserDriver** - 激光器驱动（3个独立激光器）
- **Spectrometer** - 光谱仪控制
- **GalvoMirror** - 振镜控制
- **StageController** - 位移台控制
- **DelayLine** - 延迟线控制

### 核心模块
- **Communication** - 通信基类（串口、设备基类）
- **Utils** - 工具类（配置、数据管理、导出）

### UI 模块
- **integration** - 主窗口
- **chart_dialog** - 图表对话框
- **custom_chart_view** - 自定义图表视图

## 目录结构

```
Integration/
├── Communication/      # 通信基类
├── Utils/              # 工具类
├── LaserDriver/        # 激光器模块
├── Spectrometer/       # 光谱仪模块
├── GalvoMirror/        # 振镜模块
├── StageController/    # 位移台模块
├── DelayLine/          # 延迟线模块
├── docs/               # 项目文档
│   ├── guides/         # 使用指南
│   └── modules/        # 模块文档
├── integration.*       # 主窗口
├── chart_dialog.*      # 图表对话框
├── custom_chart_view.* # 自定义图表
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

### 光谱仪控制
- 单次测量
- 持续测量（实时）
- 积分时间控制
- 峰值检测
- 数据导出

### 振镜控制
- 角度控制
- 扫描控制
- 导引激光控制

### 位移台控制
- 旋转台角度控制
- 直线台位置控制
- 位置反馈

### 延迟线控制
- 延迟时间设置
- 实时反馈

### 预设管理
- 光源功率预设
- 延迟线预设
- 批量执行
- 动态控制

## 文档

### 用户文档
- [快速开始指南](docs/guides/QUICK_START.md)
- [用户手册](docs/USER_MANUAL.md)

### 开发文档
- [项目结构](docs/PROJECT_STRUCTURE.md)
- [开发指南](docs/DEVELOPMENT_GUIDE.md)
- [技能文档](docs/SKILL.md)

### 模块文档
- [激光器模块](LaserDriver/README.md)
- [光谱仪模块](Spectrometer/README.md)
- [振镜模块](GalvoMirror/README.md)
- [位移台模块](StageController/README.md)
- [延迟线模块](DelayLine/README.md)

## 许可证
[待定]

## 联系方式
[待定]
```

### 步骤 5：创建 .gitignore（如果还没有）

```gitignore
# Qt
*.pro.user
*.pro.user.*
*.autosave
*.qm
*.qrc.depends
*.moc
moc_*.cpp
qrc_*.cpp
ui_*.h

# C++
*.o
*.obj
*.exe
*.dll
*.so
*.dylib
*.a
*.lib

# Build
build/
debug/
release/
*.log

# IDE
.vscode/
.idea/
*.suo
*.user
*.sln.docstates

# OS
.DS_Store
Thumbs.db
desktop.ini

# Temporary
*~
*.swp
*.swo
*.tmp
```

## 执行步骤

### 1. 创建文档目录
```bash
mkdir docs
mkdir docs/guides
mkdir docs/modules
```

### 2. 移动文档文件
```bash
# 移动根目录文档
mv README.md docs/ 2>nul
mv SKILL.md docs/ 2>nul
mv ENABLE_GALVO_GUIDE.md docs/guides/ 2>nul
mv GALVO_ENABLED_SUMMARY.md docs/guides/ 2>nul
mv GIT_PUSH_GUIDE.txt docs/guides/ 2>nul
mv SPECTROMETER_COMPLETION_SUMMARY.md docs/guides/ 2>nul
mv PROJECT_RESTRUCTURE_PLAN.md docs/ 2>nul
```

### 3. 创建模块 README
为每个模块创建 README.md 文件（内容见上面）

### 4. 创建新的主 README.md
在根目录创建新的 README.md（内容见上面）

### 5. 提交更改
```bash
git add .
git commit -m "docs: 重新组织项目文档结构"
```

## 优点

### ✅ 不影响编译
- 只移动文档文件
- 不修改代码文件
- 不修改 Integration.pro

### ✅ 清晰的文档结构
- 所有项目文档在 `docs/`
- 每个模块有自己的 README
- 易于查找和维护

### ✅ 易于实施
- 只需几分钟
- 风险极低
- 可以随时回滚

### ✅ 为未来重构打基础
- 文档已经整理好
- 模块结构已经清晰
- 可以逐步进行代码重构

## 下一步（可选）

如果需要进一步重构代码结构，可以参考 `docs/PROJECT_RESTRUCTURE_PLAN.md`，但建议：
1. 在新的 Git 分支上进行
2. 逐步实施，每次只移动一个模块
3. 每次移动后都要测试编译和运行
4. 保持代码可用性

## 总结

这个快速重构方案：
- ✅ 最小改动
- ✅ 不影响编译
- ✅ 立即可执行
- ✅ 风险极低
- ✅ 效果明显

建议先执行这个方案，整理好文档结构，然后再考虑是否需要进一步的代码重构。

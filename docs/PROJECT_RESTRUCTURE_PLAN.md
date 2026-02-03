# 项目结构重构方案

## 当前问题分析

### 1. 命名不统一
- ❌ `LaserDriver` vs `DelayLine` vs `Spectrometer`
- ❌ `GalvoMirror` vs `StageController`
- ❌ 有些用下划线，有些用驼峰

### 2. 文件组织混乱
- ❌ 主窗口文件（integration.*）在根目录
- ❌ 图表相关文件（chart_dialog.*）在根目录
- ❌ 文档文件散落在根目录和各模块中

### 3. 模块结构不一致
- ❌ 有些模块有 protocol.h，有些没有
- ❌ 有些模块有文档，有些没有
- ❌ 有些模块有测试文件，有些没有

## 重构方案

### 目标
1. ✅ 统一命名规范
2. ✅ 清晰的目录结构
3. ✅ 模块化设计
4. ✅ 易于维护和扩展

### 新的目录结构

```
Integration/
├── src/                          # 源代码目录
│   ├── main.cpp                  # 程序入口
│   │
│   ├── Core/                     # 核心模块
│   │   ├── Communication/        # 通信基类
│   │   │   ├── device_base.h
│   │   │   ├── device_base.cpp
│   │   │   ├── serial_port_base.h
│   │   │   ├── serial_port_base.cpp
│   │   │   └── error_codes.h
│   │   │
│   │   └── Utils/                # 工具类
│   │       ├── config_manager.h
│   │       ├── config_manager.cpp
│   │       ├── data_manager.h
│   │       ├── data_manager.cpp
│   │       ├── csv_exporter.h
│   │       ├── csv_exporter.cpp
│   │       ├── image_saver.h
│   │       ├── image_saver.cpp
│   │       ├── preset_manager.h
│   │       ├── preset_manager.cpp
│   │       ├── constants.h
│   │       └── qcustomplot.h
│   │
│   ├── Devices/                  # 设备模块（统一命名）
│   │   ├── Laser/                # 激光器模块
│   │   │   ├── laser_driver.h
│   │   │   ├── laser_driver.cpp
│   │   │   ├── laser_protocol.h
│   │   │   └── docs/
│   │   │       ├── LASER_CONTROL_ARCHITECTURE.md
│   │   │       ├── LASER_TEST_GUIDE.md
│   │   │       └── OHLD_Protocol.txt
│   │   │
│   │   ├── Spectrometer/         # 光谱仪模块
│   │   │   ├── spectrometer.h
│   │   │   ├── spectrometer.cpp
│   │   │   ├── spectrometer_protocol.h
│   │   │   ├── library/          # DLL 库文件
│   │   │   │   ├── Driver.dll
│   │   │   │   ├── Driver.lib
│   │   │   │   ├── Driver_app.h
│   │   │   │   └── DriverType.h
│   │   │   └── docs/
│   │   │       ├── SPECTROMETER_IMPLEMENTATION_STATUS.md
│   │   │       ├── SPECTROMETER_TEST_GUIDE.md
│   │   │       └── OPTOSKY_ATP_Protocol.txt
│   │   │
│   │   ├── Galvo/                # 振镜模块（改名）
│   │   │   ├── galvo_controller.h
│   │   │   ├── galvo_controller.cpp
│   │   │   ├── library/          # DLL 库文件
│   │   │   │   ├── HM_HashuScan.dll
│   │   │   │   ├── HM_HashuScan.lib
│   │   │   │   ├── HM_HashuScan.h
│   │   │   │   ├── HM_Comm.dll
│   │   │   │   ├── HM_Comm.lib
│   │   │   │   ├── HM_HashuUDM.h
│   │   │   │   └── system.ini
│   │   │   └── docs/
│   │   │       ├── GMC_USAGE.md
│   │   │       └── GALVO_ENABLED_SUMMARY.md
│   │   │
│   │   ├── Stage/                # 位移台模块（改名）
│   │   │   ├── stage_controller.h
│   │   │   ├── stage_controller.cpp
│   │   │   └── stage_protocol.h
│   │   │
│   │   └── Delay/                # 延迟线模块（改名）
│   │       ├── delay_controller.h
│   │       ├── delay_controller.cpp
│   │       └── delay_protocol.h
│   │
│   ├── UI/                       # 用户界面模块
│   │   ├── MainWindow/           # 主窗口
│   │   │   ├── integration.h
│   │   │   ├── integration.cpp
│   │   │   └── integration.ui
│   │   │
│   │   ├── Charts/               # 图表相关
│   │   │   ├── chart_dialog.h
│   │   │   ├── chart_dialog.cpp
│   │   │   ├── custom_chart_view.h
│   │   │   └── custom_chart_view.cpp
│   │   │
│   │   └── Widgets/              # 自定义控件（预留）
│   │
│   └── Resources/                # 资源文件
│       ├── icons/
│       ├── images/
│       └── styles/
│
├── docs/                         # 项目文档
│   ├── README.md                 # 项目说明
│   ├── SKILL.md                  # 技能文档
│   ├── PROJECT_STRUCTURE.md      # 项目结构说明
│   ├── DEVELOPMENT_GUIDE.md      # 开发指南
│   ├── USER_MANUAL.md            # 用户手册
│   └── guides/                   # 指南文档
│       ├── ENABLE_GALVO_GUIDE.md
│       └── GIT_PUSH_GUIDE.txt
│
├── tests/                        # 测试文件（预留）
│   ├── unit/
│   └── integration/
│
├── build/                        # 构建输出目录
│   ├── debug/
│   └── release/
│
├── config/                       # 配置文件
│   └── default_config.json
│
├── .gitignore
├── .vscode/
│   ├── launch.json
│   └── settings.json
│
└── Integration.pro               # Qt 项目文件
```

## 重构步骤

### 阶段 1：创建新目录结构（不移动文件）

```bash
# 创建主要目录
mkdir -p src/Core/Communication
mkdir -p src/Core/Utils
mkdir -p src/Devices/Laser/docs
mkdir -p src/Devices/Spectrometer/library/docs
mkdir -p src/Devices/Galvo/library/docs
mkdir -p src/Devices/Stage
mkdir -p src/Devices/Delay
mkdir -p src/UI/MainWindow
mkdir -p src/UI/Charts
mkdir -p src/UI/Widgets
mkdir -p src/Resources/icons
mkdir -p src/Resources/images
mkdir -p src/Resources/styles
mkdir -p docs/guides
mkdir -p tests/unit
mkdir -p tests/integration
mkdir -p build/debug
mkdir -p build/release
mkdir -p config
```

### 阶段 2：移动文件（保持编译可用）

#### 2.1 移动核心模块
```bash
# Communication
mv Communication/* src/Core/Communication/

# Utils
mv Utils/* src/Core/Utils/
```

#### 2.2 移动设备模块
```bash
# Laser
mv LaserDriver/laser_driver.* src/Devices/Laser/
mv LaserDriver/laser_protocol.h src/Devices/Laser/
mv LaserDriver/*.md src/Devices/Laser/docs/
mv LaserDriver/*.txt src/Devices/Laser/docs/

# Spectrometer
mv Spectrometer/spectrometer.* src/Devices/Spectrometer/
mv Spectrometer/spectrometer_protocol.h src/Devices/Spectrometer/
mv Spectrometer/*.dll src/Devices/Spectrometer/library/
mv Spectrometer/*.lib src/Devices/Spectrometer/library/
mv Spectrometer/*.h src/Devices/Spectrometer/library/
mv Spectrometer/*.md src/Devices/Spectrometer/docs/
mv Spectrometer/*.txt src/Devices/Spectrometer/docs/

# Galvo
mv GalvoMirror/galvo_gmc_controller.* src/Devices/Galvo/
mv GalvoMirror/library/* src/Devices/Galvo/library/
mv GalvoMirror/*.md src/Devices/Galvo/docs/

# Stage
mv StageController/* src/Devices/Stage/

# Delay
mv DelayLine/delay_line.* src/Devices/Delay/
mv DelayLine/delay_protocol.h src/Devices/Delay/
```

#### 2.3 移动 UI 模块
```bash
# MainWindow
mv integration.* src/UI/MainWindow/

# Charts
mv chart_dialog.* src/UI/Charts/
mv custom_chart_view.* src/UI/Charts/

# Main
mv main.cpp src/
```

#### 2.4 移动文档
```bash
mv README.md docs/
mv SKILL.md docs/
mv ENABLE_GALVO_GUIDE.md docs/guides/
mv GALVO_ENABLED_SUMMARY.md docs/guides/
mv GIT_PUSH_GUIDE.txt docs/guides/
mv SPECTROMETER_COMPLETION_SUMMARY.md docs/guides/
```

### 阶段 3：更新 Integration.pro

```qmake
QT += core gui widgets serialport charts network

CONFIG += c++11

# 定义源代码目录
SRCDIR = $$PWD/src

# 包含路径
INCLUDEPATH += $$SRCDIR
INCLUDEPATH += $$SRCDIR/Core
INCLUDEPATH += $$SRCDIR/Devices
INCLUDEPATH += $$SRCDIR/UI

# 源文件
SOURCES += \
    $$SRCDIR/main.cpp \
    $$SRCDIR/UI/MainWindow/integration.cpp \
    $$SRCDIR/UI/Charts/custom_chart_view.cpp \
    $$SRCDIR/UI/Charts/chart_dialog.cpp \
    $$SRCDIR/Core/Communication/serial_port_base.cpp \
    $$SRCDIR/Core/Communication/device_base.cpp \
    $$SRCDIR/Devices/Laser/laser_driver.cpp \
    $$SRCDIR/Devices/Spectrometer/spectrometer.cpp \
    $$SRCDIR/Devices/Stage/stage_controller.cpp \
    $$SRCDIR/Devices/Galvo/galvo_controller.cpp \
    $$SRCDIR/Devices/Delay/delay_controller.cpp \
    $$SRCDIR/Core/Utils/data_manager.cpp \
    $$SRCDIR/Core/Utils/csv_exporter.cpp \
    $$SRCDIR/Core/Utils/image_saver.cpp \
    $$SRCDIR/Core/Utils/config_manager.cpp \
    $$SRCDIR/Core/Utils/preset_manager.cpp

# 头文件
HEADERS += \
    $$SRCDIR/UI/MainWindow/integration.h \
    $$SRCDIR/UI/Charts/custom_chart_view.h \
    $$SRCDIR/UI/Charts/chart_dialog.h \
    $$SRCDIR/Core/Communication/serial_port_base.h \
    $$SRCDIR/Core/Communication/device_base.h \
    $$SRCDIR/Core/Communication/error_codes.h \
    $$SRCDIR/Devices/Laser/laser_driver.h \
    $$SRCDIR/Devices/Laser/laser_protocol.h \
    $$SRCDIR/Devices/Spectrometer/spectrometer.h \
    $$SRCDIR/Devices/Spectrometer/spectrometer_protocol.h \
    $$SRCDIR/Devices/Stage/stage_controller.h \
    $$SRCDIR/Devices/Stage/stage_protocol.h \
    $$SRCDIR/Devices/Galvo/galvo_controller.h \
    $$SRCDIR/Devices/Galvo/library/HM_HashuScan.h \
    $$SRCDIR/Devices/Galvo/library/HM_HashuUDM.h \
    $$SRCDIR/Devices/Delay/delay_controller.h \
    $$SRCDIR/Devices/Delay/delay_protocol.h \
    $$SRCDIR/Core/Utils/data_manager.h \
    $$SRCDIR/Core/Utils/csv_exporter.h \
    $$SRCDIR/Core/Utils/image_saver.h \
    $$SRCDIR/Core/Utils/config_manager.h \
    $$SRCDIR/Core/Utils/constants.h \
    $$SRCDIR/Core/Utils/preset_manager.h

# UI 文件
FORMS += \
    $$SRCDIR/UI/MainWindow/integration.ui

# 振镜控制卡库文件（思特 GMC）
LIBS += -L$$SRCDIR/Devices/Galvo/library -lHM_HashuScan
LIBS += -L$$SRCDIR/Devices/Galvo/library -lHM_Comm

# 包含路径
INCLUDEPATH += $$SRCDIR/Devices/Galvo/library
DEPENDPATH += $$SRCDIR/Devices/Galvo/library

# 输出目录
DESTDIR = $$PWD/build/release
OBJECTS_DIR = $$PWD/build/release/obj
MOC_DIR = $$PWD/build/release/moc
RCC_DIR = $$PWD/build/release/rcc
UI_DIR = $$PWD/build/release/ui

# Windows 平台：编译后自动复制振镜 DLL 到输出目录
win32 {
    QMAKE_POST_LINK += xcopy /Y /I /E $$shell_quote($$shell_path($$SRCDIR/Devices/Galvo/library)) $$shell_quote($$shell_path($$DESTDIR/Galvo))
}
```

### 阶段 4：更新头文件包含路径

所有 `#include` 语句需要更新：

**旧的：**
```cpp
#include "../Communication/device_base.h"
#include "laser_protocol.h"
```

**新的：**
```cpp
#include "Core/Communication/device_base.h"
#include "Devices/Laser/laser_protocol.h"
```

### 阶段 5：重命名文件（统一命名规范）

#### 5.1 延迟线模块
```bash
# 重命名文件
mv src/Devices/Delay/delay_line.h src/Devices/Delay/delay_controller.h
mv src/Devices/Delay/delay_line.cpp src/Devices/Delay/delay_controller.cpp

# 更新类名
# DelayLine → DelayController
```

#### 5.2 振镜模块
```bash
# 重命名文件
mv src/Devices/Galvo/galvo_gmc_controller.h src/Devices/Galvo/galvo_controller.h
mv src/Devices/Galvo/galvo_gmc_controller.cpp src/Devices/Galvo/galvo_controller.cpp

# 更新类名
# GalvoGMCController → GalvoController
```

## 命名规范

### 1. 目录命名
- ✅ 使用 PascalCase（首字母大写）
- ✅ 例如：`Devices/`, `Core/`, `Utils/`

### 2. 文件命名
- ✅ 使用 snake_case（小写+下划线）
- ✅ 例如：`laser_driver.h`, `stage_controller.cpp`

### 3. 类命名
- ✅ 使用 PascalCase
- ✅ 例如：`LaserDriver`, `StageController`

### 4. 变量命名
- ✅ 成员变量：`m_` 前缀 + camelCase
- ✅ 例如：`m_serialPort`, `m_deviceName`

### 5. 函数命名
- ✅ 使用 camelCase
- ✅ 例如：`connect()`, `setIntegrationTime()`

## 优点

### 1. 清晰的模块划分
- ✅ Core：核心功能（通信、工具）
- ✅ Devices：设备驱动（激光器、光谱仪等）
- ✅ UI：用户界面（主窗口、图表）

### 2. 统一的命名规范
- ✅ 所有模块使用相同的命名方式
- ✅ 易于理解和维护

### 3. 文档集中管理
- ✅ 所有项目文档在 `docs/` 目录
- ✅ 模块文档在各模块的 `docs/` 子目录

### 4. 易于扩展
- ✅ 添加新设备只需在 `Devices/` 下创建新目录
- ✅ 添加新 UI 组件只需在 `UI/` 下创建新目录

### 5. 构建输出分离
- ✅ 源代码在 `src/`
- ✅ 构建输出在 `build/`
- ✅ 不污染源代码目录

## 注意事项

### ⚠️ 重构风险
1. **编译错误**：所有 `#include` 路径需要更新
2. **Git 历史**：使用 `git mv` 保留文件历史
3. **测试**：重构后需要全面测试所有功能

### ⚠️ 建议
1. **分阶段进行**：不要一次性移动所有文件
2. **保持编译**：每个阶段完成后确保项目可以编译
3. **备份代码**：重构前创建 Git 分支或备份
4. **逐步测试**：每移动一个模块就测试一次

## 实施计划

### 第 1 天：准备工作
- [ ] 创建 Git 分支：`git checkout -b refactor/project-structure`
- [ ] 创建新目录结构
- [ ] 备份当前代码

### 第 2 天：移动核心模块
- [ ] 移动 Communication
- [ ] 移动 Utils
- [ ] 更新 Integration.pro
- [ ] 测试编译

### 第 3 天：移动设备模块
- [ ] 移动 Laser
- [ ] 移动 Spectrometer
- [ ] 移动 Stage
- [ ] 移动 Delay
- [ ] 测试编译

### 第 4 天：移动 UI 模块
- [ ] 移动 MainWindow
- [ ] 移动 Charts
- [ ] 更新所有 `#include` 路径
- [ ] 测试编译

### 第 5 天：重命名和清理
- [ ] 重命名 DelayLine → DelayController
- [ ] 重命名 GalvoGMCController → GalvoController
- [ ] 移动文档
- [ ] 删除空目录
- [ ] 全面测试

### 第 6 天：文档更新
- [ ] 更新 README.md
- [ ] 更新开发文档
- [ ] 创建项目结构说明
- [ ] 提交代码

## 总结

这个重构方案将：
1. ✅ 统一命名规范
2. ✅ 清晰的目录结构
3. ✅ 模块化设计
4. ✅ 易于维护和扩展
5. ✅ 符合软件工程最佳实践

**建议**：由于这是一个大规模重构，建议在新分支上进行，并逐步实施，确保每个阶段都能正常编译和运行。

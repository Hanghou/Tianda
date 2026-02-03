# 工具类模块

## 功能概述
提供配置管理、数据管理、导出和预设管理等工具类。

## 文件说明
- `config_manager.h/cpp` - 配置管理
- `data_manager.h/cpp` - 数据管理
- `csv_exporter.h/cpp` - CSV 导出
- `image_saver.h/cpp` - 图像保存
- `preset_manager.h/cpp` - 预设管理
- `constants.h` - 常量定义
- `qcustomplot.h` - 自定义绘图库

## 主要功能

### ConfigManager（配置管理）
- ✅ 保存/加载设备配置
- ✅ 串口配置管理
- ✅ 用户偏好设置
- ✅ INI 文件格式

### DataManager（数据管理）
- ✅ 光谱数据存储
- ✅ 测量结果管理
- ✅ 历史数据查询
- ✅ 数据缓存

### CSVExporter（CSV 导出）
- ✅ 光谱数据导出
- ✅ 峰值数据导出
- ✅ 自定义格式
- ✅ 批量导出

### ImageSaver（图像保存）
- ✅ 图表截图
- ✅ PNG/JPG 格式
- ✅ 自定义分辨率
- ✅ 批量保存

### PresetManager（预设管理）
- ✅ 功率预设
- ✅ 延迟线预设
- ✅ 组合预设
- ✅ 批量执行

## 使用示例

### 配置管理
```cpp
ConfigManager *config = new ConfigManager();
config->saveSerialConfig("LaserDriver", "COM3", 9600);
config->loadSerialConfig("LaserDriver", port, baudRate);
```

### 数据导出
```cpp
CSVExporter *exporter = new CSVExporter();
exporter->exportSpectrum("spectrum.csv", wavelength, intensity);
exporter->exportPeaks("peaks.csv", peaks);
```

### 图像保存
```cpp
ImageSaver *saver = new ImageSaver();
saver->saveChart("chart.png", chart);
```

### 预设管理
```cpp
PresetManager *presets = new PresetManager();
presets->addPowerPreset(preset);
presets->executePowerPreset(index);
```

## 预设数据结构

### 功率预设（振镜页）
```cpp
struct PowerPreset {
    float seedCurrent;   // 种子源电流 (mA)
    float fopoCurrent;   // FOPO电流 (A)
    float stokesCurrent; // Stokes电流 (mA)
};
```

### 功率预设（位移台页）
```cpp
struct StagePowerPreset {
    float rotationAngle;  // 旋转台角度
    float linearPosition; // 直线台位置
    float seedCurrent;    // 种子源电流
    float fopoCurrent;    // FOPO电流
    float stokesCurrent;  // Stokes电流
};
```

### 延迟线预设
```cpp
struct DelayPreset {
    float delayTime;  // 延迟时间 (ps)
};
```

## 常量定义
参见 `constants.h`：
- 默认串口参数
- 超时时间
- 缓冲区大小
- 文件路径

## 参考文档
- 常量定义：`constants.h`
- 各工具类头文件

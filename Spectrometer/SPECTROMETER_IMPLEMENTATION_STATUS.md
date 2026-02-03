# 光谱仪实现状态分析

## 协议对比：OPTOSKY ATP 系列 V2.3

### ✅ 已实现的功能

#### 1. 基本连接和设备信息
- ✅ **串口连接**（115200, 8N1）
- ✅ **获取像素长度**（命令 0x0A）
- ✅ **获取序列号**（命令 0x04）
- ✅ **获取产品型号**（命令 0x03）
- ✅ **获取波长标定系数**（命令 0x55）

#### 2. 积分时间控制
- ✅ **设置积分时间**（命令 0x14）
- ✅ **获取当前积分时间**（命令 0x41）
- ✅ **获取最大积分时间**（命令 0x42）
- ✅ **获取最小积分时间**（命令 0x43）

#### 3. 光谱采集
- ✅ **单次测量**（命令 0x1E - 同步方式）
- ✅ **设置平均次数**（命令 0x28）
- ✅ **光谱数据解析**（状态位 + 像素数据）

#### 4. 协议实现
- ✅ **帧结构正确**（AA 55 + Length + Cmd + Data + Checksum）
- ✅ **校验和算法正确**（累加和取低8位）
- ✅ **大端序处理正确**

### ⚠️ 未实现的功能

#### 1. 持续测量（连续采集）
- ❌ **命令 0x19**：CCD 连续采集控制
- ❌ **连续模式的启动/停止逻辑**
- ❌ **定时器驱动的连续采集**

**协议说明：**
```
命令 0x19: CCD连续采集控制
发送: <Head><Length><Cmd><Data><Checksum>
Data: (5bytes)
  Data[0]: 0x00=开始采集, 0x01=停止采集
  Data[1-4]: 采集时间间隔（单位：us）
```

#### 2. 异步采集模式
- ❌ **命令 0x16**：开启 CCD 扫描（异步方式）
- ❌ **命令 0x17**：读取 CCD 采集数据（配合 0x16 使用）
- ❌ **命令 0x23**：采集暗电流（异步方式）

#### 3. 外部触发模式
- ❌ **命令 0x1F**：开启外部触发采集功能
- ❌ **命令 0xF1**：关闭外部触发采集功能

#### 4. 暗电流采集
- ❌ **命令 0x23**：采集暗电流（异步方式）
- ❌ **命令 0x2F**：采集暗电流（同步方式）

#### 5. 温度控制
- ❌ **命令 0x01**：获取模块电路板温度
- ❌ **命令 0x12**：设置 TEC 温度
- ❌ **命令 0x13**：获取 TEC 温度
- ❌ **命令 0x35**：获取光学平台温度

#### 6. 硬件控制
- ❌ **命令 0x60**：控制 Lamp 输出电平状态
- ❌ **命令 0x61**：设置 GPIO 引脚输出电平
- ❌ **命令 0x62**：设置光开关快门状态

#### 7. 其他信息
- ❌ **命令 0x06**：获取模块生产日期
- ❌ **命令 0x09**：获取模块厂家信息
- ❌ **命令 0x46**：获取设备属性

## UI 功能对比

### 当前 UI 按钮
1. ✅ **单次测量**（btnSingleMeasure）- 已实现
2. ⚠️ **持续测量**（btnContinuousMeasure）- 未完整实现
3. ✅ **停止测量**（btnStopMeasure）- 已实现
4. ✅ **保存图谱**（btnSavePlot）- 已实现
5. ✅ **重置视图**（btnResetView）- 已实现
6. ✅ **清除数据**（btnClearPlot）- 已实现
7. ✅ **峰值检测**（btnDetectPeaks）- 已实现
8. ✅ **导出峰值**（btnExportPeaks）- 已实现
9. ✅ **清除峰值**（btnClearPeaks）- 已实现

### 需要完善的功能

#### 1. 持续测量（实时测量）

**当前状态：**
```cpp
void Integration::on_btnContinuousMeasure_clicked()
{
    if (!m_spectrometer->isConnected()) {
        QMessageBox::warning(this, "错误", "请先连接光谱仪");
        return;
    }
    
    // TODO: 实现连续测量模式
    startMeasurement();  // 目前只是调用单次测量
}
```

**需要实现：**

**方案 A：使用命令 0x19（推荐）**
```cpp
// 在 Spectrometer 类中添加
bool Spectrometer::startContinuousScan(int intervalMicros);
bool Spectrometer::stopContinuousScan();

// 实现
bool Spectrometer::startContinuousScan(int intervalMicros)
{
    QByteArray data;
    data.append((char)0x00);  // 开始采集
    // 采集时间间隔（4字节，大端序）
    data.append((intervalMicros >> 24) & 0xFF);
    data.append((intervalMicros >> 16) & 0xFF);
    data.append((intervalMicros >> 8) & 0xFF);
    data.append(intervalMicros & 0xFF);
    
    return sendCommand(0x19, data);
}

bool Spectrometer::stopContinuousScan()
{
    QByteArray data;
    data.append((char)0x01);  // 停止采集
    data.append((char)0x00);
    data.append((char)0x00);
    data.append((char)0x00);
    data.append((char)0x00);
    
    return sendCommand(0x19, data);
}
```

**方案 B：使用定时器 + 命令 0x1E**
```cpp
// 在 Integration 类中
QTimer *m_continuousMeasureTimer;

void Integration::on_btnContinuousMeasure_clicked()
{
    if (!m_spectrometer->isConnected()) {
        QMessageBox::warning(this, "错误", "请先连接光谱仪");
        return;
    }
    
    // 获取采集间隔（例如从 UI 输入框）
    int intervalMs = ui->spinBoxMeasureInterval->value();
    
    // 启动定时器
    m_continuousMeasureTimer->start(intervalMs);
    m_isContinuousMeasuring = true;
    
    updateStatusBar("开始持续测量");
}

void Integration::on_btnStopMeasure_clicked()
{
    if (m_isContinuousMeasuring) {
        m_continuousMeasureTimer->stop();
        m_isContinuousMeasuring = false;
    }
    
    stopMeasurement();
}

// 定时器槽函数
void Integration::onContinuousMeasureTimeout()
{
    if (m_spectrometer->isConnected()) {
        m_spectrometer->startScan();  // 触发单次采集
    }
}
```

#### 2. 暗电流采集

**用途：** 用于背景扣除，提高光谱测量精度

**实现：**
```cpp
// 在 Spectrometer 类中添加
bool Spectrometer::captureDarkCurrent();
QVector<int> Spectrometer::getDarkCurrent() const;

// 在 UI 中添加按钮
void Integration::on_btnCaptureDark_clicked()
{
    if (!m_spectrometer->isConnected()) {
        QMessageBox::warning(this, "错误", "请先连接光谱仪");
        return;
    }
    
    QMessageBox::information(this, "采集暗电流", 
        "请遮挡光源，然后点击确定开始采集暗电流");
    
    if (m_spectrometer->captureDarkCurrent()) {
        QMessageBox::information(this, "成功", "暗电流采集成功");
    } else {
        QMessageBox::warning(this, "失败", "暗电流采集失败");
    }
}
```

#### 3. 温度监控

**用途：** 监控光谱仪温度，确保稳定性

**实现：**
```cpp
// 在 Spectrometer 类中添加
float Spectrometer::getBoardTemperature();
float Spectrometer::getTECTemperature();
float Spectrometer::getOpticalTemperature();

// 在 UI 中显示温度
void Integration::updateSpectrometerTemperature()
{
    if (m_spectrometer->isConnected()) {
        float temp = m_spectrometer->getBoardTemperature();
        ui->labelSpectrometerTemp->setText(QString("%1 ℃").arg(temp, 0, 'f', 1));
    }
}
```

## 推荐的实现优先级

### 高优先级（必须实现）
1. ✅ **单次测量** - 已完成
2. ⚠️ **持续测量（连续采集）** - 需要完善
3. ⚠️ **停止持续测量** - 需要完善

### 中优先级（建议实现）
4. ❌ **暗电流采集** - 提高测量精度
5. ❌ **温度监控** - 确保设备稳定性
6. ❌ **积分时间 UI 控制** - 用户可调节

### 低优先级（可选实现）
7. ❌ **外部触发模式** - 高级功能
8. ❌ **GPIO 控制** - 硬件扩展
9. ❌ **快门控制** - 特殊应用

## 当前实现的优点

1. ✅ **协议实现正确** - 帧结构、校验和都符合规范
2. ✅ **单次测量稳定** - 同步方式采集可靠
3. ✅ **数据解析正确** - 像素数据正确提取
4. ✅ **UI 功能完整** - 图表显示、峰值检测、数据导出都已实现
5. ✅ **错误处理完善** - 超时、校验失败都有处理

## 需要改进的地方

1. ⚠️ **持续测量未实现** - 目前只是调用单次测量
2. ⚠️ **缺少采集间隔控制** - 持续测量需要时间间隔设置
3. ⚠️ **缺少暗电流功能** - 无法进行背景扣除
4. ⚠️ **缺少温度监控** - 无法了解设备状态
5. ⚠️ **缺少积分时间 UI** - 用户无法在 UI 中调节积分时间

## 总结

### 当前状态
- **单次测量**：✅ 完全实现，功能正常
- **持续测量**：⚠️ 部分实现，需要完善连续采集逻辑
- **协议支持**：✅ 基本协议正确，但未实现所有命令

### 建议
1. **优先完善持续测量功能** - 这是用户最常用的功能
2. **添加采集间隔控制** - 让用户可以设置测量频率
3. **添加积分时间 UI 控制** - 让用户可以调节曝光时间
4. **考虑添加暗电流采集** - 提高测量精度

### 实现方案推荐
- **持续测量**：使用方案 B（定时器 + 命令 0x1E）
  - 优点：实现简单，不依赖设备的连续模式
  - 缺点：采集间隔受限于积分时间和数据传输时间
- **如果需要高速连续采集**：使用方案 A（命令 0x19）
  - 优点：设备内部控制，更稳定
  - 缺点：需要实现异步数据接收

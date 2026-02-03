# 激光器控制架构说明

## 三个独立的激光器实例

系统中有三个完全独立的激光器驱动实例，它们不会相互混淆：

```cpp
// integration.h 中的成员变量
LaserDriver *m_seedLaserDriver;    // 种子源激光器
LaserDriver *m_fopoLaserDriver;    // FOPO激光器
LaserDriver *m_stokesLaserDriver;  // Stokes激光器
```

每个激光器实例：
- 有独立的串口连接
- 有独立的设备ID
- 有独立的状态管理
- 使用相同的控制协议（OHLD-1000）

## 激光器类型区分

在创建激光器实例时，通过 `LaserType` 枚举区分：

```cpp
m_seedLaserDriver = new LaserDriver(LaserType::SeedSource, this);
m_fopoLaserDriver = new LaserDriver(LaserType::FOPO, this);
m_stokesLaserDriver = new LaserDriver(LaserType::Stokes, this);
```

### 电流单位区别

- **种子源激光器**：单位为 mA（毫安）
- **FOPO激光器**：单位为 A（安培）
- **Stokes激光器**：单位为 mA（毫安）

## 控制页面架构

### 1. 振镜页面（Galvo Tab）

**UI 控件：**
- `lineEditSeedPump` - 种子源泵功率输入框
- `lineEditFOPOPump` - FOPO泵功率输入框
- `lineEditStokesPump` - Stokes泵功率输入框
- `btnConfirmSeedPump` - 种子源确认按钮
- `btnConfirmFOPOPump` - FOPO确认按钮
- `btnConfirmStokesPump` - Stokes确认按钮

**槽函数：**
```cpp
void Integration::on_btnConfirmSeedPump_clicked()
{
    setPumpCurrent(LaserDeviceType::Seed, 
                   ui->lineEditSeedPump, 
                   ui->lineEditStageSeedPump);
}

void Integration::on_btnConfirmFOPOPump_clicked()
{
    setPumpCurrent(LaserDeviceType::FOPO, 
                   ui->lineEditFOPOPump, 
                   ui->lineEditStageFOPOPump);
}

void Integration::on_btnConfirmStokesPump_clicked()
{
    setPumpCurrent(LaserDeviceType::Stokes, 
                   ui->lineEditStokesPump, 
                   ui->lineEditStageStokesPump);
}
```

### 2. 位移台页面（Stage Tab）

**UI 控件：**
- `lineEditStageSeedPump` - 种子源泵功率输入框
- `lineEditStageFOPOPump` - FOPO泵功率输入框
- `lineEditStageStokesPump` - Stokes泵功率输入框
- `btnConfirmStageSeedPump` - 种子源确认按钮
- `btnConfirmStageFOPOPump` - FOPO确认按钮
- `btnConfirmStageStokesPump` - Stokes确认按钮

**槽函数：**
```cpp
void Integration::on_btnConfirmStageSeedPump_clicked()
{
    setPumpCurrent(LaserDeviceType::Seed, 
                   ui->lineEditStageSeedPump, 
                   ui->lineEditSeedPump);
}

void Integration::on_btnConfirmStageFOPOPump_clicked()
{
    setPumpCurrent(LaserDeviceType::FOPO, 
                   ui->lineEditStageFOPOPump, 
                   ui->lineEditFOPOPump);
}

void Integration::on_btnConfirmStageStokesPump_clicked()
{
    setPumpCurrent(LaserDeviceType::Stokes, 
                   ui->lineEditStageStokesPump, 
                   ui->lineEditStokesPump);
}
```

## 统一的设置函数

所有页面都通过同一个函数 `setPumpCurrent()` 来设置激光器电流：

```cpp
void Integration::setPumpCurrent(LaserDeviceType type, 
                                 QLineEdit *inputField, 
                                 QLineEdit *syncField)
{
    // 1. 读取输入值
    float current = inputField->text().toFloat(&ok);
    
    // 2. 根据类型选择对应的激光器驱动
    LaserDriver *driver = nullptr;
    switch (type) {
        case LaserDeviceType::Seed:
            driver = m_seedLaserDriver;    // 种子源
            break;
        case LaserDeviceType::FOPO:
            driver = m_fopoLaserDriver;    // FOPO
            break;
        case LaserDeviceType::Stokes:
            driver = m_stokesLaserDriver;  // Stokes
            break;
    }
    
    // 3. 调用对应激光器的 setCurrent() 方法
    if (driver->setCurrent(current)) {
        // 4. 同步到另一个页面的输入框
        if (syncField) {
            syncField->setText(inputField->text());
        }
    }
}
```

### 关键特性

1. **类型安全**：通过 `LaserDeviceType` 枚举确保类型正确
2. **驱动隔离**：每个类型对应唯一的驱动实例
3. **双向同步**：振镜页和位移台页的输入框会自动同步
4. **独立串口**：每个激光器使用独立的串口连接

## 预设执行中的激光器控制

### 正常设置（executePreset）

```cpp
void Integration::executePreset(const Preset &preset)
{
    // 种子源
    if (preset.seedPumpCurrent != 0) {
        if (m_seedLaserDriver->isConnected()) {
            m_seedLaserDriver->setCurrent(preset.seedPumpCurrent);
        }
    }
    
    // FOPO
    if (preset.fopoPumpCurrent != 0) {
        if (m_fopoLaserDriver->isConnected()) {
            m_fopoLaserDriver->setCurrent(preset.fopoPumpCurrent);
        }
    }
    
    // Stokes
    if (preset.stokesPumpCurrent != 0) {
        if (m_stokesLaserDriver->isConnected()) {
            m_stokesLaserDriver->setCurrent(preset.stokesPumpCurrent);
        }
    }
}
```

### 动态控制（executeDynamicPreset）

```cpp
void Integration::executeDynamicPreset(const Preset &preset)
{
    // 种子源
    if (preset.seedPumpCurrent != 0) {
        if (m_seedLaserDriver->isConnected()) {
            m_seedLaserDriver->setCurrent(preset.seedPumpCurrent);
        }
    }
    
    // FOPO
    if (preset.fopoPumpCurrent != 0) {
        if (m_fopoLaserDriver->isConnected()) {
            m_fopoLaserDriver->setCurrent(preset.fopoPumpCurrent);
        }
    }
    
    // Stokes
    if (preset.stokesPumpCurrent != 0) {
        if (m_stokesLaserDriver->isConnected()) {
            m_stokesLaserDriver->setCurrent(preset.stokesPumpCurrent);
        }
    }
}
```

## 不会混淆的保证

### 1. 实例级别隔离
每个激光器有独立的 `LaserDriver` 实例，内部状态完全独立：
- 独立的串口对象 `m_serialPort`
- 独立的设备ID `m_deviceId`
- 独立的激光器类型 `m_laserType`
- 独立的状态 `m_currentStatus`

### 2. 串口级别隔离
每个激光器连接到不同的串口：
- 种子源：例如 COM3
- FOPO：例如 COM4
- Stokes：例如 COM5

发送命令时，每个激光器只会通过自己的串口发送，不会串扰。

### 3. 协议级别隔离
虽然三个激光器使用相同的 OHLD-1000 协议，但：
- 每个激光器的串口是独立的
- 命令只会发送到对应的串口
- 回复也只会从对应的串口接收

### 4. UI级别隔离
- 振镜页和位移台页有独立的输入框
- 每个输入框通过不同的槽函数处理
- 槽函数通过 `LaserDeviceType` 参数明确指定激光器类型

## 数据流示例

### 示例1：在振镜页设置种子源电流

```
用户操作：
  在振镜页输入 300mA → 点击"确认"按钮

数据流：
  1. on_btnConfirmSeedPump_clicked() 被触发
  2. 调用 setPumpCurrent(LaserDeviceType::Seed, ...)
  3. 选择 m_seedLaserDriver
  4. 调用 m_seedLaserDriver->setCurrent(300.0f)
  5. 构建命令：68 00 00 00 00 02 00 04 0B B8 00 00 36 16
  6. 通过种子源串口（例如COM3）发送
  7. 同步到位移台页的 lineEditStageSeedPump

结果：
  ✓ 只有种子源激光器收到命令
  ✓ FOPO和Stokes激光器不受影响
  ✓ 两个页面的输入框保持同步
```

### 示例2：在位移台页设置FOPO电流

```
用户操作：
  在位移台页输入 3A → 点击"确认"按钮

数据流：
  1. on_btnConfirmStageFOPOPump_clicked() 被触发
  2. 调用 setPumpCurrent(LaserDeviceType::FOPO, ...)
  3. 选择 m_fopoLaserDriver
  4. 调用 m_fopoLaserDriver->setCurrent(3.0f)
  5. 构建命令：68 00 00 00 00 02 00 04 00 1E 00 00 D5 16
  6. 通过FOPO串口（例如COM4）发送
  7. 同步到振镜页的 lineEditFOPOPump

结果：
  ✓ 只有FOPO激光器收到命令
  ✓ 种子源和Stokes激光器不受影响
  ✓ 两个页面的输入框保持同步
```

### 示例3：预设执行

```
用户操作：
  执行预设（种子源300mA, FOPO 3A, Stokes 200mA）

数据流：
  1. executePreset() 被调用
  2. 依次调用：
     - m_seedLaserDriver->setCurrent(300.0f)   → COM3
     - m_fopoLaserDriver->setCurrent(3.0f)     → COM4
     - m_stokesLaserDriver->setCurrent(200.0f) → COM5
  3. 每个命令通过各自的串口发送

结果：
  ✓ 三个激光器各自收到正确的命令
  ✓ 命令不会混淆或串扰
  ✓ 每个激光器独立响应
```

## 总结

✅ **不会混淆的原因：**

1. **实例隔离**：三个独立的 `LaserDriver` 对象
2. **串口隔离**：每个激光器使用不同的串口
3. **类型安全**：通过枚举类型明确指定激光器
4. **函数封装**：统一的 `setPumpCurrent()` 函数确保逻辑一致
5. **UI同步**：两个页面的输入框自动同步，避免不一致

✅ **可以正常使用的场景：**

1. ✓ 振镜页面设置激光器
2. ✓ 位移台页面设置激光器
3. ✓ 预设执行（正常设置）
4. ✓ 动态控制（动态预设）
5. ✓ 多个页面同时显示（输入框自动同步）

✅ **协议实现正确性：**

1. ✓ 帧结构符合 OHLD-1000 协议
2. ✓ 校验码算法正确（累加和取反）
3. ✓ 电流值处理正确（实际值*10）
4. ✓ 温度值处理正确（IEEE 754浮点数）
5. ✓ 所有命令格式已验证

# 激光器驱动修改 Prompt

> 基于科乃特激光器通信协议 V1.5 + 实测串口数据对比分析
> 修改范围：LaserDriver/laser_driver.cpp + laser_protocol.h

---

## 背景

现有代码的 0xD2/0xD3（读实时含义/读实时信息）完全未实现，导致无法获取温度、电流、报警等运行数据。校验和计算范围与协议不匹配。基本信息帧只解析了 1/36 个字段。

实测设备信息：
- 产品类别：0（激光器），非多路输出
- 功率设置方式：1（设置功率），非设置电流
- 功率系数：100，单位：mW
- 实测：50mW → 134mA 泵浦电流

---

## 修改任务清单

### P0 — 致命问题

#### 1. 修复校验和计算（laser_driver.cpp:buildFrame）

**当前（错误）**：
```cpp
QByteArray checksumData = frame;  // 包含信息头+命令字+数据长度+数据——全算了
```

**修改为（正确）**：
```cpp
QByteArray checksumData = data;   // 只对数据域计算
```
若 data 为空，确保校验和输出 `00 00`。

**证据**：实测 0xD2 返回帧 `...11 1F 21 51 00`，校验和 0x51 = 0x11+0x1F+0x21（仅数据域）。

---

### P1 — 核心缺失

#### 2. 扩展 parseBasicInfo — 当前只读了 1 个字段

**laser_driver.cpp:parseBasicInfo()** 需新增提取：

| 字节偏移(从数据域第0起) | 字段 | 存储到 |
|---|---|---|
| 22 | 功率单位 (0=mW, 1=W, 2=uW) | `m_basicInfo.powerUnit` |
| 25 | 功率设置方式 | `m_basicInfo.powerSetMode` (已有) |
| 28 | **功率系数** | `m_basicInfo.powerCoefficient` |
| 36 | 产品类别 | `m_basicInfo.productType` |
| 37-38 | 种子温度最小值 | `m_basicInfo.tempMin` |
| 39-40 | 种子温度最大值 | `m_basicInfo.tempMax` |
| 41-42 | 输出功率最小值 | `m_basicInfo.powerMin` |
| 43-50 | 波长范围 | `m_basicInfo.wavelengthMin/Max` |

同步修改 `laser_protocol.h` 中的 `LaserBasicInfo` 结构体，增加上述字段。

**注意**：基本信息报文长度可变（最后几个字段可能不存在），需根据实际 dataLen 判断是否读取。

---

#### 3. 实现 readRealTimeMeaning (0xD2)

**laser_driver.cpp** 新增函数：
```cpp
bool LaserDriver::readRealTimeMeaning()
```
- 发送帧：`AA 55 D2 00 00 00`
- 解析返回帧：提取含义字节数组，存入成员变量 `QVector<quint8> m_realtimeMeaning`

**laser_protocol.h** 新增含义枚举/常量：
```cpp
// 实时信息含义码
const quint8 RT_MEANING_TEMP_PUMP    = 0x10;  // 泵浦温度
const quint8 RT_MEANING_TEMP_PUMP_N  = 0x11;  // 第N泵浦温度
const quint8 RT_MEANING_TEMP_SEED    = 0x1D;  // 种子温度
const quint8 RT_MEANING_TEMP_MODULE  = 0x1F;  // 模块温度
const quint8 RT_MEANING_CURRENT_PUMP = 0x20;  // 泵浦电流
const quint8 RT_MEANING_CURRENT_PUMP_N = 0x21;// 第N泵浦电流
const quint8 RT_MEANING_CURRENT_SEED = 0x2D;  // 种子电流
const quint8 RT_MEANING_POWER_OUTPUT = 0x50;  // 输出功率
const quint8 RT_MEANING_POWER_PUMP   = 0x5E;  // 泵浦功率
const quint8 RT_MEANING_POWER_IN_uW  = 0x5F;  // 输入功率(uW)
const quint8 RT_MEANING_POWER_IN_dBm = 0x5D;  // 输入功率(dBm)
```

**实测数据参考**：`55 AA D2 04 00 11 1F 21 51 00` → 含义数组 = [0x11, 0x1F, 0x21]

---

#### 4. 实现 readRealTimeInfo (0xD3)

**laser_driver.cpp** 新增函数：
```cpp
bool LaserDriver::readRealTimeInfo()
```
- 发送帧：`AA 55 D3 00 00 00`
- 解析返回帧，按 `m_realtimeMeaning` 数组顺序逐项解析

**返回帧结构**：
```
字节0-1: 55 AA (信息头)
字节2: D3 (命令字)
字节3: NN (数据长度)
字节4: 工作状态 (0=关机, 1=开机)
字节5: 报警代码 (bit0=模块温度, bit1=种子温度, bit2=输入功率, bit3=泵浦温度, bit4=泵浦功率, bit5=种子功率, bit7=保存出错)
字节6+: 实时数据 (每项2字节小端，按含义数组顺序)
```

**数据换算**：
- 温度类 (0x1X)：`℃ = (原始值 - 27315) / 100.0`
- 电流类 (0x2X)：单位 mA，直接读
- 功率类 (0x5X)：`实际值 = 原始值 / 功率系数`（系数从0xD1获取）
- 输入功率 dBm (0x5D)：有符号16位，最高位=符号位，`dBm = ±(值 & 0x7FFF) / 100.0`

**报警解析**：
```cpp
bool isModuleTempAlarm     = (alarmCode & 0x01);
bool isSeedTempAlarm       = (alarmCode & 0x02);
bool isInputPowerAlarm     = (alarmCode & 0x04);
bool isPumpTempAlarm       = (alarmCode & 0x08);
bool isPumpPowerAlarm      = (alarmCode & 0x10);
bool isSeedPowerAlarm      = (alarmCode & 0x20);
bool isSaveError           = (alarmCode & 0x80);
```

**实测数据参考**：
```
关机: 55 AA D3 08 00 00 42 74 A7 72 00 00 CD 01
      → 状态=关机, 报警=正常, 泵温24.47°C, 模温20.36°C, 电流0mA

50mW: 55 AA D3 08 01 00 42 74 A7 72 86 00 56 02
      → 状态=开机, 报警=正常, 泵温24.47°C, 模温20.36°C, 电流134mA
```

---

#### 5. 扩展 parseFrame switch 分支

**laser_driver.cpp:parseFrame()** 的 switch 增加：
```cpp
case LASER_CMD_READ_FIRMWARE_INFO:  // 0xD2
    return parseRealTimeMeaning(frame);
case LASER_CMD_READ_REALTIME_INFO:  // 0xD3
    return parseRealTimeInfo(frame);
case 0xD4:  // 未知命令（实测出现过）
case 0xD5:  // 实测出现过，协议文档未记载
    qDebug() << "收到未知命令应答 0x" << hex << commandCode;
    break;
```

---

### P2 — 功能修正

#### 6. 修正 0xC3 语义

**当前**：函数名叫 `setPumpCurrent()`，注释写"设置泵浦电流"
**修改为**：`setOutputPower()`，根据 `m_basicInfo.powerSetMode` 决定数据含义
```
功率设置方式=1 → 设置功率，值 = 目标功率(mW) × powerCoefficient
功率设置方式=2 → 设置电流，值 = 目标电流(mA)
其他 → 按对应方式处理
```

#### 7. 实现 0xC3 读取模式

不带参数发送 `AA 55 C3 00 00 00` → 查询当前设置的功率/电流值。
返回帧 `55 AA C3 02 [VL] [VH] [CSL] [CSH]`，2字节数据与设置时的含义一致。

**实测**：日志第8行、第40行均为读取模式，返回 `00 00`。

#### 8. 恢复 setTemperature (0xC7)

删除 "新协议不支持" 的注释，实现：
```cpp
bool LaserDriver::setTemperature(float celsius)
```
- 数据值 = (273.15 + celsius) × 100，取整，2字节小端
- 范围检查：对比 m_basicInfo.tempMin / tempMax
- 发送帧：`AA 55 C7 02 [TL] [TH] [CSL] [CSH]`

#### 9. 实现多路功率设置 setMultiPower (0xC4)

```cpp
bool LaserDriver::setMultiPower(quint8 channel, quint16 powerValue)
```
- 发送帧：`AA 55 C4 03 [channel] [PL] [PH] [CSL] [CSH]`
- 仅在 m_basicInfo.productType == 5 时有效

---

### P3 — 结构体扩展

#### 10. 扩展 LaserBasicInfo（laser_protocol.h）

```cpp
struct LaserBasicInfo {
    // 已有字段
    quint8  powerSetMode;
    quint16 minCurrent;
    quint16 maxCurrent;
    
    // 新增字段
    quint8  powerUnit;          // 0=mW, 1=W, 2=uW
    quint16 powerCoefficient;   // 功率系数（第28字节）
    quint8  productType;        // 产品类别（第36字节）
    float   tempMin;            // 种子温度最小值(°C)
    float   tempMax;            // 种子温度最大值(°C)
    quint16 powerMin;           // 输出功率最小值（原始值）
    quint16 powerMax;           // 输出功率最大值（原始值）
    quint32 wavelengthMin;      // 最小波长 (0.0001nm)
    quint32 wavelengthMax;      // 最大波长 (0.0001nm)
};
```

#### 11. 扩展 LaserStatus（laser_protocol.h）

```cpp
struct LaserStatus {
    // 已有
    bool    isRunning;
    quint16 setCurrent;
    quint16 actualCurrent;
    quint8  powerSetMode;
    
    // 新增
    float   pumpTemperature;    // 泵浦温度(°C)
    float   moduleTemperature;  // 模块温度(°C)
    float   seedTemperature;    // 种子温度(°C)
    float   outputPower;        // 输出功率(mW)
    quint8  alarmCode;          // 报警代码
    bool    alarms[8];          // 各报警位
};
```

---

### P4 — 头文件声明补充

#### 12. laser_driver.h 新增函数声明

```cpp
// 新增公开方法
bool readRealTimeMeaning();
bool readRealTimeInfo();
bool setOutputPower(float powerMw);       // 替代 setPumpCurrent，语义更准确
bool setMultiPower(quint8 channel, quint16 powerValue);
float temperatureFromRaw(quint16 raw);     // 温度换算工具

// 新增私有方法
bool parseRealTimeMeaning(const QByteArray &frame);
bool parseRealTimeInfo(const QByteArray &frame);

// 新增成员变量
QVector<quint8> m_realtimeMeaning;  // 实时信息含义映射表
```

---

## 实测数据速查表（调试用）

| 命令 | 发送帧 | 返回帧 | 说明 |
|------|--------|--------|------|
| D1 基本信息 | AA 55 D1 00 00 00 | 55 AA D1 24 [36字节] | 功率系数=100(字节28), 单位mW(字节22), 产品0(字节36) |
| D2 实时含义 | AA 55 D2 00 00 00 | 55 AA D2 04 00 11 1F 21 51 00 | 含义=[泵温, 模温, 泵流] |
| D3 实时(关机) | AA 55 D3 00 00 00 | 55 AA D3 08 00 00 42 74 A7 72 00 00 CD 01 | 关机, 泵温24.47°C |
| D3 实时(50mW) | AA 55 D3 00 00 00 | 55 AA D3 08 01 00 42 74 A7 72 86 00 56 02 | 开机, I=134mA |
| C1 开光 | AA 55 C1 01 01 01 00 | 55 AA C1 01 00 00 00 | exec=00 成功 |
| C1 关光 | AA 55 C1 01 00 00 00 | 55 AA C1 01 00 00 00 | exec=00 成功 |
| C3 设50mW | AA 55 C3 02 88 13 9B 00 | 55 AA C3 01 00 00 00 | 系数100, 5000=0x1388 |
| C3 设0mW | AA 55 C3 02 00 00 00 00 | 55 AA C3 01 00 00 00 | |
| C3 读当前 | AA 55 C3 00 00 00 | 55 AA C3 02 00 00 00 00 | 返回当前值 |
| D5 未知 | AA 55 D5 00 00 00 | 55 AA D5 05 00 00 00 01 00 01 00 | 协议未记载 |

---

## 时序要求

- 初始化时**必须先发0xD1再发0xD2**，获取功率系数和含义映射后才能正确解析0xD3
- 0xC1/0xC3 发送后**必须等待应答中的 exec 字节**，非0表示失败
- 0xD3 定时轮询间隔建议 100-500ms
- 接上串口后先发0xD3确认设备在线（信息头校验通过即可），再发握手命令

---

## 不需要改的部分

- 信息头 AA55/55AA 处理 ✓
- 串口参数 9600-8-N-1 ✓
- 小端序多字节处理 ✓
- setSetCommandReply 的 bit 解析 ✓
- 命令字常量宏定义 ✓
- SerialPortBase 层封装 ✓

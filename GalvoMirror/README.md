# 振镜控制卡模块 (GalvoMirror)

## 概述

本模块实现了思特GMC振镜控制卡的控制功能，支持2D/3D打标、在线/脱机打标等功能。

## 硬件信息

- **控制卡型号**: 思特GMC4控制卡
- **通信方式**: TCP/IP以太网通信
- **默认IP地址**: 172.18.34.227
- **支持协议**: XY2-100、SPI、SL2

## 文件说明

- `galvo_mirror.h/cpp` - 振镜控制卡主类实现
- `galvo_protocol.h` - 协议定义和数据结构
- `library/` - 厂商提供的SDK库文件
  - `HM_HashuScan.h` - 控制卡通信接口
  - `HM_HashuUDM.h` - UDM打标文件生成接口
  - `HM_HashuScan.lib` - 静态库
  - `HM_Comm.lib` - 通信库

## 主要功能

### 1. 设备连接管理
- 搜索控制卡设备
- 通过IP地址或索引连接
- 获取设备状态和信息

### 2. 打标控制
- 下载打标文件（同步/异步）
- 开始/停止/暂停/继续打标
- 查询打标进度

### 3. 参数设置
- 设置打标偏移和旋转
- 设置坐标系和打标范围
- 配置打标参数（速度、延时等）

### 4. 振镜控制
- 振镜位置跳转
- 红光预览
- 校正表管理

### 5. IO控制
- 数字IO输入/输出控制
- 模拟量输出控制（0~10V）
- 激光器报警状态读取

### 6. 脱机打标
- 单文档脱机（无需SD卡）
- 多文档脱机（需要SD卡）
- 支持最多16个文档

## 使用示例

### 基本连接流程

```cpp
// 1. 创建振镜控制卡对象
GalvoMirror* galvo = new GalvoMirror();

// 2. 初始化
galvo->initialize();

// 3. 搜索设备
galvo->searchDevices();

// 4. 连接设备
galvo->connectByIP("172.18.34.227");

// 5. 检查连接状态
if (galvo->isConnected()) {
    qDebug() << "振镜控制卡连接成功";
}
```

### 打标流程

```cpp
// 1. 下载打标文件
galvo->downloadMarkFileSync("D:/mark.udm");

// 2. 设置参数（可选）
galvo->setOffset(0, 0, 0);
galvo->setRotation(0, 0, 0);

// 3. 开始打标
galvo->startMark();

// 4. 监听打标完成信号
connect(galvo, &GalvoMirror::markFinished, []() {
    qDebug() << "打标完成";
});
```

### IO控制示例

```cpp
// 设置输出
galvo->setOutputOn(0);   // OUT0输出高电平(5V)
galvo->setOutputOff(0);  // OUT0输出低电平(0V)

// 读取输入
int inputStatus = galvo->getInputStatus();
// 转换为二进制判断各个输入状态
bool in0 = inputStatus & (1 << 0);
bool in1 = inputStatus & (1 << 1);
```

### 脱机打标示例

```cpp
// 单文档脱机
if (!galvo->hasSDCard()) {
    galvo->setBurnMode(BURN_SINGLE);
    galvo->downloadMarkFileSync("D:/mark.udm");
    galvo->burnMarkFile(true);  // 固化到Flash
}

// 多文档脱机
if (galvo->hasSDCard()) {
    galvo->setBurnMode(BURN_MULTI);
    
    for (int i = 0; i < 3; i++) {
        galvo->setBurnIndex(i);
        galvo->setStartBurnFlag();
        galvo->downloadMarkFileSync(QString("D:/mark%1.udm").arg(i));
        galvo->burnMarkFile(true);
        
        // 等待固化完成
        while (!galvo->getBurnOverFlag()) {
            QThread::msleep(100);
        }
    }
}
```

## 网络配置

### 电脑IP设置
- IP地址: 172.18.34.2 ~ 172.18.34.123（避开227和226）
- 子网掩码: 255.255.255.0
- 避免使用: 172.18.34.227（控制卡IP）和 172.18.34.226（初始化临时IP）

### 多卡控制
- 支持同时控制多张控制卡
- 需要为每张卡设置不同的IP地址
- 使用交换机连接多张卡

## 打标参数说明

### MarkParameter结构体

```cpp
struct MarkParameter {
    unsigned int MarkSpeed;         // 打标速度(mm/s)
    unsigned int JumpSpeed;         // 跳转速度(mm/s)
    unsigned int MarkDelay;         // 打标延时(us)
    unsigned int JumpDelay;         // 跳转延时(us)
    unsigned int PolygonDelay;      // 转弯延时(us)
    unsigned int MarkCount;         // 打标次数
    float LaserOnDelay;             // 开激光延时(us)
    float LaserOffDelay;            // 关激光延时(us)
    float LaserPower;               // 激光能量(0~100)
    float Frequency;                // Q频率(kHz)
    float DutyCycle;                // 占空比(0~1)
    // ... 更多参数
};
```

### 参数调节建议

- **打标速度**: 越大越快，但效果可能变差
- **跳转速度**: 不出光时的移动速度
- **激光能量**: 0~100，能量越大痕迹越明显
- **Q频率**: 激光出光频率，频率越大激光点越多
- **开激光延时**: 太大会缺失笔画开始，太小会提前出光
- **关激光延时**: 太大会拖尾，太小会笔画不够长

## 注意事项

1. **DLL依赖**: 确保HM_Comm.dll、HM_HashuScan.dll和System.ini在可执行文件目录
2. **IP冲突**: 避免电脑IP与控制卡IP冲突
3. **脱机文件大小**: 单文档脱机时文件不能超过2MB
4. **Z轴范围**: 3D打标时Z值范围为-4~4mm
5. **消息处理**: 在Qt中需要特殊处理Windows消息循环
6. **校正表**: 控制卡已固化校正表，无需重复加载

## 开发状态

### 已完成
- [x] 基础类框架
- [x] 接口定义
- [x] 协议定义
- [x] 文档编写

### 待实现
- [ ] DLL函数调用实现
- [ ] Windows消息处理
- [ ] UDM文件生成
- [ ] 3D打标功能
- [ ] 闭环控制
- [ ] SkyWriting功能

## 参考文档

- `思特控制卡二次开发说明V2.3.txt` - 完整的开发文档
- 厂商官网: 深圳市思特光学科技有限公司

## 技术支持

如有问题，请参考开发文档或联系厂商技术支持。

# 通信基类模块

## 功能概述
提供串口通信和设备管理的基类，所有设备模块都继承自这些基类。

## 架构设计
```
DeviceBase (设备基类)
    ├── SerialPortBase (串口基类)
    │   ├── LaserDriver
    │   ├── StageController
    │   └── DelayLine
    └── Spectrometer (USB/串口)
```

## 文件说明
- `device_base.h/cpp` - 设备基类
- `serial_port_base.h/cpp` - 串口通信基类
- `error_codes.h` - 错误码定义

## 主要功能

### DeviceBase（设备基类）
- ✅ 设备状态管理
- ✅ 连接/断开控制
- ✅ 错误处理
- ✅ 信号/槽机制

### SerialPortBase（串口基类）
- ✅ 串口配置
- ✅ 数据收发
- ✅ 超时处理
- ✅ 缓冲区管理

## 设备状态枚举
```cpp
enum class DeviceStatus {
    Disconnected,  // 未连接
    Connected,     // 已连接
    Busy,          // 忙碌
    Error          // 错误
};
```

## 串口配置结构
```cpp
struct SerialConfig {
    QString portName;      // 串口名称
    qint32 baudRate;       // 波特率
    int dataBits;          // 数据位
    int stopBits;          // 停止位
    int parity;            // 校验位
};
```

## 使用示例

### 继承 SerialPortBase
```cpp
class MyDevice : public SerialPortBase {
    Q_OBJECT
public:
    MyDevice(QObject *parent = nullptr);
    
protected:
    void processReceivedData(const QByteArray &data) override;
};
```

### 实现数据处理
```cpp
void MyDevice::processReceivedData(const QByteArray &data) {
    // 解析接收到的数据
    // 发射信号通知上层
}
```

## 错误码定义
参见 `error_codes.h`：
- `ERR_PORT_OPEN_FAILED` - 串口打开失败
- `ERR_WRITE_FAILED` - 写入失败
- `ERR_READ_TIMEOUT` - 读取超时
- `ERR_INVALID_RESPONSE` - 无效响应

## 参考文档
- 错误码：`error_codes.h`
- 设备基类：`device_base.h`
- 串口基类：`serial_port_base.h`

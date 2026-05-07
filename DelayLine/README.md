# 延迟线控制模块

## 功能概述
控制光学延迟线，实现精确的时间延迟控制。

## 通信协议
- 串口通信
- 波特率：9600 bps
- 数据位：8
- 停止位：1
- 校验位：无

## 主要功能
- ✅ 延迟时间设置（皮秒级）
- ✅ 实时延迟反馈
- ✅ 延迟值监控
- ✅ 预设管理

## 文件说明
- `delay_line.h/cpp` - 延迟线控制类实现
- `delay_protocol.h` - 协议定义

## 使用示例

### 连接延迟线
```cpp
DelayLine *delayLine = new DelayLine();
delayLine->openPort("COM6", 9600);
```

### 设置延迟时间
```cpp
delayLine->setDelay(1000.0);  // 1000 ps
```

### 监听延迟变化
```cpp
connect(delayLine, &DelayLine::delayChanged,
        this, &Integration::onDelayChanged);
```

## 控制精度
- 延迟精度：1 ps
- 延迟范围：0-10000 ps

## 应用场景
- 泵浦-探测实验
- 时间分辨光谱
- 超快光学测量

## 参考文档
- 协议定义：`delay_protocol.h`

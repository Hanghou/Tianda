# 大族思特 MEMS 振镜控制卡 — Qt(C++) 二次开发文档

## 一、概述

本文档基于原 C# TCP 客户端项目，提供 Qt/C++ 二次开发所需的完整接口说明，包括通信协议、数据结构、控制流程和 UI 映射。

**控制卡参数：**
- IP 地址：`172.18.34.227`（默认）
- 心跳端口：`5998`（UDP 广播）
- 图形参数端口：`6002`（TCP）
- 控制指令端口：`6003`（TCP）

---

## 二、通信架构

```
┌─────────────┐         UDP:5998          ┌──────────────┐
│             │◄────────心跳检测──────────►│              │
│   上位机    │         TCP:6002           │   控制卡     │
│  (Qt客户端) │────────图形/参数──────────►│ 172.18.34.227│
│             │         TCP:6003           │              │
│             │◄───────控制指令───────────►│              │
└─────────────┘                            └──────────────┘
```

---

## 三、C++ 数据结构定义（Qt 适用）

### 3.1 控制指令结构体（端口 6003）

```cpp
#pragma pack(push, 1)

// 控制操作枚举
enum ControlMode {
    CMD_START    = 'e',  // 101 - 开始打标
    CMD_STOP     = 'r',  // 114 - 停止/复位
    CMD_PAUSE    = 'p',  // 112 - 暂停
    CMD_CONTINUE = 'c',  // 99  - 继续
};

// 控制指令帧 - 通过 TCP:6003 发送
struct ControlAction {
    uint32_t nDebugType;   // 固定值 114 ('r')
    uint32_t nOperation;   // 操作码，见 ControlMode 枚举
    uint32_t uParam;       // 保留，填0
    uint32_t uStatus;      // 保留，填0
    uint32_t uReserved;    // 保留，填0
};
```

### 3.2 图形参数结构体（端口 6002）

```cpp
// 激光参数
struct stLaserPara {
    uint32_t nLaserOnDelay;     // 开激光延时(μs)
    uint32_t nLaserOffDelay;    // 关激光延时(μs)
    uint32_t nFPSDelay;         // 首脉冲压缩延时(μs)
    uint32_t nFPSLength;        // 首脉冲压缩长度(μs)
    uint32_t nQDelay;           // Q延时(μs)
    float    DutyCycle;         // 出光占空比(0~1)
    float    Frequency;         // 出光频率(kHz)
    float    StandbyDutyCycle;  // 待机占空比(0~1)
    float    StandbyFrequency;  // 待机频率(kHz)
    float    nLaserPower;       // 激光功率(0~100)
};

// 振镜运动参数
struct STMarkSetting {
    int32_t nMarkV;            // 打标速度(mm/s)
    int32_t nMark2MarkDelay;   // 拐弯延时(μs)
    int32_t nJumpDelay;        // 跳转延时(μs)
    int32_t nMarkDelay;        // 打标延时(μs)
    int32_t nJumpV;            // 跳转速度(mm/s)
    int32_t ScanTimes;         // 扫描次数
};

// 图形信息
struct stShpaeInfo {
    float Shape;              // 图形类型: 0x01=点, 0x02=线, 0x03=圆, 0x08=多点阵列
    float PointX;             // 点_X坐标
    float PointY;             // 点_Y坐标
    float Point_LaseronTime;  // 点出光时间(ms)
    float Line_StartX;        // 线_起点X
    float Line_StartY;        // 线_起点Y
    float Line_EndX;          // 线_终点X
    float Line_EndY;          // 线_终点Y
    float CircleX;            // 圆_中心X
    float CircleY;            // 圆_中心Y
    float Circle_Radius;      // 圆_半径R
};

// IO控制
struct stIOControl {
    uint32_t nRedLightEnable;   // 红光预览使能 (0=关, 1=开)
    uint32_t nReadyDownEanble;  // 准备按钮按下 (0=未按, 1=按下)
    uint32_t nLightLevel;       // 光照等级
};

// 多点阵列参数
struct stPointPara {
    int32_t PointCount;           // 点数量(最大128)
    float   PointData_xPos[128];  // X坐标数组
    float   PointData_yPos[128];  // Y坐标数组
};

// 通讯数据帧
struct stCommFrame {
    int32_t      nHeader;      // 帧头标识，固定 0x5A5AA5A5
    stLaserPara  LaserPara;    // 激光参数
    STMarkSetting stmark;      // 振镜运动参数
    stShpaeInfo  ShpaeInfo;    // 图形信息
    stIOControl  IOControl;    // IO控制
    stPointPara  PointPara;    // 多点阵列
};

// 打标参数总帧 - 通过 TCP:6002 发送
struct MarkParameter {
    int32_t    m_nCmdType;     // 命令类型，固定 0x08
    char       cSysCmd;        // 系统命令，固定 0x14 (20)
    char       cStatus;        // 状态，固定 1
    int16_t    uReserved;      // 保留
    stCommFrame stUnitFrame;   // 数据帧
};
```

### 3.3 寄存器读写结构体（端口 6002）

```cpp
// 寄存器操作命令
struct structComm_RegCommand {
    char     cCmd;        // 0x03=读, 0x06=写
    char     cDataType;   // 数据类型: 0=INT, 1=UINT
    char     cStatus;     // 状态
    char     cReserved;   // 保留
    uint32_t uAddr;       // 寄存器地址
    int32_t  udData;      // 数据值(写时有效，读时为返回值)
};

// 寄存器命令帧 - 通过 TCP:6002 发送
struct structComm_CommandFrame {
    uint32_t m_nCmdType;              // 命令类型，固定 1
    uint32_t m_nCmdCount;             // 命令数量
    structComm_RegCommand RegCommand; // 寄存器命令
};

---

## 四、Qt 功能函数接口说明

### 4.1 心跳检测（UDP 5998）

**功能**：判断控制卡是否在线，软件启动时自动调用。

```cpp
// === 头文件成员变量 ===
QUdpSocket *m_udpSocket;
QTimer     *m_heartbeatTimer;
uint32_t    m_ticks = 0;
bool        m_connectStatus = false;
QString     m_cardIP = "172.18.34.227";
int         m_portHeartbeat = 5998;

// === 初始化心跳连接 ===
bool heartbeatConnect()
{
    m_udpSocket = new QUdpSocket(this);
    // 绑定本机 172.18.34.x 网段的IP到5998端口
    QList<QHostAddress> addrList = QNetworkInterface::allAddresses();
    for (const QHostAddress &addr : addrList) {
        if (addr.toString().startsWith("172.18.34")) {
            m_udpSocket->bind(addr, m_portHeartbeat);
            connect(m_udpSocket, &QUdpSocket::readyRead, this, &MainWindow::onUdpReadyRead);
            // 定时器每隔1秒发送心跳
            m_heartbeatTimer = new QTimer(this);
            connect(m_heartbeatTimer, &QTimer::timeout, this, &MainWindow::onHeartbeatTimeout);
            m_heartbeatTimer->start(1000);
            return true;
        }
    }
    return false;
}

// === 接收UDP数据 ===
void onUdpReadyRead()
{
    while (m_udpSocket->hasPendingDatagrams()) {
        QByteArray data;
        data.resize(m_udpSocket->pendingDatagramSize());
        m_udpSocket->readDatagram(data.data(), data.size());
        m_ticks++;  // 收到回复，计数+1
    }
}

// === 心跳定时器回调 ===
void onHeartbeatTimeout()
{
    uint32_t currentTicks = m_ticks;
    // 向控制卡发送任意数据
    m_udpSocket->writeDatagram("0", 1, QHostAddress(m_cardIP), m_portHeartbeat);
    QThread::msleep(100);
    // 判断是否收到回复
    m_connectStatus = (currentTicks != m_ticks);
}
```

### 4.2 TCP 连接（端口 6002 + 6003）

**功能**：建立与控制卡的双通道 TCP 连接。对应界面"连接"按钮。

```cpp
// === 头文件成员变量 ===
QTcpSocket *m_shapeSocket;   // 6002 - 图形参数
QTcpSocket *m_actionSocket;  // 6003 - 控制指令
int m_portShape = 6002;
int m_portAction = 6003;

// === 连接按钮槽函数 ===
void on_btnConnect_clicked()
{
    if (!m_connectStatus) {
        QMessageBox::warning(this, "错误", "未找到控制卡");
        return;
    }

    m_shapeSocket = new QTcpSocket(this);
    m_actionSocket = new QTcpSocket(this);

    // 连接图形参数端口
    m_shapeSocket->connectToHost(m_cardIP, m_portShape);
    if (!m_shapeSocket->waitForConnected(3000)) {
        QMessageBox::warning(this, "错误", "6002端口连接失败");
        return;
    }

    // 连接控制指令端口
    m_actionSocket->connectToHost(m_cardIP, m_portAction);
    if (!m_actionSocket->waitForConnected(3000)) {
        QMessageBox::warning(this, "错误", "6003端口连接失败");
        return;
    }

    // 绑定接收信号
    connect(m_actionSocket, &QTcpSocket::readyRead, this, &MainWindow::onActionDataReceived);
    connect(m_shapeSocket, &QTcpSocket::readyRead, this, &MainWindow::onShapeDataReceived);

    ui->btnConnect->setEnabled(false);
    ui->textReceive->append("连接成功！");
}

// === 断开按钮槽函数 ===
void on_btnDisconnect_clicked()
{
    if (m_actionSocket && m_actionSocket->state() == QAbstractSocket::ConnectedState)
        m_actionSocket->disconnectFromHost();
    if (m_shapeSocket && m_shapeSocket->state() == QAbstractSocket::ConnectedState)
        m_shapeSocket->disconnectFromHost();
    ui->btnConnect->setEnabled(true);
}
```

### 4.3 数据接收（控制卡反馈）

```cpp
// === 接收控制指令反馈(6003) ===
void onActionDataReceived()
{
    QByteArray data = m_actionSocket->readAll();
    if (data.isEmpty()) return;

    switch (data.at(0)) {
    case 'r':
        ui->textReceive->append("开始运行中...");
        break;
    case 'f':
        ui->textReceive->append("打标完成");
        break;
    }
}

// === 接收图形参数反馈(6002) ===
void onShapeDataReceived()
{
    QByteArray data = m_shapeSocket->readAll();
    if (data.isEmpty()) return;

    switch (data.at(0)) {
    case 'r':
        ui->textReceive->append("图形参数已接收，运行中...");
        break;
    case 'f':
        ui->textReceive->append("图形参数处理完成");
        break;
    }
}
```

### 4.4 发送图形参数（端口 6002）

**功能**：设置打标图形、激光参数、振镜参数后发送到控制卡。对应界面"设置图形"/"开始打标"按钮。

```cpp
// === 通用发送函数 ===
bool sendShapeMsg(const void *data, int size)
{
    if (!m_shapeSocket || m_shapeSocket->state() != QAbstractSocket::ConnectedState)
        return false;
    qint64 sent = m_shapeSocket->write(reinterpret_cast<const char*>(data), size);
    m_shapeSocket->flush();
    return (sent == size);
}

bool sendControlMsg(const void *data, int size)
{
    if (!m_actionSocket || m_actionSocket->state() != QAbstractSocket::ConnectedState)
        return false;
    qint64 sent = m_actionSocket->write(reinterpret_cast<const char*>(data), size);
    m_actionSocket->flush();
    return (sent == size);
}

// === 构造打标参数帧 ===
MarkParameter buildMarkParameter(int shapeType)
{
    // 激光参数
    stLaserPara laser = {};
    laser.nLaserOnDelay = 110;
    laser.nLaserOffDelay = 120;
    laser.nFPSDelay = 10;
    laser.nFPSLength = 20;
    laser.nQDelay = 5;
    laser.DutyCycle = 0.5f;
    laser.Frequency = 50.0f;
    laser.StandbyDutyCycle = 0.2f;
    laser.StandbyFrequency = 10.0f;
    laser.nLaserPower = 50.0f;

    // 振镜运动参数
    STMarkSetting mark = {};
    mark.nMarkV = 100;
    mark.nMark2MarkDelay = 0;
    mark.nJumpDelay = 0;
    mark.nMarkDelay = 0;
    mark.ScanTimes = 1;

    // 图形信息
    stShpaeInfo shape = {};
    shape.Shape = (float)shapeType;  // 0x01=点, 0x02=线, 0x03=圆
    shape.PointX = 0; shape.PointY = 0;
    shape.Line_StartX = -10; shape.Line_StartY = 10;
    shape.Line_EndX = 10; shape.Line_EndY = -10;
    shape.CircleX = 0; shape.CircleY = 0;
    shape.Circle_Radius = 5;
    shape.Point_LaseronTime = 1000;

    // IO控制
    stIOControl io = {};
    io.nRedLightEnable = 0;
    io.nReadyDownEanble = 1;

    // 多点阵列（如需要）
    stPointPara point = {};
    memset(&point, 0, sizeof(point));
    point.PointCount = 0;  // 非多点模式时设为0

    // 组装通讯帧
    stCommFrame comm = {};
    comm.nHeader = 0x5A5AA5A5;  // 固定帧头
    comm.LaserPara = laser;
    comm.stmark = mark;
    comm.ShpaeInfo = shape;
    comm.IOControl = io;
    comm.PointPara = point;

    // 组装总帧
    MarkParameter param = {};
    param.m_nCmdType = 0x08;    // 固定：图形参数命令
    param.cSysCmd = 0x14;       // 固定：系统命令20
    param.cStatus = 1;          // 固定
    param.uReserved = 0;
    param.stUnitFrame = comm;

    return param;
}
```

### 4.5 开始打标（端口 6003）

**功能**：先发送图形参数，再发送开始指令。对应界面"开始打标"按钮。

```cpp
void on_btnStart_clicked()
{
    if (!m_connectStatus) return;

    // 1. 先发送图形参数到 6002
    MarkParameter markParam = buildMarkParameter(m_currentShape);
    sendShapeMsg(&markParam, sizeof(markParam));

    // 2. 再发送开始指令到 6003
    ControlAction action = {};
    action.nDebugType = 114;            // 固定值 'r'
    action.nOperation = CMD_START;      // 'e' = 开始
    sendControlMsg(&action, sizeof(action));
}
```

### 4.6 停止打标（端口 6003）

**功能**：发送停止/复位指令。对应界面"停止"按钮。

```cpp
void on_btnStop_clicked()
{
    if (!m_connectStatus) return;

    ControlAction action = {};
    action.nDebugType = 114;            // 固定值
    action.nOperation = CMD_STOP;       // 'r' = 停止/复位
    sendControlMsg(&action, sizeof(action));
}
```

### 4.7 寄存器读写（端口 6002）

**功能**：读写控制卡内部寄存器，用于高级调试。

```cpp
void readRegister(uint32_t addr)
{
    structComm_CommandFrame frame = {};
    frame.m_nCmdType = 1;
    frame.m_nCmdCount = 1;
    frame.RegCommand.cCmd = 0x03;       // 读
    frame.RegCommand.cDataType = 0x00;  // INT类型
    frame.RegCommand.cStatus = 0x01;
    frame.RegCommand.uAddr = addr;
    frame.RegCommand.udData = 0;
    sendShapeMsg(&frame, sizeof(frame));
}

void writeRegister(uint32_t addr, int32_t value)
{
    structComm_CommandFrame frame = {};
    frame.m_nCmdType = 1;
    frame.m_nCmdCount = 1;
    frame.RegCommand.cCmd = 0x06;       // 写
    frame.RegCommand.cDataType = 0x00;
    frame.RegCommand.cStatus = 0x01;
    frame.RegCommand.uAddr = addr;
    frame.RegCommand.udData = value;
    sendShapeMsg(&frame, sizeof(frame));
}
```

---

## 五、UI 界面按钮与功能映射

### 5.1 界面控件清单

| Qt控件名称 | 类型 | 功能 | 对应槽函数 |
|-----------|------|------|-----------|
| `btnConnect` | QPushButton | 连接控制卡 | `on_btnConnect_clicked()` |
| `btnDisconnect` | QPushButton | 断开连接 | `on_btnDisconnect_clicked()` |
| `btnStart` | QPushButton | 开始打标 | `on_btnStart_clicked()` |
| `btnStop` | QPushButton | 停止打标 | `on_btnStop_clicked()` |
| `btnDot` | QPushButton | 选择点图形 | `on_btnDot_clicked()` |
| `btnCircle` | QPushButton | 选择圆图形 | `on_btnCircle_clicked()` |
| `textReceive` | QTextEdit | 显示接收消息 | — |

### 5.2 按钮状态逻辑

```
初始状态：btnConnect=可用, btnDot=可用, btnCircle=可用
连接成功后：btnConnect=禁用
选择点后：btnDot=禁用, btnCircle=可用
选择圆后：btnDot=可用, btnCircle=禁用
开始打标后：btnDot=禁用, btnCircle=禁用
停止后：恢复已选图形按钮状态
断开后：所有按钮恢复初始状态
```

---

## 六、输入参数说明

### 6.1 激光参数范围

| 参数 | 类型 | 范围 | 单位 | 说明 |
|------|------|------|------|------|
| nLaserOnDelay | uint32 | 0~65535 | μs | 开激光延时 |
| nLaserOffDelay | uint32 | 0~65535 | μs | 关激光延时 |
| nFPSDelay | uint32 | 0~65535 | μs | 首脉冲压缩延时 |
| nFPSLength | uint32 | 0~65535 | μs | 首脉冲压缩长度 |
| nQDelay | uint32 | 0~65535 | μs | Q延时 |
| DutyCycle | float | 0~1 | — | 出光占空比 |
| Frequency | float | >0 | kHz | 出光频率 |
| StandbyDutyCycle | float | 0~1 | — | 待机占空比 |
| StandbyFrequency | float | >0 | kHz | 待机频率 |
| nLaserPower | float | 0~100 | % | 激光功率 |

### 6.2 振镜运动参数范围

| 参数 | 类型 | 范围 | 单位 | 说明 |
|------|------|------|------|------|
| nMarkV | int32 | >0 | mm/s | 打标速度 |
| nMark2MarkDelay | int32 | ≥0 | μs | 拐弯延时 |
| nJumpDelay | int32 | ≥0 | μs | 跳转延时 |
| nMarkDelay | int32 | ≥0 | μs | 打标延时 |
| nJumpV | int32 | >0 | mm/s | 跳转速度 |
| ScanTimes | int32 | ≥1 | 次 | 扫描次数 |

### 6.3 图形参数

| 图形类型 | Shape值 | 必填参数 |
|----------|---------|----------|
| 点 | 0x01 | PointX, PointY, Point_LaseronTime |
| 线 | 0x02 | Line_StartX/Y, Line_EndX/Y |
| 圆 | 0x03 | CircleX, CircleY, Circle_Radius |
| 多点阵列 | 0x08 | PointCount, PointData_xPos[], PointData_yPos[] |

### 6.4 坐标系说明

- 坐标单位：mm（毫米）
- 原点：振镜中心位置
- X轴：水平方向（正方向向右）
- Y轴：垂直方向（正方向向上）
- 多点阵列最大支持 128 个点

---

## 七、Qt 项目所需头文件

```cpp
#include <QTcpSocket>
#include <QUdpSocket>
#include <QTimer>
#include <QThread>
#include <QNetworkInterface>
#include <QHostAddress>
#include <QMessageBox>
#include <cstring>   // memset, memcpy
#include <cstdint>   // uint32_t, int32_t
```

---

## 八、注意事项

1. **字节对齐**：所有结构体必须使用 `#pragma pack(push, 1)` 确保 1 字节对齐，否则与控制卡通信会出错。
2. **字节序**：控制卡使用小端序（Little-Endian），x86/x64 平台默认即为小端，无需转换。若在 ARM 大端平台开发需做字节序转换。
3. **发送顺序**：开始打标时必须**先发图形参数(6002)，再发控制指令(6003)**，否则控制卡无法正确执行。
4. **心跳超时**：建议心跳间隔 1 秒，超时判定 100ms。连续 3 次超时可认为控制卡离线。
5. **线程安全**：Qt 的信号槽机制天然支持跨线程通信，无需像 C# 版本那样手动 Invoke 委托。
6. **连接前检查**：发送任何数据前必须确认 `m_connectStatus == true` 且 Socket 处于 Connected 状态。
7. **多点阵列**：`stPointPara` 结构体较大（约 1028 字节），发送时确保网络缓冲区足够。

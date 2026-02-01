#ifndef SPECTROMETER_PROTOCOL_H
#define SPECTROMETER_PROTOCOL_H

#include <QByteArray>
#include <QVector>
#include <QtGlobal>

/**
 * @brief 光谱仪串口通讯协议
 * 基于OPTOSKY ATP系列光纤光谱仪通讯协议 V2.3
 */

// 命令ID定义（基于ATP系列协议V2.3）
namespace SpectrometerCmd {
    const quint8 GET_TEMPERATURE = 0x01;        // 获取电路板温度
    const quint8 GET_PN = 0x03;                 // 获取PN号
    const quint8 GET_SN = 0x04;                 // 获取SN号
    const quint8 GET_PRODUCTION_DATE = 0x06;    // 获取生产日期
    const quint8 GET_MANUFACTURER = 0x09;       // 获取厂家信息
    const quint8 GET_PIXEL_LENGTH = 0x0A;       // 获取像素长度
    const quint8 SET_TEC_TEMP = 0x12;           // 设置TEC温度
    const quint8 GET_TEC_TEMP = 0x13;           // 获取TEC温度
    const quint8 SET_INTEGRATION_TIME = 0x14;   // 设置积分时间
    const quint8 START_SCAN_ASYNC = 0x16;       // 开启CCD扫描（异步方式）
    const quint8 GET_SPECTRUM_DATA = 0x17;      // 读取CCD采集数据（配合0x16使用）
    const quint8 CCD_CONTINUOUS_SCAN = 0x19;    // CCD连续采集控制
    const quint8 START_SCAN_SYNC = 0x1E;        // 开启CCD扫描（同步方式，软件触发）
    const quint8 START_EXTERNAL_TRIGGER = 0x1F; // 开启外部触发采集功能
    const quint8 CAPTURE_DARK_ASYNC = 0x23;     // 采集暗电流（异步方式）
    const quint8 SET_AVERAGE_TIMES = 0x28;      // 设置采集平均次数
    const quint8 CAPTURE_DARK_SYNC = 0x2F;      // 采集暗电流（同步方式）
    const quint8 GET_OPTICAL_TEMP = 0x35;       // 获取光学平台温度
    const quint8 GET_INTEGRATION_TIME = 0x41;   // 获取当前积分时间
    const quint8 GET_MAX_INTEGRATION_TIME = 0x42; // 获取最大积分时间
    const quint8 GET_MIN_INTEGRATION_TIME = 0x43; // 获取最小积分时间
    const quint8 GET_DEVICE_PROPERTY = 0x46;    // 获取设备属性
    const quint8 GET_WAVELENGTH_COEFF = 0x55;   // 获取波长标定系数
    const quint8 CONTROL_LAMP = 0x60;           // 控制Lamp输出电平状态
    const quint8 SET_GPIO = 0x61;               // 设置GPIO引脚输出电平
    const quint8 SET_SHUTTER = 0x62;            // 设置光开关快门状态
    const quint8 STOP_EXTERNAL_TRIGGER = 0xF1;  // 关闭外部触发采集功能
}

/**
 * @brief 协议帧结构
 * 格式: Head(2B) + Length(2B) + Cmd(1B) + Data(N) + Checksum(1B)
 */
struct SpectrometerFrame {
    static const quint8 HEADER_1 = 0xAA;
    static const quint8 HEADER_2 = 0x55;
    
    quint16 length;      // 从Length到Checksum的字节数
    quint8 cmd;          // 命令ID
    QByteArray data;     // 数据（可选）
    quint8 checksum;     // 校验和
    
    /**
     * @brief 构建发送帧
     * @return 完整的字节数组
     */
    QByteArray toByteArray() const {
        QByteArray frame;
        
        // 帧头
        frame.append(HEADER_1);
        frame.append(HEADER_2);
        
        // 长度（大端序）
        frame.append((length >> 8) & 0xFF);
        frame.append(length & 0xFF);
        
        // 命令
        frame.append(cmd);
        
        // 数据
        frame.append(data);
        
        // 校验和
        frame.append(checksum);
        
        return frame;
    }
    
    /**
     * @brief 解析接收帧
     * @param raw 原始字节数组
     * @param frame 输出的帧结构
     * @return 解析成功返回true
     */
    static bool parse(const QByteArray &raw, SpectrometerFrame &frame) {
        if (raw.size() < 6) {  // 最小帧长度
            return false;
        }
        
        // 检查帧头
        if ((quint8)raw[0] != HEADER_1 || (quint8)raw[1] != HEADER_2) {
            return false;
        }
        
        // 解析长度（大端序）
        frame.length = ((quint8)raw[2] << 8) | (quint8)raw[3];
        
        // 检查帧长度
        if (raw.size() < 4 + frame.length) {
            return false;
        }
        
        // 解析命令
        frame.cmd = raw[4];
        
        // 解析数据
        int dataLen = frame.length - 2;  // length包含cmd(1B)和checksum(1B)
        if (dataLen > 0) {
            frame.data = raw.mid(5, dataLen);
        } else {
            frame.data.clear();
        }
        
        // 解析校验和
        frame.checksum = raw[4 + frame.length - 1];
        
        // 验证校验和
        quint8 calcChecksum = calculateChecksum(frame.length, frame.cmd, frame.data);
        if (calcChecksum != frame.checksum) {
            return false;
        }
        
        return true;
    }
    
    /**
     * @brief 计算校验和
     * @param length 长度字段
     * @param cmd 命令字段
     * @param data 数据字段
     * @return 校验和（低8位）
     */
    static quint8 calculateChecksum(quint16 length, quint8 cmd, const QByteArray &data) {
        quint32 sum = 0;
        
        // 累加长度（2字节）
        sum += (length >> 8) & 0xFF;
        sum += length & 0xFF;
        
        // 累加命令
        sum += cmd;
        
        // 累加数据
        for (int i = 0; i < data.size(); ++i) {
            sum += (quint8)data[i];
        }
        
        // 取低8位
        return sum & 0xFF;
    }
    
    /**
     * @brief 创建命令帧
     * @param cmd 命令ID
     * @param data 数据（可选）
     * @return 帧结构
     */
    static SpectrometerFrame createCommand(quint8 cmd, const QByteArray &data = QByteArray()) {
        SpectrometerFrame frame;
        frame.cmd = cmd;
        frame.data = data;
        frame.length = 2 + data.size();  // cmd(1B) + data + checksum(1B)
        frame.checksum = calculateChecksum(frame.length, frame.cmd, frame.data);
        return frame;
    }
};

#endif // SPECTROMETER_PROTOCOL_H

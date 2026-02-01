#ifndef CONSTANTS_H
#define CONSTANTS_H

/**
 * @brief 项目全局常量定义
 * 避免硬编码的魔法数字
 */

namespace Constants {
    // 串口通信
    namespace Serial {
        const int MAX_RETRY = 3;                // 最大重试次数
        const int RETRY_DELAY_MS = 1000;        // 重试延迟(毫秒)
        const int READ_TIMEOUT_MS = 5000;       // 读取超时
        const int WRITE_TIMEOUT_MS = 5000;      // 写入超时
    }
    
    // 光谱图表
    namespace Chart {
        const int MAX_DISPLAY_POINTS = 1000;    // 最大显示点数(性能优化：从2000降到1000)
        const double DEFAULT_WAVELENGTH_MIN = 200.0;   // 默认最小波长(nm)
        const double DEFAULT_WAVELENGTH_MAX = 1100.0;  // 默认最大波长(nm)
        const int DEFAULT_INTENSITY_MIN = 0;           // 默认最小强度
        const int DEFAULT_INTENSITY_MAX = 65535;       // 默认最大强度(16位)
    }
    
    // 峰值检测
    namespace Peak {
        const int DEFAULT_THRESHOLD = 1000;     // 默认强度阈值
        const int DEFAULT_WIDTH = 5;            // 默认峰宽(像素)
        const int MIN_PEAK_DISTANCE = 10;       // 最小峰间距(像素)
    }
    
    // 设备默认参数
    namespace Device {
        // 激光器
        const int LASER_DEFAULT_BAUD = 9600;
        const quint8 LASER_DEFAULT_ID = 0x01;
        
        // 光谱仪
        const int SPECTROMETER_DEFAULT_BAUD = 115200;
        const int SPECTROMETER_DEFAULT_INTEGRATION_TIME = 10000; // 10ms
        
        // 位移台
        const int STAGE_DEFAULT_BAUD = 9600;
        const quint8 STAGE_DEFAULT_ADDRESS = 1;
        
        // 延时线
        const int DELAY_DEFAULT_BAUD = 9600;
        const quint8 DELAY_DEFAULT_ID = 0x01;
        
        // 振镜
        const char* const GALVO_DEFAULT_IP = "172.18.34.227";
    }
}

#endif // CONSTANTS_H

#ifndef ERROR_CODES_H
#define ERROR_CODES_H

#include <QString>

/**
 * @brief 错误码枚举
 * 定义系统中所有可能的错误类型
 */
enum class ErrorCode {
    // 通用错误 (0-99)
    Success = 0,                    // 成功
    UnknownError = 1,               // 未知错误
    
    // 连接错误 (100-199)
    ConnectionFailed = 100,         // 连接失败
    ConnectionTimeout = 101,        // 连接超时
    DeviceNotFound = 102,           // 设备未找到
    PortInUse = 103,                // 端口被占用
    
    // 通信错误 (200-299)
    SendDataFailed = 200,           // 发送数据失败
    ReceiveDataFailed = 201,        // 接收数据失败
    DataCorrupted = 202,            // 数据损坏
    ChecksumError = 203,            // 校验和错误
    CommunicationTimeout = 204,     // 通信超时
    
    // DLL错误 (300-399)
    DLLNotFound = 300,              // DLL未找到
    DLLLoadFailed = 301,            // DLL加载失败
    FunctionNotFound = 302,         // 函数未找到
    DLLCallFailed = 303,            // DLL调用失败
    
    // 设备错误 (400-499)
    DeviceBusy = 400,               // 设备忙
    DeviceNotReady = 401,           // 设备未就绪
    ParameterOutOfRange = 402,      // 参数超出范围
    CommandNotSupported = 403,      // 命令不支持
    
    // 文件错误 (500-599)
    FileNotFound = 500,             // 文件未找到
    FileOpenFailed = 501,           // 文件打开失败
    FileWriteFailed = 502,          // 文件写入失败
    FileReadFailed = 503            // 文件读取失败
};

/**
 * @brief 获取错误信息
 * @param code 错误码
 * @return 错误信息字符串
 */
inline QString getErrorMessage(ErrorCode code) {
    switch (code) {
        case ErrorCode::Success: return "操作成功";
        case ErrorCode::UnknownError: return "未知错误";
        
        case ErrorCode::ConnectionFailed: return "连接失败";
        case ErrorCode::ConnectionTimeout: return "连接超时";
        case ErrorCode::DeviceNotFound: return "设备未找到";
        case ErrorCode::PortInUse: return "端口被占用";
        
        case ErrorCode::SendDataFailed: return "发送数据失败";
        case ErrorCode::ReceiveDataFailed: return "接收数据失败";
        case ErrorCode::DataCorrupted: return "数据损坏";
        case ErrorCode::ChecksumError: return "校验和错误";
        case ErrorCode::CommunicationTimeout: return "通信超时";
        
        case ErrorCode::DLLNotFound: return "DLL文件未找到";
        case ErrorCode::DLLLoadFailed: return "DLL加载失败";
        case ErrorCode::FunctionNotFound: return "函数未找到";
        case ErrorCode::DLLCallFailed: return "DLL调用失败";
        
        case ErrorCode::DeviceBusy: return "设备忙";
        case ErrorCode::DeviceNotReady: return "设备未就绪";
        case ErrorCode::ParameterOutOfRange: return "参数超出范围";
        case ErrorCode::CommandNotSupported: return "命令不支持";
        
        case ErrorCode::FileNotFound: return "文件未找到";
        case ErrorCode::FileOpenFailed: return "文件打开失败";
        case ErrorCode::FileWriteFailed: return "文件写入失败";
        case ErrorCode::FileReadFailed: return "文件读取失败";
        
        default: return "未定义的错误";
    }
}

#endif // ERROR_CODES_H

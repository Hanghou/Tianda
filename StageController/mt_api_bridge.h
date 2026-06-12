#ifndef MT_API_BRIDGE_H
#define MT_API_BRIDGE_H

#include <QLibrary>
#include <QString>

#ifdef Q_OS_WIN
#include <windows.h>
#else
#ifndef WINAPI
#define WINAPI
#endif
#endif

/**
 * @brief MT_API.dll 动态加载桥接类（header-only）
 *
 * 业务用途：使用 QLibrary 在运行时加载厂家 MT_API.dll，避免修改 Integration.pro 和链接 .lib。
 * 函数签名依据 CSharp Demo/MT_API.cs：StdCall + Ansi + Int32 返回值 + UInt16 轴号。
 * 调用约定：MT_API 返回 0 表示成功，非 0 表示失败。
 */
class MtApiBridge
{
public:
    static MtApiBridge &instance()
    {
        static MtApiBridge bridge;
        return bridge;
    }

    using FnInit = int (WINAPI *)();
    using FnOpenUart = int (WINAPI *)(const char *);
    using FnAxisOnly = int (WINAPI *)(unsigned short);
    using FnAxisInt = int (WINAPI *)(unsigned short, int);
    using FnAxisIntPtr = int (WINAPI *)(unsigned short, int *);
    using FnAxisStatus = int (WINAPI *)(unsigned short, unsigned char *, unsigned char *,
                                        unsigned char *, unsigned char *, unsigned char *, unsigned char *);
    using FnGetInt = int (WINAPI *)(int *);

    FnInit MT_Init = nullptr;
    FnInit MT_DeInit = nullptr;
    FnOpenUart MT_Open_UART = nullptr;
    FnInit MT_Open_USB = nullptr;        // USB 即插即用打开（无参，0=成功）
    FnInit MT_Close_UART = nullptr;
    FnInit MT_Close_USB = nullptr;
    FnInit MT_Check = nullptr;
    FnGetInt MT_Get_Axis_Num = nullptr;

    FnAxisOnly MT_Set_Axis_Mode_Position = nullptr;
    FnAxisOnly MT_Set_Axis_Mode_Position_Open = nullptr;
    FnAxisInt MT_Set_Axis_Acc = nullptr;
    FnAxisInt MT_Set_Axis_Dec = nullptr;
    FnAxisInt MT_Set_Axis_Position_V_Max = nullptr;
    FnAxisInt MT_Set_Axis_Position_P_Target_Abs = nullptr;
    FnAxisOnly MT_Set_Axis_Position_Stop = nullptr;
    FnAxisOnly MT_Set_Axis_Halt = nullptr;
    FnInit MT_Set_Axis_Halt_All = nullptr;

    FnAxisIntPtr MT_Get_Axis_Status_Run = nullptr;
    FnAxisIntPtr MT_Get_Axis_Software_P_Now = nullptr;
    FnAxisIntPtr MT_Get_Axis_V_Now = nullptr;
    FnAxisStatus MT_Get_Axis_Status = nullptr;

    FnAxisOnly MT_Set_Axis_Mode_Home_Home_Switch = nullptr;
    FnAxisInt MT_Set_Axis_Home_V = nullptr;
    FnAxisOnly MT_Set_Axis_Home_Stop = nullptr;

    FnAxisInt MT_Set_Axis_Software_Limit_Neg_Value = nullptr;
    FnAxisInt MT_Set_Axis_Software_Limit_Pos_Value = nullptr;
    FnAxisOnly MT_Set_Axis_Software_Limit_Enable = nullptr;
    FnAxisOnly MT_Set_Axis_Software_Limit_Disable = nullptr;

    /**
     * @brief 加载 MT_API.dll 并解析本项目需要的全部函数
     * @param dllPath DLL完整路径，通常为 QCoreApplication::applicationDirPath() + "/MT_API.dll"
     * @return 全部函数解析成功返回 true，否则返回 false 并记录 lastError()
     */
    bool load(const QString &dllPath)
    {
        if (m_loaded) return true;

        resetFunctions();
        m_library.setFileName(dllPath);
        if (!m_library.load()) {
            m_lastError = QString("MT_API.dll 加载失败：%1").arg(m_library.errorString());
            return false;
        }

        bool ok = true;
        ok &= resolve(MT_Init, "MT_Init");
        ok &= resolve(MT_DeInit, "MT_DeInit");
        ok &= resolve(MT_Open_UART, "MT_Open_UART");
        ok &= resolve(MT_Open_USB, "MT_Open_USB");
        ok &= resolve(MT_Close_UART, "MT_Close_UART");
        ok &= resolve(MT_Close_USB, "MT_Close_USB");
        ok &= resolve(MT_Check, "MT_Check");
        ok &= resolve(MT_Get_Axis_Num, "MT_Get_Axis_Num");
        ok &= resolve(MT_Set_Axis_Mode_Position, "MT_Set_Axis_Mode_Position");
        ok &= resolve(MT_Set_Axis_Mode_Position_Open, "MT_Set_Axis_Mode_Position_Open");
        ok &= resolve(MT_Set_Axis_Acc, "MT_Set_Axis_Acc");
        ok &= resolve(MT_Set_Axis_Dec, "MT_Set_Axis_Dec");
        ok &= resolve(MT_Set_Axis_Position_V_Max, "MT_Set_Axis_Position_V_Max");
        ok &= resolve(MT_Set_Axis_Position_P_Target_Abs, "MT_Set_Axis_Position_P_Target_Abs");
        ok &= resolve(MT_Set_Axis_Position_Stop, "MT_Set_Axis_Position_Stop");
        ok &= resolve(MT_Set_Axis_Halt, "MT_Set_Axis_Halt");
        ok &= resolve(MT_Set_Axis_Halt_All, "MT_Set_Axis_Halt_All");
        ok &= resolve(MT_Get_Axis_Status_Run, "MT_Get_Axis_Status_Run");
        ok &= resolve(MT_Get_Axis_Software_P_Now, "MT_Get_Axis_Software_P_Now");
        ok &= resolve(MT_Get_Axis_V_Now, "MT_Get_Axis_V_Now");
        ok &= resolve(MT_Get_Axis_Status, "MT_Get_Axis_Status");
        ok &= resolve(MT_Set_Axis_Mode_Home_Home_Switch, "MT_Set_Axis_Mode_Home_Home_Switch");
        ok &= resolve(MT_Set_Axis_Home_V, "MT_Set_Axis_Home_V");
        ok &= resolve(MT_Set_Axis_Home_Stop, "MT_Set_Axis_Home_Stop");
        ok &= resolve(MT_Set_Axis_Software_Limit_Neg_Value, "MT_Set_Axis_Software_Limit_Neg_Value");
        ok &= resolve(MT_Set_Axis_Software_Limit_Pos_Value, "MT_Set_Axis_Software_Limit_Pos_Value");
        ok &= resolve(MT_Set_Axis_Software_Limit_Enable, "MT_Set_Axis_Software_Limit_Enable");
        ok &= resolve(MT_Set_Axis_Software_Limit_Disable, "MT_Set_Axis_Software_Limit_Disable");

        if (!ok) {
            m_library.unload();
            resetFunctions();
            return false;
        }

        m_loaded = true;
        m_lastError.clear();
        return true;
    }

    bool isLoaded() const { return m_loaded; }
    QString lastError() const { return m_lastError; }

private:
    MtApiBridge() = default;
    ~MtApiBridge() = default;
    MtApiBridge(const MtApiBridge &) = delete;
    MtApiBridge &operator=(const MtApiBridge &) = delete;

    template<typename T>
    bool resolve(T &target, const char *name)
    {
        target = reinterpret_cast<T>(m_library.resolve(name));
        if (!target) {
            m_lastError = QString("MT_API.dll 函数解析失败：%1").arg(name);
            return false;
        }
        return true;
    }

    void resetFunctions()
    {
        MT_Init = nullptr; MT_DeInit = nullptr; MT_Open_UART = nullptr; MT_Open_USB = nullptr;
        MT_Close_UART = nullptr; MT_Close_USB = nullptr; MT_Check = nullptr; MT_Get_Axis_Num = nullptr;
        MT_Set_Axis_Mode_Position = nullptr; MT_Set_Axis_Mode_Position_Open = nullptr;
        MT_Set_Axis_Acc = nullptr; MT_Set_Axis_Dec = nullptr; MT_Set_Axis_Position_V_Max = nullptr;
        MT_Set_Axis_Position_P_Target_Abs = nullptr; MT_Set_Axis_Position_Stop = nullptr;
        MT_Set_Axis_Halt = nullptr; MT_Set_Axis_Halt_All = nullptr; MT_Get_Axis_Status_Run = nullptr;
        MT_Get_Axis_Software_P_Now = nullptr; MT_Get_Axis_V_Now = nullptr; MT_Get_Axis_Status = nullptr;
        MT_Set_Axis_Mode_Home_Home_Switch = nullptr; MT_Set_Axis_Home_V = nullptr; MT_Set_Axis_Home_Stop = nullptr;
        MT_Set_Axis_Software_Limit_Neg_Value = nullptr; MT_Set_Axis_Software_Limit_Pos_Value = nullptr;
        MT_Set_Axis_Software_Limit_Enable = nullptr; MT_Set_Axis_Software_Limit_Disable = nullptr;
        m_loaded = false;
    }

    QLibrary m_library;
    bool m_loaded = false;
    QString m_lastError;
};

#endif // MT_API_BRIDGE_H

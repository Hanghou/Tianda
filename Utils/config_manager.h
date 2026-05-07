#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <QSettings>
#include <QString>
#include <QVariant>

/**
 * @brief 配置管理类
 * 使用QSettings管理配置文件(INI格式)
 */
class ConfigManager
{
public:
    ConfigManager();
    ~ConfigManager();

    /**
     * @brief 保存设备配置
     * @param device 设备名称
     * @param key 配置键
     * @param value 配置值
     */
    void saveDeviceConfig(const QString &device, const QString &key, const QVariant &value);
    
    /**
     * @brief 加载设备配置
     * @param device 设备名称
     * @param key 配置键
     * @param defaultValue 默认值
     * @return 配置值
     */
    QVariant loadDeviceConfig(const QString &device, const QString &key, 
                             const QVariant &defaultValue = QVariant());
    
    /**
     * @brief 保存窗口状态
     * @param window 主窗口指针
     */
    void saveWindowState(class QMainWindow *window);
    
    /**
     * @brief 恢复窗口状态
     * @param window 主窗口指针
     */
    void restoreWindowState(class QMainWindow *window);

private:
    QSettings *m_settings;
};

#endif // CONFIG_MANAGER_H

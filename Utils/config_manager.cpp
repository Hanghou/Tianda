#include "config_manager.h"
#include <QMainWindow>
#include <QDebug>

ConfigManager::ConfigManager()
{
    m_settings = new QSettings("Integration.ini", QSettings::IniFormat);
    qDebug() << "配置文件路径:" << m_settings->fileName();
}

ConfigManager::~ConfigManager()
{
    delete m_settings;
}

void ConfigManager::saveDeviceConfig(const QString &device, const QString &key, const QVariant &value)
{
    m_settings->setValue(device + "/" + key, value);
    m_settings->sync();
}

QVariant ConfigManager::loadDeviceConfig(const QString &device, const QString &key, 
                                        const QVariant &defaultValue)
{
    return m_settings->value(device + "/" + key, defaultValue);
}

void ConfigManager::saveWindowState(QMainWindow *window)
{
    if (window) {
        m_settings->setValue("UI/WindowGeometry", window->saveGeometry());
        m_settings->setValue("UI/WindowState", window->saveState());
        m_settings->sync();
    }
}

void ConfigManager::restoreWindowState(QMainWindow *window)
{
    if (window) {
        window->restoreGeometry(m_settings->value("UI/WindowGeometry").toByteArray());
        window->restoreState(m_settings->value("UI/WindowState").toByteArray());
    }
}

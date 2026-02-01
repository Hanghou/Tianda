#include "preset_manager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

PresetManager::PresetManager(QObject *parent)
    : QObject(parent)
{
}

PresetManager::~PresetManager()
{
}

// ========== 功率预设组管理 ==========

bool PresetManager::addPowerPresetGroup(const PowerPresetGroup &group)
{
    m_powerPresetGroups.append(group);
    emit powerPresetGroupsChanged();
    return true;
}

bool PresetManager::updatePowerPresetGroup(int index, const PowerPresetGroup &group)
{
    if (index < 0 || index >= m_powerPresetGroups.size()) {
        return false;
    }
    
    m_powerPresetGroups[index] = group;
    emit powerPresetGroupsChanged();
    return true;
}

bool PresetManager::deletePowerPresetGroup(int index)
{
    if (index < 0 || index >= m_powerPresetGroups.size()) {
        return false;
    }
    
    m_powerPresetGroups.removeAt(index);
    emit powerPresetGroupsChanged();
    return true;
}

PowerPresetGroup PresetManager::getPowerPresetGroup(int index) const
{
    if (index < 0 || index >= m_powerPresetGroups.size()) {
        return PowerPresetGroup();
    }
    
    return m_powerPresetGroups[index];
}

QList<PowerPresetGroup> PresetManager::getAllPowerPresetGroups() const
{
    return m_powerPresetGroups;
}

int PresetManager::getPowerPresetGroupCount() const
{
    return m_powerPresetGroups.size();
}

// ========== 延迟预设组管理 ==========

bool PresetManager::addDelayPresetGroup(const DelayPresetGroup &group)
{
    m_delayPresetGroups.append(group);
    emit delayPresetGroupsChanged();
    return true;
}

bool PresetManager::updateDelayPresetGroup(int index, const DelayPresetGroup &group)
{
    if (index < 0 || index >= m_delayPresetGroups.size()) {
        return false;
    }
    
    m_delayPresetGroups[index] = group;
    emit delayPresetGroupsChanged();
    return true;
}

bool PresetManager::deleteDelayPresetGroup(int index)
{
    if (index < 0 || index >= m_delayPresetGroups.size()) {
        return false;
    }
    
    m_delayPresetGroups.removeAt(index);
    emit delayPresetGroupsChanged();
    return true;
}

DelayPresetGroup PresetManager::getDelayPresetGroup(int index) const
{
    if (index < 0 || index >= m_delayPresetGroups.size()) {
        return DelayPresetGroup();
    }
    
    return m_delayPresetGroups[index];
}

QList<DelayPresetGroup> PresetManager::getAllDelayPresetGroups() const
{
    return m_delayPresetGroups;
}

int PresetManager::getDelayPresetGroupCount() const
{
    return m_delayPresetGroups.size();
}

// ========== 数据持久化 ==========

bool PresetManager::savePowerPresetGroups(const QString &filePath)
{
    QJsonObject root;
    root["version"] = "1.0";
    
    QJsonArray groupsArray;
    for (const PowerPresetGroup &group : m_powerPresetGroups) {
        QJsonObject groupObj;
        groupObj["name"] = group.name;
        groupObj["timeInterval"] = group.timeInterval;
        groupObj["description"] = group.description;
        groupObj["createTime"] = group.createTime.toString(Qt::ISODate);
        
        QJsonArray presetsArray;
        for (const PowerPreset &preset : group.presets) {
            QJsonObject presetObj;
            presetObj["name"] = preset.name;
            presetObj["galvoAngleStart"] = preset.galvoAngleStart;
            presetObj["galvoAngleEnd"] = preset.galvoAngleEnd;
            presetObj["seedPumpCurrent"] = preset.seedPumpCurrent;
            presetObj["fopoPumpCurrent"] = preset.fopoPumpCurrent;
            presetObj["stokesPumpCurrent"] = preset.stokesPumpCurrent;
            presetsArray.append(presetObj);
        }
        
        groupObj["presets"] = presetsArray;
        groupsArray.append(groupObj);
    }
    
    root["powerPresetGroups"] = groupsArray;
    
    QJsonDocument doc(root);
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "无法打开文件保存:" << filePath;
        return false;
    }
    
    file.write(doc.toJson());
    file.close();
    return true;
}

bool PresetManager::loadPowerPresetGroups(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "无法打开文件加载:" << filePath;
        return false;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        qWarning() << "JSON格式错误";
        return false;
    }
    
    QJsonObject root = doc.object();
    QJsonArray groupsArray = root["powerPresetGroups"].toArray();
    
    m_powerPresetGroups.clear();
    
    for (const QJsonValue &groupValue : groupsArray) {
        QJsonObject groupObj = groupValue.toObject();
        
        PowerPresetGroup group;
        group.name = groupObj["name"].toString();
        group.timeInterval = groupObj["timeInterval"].toInt();
        group.description = groupObj["description"].toString();
        group.createTime = QDateTime::fromString(groupObj["createTime"].toString(), Qt::ISODate);
        
        QJsonArray presetsArray = groupObj["presets"].toArray();
        for (const QJsonValue &presetValue : presetsArray) {
            QJsonObject presetObj = presetValue.toObject();
            
            PowerPreset preset;
            preset.name = presetObj["name"].toString();
            preset.galvoAngleStart = presetObj["galvoAngleStart"].toDouble();
            preset.galvoAngleEnd = presetObj["galvoAngleEnd"].toDouble();
            preset.seedPumpCurrent = presetObj["seedPumpCurrent"].toDouble();
            preset.fopoPumpCurrent = presetObj["fopoPumpCurrent"].toDouble();
            preset.stokesPumpCurrent = presetObj["stokesPumpCurrent"].toDouble();
            
            group.presets.append(preset);
        }
        
        m_powerPresetGroups.append(group);
    }
    
    emit powerPresetGroupsChanged();
    return true;
}

bool PresetManager::saveDelayPresetGroups(const QString &filePath)
{
    QJsonObject root;
    root["version"] = "1.0";
    
    QJsonArray groupsArray;
    for (const DelayPresetGroup &group : m_delayPresetGroups) {
        QJsonObject groupObj;
        groupObj["name"] = group.name;
        groupObj["timeInterval"] = group.timeInterval;
        groupObj["description"] = group.description;
        groupObj["createTime"] = group.createTime.toString(Qt::ISODate);
        
        QJsonArray presetsArray;
        for (const DelayPreset &preset : group.presets) {
            QJsonObject presetObj;
            presetObj["name"] = preset.name;
            presetObj["galvoAngle"] = preset.galvoAngle;
            presetObj["delayTime"] = preset.delayTime;
            presetsArray.append(presetObj);
        }
        
        groupObj["presets"] = presetsArray;
        groupsArray.append(groupObj);
    }
    
    root["delayPresetGroups"] = groupsArray;
    
    QJsonDocument doc(root);
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "无法打开文件保存:" << filePath;
        return false;
    }
    
    file.write(doc.toJson());
    file.close();
    return true;
}

bool PresetManager::loadDelayPresetGroups(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "无法打开文件加载:" << filePath;
        return false;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        qWarning() << "JSON格式错误";
        return false;
    }
    
    QJsonObject root = doc.object();
    QJsonArray groupsArray = root["delayPresetGroups"].toArray();
    
    m_delayPresetGroups.clear();
    
    for (const QJsonValue &groupValue : groupsArray) {
        QJsonObject groupObj = groupValue.toObject();
        
        DelayPresetGroup group;
        group.name = groupObj["name"].toString();
        group.timeInterval = groupObj["timeInterval"].toInt();
        group.description = groupObj["description"].toString();
        group.createTime = QDateTime::fromString(groupObj["createTime"].toString(), Qt::ISODate);
        
        QJsonArray presetsArray = groupObj["presets"].toArray();
        for (const QJsonValue &presetValue : presetsArray) {
            QJsonObject presetObj = presetValue.toObject();
            
            DelayPreset preset;
            preset.name = presetObj["name"].toString();
            preset.galvoAngle = presetObj["galvoAngle"].toDouble();
            preset.delayTime = presetObj["delayTime"].toDouble();
            
            group.presets.append(preset);
        }
        
        m_delayPresetGroups.append(group);
    }
    
    emit delayPresetGroupsChanged();
    return true;
}

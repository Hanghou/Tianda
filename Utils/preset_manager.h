#ifndef PRESET_MANAGER_H
#define PRESET_MANAGER_H

#include <QObject>
#include <QList>
#include <QString>
#include <QDateTime>

/**
 * @brief 光源功率预设结构
 */
struct PowerPreset {
    QString name;                  // 预设名称（可选）
    float galvoAngleStart;         // 振镜角度起始值 (deg)
    float galvoAngleEnd;           // 振镜角度结束值 (deg)
    float seedPumpCurrent;         // 种子源泵电流 (mA)
    float fopoPumpCurrent;         // FOPO泵电流 (A)
    float stokesPumpCurrent;       // Stokes泵电流 (mA)
    
    PowerPreset()
        : galvoAngleStart(0.0f)
        , galvoAngleEnd(0.0f)
        , seedPumpCurrent(0.0f)
        , fopoPumpCurrent(0.0f)
        , stokesPumpCurrent(0.0f)
    {}
};

/**
 * @brief 光源功率预设组
 */
struct PowerPresetGroup {
    QString name;                  // 预设组名称
    QList<PowerPreset> presets;    // 预设列表
    int timeInterval;              // 时间间隔（秒）
    QString description;           // 描述
    QDateTime createTime;          // 创建时间
    
    PowerPresetGroup()
        : timeInterval(1)
        , createTime(QDateTime::currentDateTime())
    {}
};

/**
 * @brief 延迟线预设结构
 */
struct DelayPreset {
    QString name;                  // 预设名称（可选）
    float galvoAngle;              // 振镜角度 (deg) - 用于振镜页
    float stagePosition;           // 旋转台角度 (deg) - 用于位移台页
    float delayTime;               // 延迟线时延 (PS)
    
    DelayPreset()
        : galvoAngle(0.0f)
        , stagePosition(0.0f)
        , delayTime(0.0f)
    {}
};

/**
 * @brief 延迟线预设组
 */
struct DelayPresetGroup {
    QString name;                  // 预设组名称
    QList<DelayPreset> presets;    // 预设列表
    int timeInterval;              // 时间间隔（秒）
    QString description;           // 描述
    QDateTime createTime;          // 创建时间
    
    DelayPresetGroup()
        : timeInterval(1)
        , createTime(QDateTime::currentDateTime())
    {}
};

/**
 * @brief 位移台功率预设结构（包含旋转台角度和位移台位置）
 */
struct StagePowerPreset {
    QString name;                  // 预设名称（可选）
    float stageAngle;              // 旋转台角度 (deg)
    float stagePosition;           // 位移台位置 (mm)
    float seedPumpCurrent;         // 种子源泵电流 (mA)
    float fopoPumpCurrent;         // FOPO泵电流 (A)
    float stokesPumpCurrent;       // Stokes泵电流 (mA)
    
    StagePowerPreset()
        : stageAngle(0.0f)
        , stagePosition(0.0f)
        , seedPumpCurrent(0.0f)
        , fopoPumpCurrent(0.0f)
        , stokesPumpCurrent(0.0f)
    {}
};

/**
 * @brief 位移台功率预设组
 */
struct StagePowerPresetGroup {
    QString name;                  // 预设组名称
    QList<StagePowerPreset> presets;  // 预设列表
    int timeInterval;              // 时间间隔（秒）
    QString description;           // 描述
    QDateTime createTime;          // 创建时间
    
    StagePowerPresetGroup()
        : timeInterval(1)
        , createTime(QDateTime::currentDateTime())
    {}
};

/**
 * @brief 预设管理类
 */
class PresetManager : public QObject
{
    Q_OBJECT
    
public:
    explicit PresetManager(QObject *parent = nullptr);
    ~PresetManager();
    
    // 功率预设组管理
    bool addPowerPresetGroup(const PowerPresetGroup &group);
    bool updatePowerPresetGroup(int index, const PowerPresetGroup &group);
    bool deletePowerPresetGroup(int index);
    PowerPresetGroup getPowerPresetGroup(int index) const;
    QList<PowerPresetGroup> getAllPowerPresetGroups() const;
    int getPowerPresetGroupCount() const;
    
    // 延迟预设组管理
    bool addDelayPresetGroup(const DelayPresetGroup &group);
    bool updateDelayPresetGroup(int index, const DelayPresetGroup &group);
    bool deleteDelayPresetGroup(int index);
    DelayPresetGroup getDelayPresetGroup(int index) const;
    QList<DelayPresetGroup> getAllDelayPresetGroups() const;
    int getDelayPresetGroupCount() const;
    
    // 数据持久化
    bool savePowerPresetGroups(const QString &filePath);
    bool loadPowerPresetGroups(const QString &filePath);
    bool saveDelayPresetGroups(const QString &filePath);
    bool loadDelayPresetGroups(const QString &filePath);
    
signals:
    void powerPresetGroupsChanged();
    void delayPresetGroupsChanged();
    
private:
    QList<PowerPresetGroup> m_powerPresetGroups;
    QList<DelayPresetGroup> m_delayPresetGroups;
};

#endif // PRESET_MANAGER_H

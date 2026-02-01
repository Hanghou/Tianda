#ifndef DATA_MANAGER_H
#define DATA_MANAGER_H

#include <QObject>

/**
 * @brief 数据管理类
 * 管理所有设备的测量数据
 * TODO: 后续实现完整功能
 */
class DataManager : public QObject
{
    Q_OBJECT

public:
    explicit DataManager(QObject *parent = nullptr);
    ~DataManager();

    // TODO: 后续实现
    // void addDataPoint(...);
    // void clearAllData();
};

#endif // DATA_MANAGER_H

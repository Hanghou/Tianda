#ifndef CSV_EXPORTER_H
#define CSV_EXPORTER_H

#include <QObject>

/**
 * @brief CSV导出类
 * TODO: 后续实现完整功能
 */
class CSVExporter : public QObject
{
    Q_OBJECT

public:
    explicit CSVExporter(QObject *parent = nullptr);
    ~CSVExporter();
};

#endif // CSV_EXPORTER_H

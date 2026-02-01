#ifndef IMAGE_SAVER_H
#define IMAGE_SAVER_H

#include <QObject>

/**
 * @brief 图像保存类
 * TODO: 后续实现完整功能
 */
class ImageSaver : public QObject
{
    Q_OBJECT

public:
    explicit ImageSaver(QObject *parent = nullptr);
    ~ImageSaver();
};

#endif // IMAGE_SAVER_H

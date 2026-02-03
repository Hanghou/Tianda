#include "UI/integration.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    // 设置应用程序信息
    QApplication::setOrganizationName("Integration");
    QApplication::setApplicationName("多模块光学测量系统");
    QApplication::setApplicationVersion("1.0.0");
    
    Integration w;
    w.show();
    
    return a.exec();
}

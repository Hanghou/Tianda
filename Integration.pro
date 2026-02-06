QT       += core gui widgets serialport charts network

CONFIG += c++11

# MSVC 编译器配置
msvc {
    # 使用 UTF-8 编码
    QMAKE_CXXFLAGS += /utf-8
    # 禁用 Windows min/max 宏
    DEFINES += NOMINMAX
    # 禁用其他常见警告
    QMAKE_CXXFLAGS += /wd4100  # 未引用的形参
    QMAKE_CXXFLAGS += /wd4244  # 类型转换可能丢失数据
    QMAKE_CXXFLAGS += /wd4267  # size_t 转换
}

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    UI/integration.cpp \
    Communication/serial_port_base.cpp \
    Communication/device_base.cpp \
    LaserDriver/laser_driver.cpp \
    Spectrometer/spectrometer.cpp \
    StageController/stage_controller.cpp \
    DelayLine/delay_line.cpp \
    GalvoMirror/galvo_mirror.cpp \
    Utils/data_manager.cpp \
    Utils/csv_exporter.cpp \
    Utils/image_saver.cpp \
    Utils/config_manager.cpp \
    Utils/preset_manager.cpp

HEADERS += \
    UI/integration.h \
    Communication/serial_port_base.h \
    Communication/device_base.h \
    Communication/error_codes.h \
    LaserDriver/laser_driver.h \
    LaserDriver/laser_protocol.h \
    Spectrometer/spectrometer.h \
    Spectrometer/spectrometer_protocol.h \
    StageController/stage_controller.h \
    StageController/stage_protocol.h \
    DelayLine/delay_line.h \
    DelayLine/delay_protocol.h \
    GalvoMirror/galvo_mirror.h \
    GalvoMirror/galvo_protocol.h \
    Utils/data_manager.h \
    Utils/csv_exporter.h \
    Utils/image_saver.h \
    Utils/config_manager.h \
    Utils/constants.h \
    Utils/preset_manager.h

FORMS += \
    UI/integration.ui

# Default rules for deployment.
qnx: target.path = /tmp/${TARGET}/bin
else: unix:!android: target.path = /opt/${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# 振镜控制卡库文件链接
LIBS += -L$$PWD/GalvoMirror/library/ -lHM_HashuScan -lHM_Comm

# 包含路径
INCLUDEPATH += $$PWD
INCLUDEPATH += $$PWD/GalvoMirror/library
DEPENDPATH += $$PWD

DISTFILES += \
    GalvoMirror/思特控制卡二次开发说明V2.3.txt \
    GalvoMirror/README.md


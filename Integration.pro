QT       += core gui widgets serialport charts network

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    integration.cpp \
    Communication/serial_port_base.cpp \
    Communication/device_base.cpp \
    LaserDriver/laser_driver.cpp \
    Spectrometer/spectrometer.cpp \
    StageController/stage_controller.cpp \
    GalvoMirror/galvo_mirror.cpp \
    GalvoMirror/galvo_dll_wrapper.cpp \
    GalvoMirror/galvo_tcp_controller.cpp \
    DelayLine/delay_line.cpp \
    Utils/data_manager.cpp \
    Utils/csv_exporter.cpp \
    Utils/image_saver.cpp \
    Utils/config_manager.cpp \
    Utils/preset_manager.cpp

HEADERS += \
    GalvoMirror/HM_HashuUDM.h \
    integration.h \
    Communication/serial_port_base.h \
    Communication/device_base.h \
    Communication/error_codes.h \
    LaserDriver/laser_driver.h \
    LaserDriver/laser_protocol.h \
    Spectrometer/spectrometer.h \
    Spectrometer/spectrometer_protocol.h \
    StageController/stage_controller.h \
    StageController/stage_protocol.h \
    GalvoMirror/galvo_mirror.h \
    GalvoMirror/galvo_dll_wrapper.h \
    GalvoMirror/galvo_tcp_controller.h \
    DelayLine/delay_line.h \
    DelayLine/delay_protocol.h \
    Utils/data_manager.h \
    Utils/csv_exporter.h \
    Utils/image_saver.h \
    Utils/config_manager.h \
    Utils/constants.h \
    Utils/preset_manager.h

FORMS += \
    integration.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# 注释掉DLL链接，改为运行时动态加载
# LIBS += -L$$PWD/Spectrometer/ -lDriver_app
# LIBS += -L$$PWD/GalvoMirror/ -lHM_HashuScan -lHM_Comm

INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

DISTFILES += \
    GalvoMirror/HM_Comm.dll \
    GalvoMirror/HM_Comm.lib \
    GalvoMirror/HM_HashuScan.dll \
    GalvoMirror/HM_HashuScan.lib \
    GalvoMirror/system.ini

# Windows平台：编译后自动复制振镜DLL到输出目录
win32 {
    # 复制GalvoMirror文件夹（振镜需要DLL）
    QMAKE_POST_LINK += xcopy /Y /I /E $$shell_quote($$shell_path($$PWD/GalvoMirror)) $$shell_quote($$shell_path($$OUT_PWD/GalvoMirror))
}

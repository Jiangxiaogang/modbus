# QT5.12 Project File
QT       += core gui widgets network serialport
TARGET    = modbus
TEMPLATE  = app
CONFIG   += c++11

# Include paths
INCLUDEPATH += $$PWD/src

# Source files
SOURCES += \
    src/main.cpp \
    src/mainwindow.cpp \
    src/connectionpanel.cpp \
    src/registerview.cpp \
    src/commlogview.cpp \
    src/addregisterdialog.cpp \
    src/aboutdialog.cpp \
    src/modbusdefs.cpp \
    src/modbuscodec.cpp \
    src/tcptransport.cpp \
    src/udptransport.cpp \
    src/modbusclient.cpp \
    src/modbusdevice.cpp \
    src/modbusworker.cpp \
    src/commmonitor.cpp \
    src/realtimedata.cpp \
    src/serialtransport.cpp

# Header files
HEADERS += \
    src/mainwindow.h \
    src/connectionpanel.h \
    src/registerview.h \
    src/commlogview.h \
    src/addregisterdialog.h \
    src/aboutdialog.h \
    src/version.h \
    src/modbusdefs.h \
    src/modbuscodec.h \
    src/tcptransport.h \
    src/udptransport.h \
    src/modbusclient.h \
    src/modbusdevice.h \
    src/modbusworker.h \
    src/commmonitor.h \
    src/commevent.h \
    src/realtimedata.h \
    src/modbustransport.h \
    src/serialtransport.h

# Windows EXE resource
RC_FILE = res/app.rc

# GCC spec
QMAKE_CFLAGS += -std=c11        # C 语言标准
QMAKE_CXXFLAGS += -std=c++11    # C++ 语言标准
QMAKE_CXXFLAGS_WARN_ON = -Wall

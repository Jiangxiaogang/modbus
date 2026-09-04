# QT4.8.7 Project File
QT       += core gui network
TARGET    = modbus
TEMPLATE  = app
CONFIG   += c++11

# Include paths
INCLUDEPATH += $$PWD/src $$PWD/src/modbus

# Source files
SOURCES += \
    src/main.cpp \
    src/mainwindow.cpp \
    src/connectionpanel.cpp \
    src/registerview.cpp \
    src/addregisterdialog.cpp \
    src/modbus/modbusdefs.cpp \
    src/modbus/modbuscodec.cpp \
    src/modbus/serialtransport.cpp \
    src/modbus/networktransport.cpp \
    src/modbus/modbusclient.cpp \
    src/modbus/modbusworker.cpp

# Header files
HEADERS += \
    src/mainwindow.h \
    src/connectionpanel.h \
    src/registerview.h \
    src/addregisterdialog.h \
    src/modbus/modbusdefs.h \
    src/modbus/modbuscodec.h \
    src/modbus/transport.h \
    src/modbus/serialtransport.h \
    src/modbus/networktransport.h \
    src/modbus/modbusclient.h \
    src/modbus/modbusworker.h

# Windows EXE resource
RC_FILE = res/app.rc

# GCC spec
QMAKE_CFLAGS += -std=c11        # C 语言标准
QMAKE_CXXFLAGS += -std=c++11    # C++ 语言标准
QMAKE_CXXFLAGS_WARN_ON = -Wall

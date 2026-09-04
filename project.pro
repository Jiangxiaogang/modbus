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
    src/modbusdefs.cpp \
    src/modbuscodec.cpp \
    src/tcptransport.cpp \
    src/udptransport.cpp \
    src/modbusclient.cpp \
    src/modbusworker.cpp \
    src/serialtransport.cpp \
    src/seriallist.cpp

# Header files
HEADERS += \
    src/mainwindow.h \
    src/connectionpanel.h \
    src/registerview.h \
    src/addregisterdialog.h \
    src/modbusdefs.h \
    src/modbuscodec.h \
    src/transport.h \
    src/tcptransport.h \
    src/udptransport.h \
    src/modbusclient.h \
    src/modbusworker.h \
    src/modbustransport.h \
    src/serialtransport.h \
    src/seriallist.h

# Windows EXE resource
RC_FILE = res/app.rc

# GCC spec
QMAKE_CFLAGS += -std=c11        # C 语言标准
QMAKE_CXXFLAGS += -std=c++11    # C++ 语言标准
QMAKE_CXXFLAGS_WARN_ON = -Wall

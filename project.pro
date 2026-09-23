QT       += core gui widgets network serialport
TARGET    = modbus
TEMPLATE  = app
CONFIG   += c++11

INCLUDEPATH += $$PWD/src

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
    src/modbusdevice.cpp \
    src/modbusworker.cpp \
    src/registerdata.cpp \
    src/serialtransport.cpp

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
    src/modbusdevice.h \
    src/modbusworker.h \
    src/registerdata.h \
    src/modbustransport.h \
    src/serialtransport.h

RC_FILE = res/app.rc

QMAKE_CFLAGS += -std=c11
QMAKE_CXXFLAGS += -std=c++11
QMAKE_CXXFLAGS_WARN_ON = -Wall

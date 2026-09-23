@echo off
set QTDIR=D:\IDE\QT\Qt5.12.12\5.12.12\mingw73_32
set QTMINGW=D:\IDE\QT\Qt5.12.12\Tools\mingw730_32
set QMAKESPEC=win32-g++
set QT_PLUGIN_PATH=%QTDIR%\plugins
set PATH=%QTDIR%\bin;%QTDIR%\plugins\platforms;%QTMINGW%\bin;%PATH%;
qmake -r
mingw32-make all

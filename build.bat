@echo off
set QTDIR=D:\IDE\QT\Qt4.8.7
set QTMINGW=D:\IDE\QT\Qt4.8.7\mingw32
set QMAKESPEC=win32-g++
set PATH=%QTDIR%\bin;%QTMINGW%\bin;%PATH%;
qmake -r
mingw32-make

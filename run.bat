@echo off
setlocal
set QTDIR=D:\IDE\QT\Qt5.12.12\5.12.12\mingw73_32
set QTMINGW=D:\IDE\QT\Qt5.12.12\Tools\mingw730_32
set QT_PLUGIN_PATH=%QTDIR%\plugins
set PATH=%QTDIR%\bin;%QTDIR%\plugins\platforms;%QTMINGW%\bin;%PATH%

set APP=%~dp0release\modbus.exe
if not exist "%APP%" set APP=%~dp0debug\modbus.exe
if not exist "%APP%" (
    echo modbus.exe not found. Please run build.bat first.
    pause
    exit /b 1
)

start "" "%APP%"
endlocal

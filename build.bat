@echo off
setlocal
set QTDIR=D:\IDE\QT\Qt5.12.12\5.12.12\msvc2017
set VCVARS=C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\VC\Auxiliary\Build\vcvarsall.bat
set QMAKESPEC=win32-msvc
set QT_PLUGIN_PATH=%QTDIR%\plugins

if not exist "%VCVARS%" goto :novc
call "%VCVARS%" x86
if errorlevel 1 goto :fail

set PATH=%QTDIR%\bin;%QT_PLUGIN_PATH%\platforms;%PATH%

qmake -r
if errorlevel 1 goto :fail
nmake all
if errorlevel 1 goto :fail

echo.
echo Deploying to dist...
if exist dist rmdir /s /q dist
mkdir dist
copy /y release\modbus.exe dist\modbus.exe >nul
windeployqt --release --compiler-runtime --no-translations dist\modbus.exe
if errorlevel 1 goto :fail

echo.
echo Deploy finished: %CD%\dist
goto :end

:fail
echo.
echo Build or deploy failed.
exit /b 1

:novc
echo.
echo vcvarsall.bat not found: %VCVARS%
exit /b 1

:end
endlocal

@echo off
REM build.bat — Windows cmd 下的构建入口（内部调用 bash build.sh）
REM 用法：build.bat [normal|bringup]
cd /d "%~dp0"
bash build.sh %1
if errorlevel 1 exit /b 1
echo.
echo 产物位于 build\Morse_CH32V003.hex / .bin

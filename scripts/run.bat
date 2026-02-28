@echo off
setlocal

if "%HWB_PRESET%"=="" set "HWB_PRESET=windows-msvc-release"

set HWB_PRESET | findstr /R /C:"^HWB_PRESET=[A-Za-z0-9_-][A-Za-z0-9_-]*$" >nul || (
    echo Error: Invalid HWB_PRESET value. Use only letters, digits, '-' and '_'.
    exit /b 1
)

set "ROOT=%~dp0.."
set "BUILD=%ROOT%\build\%HWB_PRESET%"
set "OUT=%1"
if "%OUT%"=="" set "OUT=%ROOT%\hwbench-results.json"

cmake --preset "%HWB_PRESET%" -S "%ROOT%" || exit /b 1
cmake --build "%BUILD%" --config Release || exit /b 1

set "EXE=%BUILD%\Release\hwbench-c.exe"
if not exist "%EXE%" set "EXE=%BUILD%\hwbench-c.exe"
"%EXE%" --suite quick --samples 5 --warmup-ms 100 --min-sample-ms 100 --out "%OUT%" || exit /b 1

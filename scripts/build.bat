@echo off
setlocal

if "%HWB_PRESET%"=="" set "HWB_PRESET=windows-msvc-release"

for /f "delims=ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_" %%A in ("%HWB_PRESET%") do (
    echo Error: Invalid HWB_PRESET value "%HWB_PRESET%".
    exit /b 1
)

set "ROOT=%~dp0.."
set "BUILD=%ROOT%\build\%HWB_PRESET%"

cmake --preset "%HWB_PRESET%" -S "%ROOT%" || exit /b 1
cmake --build "%BUILD%" --config Release || exit /b 1

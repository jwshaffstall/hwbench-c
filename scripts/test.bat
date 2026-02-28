@echo off
setlocal

if "%HWB_PRESET%"=="" set "HWB_PRESET=windows-msvc-release"
set "ROOT=%~dp0.."
set "BUILD=%ROOT%\build\%HWB_PRESET%"

cmake --preset %HWB_PRESET% -S "%ROOT%" || exit /b 1
cmake --build "%BUILD%" --config Release || exit /b 1
ctest --test-dir "%BUILD%" --output-on-failure -C Release || exit /b 1

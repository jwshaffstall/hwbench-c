@echo off
setlocal

if "%HWB_PRESET%"=="" set "HWB_PRESET=windows-msvc-release"

set HWB_PRESET | findstr /R /C:"^HWB_PRESET=[A-Za-z0-9_-][A-Za-z0-9_-]*$" >nul || (
    echo Error: Invalid HWB_PRESET value. Use only letters, digits, '-' and '_'.
    exit /b 1
)

set "ROOT=%~dp0.."
set "BUILD=%ROOT%\build\%HWB_PRESET%"

set "DURATION=%1"
if "%DURATION%"=="" set "DURATION=30"
if "%DURATION%"=="10" goto duration_ok
if "%DURATION%"=="30" goto duration_ok
if "%DURATION%"=="60" goto duration_ok
echo Usage: %~n0 [10^|30^|60] [out_path]
exit /b 1
:duration_ok

set "OUT=%2"
if "%OUT%"=="" set "OUT=%ROOT%\hwbench-stress.json"

cmake --preset "%HWB_PRESET%" -S "%ROOT%" || exit /b 1
cmake --build "%BUILD%" --config Release || exit /b 1

set "EXE=%BUILD%\Release\hwbench-c.exe"
if not exist "%EXE%" set "EXE=%BUILD%\hwbench-c.exe"

set "CMD=%EXE% --stress %DURATION% --out %OUT%"
if defined HWB_STRESS_THREADS set "CMD=%CMD% --threads %HWB_STRESS_THREADS%"

echo Running stress mode for %DURATION%s via %EXE%
%CMD% || exit /b 1

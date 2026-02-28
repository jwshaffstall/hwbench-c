@echo off
setlocal

if "%HWB_PRESET%"=="" (
  if /I "%OS%"=="Windows_NT" (
    set "HWB_PRESET=windows-msvc-release"
  ) else (
    echo Unable to infer preset. Set HWB_PRESET.
    exit /b 1
  )
)

set HWB_PRESET | findstr /R /C:"^HWB_PRESET=[A-Za-z0-9_-][A-Za-z0-9_-]*$" >nul || (
    echo Error: Invalid HWB_PRESET value. Use only letters, digits, '-' and '_'.
    exit /b 1
)

where cmake >nul 2>nul || (echo cmake not found & exit /b 1)
where ninja >nul 2>nul || (echo ninja not found & exit /b 1)

cmake --preset "%HWB_PRESET%" -S "%~dp0.."

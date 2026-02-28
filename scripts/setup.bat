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

for /f "delims=ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_" %%A in ("%HWB_PRESET%") do (
    echo Error: Invalid HWB_PRESET value "%HWB_PRESET%".
    exit /b 1
)

where cmake >nul 2>nul || (echo cmake not found & exit /b 1)
where ninja >nul 2>nul || (echo ninja not found & exit /b 1)

cmake --preset "%HWB_PRESET%" -S "%~dp0.."

@echo off
setlocal
cd /d "%~dp0\.."
echo Linting C files (headers are checked when included)...
if "%~1"=="" goto no_build_dir

:has_build_dir
for /R src %%f in (*.c) do clang-tidy -p "%~1" "%%f"
for /R apps %%f in (*.c) do clang-tidy -p "%~1" "%%f"
for /R tests %%f in (*.c) do clang-tidy -p "%~1" "%%f"
goto end

:no_build_dir
for /R src %%f in (*.c) do clang-tidy "%%f" -- -Iinclude
for /R apps %%f in (*.c) do clang-tidy "%%f" -- -Iinclude
for /R tests %%f in (*.c) do clang-tidy "%%f" -- -Iinclude

:end
echo Done.

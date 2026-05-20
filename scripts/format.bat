@echo off
setlocal
cd /d "%~dp0\.."
echo Formatting C/C++ files...
for /R src %%f in (*.c *.h) do clang-format -i "%%f"
for /R include %%f in (*.c *.h) do clang-format -i "%%f"
for /R apps %%f in (*.c *.h) do clang-format -i "%%f"
for /R tests %%f in (*.c *.h) do clang-format -i "%%f"
echo Done.

$ErrorActionPreference = "Stop"
$rootDir = Split-Path -Parent $MyInvocation.MyCommand.Path | Split-Path -Parent
Set-Location $rootDir
Write-Host "Formatting C/C++ files..."
Get-ChildItem -Path src, include, apps, tests -Include *.c, *.h -Recurse | ForEach-Object {
    clang-format -i $_.FullName
}
Write-Host "Done."

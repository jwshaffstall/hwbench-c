param(
    [string]$BuildDir = ""
)
$ErrorActionPreference = "Stop"
$rootDir = Split-Path -Parent $MyInvocation.MyCommand.Path | Split-Path -Parent
Set-Location $rootDir
Write-Host "Linting C files (headers are checked when included)..."
$files = Get-ChildItem -Path src, apps, tests -Include *.c -Recurse

if ($BuildDir) {
    $files | ForEach-Object { clang-tidy -p $BuildDir $_.FullName }
} else {
    $files | ForEach-Object { clang-tidy $_.FullName -- -Iinclude }
}
Write-Host "Done."

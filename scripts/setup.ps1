$ErrorActionPreference = 'Stop'

if (-not $env:HWB_PRESET) {
  if ($IsWindows -or $env:OS -eq 'Windows_NT') {
    $env:HWB_PRESET = 'windows-msvc-release'
  } elseif ($IsMacOS) {
    $env:HWB_PRESET = 'macos-clang-release'
  } elseif ($IsLinux) {
    $env:HWB_PRESET = 'linux-gcc-release'
  } else {
    throw 'Unable to infer preset. Set HWB_PRESET.'
  }
}

Get-Command cmake | Out-Null
Get-Command ninja | Out-Null

$root = Join-Path $PSScriptRoot '..'
cmake --preset $env:HWB_PRESET -S $root

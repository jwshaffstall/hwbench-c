$ErrorActionPreference = 'Stop'

if (-not $env:HWB_PRESET) {
  if ($IsWindows -or $env:OS -eq 'Windows_NT') { $env:HWB_PRESET = 'windows-msvc-release' }
  elseif ($IsMacOS) { $env:HWB_PRESET = 'macos-clang-release' }
  else { $env:HWB_PRESET = 'linux-gcc-release' }
}

$root = Join-Path $PSScriptRoot '..'
$build = Join-Path $root ("build/{0}" -f $env:HWB_PRESET)
cmake --preset $env:HWB_PRESET -S $root
cmake --build $build --config Release

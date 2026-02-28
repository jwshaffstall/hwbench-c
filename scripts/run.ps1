param(
  [string]$OutPath = ""
)
$ErrorActionPreference = 'Stop'

if (-not $env:HWB_PRESET) {
  if ($IsWindows -or $env:OS -eq 'Windows_NT') { $env:HWB_PRESET = 'windows-msvc-release' }
  elseif ($IsMacOS) { $env:HWB_PRESET = 'macos-clang-release' }
  else { $env:HWB_PRESET = 'linux-gcc-release' }
}

$root = Join-Path $PSScriptRoot '..'
$build = Join-Path $root ("build/{0}" -f $env:HWB_PRESET)
if (-not $OutPath) { $OutPath = Join-Path $root 'hwbench-results.json' }

cmake --preset $env:HWB_PRESET -S $root
cmake --build $build --config Release

$exe = Join-Path $build 'hwbench-c'
if (Test-Path (Join-Path $build 'Release/hwbench-c.exe')) { $exe = Join-Path $build 'Release/hwbench-c.exe' }
elseif (Test-Path (Join-Path $build 'hwbench-c.exe')) { $exe = Join-Path $build 'hwbench-c.exe' }

& $exe --suite quick --samples 5 --warmup-ms 100 --min-sample-ms 100 --out $OutPath

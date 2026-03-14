param(
  [int]$Duration = 30,
  [string]$OutPath = ""
)
$ErrorActionPreference = 'Stop'

if ($Duration -notin 10,30,60) {
  Write-Host "Usage: ./stress.ps1 [-Duration 10|30|60] [-OutPath path]"
  exit 1
}

if (-not $env:HWB_PRESET) {
  if ($IsWindows -or $env:OS -eq 'Windows_NT') { $env:HWB_PRESET = 'windows-msvc-release' }
  elseif ($IsMacOS) { $env:HWB_PRESET = 'macos-clang-release' }
  else { $env:HWB_PRESET = 'linux-gcc-release' }
}

$root = Join-Path $PSScriptRoot '..'
$build = Join-Path $root ("build/{0}" -f $env:HWB_PRESET)
if (-not $OutPath) { $OutPath = Join-Path $root 'hwbench-stress.json' }

cmake --preset $env:HWB_PRESET -S $root
cmake --build $build --config Release

$exe = Join-Path $build 'hwbench-c'
if (Test-Path (Join-Path $build 'Release/hwbench-c.exe')) { $exe = Join-Path $build 'Release/hwbench-c.exe' }
elseif (Test-Path (Join-Path $build 'hwbench-c.exe')) { $exe = Join-Path $build 'hwbench-c.exe' }

$cmd = @($exe, '--stress', $Duration, '--out', $OutPath)
if ($env:HWB_STRESS_THREADS) {
  $cmd += @('--threads', $env:HWB_STRESS_THREADS)
}

Write-Host "Running stress mode for $Duration s via $exe"
& $cmd

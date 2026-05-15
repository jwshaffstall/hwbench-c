param(
  [Parameter(Mandatory=$true)][ValidateSet('Debug','Release')][string]$Config
)
$ErrorActionPreference = 'Stop'

Get-Command cmake | Out-Null
Get-Command ninja | Out-Null

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$isWin = $IsWindows -or $env:OS -eq 'Windows_NT'

if ($env:HWB_PRESET) {
  $preset = $env:HWB_PRESET
  $build = Join-Path $root ("build/{0}" -f $preset)
  $extraConfigure = @()
} elseif ($isWin) {
  $preset = if ($Config -eq 'Debug') { 'windows-msvc-debug' } else { 'windows-msvc-release' }
  $build = Join-Path $root ("build/{0}" -f $preset)
  $extraConfigure = @()
} else {
  if ($IsMacOS) { $basePreset = 'macos-clang-release' }
  elseif ($IsLinux) { $basePreset = 'linux-gcc-release' }
  else { throw 'Unable to infer preset. Set HWB_PRESET.' }

  $preset = $basePreset
  $suffix = if ($Config -eq 'Debug') { '-debug' } else { '-optimized' }
  $build = Join-Path $root ("build/{0}{1}" -f $basePreset, $suffix)
  $extraConfigure = @('-B', $build, "-DCMAKE_BUILD_TYPE=$Config")
}

Write-Host "==> Setup ($Config) — preset=$preset build=$build"
cmake --preset $preset -S $root @extraConfigure

Write-Host "==> Build ($Config)"
cmake --build $build --config $Config

Write-Host "==> Test ($Config)"
ctest --test-dir $build --output-on-failure -C $Config

$exe = Join-Path $build 'hwbench-c'
$multi = Join-Path $build ("{0}/hwbench-c.exe" -f $Config)
$flat  = Join-Path $build 'hwbench-c.exe'
if (Test-Path $multi)        { $exe = $multi }
elseif (Test-Path $flat)     { $exe = $flat }
elseif (Test-Path "$exe.exe"){ $exe = "$exe.exe" }

$tmpOut = Join-Path $build ("hwbench-results.{0}.tmp.json" -f $Config.ToLower())
Write-Host "==> Run ($Config) — $exe"
try {
  & $exe --suite quick --samples 5 --warmup-ms 100 --min-sample-ms 100 --out $tmpOut
} finally {
  Remove-Item -Force -ErrorAction SilentlyContinue $tmpOut
}

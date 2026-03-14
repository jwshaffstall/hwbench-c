# hwbench-c

`hwbench-c` is a portable C benchmark suite scaffold with a working CLI, core harness, JSON output, and initial benchmark coverage.

## Implemented now (bootstrap)

- CMake 3.25+ project with cross-platform-oriented options and presets.
- Core benchmark harness:
  - benchmark registry and dispatch
  - monotonic high-resolution timer
  - summary statistics (min/max/median/mean/stdev/percentiles/CV)
  - JSON result emission
- Initial benchmarks:
  - `cpu.scalar.int_add`
  - `cpu.scalar.fp_fma`
  - `memory.stream.copy`
  - `memory.stream.scale`
  - `memory.stream.add`
  - `memory.stream.triad`
  - `memory.latency.pointer_chase`
  - `storage.seq_read`
  - `storage.seq_write`
  - `storage.rand4k_read`
  - `storage.rand4k_write`
  - `gpu.compute.fp32_vec_add`
  - `gpu.compute.fp32_fma`
  - `gpu.compute.i32_mad`
- Unit/smoke tests via CTest.

## Pragmatic deviations from spec

The specification is broad and targets many optional third-party integrations and GPU backends. This initial implementation intentionally focuses on a robust Phase 0 baseline:

- Render and compute backends are disabled by default (`HWB_BUILD_RENDER=OFF`, `HWB_BUILD_COMPUTE=OFF`).
- Optional third-party adapters (SDL3, bgfx, SQLite, xxHash, Zstd, etc.) are represented as CMake options but not yet integrated.
- The benchmark list currently includes a small starter set to keep the code auditable and easy to validate.

## Build and run

```bash
cmake --preset linux-gcc-release
cmake --build --preset linux-gcc-release
ctest --preset linux-gcc-release
./build/linux-gcc-release/hwbench-c --suite quick --out results.json
./build/linux-gcc-release/hwbench-c --list
./build/linux-gcc-release/hwbench-c --stress 30
```


## Hardware detection and reporting

`hwbench-c` now detects and reports local hardware metadata for benchmark context:

- CPU model
- logical and physical core counts
- total system memory
- total root-drive capacity and best-effort storage model
- best-effort GPU identifier

This metadata is shown in CLI output before benchmark rows and is also included under `machine` in JSON result files.

## Cross-platform scripts

The `scripts/` folder includes setup/build/test/run helpers for Bash (`.sh`), Windows Command Prompt (`.bat`), and PowerShell (`.ps1`).

- Bash
  - `./scripts/setup.sh`
  - `./scripts/build.sh`
  - `./scripts/test.sh`
  - `./scripts/run.sh [output.json]`
  - `./scripts/stress.sh [10|30|60] [output.json]`
- Cmd.exe
  - `scripts\setup.bat`
  - `scripts\build.bat`
  - `scripts\test.bat`
  - `scripts\run.bat [output.json]`
  - `scripts\stress.bat [10|30|60] [output.json]`
- PowerShell
  - `./scripts/setup.ps1`
  - `./scripts/build.ps1`
  - `./scripts/test.ps1`
  - `./scripts/run.ps1 [-OutPath output.json]`
  - `./scripts/stress.ps1 [-Duration 10|30|60] [-OutPath output.json]`

By default each script auto-selects a preset based on the host OS. Override it with the `HWB_PRESET` environment variable.

## Continuous Integration

GitHub Actions workflows are provided per platform:

- `.github/workflows/linux.yml`
- `.github/workflows/macos.yml`
- `.github/workflows/windows.yml`

Each workflow configures, builds, and runs tests for its target OS.

## Example output

```bash
./build/linux-gcc-release/hwbench-c --bench cpu.scalar.int_add --samples 5
```

Writes JSON results to `hwbench-results.json` by default or `--out <path>`.

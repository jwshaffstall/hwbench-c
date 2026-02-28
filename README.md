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
  - `memory.stream.triad`
- Unit/smoke tests via CTest.

## Pragmatic deviations from spec

The specification is broad and targets many optional third-party integrations and GPU backends. This initial implementation intentionally focuses on a robust Phase 0 baseline:

- Render and compute backends are disabled by default (`HWB_BUILD_RENDER=OFF`, `HWB_BUILD_COMPUTE=OFF`).
- Optional third-party adapters (SDL3, bgfx, SQLite, xxHash, Zstd, etc.) are represented as CMake options but not yet integrated.
- The benchmark list currently includes only two must-have starter benchmarks to keep the code auditable and easy to validate.

## Build and run

```bash
cmake --preset linux-gcc-release
cmake --build --preset linux-gcc-release
ctest --preset linux-gcc-release
./build/linux-gcc-release/hwbench-c --suite quick --out results.json
./build/linux-gcc-release/hwbench-c --list
```

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

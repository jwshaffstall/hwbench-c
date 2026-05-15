# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build, test, run

Configure / build / test / run all flow through CMake presets. Pick the preset matching the host OS:

| Platform | Preset | Build dir |
|----------|--------|-----------|
| Linux    | `linux-gcc-release` (or `linux-clang-release`) | `build/<preset>/` |
| macOS    | `macos-clang-release` | `build/macos-clang-release/` |
| Windows  | `windows-msvc-release` (also `-debug`, `windows-clangcl-release`) | `build/<preset>/` (binary under `Release/` for multi-config) |

```bash
cmake --preset linux-gcc-release
cmake --build --preset linux-gcc-release
ctest --preset linux-gcc-release
./build/linux-gcc-release/hwbench-c --suite quick --out results.json
```

Windows (multi-config) variant:
```cmd
cmake --build build\windows-msvc-release --config Release
ctest --test-dir build\windows-msvc-release -C Release --output-on-failure
```

Single test target: `ctest --preset linux-gcc-release -R <test-name>` (e.g. `hwbench-smoke-list`, `hwbench-smoke-quick`, or any test registered from `tests/unit/`).

The `scripts/` directory has matching `.sh` / `.bat` / `.ps1` helpers that auto-pick the preset from the host OS; override with `HWB_PRESET=<preset-name>`.

Useful CLI flags: `--list`, `--bench <id>`, `--samples N`, `--suite quick`, `--stress <seconds>`, `--out <path>`.

## Architecture

Two static libraries plus a CLI:

- **`hwbench_core`** (`src/core/`): cross-platform primitives — high-resolution timer (`timer.c`), summary stats (`stats.c`), JSON emitter (`json.c`), hardware detection (`system.c`), and the benchmark registry/dispatch (`bench.c`). Public surface lives in `include/hwbench/*.h`.
- **`hwbench_benches`** (`src/benches/`): individual benchmark implementations (CPU scalar, memory stream, memory latency, storage, GPU compute). Each benchmark is a standalone `.c` file that registers itself via `register.c`, which is the single point that wires benchmarks into the core registry.
- **`hwbench-c`** CLI (`apps/hwbench-cli/main.c`): owns argument parsing, allocation of result buffers, invocation of the registry, hardware detection, and JSON emission. The core library deliberately avoids dynamic allocation — the CLI owns it.

Adding a benchmark = new `src/benches/<id>.c` + an entry in `src/benches/register.c`. Benchmarks return `0` on success, negative on error, and `-2` to signal "not supported on this platform" (the CLI silently skips `-2` in the quick suite; `--bench` prints a message).

Hardware metadata (CPU model, logical/physical cores, memory, storage capacity + best-effort model, GPU id) is gathered by `hwb_detect_hardware`, displayed in the human-readable output, and embedded under `machine` in the JSON.

## Conventions that matter

- **C11, no GNU extensions** (`CMAKE_C_EXTENSIONS OFF`).
- Public symbols are `hwb_`-prefixed; every non-static `hwb_*` in `src/core/` needs a declaration in `include/hwbench/*.h`.
- Error contract: `int` return, `0` success, negative error, `-2` reserved for "unsupported benchmark". Preserve this — the CLI branches on it.
- `hwb_detect_hardware` zero-initialises its output and guarantees `logical_cores >= 1`, `physical_cores >= logical_cores`, and non-empty string fields (`"unknown"` fallback).
- `_POSIX_C_SOURCE=200809L` is set in `CMakeLists.txt` for the core library on non-Windows/non-macOS only — do not add it in source files.
- `libm` is linked from CMake for Linux/non-Apple Unix; do not add `-lm` manually.
- Linux tests that use `fmemopen` must `#define _GNU_SOURCE` before system headers (see `tests/unit/test_system_linux_parse.c`).
- Render/compute/optional third-party backends (`HWB_BUILD_RENDER`, `HWB_BUILD_COMPUTE`, `HWB_USE_SDL3/BGFX/SQLITE/...`) are OFF by default and not yet integrated.

## CI

Three workflows (`.github/workflows/{linux,macos,windows}.yml`) run configure → build → `ctest` for each platform. All three must pass before merging a PR to `master`.

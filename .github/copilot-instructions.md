# hwbench-c – Copilot Coding Agent Instructions

## Project summary

`hwbench-c` is a portable, cross-platform C11 benchmark suite scaffold. It exposes a small
public C library (`hwbench_core`, `hwbench_benches`) and a CLI application (`hwbench-c`).
Benchmarks cover CPU scalar arithmetic and memory-stream operations. Results are emitted as
JSON and printed in tab-separated form to stdout. Hardware metadata (CPU model, core counts,
memory, storage, GPU) is detected at runtime and embedded in results.

## Build requirements

| Tool | Minimum version |
|------|----------------|
| CMake | 3.25 |
| Ninja | any recent |
| C compiler | GCC / Clang / MSVC / clang-cl |

`cmake` and `ninja` must be on `PATH`. No other third-party dependencies are needed for the
default build (all optional adapters are disabled by default).

## Build, test, and run

### Linux (default preset: `linux-gcc-release`)

```bash
# Configure
cmake --preset linux-gcc-release

# Build
cmake --build --preset linux-gcc-release

# Test (runs unit tests + smoke tests)
ctest --preset linux-gcc-release

# Run CLI
./build/linux-gcc-release/hwbench-c --suite quick --out results.json
./build/linux-gcc-release/hwbench-c --list
./build/linux-gcc-release/hwbench-c --bench cpu.scalar.int_add --samples 5
```

### macOS (preset: `macos-clang-release`)

```bash
cmake --preset macos-clang-release
cmake --build build/macos-clang-release
ctest --test-dir build/macos-clang-release --output-on-failure
./build/macos-clang-release/hwbench-c --list
```

### Windows (preset: `windows-msvc-release`)

```cmd
cmake --preset windows-msvc-release
cmake --build build\windows-msvc-release --config Release
ctest --test-dir build\windows-msvc-release -C Release --output-on-failure
build\windows-msvc-release\Release\hwbench-c.exe --list
```

### Helper scripts (auto-select preset from OS)

```bash
./scripts/setup.sh   # verify cmake+ninja present, configure
./scripts/build.sh   # configure + build
./scripts/test.sh    # configure + build + ctest
./scripts/run.sh [output.json]  # configure + build + run quick suite
```

Override the preset with `HWB_PRESET=<preset-name>` before running any script.

## Available CMake presets

| Preset | Platform | Compiler |
|--------|----------|----------|
| `linux-gcc-release` | Linux | GCC |
| `linux-clang-release` | Linux | Clang |
| `macos-clang-release` | macOS | Apple Clang |
| `windows-msvc-release` | Windows | MSVC |
| `windows-msvc-debug` | Windows | MSVC (Debug) |
| `windows-clangcl-release` | Windows | clang-cl |

Build output lands in `build/<preset-name>/`.

## CMake options

| Option | Default | Description |
|--------|---------|-------------|
| `HWB_BUILD_SHARED` | OFF | Build shared libraries instead of static |
| `HWB_BUILD_TESTS` | ON | Build unit/smoke test executable |
| `HWB_BUILD_RENDER` | OFF | Enable render benchmarks (not yet integrated) |
| `HWB_BUILD_COMPUTE` | OFF | Enable compute benchmarks (not yet integrated) |
| `HWB_USE_SDL3/BGFX/SQLITE/…` | OFF | Optional third-party adapters (not yet integrated) |

## Project layout

```
.github/
  workflows/          # CI: linux.yml, macos.yml, windows.yml
CMakeLists.txt        # single top-level build file
CMakePresets.json     # all platform presets
include/hwbench/      # public headers (system.h, bench.h, stats.h, json.h, timer.h, benches.h)
src/
  core/               # bench.c, json.c, stats.c, system.c, timer.c  → hwbench_core library
  benches/            # cpu_scalar_int_add.c, cpu_scalar_fp_fma.c,
                      # memory_stream_copy.c, memory_stream_triad.c,
                      # register.c  → hwbench_benches library
apps/hwbench-cli/     # main.c  → hwbench-c executable
tests/unit/           # test_main.c, test_stats.c, test_registry.c,
                      # test_system.c, test_system_linux_parse.c  → hwbench-tests executable
scripts/              # setup/build/test/run helpers for .sh, .bat, .ps1
```

## Coding conventions

- **Language standard**: C11, `CMAKE_C_EXTENSIONS OFF` (no GNU extensions in production code).
- **Naming**: all public API symbols use the `hwb_` prefix. Types: `hwb_foo_t` or `hwb_foo`
  structs. Functions: `hwb_verb_noun()`.
- **Headers**: every non-static `hwb_*` function in `src/core/` must have a declaration in the
  corresponding `include/hwbench/*.h` header.
- **Linux-specific test files** that use `fmemopen` must define `_GNU_SOURCE` before any system
  headers (see `tests/unit/test_system_linux_parse.c`).
- **Error handling**: functions return `int` (0 = success, negative = error). `hwb_run_benchmark`
  returns `-2` when a benchmark is not supported on the current platform; the CLI silently skips
  these in the quick suite and prints a message for `--bench`.
- **hwb_detect_hardware**: zero-initialises the output struct, guarantees `logical_cores >= 1`,
  `physical_cores >= logical_cores`, and non-empty strings (`"unknown"` fallback).
- **POSIX feature macros**: `_POSIX_C_SOURCE=200809L` is set only for the core library on
  non-Windows/non-macOS targets (via CMakeLists.txt); do not add it redundantly in source files.
- **Math library**: `libm` is linked automatically for Linux/non-Apple Unix targets in
  CMakeLists.txt; no need to add `-lm` manually.
- **No dynamic memory in core library paths** unless unavoidable; the CLI owns result allocation.

## Tests

The test binary is `hwbench-tests` (built from `tests/unit/test_*.c`). CTest also runs two
smoke tests against the CLI: `hwbench-smoke-list` and `hwbench-smoke-quick`.

Run all tests:
```bash
ctest --preset linux-gcc-release          # Linux
ctest --test-dir build/macos-clang-release --output-on-failure  # macOS
```

Add new unit tests by adding a `test_*.c` file to `tests/unit/` and registering it in
`CMakeLists.txt` under `add_executable(hwbench-tests ...)`.

## CI

Three GitHub Actions workflows run on every push to `master` and on every pull request:

| Workflow | File | Preset used |
|----------|------|-------------|
| Linux CI | `.github/workflows/linux.yml` | `linux-gcc-release` |
| macOS CI | `.github/workflows/macos.yml` | `macos-clang-release` |
| Windows CI | `.github/workflows/windows.yml` | `windows-msvc-release` |

Each workflow runs configure → build → `ctest`. PRs must pass all three before merging.
Always ensure `ctest` passes locally with the Linux preset before pushing.

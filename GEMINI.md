# hwbench-c Project Instructions

## Project Overview

`hwbench-c` is a portable, cross-platform hardware benchmark suite scaffold written primarily in C (C11). It features a command-line interface (CLI), a core benchmark harness, JSON result emission, and various benchmark workloads for testing CPU, memory, and storage performance. 

The project is designed to be highly modular, supporting plugins and optional backend libraries, though the current "bootstrap" implementation focuses on a robust baseline without external dependencies like graphics or compute engines (except optional OpenCL).

## Repository Structure

- **`apps/hwbench-cli/`**: Contains the main CLI entry point (`main.c`).
- **`src/`**: Contains the source code.
  - **`src/core/`**: Core benchmark harness, timer, stats, system metadata detection, and JSON output.
  - **`src/benches/`**: Individual benchmark implementations (e.g., CPU arithmetic, memory stream, storage I/O, stress).
- **`include/hwbench/`**: Public headers defining the core APIs (e.g., `bench.h`, `system.h`, `stats.h`).
- **`tests/`**: Unit and smoke tests.
- **`scripts/`**: Cross-platform helper scripts for setup, building, running, and testing (`.sh`, `.bat`, `.ps1`).
- **`CMakeLists.txt`** / **`CMakePresets.json`**: CMake build configuration and presets for various platforms.

## Building and Running

The project uses **CMake 3.25+** as its build system. Cross-platform scripts are provided in the `scripts/` directory to simplify common tasks.

### Using Scripts
- **Bash (Linux/macOS):** `./scripts/build.sh`, `./scripts/test.sh`, `./scripts/run.sh`
- **Cmd (Windows):** `scripts\build.bat`, `scripts\test.bat`, `scripts\run.bat`
- **PowerShell:** `./scripts/build.ps1`, `./scripts/test.ps1`, `./scripts/run.ps1`

### Using CMake Directly
```bash
# Configure
cmake --preset <preset-name> # e.g., linux-gcc-release, windows-msvc-release

# Build
cmake --build --preset <preset-name>

# Run Tests
ctest --preset <preset-name>

# Run CLI
./build/<preset-name>/hwbench-c --suite quick --out results.json
```

## Development Conventions

- **Language:** Written in **C11** (`CMAKE_C_STANDARD 11`). Adhere to C11 standards and avoid non-standard extensions unless explicitly required and guarded.
- **Testing:** Unit tests are plain C files located in `tests/unit/`. They do not use a heavyweight external testing framework; instead, they rely on simple return codes and basic assertions. Tests are orchestrated via **CTest**.
- **Build System:** Use CMake (`CMakeLists.txt`). When adding new files, update the corresponding `add_library` or `add_executable` calls in the CMake configuration. Keep optional features behind CMake `option()` flags.
- **Modularity:** Benchmarks should be registered via the internal registry API. Each distinct benchmark or hardware subsystem test should generally reside in its own file in `src/benches/`.
- **Licensing:** The project is intended to be permissively licensed (MIT / BSD-2-Clause style) and avoids copyleft dependencies for the core suite.
# Agent Instructions for `hwbench-c`

Use `.github/copilot-instructions.md` as the primary source of repository guidance.

## Fast path

1. Configure with a platform preset:
   - Linux: `cmake --preset linux-gcc-release`
   - macOS: `cmake --preset macos-clang-release`
   - Windows: `cmake --preset windows-msvc-release`
2. Build:
   - Linux: `cmake --build --preset linux-gcc-release`
   - macOS: `cmake --build build/macos-clang-release`
   - Windows: `cmake --build build/windows-msvc-release --config Release`
3. Test:
   - Linux: `ctest --preset linux-gcc-release`
   - macOS: `ctest --test-dir build/macos-clang-release --output-on-failure`
   - Windows: `ctest --test-dir build/windows-msvc-release -C Release --output-on-failure`

## Project facts

- C11 (`CMAKE_C_EXTENSIONS OFF`).
- Main libraries: `hwbench_core`, `hwbench_benches`.
- CLI executable: `hwbench-c` (`apps/hwbench-cli/main.c`).
- Unit/smoke tests: `hwbench-tests` plus CTest smoke cases.

## Code conventions

- Public symbols use `hwb_` prefix.
- Keep public declarations in `include/hwbench/*.h`.
- Preserve error-code behavior (`0` success, negative for errors, `-2` unsupported benchmark).
- For Linux tests using `fmemopen`, define `_GNU_SOURCE` before system headers.

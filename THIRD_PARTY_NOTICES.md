# THIRD_PARTY_NOTICES

This project currently does **not** vendor or bundle external third-party libraries in the
repository source tree.

At build/runtime it uses platform-provided components:

- C standard library implementation (toolchain/platform provided).
- Native threading APIs:
  - POSIX threads (`pthread`) on Unix-like platforms.
  - Win32 threading APIs on Windows.

These components are distributed under the terms provided by the operating system and/or
compiler toolchain in use.

## Optional integrations (currently disabled by default)

`hwbench-c` exposes CMake options for optional adapters/backends (for example SDL3, bgfx,
SQLite, xxHash, Zstandard, libjpeg-turbo, clpeak, vkpeak, Dawn), but they are not integrated
into the default build in this repository at this time.

If/when such dependencies are enabled and linked, this file should be updated with their
corresponding license notices.

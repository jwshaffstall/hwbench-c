cmake_minimum_required(VERSION 3.25)
project(hwbench-c VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

include(CTest)

option(HWB_BUILD_SHARED "Build shared libraries" OFF)
option(HWB_BUILD_TESTS "Build unit tests" ON)
option(HWB_BUILD_RENDER "Enable render benchmarks" OFF)
option(HWB_BUILD_COMPUTE "Enable compute benchmarks" OFF)
option(HWB_USE_SDL3 "Use SDL3 platform layer" OFF)
option(HWB_USE_BGFX "Enable bgfx renderer backend" OFF)
option(HWB_USE_SQLITE "Enable SQLite benchmarks" OFF)
option(HWB_USE_XXHASH "Enable xxHash benchmarks" OFF)
option(HWB_USE_ZSTD "Enable Zstandard benchmarks" OFF)
option(HWB_USE_JPEG_TURBO "Enable libjpeg-turbo benchmarks" OFF)
option(HWB_USE_CLPEAK "Enable clpeak adapter" OFF)
option(HWB_USE_VKPEAK "Enable vkpeak adapter" OFF)
option(HWB_USE_DAWN "Enable Dawn/WebGPU backend" OFF)

set(HWB_LIB_TYPE STATIC)
if(HWB_BUILD_SHARED)
  set(HWB_LIB_TYPE SHARED)
endif()

add_library(hwbench_core ${HWB_LIB_TYPE}
  src/core/bench.c
  src/core/json.c
  src/core/stats.c
  src/core/timer.c
  src/core/system.c
)
target_include_directories(hwbench_core PUBLIC include)
if(NOT WIN32 AND NOT APPLE)
  target_compile_definitions(hwbench_core PRIVATE _POSIX_C_SOURCE=200809L)
endif()

if(UNIX AND NOT APPLE)
  target_link_libraries(hwbench_core PUBLIC m)
endif()

add_library(hwbench_benches ${HWB_LIB_TYPE}
  src/benches/cpu_scalar_int_add.c
  src/benches/cpu_scalar_fp_fma.c
  src/benches/memory_stream_copy.c
  src/benches/memory_stream_scale.c
  src/benches/memory_stream_add.c
  src/benches/memory_stream_triad.c
  src/benches/memory_latency_pointer_chase.c
  src/benches/storage_seq_read.c
  src/benches/storage_seq_write.c
  src/benches/storage_rand4k_read.c
  src/benches/storage_rand4k_write.c
  src/benches/register.c
)
target_include_directories(hwbench_benches PUBLIC include)
target_link_libraries(hwbench_benches PUBLIC hwbench_core)

add_executable(hwbench-c apps/hwbench-cli/main.c)
target_link_libraries(hwbench-c PRIVATE hwbench_core hwbench_benches)

if(HWB_BUILD_TESTS)
  add_executable(hwbench-tests
    tests/unit/test_main.c
    tests/unit/test_stats.c
    tests/unit/test_registry.c
    tests/unit/test_system.c
    tests/unit/test_system_linux_parse.c
  )
  target_link_libraries(hwbench-tests PRIVATE hwbench_core hwbench_benches)
  add_test(NAME hwbench-tests COMMAND hwbench-tests)

  add_test(NAME hwbench-smoke-list COMMAND hwbench-c --list)
  add_test(NAME hwbench-smoke-quick COMMAND hwbench-c --suite quick --samples 3 --warmup-ms 10 --min-sample-ms 10)
  add_test(NAME hwbench-smoke-suite-cpu-memory COMMAND hwbench-c --suite cpu,memory --samples 1 --warmup-ms 5 --min-sample-ms 5)
  add_test(NAME hwbench-smoke-suite-invalid COMMAND hwbench-c --suite cpu,,memory)
  set_tests_properties(hwbench-smoke-suite-invalid PROPERTIES WILL_FAIL TRUE)
endif()

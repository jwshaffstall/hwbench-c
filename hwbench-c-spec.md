# hwbench-c (working title)
## Design and implementation specification for a cross-platform C hardware benchmark suite

Version: 0.1 draft
Date: 2026-02-28
Status: proposed

## Executive summary

This document proposes **hwbench-c**, an open-source, permissively licensed, cross-platform benchmark suite written primarily in portable C and built with CMake. The suite is intended to run locally on **Windows, macOS, Linux, Android, and iOS**, with support for major toolchains including MSVC, clang-cl, Clang, GCC, Apple Clang, Xcode, and Android NDK Clang. CMake's toolchain model is a good fit for this because it explicitly supports host builds and cross-compiling through toolchain files and platform-specific variables such as Android and Apple target settings. [R1][R2][R3][R4]

The project should measure:

- Single-core CPU performance
- Multi-core CPU performance
- Memory bandwidth and latency behavior
- SIMD / vector instruction performance
- CPU cache hierarchy behavior
- Storage / filesystem performance
- Graphics rendering performance
- Graphics memory bandwidth / transfer behavior
- GPU compute performance

I am assuming "multi-color CPU" in the request meant **multi-core CPU**.

The recommended architecture is a **small custom benchmark harness in C** at the center, with **optional adapters for selected existing permissive open-source benchmarks and libraries** where they add clear value. The suite should avoid depending on one giant external framework that dictates every benchmark category. Instead, it should expose a stable internal benchmark API, a machine-readable JSON result format, and pluggable benchmark modules.

My recommended technical direction after comparing options is:

1. **Custom C harness** for timing, scheduling, statistics, calibration, and result output.
2. **SDL3** as the default cross-platform app/runtime layer for windows, timing, threads, filesystem helpers, and mobile app integration where useful. SDL is written in C, officially supports Windows, macOS, Linux, iOS, and Android, and is under the zlib license. SDL also exposes a high-resolution counter intended for profiling. [R5][R6][R7]
3. **bgfx** as the initial graphics rendering benchmark backend because it already supports a broad set of rendering APIs and platforms, is BSD-2-Clause licensed, and has a documented C99 API example. [R8][R9][R10]
4. **Native compute backends** behind one internal abstraction:
   - Vulkan compute for Windows/Linux/Android and optionally Apple through MoltenVK
   - Metal compute for Apple platforms
   - D3D12 compute for Windows
   - Optional experimental WebGPU/Dawn backend later
   Dawn is attractive because it implements `webgpu.h` across D3D12, Metal, Vulkan, and OpenGL, but its current support matrix still lists Android as work in progress and iOS as best-effort, so it should not be the only required compute path in v1. [R11][R12][R13][R14]
5. **Selected third-party benchmark payloads** for realistic workloads: STREAM-style memory kernels, SQLite `speedtest1.c`, xxHash, Zstandard, libjpeg-turbo, clpeak, and vkpeak. STREAM, SQLite, xxHash, Zstandard, libjpeg-turbo, clpeak, and vkpeak each provide useful coverage for memory, database/file-backed work, hashing, compression, image codec SIMD use, and synthetic GPU peak testing. [R15][R16][R17][R18][R19][R20][R21][R22][R23][R24]

## Project goals

### Primary goals

- Be easy to clone, configure, build, and run locally with CMake presets.
- Be mostly written in C with a small, auditable codebase.
- Produce trustworthy, repeatable results with clear metadata.
- Run on desktop and mobile platforms without requiring cloud infrastructure.
- Allow both quick smoke tests and deeper benchmark runs.
- Separate "portable everywhere" benchmarks from "backend-specific" ones.
- Support both synthetic microbenchmarks and representative real-world workloads.
- Keep licensing permissive and distribution-friendly.

### Non-goals for v1

- Competing with Phoronix Test Suite on breadth.
- Reproducing every vendor-specific profiler feature.
- Providing kernel-driver-level storage analysis.
- Providing browser-based benchmarking as a first-class target.
- Attempting perfect cross-platform score comparability for GPU compute across different APIs.

## Naming

A quick search shows that **hwbench** is already in use by at least an existing GitHub project and the `hwbench.com` site, so the project should probably not ship under the bare `hwbench` name. [R25]

Recommended names:

- `hwbench-c` - still readable, C-focused, and likely acceptable as a repo name
- `benchforge-c` - more brandable
- `forgebench-c` - short and distinctive
- `ironbench-c` - memorable, though less descriptive

My recommendation is to use:

**Project name:** hwbench-c  
**Repository slug:** `hwbench-c`

## Product definition

### What hwbench-c is

A modular benchmark suite with:

- A core runner executable
- A registry of benchmark modules
- Optional backend libraries for graphics and compute
- Optional third-party payloads vendored as submodules or fetched dependencies
- JSON and console output
- Optional HTML report generation later

### What a user should be able to do

Examples:

```bash
hwbench-c --list
hwbench-c --suite quick
hwbench-c --suite cpu,memory,storage
hwbench-c --bench cpu.scalar.add --threads 1
hwbench-c --bench cpu.hash.blake3 --threads all
hwbench-c --bench render.bgfx.cubes --frames 5000
hwbench-c --bench compute.vulkan.saxpy --size 268435456
hwbench-c --out results/my-machine.json
```

## Benchmark taxonomy

The suite should organize benchmarks into nine top-level families.

### 1. CPU scalar

Purpose: measure instruction throughput, branch behavior, integer and floating-point arithmetic, and compiler optimization quality.

Representative benchmarks:

- Integer add/mul/div loops
- Floating-point add/mul/fma loops
- Branch-heavy control-flow kernels
- Function call overhead
- Atomic increment / compare-exchange contention
- Thread scheduling overhead

Metrics:

- ns/op
- ops/s
- cycles/op where available
- instructions retired where available
- branch mispredict rate where available

### 2. CPU SIMD / vectorization

Purpose: measure ISA-specific vector throughput and compiler/intrinsics behavior.

Representative benchmarks:

- memcpy-style copy kernel
- SAXPY / AXPY
- dot product
- horizontal reduction
- 4x4 and 8x8 matrix kernels
- pixel transform kernels
- hash/compression/image-codec workloads using real libraries

ISA lanes to target:

- Scalar fallback
- SSE2
- SSE4.1
- AVX2
- AVX-512 where available
- NEON
- SVE / SVE2 optionally later

Design rules:

- Each ISA path lives in its own translation unit.
- Runtime dispatch selects the best supported path.
- The benchmark reports both selected ISA and fallback path.
- Results are never compared across different kernels as if they were identical.

### 3. Multi-core CPU

Purpose: measure scaling under independent work and contended work.

Representative benchmarks:

- Embarrassingly parallel compute kernels
- Hashing many independent buffers
- Compression of independent blocks
- Thread-pool queue throughput
- Producer/consumer ring buffers
- False-sharing stress
- NUMA-aware variants on supported desktop/server systems

Metrics:

- speedup vs 1 thread
- efficiency vs ideal linear scaling
- throughput per watt if power APIs are added later
- tail latency under contention

### 4. Memory subsystem

Purpose: measure sustained throughput and latency-sensitive access patterns.

Representative benchmarks:

- STREAM-like copy, scale, add, triad
- sequential read/write bandwidth
- strided access bandwidth
- random pointer chasing
- TLB-sensitive page-walk tests
- read-for-ownership and write-combine patterns

Rationale:

The original STREAM benchmark is explicitly designed to use datasets much larger than available cache and is widely treated as a sustained memory-bandwidth benchmark, so hwbench-c should include either a licensed-compatible import or a compatible "STREAM-style" module with clear labeling. If official STREAM code is bundled or results are published as STREAM results, the suite must respect STREAM's run rules. [R15][R16]

### 5. CPU cache hierarchy

Purpose: estimate working-set transitions across L1/L2/L3/system memory.

Representative benchmarks:

- dependent pointer-chase latency sweep
- bandwidth sweep by working-set size
- associativity stress patterns
- cacheline sharing stress
- prefetch-friendly vs prefetch-hostile patterns

Outputs:

- latency curve by working-set size
- inferred transition points
- best-effort cache hierarchy summary

Important note:

This suite should not pretend to perfectly detect all cache sizes from timings alone. It should present measured curves and optional inferred breakpoints, not fake certainty.

### 6. Storage / filesystem

Purpose: approximate local storage behavior from user space.

Representative benchmarks:

- large sequential read/write
- random 4 KiB read/write
- sync-heavy small writes
- metadata-heavy create/stat/delete workload
- mmap read sweep
- SQLite workload via `speedtest1.c`
- optional blob key/value workload later

Rationale:

SQLite documents `speedtest1.c` as a program that estimates performance under a typical workload, and SQLite itself is public domain, which makes it a strong candidate for a bundled real-world storage/database benchmark. [R17][R18][R19]

Platform caveat:

On iOS and Android, these tests measure performance inside the app sandbox and approved filesystem APIs. They are still useful, but they are not raw block-device benchmarks.

### 7. Graphics rendering

Purpose: measure draw submission, state changes, batching, overdraw, fill, texture sampling, and simple scene throughput.

Representative scenes:

- clear-only baseline
- sprite batch throughput
- instanced cube scene
- forward-lit mesh scene
- alpha-blended particles
- post-process fullscreen pass chain
- render-target ping-pong

Recommended backend:

Use bgfx first. It supports Direct3D 11, Direct3D 12, Metal, OpenGL, OpenGL ES, Vulkan, and WebGPU via Dawn Native, is BSD-2-Clause licensed, supports Android/iOS/macOS/Linux/Windows, and has C99 API examples. That combination makes it unusually practical for a benchmark suite whose control logic is in C. [R8][R9][R10]

### 8. Graphics memory / transfer behavior

Purpose: measure upload/download and resource-management costs.

Representative benchmarks:

- dynamic vertex buffer update throughput
- texture upload throughput
- staging-to-device copy throughput
- render-target readback throughput
- uniform / push-constant update stress
- large resource creation/destruction churn

Outputs:

- MB/s or GB/s
- frame-time spikes caused by uploads
- readback latency

### 9. GPU compute

Purpose: measure general-purpose compute throughput and transfer overhead.

Representative kernels:

- SAXPY / vector add
- reduction
- scan / prefix sum
- matrix multiply
- histogram
- image convolution
- bitonic sort or radix sort later

Backend strategy:

- **Vulkan compute** for the broadest portable native path
- **Metal compute** for Apple-first native path
- **D3D12 compute** for Windows-first native path
- **Optional Dawn/WebGPU** experimental backend later

Apple portability path:

MoltenVK provides Vulkan capability on macOS and iOS by layering Vulkan over Metal and is compatible with standard Apple distribution channels, so it is a useful portability option. However, because it is a Vulkan portability implementation over Metal, Metal-native compute should remain a first-class backend for Apple systems. [R13][R14]

Synthetic peak tools:

- `clpeak` for OpenCL peak capability testing where OpenCL is available
- `vkpeak` for Vulkan peak capability testing

Both projects explicitly describe themselves as synthetic vector-operation peak tests rather than real-world workloads, which is exactly how hwbench-c should label them. [R23][R24]

## Measurement model

Each benchmark must declare:

- category
- benchmark id
- short name
- long description
- units
- setup cost profile
- warmup strategy
- minimum runtime target
- supported platforms
- supported architectures
- required features
- determinism level
- whether results are synthetic or representative

### Standard benchmark life cycle

1. Parse CLI and resolve benchmark selection.
2. Probe hardware and OS metadata.
3. Validate required features.
4. Prepare isolated output directory.
5. Warm up benchmark.
6. Calibrate iteration count toward a target minimum sample duration.
7. Execute multiple samples.
8. Compute summary statistics.
9. Serialize machine-readable results.
10. Optionally emit text table and plots.

### Timing rules

- Use a monotonic high-resolution timer.
- Prefer platform-native timers internally.
- SDL's performance counter may be used as a portable default abstraction where appropriate. SDL explicitly documents it as a high-resolution profiling counter. [R7]
- Separate one-time setup from steady-state timing.
- Report warmup separately from measured samples.
- Use median and percentile summaries, not only mean.
- Detect obviously unstable runs and flag them.

### Statistics to report

Per benchmark:

- min
- max
- median
- mean
- standard deviation
- p5 / p25 / p75 / p95 / p99 when enough samples exist
- coefficient of variation
- sample count
- total measured time

### Noise control

Desktop options:

- optional process priority boost
- optional CPU affinity pinning
- optional thread pinning per worker
- optional governor / power-plan advisory warnings

Mobile options:

- battery level capture
- thermal-state capture when available
- foreground-only execution warnings
- device idle / charging advisory notes

The suite should warn when it cannot control important noise sources.

## Result schema

The default output should be JSON.

Top-level fields:

```json
{
  "schema_version": "1.0",
  "suite_version": "0.1.0",
  "run_id": "uuid",
  "timestamp_utc": "2026-02-28T00:00:00Z",
  "machine": {},
  "os": {},
  "toolchain": {},
  "git": {},
  "environment": {},
  "benchmarks": []
}
```

Each benchmark result should include:

```json
{
  "id": "cpu.hash.xxhash",
  "category": "cpu",
  "variant": "xxh3_avx2",
  "threads": 8,
  "units": "GB/s",
  "samples": [12.1, 12.3, 12.0, 12.4],
  "summary": {
    "median": 12.2,
    "mean": 12.2,
    "stdev": 0.15,
    "p95": 12.4
  },
  "metadata": {
    "isa": "AVX2",
    "dataset_bytes": 1073741824,
    "warmup_ms": 500,
    "measured_ms": 4000,
    "synthetic": false
  }
}
```

## Internal architecture

### Core libraries

- `hwbench_core`
  - registry
  - CLI parsing
  - JSON output
  - timers
  - CPU feature detection
  - affinity helpers
  - statistics
  - dataset generation
  - result serialization

- `hwbench_platform`
  - OS abstraction layer
  - threads / mutex / semaphore wrappers if needed
  - filesystem paths
  - power / thermal hooks where available

- `hwbench_render`
  - render benchmark interface
  - bgfx backend adapter
  - optional native backend adapters later

- `hwbench_compute`
  - compute benchmark interface
  - Vulkan backend
  - Metal backend
  - D3D12 backend
  - optional Dawn backend later

- `hwbench_ext`
  - adapters for bundled third-party projects

### Benchmark registration API

Example C API shape:

```c
typedef struct hwb_benchmark_desc {
    const char* id;
    const char* category;
    const char* name;
    const char* unit;
    uint32_t flags;
    bool (*is_supported)(const struct hwb_context* ctx);
    int  (*prepare)(struct hwb_context* ctx, void** state);
    int  (*run)(struct hwb_context* ctx, void* state, struct hwb_sample* out);
    void (*cleanup)(struct hwb_context* ctx, void* state);
} hwb_benchmark_desc;
```

### Benchmark classes

Three classes:

- **micro** - tiny kernels, usually synthetic
- **component** - focused real library/component tests
- **scenario** - larger end-to-end scenes or workflows

This classification should be visible in output so users do not confuse a peak synthetic kernel with a realistic workload.

## Third-party dependency policy

### Principles

- Prefer permissive licenses: public domain, CC0, zlib, MIT, BSD, Apache-2.0.
- Keep all third-party code optional at configure time where feasible.
- Vendor exact revisions through submodules or `FetchContent` with lockfile-like pinning.
- Record upstream revision and license in results and about output.

### Recommended bundled / optional components

#### Strong recommendations

1. **SDL3** - platform/runtime helper layer; C; zlib. [R5][R6]
2. **bgfx** - rendering backend; BSD-2-Clause; broad backend/platform coverage; C99 examples exist. [R8][R9][R10]
3. **SQLite** - storage/database benchmark payload through `speedtest1.c`; public domain. [R17][R18][R19]
4. **xxHash** - fast hashing throughput; BSD-2-Clause. [R20][R21]
5. **Zstandard** - compression/decompression throughput; dual BSD/GPLv2 reference implementation, so use only under the BSD terms and preserve notices carefully. [R22]
6. **libjpeg-turbo** - codec and SIMD-heavy real-world workload; BSD-style licensing. [R26]

#### Good optional additions

7. **clpeak** - OpenCL synthetic peak tests; Apache-2.0. [R23]
8. **vkpeak** - Vulkan synthetic peak tests; MIT. [R24]

#### Use carefully

9. **Official STREAM code** or a STREAM-style compatible module. The original benchmark is very useful, but its naming and published-result rules matter. If exact STREAM code is bundled, the suite must carry its license/run-rule documentation clearly. [R15][R16]

## Build and packaging specification

### CMake baseline

Recommended minimum: **CMake 3.25+**

Reasons:

- strong preset support
- better Apple and Android workflows
- good modern target usage patterns

CMake should be the only required meta-build system. It already documents cross-compiling through toolchain files and Android-specific variables, and supports Apple architecture selection through standard variables. [R1][R2][R3][R4]

### Top-level options

```cmake
option(HWB_BUILD_SHARED "Build shared libraries" OFF)
option(HWB_BUILD_TESTS "Build unit tests" ON)
option(HWB_BUILD_RENDER "Enable render benchmarks" ON)
option(HWB_BUILD_COMPUTE "Enable compute benchmarks" ON)
option(HWB_USE_SDL3 "Use SDL3 platform layer" ON)
option(HWB_USE_BGFX "Enable bgfx renderer backend" ON)
option(HWB_USE_SQLITE "Enable SQLite benchmarks" ON)
option(HWB_USE_XXHASH "Enable xxHash benchmarks" ON)
option(HWB_USE_ZSTD "Enable Zstandard benchmarks" ON)
option(HWB_USE_JPEG_TURBO "Enable libjpeg-turbo benchmarks" ON)
option(HWB_USE_CLPEAK "Enable clpeak adapter" OFF)
option(HWB_USE_VKPEAK "Enable vkpeak adapter" OFF)
option(HWB_USE_DAWN "Enable Dawn/WebGPU backend" OFF)
```

### Presets

Ship `CMakePresets.json` with at least:

- `windows-msvc-debug`
- `windows-msvc-release`
- `windows-clangcl-release`
- `linux-gcc-release`
- `linux-clang-release`
- `macos-clang-release`
- `android-arm64-release`
- `ios-arm64-release`

### Platform notes

#### Windows

Supported toolchains:

- MSVC
- clang-cl
- MinGW-w64 GCC/Clang as community-tier support

Graphics backends:

- D3D11 via bgfx
- D3D12 via bgfx or native compute backend
- Vulkan where installed

#### macOS

Supported toolchain:

- Apple Clang / Xcode

Graphics/compute:

- Metal-first
- bgfx over Metal
- Vulkan path optionally through MoltenVK

#### Linux

Supported toolchains:

- GCC
- Clang

Graphics/compute:

- Vulkan
- OpenGL / OpenGL ES through bgfx where needed

#### Android

Supported toolchain:

- Android NDK Clang via CMake toolchain flow

Runtime:

- SDL3 app integration for benchmark shell app
- Vulkan compute/render where device support exists
- optional OpenCL / clpeak on compatible devices only

#### iOS

Supported toolchain:

- Xcode / Apple Clang

Runtime:

- SDL3 app integration or native app shell
- Metal compute
- bgfx over Metal
- optional Vulkan path via MoltenVK if practical

## Source tree layout

```text
hwbench-c/
  CMakeLists.txt
  CMakePresets.json
  cmake/
    toolchains/
    modules/
  docs/
    design.md
    benchmark-methodology.md
    result-schema.md
    licenses/
  include/
    hwbench/
  src/
    core/
    platform/
    cpu/
    memory/
    cache/
    storage/
    render/
    compute/
    ext/
  third_party/
    sdl3/
    bgfx/
    sqlite/
    xxhash/
    zstd/
    libjpeg-turbo/
    clpeak/
    vkpeak/
  benches/
    manifests/
    scenes/
    datasets/
  tests/
    unit/
    smoke/
    golden/
  scripts/
    ci/
    format/
    package/
  apps/
    hwbench-cli/
    hwbench-mobile/
```

## Benchmark manifest system

Every benchmark should also have a data manifest, even when compiled in. This helps document dataset sizes, warmup defaults, and stability expectations.

Example:

```yaml
id: memory.stream.triad
class: micro
category: memory
unit: GB/s
warmup_ms: 500
min_sample_ms: 1000
recommended_samples: 9
synthetic: true
threads:
  default: 1
  allow_all: true
requires:
  isa_any_of: [scalar, sse2, avx2, neon]
platforms: [windows, macos, linux, android, ios]
```

## Quality and correctness requirements

### Unit tests

- statistics correctness
- timer conversion correctness
- CPU feature detection sanity
- JSON schema validation
- deterministic dataset generation
- benchmark registration integrity

### Smoke tests

Each enabled benchmark module must have a short smoke configuration that runs in CI in under a few minutes.

### Golden-result tests

Not performance values, but correctness outputs:

- hash digests
- compression round-trip integrity
- image decode/encode correctness
- compute kernel output checksums
- rendering checksum or image-diff where practical

### Performance sanity tests

The suite should detect and fail obvious nonsense:

- negative durations
- impossible throughput
- zero-variance suspicious samples from broken timers
- mismatched dataset sizes
- wrong-thread-count reporting

## Reporting and UX

### Console output

Default human-readable table:

```text
BENCHMARK                    VARIANT        THREADS   MEDIAN     UNIT   CV%
cpu.hash.xxhash             xxh3_avx2      1         28.4       GB/s   0.8
memory.stream.triad         avx2           8         87.2       GB/s   1.2
storage.sqlite.speedtest1   default        1         2.31       score  3.7
render.bgfx.cubes           metal          1         4180       fps    1.9
```

### JSON output

Always available and stable.

### HTML report

Post-v1 feature. Generate plots for:

- scaling curves
- memory hierarchy curves
- per-ISA comparison
- render frame-time distributions
- GPU transfer throughput plots

## Recommended initial benchmark set for v1

### Must-have v1

- cpu.scalar.int_add
- cpu.scalar.fp_fma
- cpu.branch.mispredict
- cpu.simd.saxpy
- cpu.simd.dot
- cpu.hash.xxhash
- cpu.hash.blake3
- cpu.compress.zstd
- memory.stream.copy
- memory.stream.scale
- memory.stream.add
- memory.stream.triad
- cache.pointer_chase
- cache.bandwidth_sweep
- storage.seq_read
- storage.seq_write
- storage.rand4k_read
- storage.rand4k_write
- storage.sqlite.speedtest1
- render.bgfx.clear
- render.bgfx.sprites
- render.bgfx.cubes
- render.upload.texture_stream
- compute.vulkan.saxpy
- compute.metal.saxpy
- compute.d3d12.saxpy

### Nice-to-have v1.1

- libjpeg-turbo encode/decode benches
- histogram compute kernel
- reduction / scan compute kernels
- post-process render chain
- mmap storage benchmark
- false-sharing stress
- queue throughput benchmark
- thermal-state annotations on mobile

### Explicitly defer

- CUDA / ROCm native backends
- browser/WASM runner
- power and energy as a required cross-platform metric
- vendor-specific PMU integration as a core feature

## Cross-platform support tiers

### Tier 1 - expected first-class

- Windows x64
- macOS arm64
- Linux x64
- Android arm64
- iOS arm64

### Tier 2 - best effort

- Windows arm64
- Linux arm64
- macOS x64
- Android x86_64 emulator/dev workflows

### Tier 3 - community maintained

- older Apple Intel systems
- niche Linux distros
- experimental WebGPU backend targets

## CI strategy

Use GitHub Actions for the public matrix where feasible and allow optional local scripts for mobile/device deployment.

Suggested public CI matrix:

- Windows latest + MSVC
- Windows latest + clang-cl
- Ubuntu latest + GCC
- Ubuntu latest + Clang
- macOS latest + Apple Clang

Suggested device validation matrix outside standard hosted CI:

- Android arm64 physical device
- iPhone / iPad physical device

Each CI run should:

- build all enabled targets
- run unit tests
- run smoke benchmarks with tiny datasets
- validate JSON schema
- publish artifacts

## Licensing strategy

### Project license

Use **MIT** or **BSD-2-Clause** for the core project.

I recommend **MIT** for the project itself because it is familiar and broadly accepted, while preserving room to consume BSD/zlib/Apache/public-domain dependencies.

### Third-party handling

- Keep a `docs/licenses/` directory.
- Record exact upstream revisions.
- Include a generated third-party notices file in release artifacts.
- Keep optional dependencies truly optional.
- Do not vendor copyleft-only benchmarking tools into the default tree.

## Implementation roadmap

### Phase 0 - bootstrap

- repository scaffolding
- CMake presets
- core timing/statistics library
- JSON schema
- CLI runner
- one scalar CPU bench
- one memory bench

### Phase 1 - useful desktop baseline

- CPU scalar and SIMD modules
- multi-core scaling module
- cache sweep module
- storage microbench module
- xxHash / zstd / BLAKE3 adapters
- Windows/macOS/Linux release builds

### Phase 2 - graphics and compute

- SDL3 integration
- bgfx render backend
- Vulkan compute backend
- Metal compute backend
- D3D12 compute backend
- first graphics upload tests

### Phase 3 - mobile

- Android shell app
- iOS shell app
- sandbox-friendly storage paths
- thermal/battery metadata capture
- mobile preset documentation

### Phase 4 - external payloads and polish

- SQLite speedtest1 adapter
- libjpeg-turbo adapter
- clpeak / vkpeak integration
- HTML report generator
- baseline comparison tooling

## Key design decisions and rationale

### Why not rely on a single all-in-one benchmark suite?

Because the requested scope crosses CPU, memory, storage, rendering, graphics-memory behavior, and GPU compute. No single permissive C project neatly covers all of that while remaining pleasant to build on Windows/macOS/Linux/Android/iOS.

### Why a custom core harness?

Because timing methodology, metadata capture, JSON schema stability, and result classification are the real product. The external benchmarks are payloads, not the product architecture.

### Why bgfx for rendering first?

Because its combination of broad backend support, broad platform support, permissive BSD-2-Clause license, and documented C99 API examples makes it a strong practical fit for a C benchmark suite. [R8][R9][R10]

### Why not make Dawn the only GPU abstraction on day one?

Because it is promising and cross-platform, but its current support notes still show Android as work in progress and iOS as best-effort, which is not ideal for a mandatory baseline abstraction when reliable local builds on Android and iOS are core requirements. [R11][R12]

### Why include both synthetic and real workloads?

Because peak throughput kernels answer different questions than real libraries. clpeak and vkpeak are explicitly synthetic peak tools, while SQLite `speedtest1.c`, xxHash, Zstandard, and libjpeg-turbo better reflect real software behavior. [R17][R20][R21][R22][R23][R24][R26]

## Risks and mitigations

### Risk: false precision and misleading comparability

Mitigation:

- label synthetic vs representative clearly
- store full metadata
- avoid aggregate "one number" scores by default
- separate API/backend variants visibly

### Risk: mobile thermal throttling ruins repeatability

Mitigation:

- capture temperature/thermal-state hints when available
- support short and long modes
- warn when long runs drift badly

### Risk: optional dependency sprawl

Mitigation:

- keep the core suite useful with zero optional dependencies
- gate every third-party integration behind feature options
- ship a narrow recommended set first

### Risk: Apple / Android GPU backend complexity

Mitigation:

- decouple rendering and compute backends
- start with one clear benchmark per backend
- do not block the rest of the suite on full feature parity

## Recommended next step after this spec

The best next deliverable would be a **repository bootstrap plan** containing:

1. exact folder tree
2. top-level `CMakeLists.txt`
3. `CMakePresets.json`
4. JSON schema file
5. core benchmark registration API headers
6. one sample benchmark from each of CPU, memory, and storage
7. a short contributor/build guide

That would turn this design into a ready-to-implement scaffold.

## Sources

[R1] CMake, *cmake-toolchains(7)*. Documents CMake toolchains and cross-compiling through toolchain files. https://cmake.org/cmake/help/latest/manual/cmake-toolchains.7.html

[R2] CMake, *CMAKE_ANDROID_API*. Documents Android API targeting in cross-compiles. https://cmake.org/cmake/help/latest/variable/CMAKE_ANDROID_API.html

[R3] CMake, *CMAKE_OSX_ARCHITECTURES*. Documents Apple architecture targeting for macOS and iOS. https://cmake.org/cmake/help/latest/variable/CMAKE_OSX_ARCHITECTURES.html

[R4] CMake, *CMAKE_SYSTEM_NAME*. Documents target OS naming in builds. https://cmake.org/cmake/help/latest/variable/CMAKE_SYSTEM_NAME.html

[R5] SDL Wiki, *SDL3 Front Page*. Notes that SDL officially supports Windows, macOS, Linux, iOS, and Android; SDL is written in C; SDL3 uses the zlib license. https://wiki.libsdl.org/SDL3/FrontPage

[R6] SDL Wiki, *SDL3 README-platforms*. Lists supported SDL3 platforms. https://wiki.libsdl.org/SDL3/README-platforms

[R7] SDL Wiki, *SDL_GetPerformanceCounter*. Documents the high-resolution performance counter for profiling. https://wiki.libsdl.org/SDL3/SDL_GetPerformanceCounter

[R8] bgfx documentation, *Overview*. Describes bgfx as cross-platform, graphics API agnostic, BSD-2-Clause licensed, with broad backend support. https://bkaradzic.github.io/bgfx/overview.html

[R9] bgfx GitHub repository. Lists supported platforms and compilers. https://github.com/bkaradzic/bgfx

[R10] bgfx example `25-c99/helloworld.c`. Demonstrates the C99 API. https://github.com/bkaradzic/bgfx/blob/master/examples/25-c99/helloworld.c

[R11] Dawn README. Describes Dawn as an open-source cross-platform implementation of WebGPU / `webgpu.h`. https://github.com/google/dawn

[R12] Dawn support documentation. Notes OS/API support including Android work in progress and iOS best-effort. https://dawn.googlesource.com/dawn/+/HEAD/docs/support.md

[R13] MoltenVK GitHub repository. Describes MoltenVK as Vulkan functionality built on Apple's Metal framework for macOS and iOS-family platforms. https://github.com/KhronosGroup/MoltenVK

[R14] MoltenVK project page. Describes Vulkan on iOS and macOS via Metal and notes App Store compatibility. https://moltengl.com/moltenvk/

[R15] STREAM reference information. Explains that STREAM is designed for datasets much larger than cache and is intended to indicate sustained memory bandwidth. https://www.cs.virginia.edu/stream/ref.html

[R16] STREAM code/license information. Documents STREAM licensing and publication/run-rule constraints. https://www.cs.virginia.edu/stream/FTP/Code/LICENSE.txt

[R17] SQLite testing documentation. Notes that `speedtest1.c` estimates performance under a typical workload. https://sqlite.org/testing.html

[R18] SQLite CPU/performance documentation. Describes `speedtest1.c` as a typical workload generator used for SQLite performance measurement. https://sqlite.org/cpu.html

[R19] SQLite release history. Notes that SQLite code is in the public domain. https://sqlite.org/changes.html

[R20] xxHash repository. Describes xxHash as an extremely fast, highly portable hash algorithm. https://github.com/Cyan4973/xxHash

[R21] xxHash license. BSD-2-Clause license. https://github.com/Cyan4973/xxHash/blob/dev/LICENSE

[R22] Zstandard repository and license materials. Documents the reference implementation as dual BSD or GPLv2 licensed. https://github.com/facebook/zstd

[R23] clpeak repository. Documents clpeak as an Apache-2.0 synthetic benchmark for OpenCL peak capabilities using vector operations. https://github.com/krrishnarraj/clpeak

[R24] vkpeak repository. Documents vkpeak as an MIT-licensed synthetic benchmark for Vulkan peak capabilities using vector operations. https://github.com/nihui/vkpeak

[R25] Existing `hwbench` usage discovered during research, including the Criteo repository and hwbench.com. https://github.com/criteo/hwbench and https://hwbench.com/

[R26] libjpeg-turbo repository. Documents BSD-style licensing and SIMD-accelerated JPEG compression/decompression. https://github.com/libjpeg-turbo/libjpeg-turbo and https://libjpeg-turbo.org/

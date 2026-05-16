# hwbench-c

`hwbench-c` is a portable C benchmark suite with a CLI, core harness, JSON output, and coverage across CPU, memory, storage, and GPU benchmarks.

## Implemented now

- CMake 3.25+ project with cross-platform-oriented options and presets.
- Core benchmark harness:
  - benchmark registry and dispatch
  - monotonic high-resolution timer
  - summary statistics (min/max/median/mean/stdev/percentiles/CV)
  - JSON result emission
  - shared sample runner (`hwb_run_samples`)
- Initial benchmarks:
  - `cpu.scalar.int_add`
  - `cpu.scalar.fp_fma`
  - `cpu.parallel.int_add.{2c..8c}`
  - `cpu.simd.{i8,i16,i32,i64,f32,f64}_add`
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
- Stress tests: `stress.cpu` (timed), `stress.gpu` (timed, OpenCL).
- Unit/smoke tests via CTest.

## How benchmarking works

### Execution flow

1. The CLI initialises the benchmark registry via `hwb_register_default_benches()`, which populates a `hwb_registry` with an array of `hwb_benchmark_desc` pointers.
2. The CLI resolves which benchmarks to run based on `--bench <id>`, `--suite quick|cpu|memory|storage|gpu`, or runs all by default.
3. For each selected benchmark, the CLI calls `hwb_run_benchmark()`, which:
   - Checks `is_supported` (returns `-2` if unsupported).
   - Calls the benchmark's `run` function.
4. The benchmark's `run` function:
   - Zero-initialises and populates metadata on `hwb_benchmark_result`.
   - Calls `hwb_run_samples()` with a warmup callback and a sample callback.
   - `hwb_run_samples()` handles the timing loop:
     - **Warmup phase**: runs `warmup_pass` repeatedly until `ctx->warmup_ms` elapses.
     - **Sample phase**: runs `sample_pass` exactly `ctx->samples` times (capped at `HWB_MAX_SAMPLES` = 64), recording one `double` value per pass.
     - Computes summary statistics via `hwb_compute_stats()`.
5. Results are printed as tab-separated rows and written to a JSON file.

### The `hwb_run_samples` runner

Most benchmarks use the shared `hwb_run_samples` runner defined in `src/core/bench.c`. It accepts two callbacks:

```c
typedef int (*hwb_bench_warmup_pass_fn)(void* user_data);
typedef int (*hwb_bench_sample_pass_fn)(void* user_data, double* out_value);

int hwb_run_samples(const hwb_context* ctx,
                    hwb_bench_warmup_pass_fn warmup_pass,
                    hwb_bench_sample_pass_fn sample_pass,
                    void* user_data,
                    hwb_benchmark_result* out);
```

- **`warmup_pass`** — performs one warmup iteration. Called repeatedly until warmup time elapses. May be `NULL` for benchmarks that don't need warmup.
- **`sample_pass`** — performs one measured sample. The runner reads the wall-clock before and after the call; the callback writes the computed metric (e.g. Mops/s, GB/s, IOPS) to `*out_value`.
- **`user_data`** — a pointer to benchmark-specific state (buffers, iteration counts, file handles, etc.) passed to both callbacks.

The runner guarantees:
- Warmup results are discarded; only elapsed warmup time is recorded.
- Each sample is independently timed (no batching).
- Stats (min, max, median, mean, stdev, p5, p25, p75, p95, p99, CV) are computed after all samples.

### Benchmarks that don't use `hwb_run_samples`

Some benchmarks have structurally different execution models and manage their own timing:

| Benchmark | Reason |
|---|---|
| `memory.latency.pointer_chase` | Step-count calibration loop before warmup; timing is inside a helper function |
| `cpu.parallel.int_add.*` | Thread create/join inside each sample (includes thread overhead) |
| `cpu.simd.*_add` | Massive ISA-dispatching kernel; returns lane count for metric scaling |
| `stress.cpu` / `stress.gpu` | Duration-based loop (not sample-based); produces a single aggregated result |

### Error contract

All benchmark functions return `int`:

| Return | Meaning |
|---|---|
| `0` | Success |
| `-1` | Error (allocation failure, I/O error, etc.) |
| `-2` | Unsupported on this platform (silently skipped in quick suite) |

### CLI flags

| Flag | Description |
|---|---|
| `--list` | Print all registered benchmarks and exit |
| `--bench <id>` | Run a single benchmark by ID |
| `--suite quick` | Run all benchmarks (silently skips unsupported) |
| `--suite cpu,memory,storage,gpu` | Run benchmarks matching selected categories |
| `--samples N` | Number of samples per benchmark (default 7, max 64) |
| `--warmup-ms N` | Warmup duration in milliseconds (default 250) |
| `--min-sample-ms N` | Minimum target duration per sample (default 500) |
| `--threads N` | Thread count for parallel benchmarks (default 1) |
| `--stress 10\|30\|60` | Run timed CPU (+ GPU if available) stress test |
| `--out <path>` | JSON output path (default `hwbench-results.json`) |

## Pragmatic deviations from spec

The specification is broad and targets many optional third-party integrations and GPU backends. This implementation intentionally focuses on a robust Phase 0 baseline:

- Render and compute backends are disabled by default (`HWB_BUILD_RENDER=OFF`, `HWB_BUILD_COMPUTE=OFF`).
- Optional third-party adapters (SDL3, bgfx, SQLite, xxHash, Zstd, etc.) are represented as CMake options but not yet integrated.

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

`hwbench-c` detects and reports local hardware metadata for benchmark context:

- CPU model
- Logical and physical core counts
- Total system memory
- Total root-drive capacity and best-effort storage model
- Best-effort GPU identifier

This metadata is shown in CLI output before benchmark rows and is included under `machine` in JSON result files. Hardware is detected once at startup and passed to both the human-readable report and the JSON emitter.

## Adding a new benchmark

### Standard benchmarks (use `hwb_run_samples`)

1. **Create `src/benches/<id>.c`.** The file needs:

   - A `typedef struct` for benchmark-specific state (buffers, iteration counts, etc.).
   - A `static bool <name>_supported(const hwb_context* ctx)` function. Return `true` if the benchmark can run on this host, `false` otherwise.
   - A `static int <name>_warmup(void* user_data)` function. Performs one warmup pass. Return `0` on success, `-1` on error.
   - A `static int <name>_sample(void* user_data, double* out_value)` function. Performs one measured sample, writes the metric to `*out_value`. Return `0` on success, `-1` on error.
   - A `static int <name>_run(const hwb_context* ctx, hwb_benchmark_result* out)` function that:
     - Zero-initialises `out` and sets metadata fields (`id`, `category`, `variant`, `unit`, `threads`, `class_kind`, `synthetic`).
     - Allocates any resources (buffers, files, etc.).
     - Initialises the state struct.
     - Calls `hwb_run_samples(ctx, warmup, sample, &state, out)`.
     - Cleans up resources and returns the result.
   - A `const hwb_benchmark_desc hwb_bench_<id>` descriptor at file scope.

2. **Register it in `src/benches/register.c`.** Add an `extern` declaration and add `&hwb_bench_<id>` to the `entries` array.

3. **Add the `.c` file to the CMake target** if it isn't already picked up by a glob (the project uses explicit source lists in `CMakeLists.txt`).

### Non-standard benchmarks

For benchmarks with structurally different execution models (duration-based loops, thread-per-sample, calibration phases), implement the full timing logic inside the `run` function directly. See `src/benches/stress.c` or `src/benches/memory_latency_pointer_chase.c` for examples.

### Example: minimal CPU benchmark

```c
#include "hwbench/bench.h"
#include "hwbench/timer.h"

#include <stdint.h>
#include <string.h>

typedef struct {
  volatile uint64_t acc;
  uint64_t iters;
} my_bench_state;

static bool my_bench_supported(const hwb_context* ctx) {
  (void)ctx;
  return true;
}

static int my_bench_warmup(void* user_data) {
  my_bench_state* st = (my_bench_state*)user_data;
  for (uint64_t i = 0; i < st->iters / 10; ++i) {
    st->acc += i;
  }
  return 0;
}

static int my_bench_sample(void* user_data, double* out_value) {
  my_bench_state* st = (my_bench_state*)user_data;
  double t0 = hwb_now_seconds();
  for (uint64_t i = 0; i < st->iters; ++i) {
    st->acc += i;
  }
  double t1 = hwb_now_seconds();
  *out_value = (double)st->iters / (t1 - t0) / 1e6;  // Mops/s
  return 0;
}

static int my_bench_run(const hwb_context* ctx, hwb_benchmark_result* out) {
  memset(out, 0, sizeof(*out));
  out->id = "my.bench";
  out->category = "cpu";
  out->variant = "scalar";
  out->unit = "Mops/s";
  out->threads = ctx->threads;
  out->class_kind = HWB_BENCH_CLASS_MICRO;
  out->synthetic = true;

  my_bench_state st = {.acc = 0, .iters = 25000000ULL};
  return hwb_run_samples(ctx, my_bench_warmup, my_bench_sample, &st, out);
}

const hwb_benchmark_desc hwb_bench_my_bench = {
  .id = "my.bench",
  .category = "cpu",
  .name = "My benchmark",
  .unit = "Mops/s",
  .variant = "scalar",
  .class_kind = HWB_BENCH_CLASS_MICRO,
  .synthetic = true,
  .is_supported = my_bench_supported,
  .run = my_bench_run,
};
```

### Anti-optimisation tips

Compilers will eliminate loops whose results are unused. Common defences:

- Declare accumulators as `volatile`.
- After the sample loop, accumulate results into a `volatile double sink` and `(void)sink`.
- For SIMD benchmarks, use `memcpy` to extract a lane value into a volatile.

### Benchmark categories

The `category` field determines suite membership:

| Category | Suite | Description |
|---|---|---|
| `cpu` | `--suite cpu` | CPU compute benchmarks |
| `memory` | `--suite memory` | Memory bandwidth and latency |
| `storage` | `--suite storage` | Storage I/O throughput and IOPS |
| `gpu` | `--suite gpu` | GPU compute (OpenCL) |
| `stress` | (not in quick suite) | Timed stress tests |

### `class_kind` values

| Value | Meaning |
|---|---|
| `HWB_BENCH_CLASS_MICRO` | Single-operation micro-benchmark (e.g. scalar add) |
| `HWB_BENCH_CLASS_COMPONENT` | Multi-step component benchmark (e.g. STREAM, storage I/O) |
| `HWB_BENCH_CLASS_SCENARIO` | End-to-end scenario (e.g. stress tests) |

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

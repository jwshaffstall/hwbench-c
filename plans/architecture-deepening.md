# Architecture Deepening Opportunities

> Generated: 2026-05-16
> Scope: hwbench-c codebase (core library, benchmarks, CLI)

## Glossary

Terms used throughout follow the project's LANGUAGE.md definitions:

- **Module** — anything with an interface and an implementation.
- **Interface** — everything a caller must know: types, invariants, error modes, ordering, config.
- **Implementation** — the code inside.
- **Depth** — leverage at the interface: a lot of behaviour behind a small interface.
- **Seam** — where an interface lives; a place behaviour can be altered without editing in place.
- **Adapter** — a concrete thing satisfying an interface at a seam.
- **Locality** — change, bugs, knowledge concentrated in one place.

---

## 1. JSON Emitter Leaks Hardware Detection Seam

**Files:** `src/core/json.c`, `apps/hwbench-cli/main.c`

### Problem

`hwb_write_json_results` calls `hwb_detect_hardware` internally (`json.c:65`) to populate the `machine` section. The JSON module's Interface is not "serialize these results" — it is "serialize results AND re-detect hardware." The CLI also calls `hwb_detect_hardware` separately for the human-readable report (`main.c:36`), so hardware is detected **twice** per run. This is a concern leak across seams.

Apply the **deletion test**: delete the `hwb_detect_hardware` call from `json.c` and the caller (CLI) must supply hardware info — which it already has. The complexity concentrates in the CLI where it belongs.

### Solution

Pass `hwb_hardware_info` as a parameter to `hwb_write_json_results`. The CLI detects once, passes the struct to both the human-readable report and the JSON emitter.

New Interface:
```c
int hwb_write_json_results(const char* path,
                           const hwb_benchmark_result* results,
                           size_t result_count,
                           const hwb_hardware_info* hw,
                           const char* suite_version,
                           const char* run_id);
```

### Benefits

- **Locality:** Hardware detection lives in one place (CLI orchestration), not scattered between CLI and JSON.
- **Leverage:** The JSON module becomes deep — small interface (results + hw_info + metadata → file), all formatting complexity hidden.
- **Testability:** Tests can pass a known `hwb_hardware_info` struct and verify JSON output deterministically, without the hidden `hwb_detect_hardware` call. Currently impossible to test JSON output with controlled hardware data.

---

## 2. Benchmark Run Loop Is Copy-Pasted, Not a Seam

**Files:** All `src/benches/*.c` files (cpu_scalar_int_add.c, memory_stream_copy.c, storage_seq_read.c, gpu_compute_opencl.c, etc.)

### Problem

Every benchmark file duplicates the same run loop:

1. `memset(out, 0, sizeof(*out))`
2. Populate metadata (id, category, variant, unit, threads, class_kind, synthetic)
3. Warmup loop with `hwb_now_seconds()` timing
4. Sample loop: time each iteration, record value
5. `hwb_compute_stats(out->samples, out->sample_count, &out->summary)`

The only thing that differs is the inner work. Apply the **deletion test**: if you delete this pattern, it reappears in every single benchmark file. The pattern has no **locality** — a bug in warmup timing or sample collection must be fixed in N places.

### Solution

Extract a single `hwb_run_samples` function in `src/core/bench.c` that accepts a work callback:

```c
typedef int (*hwb_work_fn)(const hwb_context* ctx, int sample_index, double* out_value);

int hwb_run_samples(const hwb_context* ctx,
                    hwb_work_fn work,
                    hwb_benchmark_result* out);
```

The runner handles warmup, sample loop, timing, and stats. Benchmarks shrink to: "here's my work function, here's my metadata."

### Benefits

- **Locality:** Warmup logic, sample timing, and stats computation live in one place. Fix once, fix everywhere.
- **Leverage:** Adding a new benchmark becomes: define the work function + declare the desc. The run loop complexity vanishes from the Interface.
- **Testability:** The sample runner can be tested independently with a mock work function. Individual benchmarks can be tested for correctness of their work without timing concerns.

---

## 3. CLI `main.c` Is Shallow — Argument Parsing, Suite Selection, and Orchestration Are One Module

**Files:** `apps/hwbench-cli/main.c`

### Problem

`main` does everything:

- Parse args (lines 66–139)
- Resolve suite tokens with inline string manipulation (lines 74–114, ~40 lines)
- Select benchmarks by category (lines 216–234)
- Execute, print, write JSON

The Interface (CLI flags) is nearly as complex as the implementation. Apply the **deletion test**: delete `main` and all that logic reappears as N callers trying to orchestrate benchmarks.

### Solution

Extract three modules:

1. **Argument parser**: takes `argc/argv`, returns a structured config (suite flags, bench_id, samples, threads, etc.)
2. **Suite resolver**: takes config + registry, returns the list of `hwb_benchmark_desc*` to run
3. **Orchestrator**: takes config + resolved list, runs them, collects results

```c
typedef struct {
    const char* bench_id;
    int list_only;
    int run_quick;
    int run_cpu_suite;
    int run_memory_suite;
    int run_storage_suite;
    int run_gpu_suite;
    int run_stress;
    int stress_seconds;
    const char* out_path;
    hwb_context ctx;
} hwb_cli_config;

int hwb_parse_args(int argc, char** argv, hwb_cli_config* out);
size_t hwb_resolve_suite(const hwb_cli_config* config,
                         const hwb_registry* registry,
                         const hwb_benchmark_desc** out,
                         size_t out_cap);
```

### Benefits

- **Locality:** Suite selection logic (which categories map to which suites) lives in one place, not inline in `main`.
- **Leverage:** The orchestrator's Interface is "run these benchmarks with this config, return results." All the flag-parsing noise is gone.
- **Testability:** Each module can be tested in isolation. Suite resolver can be tested with a fake registry. Argument parser can be tested with synthetic `argv`. Currently, testing any of this requires running the full CLI.

---

## 4. Registry Alias Logic Is Hardcoded, Not Data-Driven

**Files:** `src/core/bench.c` (lines 15–21)

### Problem

`hwb_registry_find` has hardcoded `strcmp` branches:

```c
if (strcmp(id, "cpu.scalar.add") == 0) {
    canonical_id = "cpu.scalar.int_add";
} else if (strcmp(id, "storage.file.seq_write") == 0) {
    canonical_id = "storage.seq_write";
}
```

Every new alias requires editing core library code. This is **shallow** — the Interface (find by id) doesn't communicate that alias resolution is part of the contract. The tests are the only callers of these aliases.

### Solution

Option A: Add an `aliases` field to `hwb_benchmark_desc` (array of `const char*`), or a separate alias table in `register.c`. The find function iterates aliases generically.

Option B: Remove aliases entirely — they exist for backward compatibility but the tests are the only caller.

### Benefits

- **Locality:** Alias definitions live with the benchmark they alias (in `register.c` or the desc itself), not in the core find function.
- **Leverage:** `hwb_registry_find` becomes a pure lookup — no special cases.
- **Testability:** Aliases can be tested as data, not as branching logic in the finder.

---

## 5. GPU OpenCL Setup Is Duplicated Between Benchmarks and Stress

**Files:** `src/benches/gpu_compute_opencl.c`

### Problem

`hwb_run_gpu_stress` (line 376) duplicates the entire OpenCL init → buffer create → kernel set → enqueue → cleanup flow from `run_float_kernel` (line 139). The `gpu_int_mad_run` (line 276) also duplicates it instead of using `run_float_kernel`. These are not **adapters** to a common seam — they are copy-paste with minor parameter differences.

Apply the **deletion test**: delete `hwb_run_gpu_stress` and the same OpenCL boilerplate reappears.

### Solution

Extract a `hwb_opencl_run_kernel` function that takes:

- env (context, queue, device, program)
- kernel name
- element count
- duration-or-samples mode
- a per-iteration flops callback or constant

Both benchmarks and stress call this single function.

### Benefits

- **Locality:** OpenCL lifecycle (init → buffers → enqueue → cleanup) lives in one place.
- **Leverage:** Adding a new GPU kernel becomes: define the kernel string + call the runner.
- **Testability:** The OpenCL runner can be tested with a mock kernel. Stress vs benchmark mode is just a parameter difference.

---

## 6. `hwb_compute_stats` Allocates Internally, Violating Core Allocation Contract

**Files:** `src/core/stats.c`, `include/hwbench/stats.h`

### Problem

`hwb_compute_stats` calls `malloc`/`free` for the sorted array (`stats.c:31`, `stats.c:63`). CLAUDE.md states:

> "The core library deliberately avoids dynamic allocation — the CLI owns it."

This module violates that principle. For small sample counts (always ≤ 64 per `HWB_MAX_SAMPLES`), the allocation is unnecessary overhead and a potential failure point (`-2` return on OOM).

### Solution

Accept a caller-provided scratch buffer, or use a stack-allocated array for the known-max size:

```c
int hwb_compute_stats(const double* values,
                      size_t count,
                      double* scratch,
                      size_t scratch_cap,
                      hwb_stats* out);
```

### Benefits

- **Locality:** Memory management stays with the caller (CLI), consistent with the project's allocation contract.
- **Leverage:** The function can never fail from OOM for reasonable sample counts. The `-2` error code disappears.
- **Testability:** No need to test the malloc failure path. Tests become simpler and more deterministic.

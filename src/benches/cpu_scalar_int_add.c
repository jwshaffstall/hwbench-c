#include "hwbench/bench.h"
#include "hwbench/stats.h"
#include "hwbench/timer.h"

#include <stdint.h>
#include <string.h>

static bool cpu_scalar_supported(const hwb_context* ctx) {
  (void)ctx;
  return true;
}

static int cpu_scalar_run(const hwb_context* ctx, hwb_benchmark_result* out) {
  memset(out, 0, sizeof(*out));
  out->id = "cpu.scalar.int_add";
  out->category = "cpu";
  out->variant = "scalar";
  out->unit = "Mops/s";
  out->threads = ctx->threads;
  out->class_kind = HWB_BENCH_CLASS_MICRO;
  out->synthetic = true;

  volatile uint64_t acc = 0;
  const uint64_t iters = 25000000ULL;

  double warmup_start = hwb_now_seconds();
  while ((hwb_now_seconds() - warmup_start) * 1000.0 < (double)ctx->warmup_ms) {
    for (uint64_t i = 0; i < iters / 10; ++i) {
      acc += i;
    }
  }
  out->warmup_ms = (hwb_now_seconds() - warmup_start) * 1000.0;

  double measured_start = hwb_now_seconds();
  for (int s = 0; s < ctx->samples && s < HWB_MAX_SAMPLES; ++s) {
    double t0 = hwb_now_seconds();
    for (uint64_t i = 0; i < iters; ++i) {
      acc += i;
    }
    double t1 = hwb_now_seconds();
    double ops_sec = (double)iters / (t1 - t0);
    out->samples[out->sample_count++] = ops_sec / 1e6;
  }
  out->measured_ms = (hwb_now_seconds() - measured_start) * 1000.0;

  (void)acc;
  return hwb_compute_stats(out->samples, out->sample_count, &out->summary);
}

const hwb_benchmark_desc hwb_bench_cpu_scalar_int_add = {
  .id = "cpu.scalar.int_add",
  .category = "cpu",
  .name = "CPU scalar integer add throughput",
  .unit = "Mops/s",
  .variant = "scalar",
  .class_kind = HWB_BENCH_CLASS_MICRO,
  .synthetic = true,
  .is_supported = cpu_scalar_supported,
  .run = cpu_scalar_run,
};

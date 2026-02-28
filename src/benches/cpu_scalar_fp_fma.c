#include "hwbench/bench.h"
#include "hwbench/stats.h"
#include "hwbench/timer.h"

#include <math.h>
#include <string.h>

static bool cpu_scalar_fp_supported(const hwb_context* ctx) {
  (void)ctx;
#if defined(__FMA__) || defined(__ARM_FEATURE_FMA) || defined(__aarch64__) || defined(_M_ARM64)
  return true;
#else
  return false;
#endif
}

static int cpu_scalar_fp_run(const hwb_context* ctx, hwb_benchmark_result* out) {
  memset(out, 0, sizeof(*out));
  out->id = "cpu.scalar.fp_fma";
  out->category = "cpu";
  out->variant = "scalar";
  out->unit = "Mops/s";
  out->threads = ctx->threads;
  out->class_kind = HWB_BENCH_CLASS_MICRO;
  out->synthetic = true;

  volatile double a = 1.1;
  volatile double b = 1.0000001;
  volatile double c = 0.9999999;
  const unsigned long long iters = 20000000ULL;

  double warmup_start = hwb_now_seconds();
  while ((hwb_now_seconds() - warmup_start) * 1000.0 < (double)ctx->warmup_ms) {
    for (unsigned long long i = 0; i < iters / 8; ++i) {
#if defined(__clang__) || defined(__GNUC__)
      a = __builtin_fma(a, b, c);
      b = __builtin_fma(b, c, a);
      c = __builtin_fma(c, a, b);
#else
      a = fma(a, b, c);
      b = fma(b, c, a);
      c = fma(c, a, b);
#endif
    }
  }
  out->warmup_ms = (hwb_now_seconds() - warmup_start) * 1000.0;

  double measured_start = hwb_now_seconds();
  for (int s = 0; s < ctx->samples && s < HWB_MAX_SAMPLES; ++s) {
    double t0 = hwb_now_seconds();
    for (unsigned long long i = 0; i < iters; ++i) {
#if defined(__clang__) || defined(__GNUC__)
      a = __builtin_fma(a, b, c);
      b = __builtin_fma(b, c, a);
      c = __builtin_fma(c, a, b);
#else
      a = fma(a, b, c);
      b = fma(b, c, a);
      c = fma(c, a, b);
#endif
    }
    double t1 = hwb_now_seconds();

    const double ops = (double)iters * 3.0;
    out->samples[out->sample_count++] = (ops / (t1 - t0)) / 1e6;
  }
  out->measured_ms = (hwb_now_seconds() - measured_start) * 1000.0;

  (void)a;
  (void)b;
  (void)c;
  return hwb_compute_stats(out->samples, out->sample_count, &out->summary);
}

const hwb_benchmark_desc hwb_bench_cpu_scalar_fp_fma = {
  .id = "cpu.scalar.fp_fma",
  .category = "cpu",
  .name = "CPU scalar floating-point fused-multiply-add throughput",
  .unit = "Mops/s",
  .variant = "scalar",
  .class_kind = HWB_BENCH_CLASS_MICRO,
  .synthetic = true,
  .is_supported = cpu_scalar_fp_supported,
  .run = cpu_scalar_fp_run,
};

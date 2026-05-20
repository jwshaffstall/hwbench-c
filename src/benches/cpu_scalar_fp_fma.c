#include "hwbench/bench.h"
#include "hwbench/timer.h"

#include <math.h>
#include <string.h>

typedef struct {
  volatile double a;
  volatile double b;
  volatile double c;
  unsigned long long iters;
} cpu_scalar_fp_state;

static bool cpu_scalar_fp_supported(const hwb_context* ctx) {
  (void)ctx;
#if defined(__FMA__) || defined(__ARM_FEATURE_FMA) || defined(__aarch64__) || defined(_M_ARM64)
  return true;
#else
  return false;
#endif
}

static int cpu_scalar_fp_warmup(void* user_data) {
  cpu_scalar_fp_state* st = (cpu_scalar_fp_state*)user_data;
  for (unsigned long long i = 0; i < st->iters / 8; ++i) {
#if defined(__clang__) || defined(__GNUC__)
    st->a = __builtin_fma(st->a, st->b, st->c);
    st->b = __builtin_fma(st->b, st->c, st->a);
    st->c = __builtin_fma(st->c, st->a, st->b);
#else
    st->a = fma(st->a, st->b, st->c);
    st->b = fma(st->b, st->c, st->a);
    st->c = fma(st->c, st->a, st->b);
#endif
  }
  return 0;
}

static int cpu_scalar_fp_sample(void* user_data, double* out_value) {
  cpu_scalar_fp_state* st = (cpu_scalar_fp_state*)user_data;
  double t0 = hwb_now_seconds();
  for (unsigned long long i = 0; i < st->iters; ++i) {
#if defined(__clang__) || defined(__GNUC__)
    st->a = __builtin_fma(st->a, st->b, st->c);
    st->b = __builtin_fma(st->b, st->c, st->a);
    st->c = __builtin_fma(st->c, st->a, st->b);
#else
    st->a = fma(st->a, st->b, st->c);
    st->b = fma(st->b, st->c, st->a);
    st->c = fma(st->c, st->a, st->b);
#endif
  }
  double t1 = hwb_now_seconds();
  *out_value = ((double)st->iters * 3.0 / (t1 - t0)) / 1e6;
  return 0;
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

  cpu_scalar_fp_state st = {.a = 1.1, .b = 1.0000001, .c = 0.9999999, .iters = 20000000ULL};
  return hwb_run_samples(ctx, cpu_scalar_fp_warmup, cpu_scalar_fp_sample, &st, out);
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

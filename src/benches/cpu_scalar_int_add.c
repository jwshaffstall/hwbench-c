#include "hwbench/bench.h"
#include "hwbench/timer.h"

#include <stdint.h>
#include <string.h>

typedef struct {
  volatile uint64_t acc;
  uint64_t iters;
} cpu_scalar_state;

static bool cpu_scalar_supported(const hwb_context* ctx) {
  (void)ctx;
  return true;
}

static int cpu_scalar_warmup(void* user_data) {
  cpu_scalar_state* st = (cpu_scalar_state*)user_data;
  for (uint64_t i = 0; i < st->iters / 10; ++i) {
    st->acc += i;
  }
  return 0;
}

static int cpu_scalar_sample(void* user_data, double* out_value) {
  cpu_scalar_state* st = (cpu_scalar_state*)user_data;
  double t0 = hwb_now_seconds();
  for (uint64_t i = 0; i < st->iters; ++i) {
    st->acc += i;
  }
  double t1 = hwb_now_seconds();
  *out_value = (double)st->iters / (t1 - t0) / 1e6;
  return 0;
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

  cpu_scalar_state st = {.acc = 0, .iters = 25000000ULL};
  return hwb_run_samples(ctx, cpu_scalar_warmup, cpu_scalar_sample, &st, out);
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

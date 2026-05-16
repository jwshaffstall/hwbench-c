#include "hwbench/bench.h"
#include "hwbench/timer.h"

#include <stdlib.h>
#include <string.h>

typedef struct {
  double* a;
  double* b;
  double* c;
  size_t n;
  double scalar;
  double bytes;
} stream_triad_state;

static bool memory_stream_triad_supported(const hwb_context* ctx) {
  (void)ctx;
  return true;
}

static int stream_triad_warmup(void* user_data) {
  stream_triad_state* st = (stream_triad_state*)user_data;
  for (size_t i = 0; i < st->n; ++i) {
    st->a[i] = st->b[i] + st->scalar * st->c[i];
  }
  return 0;
}

static int stream_triad_sample(void* user_data, double* out_value) {
  stream_triad_state* st = (stream_triad_state*)user_data;
  double t0 = hwb_now_seconds();
  for (size_t i = 0; i < st->n; ++i) {
    st->a[i] = st->b[i] + st->scalar * st->c[i];
  }
  double t1 = hwb_now_seconds();
  *out_value = (st->bytes / (t1 - t0)) / 1e9;
  return 0;
}

static int memory_stream_triad_run(const hwb_context* ctx, hwb_benchmark_result* out) {
  memset(out, 0, sizeof(*out));
  out->id = "memory.stream.triad";
  out->category = "memory";
  out->variant = "scalar";
  out->unit = "GB/s";
  out->threads = ctx->threads;
  out->class_kind = HWB_BENCH_CLASS_MICRO;
  out->synthetic = true;

  const size_t n = 8 * 1024 * 1024;
  double* a = (double*)malloc(sizeof(double) * n);
  double* b = (double*)malloc(sizeof(double) * n);
  double* c = (double*)malloc(sizeof(double) * n);
  if (!a || !b || !c) {
    free(a); free(b); free(c);
    return -1;
  }

  for (size_t i = 0; i < n; ++i) {
    a[i] = 1.0;
    b[i] = 2.0;
    c[i] = 0.5;
  }

  stream_triad_state st = {.a = a, .b = b, .c = c, .n = n, .scalar = 3.0, .bytes = (double)(3 * sizeof(double) * n)};
  int rc = hwb_run_samples(ctx, stream_triad_warmup, stream_triad_sample, &st, out);

  free(a); free(b); free(c);
  return rc;
}

const hwb_benchmark_desc hwb_bench_memory_stream_triad = {
  .id = "memory.stream.triad",
  .category = "memory",
  .name = "STREAM-style triad bandwidth",
  .unit = "GB/s",
  .variant = "scalar",
  .class_kind = HWB_BENCH_CLASS_MICRO,
  .synthetic = true,
  .is_supported = memory_stream_triad_supported,
  .run = memory_stream_triad_run,
};

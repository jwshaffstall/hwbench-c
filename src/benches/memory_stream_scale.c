#include "hwbench/bench.h"
#include "hwbench/timer.h"

#include <stdlib.h>
#include <string.h>

typedef struct {
  double* b;
  double* c;
  size_t n;
  double scalar;
  double bytes;
} stream_scale_state;

static bool memory_stream_scale_supported(const hwb_context* ctx) {
  (void)ctx;
  return true;
}

static int stream_scale_warmup(void* user_data) {
  stream_scale_state* st = (stream_scale_state*)user_data;
  for (size_t i = 0; i < st->n; ++i) {
    st->c[i] = st->scalar * st->b[i];
  }
  return 0;
}

static int stream_scale_sample(void* user_data, double* out_value) {
  stream_scale_state* st = (stream_scale_state*)user_data;
  double t0 = hwb_now_seconds();
  for (size_t i = 0; i < st->n; ++i) {
    st->c[i] = st->scalar * st->b[i];
  }
  double t1 = hwb_now_seconds();
  *out_value = (st->bytes / (t1 - t0)) / 1e9;
  return 0;
}

static int memory_stream_scale_run(const hwb_context* ctx, hwb_benchmark_result* out) {
  memset(out, 0, sizeof(*out));
  out->id = "memory.stream.scale";
  out->category = "memory";
  out->variant = "scalar";
  out->unit = "GB/s";
  out->threads = ctx->threads;
  out->class_kind = HWB_BENCH_CLASS_MICRO;
  out->synthetic = true;

  const size_t n = 8 * 1024 * 1024;
  double* b = (double*)malloc(sizeof(double) * n);
  double* c = (double*)malloc(sizeof(double) * n);
  if (!b || !c) {
    free(b);
    free(c);
    return -1;
  }

  for (size_t i = 0; i < n; ++i) {
    b[i] = (double)i * 0.5;
    c[i] = 1.0;
  }

  stream_scale_state st = {.b = b, .c = c, .n = n, .scalar = 2.5, .bytes = (double)(2 * sizeof(double) * n)};
  int rc = hwb_run_samples(ctx, stream_scale_warmup, stream_scale_sample, &st, out);

  volatile double sink = 0.0;
  for (size_t i = 0; i < n; ++i) {
    sink += c[i];
  }
  (void)sink;

  free(b);
  free(c);
  return rc;
}

const hwb_benchmark_desc hwb_bench_memory_stream_scale = {
  .id = "memory.stream.scale",
  .category = "memory",
  .name = "STREAM-style scale bandwidth",
  .unit = "GB/s",
  .variant = "scalar",
  .class_kind = HWB_BENCH_CLASS_MICRO,
  .synthetic = true,
  .is_supported = memory_stream_scale_supported,
  .run = memory_stream_scale_run,
};

#include "hwbench/bench.h"
#include "hwbench/timer.h"

#include <stdlib.h>
#include <string.h>

typedef struct {
  double* a;
  double* b;
  size_t n;
  double bytes;
} stream_copy_state;

static bool memory_stream_copy_supported(const hwb_context* ctx) {
  (void)ctx;
  return true;
}

static int stream_copy_warmup(void* user_data) {
  stream_copy_state* st = (stream_copy_state*)user_data;
  for (size_t i = 0; i < st->n; ++i) {
    st->b[i] = st->a[i];
  }
  return 0;
}

static int stream_copy_sample(void* user_data, double* out_value) {
  stream_copy_state* st = (stream_copy_state*)user_data;
  double t0 = hwb_now_seconds();
  for (size_t i = 0; i < st->n; ++i) {
    st->b[i] = st->a[i];
  }
  double t1 = hwb_now_seconds();
  *out_value = (st->bytes / (t1 - t0)) / 1e9;
  return 0;
}

static int memory_stream_copy_run(const hwb_context* ctx, hwb_benchmark_result* out) {
  memset(out, 0, sizeof(*out));
  out->id = "memory.stream.copy";
  out->category = "memory";
  out->variant = "scalar";
  out->unit = "GB/s";
  out->threads = ctx->threads;
  out->class_kind = HWB_BENCH_CLASS_MICRO;
  out->synthetic = true;

  const size_t n = 8 * 1024 * 1024;
  double* a = (double*)malloc(sizeof(double) * n);
  double* b = (double*)malloc(sizeof(double) * n);
  if (!a || !b) {
    free(a);
    free(b);
    return -1;
  }

  for (size_t i = 0; i < n; ++i) {
    a[i] = (double)i;
    b[i] = 0.0;
  }

  stream_copy_state st = {.a = a, .b = b, .n = n, .bytes = (double)(2 * sizeof(double) * n)};
  int rc = hwb_run_samples(ctx, stream_copy_warmup, stream_copy_sample, &st, out);

  volatile double sink = 0.0;
  for (size_t i = 0; i < n; ++i) {
    sink += b[i];
  }
  (void)sink;

  free(a);
  free(b);
  return rc;
}

const hwb_benchmark_desc hwb_bench_memory_stream_copy = {
  .id = "memory.stream.copy",
  .category = "memory",
  .name = "STREAM-style copy bandwidth",
  .unit = "GB/s",
  .variant = "scalar",
  .class_kind = HWB_BENCH_CLASS_MICRO,
  .synthetic = true,
  .is_supported = memory_stream_copy_supported,
  .run = memory_stream_copy_run,
};

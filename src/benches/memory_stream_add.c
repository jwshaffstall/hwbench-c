#include "hwbench/bench.h"
#include "hwbench/timer.h"

#include <stdlib.h>
#include <string.h>

typedef struct {
  double* a;
  double* b;
  double* c;
  size_t n;
  double bytes;
} stream_add_state;

static bool memory_stream_add_supported(const hwb_context* ctx) {
  (void)ctx;
  return true;
}

static int stream_add_warmup(void* user_data) {
  stream_add_state* st = (stream_add_state*)user_data;
  for (size_t i = 0; i < st->n; ++i) {
    st->a[i] = st->b[i] + st->c[i];
  }
  return 0;
}

static int stream_add_sample(void* user_data, double* out_value) {
  stream_add_state* st = (stream_add_state*)user_data;
  double t0 = hwb_now_seconds();
  for (size_t i = 0; i < st->n; ++i) {
    st->a[i] = st->b[i] + st->c[i];
  }
  double t1 = hwb_now_seconds();
  *out_value = (st->bytes / (t1 - t0)) / 1e9;
  return 0;
}

static int memory_stream_add_run(const hwb_context* ctx, hwb_benchmark_result* out) {
  memset(out, 0, sizeof(*out));
  out->id = "memory.stream.add";
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
    free(a);
    free(b);
    free(c);
    return -1;
  }

  for (size_t i = 0; i < n; ++i) {
    a[i] = 1.0;
    b[i] = (double)i * 0.25;
    c[i] = (double)i * 0.75;
  }

  stream_add_state st = {.a = a, .b = b, .c = c, .n = n, .bytes = (double)(3 * sizeof(double) * n)};
  int rc = hwb_run_samples(ctx, stream_add_warmup, stream_add_sample, &st, out);

  volatile double sink = 0.0;
  for (size_t i = 0; i < n; ++i) {
    sink += a[i];
  }
  (void)sink;

  free(a);
  free(b);
  free(c);
  return rc;
}

const hwb_benchmark_desc hwb_bench_memory_stream_add = {
  .id = "memory.stream.add",
  .category = "memory",
  .name = "STREAM-style add bandwidth",
  .unit = "GB/s",
  .variant = "scalar",
  .class_kind = HWB_BENCH_CLASS_MICRO,
  .synthetic = true,
  .is_supported = memory_stream_add_supported,
  .run = memory_stream_add_run,
};

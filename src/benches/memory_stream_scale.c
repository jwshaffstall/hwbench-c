#include "hwbench/bench.h"
#include "hwbench/stats.h"
#include "hwbench/timer.h"

#include <stdlib.h>
#include <string.h>

static bool memory_stream_scale_supported(const hwb_context* ctx) {
  (void)ctx;
  return true;
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

  const double scalar = 2.5;
  double warmup_start = hwb_now_seconds();
  while ((hwb_now_seconds() - warmup_start) * 1000.0 < (double)ctx->warmup_ms) {
    for (size_t i = 0; i < n; ++i) {
      c[i] = scalar * b[i];
    }
  }
  out->warmup_ms = (hwb_now_seconds() - warmup_start) * 1000.0;

  double measured_start = hwb_now_seconds();
  for (int s = 0; s < ctx->samples && s < HWB_MAX_SAMPLES; ++s) {
    double t0 = hwb_now_seconds();
    for (size_t i = 0; i < n; ++i) {
      c[i] = scalar * b[i];
    }
    double t1 = hwb_now_seconds();

    const double bytes = (double)(2 * sizeof(double) * n);
    out->samples[out->sample_count++] = (bytes / (t1 - t0)) / 1e9;
  }
  out->measured_ms = (hwb_now_seconds() - measured_start) * 1000.0;

  volatile double sink = 0.0;
  for (size_t i = 0; i < n; ++i) {
    sink += c[i];
  }
  (void)sink;

  free(b);
  free(c);
  return hwb_compute_stats(out->samples, out->sample_count, &out->summary);
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

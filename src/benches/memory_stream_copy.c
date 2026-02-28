#include "hwbench/bench.h"
#include "hwbench/stats.h"
#include "hwbench/timer.h"

#include <stdlib.h>
#include <string.h>

static bool memory_stream_copy_supported(const hwb_context* ctx) {
  (void)ctx;
  return true;
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

  double warmup_start = hwb_now_seconds();
  while ((hwb_now_seconds() - warmup_start) * 1000.0 < (double)ctx->warmup_ms) {
    for (size_t i = 0; i < n; ++i) {
      b[i] = a[i];
    }
  }
  out->warmup_ms = (hwb_now_seconds() - warmup_start) * 1000.0;

  double measured_start = hwb_now_seconds();
  for (int s = 0; s < ctx->samples && s < HWB_MAX_SAMPLES; ++s) {
    double t0 = hwb_now_seconds();
    for (size_t i = 0; i < n; ++i) {
      b[i] = a[i];
    }
    double t1 = hwb_now_seconds();

    const double bytes = (double)(2 * sizeof(double) * n);
    out->samples[out->sample_count++] = (bytes / (t1 - t0)) / 1e9;
  }
  out->measured_ms = (hwb_now_seconds() - measured_start) * 1000.0;

  free(a);
  free(b);
  return hwb_compute_stats(out->samples, out->sample_count, &out->summary);
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

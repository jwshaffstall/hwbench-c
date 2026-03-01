#include "hwbench/bench.h"
#include "hwbench/stats.h"
#include "hwbench/timer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool storage_seq_write_supported(const hwb_context* ctx) {
  (void)ctx;
  return true;
}

static int run_one_pass(FILE* f, unsigned char* buf, size_t chunk_size, size_t chunk_count, double* mib_per_s) {
  if (!f || !buf || !mib_per_s) return -1;
  rewind(f);

  double t0 = hwb_now_seconds();
  for (size_t i = 0; i < chunk_count; ++i) {
    if (fwrite(buf, 1, chunk_size, f) != chunk_size) {
      return -1;
    }
  }
  if (fflush(f) != 0) {
    return -1;
  }
  double t1 = hwb_now_seconds();
  double mib = (double)(chunk_size * chunk_count) / (1024.0 * 1024.0);
  *mib_per_s = mib / (t1 - t0);
  return 0;
}

static int storage_seq_write_run(const hwb_context* ctx, hwb_benchmark_result* out) {
  memset(out, 0, sizeof(*out));
  out->id = "storage.seq_write";
  out->category = "storage";
  out->variant = "tempfile";
  out->unit = "MiB/s";
  out->threads = ctx->threads;
  out->class_kind = HWB_BENCH_CLASS_COMPONENT;
  out->synthetic = true;

  const size_t chunk_size = 1024 * 1024;
  const size_t chunk_count = 64;
  unsigned char* buf = (unsigned char*)malloc(chunk_size);
  if (!buf) return -1;
  for (size_t i = 0; i < chunk_size; ++i) {
    buf[i] = (unsigned char)(i & 0xFFU);
  }

  FILE* f = tmpfile();
  if (!f) {
    free(buf);
    return -1;
  }

  double warmup_start = hwb_now_seconds();
  while ((hwb_now_seconds() - warmup_start) * 1000.0 < (double)ctx->warmup_ms) {
    double ignored = 0.0;
    if (run_one_pass(f, buf, chunk_size, chunk_count, &ignored) != 0) {
      fclose(f);
      free(buf);
      return -1;
    }
  }
  out->warmup_ms = (hwb_now_seconds() - warmup_start) * 1000.0;

  double measured_start = hwb_now_seconds();
  for (int s = 0; s < ctx->samples && s < HWB_MAX_SAMPLES; ++s) {
    double mib_per_s = 0.0;
    if (run_one_pass(f, buf, chunk_size, chunk_count, &mib_per_s) != 0) {
      fclose(f);
      free(buf);
      return -1;
    }
    out->samples[out->sample_count++] = mib_per_s;
  }
  out->measured_ms = (hwb_now_seconds() - measured_start) * 1000.0;

  fclose(f);
  free(buf);
  return hwb_compute_stats(out->samples, out->sample_count, &out->summary);
}

const hwb_benchmark_desc hwb_bench_storage_seq_write = {
  .id = "storage.seq_write",
  .category = "storage",
  .name = "Sequential temp-file write throughput",
  .unit = "MiB/s",
  .variant = "tempfile",
  .class_kind = HWB_BENCH_CLASS_COMPONENT,
  .synthetic = true,
  .is_supported = storage_seq_write_supported,
  .run = storage_seq_write_run,
};

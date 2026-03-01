#include "hwbench/bench.h"
#include "hwbench/stats.h"
#include "hwbench/timer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool storage_rand4k_read_supported(const hwb_context* ctx) {
  (void)ctx;
  return true;
}

static int prepare_file(FILE* f, const unsigned char* page, size_t pages) {
  rewind(f);
  for (size_t i = 0; i < pages; ++i) {
    if (fwrite(page, 1, 4096, f) != 4096) return -1;
  }
  if (fflush(f) != 0) return -1;
  return 0;
}

static int run_pass(FILE* f, unsigned char* page, size_t pages, double* iops) {
  double t0 = hwb_now_seconds();
  for (size_t i = 0; i < pages; ++i) {
    size_t idx = (i * 104729U) % pages;
    if (fseek(f, (long)(idx * 4096), SEEK_SET) != 0) return -1;
    if (fread(page, 1, 4096, f) != 4096) return -1;
  }
  double t1 = hwb_now_seconds();
  *iops = (double)pages / (t1 - t0);
  return 0;
}

static int storage_rand4k_read_run(const hwb_context* ctx, hwb_benchmark_result* out) {
  memset(out, 0, sizeof(*out));
  out->id = "storage.rand4k_read";
  out->category = "storage";
  out->variant = "tempfile";
  out->unit = "IOPS";
  out->threads = ctx->threads;
  out->class_kind = HWB_BENCH_CLASS_COMPONENT;
  out->synthetic = true;

  const size_t pages = 16384;
  unsigned char* page = (unsigned char*)malloc(4096);
  if (!page) return -1;
  memset(page, 0xA5, 4096);

  FILE* f = tmpfile();
  if (!f) {
    free(page);
    return -1;
  }
  if (prepare_file(f, page, pages) != 0) {
    fclose(f);
    free(page);
    return -1;
  }

  double warmup_start = hwb_now_seconds();
  while ((hwb_now_seconds() - warmup_start) * 1000.0 < (double)ctx->warmup_ms) {
    double ignored = 0.0;
    if (run_pass(f, page, pages, &ignored) != 0) {
      fclose(f);
      free(page);
      return -1;
    }
  }
  out->warmup_ms = (hwb_now_seconds() - warmup_start) * 1000.0;

  double measured_start = hwb_now_seconds();
  for (int s = 0; s < ctx->samples && s < HWB_MAX_SAMPLES; ++s) {
    double iops = 0.0;
    if (run_pass(f, page, pages, &iops) != 0) {
      fclose(f);
      free(page);
      return -1;
    }
    out->samples[out->sample_count++] = iops;
  }
  out->measured_ms = (hwb_now_seconds() - measured_start) * 1000.0;

  fclose(f);
  free(page);
  return hwb_compute_stats(out->samples, out->sample_count, &out->summary);
}

const hwb_benchmark_desc hwb_bench_storage_rand4k_read = {
  .id = "storage.rand4k_read",
  .category = "storage",
  .name = "Random 4K temp-file read",
  .unit = "IOPS",
  .variant = "tempfile",
  .class_kind = HWB_BENCH_CLASS_COMPONENT,
  .synthetic = true,
  .is_supported = storage_rand4k_read_supported,
  .run = storage_rand4k_read_run,
};

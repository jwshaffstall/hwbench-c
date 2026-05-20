#include "hwbench/bench.h"
#include "hwbench/timer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  FILE* f;
  unsigned char* buf;
  size_t chunk_size;
  size_t chunk_count;
  double mib;
} storage_seq_write_state;

static bool storage_seq_write_supported(const hwb_context* ctx) {
  (void)ctx;
  return true;
}

static int storage_seq_write_pass(void* user_data, double* out_value) {
  storage_seq_write_state* st = (storage_seq_write_state*)user_data;
  rewind(st->f);

  double t0 = hwb_now_seconds();
  for (size_t i = 0; i < st->chunk_count; ++i) {
    if (fwrite(st->buf, 1, st->chunk_size, st->f) != st->chunk_size) {
      return -1;
    }
  }
  if (fflush(st->f) != 0) {
    return -1;
  }
  double t1 = hwb_now_seconds();
  *out_value = st->mib / (t1 - t0);
  return 0;
}

static int storage_seq_write_warmup(void* user_data) {
  double ignored;
  return storage_seq_write_pass(user_data, &ignored);
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

  double mib = (double)(chunk_size * chunk_count) / (1024.0 * 1024.0);
  storage_seq_write_state st = {.f = f, .buf = buf, .chunk_size = chunk_size, .chunk_count = chunk_count, .mib = mib};
  int rc = hwb_run_samples(ctx, storage_seq_write_warmup, storage_seq_write_pass, &st, out);

  fclose(f);
  free(buf);
  return rc;
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

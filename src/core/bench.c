#include "hwbench/bench.h"
#include "hwbench/stats.h"
#include "hwbench/timer.h"

#include <string.h>

void hwb_registry_init(hwb_registry* registry, const hwb_benchmark_desc** entries, size_t count) {
  registry->entries = entries;
  registry->count = count;
}

const hwb_benchmark_desc* hwb_registry_find(const hwb_registry* registry, const char* id) {
  if (!registry || !id) {
    return NULL;
  }

  const char* canonical_id = id;
  if (strcmp(id, "cpu.scalar.add") == 0) {
    canonical_id = "cpu.scalar.int_add";
  } else if (strcmp(id, "storage.file.seq_write") == 0) {
    canonical_id = "storage.seq_write";
  }

  for (size_t i = 0; i < registry->count; ++i) {
    if (strcmp(registry->entries[i]->id, canonical_id) == 0) {
      return registry->entries[i];
    }
  }

  return NULL;
}

int hwb_run_benchmark(const hwb_context* ctx, const hwb_benchmark_desc* desc, hwb_benchmark_result* out) {
  if (!desc || !desc->run || !out) {
    return -1;
  }
  if (desc->is_supported && !desc->is_supported(ctx)) {
    return -2;
  }
  return desc->run(ctx, out);
}

int hwb_run_samples(const hwb_context* ctx,
                    hwb_bench_warmup_pass_fn warmup_pass,
                    hwb_bench_sample_pass_fn sample_pass,
                    void* user_data,
                    hwb_benchmark_result* out) {
  if (!ctx || !sample_pass || !out) {
    return -1;
  }

  double warmup_start = hwb_now_seconds();
  while ((hwb_now_seconds() - warmup_start) * 1000.0 < (double)ctx->warmup_ms) {
    if (warmup_pass && warmup_pass(user_data) != 0) {
      return -1;
    }
  }
  out->warmup_ms = (hwb_now_seconds() - warmup_start) * 1000.0;

  double measured_start = hwb_now_seconds();
  for (int s = 0; s < ctx->samples && s < HWB_MAX_SAMPLES; ++s) {
    double value;
    if (sample_pass(user_data, &value) != 0) {
      return -1;
    }
    out->samples[out->sample_count++] = value;
  }
  out->measured_ms = (hwb_now_seconds() - measured_start) * 1000.0;

  return hwb_compute_stats(out->samples, out->sample_count, &out->summary);
}

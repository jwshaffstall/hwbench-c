#include "hwbench/bench.h"
#include "hwbench/stats.h"
#include "hwbench/timer.h"

#include <string.h>

void hwb_registry_init(hwb_registry* registry, const hwb_benchmark_desc** entries, size_t count) {
  registry->entries = entries;
  registry->count = count;
  registry->aliases = NULL;
  registry->alias_count = 0;
}

void hwb_registry_set_aliases(hwb_registry* registry, const hwb_alias_entry* aliases, size_t alias_count) {
  registry->aliases = aliases;
  registry->alias_count = alias_count;
}

const hwb_benchmark_desc* hwb_registry_find(const hwb_registry* registry, const char* id) {
  if (!registry || !id) {
    return NULL;
  }

  for (size_t i = 0; i < registry->count; ++i) {
    if (strcmp(registry->entries[i]->id, id) == 0) {
      return registry->entries[i];
    }
  }

  for (size_t i = 0; i < registry->alias_count; ++i) {
    if (strcmp(registry->aliases[i].alias, id) == 0) {
      const char* canonical = registry->aliases[i].canonical_id;
      for (size_t j = 0; j < registry->count; ++j) {
        if (strcmp(registry->entries[j]->id, canonical) == 0) {
          return registry->entries[j];
        }
      }
      return NULL;
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

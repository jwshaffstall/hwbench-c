#include "hwbench/bench.h"

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

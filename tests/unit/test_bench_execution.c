#include "hwbench/bench.h"
#include "hwbench/benches.h"

#include <stdio.h>

int test_bench_execution(void) {
  hwb_registry reg;
  hwb_register_default_benches(&reg);

  hwb_context ctx = {
    .threads = 1,
    .warmup_ms = 1,
    .min_sample_ms = 1,
    .samples = 1,
    .synthetic = true,
  };

  for (size_t i = 0; i < reg.count; ++i) {
    const hwb_benchmark_desc* desc = reg.entries[i];
    hwb_benchmark_result out;
    int rc = hwb_run_benchmark(&ctx, desc, &out);
    if (rc != 0 && rc != -2) {
      printf("benchmark failed: %s (rc=%d)\n", desc->id, rc);
      return 1;
    }

    if (rc == 0) {
      if (out.sample_count == 0) {
        printf("benchmark produced no samples: %s\n", desc->id);
        return 1;
      }
      if (out.id == NULL || out.category == NULL || out.unit == NULL) {
        printf("benchmark missing metadata: %s\n", desc->id);
        return 1;
      }
    }
  }

  return 0;
}

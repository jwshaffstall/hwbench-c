#include "hwbench/benches.h"

extern const hwb_benchmark_desc hwb_bench_cpu_scalar_int_add;
extern const hwb_benchmark_desc hwb_bench_memory_stream_triad;

void hwb_register_default_benches(hwb_registry* registry) {
  static const hwb_benchmark_desc* entries[] = {
    &hwb_bench_cpu_scalar_int_add,
    &hwb_bench_memory_stream_triad,
  };
  hwb_registry_init(registry, entries, sizeof(entries) / sizeof(entries[0]));
}

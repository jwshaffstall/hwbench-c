#include "hwbench/benches.h"

extern const hwb_benchmark_desc hwb_bench_cpu_scalar_int_add;
extern const hwb_benchmark_desc hwb_bench_cpu_scalar_fp_fma;
extern const hwb_benchmark_desc hwb_bench_memory_stream_copy;
extern const hwb_benchmark_desc hwb_bench_memory_stream_triad;
extern const hwb_benchmark_desc hwb_bench_storage_file_seq_write;

void hwb_register_default_benches(hwb_registry* registry) {
  static const hwb_benchmark_desc* entries[] = {
    &hwb_bench_cpu_scalar_int_add,
    &hwb_bench_cpu_scalar_fp_fma,
    &hwb_bench_memory_stream_copy,
    &hwb_bench_memory_stream_triad,
    &hwb_bench_storage_file_seq_write,
  };
  hwb_registry_init(registry, entries, sizeof(entries) / sizeof(entries[0]));
}

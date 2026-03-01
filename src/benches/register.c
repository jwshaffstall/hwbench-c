#include "hwbench/benches.h"

extern const hwb_benchmark_desc hwb_bench_cpu_scalar_int_add;
extern const hwb_benchmark_desc hwb_bench_cpu_scalar_fp_fma;
extern const hwb_benchmark_desc hwb_bench_cpu_parallel_int_add_2c;
extern const hwb_benchmark_desc hwb_bench_cpu_parallel_int_add_3c;
extern const hwb_benchmark_desc hwb_bench_cpu_parallel_int_add_4c;
extern const hwb_benchmark_desc hwb_bench_cpu_parallel_int_add_5c;
extern const hwb_benchmark_desc hwb_bench_cpu_parallel_int_add_6c;
extern const hwb_benchmark_desc hwb_bench_cpu_parallel_int_add_7c;
extern const hwb_benchmark_desc hwb_bench_cpu_parallel_int_add_8c;
extern const hwb_benchmark_desc hwb_bench_cpu_simd_i8_add;
extern const hwb_benchmark_desc hwb_bench_cpu_simd_i16_add;
extern const hwb_benchmark_desc hwb_bench_cpu_simd_i32_add;
extern const hwb_benchmark_desc hwb_bench_cpu_simd_i64_add;
extern const hwb_benchmark_desc hwb_bench_cpu_simd_f32_add;
extern const hwb_benchmark_desc hwb_bench_cpu_simd_f64_add;
extern const hwb_benchmark_desc hwb_bench_memory_stream_copy;
extern const hwb_benchmark_desc hwb_bench_memory_stream_scale;
extern const hwb_benchmark_desc hwb_bench_memory_stream_add;
extern const hwb_benchmark_desc hwb_bench_memory_stream_triad;
extern const hwb_benchmark_desc hwb_bench_memory_latency_pointer_chase;
extern const hwb_benchmark_desc hwb_bench_storage_seq_read;
extern const hwb_benchmark_desc hwb_bench_storage_seq_write;
extern const hwb_benchmark_desc hwb_bench_storage_rand4k_read;
extern const hwb_benchmark_desc hwb_bench_storage_rand4k_write;

void hwb_register_default_benches(hwb_registry* registry) {
  static const hwb_benchmark_desc* entries[] = {
    &hwb_bench_cpu_scalar_int_add,
    &hwb_bench_cpu_scalar_fp_fma,
    &hwb_bench_cpu_parallel_int_add_2c,
    &hwb_bench_cpu_parallel_int_add_3c,
    &hwb_bench_cpu_parallel_int_add_4c,
    &hwb_bench_cpu_parallel_int_add_5c,
    &hwb_bench_cpu_parallel_int_add_6c,
    &hwb_bench_cpu_parallel_int_add_7c,
    &hwb_bench_cpu_parallel_int_add_8c,
    &hwb_bench_cpu_simd_i8_add,
    &hwb_bench_cpu_simd_i16_add,
    &hwb_bench_cpu_simd_i32_add,
    &hwb_bench_cpu_simd_i64_add,
    &hwb_bench_cpu_simd_f32_add,
    &hwb_bench_cpu_simd_f64_add,
    &hwb_bench_memory_stream_copy,
    &hwb_bench_memory_stream_scale,
    &hwb_bench_memory_stream_add,
    &hwb_bench_memory_stream_triad,
    &hwb_bench_memory_latency_pointer_chase,
    &hwb_bench_storage_seq_read,
    &hwb_bench_storage_seq_write,
    &hwb_bench_storage_rand4k_read,
    &hwb_bench_storage_rand4k_write,
  };
  hwb_registry_init(registry, entries, sizeof(entries) / sizeof(entries[0]));
}

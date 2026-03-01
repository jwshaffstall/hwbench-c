#include "hwbench/bench.h"
#include "hwbench/benches.h"

#include <stdio.h>

int test_registry(void) {
  hwb_registry reg;
  hwb_register_default_benches(&reg);
  if (reg.count < 27) {
    puts("expected at least 27 benchmarks");
    return 1;
  }

  const hwb_benchmark_desc* int_add = hwb_registry_find(&reg, "cpu.scalar.int_add");
  if (!int_add || !int_add->run) {
    puts("missing cpu.scalar.int_add run callback");
    return 1;
  }

  const hwb_benchmark_desc* fp_fma = hwb_registry_find(&reg, "cpu.scalar.fp_fma");
  if (!fp_fma || !fp_fma->run) {
    puts("missing cpu.scalar.fp_fma run callback");
    return 1;
  }

  const hwb_benchmark_desc* parallel_4c = hwb_registry_find(&reg, "cpu.parallel.int_add.4c");
  if (!parallel_4c || !parallel_4c->run) {
    puts("missing cpu.parallel.int_add.4c run callback");
    return 1;
  }

  const hwb_benchmark_desc* simd_i32 = hwb_registry_find(&reg, "cpu.simd.i32_add");
  if (!simd_i32 || !simd_i32->run) {
    puts("missing cpu.simd.i32_add run callback");
    return 1;
  }

  const hwb_benchmark_desc* stream_copy = hwb_registry_find(&reg, "memory.stream.copy");
  if (!stream_copy || !stream_copy->run) {
    puts("missing memory.stream.copy run callback");
    return 1;
  }

  const hwb_benchmark_desc* stream_scale = hwb_registry_find(&reg, "memory.stream.scale");
  if (!stream_scale || !stream_scale->run) {
    puts("missing memory.stream.scale run callback");
    return 1;
  }

  const hwb_benchmark_desc* stream_add = hwb_registry_find(&reg, "memory.stream.add");
  if (!stream_add || !stream_add->run) {
    puts("missing memory.stream.add run callback");
    return 1;
  }

  const hwb_benchmark_desc* stream_triad = hwb_registry_find(&reg, "memory.stream.triad");
  if (!stream_triad || !stream_triad->run) {
    puts("missing memory.stream.triad run callback");
    return 1;
  }


  const hwb_benchmark_desc* pointer_chase = hwb_registry_find(&reg, "memory.latency.pointer_chase");
  if (!pointer_chase || !pointer_chase->run) {
    puts("missing memory.latency.pointer_chase run callback");
    return 1;
  }

  const hwb_benchmark_desc* storage_seq_read = hwb_registry_find(&reg, "storage.seq_read");
  if (!storage_seq_read || !storage_seq_read->run) {
    puts("missing storage.seq_read run callback");
    return 1;
  }

  const hwb_benchmark_desc* storage_rand4k_read = hwb_registry_find(&reg, "storage.rand4k_read");
  if (!storage_rand4k_read || !storage_rand4k_read->run) {
    puts("missing storage.rand4k_read run callback");
    return 1;
  }

  const hwb_benchmark_desc* storage_rand4k_write = hwb_registry_find(&reg, "storage.rand4k_write");
  if (!storage_rand4k_write || !storage_rand4k_write->run) {
    puts("missing storage.rand4k_write run callback");
    return 1;
  }


  const hwb_benchmark_desc* gpu_vec_add = hwb_registry_find(&reg, "gpu.compute.fp32_vec_add");
  if (!gpu_vec_add || !gpu_vec_add->run) {
    puts("missing gpu.compute.fp32_vec_add run callback");
    return 1;
  }

  const hwb_benchmark_desc* gpu_fma = hwb_registry_find(&reg, "gpu.compute.fp32_fma");
  if (!gpu_fma || !gpu_fma->run) {
    puts("missing gpu.compute.fp32_fma run callback");
    return 1;
  }

  const hwb_benchmark_desc* gpu_i32 = hwb_registry_find(&reg, "gpu.compute.i32_mad");
  if (!gpu_i32 || !gpu_i32->run) {
    puts("missing gpu.compute.i32_mad run callback");
    return 1;
  }

  const hwb_benchmark_desc* storage_seq_write = hwb_registry_find(&reg, "storage.seq_write");
  if (!storage_seq_write || !storage_seq_write->run) {
    puts("missing storage.seq_write run callback");
    return 1;
  }

  const hwb_benchmark_desc* int_add_alias = hwb_registry_find(&reg, "cpu.scalar.add");
  if (!int_add_alias || int_add_alias != int_add) {
    puts("cpu.scalar.add alias did not resolve to cpu.scalar.int_add");
    return 1;
  }

  const hwb_benchmark_desc* storage_seq_write_alias = hwb_registry_find(&reg, "storage.file.seq_write");
  if (!storage_seq_write_alias || storage_seq_write_alias != storage_seq_write) {
    puts("storage.file.seq_write alias did not resolve to storage.seq_write");
    return 1;
  }

  return 0;
}

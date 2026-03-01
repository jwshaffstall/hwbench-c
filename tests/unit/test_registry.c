#include "hwbench/bench.h"
#include "hwbench/benches.h"

#include <stdio.h>

int test_registry(void) {
  hwb_registry reg;
  hwb_register_default_benches(&reg);
  if (reg.count < 5) {
    puts("expected at least 5 benchmarks");
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

  const hwb_benchmark_desc* stream_copy = hwb_registry_find(&reg, "memory.stream.copy");
  if (!stream_copy || !stream_copy->run) {
    puts("missing memory.stream.copy run callback");
    return 1;
  }

  const hwb_benchmark_desc* storage_seq_write = hwb_registry_find(&reg, "storage.file.seq_write");
  if (!storage_seq_write || !storage_seq_write->run) {
    puts("missing storage.file.seq_write run callback");
    return 1;
  }

  return 0;
}

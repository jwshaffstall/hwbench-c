#include "hwbench/bench.h"
#include "hwbench/benches.h"

#include <stdio.h>

int test_registry(void) {
  hwb_registry reg;
  hwb_register_default_benches(&reg);
  if (reg.count < 2) {
    puts("expected at least 2 benchmarks");
    return 1;
  }

  const hwb_benchmark_desc* b = hwb_registry_find(&reg, "cpu.scalar.int_add");
  if (!b) {
    puts("missing cpu.scalar.int_add");
    return 1;
  }
  if (!b->run) {
    puts("missing run callback");
    return 1;
  }
  return 0;
}

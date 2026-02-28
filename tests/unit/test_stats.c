#include "hwbench/stats.h"

#include <math.h>
#include <stdio.h>

int test_stats(void) {
  double values[] = {1.0, 2.0, 3.0, 4.0, 5.0};
  hwb_stats stats;
  if (hwb_compute_stats(values, 5, &stats) != 0) {
    puts("hwb_compute_stats failed");
    return 1;
  }
  if (fabs(stats.mean - 3.0) > 1e-9) {
    puts("mean mismatch");
    return 1;
  }
  if (fabs(stats.median - 3.0) > 1e-9) {
    puts("median mismatch");
    return 1;
  }
  if (stats.max < stats.min) {
    puts("max/min mismatch");
    return 1;
  }
  return 0;
}

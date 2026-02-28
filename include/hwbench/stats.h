#ifndef HWBENCH_STATS_H
#define HWBENCH_STATS_H

#include <stddef.h>
#include "hwbench/bench.h"

int hwb_compute_stats(const double* values, size_t count, hwb_stats* out);

#endif

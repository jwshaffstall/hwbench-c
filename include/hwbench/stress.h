#ifndef HWBENCH_STRESS_H
#define HWBENCH_STRESS_H

#include "hwbench/bench.h"

#ifdef __cplusplus
extern "C" {
#endif

int hwb_stress_resolve_seconds(int requested_seconds);
int hwb_run_cpu_stress(int seconds, int max_threads, hwb_benchmark_result* out);
int hwb_run_gpu_stress(int seconds, hwb_benchmark_result* out);

#ifdef __cplusplus
}
#endif

#endif

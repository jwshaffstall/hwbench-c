#ifndef HWBENCH_JSON_H
#define HWBENCH_JSON_H

#include <stddef.h>
#include "hwbench/bench.h"
#include "hwbench/system.h"

int hwb_write_json_results(const char* path,
                           const hwb_benchmark_result* results,
                           size_t result_count,
                           const hwb_hardware_info* hw,
                           const char* suite_version,
                           const char* run_id);

#endif

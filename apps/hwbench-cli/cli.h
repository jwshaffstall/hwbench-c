#ifndef HWBENCH_CLI_H
#define HWBENCH_CLI_H

#include "hwbench/bench.h"
#include "hwbench/system.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  HWB_CLI_MODE_LIST,
  HWB_CLI_MODE_SINGLE,
  HWB_CLI_MODE_QUICK,
  HWB_CLI_MODE_SUITE,
  HWB_CLI_MODE_STRESS,
  HWB_CLI_MODE_NONE
} hwb_cli_mode;

typedef struct {
  hwb_cli_mode mode;
  const char* bench_id;
  int stress_seconds;
  const char* out_path;
  hwb_context ctx;
  int threads_set;
  int run_cpu;
  int run_memory;
  int run_storage;
  int run_gpu;
} hwb_cli_config;

int hwb_cli_parse(int argc, char** argv, hwb_cli_config* out);

size_t hwb_cli_resolve_suite(const hwb_cli_config* config,
                             const hwb_registry* registry,
                             const hwb_benchmark_desc** out,
                             size_t out_cap);

int hwb_cli_run(const hwb_cli_config* config,
                const hwb_registry* registry,
                const hwb_hardware_info* hw);

#ifdef __cplusplus
}
#endif

#endif

#include "cli.h"
#include "hwbench/benches.h"
#include "hwbench/system.h"

#include <stdio.h>

int main(int argc, char** argv) {
  hwb_registry registry;
  hwb_register_default_benches(&registry);

  hwb_cli_config config;
  if (hwb_cli_parse(argc, argv, &config) != 0) {
    return 1;
  }

  if (config.mode == HWB_CLI_MODE_LIST) {
    for (size_t i = 0; i < registry.count; ++i) {
      const hwb_benchmark_desc* b = registry.entries[i];
      printf("%s\t%s\t%s\n", b->id, b->category, b->name);
    }
    return 0;
  }

  hwb_hardware_info hw;
  if (hwb_detect_hardware(&hw) != 0) {
    fprintf(stderr, "Failed to detect hardware\n");
    return 1;
  }

  return hwb_cli_run(&config, &registry, &hw);
}

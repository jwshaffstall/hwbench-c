#include "hwbench/system.h"

#include <stdio.h>
#include <string.h>

int test_system(void) {
  hwb_hardware_info hw;
  if (hwb_detect_hardware(&hw) != 0) {
    puts("hwb_detect_hardware failed");
    return 1;
  }

  if (hw.logical_cores < 1) {
    puts("logical_cores should be >= 1");
    return 1;
  }

  if (hw.physical_cores < 1) {
    puts("physical_cores should be >= 1");
    return 1;
  }

  if (hw.physical_cores > hw.logical_cores) {
    puts("physical_cores should be <= logical_cores");
    return 1;
  }

  if (strlen(hw.cpu_model) == 0) {
    puts("cpu_model should not be empty");
    return 1;
  }

  if (strlen(hw.gpu_name) == 0) {
    puts("gpu_name should not be empty");
    return 1;
  }

  return 0;
}

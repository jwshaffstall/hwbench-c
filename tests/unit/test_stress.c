#include "hwbench/stress.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
static void hwb_set_stress_override(const char* value) {
  (void)_putenv_s("HWB_STRESS_SECONDS_OVERRIDE", value);
}
#else
extern int putenv(char*);
static void hwb_set_stress_override(const char* value) {
  const char* key = "HWB_STRESS_SECONDS_OVERRIDE";
  size_t len = strlen(key) + strlen(value) + 2;
  char* entry = (char*)malloc(len);
  if (!entry) {
    return;
  }
  snprintf(entry, len, "%s=%s", key, value);
  (void)putenv(entry);
}
#endif

int test_stress(void) {
  hwb_set_stress_override("1");

  hwb_benchmark_result cpu_res;
  if (hwb_run_cpu_stress(10, 2, &cpu_res) != 0) {
    puts("cpu stress run failed");
    return 1;
  }
  if (cpu_res.sample_count == 0 || cpu_res.samples[0] <= 0.0) {
    puts("cpu stress did not produce samples");
    return 1;
  }
  if (cpu_res.measured_ms <= 0.0) {
    puts("cpu stress duration invalid");
    return 1;
  }

  hwb_benchmark_result gpu_res;
  int gpu_rc = hwb_run_gpu_stress(10, &gpu_res);
  if (gpu_rc == 0) {
    if (gpu_res.sample_count == 0 || gpu_res.samples[0] <= 0.0) {
      puts("gpu stress produced empty sample");
      return 1;
    }
  } else if (gpu_rc != -2) {
    puts("gpu stress returned failure");
    return 1;
  }

  return 0;
}

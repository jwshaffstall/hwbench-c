#ifndef HWBENCH_SYSTEM_H
#define HWBENCH_SYSTEM_H

#ifdef __cplusplus
extern "C" {
#endif

#define HWB_HWSTR_SMALL 64
#define HWB_HWSTR_MEDIUM 128
#define HWB_HWSTR_LARGE 256

typedef struct hwb_hardware_info {
  char cpu_model[HWB_HWSTR_LARGE];
  int logical_cores;
  int physical_cores;
  unsigned long long memory_total_mb;
  unsigned long long storage_total_gb;
  char storage_name[HWB_HWSTR_MEDIUM];
  char gpu_name[HWB_HWSTR_LARGE];
} hwb_hardware_info;

const char* hwb_os_name(void);
const char* hwb_arch_name(void);
int hwb_detect_hardware(hwb_hardware_info* out);

#ifdef __cplusplus
}
#endif

#endif

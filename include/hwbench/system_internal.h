#ifndef HWB_SYSTEM_INTERNAL_H
#define HWB_SYSTEM_INTERNAL_H

#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__linux__)
/* Internal/test helper: parses /proc/cpuinfo content for Linux CPU metadata. */
int hwb_parse_linux_cpuinfo_stream(FILE* f, char* cpu_model, size_t cpu_model_size, int* logical_cores, int* physical_cores);
#endif

#ifdef __cplusplus
}
#endif

#endif /* HWB_SYSTEM_INTERNAL_H */

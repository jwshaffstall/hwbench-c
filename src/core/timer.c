#include "hwbench/timer.h"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <time.h>
#endif

#if defined(_WIN32)
static INIT_ONCE hwb_qpc_init_once = INIT_ONCE_STATIC_INIT;
static LARGE_INTEGER hwb_qpc_freq;
static BOOL hwb_qpc_available = FALSE;

static BOOL CALLBACK hwb_init_qpc(PINIT_ONCE init_once_param, PVOID param, PVOID* context) {
  (void)init_once_param;
  (void)param;
  (void)context;
  hwb_qpc_available = QueryPerformanceFrequency(&hwb_qpc_freq);
  return TRUE;
}
#endif

double hwb_now_seconds(void) {
#if defined(_WIN32)
  InitOnceExecuteOnce(&hwb_qpc_init_once, hwb_init_qpc, NULL, NULL);

  if (hwb_qpc_available && hwb_qpc_freq.QuadPart > 0) {
    LARGE_INTEGER counter;
    if (QueryPerformanceCounter(&counter)) {
      return (double)counter.QuadPart / (double)hwb_qpc_freq.QuadPart;
    }
  }

  return (double)GetTickCount64() / 1000.0;
#else
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
#endif
}

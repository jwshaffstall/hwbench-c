#include "hwbench/timer.h"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <time.h>
#endif

double hwb_now_seconds(void) {
#if defined(_WIN32)
  static LARGE_INTEGER freq;
  static int initialized = 0;
  LARGE_INTEGER counter;
  if (!initialized) {
    QueryPerformanceFrequency(&freq);
    initialized = 1;
  }
  QueryPerformanceCounter(&counter);
  return (double)counter.QuadPart / (double)freq.QuadPart;
#else
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
#endif
}

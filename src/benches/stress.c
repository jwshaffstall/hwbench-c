#include "hwbench/stress.h"

#include "hwbench/stats.h"
#include "hwbench/system.h"
#include "hwbench/timer.h"

#include <stdatomic.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
typedef HANDLE hwb_thread_t;
#else
#include <pthread.h>
#include <sched.h>
#include <time.h>
typedef pthread_t hwb_thread_t;
extern int nanosleep(const struct timespec*, struct timespec*);
#endif

#define HWB_STRESS_MAX_THREADS 32
#define HWB_STRESS_CHUNK 32768

int hwb_stress_resolve_seconds(int requested_seconds) {
  const char* override_env = getenv("HWB_STRESS_SECONDS_OVERRIDE");
  if (override_env && override_env[0] != '\0') {
    int override_val = atoi(override_env);
    if (override_val > 0 && override_val <= requested_seconds) {
      return override_val;
    }
  }
  return requested_seconds;
}

static int hwb_pick_stress_threads(int max_threads) {
  if (max_threads > 0 && max_threads <= HWB_STRESS_MAX_THREADS) {
    return max_threads;
  }

  hwb_hardware_info hw;
  if (hwb_detect_hardware(&hw) != 0 || hw.logical_cores <= 0) {
    return 1;
  }

  int threads = hw.logical_cores > 1 ? hw.logical_cores - 1 : 1;
  if (threads > HWB_STRESS_MAX_THREADS) {
    threads = HWB_STRESS_MAX_THREADS;
  }
  return threads;
}

static void hwb_stress_yield(void) {
#if defined(_WIN32)
  SwitchToThread();
#else
  sched_yield();
#endif
}

static void hwb_stress_sleep_briefly(void) {
#if defined(_WIN32)
  Sleep(1);
#else
  struct timespec ts;
  ts.tv_sec = 0;
  ts.tv_nsec = 1000000; /* 1ms */
  nanosleep(&ts, NULL);
#endif
}

typedef struct hwb_stress_cpu_task {
  atomic_bool* stop_flag;
  uint64_t iterations;
} hwb_stress_cpu_task;

static void hwb_stress_cpu_worker(hwb_stress_cpu_task* task) {
  volatile uint64_t acc = 1;
  while (!atomic_load(task->stop_flag)) {
    for (int i = 0; i < HWB_STRESS_CHUNK; ++i) {
      acc = acc * 1664525u + 1013904223u;
    }
    task->iterations += HWB_STRESS_CHUNK;
    hwb_stress_yield();
  }
  (void)acc;
}

#if defined(_WIN32)
static DWORD WINAPI hwb_stress_cpu_worker_win(LPVOID arg) {
  hwb_stress_cpu_worker((hwb_stress_cpu_task*)arg);
  return 0;
}
#else
static void* hwb_stress_cpu_worker_posix(void* arg) {
  hwb_stress_cpu_worker((hwb_stress_cpu_task*)arg);
  return NULL;
}
#endif

int hwb_run_cpu_stress(int seconds, int max_threads, hwb_benchmark_result* out) {
  if (!out) {
    return -1;
  }

  int duration = hwb_stress_resolve_seconds(seconds);
  if (duration <= 0) {
    return -1;
  }

  int threads = hwb_pick_stress_threads(max_threads);
  if (threads <= 0) {
    return -1;
  }

  memset(out, 0, sizeof(*out));
  out->id = "stress.cpu";
  out->category = "stress";
  out->variant = "cpu";
  out->unit = "Mops/s";
  out->threads = threads;
  out->class_kind = HWB_BENCH_CLASS_SCENARIO;
  out->synthetic = true;

  hwb_stress_cpu_task* tasks = calloc((size_t)threads, sizeof(hwb_stress_cpu_task));
  hwb_thread_t* workers = calloc((size_t)threads, sizeof(hwb_thread_t));
  if (!tasks || !workers) {
    free(tasks);
    free(workers);
    return -1;
  }

  atomic_bool stop_flag = false;
  for (int i = 0; i < threads; ++i) {
    tasks[i].stop_flag = &stop_flag;
#if defined(_WIN32)
    workers[i] = CreateThread(NULL, 0, hwb_stress_cpu_worker_win, &tasks[i], 0, NULL);
    if (workers[i] == NULL) {
      atomic_store(&stop_flag, true);
      for (int j = 0; j < i; ++j) {
        WaitForSingleObject(workers[j], INFINITE);
        CloseHandle(workers[j]);
      }
      free(tasks);
      free(workers);
      return -1;
    }
#else
    if (pthread_create(&workers[i], NULL, hwb_stress_cpu_worker_posix, &tasks[i]) != 0) {
      atomic_store(&stop_flag, true);
      for (int j = 0; j < i; ++j) {
        (void)pthread_join(workers[j], NULL);
      }
      free(tasks);
      free(workers);
      return -1;
    }
#endif
  }

  double start = hwb_now_seconds();
  double end_time = start + (double)duration;
  while (hwb_now_seconds() < end_time) {
    hwb_stress_sleep_briefly();
  }
  atomic_store(&stop_flag, true);

  for (int i = 0; i < threads; ++i) {
#if defined(_WIN32)
    WaitForSingleObject(workers[i], INFINITE);
    CloseHandle(workers[i]);
#else
    (void)pthread_join(workers[i], NULL);
#endif
  }

  double elapsed = hwb_now_seconds() - start;
  uint64_t total_iterations = 0;
  for (int i = 0; i < threads; ++i) {
    total_iterations += tasks[i].iterations;
  }

  free(tasks);
  free(workers);

  if (elapsed <= 0.0) {
    return -1;
  }

  double mops = ((double)total_iterations / elapsed) / 1e6;
  out->samples[out->sample_count++] = mops;
  out->measured_ms = elapsed * 1000.0;
  out->warmup_ms = 0.0;

  return hwb_compute_stats(out->samples, out->sample_count, &out->summary);
}

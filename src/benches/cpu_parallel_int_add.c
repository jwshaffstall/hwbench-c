#include "hwbench/bench.h"
#include "hwbench/stats.h"
#include "hwbench/system.h"
#include "hwbench/timer.h"

#include <stdint.h>
#include <string.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
typedef HANDLE hwb_thread_t;
#else
#include <pthread.h>
typedef pthread_t hwb_thread_t;
#endif

typedef struct hwb_parallel_task {
  uint64_t iterations;
  volatile uint64_t acc;
} hwb_parallel_task;

static void hwb_parallel_worker(hwb_parallel_task* task) {
  volatile uint64_t local = task->acc;
  for (uint64_t i = 0; i < task->iterations; ++i) {
    local += (i * 3ULL) + 1ULL;
  }
  task->acc = local;
}

#if defined(_WIN32)
static DWORD WINAPI hwb_parallel_worker_win(LPVOID arg) {
  hwb_parallel_worker((hwb_parallel_task*)arg);
  return 0;
}
#else
static void* hwb_parallel_worker_posix(void* arg) {
  hwb_parallel_worker((hwb_parallel_task*)arg);
  return NULL;
}
#endif

static int hwb_parallel_run_once(int workers, uint64_t iterations_per_worker, hwb_parallel_task* tasks,
                                 hwb_thread_t* threads) {
  for (int i = 0; i < workers; ++i) {
    tasks[i].iterations = iterations_per_worker;
    tasks[i].acc = (uint64_t)(i + 1);
#if defined(_WIN32)
    threads[i] = CreateThread(NULL, 0, hwb_parallel_worker_win, &tasks[i], 0, NULL);
    if (threads[i] == NULL) {
      for (int j = 0; j < i; ++j) {
        WaitForSingleObject(threads[j], INFINITE);
        CloseHandle(threads[j]);
      }
      return -1;
    }
#else
    if (pthread_create(&threads[i], NULL, hwb_parallel_worker_posix, &tasks[i]) != 0) {
      for (int j = 0; j < i; ++j) {
        (void)pthread_join(threads[j], NULL);
      }
      return -1;
    }
#endif
  }

  for (int i = 0; i < workers; ++i) {
#if defined(_WIN32)
    WaitForSingleObject(threads[i], INFINITE);
    CloseHandle(threads[i]);
#else
    (void)pthread_join(threads[i], NULL);
#endif
  }

  return 0;
}

static bool hwb_parallel_supported_for(const hwb_context* ctx, int workers) {
  (void)ctx;
  hwb_hardware_info hw;
  if (hwb_detect_hardware(&hw) != 0) {
    return false;
  }
  return hw.logical_cores >= workers;
}

static int hwb_parallel_run_for(const hwb_context* ctx, hwb_benchmark_result* out, const char* id,
                                const char* variant, int workers) {
  memset(out, 0, sizeof(*out));
  out->id = id;
  out->category = "cpu";
  out->variant = variant;
  out->unit = "Mops/s";
  out->threads = workers;
  out->class_kind = HWB_BENCH_CLASS_SCENARIO;
  out->synthetic = true;

  hwb_parallel_task tasks[8];
  hwb_thread_t threads[8];
  const uint64_t iterations_per_worker = 4000000ULL;

  double warmup_start = hwb_now_seconds();
  while ((hwb_now_seconds() - warmup_start) * 1000.0 < (double)ctx->warmup_ms) {
    if (hwb_parallel_run_once(workers, iterations_per_worker / 4ULL, tasks, threads) != 0) {
      return -1;
    }
  }
  out->warmup_ms = (hwb_now_seconds() - warmup_start) * 1000.0;

  double measured_start = hwb_now_seconds();
  for (int s = 0; s < ctx->samples && s < HWB_MAX_SAMPLES; ++s) {
    double t0 = hwb_now_seconds();
    if (hwb_parallel_run_once(workers, iterations_per_worker, tasks, threads) != 0) {
      return -1;
    }
    double t1 = hwb_now_seconds();

    double ops = (double)workers * (double)iterations_per_worker;
    out->samples[out->sample_count++] = (ops / (t1 - t0)) / 1e6;
  }
  out->measured_ms = (hwb_now_seconds() - measured_start) * 1000.0;

  return hwb_compute_stats(out->samples, out->sample_count, &out->summary);
}

#define HWB_DEFINE_PAR_BENCH(CORES)                                                                      \
  static bool hwb_parallel_##CORES##_supported(const hwb_context* ctx) {                                 \
    return hwb_parallel_supported_for(ctx, CORES);                                                        \
  }                                                                                                        \
  static int hwb_parallel_##CORES##_run(const hwb_context* ctx, hwb_benchmark_result* out) {             \
    return hwb_parallel_run_for(ctx, out, "cpu.parallel.int_add." #CORES "c", "parallel_" #CORES "c", \
                                CORES);                                                                    \
  }                                                                                                        \
  const hwb_benchmark_desc hwb_bench_cpu_parallel_int_add_##CORES##c = {                                 \
    .id = "cpu.parallel.int_add." #CORES "c",                                                           \
    .category = "cpu",                                                                                   \
    .name = "CPU parallel integer add throughput (" #CORES " cores)",                                  \
    .unit = "Mops/s",                                                                                    \
    .variant = "parallel_" #CORES "c",                                                                  \
    .class_kind = HWB_BENCH_CLASS_SCENARIO,                                                               \
    .synthetic = true,                                                                                    \
    .is_supported = hwb_parallel_##CORES##_supported,                                                     \
    .run = hwb_parallel_##CORES##_run,                                                                    \
  }

HWB_DEFINE_PAR_BENCH(2);
HWB_DEFINE_PAR_BENCH(3);
HWB_DEFINE_PAR_BENCH(4);
HWB_DEFINE_PAR_BENCH(5);
HWB_DEFINE_PAR_BENCH(6);
HWB_DEFINE_PAR_BENCH(7);
HWB_DEFINE_PAR_BENCH(8);

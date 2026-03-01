#include "hwbench/bench.h"
#include "hwbench/stats.h"
#include "hwbench/timer.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static bool memory_latency_pointer_chase_supported(const hwb_context* ctx) {
  (void)ctx;
  return true;
}

static uint32_t lcg_next(uint32_t* state) {
  *state = (*state * 1664525u) + 1013904223u;
  return *state;
}

static int make_permutation(uint32_t* next, size_t n) {
  uint32_t* perm = (uint32_t*)malloc(sizeof(uint32_t) * n);
  if (!perm) {
    return -1;
  }

  for (size_t i = 0; i < n; ++i) {
    perm[i] = (uint32_t)i;
  }

  uint32_t seed = 0xC0FFEEu;
  for (size_t i = n - 1; i > 0; --i) {
    size_t j = (size_t)(lcg_next(&seed) % (uint32_t)(i + 1));
    uint32_t tmp = perm[i];
    perm[i] = perm[j];
    perm[j] = tmp;
  }

  for (size_t i = 0; i + 1 < n; ++i) {
    next[perm[i]] = perm[i + 1];
  }
  next[perm[n - 1]] = perm[0];

  free(perm);
  return 0;
}

static double run_steps(const uint32_t* next, size_t steps) {
  volatile uint32_t idx = 0;
  double t0 = hwb_now_seconds();
  for (size_t i = 0; i < steps; ++i) {
    idx = next[idx];
  }
  double t1 = hwb_now_seconds();
  (void)idx;
  return (t1 - t0) * 1e9 / (double)steps;
}

static int memory_latency_pointer_chase_run(const hwb_context* ctx, hwb_benchmark_result* out) {
  memset(out, 0, sizeof(*out));
  out->id = "memory.latency.pointer_chase";
  out->category = "memory";
  out->variant = "random_64MiB";
  out->unit = "ns/access";
  out->threads = ctx->threads;
  out->class_kind = HWB_BENCH_CLASS_MICRO;
  out->synthetic = true;

  const size_t n = (64u * 1024u * 1024u) / sizeof(uint32_t);
  uint32_t* next = (uint32_t*)malloc(sizeof(uint32_t) * n);
  if (!next) {
    return -1;
  }
  if (make_permutation(next, n) != 0) {
    free(next);
    return -1;
  }

  size_t steps = 1u << 20;
  while (steps < (1u << 28)) {
    double seconds = (run_steps(next, steps) * (double)steps) / 1e9;
    if (seconds * 1000.0 >= (double)ctx->min_sample_ms) {
      break;
    }
    steps *= 2;
  }

  double warmup_start = hwb_now_seconds();
  while ((hwb_now_seconds() - warmup_start) * 1000.0 < (double)ctx->warmup_ms) {
    (void)run_steps(next, steps);
  }
  out->warmup_ms = (hwb_now_seconds() - warmup_start) * 1000.0;

  double measured_start = hwb_now_seconds();
  for (int s = 0; s < ctx->samples && s < HWB_MAX_SAMPLES; ++s) {
    out->samples[out->sample_count++] = run_steps(next, steps);
  }
  out->measured_ms = (hwb_now_seconds() - measured_start) * 1000.0;

  free(next);
  return hwb_compute_stats(out->samples, out->sample_count, &out->summary);
}

const hwb_benchmark_desc hwb_bench_memory_latency_pointer_chase = {
  .id = "memory.latency.pointer_chase",
  .category = "memory",
  .name = "Random pointer-chase latency",
  .unit = "ns/access",
  .variant = "random_64MiB",
  .class_kind = HWB_BENCH_CLASS_MICRO,
  .synthetic = true,
  .is_supported = memory_latency_pointer_chase_supported,
  .run = memory_latency_pointer_chase_run,
};

#ifndef HWBENCH_BENCH_H
#define HWBENCH_BENCH_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HWB_MAX_SAMPLES 64

typedef enum hwb_bench_class {
  HWB_BENCH_CLASS_MICRO = 0,
  HWB_BENCH_CLASS_COMPONENT = 1,
  HWB_BENCH_CLASS_SCENARIO = 2
} hwb_bench_class;

typedef struct hwb_context {
  int threads;
  int warmup_ms;
  int min_sample_ms;
  int samples;
  bool synthetic;
} hwb_context;

typedef struct hwb_stats {
  double min;
  double max;
  double median;
  double mean;
  double stdev;
  double p5;
  double p25;
  double p75;
  double p95;
  double p99;
  double cv;
} hwb_stats;

typedef struct hwb_benchmark_result {
  const char* id;
  const char* category;
  const char* variant;
  const char* unit;
  int threads;
  hwb_bench_class class_kind;
  bool synthetic;
  double samples[HWB_MAX_SAMPLES];
  size_t sample_count;
  hwb_stats summary;
  double warmup_ms;
  double measured_ms;
} hwb_benchmark_result;

struct hwb_benchmark_desc;
typedef struct hwb_benchmark_desc hwb_benchmark_desc;

typedef bool (*hwb_is_supported_fn)(const hwb_context* ctx);
typedef int (*hwb_run_fn)(const hwb_context* ctx, hwb_benchmark_result* out);

typedef int (*hwb_bench_warmup_pass_fn)(void* user_data);
typedef int (*hwb_bench_sample_pass_fn)(void* user_data, double* out_value);

int hwb_run_samples(const hwb_context* ctx,
                    hwb_bench_warmup_pass_fn warmup_pass,
                    hwb_bench_sample_pass_fn sample_pass,
                    void* user_data,
                    hwb_benchmark_result* out);

struct hwb_benchmark_desc {
  const char* id;
  const char* category;
  const char* name;
  const char* unit;
  const char* variant;
  hwb_bench_class class_kind;
  bool synthetic;
  hwb_is_supported_fn is_supported;
  hwb_run_fn run;
};

typedef struct hwb_alias_entry {
  const char* alias;
  const char* canonical_id;
} hwb_alias_entry;

typedef struct hwb_registry {
  const hwb_benchmark_desc** entries;
  size_t count;
  const hwb_alias_entry* aliases;
  size_t alias_count;
} hwb_registry;

void hwb_registry_init(hwb_registry* registry, const hwb_benchmark_desc** entries, size_t count);
void hwb_registry_set_aliases(hwb_registry* registry, const hwb_alias_entry* aliases, size_t alias_count);
const hwb_benchmark_desc* hwb_registry_find(const hwb_registry* registry, const char* id);
int hwb_run_benchmark(const hwb_context* ctx, const hwb_benchmark_desc* desc, hwb_benchmark_result* out);

#ifdef __cplusplus
}
#endif

#endif

#include "hwbench/bench.h"
#include "hwbench/benches.h"
#include "hwbench/json.h"
#include "hwbench/system.h"
#include "hwbench/stress.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_usage(void) {
  puts("hwbench-c options:\n"
       "  --list\n"
       "  --suite quick|cpu|memory|storage|gpu|cpu,memory,storage,gpu\n"
       "  --bench <id>\n"
       "  --stress 10|30|60\n"
       "  --samples <n>\n"
       "  --warmup-ms <ms>\n"
       "  --min-sample-ms <ms>\n"
       "  --threads <n>\n"
       "  --out <path>");
}


static void print_result_header(void) {
  puts("BENCHMARK	VARIANT	THREADS	MEDIAN	UNIT	CV%");
}

static void print_result_row(const hwb_benchmark_result* r) {
  printf("%s	%s	%d	%.3f	%s	%.2f\n",
         r->id, r->variant, r->threads, r->summary.median, r->unit, r->summary.cv);
}

static void print_hardware_report(void) {
  hwb_hardware_info hw;
  if (hwb_detect_hardware(&hw) != 0) {
    return;
  }

  puts("HARDWARE");
  printf("  CPU: %s\n", hw.cpu_model);
  printf("  Cores: %d logical / %d physical\n", hw.logical_cores, hw.physical_cores);
  printf("  Memory: %llu MB\n", hw.memory_total_mb);
  printf("  Storage: %s (%llu GB total, %d devices)\n", hw.storage_name, hw.storage_total_gb, hw.storage_device_count);
  printf("  Storage devices: %s\n", hw.storage_devices);
  printf("  GPU: %s\n", hw.gpu_name);
}

int main(int argc, char** argv) {
  hwb_registry registry;
  hwb_register_default_benches(&registry);

  hwb_context ctx = {.threads = 1, .warmup_ms = 250, .min_sample_ms = 500, .samples = 7, .synthetic = true};
  const char* bench_id = NULL;
  int list_only = 0;
  int run_quick = 0;
  int run_cpu_suite = 0;
  int run_memory_suite = 0;
  int run_storage_suite = 0;
  int run_gpu_suite = 0;
  int run_stress = 0;
  int stress_seconds = 0;
  const char* out_path = "hwbench-results.json";
  int threads_set = 0;

  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "--list") == 0) {
      list_only = 1;
    } else if (strcmp(argv[i], "--suite") == 0 && i + 1 < argc) {
      const char* suite = argv[++i];
      if (strcmp(suite, "quick") == 0) {
        run_quick = 1;
      } else {
        const char* cursor = suite;
        while (1) {
          while (*cursor == ' ' || *cursor == '\t') cursor++;
          if (*cursor == '\0') {
            print_usage();
            return 1;
          }
          const char* token_start = cursor;
          while (*cursor != '\0' && *cursor != ',') cursor++;
          const char* token_end = cursor;
          while (token_end > token_start && (token_end[-1] == ' ' || token_end[-1] == '\t')) {
            token_end--;
          }
          size_t len = (size_t)(token_end - token_start);
          if (len == 0) {
            print_usage();
            return 1;
          }
          if (len == 3 && strncmp(token_start, "cpu", 3) == 0) {
            run_cpu_suite = 1;
          } else if (len == 6 && strncmp(token_start, "memory", 6) == 0) {
            run_memory_suite = 1;
          } else if (len == 7 && strncmp(token_start, "storage", 7) == 0) {
            run_storage_suite = 1;
          } else if (len == 3 && strncmp(token_start, "gpu", 3) == 0) {
            run_gpu_suite = 1;
          } else {
            print_usage();
            return 1;
          }
          if (*cursor == ',') {
            cursor++;
            continue;
          }
          break;
        }
        if (!(run_cpu_suite || run_memory_suite || run_storage_suite || run_gpu_suite)) {
          print_usage();
          return 1;
        }
      }
    } else if (strcmp(argv[i], "--bench") == 0 && i + 1 < argc) {
      bench_id = argv[++i];
    } else if (strcmp(argv[i], "--stress") == 0 && i + 1 < argc) {
      stress_seconds = atoi(argv[++i]);
      if (stress_seconds != 10 && stress_seconds != 30 && stress_seconds != 60) {
        print_usage();
        return 1;
      }
      run_stress = 1;
    } else if (strcmp(argv[i], "--samples") == 0 && i + 1 < argc) {
      ctx.samples = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--warmup-ms") == 0 && i + 1 < argc) {
      ctx.warmup_ms = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--min-sample-ms") == 0 && i + 1 < argc) {
      ctx.min_sample_ms = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--threads") == 0 && i + 1 < argc) {
      ctx.threads = atoi(argv[++i]);
      threads_set = 1;
    } else if (strcmp(argv[i], "--out") == 0 && i + 1 < argc) {
      out_path = argv[++i];
    } else {
      print_usage();
      return 1;
    }
  }

  if (list_only) {
    for (size_t i = 0; i < registry.count; ++i) {
      const hwb_benchmark_desc* b = registry.entries[i];
      printf("%s\t%s\t%s\n", b->id, b->category, b->name);
    }
    return 0;
  }

  if (run_stress && (bench_id || run_quick || run_cpu_suite || run_memory_suite || run_storage_suite || run_gpu_suite)) {
    print_usage();
    return 1;
  }

  size_t max_results = run_stress ? 2 : (registry.count > 0 ? registry.count : 1);
  hwb_benchmark_result* results = malloc(max_results * sizeof(hwb_benchmark_result));
  if (!results) {
    fprintf(stderr, "Out of memory\n");
    return 1;
  }
  size_t result_count = 0;
  int printed_header = 0;

  if (run_stress) {
    int cpu_threads = threads_set ? ctx.threads : 0;
    int rc = hwb_run_cpu_stress(stress_seconds, cpu_threads, &results[result_count]);
    if (rc != 0) {
      fprintf(stderr, "CPU stress run failed\n");
      free(results);
      return 3;
    }
    result_count++;
    if (!printed_header) {
      print_hardware_report();
      print_result_header();
      printed_header = 1;
    }
    print_result_row(&results[result_count - 1]);

    rc = hwb_run_gpu_stress(stress_seconds, &results[result_count]);
    if (rc == 0) {
      result_count++;
      print_result_row(&results[result_count - 1]);
    } else if (rc != -2) {
      fprintf(stderr, "GPU stress run failed\n");
      free(results);
      return 3;
    } else {
      printf("GPU stress unsupported on this host.\n");
    }
  } else if (bench_id) {
    const hwb_benchmark_desc* b = hwb_registry_find(&registry, bench_id);
    if (!b) {
      fprintf(stderr, "Unknown benchmark id: %s\n", bench_id);
      free(results);
      return 2;
    }
    int rc = hwb_run_benchmark(&ctx, b, &results[result_count]);
    if (rc == -2) {
      printf("Benchmark unsupported: %s\n", bench_id);
      free(results);
      return 0;
    }
    if (rc != 0) {
      fprintf(stderr, "Benchmark failed: %s\n", bench_id);
      free(results);
      return 3;
    }
    result_count++;
    if (!printed_header) {
      print_hardware_report();
      print_result_header();
      printed_header = 1;
    }
    print_result_row(&results[result_count - 1]);
  } else if (run_quick || argc == 1 || run_cpu_suite || run_memory_suite || run_storage_suite || run_gpu_suite) {
    for (size_t i = 0; i < registry.count && result_count < max_results; ++i) {
      const hwb_benchmark_desc* b = registry.entries[i];
      int selected = run_quick || argc == 1;
      if (!selected && run_cpu_suite && strcmp(b->category, "cpu") == 0) selected = 1;
      if (!selected && run_memory_suite && strcmp(b->category, "memory") == 0) selected = 1;
      if (!selected && run_storage_suite && strcmp(b->category, "storage") == 0) selected = 1;
      if (!selected && run_gpu_suite && strcmp(b->category, "gpu") == 0) selected = 1;
      if (!selected) continue;

      if (hwb_run_benchmark(&ctx, b, &results[result_count]) == 0) {
        result_count++;
        if (!printed_header) {
          print_hardware_report();
          print_result_header();
          printed_header = 1;
        }
        print_result_row(&results[result_count - 1]);
      }
    }
  } else {
    print_usage();
    free(results);
    return 1;
  }

  if (!printed_header) {
    print_hardware_report();
    print_result_header();
  }

  if (hwb_write_json_results(out_path, results, result_count, "0.1.0", "local-run") != 0) {
    fprintf(stderr, "Failed to write JSON output: %s\n", out_path);
    free(results);
    return 4;
  }

  free(results);
  return 0;
}

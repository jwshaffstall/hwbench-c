#include "cli.h"
#include "hwbench/benches.h"
#include "hwbench/json.h"
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

static int parse_suite_tokens(const char* suite, int* run_cpu, int* run_memory, int* run_storage, int* run_gpu) {
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
      *run_cpu = 1;
    } else if (len == 6 && strncmp(token_start, "memory", 6) == 0) {
      *run_memory = 1;
    } else if (len == 7 && strncmp(token_start, "storage", 7) == 0) {
      *run_storage = 1;
    } else if (len == 3 && strncmp(token_start, "gpu", 3) == 0) {
      *run_gpu = 1;
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
  return 0;
}

int hwb_cli_parse(int argc, char** argv, hwb_cli_config* out) {
  hwb_context ctx = {.threads = 1, .warmup_ms = 250, .min_sample_ms = 500, .samples = 7, .synthetic = true};
  const char* bench_id = NULL;
  hwb_cli_mode mode = HWB_CLI_MODE_NONE;
  int run_cpu = 0;
  int run_memory = 0;
  int run_storage = 0;
  int run_gpu = 0;
  int stress_seconds = 0;
  const char* out_path = "hwbench-results.json";
  int threads_set = 0;

  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "--list") == 0) {
      mode = HWB_CLI_MODE_LIST;
    } else if (strcmp(argv[i], "--suite") == 0 && i + 1 < argc) {
      const char* suite = argv[++i];
      if (strcmp(suite, "quick") == 0) {
        mode = HWB_CLI_MODE_QUICK;
      } else {
        if (parse_suite_tokens(suite, &run_cpu, &run_memory, &run_storage, &run_gpu) != 0) {
          return 1;
        }
        if (!(run_cpu || run_memory || run_storage || run_gpu)) {
          print_usage();
          return 1;
        }
        mode = HWB_CLI_MODE_SUITE;
      }
    } else if (strcmp(argv[i], "--bench") == 0 && i + 1 < argc) {
      bench_id = argv[++i];
      mode = HWB_CLI_MODE_SINGLE;
    } else if (strcmp(argv[i], "--stress") == 0 && i + 1 < argc) {
      stress_seconds = atoi(argv[++i]);
      if (stress_seconds != 10 && stress_seconds != 30 && stress_seconds != 60) {
        print_usage();
        return 1;
      }
      mode = HWB_CLI_MODE_STRESS;
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

  if (mode == HWB_CLI_MODE_NONE && argc > 1) {
    print_usage();
    return 1;
  }

  out->mode = mode;
  out->bench_id = bench_id;
  out->stress_seconds = stress_seconds;
  out->out_path = out_path;
  out->ctx = ctx;
  out->threads_set = threads_set;
  out->run_cpu = run_cpu;
  out->run_memory = run_memory;
  out->run_storage = run_storage;
  out->run_gpu = run_gpu;
  return 0;
}

size_t hwb_cli_resolve_suite(const hwb_cli_config* config,
                             const hwb_registry* registry,
                             const hwb_benchmark_desc** out,
                             size_t out_cap) {
  size_t count = 0;
  for (size_t i = 0; i < registry->count && count < out_cap; ++i) {
    const hwb_benchmark_desc* b = registry->entries[i];
    int selected = 0;
    if (config->mode == HWB_CLI_MODE_QUICK) {
      selected = 1;
    } else if (config->mode == HWB_CLI_MODE_SUITE) {
      if (config->run_cpu && strcmp(b->category, "cpu") == 0) selected = 1;
      if (config->run_memory && strcmp(b->category, "memory") == 0) selected = 1;
      if (config->run_storage && strcmp(b->category, "storage") == 0) selected = 1;
      if (config->run_gpu && strcmp(b->category, "gpu") == 0) selected = 1;
    }
    if (selected) {
      out[count++] = b;
    }
  }
  return count;
}

static void print_result_header(void) {
  puts("BENCHMARK\tVARIANT\tTHREADS\tMEDIAN\tUNIT\tCV%");
}

static void print_result_row(const hwb_benchmark_result* r) {
  printf("%s\t%s\t%d\t%.3f\t%s\t%.2f\n",
         r->id, r->variant, r->threads, r->summary.median, r->unit, r->summary.cv);
}

static void print_hardware_report(const hwb_hardware_info* hw) {
  puts("HARDWARE");
  printf("  CPU: %s\n", hw->cpu_model);
  printf("  Cores: %d logical / %d physical\n", hw->logical_cores, hw->physical_cores);
  printf("  Memory: %llu MB\n", hw->memory_total_mb);
  printf("  Storage: %s (%llu GB total, %d devices)\n", hw->storage_name, hw->storage_total_gb, hw->storage_device_count);
  printf("  Storage devices: %s\n", hw->storage_devices);
  printf("  GPU: %s\n", hw->gpu_name);
}

int hwb_cli_run(const hwb_cli_config* config,
                const hwb_registry* registry,
                const hwb_hardware_info* hw) {
  size_t max_results = config->mode == HWB_CLI_MODE_STRESS ? 2 : (registry->count > 0 ? registry->count : 1);
  hwb_benchmark_result* results = malloc(max_results * sizeof(hwb_benchmark_result));
  if (!results) {
    fprintf(stderr, "Out of memory\n");
    return 1;
  }
  size_t result_count = 0;
  int printed_header = 0;

  if (config->mode == HWB_CLI_MODE_STRESS) {
    int cpu_threads = config->threads_set ? config->ctx.threads : 0;
    int rc = hwb_run_cpu_stress(config->stress_seconds, cpu_threads, &results[result_count]);
    if (rc != 0) {
      fprintf(stderr, "CPU stress run failed\n");
      free(results);
      return 3;
    }
    result_count++;
    if (!printed_header) {
      print_hardware_report(hw);
      print_result_header();
      printed_header = 1;
    }
    print_result_row(&results[result_count - 1]);

    rc = hwb_run_gpu_stress(config->stress_seconds, &results[result_count]);
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
  } else if (config->mode == HWB_CLI_MODE_SINGLE) {
    const hwb_benchmark_desc* b = hwb_registry_find(registry, config->bench_id);
    if (!b) {
      fprintf(stderr, "Unknown benchmark id: %s\n", config->bench_id);
      free(results);
      return 2;
    }
    int rc = hwb_run_benchmark(&config->ctx, b, &results[result_count]);
    if (rc == -2) {
      printf("Benchmark unsupported: %s\n", config->bench_id);
      free(results);
      return 0;
    }
    if (rc != 0) {
      fprintf(stderr, "Benchmark failed: %s\n", config->bench_id);
      free(results);
      return 3;
    }
    result_count++;
    if (!printed_header) {
      print_hardware_report(hw);
      print_result_header();
      printed_header = 1;
    }
    print_result_row(&results[result_count - 1]);
  } else {
    const hwb_benchmark_desc** selected = malloc(max_results * sizeof(const hwb_benchmark_desc*));
    if (!selected) {
      fprintf(stderr, "Out of memory\n");
      free(results);
      return 1;
    }
    size_t sel_count = hwb_cli_resolve_suite(config, registry, selected, max_results);

    for (size_t i = 0; i < sel_count && result_count < max_results; ++i) {
      if (hwb_run_benchmark(&config->ctx, selected[i], &results[result_count]) == 0) {
        result_count++;
        if (!printed_header) {
          print_hardware_report(hw);
          print_result_header();
          printed_header = 1;
        }
        print_result_row(&results[result_count - 1]);
      }
    }
    free(selected);
  }

  if (!printed_header) {
    print_hardware_report(hw);
    print_result_header();
  }

  if (hwb_write_json_results(config->out_path, results, result_count, hw, "0.1.0", "local-run") != 0) {
    fprintf(stderr, "Failed to write JSON output: %s\n", config->out_path);
    free(results);
    return 4;
  }

  free(results);
  return 0;
}

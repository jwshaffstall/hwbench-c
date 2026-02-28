#include "hwbench/bench.h"
#include "hwbench/benches.h"
#include "hwbench/json.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_usage(void) {
  puts("hwbench-c options:\n"
       "  --list\n"
       "  --suite quick\n"
       "  --bench <id>\n"
       "  --samples <n>\n"
       "  --warmup-ms <ms>\n"
       "  --min-sample-ms <ms>\n"
       "  --threads <n>\n"
       "  --out <path>");
}

int main(int argc, char** argv) {
  hwb_registry registry;
  hwb_register_default_benches(&registry);

  hwb_context ctx = {.threads = 1, .warmup_ms = 250, .min_sample_ms = 500, .samples = 7, .synthetic = true};
  const char* bench_id = NULL;
  int list_only = 0;
  int run_quick = 0;
  const char* out_path = "hwbench-results.json";

  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "--list") == 0) {
      list_only = 1;
    } else if (strcmp(argv[i], "--suite") == 0 && i + 1 < argc) {
      if (strcmp(argv[++i], "quick") == 0) run_quick = 1;
    } else if (strcmp(argv[i], "--bench") == 0 && i + 1 < argc) {
      bench_id = argv[++i];
    } else if (strcmp(argv[i], "--samples") == 0 && i + 1 < argc) {
      ctx.samples = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--warmup-ms") == 0 && i + 1 < argc) {
      ctx.warmup_ms = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--min-sample-ms") == 0 && i + 1 < argc) {
      ctx.min_sample_ms = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--threads") == 0 && i + 1 < argc) {
      ctx.threads = atoi(argv[++i]);
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

  hwb_benchmark_result results[HWB_MAX_SAMPLES];
  size_t result_count = 0;

  if (bench_id) {
    const hwb_benchmark_desc* b = hwb_registry_find(&registry, bench_id);
    if (!b) {
      fprintf(stderr, "Unknown benchmark id: %s\n", bench_id);
      return 2;
    }
    if (hwb_run_benchmark(&ctx, b, &results[result_count]) != 0) {
      fprintf(stderr, "Benchmark failed: %s\n", bench_id);
      return 3;
    }
    result_count++;
  } else if (run_quick || argc == 1) {
    for (size_t i = 0; i < registry.count; ++i) {
      if (hwb_run_benchmark(&ctx, registry.entries[i], &results[result_count]) == 0) {
        result_count++;
      }
    }
  } else {
    print_usage();
    return 1;
  }

  puts("BENCHMARK\tVARIANT\tTHREADS\tMEDIAN\tUNIT\tCV%");
  for (size_t i = 0; i < result_count; ++i) {
    hwb_benchmark_result* r = &results[i];
    printf("%s\t%s\t%d\t%.3f\t%s\t%.2f\n",
           r->id, r->variant, r->threads, r->summary.median, r->unit, r->summary.cv);
  }

  if (hwb_write_json_results(out_path, results, result_count, "0.1.0", "local-run") != 0) {
    fprintf(stderr, "Failed to write JSON output: %s\n", out_path);
    return 4;
  }

  return 0;
}

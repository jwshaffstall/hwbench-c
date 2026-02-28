#include "hwbench/json.h"
#include "hwbench/system.h"

#include <stdio.h>
#include <time.h>

static void write_escaped(FILE* f, const char* s) {
  for (; *s; ++s) {
    if (*s == '"' || *s == '\\') {
      fputc('\\', f);
    }
    fputc(*s, f);
  }
}

int hwb_write_json_results(const char* path,
                           const hwb_benchmark_result* results,
                           size_t result_count,
                           const char* suite_version,
                           const char* run_id) {
  FILE* f = fopen(path, "w");
  if (!f) {
    return -1;
  }

  time_t now = time(NULL);
  struct tm utc_tm;
#if defined(_WIN32)
  gmtime_s(&utc_tm, &now);
#else
  gmtime_r(&now, &utc_tm);
#endif
  char ts[64];
  strftime(ts, sizeof(ts), "%Y-%m-%dT%H:%M:%SZ", &utc_tm);

  fprintf(f, "{\n");
  fprintf(f, "  \"schema_version\": \"1.0\",\n");
  fprintf(f, "  \"suite_version\": \""); write_escaped(f, suite_version); fprintf(f, "\",\n");
  fprintf(f, "  \"run_id\": \""); write_escaped(f, run_id); fprintf(f, "\",\n");
  fprintf(f, "  \"timestamp_utc\": \"%s\",\n", ts);
  fprintf(f, "  \"machine\": {\"arch\": \"%s\"},\n", hwb_arch_name());
  fprintf(f, "  \"os\": {\"name\": \"%s\"},\n", hwb_os_name());
  fprintf(f, "  \"benchmarks\": [\n");

  for (size_t i = 0; i < result_count; ++i) {
    const hwb_benchmark_result* r = &results[i];
    fprintf(f, "    {\n");
    fprintf(f, "      \"id\": \"%s\",\n", r->id);
    fprintf(f, "      \"category\": \"%s\",\n", r->category);
    fprintf(f, "      \"variant\": \"%s\",\n", r->variant);
    fprintf(f, "      \"threads\": %d,\n", r->threads);
    fprintf(f, "      \"units\": \"%s\",\n", r->unit);
    fprintf(f, "      \"samples\": [");
    for (size_t j = 0; j < r->sample_count; ++j) {
      fprintf(f, "%s%.6f", j ? ", " : "", r->samples[j]);
    }
    fprintf(f, "],\n");
    fprintf(f, "      \"summary\": {\"min\": %.6f, \"max\": %.6f, \"median\": %.6f, \"mean\": %.6f, \"stdev\": %.6f, \"p95\": %.6f, \"cv\": %.6f},\n",
            r->summary.min, r->summary.max, r->summary.median, r->summary.mean, r->summary.stdev, r->summary.p95, r->summary.cv);
    fprintf(f, "      \"metadata\": {\"warmup_ms\": %.3f, \"measured_ms\": %.3f, \"synthetic\": %s}\n",
            r->warmup_ms, r->measured_ms, r->synthetic ? "true" : "false");
    fprintf(f, "    }%s\n", (i + 1 < result_count) ? "," : "");
  }

  fprintf(f, "  ]\n");
  fprintf(f, "}\n");

  fclose(f);
  return 0;
}

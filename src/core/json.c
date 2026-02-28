#include "hwbench/json.h"
#include "hwbench/system.h"

#include <stdio.h>
#include <time.h>

static void write_json_string(FILE* f, const char* s) {
  static const char hex[] = "0123456789abcdef";
  if (!s) {
    fputs("null", f);
    return;
  }

  fputc('"', f);
  for (; *s; ++s) {
    unsigned char c = (unsigned char)*s;
    switch (c) {
      case '"': fputs("\\\"", f); break;
      case '\\': fputs("\\\\", f); break;
      case '\b': fputs("\\b", f); break;
      case '\f': fputs("\\f", f); break;
      case '\n': fputs("\\n", f); break;
      case '\r': fputs("\\r", f); break;
      case '\t': fputs("\\t", f); break;
      default:
        if (c < 0x20) {
          fputs("\\u00", f);
          fputc(hex[(c >> 4) & 0x0F], f);
          fputc(hex[c & 0x0F], f);
        } else {
          fputc((int)c, f);
        }
        break;
    }
  }
  fputc('"', f);
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
  fprintf(f, "  \"suite_version\": "); write_json_string(f, suite_version); fprintf(f, ",\n");
  fprintf(f, "  \"run_id\": "); write_json_string(f, run_id); fprintf(f, ",\n");
  fprintf(f, "  \"timestamp_utc\": \"%s\",\n", ts);
  fprintf(f, "  \"machine\": {\"arch\": "); write_json_string(f, hwb_arch_name()); fprintf(f, "},\n");
  fprintf(f, "  \"os\": {\"name\": "); write_json_string(f, hwb_os_name()); fprintf(f, "},\n");
  fprintf(f, "  \"benchmarks\": [\n");

  for (size_t i = 0; i < result_count; ++i) {
    const hwb_benchmark_result* r = &results[i];
    fprintf(f, "    {\n");
    fprintf(f, "      \"id\": "); write_json_string(f, r->id); fprintf(f, ",\n");
    fprintf(f, "      \"category\": "); write_json_string(f, r->category); fprintf(f, ",\n");
    fprintf(f, "      \"variant\": "); write_json_string(f, r->variant); fprintf(f, ",\n");
    fprintf(f, "      \"threads\": %d,\n", r->threads);
    fprintf(f, "      \"units\": "); write_json_string(f, r->unit); fprintf(f, ",\n");
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

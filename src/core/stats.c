#include "hwbench/stats.h"

#include <math.h>
#include <stddef.h>
#include <stdlib.h>

static int compare_double(const void* a, const void* b) {
  const double da = *(const double*)a;
  const double db = *(const double*)b;
  if (da < db) return -1;
  if (da > db) return 1;
  return 0;
}

static double percentile(const double* sorted, size_t count, double p) {
  if (count == 1) {
    return sorted[0];
  }
  const double idx = p * (double)(count - 1);
  const size_t lo = (size_t)idx;
  const size_t hi = (lo + 1 < count) ? lo + 1 : lo;
  const double frac = idx - (double)lo;
  return sorted[lo] * (1.0 - frac) + sorted[hi] * frac;
}

int hwb_compute_stats(const double* values, size_t count, hwb_stats* out) {
  if (!values || !out || count == 0) {
    return -1;
  }

  double* sorted = (double*)malloc(sizeof(double) * count);
  if (!sorted) {
    return -2;
  }

  double sum = 0.0;
  for (size_t i = 0; i < count; ++i) {
    sorted[i] = values[i];
    sum += values[i];
  }
  qsort(sorted, count, sizeof(double), compare_double);

  out->min = sorted[0];
  out->max = sorted[count - 1];
  out->mean = sum / (double)count;
  out->median = percentile(sorted, count, 0.5);

  double variance = 0.0;
  for (size_t i = 0; i < count; ++i) {
    double diff = values[i] - out->mean;
    variance += diff * diff;
  }
  variance /= (double)count;
  out->stdev = sqrt(variance);

  out->p5 = percentile(sorted, count, 0.05);
  out->p25 = percentile(sorted, count, 0.25);
  out->p75 = percentile(sorted, count, 0.75);
  out->p95 = percentile(sorted, count, 0.95);
  out->p99 = percentile(sorted, count, 0.99);
  out->cv = (out->mean != 0.0) ? (out->stdev / out->mean) * 100.0 : 0.0;

  free(sorted);
  return 0;
}

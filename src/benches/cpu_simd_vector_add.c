#include "hwbench/bench.h"
#include "hwbench/stats.h"
#include "hwbench/timer.h"

#include <string.h>

#if defined(__AVX2__) || defined(__SSE2__) || defined(__ARM_NEON) || defined(__ARM_NEON__)
#define HWB_HAS_SIMD 1
#else
#define HWB_HAS_SIMD 0
#endif

#if defined(__AVX2__) || defined(__SSE2__)
#include <immintrin.h>
#endif
#if defined(__ARM_NEON) || defined(__ARM_NEON__)
#include <arm_neon.h>
#endif

typedef enum hwb_simd_kind {
  HWB_SIMD_I8,
  HWB_SIMD_I16,
  HWB_SIMD_I32,
  HWB_SIMD_I64,
  HWB_SIMD_F32,
  HWB_SIMD_F64
} hwb_simd_kind;

static volatile double hwb_simd_sink = 0.0;

static bool hwb_simd_supported(const hwb_context* ctx) {
  (void)ctx;
  return HWB_HAS_SIMD != 0;
}

static bool hwb_simd_i64_supported(const hwb_context* ctx) {
  (void)ctx;
#if HWB_HAS_SIMD && (defined(__AVX2__) || defined(__SSE2__) || defined(__aarch64__) || defined(_M_ARM64))
  return true;
#else
  return false;
#endif
}

static bool hwb_simd_f64_supported(const hwb_context* ctx) {
  (void)ctx;
#if HWB_HAS_SIMD && (defined(__AVX2__) || defined(__SSE2__) || defined(__aarch64__) || defined(_M_ARM64))
  return true;
#else
  return false;
#endif
}

static double hwb_simd_run_kernel(hwb_simd_kind kind, unsigned long long iters) {
#if defined(__AVX2__)
  if (kind == HWB_SIMD_I8) {
    __m256i a = _mm256_set1_epi8(1);
    __m256i b = _mm256_set1_epi8(3);
    for (unsigned long long i = 0; i < iters; ++i) a = _mm256_add_epi8(a, b);
    {
      unsigned char tmp[32];
      _mm256_storeu_si256((__m256i*)tmp, a);
      hwb_simd_sink += (double)tmp[0];
    }
    return 32.0;
  }
  if (kind == HWB_SIMD_I16) {
    __m256i a = _mm256_set1_epi16(2);
    __m256i b = _mm256_set1_epi16(7);
    for (unsigned long long i = 0; i < iters; ++i) a = _mm256_add_epi16(a, b);
    {
      short tmp[16];
      _mm256_storeu_si256((__m256i*)tmp, a);
      hwb_simd_sink += (double)tmp[0];
    }
    return 16.0;
  }
  if (kind == HWB_SIMD_I32) {
    __m256i a = _mm256_set1_epi32(4);
    __m256i b = _mm256_set1_epi32(11);
    for (unsigned long long i = 0; i < iters; ++i) a = _mm256_add_epi32(a, b);
    {
      int tmp[8];
      _mm256_storeu_si256((__m256i*)tmp, a);
      hwb_simd_sink += (double)tmp[0];
    }
    return 8.0;
  }
  if (kind == HWB_SIMD_I64) {
    __m256i a = _mm256_set1_epi64x(9);
    __m256i b = _mm256_set1_epi64x(13);
    for (unsigned long long i = 0; i < iters; ++i) a = _mm256_add_epi64(a, b);
    {
      long long tmp[4];
      _mm256_storeu_si256((__m256i*)tmp, a);
      hwb_simd_sink += (double)tmp[0];
    }
    return 4.0;
  }
  if (kind == HWB_SIMD_F32) {
    __m256 a = _mm256_set1_ps(1.5f);
    __m256 b = _mm256_set1_ps(0.5f);
    for (unsigned long long i = 0; i < iters; ++i) a = _mm256_add_ps(a, b);
    {
      float tmp[8];
      _mm256_storeu_ps(tmp, a);
      hwb_simd_sink += (double)tmp[0];
    }
    return 8.0;
  }
  {
    __m256d a = _mm256_set1_pd(1.25);
    __m256d b = _mm256_set1_pd(0.75);
    for (unsigned long long i = 0; i < iters; ++i) a = _mm256_add_pd(a, b);
    {
      double tmp[4];
      _mm256_storeu_pd(tmp, a);
      hwb_simd_sink += tmp[0];
    }
    return 4.0;
  }
#elif defined(__SSE2__)
  if (kind == HWB_SIMD_I8) {
    __m128i a = _mm_set1_epi8(1);
    __m128i b = _mm_set1_epi8(3);
    for (unsigned long long i = 0; i < iters; ++i) a = _mm_add_epi8(a, b);
    {
      unsigned char tmp[16];
      _mm_storeu_si128((__m128i*)tmp, a);
      hwb_simd_sink += (double)tmp[0];
    }
    return 16.0;
  }
  if (kind == HWB_SIMD_I16) {
    __m128i a = _mm_set1_epi16(2);
    __m128i b = _mm_set1_epi16(7);
    for (unsigned long long i = 0; i < iters; ++i) a = _mm_add_epi16(a, b);
    {
      short tmp[8];
      _mm_storeu_si128((__m128i*)tmp, a);
      hwb_simd_sink += (double)tmp[0];
    }
    return 8.0;
  }
  if (kind == HWB_SIMD_I32) {
    __m128i a = _mm_set1_epi32(4);
    __m128i b = _mm_set1_epi32(11);
    for (unsigned long long i = 0; i < iters; ++i) a = _mm_add_epi32(a, b);
    {
      int tmp[4];
      _mm_storeu_si128((__m128i*)tmp, a);
      hwb_simd_sink += (double)tmp[0];
    }
    return 4.0;
  }
  if (kind == HWB_SIMD_I64) {
    __m128i a = _mm_set1_epi64x(9);
    __m128i b = _mm_set1_epi64x(13);
    for (unsigned long long i = 0; i < iters; ++i) a = _mm_add_epi64(a, b);
    {
      long long tmp[2];
      _mm_storeu_si128((__m128i*)tmp, a);
      hwb_simd_sink += (double)tmp[0];
    }
    return 2.0;
  }
  if (kind == HWB_SIMD_F32) {
    __m128 a = _mm_set1_ps(1.5f);
    __m128 b = _mm_set1_ps(0.5f);
    for (unsigned long long i = 0; i < iters; ++i) a = _mm_add_ps(a, b);
    {
      float tmp[4];
      _mm_storeu_ps(tmp, a);
      hwb_simd_sink += (double)tmp[0];
    }
    return 4.0;
  }
  {
    __m128d a = _mm_set1_pd(1.25);
    __m128d b = _mm_set1_pd(0.75);
    for (unsigned long long i = 0; i < iters; ++i) a = _mm_add_pd(a, b);
    {
      double tmp[2];
      _mm_storeu_pd(tmp, a);
      hwb_simd_sink += tmp[0];
    }
    return 2.0;
  }
#elif defined(__ARM_NEON) || defined(__ARM_NEON__)
  if (kind == HWB_SIMD_I8) {
    int8x16_t a = vdupq_n_s8(1);
    int8x16_t b = vdupq_n_s8(3);
    for (unsigned long long i = 0; i < iters; ++i) a = vaddq_s8(a, b);
    hwb_simd_sink += (double)vgetq_lane_s8(a, 0);
    return 16.0;
  }
  if (kind == HWB_SIMD_I16) {
    int16x8_t a = vdupq_n_s16(2);
    int16x8_t b = vdupq_n_s16(7);
    for (unsigned long long i = 0; i < iters; ++i) a = vaddq_s16(a, b);
    hwb_simd_sink += (double)vgetq_lane_s16(a, 0);
    return 8.0;
  }
  if (kind == HWB_SIMD_I32) {
    int32x4_t a = vdupq_n_s32(4);
    int32x4_t b = vdupq_n_s32(11);
    for (unsigned long long i = 0; i < iters; ++i) a = vaddq_s32(a, b);
    hwb_simd_sink += (double)vgetq_lane_s32(a, 0);
    return 4.0;
  }
#if defined(__aarch64__) || defined(_M_ARM64)
  if (kind == HWB_SIMD_I64) {
    int64x2_t a = vdupq_n_s64(9);
    int64x2_t b = vdupq_n_s64(13);
    for (unsigned long long i = 0; i < iters; ++i) a = vaddq_s64(a, b);
    hwb_simd_sink += (double)vgetq_lane_s64(a, 0);
    return 2.0;
  }
#endif
  if (kind == HWB_SIMD_F32) {
    float32x4_t a = vdupq_n_f32(1.5f);
    float32x4_t b = vdupq_n_f32(0.5f);
    for (unsigned long long i = 0; i < iters; ++i) a = vaddq_f32(a, b);
    hwb_simd_sink += (double)vgetq_lane_f32(a, 0);
    return 4.0;
  }
#if defined(__aarch64__) || defined(_M_ARM64)
  {
    float64x2_t a = vdupq_n_f64(1.25);
    float64x2_t b = vdupq_n_f64(0.75);
    for (unsigned long long i = 0; i < iters; ++i) a = vaddq_f64(a, b);
    hwb_simd_sink += vgetq_lane_f64(a, 0);
    return 2.0;
  }
#else
  return 0.0;
#endif
#else
  (void)kind;
  (void)iters;
  return 0.0;
#endif
}

static int hwb_simd_run(const hwb_context* ctx, hwb_benchmark_result* out, const char* id,
                        const char* variant, hwb_simd_kind kind) {
  memset(out, 0, sizeof(*out));
  out->id = id;
  out->category = "cpu";
  out->variant = variant;
  out->unit = "Mops/s";
  out->threads = ctx->threads;
  out->class_kind = HWB_BENCH_CLASS_MICRO;
  out->synthetic = true;

  const unsigned long long iters = 30000000ULL;
  double warmup_start = hwb_now_seconds();
  while ((hwb_now_seconds() - warmup_start) * 1000.0 < (double)ctx->warmup_ms) {
    (void)hwb_simd_run_kernel(kind, iters / 10ULL);
  }
  out->warmup_ms = (hwb_now_seconds() - warmup_start) * 1000.0;

  double measured_start = hwb_now_seconds();
  for (int s = 0; s < ctx->samples && s < HWB_MAX_SAMPLES; ++s) {
    double t0 = hwb_now_seconds();
    const double lanes = hwb_simd_run_kernel(kind, iters);
    double t1 = hwb_now_seconds();
    if (lanes <= 0.0) {
      return -2;
    }

    out->samples[out->sample_count++] = ((double)iters * lanes / (t1 - t0)) / 1e6;
  }
  out->measured_ms = (hwb_now_seconds() - measured_start) * 1000.0;

  return hwb_compute_stats(out->samples, out->sample_count, &out->summary);
}

#define HWB_DEFINE_SIMD_BENCH(NAME, ID, VARIANT, KIND, SUPPORT_FN)                                        \
  static int hwb_simd_##NAME##_run(const hwb_context* ctx, hwb_benchmark_result* out) {                   \
    return hwb_simd_run(ctx, out, ID, VARIANT, KIND);                                                      \
  }                                                                                                         \
  const hwb_benchmark_desc hwb_bench_cpu_simd_##NAME = {                                                   \
    .id = ID,                                                                                               \
    .category = "cpu",                                                                                    \
    .name = "CPU SIMD " VARIANT " throughput",                                                           \
    .unit = "Mops/s",                                                                                     \
    .variant = VARIANT,                                                                                     \
    .class_kind = HWB_BENCH_CLASS_MICRO,                                                                   \
    .synthetic = true,                                                                                      \
    .is_supported = SUPPORT_FN,                                                                             \
    .run = hwb_simd_##NAME##_run,                                                                           \
  }

HWB_DEFINE_SIMD_BENCH(i8_add, "cpu.simd.i8_add", "simd_i8", HWB_SIMD_I8, hwb_simd_supported);
HWB_DEFINE_SIMD_BENCH(i16_add, "cpu.simd.i16_add", "simd_i16", HWB_SIMD_I16, hwb_simd_supported);
HWB_DEFINE_SIMD_BENCH(i32_add, "cpu.simd.i32_add", "simd_i32", HWB_SIMD_I32, hwb_simd_supported);
HWB_DEFINE_SIMD_BENCH(i64_add, "cpu.simd.i64_add", "simd_i64", HWB_SIMD_I64, hwb_simd_i64_supported);
HWB_DEFINE_SIMD_BENCH(f32_add, "cpu.simd.f32_add", "simd_f32", HWB_SIMD_F32, hwb_simd_supported);
HWB_DEFINE_SIMD_BENCH(f64_add, "cpu.simd.f64_add", "simd_f64", HWB_SIMD_F64, hwb_simd_f64_supported);

#include "hwbench/bench.h"
#include "hwbench/stress.h"
#include "hwbench/stats.h"
#include "hwbench/timer.h"

#include <stdlib.h>
#include <string.h>
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sched.h>
#endif

#if defined(HWB_HAVE_OPENCL)
#if !defined(CL_TARGET_OPENCL_VERSION)
#define CL_TARGET_OPENCL_VERSION 120
#endif
#if defined(__APPLE__)
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#define HWB_GPU_ELEMS (1u << 22)
#define HWB_GPU_STRESS_ELEMS (1u << 20)

typedef struct hwb_opencl_env {
  cl_context context;
  cl_command_queue queue;
  cl_device_id device;
  cl_program program;
} hwb_opencl_env;

static void hwb_opencl_shutdown(hwb_opencl_env* env);

static const char* hwb_opencl_kernel_source =
  "__kernel void vec_add(__global const float* a, __global const float* b, __global float* out) {"
  "  size_t i = get_global_id(0);"
  "  out[i] = a[i] + b[i];"
  "}"
  "__kernel void fma3(__global const float* a, __global const float* b, __global const float* c, __global float* out) {"
  "  size_t i = get_global_id(0);"
  "  out[i] = fma(a[i], b[i], c[i]);"
  "}"
  "__kernel void int_mad(__global const int* a, __global const int* b, __global const int* c, __global int* out) {"
  "  size_t i = get_global_id(0);"
  "  out[i] = a[i] * b[i] + c[i];"
  "}";

static int hwb_opencl_init(hwb_opencl_env* env) {
  cl_int err = CL_SUCCESS;
  cl_uint platform_count = 0;
  cl_platform_id platforms[8];

  memset(env, 0, sizeof(*env));

  err = clGetPlatformIDs(8, platforms, &platform_count);
  if (err != CL_SUCCESS || platform_count == 0) {
    goto fail;
  }

  for (cl_uint i = 0; i < platform_count; ++i) {
    cl_device_id device = NULL;
    err = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    if (err == CL_SUCCESS && device) {
      env->device = device;
      break;
    }
  }

  if (!env->device) {
    goto fail;
  }

  env->context = clCreateContext(NULL, 1, &env->device, NULL, NULL, &err);
  if (!env->context || err != CL_SUCCESS) {
    goto fail;
  }

  env->queue = clCreateCommandQueue(env->context, env->device, 0, &err);
  if (!env->queue || err != CL_SUCCESS) {
    goto fail;
  }

  env->program = clCreateProgramWithSource(env->context, 1, &hwb_opencl_kernel_source, NULL, &err);
  if (!env->program || err != CL_SUCCESS) {
    goto fail;
  }

  err = clBuildProgram(env->program, 1, &env->device, NULL, NULL, NULL);
  if (err != CL_SUCCESS) {
    goto fail;
  }

  return 0;

fail:
  hwb_opencl_shutdown(env);
  memset(env, 0, sizeof(*env));
  return -1;
}

static void hwb_opencl_shutdown(hwb_opencl_env* env) {
  if (env->program) {
    clReleaseProgram(env->program);
  }
  if (env->queue) {
    clReleaseCommandQueue(env->queue);
  }
  if (env->context) {
    clReleaseContext(env->context);
  }
  memset(env, 0, sizeof(*env));
}

static bool gpu_compute_supported(const hwb_context* ctx) {
  (void)ctx;
  cl_int err = CL_SUCCESS;
  cl_uint platform_count = 0;
  cl_platform_id platforms[8];

  err = clGetPlatformIDs(8, platforms, &platform_count);
  if (err != CL_SUCCESS || platform_count == 0) {
    return false;
  }

  for (cl_uint i = 0; i < platform_count; ++i) {
    cl_device_id device = NULL;
    err = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    if (err == CL_SUCCESS && device) {
      return true;
    }
  }

  return false;
}

typedef struct {
  void* a;
  void* b;
  void* c;
  cl_mem buf_a;
  cl_mem buf_b;
  cl_mem buf_c;
  cl_mem buf_out;
  cl_kernel kernel;
  size_t elem_count;
  int elem_size;
  int input_count;
} hwb_opencl_buffers;

static void hwb_opencl_cleanup(hwb_opencl_buffers* bufs) {
  if (bufs->kernel) clReleaseKernel(bufs->kernel);
  if (bufs->buf_out) clReleaseMemObject(bufs->buf_out);
  if (bufs->buf_c) clReleaseMemObject(bufs->buf_c);
  if (bufs->buf_b) clReleaseMemObject(bufs->buf_b);
  if (bufs->buf_a) clReleaseMemObject(bufs->buf_a);
  free(bufs->c);
  free(bufs->b);
  free(bufs->a);
}

static int hwb_opencl_setup_buffers(hwb_opencl_env* env,
                                    hwb_opencl_buffers* bufs,
                                    const char* kernel_name,
                                    size_t elem_count,
                                    int elem_size,
                                    int input_count,
                                    void (*fill_fn)(void* a, void* b, void* c, size_t n)) {
  cl_int err = CL_SUCCESS;
  memset(bufs, 0, sizeof(*bufs));
  bufs->elem_count = elem_count;
  bufs->elem_size = elem_size;
  bufs->input_count = input_count;

  bufs->a = malloc(elem_count * (size_t)elem_size);
  bufs->b = malloc(elem_count * (size_t)elem_size);
  if (input_count >= 3) {
    bufs->c = malloc(elem_count * (size_t)elem_size);
  }
  if (!bufs->a || !bufs->b || (input_count >= 3 && !bufs->c)) {
    return -1;
  }

  fill_fn(bufs->a, bufs->b, bufs->c, elem_count);

  bufs->buf_a = clCreateBuffer(env->context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                               elem_count * (size_t)elem_size, bufs->a, &err);
  if (!bufs->buf_a || err != CL_SUCCESS) return -1;
  bufs->buf_b = clCreateBuffer(env->context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                               elem_count * (size_t)elem_size, bufs->b, &err);
  if (!bufs->buf_b || err != CL_SUCCESS) return -1;
  if (input_count >= 3) {
    bufs->buf_c = clCreateBuffer(env->context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                 elem_count * (size_t)elem_size, bufs->c, &err);
    if (!bufs->buf_c || err != CL_SUCCESS) return -1;
  }
  bufs->buf_out = clCreateBuffer(env->context, CL_MEM_WRITE_ONLY,
                                 elem_count * (size_t)elem_size, NULL, &err);
  if (!bufs->buf_out || err != CL_SUCCESS) return -1;

  bufs->kernel = clCreateKernel(env->program, kernel_name, &err);
  if (!bufs->kernel || err != CL_SUCCESS) return -1;

  int arg_idx = 0;
  err = clSetKernelArg(bufs->kernel, arg_idx++, sizeof(cl_mem), &bufs->buf_a);
  if (err != CL_SUCCESS) return -1;
  err = clSetKernelArg(bufs->kernel, arg_idx++, sizeof(cl_mem), &bufs->buf_b);
  if (err != CL_SUCCESS) return -1;
  if (input_count >= 3) {
    err = clSetKernelArg(bufs->kernel, arg_idx++, sizeof(cl_mem), &bufs->buf_c);
    if (err != CL_SUCCESS) return -1;
  }
  err = clSetKernelArg(bufs->kernel, arg_idx++, sizeof(cl_mem), &bufs->buf_out);
  if (err != CL_SUCCESS) return -1;

  return 0;
}

static int hwb_opencl_run_sampled(hwb_opencl_env* env,
                                  hwb_opencl_buffers* bufs,
                                  const hwb_context* ctx,
                                  hwb_benchmark_result* out,
                                  double flops_per_elem) {
  cl_int err = CL_SUCCESS;
  size_t global = bufs->elem_count;

  double warmup_start = hwb_now_seconds();
  while ((hwb_now_seconds() - warmup_start) * 1000.0 < (double)ctx->warmup_ms) {
    err = clEnqueueNDRangeKernel(env->queue, bufs->kernel, 1, NULL, &global, NULL, 0, NULL, NULL);
    if (err != CL_SUCCESS) return -1;
    err = clFinish(env->queue);
    if (err != CL_SUCCESS) return -1;
  }
  out->warmup_ms = (hwb_now_seconds() - warmup_start) * 1000.0;

  double measured_start = hwb_now_seconds();
  for (int s = 0; s < ctx->samples && s < HWB_MAX_SAMPLES; ++s) {
    double t0 = hwb_now_seconds();
    err = clEnqueueNDRangeKernel(env->queue, bufs->kernel, 1, NULL, &global, NULL, 0, NULL, NULL);
    if (err != CL_SUCCESS) return -1;
    err = clFinish(env->queue);
    if (err != CL_SUCCESS) return -1;
    double t1 = hwb_now_seconds();
    out->samples[out->sample_count++] = ((double)bufs->elem_count * flops_per_elem) / (t1 - t0) / 1e9;
  }
  out->measured_ms = (hwb_now_seconds() - measured_start) * 1000.0;

  return hwb_compute_stats(out->samples, out->sample_count, &out->summary);
}

static void fill_float_2in(void* a, void* b, void* c, size_t n) {
  float* fa = (float*)a;
  float* fb = (float*)b;
  (void)c;
  for (size_t i = 0; i < n; ++i) {
    fa[i] = (float)(i % 1024) * 0.001f;
    fb[i] = (float)(i % 251) * 0.002f;
  }
}

static void fill_float_3in(void* a, void* b, void* c, size_t n) {
  float* fa = (float*)a;
  float* fb = (float*)b;
  float* fc = (float*)c;
  for (size_t i = 0; i < n; ++i) {
    fa[i] = (float)(i % 1024) * 0.001f;
    fb[i] = (float)(i % 251) * 0.002f;
    fc[i] = (float)(i % 127) * 0.003f;
  }
}

static void fill_int_3in(void* a, void* b, void* c, size_t n) {
  int* ia = (int*)a;
  int* ib = (int*)b;
  int* ic = (int*)c;
  for (size_t i = 0; i < n; ++i) {
    ia[i] = (int)(i % 65521);
    ib[i] = (int)((i + 17) % 32749);
    ic[i] = (int)(i % 8191);
  }
}

static int run_float_kernel(const hwb_context* ctx,
                            hwb_benchmark_result* out,
                            const char* id,
                            const char* variant,
                            const char* kernel_name,
                            int input_count,
                            double flops_per_elem,
                            void (*fill_fn)(void*, void*, void*, size_t)) {
  hwb_opencl_env env;
  hwb_opencl_buffers bufs;
  int rc;

  memset(out, 0, sizeof(*out));
  out->id = id;
  out->category = "gpu";
  out->variant = variant;
  out->unit = "GFLOP/s";
  out->threads = 1;
  out->class_kind = HWB_BENCH_CLASS_COMPONENT;
  out->synthetic = true;

  if (hwb_opencl_init(&env) != 0) {
    return -2;
  }

  if (hwb_opencl_setup_buffers(&env, &bufs, kernel_name, HWB_GPU_ELEMS,
                               (int)sizeof(float), input_count, fill_fn) != 0) {
    hwb_opencl_shutdown(&env);
    return -1;
  }

  rc = hwb_opencl_run_sampled(&env, &bufs, ctx, out, flops_per_elem);

  hwb_opencl_cleanup(&bufs);
  hwb_opencl_shutdown(&env);
  return rc;
}

static int gpu_vec_add_run(const hwb_context* ctx, hwb_benchmark_result* out) {
  return run_float_kernel(ctx, out,
                          "gpu.compute.fp32_vec_add",
                          "opencl",
                          "vec_add",
                          2,
                          1.0,
                          fill_float_2in);
}

static int gpu_fma_run(const hwb_context* ctx, hwb_benchmark_result* out) {
  return run_float_kernel(ctx, out,
                          "gpu.compute.fp32_fma",
                          "opencl",
                          "fma3",
                          3,
                          2.0,
                          fill_float_3in);
}

static int gpu_int_mad_run(const hwb_context* ctx, hwb_benchmark_result* out) {
  hwb_opencl_env env;
  hwb_opencl_buffers bufs;
  int rc;

  memset(out, 0, sizeof(*out));
  out->id = "gpu.compute.i32_mad";
  out->category = "gpu";
  out->variant = "opencl";
  out->unit = "Gops/s";
  out->threads = 1;
  out->class_kind = HWB_BENCH_CLASS_COMPONENT;
  out->synthetic = true;

  if (hwb_opencl_init(&env) != 0) {
    return -2;
  }

  if (hwb_opencl_setup_buffers(&env, &bufs, "int_mad", HWB_GPU_ELEMS,
                               (int)sizeof(int), 3, fill_int_3in) != 0) {
    hwb_opencl_shutdown(&env);
    return -1;
  }

  rc = hwb_opencl_run_sampled(&env, &bufs, ctx, out, 2.0);

  hwb_opencl_cleanup(&bufs);
  hwb_opencl_shutdown(&env);
  return rc;
}

int hwb_run_gpu_stress(int seconds, hwb_benchmark_result* out) {
  if (!out) {
    return -1;
  }

  int duration = hwb_stress_resolve_seconds(seconds);
  if (duration <= 0) {
    return -1;
  }

  hwb_opencl_env env;
  hwb_opencl_buffers bufs;
  int rc = -1;

  memset(out, 0, sizeof(*out));
  out->id = "stress.gpu";
  out->category = "stress";
  out->variant = "opencl";
  out->unit = "GFLOP/s";
  out->threads = 1;
  out->class_kind = HWB_BENCH_CLASS_SCENARIO;
  out->synthetic = true;

  if (hwb_opencl_init(&env) != 0) {
    return -2;
  }

  if (hwb_opencl_setup_buffers(&env, &bufs, "fma3", HWB_GPU_STRESS_ELEMS,
                               (int)sizeof(float), 3, fill_float_3in) != 0) {
    hwb_opencl_shutdown(&env);
    return -1;
  }

  cl_int err = CL_SUCCESS;
  size_t global = bufs.elem_count;
  double start = hwb_now_seconds();
  double end_time = start + (double)duration;
  double flops_accum = 0.0;

  while (hwb_now_seconds() < end_time) {
    err = clEnqueueNDRangeKernel(env.queue, bufs.kernel, 1, NULL, &global, NULL, 0, NULL, NULL);
    if (err != CL_SUCCESS) goto cleanup;
    err = clFinish(env.queue);
    if (err != CL_SUCCESS) goto cleanup;
    flops_accum += ((double)bufs.elem_count * 2.0);
#if defined(_WIN32)
    SwitchToThread();
#else
    sched_yield();
#endif
  }

  double elapsed = hwb_now_seconds() - start;
  if (elapsed <= 0.0) goto cleanup;

  out->samples[out->sample_count++] = (flops_accum / elapsed) / 1e9;
  out->measured_ms = elapsed * 1000.0;
  out->warmup_ms = 0.0;

  rc = hwb_compute_stats(out->samples, out->sample_count, &out->summary);

cleanup:
  hwb_opencl_cleanup(&bufs);
  hwb_opencl_shutdown(&env);
  return rc;
}

#else

static bool gpu_compute_supported(const hwb_context* ctx) {
  (void)ctx;
  return false;
}

static int gpu_vec_add_run(const hwb_context* ctx, hwb_benchmark_result* out) {
  (void)ctx;
  (void)out;
  return -2;
}

static int gpu_fma_run(const hwb_context* ctx, hwb_benchmark_result* out) {
  (void)ctx;
  (void)out;
  return -2;
}

static int gpu_int_mad_run(const hwb_context* ctx, hwb_benchmark_result* out) {
  (void)ctx;
  (void)out;
  return -2;
}

int hwb_run_gpu_stress(int seconds, hwb_benchmark_result* out) {
  (void)seconds;
  (void)out;
  return -2;
}

#endif

const hwb_benchmark_desc hwb_bench_gpu_compute_fp32_vec_add = {
  .id = "gpu.compute.fp32_vec_add",
  .category = "gpu",
  .name = "GPU FP32 vector add throughput",
  .unit = "GFLOP/s",
  .variant = "opencl",
  .class_kind = HWB_BENCH_CLASS_COMPONENT,
  .synthetic = true,
  .is_supported = gpu_compute_supported,
  .run = gpu_vec_add_run,
};

const hwb_benchmark_desc hwb_bench_gpu_compute_fp32_fma = {
  .id = "gpu.compute.fp32_fma",
  .category = "gpu",
  .name = "GPU FP32 fused multiply-add throughput",
  .unit = "GFLOP/s",
  .variant = "opencl",
  .class_kind = HWB_BENCH_CLASS_COMPONENT,
  .synthetic = true,
  .is_supported = gpu_compute_supported,
  .run = gpu_fma_run,
};

const hwb_benchmark_desc hwb_bench_gpu_compute_i32_mad = {
  .id = "gpu.compute.i32_mad",
  .category = "gpu",
  .name = "GPU INT32 multiply-add throughput",
  .unit = "Gops/s",
  .variant = "opencl",
  .class_kind = HWB_BENCH_CLASS_COMPONENT,
  .synthetic = true,
  .is_supported = gpu_compute_supported,
  .run = gpu_int_mad_run,
};

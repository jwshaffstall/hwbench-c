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

static int run_float_kernel(const hwb_context* ctx,
                            hwb_benchmark_result* out,
                            const char* id,
                            const char* variant,
                            const char* kernel_name,
                            bool use_c_input,
                            double flops_per_elem) {
  hwb_opencl_env env;
  cl_int err = CL_SUCCESS;
  cl_mem buf_a = NULL;
  cl_mem buf_b = NULL;
  cl_mem buf_c = NULL;
  cl_mem buf_out = NULL;
  cl_kernel kernel = NULL;
  float* a = NULL;
  float* b = NULL;
  float* c = NULL;
  int rc = -1;

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

  a = (float*)malloc(HWB_GPU_ELEMS * sizeof(float));
  b = (float*)malloc(HWB_GPU_ELEMS * sizeof(float));
  if (use_c_input) {
    c = (float*)malloc(HWB_GPU_ELEMS * sizeof(float));
  }
  if (!a || !b || (use_c_input && !c)) {
    rc = -1;
    goto cleanup;
  }

  for (size_t i = 0; i < HWB_GPU_ELEMS; ++i) {
    a[i] = (float)(i % 1024) * 0.001f;
    b[i] = (float)(i % 251) * 0.002f;
    if (use_c_input) {
      c[i] = (float)(i % 127) * 0.003f;
    }
  }

  buf_a = clCreateBuffer(env.context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                         HWB_GPU_ELEMS * sizeof(float), a, &err);
  if (!buf_a || err != CL_SUCCESS) goto cleanup;
  buf_b = clCreateBuffer(env.context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                         HWB_GPU_ELEMS * sizeof(float), b, &err);
  if (!buf_b || err != CL_SUCCESS) goto cleanup;
  if (use_c_input) {
    buf_c = clCreateBuffer(env.context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                           HWB_GPU_ELEMS * sizeof(float), c, &err);
    if (!buf_c || err != CL_SUCCESS) goto cleanup;
  }
  buf_out = clCreateBuffer(env.context, CL_MEM_WRITE_ONLY,
                           HWB_GPU_ELEMS * sizeof(float), NULL, &err);
  if (!buf_out || err != CL_SUCCESS) goto cleanup;

  kernel = clCreateKernel(env.program, kernel_name, &err);
  if (!kernel || err != CL_SUCCESS) goto cleanup;

  unsigned arg_index = 0;
  err = clSetKernelArg(kernel, arg_index++, sizeof(cl_mem), &buf_a);
  if (err != CL_SUCCESS) goto cleanup;
  err = clSetKernelArg(kernel, arg_index++, sizeof(cl_mem), &buf_b);
  if (err != CL_SUCCESS) goto cleanup;
  if (use_c_input) {
    err = clSetKernelArg(kernel, arg_index++, sizeof(cl_mem), &buf_c);
    if (err != CL_SUCCESS) goto cleanup;
  }
  err = clSetKernelArg(kernel, arg_index++, sizeof(cl_mem), &buf_out);
  if (err != CL_SUCCESS) goto cleanup;

  size_t global = HWB_GPU_ELEMS;

  double warmup_start = hwb_now_seconds();
  while ((hwb_now_seconds() - warmup_start) * 1000.0 < (double)ctx->warmup_ms) {
    err = clEnqueueNDRangeKernel(env.queue, kernel, 1, NULL, &global, NULL, 0, NULL, NULL);
    if (err != CL_SUCCESS) goto cleanup;
    err = clFinish(env.queue);
    if (err != CL_SUCCESS) goto cleanup;
  }
  out->warmup_ms = (hwb_now_seconds() - warmup_start) * 1000.0;

  double measured_start = hwb_now_seconds();
  for (int s = 0; s < ctx->samples && s < HWB_MAX_SAMPLES; ++s) {
    double t0 = hwb_now_seconds();
    err = clEnqueueNDRangeKernel(env.queue, kernel, 1, NULL, &global, NULL, 0, NULL, NULL);
    if (err != CL_SUCCESS) goto cleanup;
    err = clFinish(env.queue);
    if (err != CL_SUCCESS) goto cleanup;
    double t1 = hwb_now_seconds();
    out->samples[out->sample_count++] = ((double)HWB_GPU_ELEMS * flops_per_elem) / (t1 - t0) / 1e9;
  }
  out->measured_ms = (hwb_now_seconds() - measured_start) * 1000.0;

  rc = hwb_compute_stats(out->samples, out->sample_count, &out->summary);

cleanup:
  if (kernel) clReleaseKernel(kernel);
  if (buf_out) clReleaseMemObject(buf_out);
  if (buf_c) clReleaseMemObject(buf_c);
  if (buf_b) clReleaseMemObject(buf_b);
  if (buf_a) clReleaseMemObject(buf_a);
  free(c);
  free(b);
  free(a);
  hwb_opencl_shutdown(&env);

  return rc;
}

static int gpu_vec_add_run(const hwb_context* ctx, hwb_benchmark_result* out) {
  return run_float_kernel(ctx, out,
                          "gpu.compute.fp32_vec_add",
                          "opencl",
                          "vec_add",
                          false,
                          1.0);
}

static int gpu_fma_run(const hwb_context* ctx, hwb_benchmark_result* out) {
  return run_float_kernel(ctx, out,
                          "gpu.compute.fp32_fma",
                          "opencl",
                          "fma3",
                          true,
                          2.0);
}

static int gpu_int_mad_run(const hwb_context* ctx, hwb_benchmark_result* out) {
  hwb_opencl_env env;
  cl_int err = CL_SUCCESS;
  cl_mem buf_a = NULL;
  cl_mem buf_b = NULL;
  cl_mem buf_c = NULL;
  cl_mem buf_out = NULL;
  cl_kernel kernel = NULL;
  int* a = NULL;
  int* b = NULL;
  int* c = NULL;
  int rc = -1;

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

  a = (int*)malloc(HWB_GPU_ELEMS * sizeof(int));
  b = (int*)malloc(HWB_GPU_ELEMS * sizeof(int));
  c = (int*)malloc(HWB_GPU_ELEMS * sizeof(int));
  if (!a || !b || !c) goto cleanup;

  for (size_t i = 0; i < HWB_GPU_ELEMS; ++i) {
    a[i] = (int)(i % 65521);
    b[i] = (int)((i + 17) % 32749);
    c[i] = (int)(i % 8191);
  }

  buf_a = clCreateBuffer(env.context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                         HWB_GPU_ELEMS * sizeof(int), a, &err);
  if (!buf_a || err != CL_SUCCESS) goto cleanup;
  buf_b = clCreateBuffer(env.context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                         HWB_GPU_ELEMS * sizeof(int), b, &err);
  if (!buf_b || err != CL_SUCCESS) goto cleanup;
  buf_c = clCreateBuffer(env.context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                         HWB_GPU_ELEMS * sizeof(int), c, &err);
  if (!buf_c || err != CL_SUCCESS) goto cleanup;
  buf_out = clCreateBuffer(env.context, CL_MEM_WRITE_ONLY,
                           HWB_GPU_ELEMS * sizeof(int), NULL, &err);
  if (!buf_out || err != CL_SUCCESS) goto cleanup;

  kernel = clCreateKernel(env.program, "int_mad", &err);
  if (!kernel || err != CL_SUCCESS) goto cleanup;

  err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &buf_a);
  if (err != CL_SUCCESS) goto cleanup;
  err = clSetKernelArg(kernel, 1, sizeof(cl_mem), &buf_b);
  if (err != CL_SUCCESS) goto cleanup;
  err = clSetKernelArg(kernel, 2, sizeof(cl_mem), &buf_c);
  if (err != CL_SUCCESS) goto cleanup;
  err = clSetKernelArg(kernel, 3, sizeof(cl_mem), &buf_out);
  if (err != CL_SUCCESS) goto cleanup;

  size_t global = HWB_GPU_ELEMS;

  double warmup_start = hwb_now_seconds();
  while ((hwb_now_seconds() - warmup_start) * 1000.0 < (double)ctx->warmup_ms) {
    err = clEnqueueNDRangeKernel(env.queue, kernel, 1, NULL, &global, NULL, 0, NULL, NULL);
    if (err != CL_SUCCESS) goto cleanup;
    err = clFinish(env.queue);
    if (err != CL_SUCCESS) goto cleanup;
  }
  out->warmup_ms = (hwb_now_seconds() - warmup_start) * 1000.0;

  double measured_start = hwb_now_seconds();
  for (int s = 0; s < ctx->samples && s < HWB_MAX_SAMPLES; ++s) {
    double t0 = hwb_now_seconds();
    err = clEnqueueNDRangeKernel(env.queue, kernel, 1, NULL, &global, NULL, 0, NULL, NULL);
    if (err != CL_SUCCESS) goto cleanup;
    err = clFinish(env.queue);
    if (err != CL_SUCCESS) goto cleanup;
    double t1 = hwb_now_seconds();
    out->samples[out->sample_count++] = ((double)HWB_GPU_ELEMS * 2.0) / (t1 - t0) / 1e9;
  }
  out->measured_ms = (hwb_now_seconds() - measured_start) * 1000.0;

  rc = hwb_compute_stats(out->samples, out->sample_count, &out->summary);

cleanup:
  if (kernel) clReleaseKernel(kernel);
  if (buf_out) clReleaseMemObject(buf_out);
  if (buf_c) clReleaseMemObject(buf_c);
  if (buf_b) clReleaseMemObject(buf_b);
  if (buf_a) clReleaseMemObject(buf_a);
  free(c);
  free(b);
  free(a);
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
  cl_int err = CL_SUCCESS;
  cl_mem buf_a = NULL;
  cl_mem buf_b = NULL;
  cl_mem buf_c = NULL;
  cl_mem buf_out = NULL;
  cl_kernel kernel = NULL;
  float* a = NULL;
  float* b = NULL;
  float* c = NULL;
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

  a = (float*)malloc(HWB_GPU_STRESS_ELEMS * sizeof(float));
  b = (float*)malloc(HWB_GPU_STRESS_ELEMS * sizeof(float));
  c = (float*)malloc(HWB_GPU_STRESS_ELEMS * sizeof(float));
  if (!a || !b || !c) goto cleanup;

  for (size_t i = 0; i < HWB_GPU_STRESS_ELEMS; ++i) {
    a[i] = (float)(i % 1024) * 0.001f;
    b[i] = (float)(i % 251) * 0.002f;
    c[i] = (float)(i % 127) * 0.003f;
  }

  buf_a = clCreateBuffer(env.context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                         HWB_GPU_STRESS_ELEMS * sizeof(float), a, &err);
  if (!buf_a || err != CL_SUCCESS) goto cleanup;
  buf_b = clCreateBuffer(env.context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                         HWB_GPU_STRESS_ELEMS * sizeof(float), b, &err);
  if (!buf_b || err != CL_SUCCESS) goto cleanup;
  buf_c = clCreateBuffer(env.context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                         HWB_GPU_STRESS_ELEMS * sizeof(float), c, &err);
  if (!buf_c || err != CL_SUCCESS) goto cleanup;
  buf_out = clCreateBuffer(env.context, CL_MEM_WRITE_ONLY,
                           HWB_GPU_STRESS_ELEMS * sizeof(float), NULL, &err);
  if (!buf_out || err != CL_SUCCESS) goto cleanup;

  kernel = clCreateKernel(env.program, "fma3", &err);
  if (!kernel || err != CL_SUCCESS) goto cleanup;

  err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &buf_a);
  if (err != CL_SUCCESS) goto cleanup;
  err = clSetKernelArg(kernel, 1, sizeof(cl_mem), &buf_b);
  if (err != CL_SUCCESS) goto cleanup;
  err = clSetKernelArg(kernel, 2, sizeof(cl_mem), &buf_c);
  if (err != CL_SUCCESS) goto cleanup;
  err = clSetKernelArg(kernel, 3, sizeof(cl_mem), &buf_out);
  if (err != CL_SUCCESS) goto cleanup;

  size_t global = HWB_GPU_STRESS_ELEMS;
  double start = hwb_now_seconds();
  double end_time = start + (double)duration;
  double flops_accum = 0.0;

  while (hwb_now_seconds() < end_time) {
    err = clEnqueueNDRangeKernel(env.queue, kernel, 1, NULL, &global, NULL, 0, NULL, NULL);
    if (err != CL_SUCCESS) goto cleanup;
    err = clFinish(env.queue);
    if (err != CL_SUCCESS) goto cleanup;
    flops_accum += ((double)HWB_GPU_STRESS_ELEMS * 2.0);
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
  if (kernel) clReleaseKernel(kernel);
  if (buf_out) clReleaseMemObject(buf_out);
  if (buf_c) clReleaseMemObject(buf_c);
  if (buf_b) clReleaseMemObject(buf_b);
  if (buf_a) clReleaseMemObject(buf_a);
  free(c);
  free(b);
  free(a);
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

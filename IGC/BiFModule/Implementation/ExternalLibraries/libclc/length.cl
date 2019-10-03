/*
 * Copyright (c) 2014 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "../../include/BiF_Definitions.cl"
#include "../../../Headers/spirv.h"

float __builtin_spirv_OpenCL_length_f32(float p) {
  return __builtin_spirv_OpenCL_fabs_f32(p);
}

float __builtin_spirv_OpenCL_length_v2f32(float2 p) {
  float l2 = __builtin_spirv_OpDot_v2f32_v2f32(p, p);
  /*Currently we are not optimizing away the
    underflow case when fast-relaxed-math is enabled*/
  if (l2 < FLT_MIN) {
    p *= 0x1.0p+86F;
    return __builtin_spirv_OpenCL_sqrt_f32(__builtin_spirv_OpDot_v2f32_v2f32(p, p)) * 0x1.0p-86F;
  } else if (__intel_relaxed_isinf(l2)) {
    p *= 0x1.0p-65F;
    return __builtin_spirv_OpenCL_sqrt_f32(__builtin_spirv_OpDot_v2f32_v2f32(p, p)) * 0x1.0p+65F;
  }
  return __builtin_spirv_OpenCL_sqrt_f32(l2);
}

float __builtin_spirv_OpenCL_length_v3f32(float3 p) {
  float l2 = __builtin_spirv_OpDot_v3f32_v3f32(p, p);
  if (l2 < FLT_MIN) {
    p *= 0x1.0p+86F;
    return __builtin_spirv_OpenCL_sqrt_f32(__builtin_spirv_OpDot_v3f32_v3f32(p, p)) * 0x1.0p-86F;
  } else if (__intel_relaxed_isinf(l2)) {
    p *= 0x1.0p-65F;
    return __builtin_spirv_OpenCL_sqrt_f32(__builtin_spirv_OpDot_v3f32_v3f32(p, p)) * 0x1.0p+65F;
  }
  return __builtin_spirv_OpenCL_sqrt_f32(l2);
}

float __builtin_spirv_OpenCL_length_v4f32(float4 p) {
  float l2 = __builtin_spirv_OpDot_v4f32_v4f32(p, p);
  if (l2 < FLT_MIN) {
    p *= 0x1.0p+86F;
    return __builtin_spirv_OpenCL_sqrt_f32(__builtin_spirv_OpDot_v4f32_v4f32(p, p)) * 0x1.0p-86F;
  } else if (__intel_relaxed_isinf(l2)) {
    p *= 0x1.0p-65F;
    return __builtin_spirv_OpenCL_sqrt_f32(__builtin_spirv_OpDot_v4f32_v4f32(p, p)) * 0x1.0p+65F;
  }
  return __builtin_spirv_OpenCL_sqrt_f32(l2);
}

#ifdef cl_khr_fp64
#pragma OPENCL EXTENSION cl_khr_fp64 : enable

double __builtin_spirv_OpenCL_length_f64(double p){
  return __builtin_spirv_OpenCL_fabs_f64(p);
}

double __builtin_spirv_OpenCL_length_v2f64(double2 p) {
  double l2 = __builtin_spirv_OpDot_v2f64_v2f64(p, p);
  if (l2 < DBL_MIN) {
      p *= 0x1.0p+563;
      return __builtin_spirv_OpenCL_sqrt_f64(__builtin_spirv_OpDot_v2f64_v2f64(p, p)) * 0x1.0p-563;
  } else if (__intel_relaxed_isinf(l2)) {
      p *= 0x1.0p-513;
      return __builtin_spirv_OpenCL_sqrt_f64(__builtin_spirv_OpDot_v2f64_v2f64(p, p)) * 0x1.0p+513;
  }
  return __builtin_spirv_OpenCL_sqrt_f64(l2);
}

double __builtin_spirv_OpenCL_length_v3f64(double3 p) {
  double l2 = __builtin_spirv_OpDot_v3f64_v3f64(p, p);
  if (l2 < DBL_MIN) {
      p *= 0x1.0p+563;
      return __builtin_spirv_OpenCL_sqrt_f64(__builtin_spirv_OpDot_v3f64_v3f64(p, p)) * 0x1.0p-563;
  } else if (__intel_relaxed_isinf(l2)) {
      p *= 0x1.0p-513;
      return __builtin_spirv_OpenCL_sqrt_f64(__builtin_spirv_OpDot_v3f64_v3f64(p, p)) * 0x1.0p+513;
  }
  return __builtin_spirv_OpenCL_sqrt_f64(l2);
}

double __builtin_spirv_OpenCL_length_v4f64(double4 p) {
  double l2 = __builtin_spirv_OpDot_v4f64_v4f64(p, p);
  if (l2 < DBL_MIN) {
      p *= 0x1.0p+563;
      return __builtin_spirv_OpenCL_sqrt_f64(__builtin_spirv_OpDot_v4f64_v4f64(p, p)) * 0x1.0p-563;
  } else if (__intel_relaxed_isinf(l2)) {
      p *= 0x1.0p-513;
      return __builtin_spirv_OpenCL_sqrt_f64(__builtin_spirv_OpDot_v4f64_v4f64(p, p)) * 0x1.0p+513;
  }
  return __builtin_spirv_OpenCL_sqrt_f64(l2);
}

#endif

#ifdef cl_khr_fp16
#pragma OPENCL EXTENSION cl_khr_fp16 : enable

half __builtin_spirv_OpenCL_length_f16(half p){
  return __builtin_spirv_OpenCL_fabs_f16(p);
}

half __builtin_spirv_OpenCL_length_v2f16(half2 p) {
  half l2 = __builtin_spirv_OpDot_v2f16_v2f16(p, p);
  if (l2 < HALF_MIN) {
      p *= HALF_MAX_SQRT;
      return __builtin_spirv_OpenCL_sqrt_f16(__builtin_spirv_OpDot_v2f16_v2f16(p, p)) * HALF_MIN_SQRT;
  } else if (__intel_relaxed_isinf(l2)) {
      p *= HALF_MIN_SQRT;
      return __builtin_spirv_OpenCL_sqrt_f16(__builtin_spirv_OpDot_v2f16_v2f16(p, p)) * HALF_MAX_SQRT;
  }
  return __builtin_spirv_OpenCL_sqrt_f16(l2);
}

half __builtin_spirv_OpenCL_length_v3f16(half3 p) {
  half l2 = __builtin_spirv_OpDot_v3f16_v3f16(p, p);
  if (l2 < HALF_MIN) {
      p *= HALF_MAX_SQRT;
      return __builtin_spirv_OpenCL_sqrt_f16(__builtin_spirv_OpDot_v3f16_v3f16(p, p)) * HALF_MIN_SQRT;
  } else if (__intel_relaxed_isinf(l2)) {
      p *= HALF_MIN_SQRT;
      return __builtin_spirv_OpenCL_sqrt_f16(__builtin_spirv_OpDot_v3f16_v3f16(p, p)) *HALF_MAX_SQRT;
  }
  return __builtin_spirv_OpenCL_sqrt_f16(l2);
}

half __builtin_spirv_OpenCL_length_v4f16(half4 p) {
  half l2 = __builtin_spirv_OpDot_v4f16_v4f16(p, p);
  if (l2 < HALF_MIN) {
      p *= HALF_MAX_SQRT;
      return __builtin_spirv_OpenCL_sqrt_f16(__builtin_spirv_OpDot_v4f16_v4f16(p, p)) * HALF_MIN_SQRT;
  } else if (__intel_relaxed_isinf(l2)) {
      p *= HALF_MIN_SQRT;
      return __builtin_spirv_OpenCL_sqrt_f16(__builtin_spirv_OpDot_v4f16_v4f16(p, p)) * HALF_MAX_SQRT;
  }
  return __builtin_spirv_OpenCL_sqrt_f16(l2);
}

#endif

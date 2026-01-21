/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/*========================== begin_copyright_notice ============================

Copyright (c) 2014 Advanced Micro Devices, Inc.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

============================= end_copyright_notice ===========================*/

#include "../../include/BiF_Definitions.cl"
#include "../../../Headers/spirv.h"

float __attribute__((overloadable)) __spirv_ocl_length(float p) {
  return __spirv_ocl_fabs(p);
}

float __attribute__((overloadable)) __spirv_ocl_length(float2 p) {
  float l2 = __spirv_Dot(p, p);
  /*Currently we are not optimizing away the
    underflow case when fast-relaxed-math is enabled*/
  if (l2 < FLT_MIN) {
    p *= 0x1.0p+86F;
    return __spirv_ocl_sqrt(__spirv_Dot(p, p)) * 0x1.0p-86F;
  } else if (__intel_relaxed_isinf(l2)) {
    p *= 0x1.0p-65F;
    return __spirv_ocl_sqrt(__spirv_Dot(p, p)) * 0x1.0p+65F;
  }
  return __spirv_ocl_sqrt(l2);
}

float __attribute__((overloadable)) __spirv_ocl_length(float3 p) {
  float l2 = __spirv_Dot(p, p);
  if (l2 < FLT_MIN) {
    p *= 0x1.0p+86F;
    return __spirv_ocl_sqrt(__spirv_Dot(p, p)) * 0x1.0p-86F;
  } else if (__intel_relaxed_isinf(l2)) {
    p *= 0x1.0p-65F;
    return __spirv_ocl_sqrt(__spirv_Dot(p, p)) * 0x1.0p+65F;
  }
  return __spirv_ocl_sqrt(l2);
}

float __attribute__((overloadable)) __spirv_ocl_length(float4 p) {
  float l2 = __spirv_Dot(p, p);
  if (l2 < FLT_MIN) {
    p *= 0x1.0p+86F;
    return __spirv_ocl_sqrt(__spirv_Dot(p, p)) * 0x1.0p-86F;
  } else if (__intel_relaxed_isinf(l2)) {
    p *= 0x1.0p-65F;
    return __spirv_ocl_sqrt(__spirv_Dot(p, p)) * 0x1.0p+65F;
  }
  return __spirv_ocl_sqrt(l2);
}

#ifdef cl_khr_fp64
#pragma OPENCL EXTENSION cl_khr_fp64 : enable

double __attribute__((overloadable)) __spirv_ocl_length(double p){
  return __spirv_ocl_fabs(p);
}

double __attribute__((overloadable)) __spirv_ocl_length(double2 p) {
  double l2 = __spirv_Dot(p, p);
  if (l2 < DBL_MIN) {
      p *= 0x1.0p+563;
      return __spirv_ocl_sqrt(__spirv_Dot(p, p)) * 0x1.0p-563;
  } else if (__intel_relaxed_isinf(l2)) {
      p *= 0x1.0p-513;
      return __spirv_ocl_sqrt(__spirv_Dot(p, p)) * 0x1.0p+513;
  }
  return __spirv_ocl_sqrt(l2);
}

double __attribute__((overloadable)) __spirv_ocl_length(double3 p) {
  double l2 = __spirv_Dot(p, p);
  if (l2 < DBL_MIN) {
      p *= 0x1.0p+563;
      return __spirv_ocl_sqrt(__spirv_Dot(p, p)) * 0x1.0p-563;
  } else if (__intel_relaxed_isinf(l2)) {
      p *= 0x1.0p-513;
      return __spirv_ocl_sqrt(__spirv_Dot(p, p)) * 0x1.0p+513;
  }
  return __spirv_ocl_sqrt(l2);
}

double __attribute__((overloadable)) __spirv_ocl_length(double4 p) {
  double l2 = __spirv_Dot(p, p);
  if (l2 < DBL_MIN) {
      p *= 0x1.0p+563;
      return __spirv_ocl_sqrt(__spirv_Dot(p, p)) * 0x1.0p-563;
  } else if (__intel_relaxed_isinf(l2)) {
      p *= 0x1.0p-513;
      return __spirv_ocl_sqrt(__spirv_Dot(p, p)) * 0x1.0p+513;
  }
  return __spirv_ocl_sqrt(l2);
}

#endif

#ifdef cl_khr_fp16
#pragma OPENCL EXTENSION cl_khr_fp16 : enable

half __attribute__((overloadable)) __spirv_ocl_length(half p){
  return __spirv_ocl_fabs(p);
}

half __attribute__((overloadable)) __spirv_ocl_length(half2 p) {
  half l2 = __spirv_Dot(p, p);
  if (l2 < HALF_MIN) {
      p *= HALF_MAX_SQRT;
      return __spirv_ocl_sqrt(__spirv_Dot(p, p)) * HALF_MIN_SQRT;
  } else if (__intel_relaxed_isinf(l2)) {
      p *= HALF_MIN_SQRT;
      return __spirv_ocl_sqrt(__spirv_Dot(p, p)) * HALF_MAX_SQRT;
  }
  return __spirv_ocl_sqrt(l2);
}

half __attribute__((overloadable)) __spirv_ocl_length(half3 p) {
  half l2 = __spirv_Dot(p, p);
  if (l2 < HALF_MIN) {
      p *= HALF_MAX_SQRT;
      return __spirv_ocl_sqrt(__spirv_Dot(p, p)) * HALF_MIN_SQRT;
  } else if (__intel_relaxed_isinf(l2)) {
      p *= HALF_MIN_SQRT;
      return __spirv_ocl_sqrt(__spirv_Dot(p, p)) *HALF_MAX_SQRT;
  }
  return __spirv_ocl_sqrt(l2);
}

half __attribute__((overloadable)) __spirv_ocl_length(half4 p) {
  half l2 = __spirv_Dot(p, p);
  if (l2 < HALF_MIN) {
      p *= HALF_MAX_SQRT;
      return __spirv_ocl_sqrt(__spirv_Dot(p, p)) * HALF_MIN_SQRT;
  } else if (__intel_relaxed_isinf(l2)) {
      p *= HALF_MIN_SQRT;
      return __spirv_ocl_sqrt(__spirv_Dot(p, p)) * HALF_MAX_SQRT;
  }
  return __spirv_ocl_sqrt(l2);
}

#endif


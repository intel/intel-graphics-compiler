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

static int2 INLINE OVERLOADABLE __convert_int2(__bool2 b)
{
    return -(int2)(b.x, b.y);
}

static int3 INLINE OVERLOADABLE __convert_int3(__bool3 b)
{
    return -(int3)(b.x, b.y, b.z);
}

static int4 INLINE OVERLOADABLE __convert_int4(__bool4 b)
{
    return -(int4)(b.x, b.y, b.z, b.w);
}

static int8 INLINE OVERLOADABLE __convert_int8(__bool8 b)
{
    return (int8)(__convert_int4(b.lo), __convert_int4(b.hi));
}

static int16 INLINE OVERLOADABLE __convert_int16(__bool16 b)
{
    return (int16)(__convert_int8(b.lo), __convert_int8(b.hi));
}

#if defined(cl_khr_fp64)

static long2 INLINE OVERLOADABLE __convert_long2(__bool2 b)
{
    return -(long2)(b.x, b.y);
}

static long3 INLINE OVERLOADABLE __convert_long3(__bool3 b)
{
    return -(long3)(b.x, b.y, b.z);
}

static long4 INLINE OVERLOADABLE __convert_long4(__bool4 b)
{
    return -(long4)(b.x, b.y, b.z, b.w);
}

static long8 INLINE OVERLOADABLE __convert_long8(__bool8 b)
{
    return (long8)(__convert_long4(b.lo), __convert_long4(b.hi));
}

static long16 INLINE OVERLOADABLE __convert_long16(__bool16 b)
{
    return (long16)(__convert_long8(b.lo), __convert_long8(b.hi));
}

#endif

static short2 INLINE OVERLOADABLE __convert_short2(__bool2 b)
{
    return -(short2)(b.x, b.y);
}

static short3 INLINE OVERLOADABLE __convert_short3(__bool3 b)
{
    return -(short3)(b.x, b.y, b.z);
}

static short4 INLINE OVERLOADABLE __convert_short4(__bool4 b)
{
    return -(short4)(b.x, b.y, b.z, b.w);
}

static short8 INLINE OVERLOADABLE __convert_short8(__bool8 b)
{
    return (short8)(__convert_short4(b.lo), __convert_short4(b.hi));
}

static short16 INLINE OVERLOADABLE __convert_short16(__bool16 b)
{
    return (short16)(__convert_short8(b.lo), __convert_short8(b.hi));
}

float __attribute__((overloadable)) __spirv_ocl_normalize(float p) {
  return __spirv_ocl_sign(p);
}

float2 __attribute__((overloadable)) __spirv_ocl_normalize(float2 p) {
  if (all(p == (float2)0.0F))
    return p;

  float l2 = __spirv_Dot(p, p);

  /*Currently we are not optimizing away the
    underflow case when fast-relaxed-math is enabled*/
  if (l2 < FLT_MIN) {
    p *= 0x1.0p+86F;
    l2 = __spirv_Dot(p, p);
  } else if (__intel_relaxed_isinf(l2)) {
    p *= 0x1.0p-65f;
    l2 = __spirv_Dot(p, p);
    if (l2 == INFINITY) {
      p = __spirv_ocl_copysign(__spirv_ocl_select((float2)0.0F, (float2)1.0F, __convert_int2(__spirv_IsInf(p))), p);
      l2 = __spirv_Dot(p, p);
    }
  }
  return p * __spirv_ocl_rsqrt(l2);
}

float3 __attribute__((overloadable)) __spirv_ocl_normalize(float3 p) {
  if (all(p == (float3)0.0F))
    return p;

  float l2 = __spirv_Dot(p, p);

  if (l2 < FLT_MIN) {
    p *= 0x1.0p+86F;
    l2 = __spirv_Dot(p, p);
  } else if (__intel_relaxed_isinf(l2)) {
    p *= 0x1.0p-66f;
    l2 = __spirv_Dot(p, p);
    if (l2 == INFINITY) {
      p = __spirv_ocl_copysign(__spirv_ocl_select((float3)0.0F, (float3)1.0F, __convert_int3(__spirv_IsInf(p))), p);
      l2 = __spirv_Dot(p, p);
    }
  }
  return p * __spirv_ocl_rsqrt(l2);
}

float4 __attribute__((overloadable)) __spirv_ocl_normalize(float4 p) {
  if (all(p == (float4)0.0F))
    return p;

  float l2 = __spirv_Dot(p, p);

  if (l2 < FLT_MIN) {
    p *= 0x1.0p+86F;
    l2 = __spirv_Dot(p, p);
  } else if (__intel_relaxed_isinf(l2)) {
    p *= 0x1.0p-66f;
    l2 = __spirv_Dot(p, p);
    if (l2 == INFINITY) {
      p = __spirv_ocl_copysign(__spirv_ocl_select((float4)0.0F, (float4)1.0F, __convert_int4(__spirv_IsInf(p))), p);
      l2 = __spirv_Dot(p, p);
    }
  }
  return p * __spirv_ocl_rsqrt(l2);
}

#ifdef cl_khr_fp64

#pragma OPENCL EXTENSION cl_khr_fp64 : enable

double __attribute__((overloadable)) __spirv_ocl_normalize(double p) {
  return __spirv_ocl_sign(p);
}

double2 __attribute__((overloadable)) __spirv_ocl_normalize(double2 p) {
  if (all(p == (double2)0.0))
    return p;

  double l2 = __spirv_Dot(p, p);

  if (l2 < DBL_MIN) {
    p *= 0x1.0p+563;
    l2 = __spirv_Dot(p, p);
  } else if (__intel_relaxed_isinf(l2)) {
    p *= 0x1.0p-513;
    l2 = __spirv_Dot(p, p);
    if (l2 == INFINITY) {
      p = __spirv_ocl_copysign(__spirv_ocl_select((double2)0.0, (double2)1.0, __convert_long2(__spirv_IsInf(p))), p);
      l2 = __spirv_Dot(p, p);
    }
  }
  return p * __spirv_ocl_rsqrt(l2);
}

double3 __attribute__((overloadable)) __spirv_ocl_normalize(double3 p) {
  if (all(p == (double3)0.0))
    return p;

  double l2 = __spirv_Dot(p, p);

  if (l2 < DBL_MIN) {
    p *= 0x1.0p+563;
    l2 = __spirv_Dot(p, p);
  } else if (__intel_relaxed_isinf(l2)) {
    p *= 0x1.0p-514;
    l2 = __spirv_Dot(p, p);
    if (l2 == INFINITY) {
      p = __spirv_ocl_copysign(__spirv_ocl_select((double3)0.0, (double3)1.0, __convert_long3(__spirv_IsInf(p))), p);
      l2 = __spirv_Dot(p, p);
    }
  }
  return p * __spirv_ocl_rsqrt(l2);
}

double4 __attribute__((overloadable)) __spirv_ocl_normalize(double4 p) {
  if (all(p == (double4)0.0))
    return p;

  double l2 = __spirv_Dot(p, p);

  if (l2 < DBL_MIN) {
    p *= 0x1.0p+563;
    l2 = __spirv_Dot(p, p);
  } else if (__intel_relaxed_isinf(l2)) {
    p *= 0x1.0p-514;
    l2 = __spirv_Dot(p, p);
    if (l2 == INFINITY) {
      p = __spirv_ocl_copysign(__spirv_ocl_select((double4)0.0, (double4)1.0, __convert_long4(__spirv_IsInf(p))), p);
      l2 = __spirv_Dot(p, p);
    }
  }
  return p * __spirv_ocl_rsqrt(l2);
}

#endif

#if defined(cl_khr_fp16)

INLINE half __attribute__((overloadable)) __spirv_ocl_normalize(half p ){
    return __spirv_ocl_sign(p);
}

half2 __attribute__((overloadable)) __spirv_ocl_normalize(half2 p ){
    if (all(p == (half2)0.0F))
    return p;

  half l2 = __spirv_Dot(p, p);

  if (l2 < HALF_MIN) {
    p *= HALF_MAX_SQRT;
    l2 = __spirv_Dot(p, p);
  } else if (__intel_relaxed_isinf(l2)) {
    p *= HALF_MIN_SQRT;
    l2 = __spirv_Dot(p, p);
    if (l2 == INFINITY) {
      p = __spirv_ocl_copysign(__spirv_ocl_select((half2)0.0F, (half2)1.0F, __convert_short2(__spirv_IsInf(p))), p);
      l2 = __spirv_Dot(p, p);
    }
  }
  return p * __spirv_ocl_rsqrt(l2);
}

half3 __attribute__((overloadable)) __spirv_ocl_normalize(half3 p ){
    if (all(p == (half3)0.0F))
    return p;

  half l2 = __spirv_Dot(p, p);

  if (l2 < HALF_MIN) {
    p *= HALF_MAX_SQRT;
    l2 = __spirv_Dot(p, p);
  } else if (__intel_relaxed_isinf(l2)) {
    p *= HALF_MIN_SQRT;
    l2 = __spirv_Dot(p, p);
    if (l2 == INFINITY) {
      p = __spirv_ocl_copysign(__spirv_ocl_select((half3)0.0F, (half3)1.0F, __convert_short3(__spirv_IsInf(p))), p);
      l2 = __spirv_Dot(p, p);
    }
  }
  return p * __spirv_ocl_rsqrt(l2);
}

half4 __attribute__((overloadable)) __spirv_ocl_normalize(half4 p ){
    if (all(p == (half4)0.0F))
    return p;

  half l2 = __spirv_Dot(p, p);

  if (l2 < HALF_MIN) {
    p *= HALF_MAX_SQRT;
    l2 = __spirv_Dot(p, p);
  } else if (__intel_relaxed_isinf(l2)) {
    p *= HALF_MIN_SQRT;
    l2 = __spirv_Dot(p, p);
    if (l2 == INFINITY) {
      p = __spirv_ocl_copysign(__spirv_ocl_select((half4)0.0F, (half4)1.0F, __convert_short4(__spirv_IsInf(p))), p);
      l2 = __spirv_Dot(p, p);
    }
  }
  return p * __spirv_ocl_rsqrt(l2);
}

#endif // defined(cl_khr_fp16)


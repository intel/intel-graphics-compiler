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

static uint2 INLINE OVERLOADABLE __convert_uint2(__bool2 b)
{
    return as_uint2(-(int2)(b.x, b.y));
}

static uint3 INLINE OVERLOADABLE __convert_uint3(__bool3 b)
{
    return as_uint3(-(int3)(b.x, b.y, b.z));
}

static uint4 INLINE OVERLOADABLE __convert_uint4(__bool4 b)
{
    return as_uint4(-(int4)(b.x, b.y, b.z, b.w));
}

static uint8 INLINE OVERLOADABLE __convert_uint8(__bool8 b)
{
    return (uint8)(__convert_uint4(b.lo), __convert_uint4(b.hi));
}

static uint16 INLINE OVERLOADABLE __convert_uint16(__bool16 b)
{
    return (uint16)(__convert_uint8(b.lo), __convert_uint8(b.hi));
}

#if defined(cl_khr_fp64)

static ulong2 INLINE OVERLOADABLE __convert_ulong2(__bool2 b)
{
    return as_ulong2(-(long2)(b.x, b.y));
}

static ulong3 INLINE OVERLOADABLE __convert_ulong3(__bool3 b)
{
    return as_ulong3(-(long3)(b.x, b.y, b.z));
}

static ulong4 INLINE OVERLOADABLE __convert_ulong4(__bool4 b)
{
    return as_ulong4(-(long4)(b.x, b.y, b.z, b.w));
}

static ulong8 INLINE OVERLOADABLE __convert_ulong8(__bool8 b)
{
    return (ulong8)(__convert_ulong4(b.lo), __convert_ulong4(b.hi));
}

static ulong16 INLINE OVERLOADABLE __convert_ulong16(__bool16 b)
{
    return (ulong16)(__convert_ulong8(b.lo), __convert_ulong8(b.hi));
}

#endif

static ushort2 INLINE OVERLOADABLE __convert_ushort2(__bool2 b)
{
    return as_ushort2(-(short2)(b.x, b.y));
}

static ushort3 INLINE OVERLOADABLE __convert_ushort3(__bool3 b)
{
    return as_ushort3(-(short3)(b.x, b.y, b.z));
}

static ushort4 INLINE OVERLOADABLE __convert_ushort4(__bool4 b)
{
    return as_ushort4(-(ushort4)(b.x, b.y, b.z, b.w));
}

static ushort8 INLINE OVERLOADABLE __convert_ushort8(__bool8 b)
{
    return (ushort8)(__convert_ushort4(b.lo), __convert_ushort4(b.hi));
}

static ushort16 INLINE OVERLOADABLE __convert_ushort16(__bool16 b)
{
    return (ushort16)(__convert_ushort8(b.lo), __convert_ushort8(b.hi));
}

float __builtin_spirv_OpenCL_normalize_f32(float p) {
  return __builtin_spirv_OpenCL_sign_f32(p);
}

float2 __builtin_spirv_OpenCL_normalize_v2f32(float2 p) {
  if (all(p == (float2)0.0F))
    return p;

  float l2 = __builtin_spirv_OpDot_v2f32_v2f32(p, p);

  /*Currently we are not optimizing away the
    underflow case when fast-relaxed-math is enabled*/
  if (l2 < FLT_MIN) {
    p *= 0x1.0p+86F;
    l2 = __builtin_spirv_OpDot_v2f32_v2f32(p, p);
  } else if (__intel_relaxed_isinf(l2)) {
    p *= 0x1.0p-65f;
    l2 = __builtin_spirv_OpDot_v2f32_v2f32(p, p);
    if (l2 == INFINITY) {
      p = __builtin_spirv_OpenCL_copysign_v2f32_v2f32(__builtin_spirv_OpenCL_select_v2f32_v2f32_v2i32((float2)0.0F, (float2)1.0F, __convert_uint2(__builtin_spirv_OpIsInf_v2f32(p))), p);
      l2 = __builtin_spirv_OpDot_v2f32_v2f32(p, p);
    }
  }
  return p * __builtin_spirv_OpenCL_rsqrt_f32(l2);
}

float3 __builtin_spirv_OpenCL_normalize_v3f32(float3 p) {
  if (all(p == (float3)0.0F))
    return p;

  float l2 = __builtin_spirv_OpDot_v3f32_v3f32(p, p);

  if (l2 < FLT_MIN) {
    p *= 0x1.0p+86F;
    l2 = __builtin_spirv_OpDot_v3f32_v3f32(p, p);
  } else if (__intel_relaxed_isinf(l2)) {
    p *= 0x1.0p-66f;
    l2 = __builtin_spirv_OpDot_v3f32_v3f32(p, p);
    if (l2 == INFINITY) {
      p = __builtin_spirv_OpenCL_copysign_v3f32_v3f32(__builtin_spirv_OpenCL_select_v3f32_v3f32_v3i32((float3)0.0F, (float3)1.0F, __convert_uint3(__builtin_spirv_OpIsInf_v3f32(p))), p);
      l2 = __builtin_spirv_OpDot_v3f32_v3f32(p, p);
    }
  }
  return p * __builtin_spirv_OpenCL_rsqrt_f32(l2);
}

float4 __builtin_spirv_OpenCL_normalize_v4f32(float4 p) {
  if (all(p == (float4)0.0F))
    return p;

  float l2 = __builtin_spirv_OpDot_v4f32_v4f32(p, p);

  if (l2 < FLT_MIN) {
    p *= 0x1.0p+86F;
    l2 = __builtin_spirv_OpDot_v4f32_v4f32(p, p);
  } else if (__intel_relaxed_isinf(l2)) {
    p *= 0x1.0p-66f;
    l2 = __builtin_spirv_OpDot_v4f32_v4f32(p, p);
    if (l2 == INFINITY) {
      p = __builtin_spirv_OpenCL_copysign_v4f32_v4f32(__builtin_spirv_OpenCL_select_v4f32_v4f32_v4i32((float4)0.0F, (float4)1.0F, __convert_uint4(__builtin_spirv_OpIsInf_v4f32(p))), p);
      l2 = __builtin_spirv_OpDot_v4f32_v4f32(p, p);
    }
  }
  return p * __builtin_spirv_OpenCL_rsqrt_f32(l2);
}

#ifdef cl_khr_fp64

#pragma OPENCL EXTENSION cl_khr_fp64 : enable

double __builtin_spirv_OpenCL_normalize_f64(double p) {
  return __builtin_spirv_OpenCL_sign_f64(p);
}

double2 __builtin_spirv_OpenCL_normalize_v2f64(double2 p) {
  if (all(p == (double2)0.0))
    return p;

  double l2 = __builtin_spirv_OpDot_v2f64_v2f64(p, p);

  if (l2 < DBL_MIN) {
    p *= 0x1.0p+563;
    l2 = __builtin_spirv_OpDot_v2f64_v2f64(p, p);
  } else if (__intel_relaxed_isinf(l2)) {
    p *= 0x1.0p-513;
    l2 = __builtin_spirv_OpDot_v2f64_v2f64(p, p);
    if (l2 == INFINITY) {
      p = __builtin_spirv_OpenCL_copysign_v2f64_v2f64(__builtin_spirv_OpenCL_select_v2f64_v2f64_v2i64((double2)0.0, (double2)1.0, __convert_ulong2(__builtin_spirv_OpIsInf_v2f64(p))), p);
      l2 = __builtin_spirv_OpDot_v2f64_v2f64(p, p);
    }
  }
  return p * __builtin_spirv_OpenCL_rsqrt_f64(l2);
}

double3 __builtin_spirv_OpenCL_normalize_v3f64(double3 p) {
  if (all(p == (double3)0.0))
    return p;

  double l2 = __builtin_spirv_OpDot_v3f64_v3f64(p, p);

  if (l2 < DBL_MIN) {
    p *= 0x1.0p+563;
    l2 = __builtin_spirv_OpDot_v3f64_v3f64(p, p);
  } else if (__intel_relaxed_isinf(l2)) {
    p *= 0x1.0p-514;
    l2 = __builtin_spirv_OpDot_v3f64_v3f64(p, p);
    if (l2 == INFINITY) {
      p = __builtin_spirv_OpenCL_copysign_v3f64_v3f64(__builtin_spirv_OpenCL_select_v3f64_v3f64_v3i64((double3)0.0, (double3)1.0, __convert_ulong3(__builtin_spirv_OpIsInf_v3f64(p))), p);
      l2 = __builtin_spirv_OpDot_v3f64_v3f64(p, p);
    }
  }
  return p * __builtin_spirv_OpenCL_rsqrt_f64(l2);
}

double4 __builtin_spirv_OpenCL_normalize_v4f64(double4 p) {
  if (all(p == (double4)0.0))
    return p;

  double l2 = __builtin_spirv_OpDot_v4f64_v4f64(p, p);

  if (l2 < DBL_MIN) {
    p *= 0x1.0p+563;
    l2 = __builtin_spirv_OpDot_v4f64_v4f64(p, p);
  } else if (__intel_relaxed_isinf(l2)) {
    p *= 0x1.0p-514;
    l2 = __builtin_spirv_OpDot_v4f64_v4f64(p, p);
    if (l2 == INFINITY) {
      p = __builtin_spirv_OpenCL_copysign_v4f64_v4f64(__builtin_spirv_OpenCL_select_v4f64_v4f64_v4i64((double4)0.0, (double4)1.0, __convert_ulong4(__builtin_spirv_OpIsInf_v4f64(p))), p);
      l2 = __builtin_spirv_OpDot_v4f64_v4f64(p, p);
    }
  }
  return p * __builtin_spirv_OpenCL_rsqrt_f64(l2);
}

#endif


#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_normalize_f16(half p ){
    return __builtin_spirv_OpenCL_sign_f16(p);
}

half2 __builtin_spirv_OpenCL_normalize_v2f16(half2 p ){
    if (all(p == (half2)0.0F))
    return p;

  half l2 = __builtin_spirv_OpDot_v2f16_v2f16(p, p);

  if (l2 < HALF_MIN) {
    p *= HALF_MAX_SQRT;
    l2 = __builtin_spirv_OpDot_v2f16_v2f16(p, p);
  } else if (__intel_relaxed_isinf(l2)) {
    p *= HALF_MIN_SQRT;
    l2 = __builtin_spirv_OpDot_v2f16_v2f16(p, p);
    if (l2 == INFINITY) {
      p = __builtin_spirv_OpenCL_copysign_v2f16_v2f16(__builtin_spirv_OpenCL_select_v2f16_v2f16_v2i16((half2)0.0F, (half2)1.0F, __convert_ushort2(__builtin_spirv_OpIsInf_v2f16(p))), p);
      l2 = __builtin_spirv_OpDot_v2f16_v2f16(p, p);
    }
  }
  return p * __builtin_spirv_OpenCL_rsqrt_f16(l2);
}

half3 __builtin_spirv_OpenCL_normalize_v3f16(half3 p ){
    if (all(p == (half3)0.0F))
    return p;

  half l2 = __builtin_spirv_OpDot_v3f16_v3f16(p, p);

  if (l2 < HALF_MIN) {
    p *= HALF_MAX_SQRT;
    l2 = __builtin_spirv_OpDot_v3f16_v3f16(p, p);
  } else if (__intel_relaxed_isinf(l2)) {
    p *= HALF_MIN_SQRT;
    l2 = __builtin_spirv_OpDot_v3f16_v3f16(p, p);
    if (l2 == INFINITY) {
      p = __builtin_spirv_OpenCL_copysign_v3f16_v3f16(__builtin_spirv_OpenCL_select_v3f16_v3f16_v3i16((half3)0.0F, (half3)1.0F, __convert_ushort3(__builtin_spirv_OpIsInf_v3f16(p))), p);
      l2 = __builtin_spirv_OpDot_v3f16_v3f16(p, p);
    }
  }
  return p * __builtin_spirv_OpenCL_rsqrt_f16(l2);
}

half4 __builtin_spirv_OpenCL_normalize_v4f16(half4 p ){
    if (all(p == (half4)0.0F))
    return p;

  half l2 = __builtin_spirv_OpDot_v4f16_v4f16(p, p);

  if (l2 < HALF_MIN) {
    p *= HALF_MAX_SQRT;
    l2 = __builtin_spirv_OpDot_v4f16_v4f16(p, p);
  } else if (__intel_relaxed_isinf(l2)) {
    p *= HALF_MIN_SQRT;
    l2 = __builtin_spirv_OpDot_v4f16_v4f16(p, p);
    if (l2 == INFINITY) {
      p = __builtin_spirv_OpenCL_copysign_v4f16_v4f16(__builtin_spirv_OpenCL_select_v4f16_v4f16_v4i16((half4)0.0F, (half4)1.0F, __convert_ushort4(__builtin_spirv_OpIsInf_v4f16(p))), p);
      l2 = __builtin_spirv_OpDot_v4f16_v4f16(p, p);
    }
  }
  return p * __builtin_spirv_OpenCL_rsqrt_f16(l2);
}

#endif // defined(cl_khr_fp16)




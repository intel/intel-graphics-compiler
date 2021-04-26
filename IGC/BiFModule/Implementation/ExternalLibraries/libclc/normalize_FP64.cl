/*
 * Copyright (c) 2020 Advanced Micro Devices, Inc.
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

#pragma OPENCL EXTENSION cl_khr_fp64 : enable

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

double __builtin_spirv_OpenCL_normalize_f64(double p) {
  return __builtin_spirv_OpenCL_sign_f64(p);
}

double2 __builtin_spirv_OpenCL_normalize_v2f64(double2 p) {
  if (all(p == (double2)0.0))
    return p;

  double l2 = SPIRV_BUILTIN(Dot, _v2f64_v2f64, )(p, p);

  if (l2 < DBL_MIN) {
    p *= 0x1.0p+563;
    l2 = SPIRV_BUILTIN(Dot, _v2f64_v2f64, )(p, p);
  } else if (__intel_relaxed_isinf(l2)) {
    p *= 0x1.0p-513;
    l2 = SPIRV_BUILTIN(Dot, _v2f64_v2f64, )(p, p);
    if (l2 == INFINITY) {
      p = __builtin_spirv_OpenCL_copysign_v2f64_v2f64(__builtin_spirv_OpenCL_select_v2f64_v2f64_v2i64((double2)0.0, (double2)1.0, __convert_ulong2(SPIRV_BUILTIN(IsInf, _v2f64, )(p))), p);
      l2 = SPIRV_BUILTIN(Dot, _v2f64_v2f64, )(p, p);
    }
  }
  return p * __builtin_spirv_OpenCL_rsqrt_f64(l2);
}

double3 __builtin_spirv_OpenCL_normalize_v3f64(double3 p) {
  if (all(p == (double3)0.0))
    return p;

  double l2 = SPIRV_BUILTIN(Dot, _v3f64_v3f64, )(p, p);

  if (l2 < DBL_MIN) {
    p *= 0x1.0p+563;
    l2 = SPIRV_BUILTIN(Dot, _v3f64_v3f64, )(p, p);
  } else if (__intel_relaxed_isinf(l2)) {
    p *= 0x1.0p-514;
    l2 = SPIRV_BUILTIN(Dot, _v3f64_v3f64, )(p, p);
    if (l2 == INFINITY) {
      p = __builtin_spirv_OpenCL_copysign_v3f64_v3f64(__builtin_spirv_OpenCL_select_v3f64_v3f64_v3i64((double3)0.0, (double3)1.0, __convert_ulong3(SPIRV_BUILTIN(IsInf, _v3f64, )(p))), p);
      l2 = SPIRV_BUILTIN(Dot, _v3f64_v3f64, )(p, p);
    }
  }
  return p * __builtin_spirv_OpenCL_rsqrt_f64(l2);
}

double4 __builtin_spirv_OpenCL_normalize_v4f64(double4 p) {
  if (all(p == (double4)0.0))
    return p;

  double l2 = SPIRV_BUILTIN(Dot, _v4f64_v4f64, )(p, p);

  if (l2 < DBL_MIN) {
    p *= 0x1.0p+563;
    l2 = SPIRV_BUILTIN(Dot, _v4f64_v4f64, )(p, p);
  } else if (__intel_relaxed_isinf(l2)) {
    p *= 0x1.0p-514;
    l2 = SPIRV_BUILTIN(Dot, _v4f64_v4f64, )(p, p);
    if (l2 == INFINITY) {
      p = __builtin_spirv_OpenCL_copysign_v4f64_v4f64(__builtin_spirv_OpenCL_select_v4f64_v4f64_v4i64((double4)0.0, (double4)1.0, __convert_ulong4(SPIRV_BUILTIN(IsInf, _v4f64, )(p))), p);
      l2 = SPIRV_BUILTIN(Dot, _v4f64_v4f64, )(p, p);
    }
  }
  return p * __builtin_spirv_OpenCL_rsqrt_f64(l2);
}

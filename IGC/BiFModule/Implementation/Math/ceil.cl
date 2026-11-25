/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.h"
#include "../../Headers/spirv.h"

INLINE
float __attribute__((overloadable)) __spirv_ocl_ceil(float x) { return __builtin_IB_frnd_pi(x); }

#ifdef cl_khr_fp16
INLINE
half __attribute__((overloadable)) __spirv_ocl_ceil(half x) {
  return __spirv_FConvert_Rhalf(__spirv_ocl_ceil(__spirv_FConvert_Rfloat(x)));
}
#endif

#if defined(cl_khr_fp64)
INLINE
double __attribute__((overloadable)) __spirv_ocl_ceil(double x) {
  // First part of the algorithm performs rounding towards zero by truncating bits in
  // the fractional part of the number.This is done by finding out the position of the
  // fractional bits of the mantissa and masking them out with zeros.

  double roundedToZeroVal = __spirv_ocl_trunc(x);
  unsigned high32Bit = (int)(as_ulong(x) >> 32);
  ulong fraction = as_ulong(x) - as_ulong(roundedToZeroVal); // getting fraction

  // Second part is to calculate exponent adjustment based on sign of source

  uint temp5 = (uint)(as_ulong(fraction));
  uint temp6 = (uint)(as_ulong(fraction) >> 32);
  uint sign = high32Bit & 0x80000000;
  uint expBias = (sign == 0) ? 0x3ff00000 : 0x80000000;
  uint orDst = temp5 | temp6;
  ulong signAdjustedVal = (orDst == 0) ? sign : (expBias);
  return as_double(signAdjustedVal << 32) + as_double(roundedToZeroVal);
}
#endif

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS(ceil, float, float, f32)

#if defined(cl_khr_fp64)

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS(ceil, double, double, f64)

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS(ceil, half, half, f16)

#endif // defined(cl_khr_fp16)

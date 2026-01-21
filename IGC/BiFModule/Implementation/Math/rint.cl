/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( rint, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __attribute__((overloadable)) __spirv_ocl_rint( double x )
{
  double absolute_x;
  double rounded_int;
  uint exp_;

  // abs value
  absolute_x = as_double(as_ulong(x) & ~DOUBLE_SIGN_MASK);

  // round to nearest int if mantissa contains fractional parts
  exp_ = as_ulong(absolute_x) >> DOUBLE_MANTISSA_BITS;
  double nearest_int = 0.5 * (double)((exp_ < DOUBLE_MANTISSA_BITS + DOUBLE_BIAS) & 1);
  rounded_int = __spirv_ocl_trunc(absolute_x + nearest_int);

  // get the parity bit; does src has a fraction equal to 0.5?
  uint parity = ((ulong)rounded_int) & 0x1;
  uint has_a_half = ((rounded_int - absolute_x) == 0.5) & 0x1;

  // if so, adjust the previous, truncated round, to the nearest even
  rounded_int = rounded_int - 1.0 * (double)(has_a_half & parity);

  // reapply the sign
  ulong sign = as_ulong(x) & DOUBLE_SIGN_MASK;
  rounded_int = as_double(sign | as_ulong(rounded_int));

  return rounded_int;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( rint, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( rint, half, half, f16 )

#endif // defined(cl_khr_fp16)


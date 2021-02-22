/*========================== begin_copyright_notice ============================

Copyright (c) 2000-2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_rint, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __builtin_spirv_OpenCL_rint_f64( double x )
{
  double absolute_x;
  double rounded_int;
  uint exp_;

  // abs value
  absolute_x = as_double(as_ulong(x) & ~DOUBLE_SIGN_MASK);

  // round to nearest int if mantissa contains fractional parts
  exp_ = as_ulong(absolute_x) >> DOUBLE_MANTISSA_BITS;
  double nearest_int = 0.5 * (double)((exp_ < DOUBLE_MANTISSA_BITS + DOUBLE_BIAS) & 1);
  rounded_int = __builtin_spirv_OpenCL_trunc_f64(absolute_x + nearest_int);

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

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_rint, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_rint, half, half, f16 )

#endif // defined(cl_khr_fp16)

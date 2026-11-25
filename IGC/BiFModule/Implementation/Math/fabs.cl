/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.h"
#include "../../Headers/spirv.h"

INLINE
float __attribute__((overloadable)) __spirv_ocl_fabs(float x) { return as_float(as_uint(x) & 0x7FFFFFFF); }

#ifdef cl_khr_fp16
INLINE
half __attribute__((overloadable)) __spirv_ocl_fabs(half x) {
  // return __builtin_IB_fabsh(x);
  ushort mask = 0x7FFF;
  ushort temp = as_ushort(x) & mask;
  return as_half(temp);
}
#endif

#if defined(cl_khr_fp64)
INLINE
double __attribute__((overloadable)) __spirv_ocl_fabs(double x) {
  uint2 temp = as_uint2(x);
  temp.s1 = temp.s1 & 0x7FFFFFFF;
  return as_double(temp);
}
#endif

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS(fabs, float, float, f32)

#if defined(cl_khr_fp64)

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS(fabs, double, double, f64)

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS(fabs, half, half, f16)

#endif // defined(cl_khr_fp16)

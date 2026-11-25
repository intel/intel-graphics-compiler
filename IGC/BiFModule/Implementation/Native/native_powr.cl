/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.h"
#include "../../Headers/spirv.h"

INLINE
float __attribute__((overloadable)) __spirv_ocl_native_powr(float x, float y) {
  return __builtin_IB_native_powrf(x, y);
}

#ifdef cl_khr_fp16
INLINE
half __attribute__((overloadable)) __spirv_ocl_native_powr(half x, half y) {
  return (half)__builtin_IB_native_powrf((float)x, (float)y);
}
#endif

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS(native_powr, float, float, f32)

#if defined(cl_khr_fp64)

INLINE double __attribute__((overloadable)) __spirv_ocl_native_powr(double x, double y) {
  return __spirv_ocl_native_powr((float)x, (float)y);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS(native_powr, double, double, f64)

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS(native_powr, half, half, f16)

#endif // defined(cl_khr_fp16)

// TODO: do we support pow natively?

/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.h"
#include "../../Headers/spirv.h"

INLINE
float __attribute__((overloadable)) __spirv_ocl_native_rsqrt(float x) {
  return __spirv_ocl_native_recip(__spirv_ocl_native_sqrt(x));
}

#ifdef cl_khr_fp16
INLINE
half __attribute__((overloadable)) __spirv_ocl_native_rsqrt(half x) {
  return __spirv_ocl_native_recip(__spirv_ocl_native_sqrt(x));
}
#endif

#if defined(cl_khr_fp64)
INLINE double __attribute__((overloadable)) __spirv_ocl_native_rsqrt(double x) {
  return (1 / __builtin_IB_native_sqrtd(x));
}
#endif

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS(native_rsqrt, float, float, f32)

#if defined(cl_khr_fp64)

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS(native_rsqrt, double, double, f64)

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS(native_rsqrt, half, half, f16)

#endif // defined(cl_khr_fp16)

/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.h"
#include "../../Headers/spirv.h"

INLINE
float __attribute__((overloadable)) __spirv_ocl_native_tan(float x) {
  return __spirv_ocl_native_divide(__spirv_ocl_native_sin(x), __spirv_ocl_native_cos(x));
}

#ifdef cl_khr_fp16
INLINE
half __attribute__((overloadable)) __spirv_ocl_native_tan(half x) {
  return __spirv_ocl_native_divide(__spirv_ocl_native_sin((float)x), __spirv_ocl_native_cos((float)x));
}
#endif

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS(native_tan, float, float, f32)

#if defined(cl_khr_fp64)

INLINE double __attribute__((overloadable)) __spirv_ocl_native_tan(double x) {
  return __spirv_ocl_native_tan((float)x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS(native_tan, double, double, f64)

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS(native_tan, half, half, f16)

#endif // defined(cl_khr_fp16)

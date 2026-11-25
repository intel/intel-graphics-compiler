/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.h"
#include "../../Headers/spirv.h"

INLINE float __attribute__((overloadable)) __spirv_ocl_fclamp(float x, float minval, float maxval) {
  return __spirv_ocl_fmin(__spirv_ocl_fmax(x, minval), maxval);
}

#ifdef cl_khr_fp16
INLINE half __attribute__((overloadable)) __spirv_ocl_fclamp(half x, half minval, half maxval) {
  return __spirv_ocl_fmin(__spirv_ocl_fmax(x, minval), maxval);
}
#endif

#if defined(cl_khr_fp64)
INLINE double __attribute__((overloadable)) __spirv_ocl_fclamp(double x, double minval, double maxval) {
  return __spirv_ocl_fmin(__spirv_ocl_fmax(x, minval), maxval);
}
#endif

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_3ARGS(fclamp, float, float, f32)

#if defined(cl_khr_fp64)

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_3ARGS(fclamp, double, double, f64)

#endif // defined(cl_khr_fp64)

#ifdef cl_khr_fp16

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_3ARGS(fclamp, half, half, f16)

#endif // defined(cl_khr_fp16)

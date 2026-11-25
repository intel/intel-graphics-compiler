/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.h"
#include "../../Headers/spirv.h"
#include "../IMF/FP32/asin_s_la.h"

#if defined(cl_khr_fp64)
#include "../IMF/FP64/asin_d_la.h"
#endif // defined(cl_khr_fp64)

INLINE
float __attribute__((overloadable)) __spirv_ocl_asin(float value) { return __ocl_svml_asinf(value); }

#ifdef cl_khr_fp16
INLINE
half __attribute__((overloadable)) __spirv_ocl_asin(half x) { return (half)__spirv_ocl_asin((float)x); }
#endif

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS(asin, float, float, f32)

#if defined(cl_khr_fp64)

INLINE double __attribute__((overloadable)) __spirv_ocl_asin(double x) { return __ocl_svml_asin(x); }

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS(asin, double, double, f64)

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS(asin, half, half, f16)

#endif // defined(cl_khr_fp16)

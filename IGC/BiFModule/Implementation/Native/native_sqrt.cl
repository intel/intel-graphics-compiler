/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.h"
#include "../../Headers/spirv.h"

INLINE
float __attribute__((overloadable)) __spirv_ocl_native_sqrt(float x) { return __builtin_IB_native_sqrtf(x); }

#ifdef cl_khr_fp16
INLINE
half __attribute__((overloadable)) __spirv_ocl_native_sqrt(half x) { return __builtin_IB_native_sqrth(x); }
#endif

#if defined(cl_khr_fp64)
INLINE double __attribute__((overloadable)) __spirv_ocl_native_sqrt(double x) { return __builtin_IB_native_sqrtd(x); }
#endif

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS(native_sqrt, float, float, f32)

#if defined(cl_khr_fp64)

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS(native_sqrt, double, double, f64)

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS(native_sqrt, half, half, f16)

#endif // defined(cl_khr_fp16)

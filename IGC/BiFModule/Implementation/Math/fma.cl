/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.h"
#include "../../Headers/spirv.h"

#ifdef cl_khr_fp16
INLINE
half __attribute__((overloadable)) __spirv_ocl_fma(half a, half b, half c) { return __builtin_IB_fmah(a, b, c); }

INLINE
float __attribute__((overloadable)) __spirv_ocl_fma(float a, float b, float c) { return __builtin_fmaf(a, b, c); }

INLINE
double __attribute__((overloadable)) __spirv_ocl_fma(double a, double b, double c) { return __builtin_fma(a, b, c); }
#endif

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_3ARGS(fma, float, float, f32)

#if defined(cl_khr_fp64)

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_3ARGS(fma, double, double, f64)

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_3ARGS(fma, half, half, f16)

#endif // defined(cl_khr_fp16)

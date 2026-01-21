/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float __attribute__((overloadable)) __spirv_ocl_step(float edge, float x ){
    return x < edge ? 0.0f : 1.0f;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS( step, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __attribute__((overloadable)) __spirv_ocl_step(double edge, double x ){
    return x < edge ? 0.0 : 1.0;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS( step, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __attribute__((overloadable)) __spirv_ocl_step(half edge, half x ){
    return x < edge ? (half)0.0f : (half)1.0f;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS( step, half, half, f16 )

#endif // defined(cl_khr_fp16)


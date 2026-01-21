/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float __attribute__((overloadable)) __spirv_ocl_fdim( float x, float y )
{
    float r = x - y;
    float n = __spirv_ocl_nan(0);
    int i = __intel_relaxed_isnan(x) | __intel_relaxed_isnan(y);
    r = x > y ? r : 0.0f;
    r = i ? n : r;
    return r;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS( fdim, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __attribute__((overloadable)) __spirv_ocl_fdim( double x, double y )
{
    double r = x - y;
    double n = __spirv_ocl_nan(0);
    int i = __intel_relaxed_isnan(x) | __intel_relaxed_isnan(y);
    r = x > y ? r : 0.0f;
    r = i ? n : r;
    return r;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS( fdim, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __attribute__((overloadable)) __spirv_ocl_fdim( half x, half y )
{
    half r = x - y;
    half n = __spirv_ocl_nan(0);
    int i = __intel_relaxed_isnan(x) | __intel_relaxed_isnan(y);
    r = x > y ? r : 0.0f;
    r = i ? n : r;
    return r;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS( fdim, half, half, f16 )

#endif // defined(cl_khr_fp16)


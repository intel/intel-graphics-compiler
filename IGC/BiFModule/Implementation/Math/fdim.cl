/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float __builtin_spirv_OpenCL_fdim_f32_f32( float x, float y )
{
    float r = x - y;
    float n = __builtin_spirv_OpenCL_nan_i32(0u);
    int i = __intel_relaxed_isnan(x) | __intel_relaxed_isnan(y);
    r = x > y ? r : 0.0f;
    r = i ? n : r;
    return r;
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( __builtin_spirv_OpenCL_fdim, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __builtin_spirv_OpenCL_fdim_f64_f64( double x, double y )
{
    double r = x - y;
    double n = __builtin_spirv_OpenCL_nan_i64(0ul);
    int i = __intel_relaxed_isnan(x) | __intel_relaxed_isnan(y);
    r = x > y ? r : 0.0f;
    r = i ? n : r;
    return r;
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( __builtin_spirv_OpenCL_fdim, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_fdim_f16_f16( half x, half y )
{
    half r = x - y;
    half n = __builtin_spirv_OpenCL_nan_i16(0u);
    int i = __intel_relaxed_isnan(x) | __intel_relaxed_isnan(y);
    r = x > y ? r : 0.0f;
    r = i ? n : r;
    return r;
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( __builtin_spirv_OpenCL_fdim, half, half, f16 )

#endif // defined(cl_khr_fp16)

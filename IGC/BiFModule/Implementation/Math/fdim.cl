/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fdim, _f32_f32, )( float x, float y )
{
    float r = x - y;
    float n = SPIRV_OCL_BUILTIN(nan, _i32, )(0);
    int i = __intel_relaxed_isnan(x) | __intel_relaxed_isnan(y);
    r = x > y ? r : 0.0f;
    r = i ? n : r;
    return r;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS( fdim, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fdim, _f64_f64, )( double x, double y )
{
    double r = x - y;
    double n = SPIRV_OCL_BUILTIN(nan, _i64, )(0);
    int i = __intel_relaxed_isnan(x) | __intel_relaxed_isnan(y);
    r = x > y ? r : 0.0f;
    r = i ? n : r;
    return r;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS( fdim, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fdim, _f16_f16, )( half x, half y )
{
    half r = x - y;
    half n = SPIRV_OCL_BUILTIN(nan, _i16, )(0);
    int i = __intel_relaxed_isnan(x) | __intel_relaxed_isnan(y);
    r = x > y ? r : 0.0f;
    r = i ? n : r;
    return r;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS( fdim, half, half, f16 )

#endif // defined(cl_khr_fp16)

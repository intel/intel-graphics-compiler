/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float __attribute__((overloadable)) __spirv_ocl_sign( float x )
{
    float result = x;
    result = ( x > 0.0f ) ?  1.0f : result;
    result = ( x < 0.0f ) ? -1.0f : result;
    result = __intel_relaxed_isnan(x) ? 0.0f : result;
    return result ;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( sign, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __attribute__((overloadable)) __spirv_ocl_sign( double x )
{
    double result = x;
    result = ( x > 0.0 ) ?  1.0 : result;
    result = ( x < 0.0 ) ? -1.0 : result;
    result = __intel_relaxed_isnan(x) ? 0.0 : result;
    return result ;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( sign, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __attribute__((overloadable)) __spirv_ocl_sign( half x )
{
    half result = x;
    result = ( x > (half)0.0f ) ? (half) 1.0f : result;
    result = ( x < (half)0.0f ) ? (half)-1.0f : result;
    result = __intel_relaxed_isnan(x) ? (half)0.0f : result;
    return result ;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( sign, half, half, f16 )

#endif // defined(cl_khr_fp16)


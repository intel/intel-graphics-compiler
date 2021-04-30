/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float __builtin_spirv_OpenCL_sign_f32( float x )
{
    float result = x;
    result = ( x > 0.0f ) ?  1.0f : result;
    result = ( x < 0.0f ) ? -1.0f : result;
    result = __intel_relaxed_isnan(x) ? 0.0f : result;
    return result ;
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_sign, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __builtin_spirv_OpenCL_sign_f64( double x )
{
    double result = x;
    result = ( x > 0.0 ) ?  1.0 : result;
    result = ( x < 0.0 ) ? -1.0 : result;
    result = __intel_relaxed_isnan(x) ? 0.0 : result;
    return result ;
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_sign, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_sign_f16( half x )
{
    half result = x;
    result = ( x > (half)0.0f ) ? (half) 1.0f : result;
    result = ( x < (half)0.0f ) ? (half)-1.0f : result;
    result = __intel_relaxed_isnan(x) ? (half)0.0f : result;
    return result ;
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_sign, half, half, f16 )

#endif // defined(cl_khr_fp16)

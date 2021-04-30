/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

float __builtin_spirv_OpenCL_logb_f32( float x )
{
    float result = 0.0f;

    if( __intel_relaxed_isnormal( x ) )
    {
        result = ( (as_int(x) & FLOAT_EXPONENT_MASK ) >> FLOAT_MANTISSA_BITS) - FLOAT_BIAS;
    }
    else if( __intel_relaxed_isnan( x ) )
    {
        result = __builtin_spirv_OpenCL_nan_i32(0U);
    }
    else if( __intel_relaxed_isinf( x ) )
    {
        result = INFINITY;
    }
    else if( x == 0.0f )
    {
        result = -INFINITY;
    }
    else
    {
        x = x * ( 1 << FLOAT_MANTISSA_BITS );
        result = ( (as_int(x) & FLOAT_EXPONENT_MASK ) >> FLOAT_MANTISSA_BITS) - FLOAT_BIAS - FLOAT_MANTISSA_BITS;
    }

    return result;
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_logb, float, float, f32 )

#if defined(cl_khr_fp64)

double __builtin_spirv_OpenCL_logb_f64( double x )
{
    double result = 0.0;

    if( SPIRV_BUILTIN(IsNormal, _f64, )( x ) )
    {
        result = ( (as_long(x) & DOUBLE_EXPONENT_MASK ) >> DOUBLE_MANTISSA_BITS) - DOUBLE_BIAS;
    }
    else if( SPIRV_BUILTIN(IsNan, _f64, )( x ) )
    {
        result = __builtin_spirv_OpenCL_nan_i64(0UL);
    }
    else if( SPIRV_BUILTIN(IsInf, _f64, )( x ) )
    {
        result = INFINITY;
    }
    else if( x == 0.0 )
    {
        result = -INFINITY;
    }
    else
    {
        x = x * ( 1UL << DOUBLE_MANTISSA_BITS );
        result = ( (as_long(x) & DOUBLE_EXPONENT_MASK ) >> DOUBLE_MANTISSA_BITS) - DOUBLE_BIAS - DOUBLE_MANTISSA_BITS;
    }

    return result;
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_logb, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_logb_f16( half x )
{
    return (half)__builtin_spirv_OpenCL_logb_f32((float)x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_logb, half, half, f16 )

#endif // defined(cl_khr_fp16)

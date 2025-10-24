/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

int SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ilogb, _f32, )( float x )
{
    int result = 0;

    if( __intel_relaxed_isnormal( x ) )
    {
        result = ( (as_int(x) & FLOAT_EXPONENT_MASK ) >> FLOAT_MANTISSA_BITS) - FLOAT_BIAS;
    }
    else if( __intel_relaxed_isnan( x ) |
             __intel_relaxed_isinf( x ) )
    {
        result = FP_ILOGBNAN;
    }
    else if( x == 0.0f )
    {
        result = FP_ILOGB0;
    }
    else
    {
        x = x * ( 1 << FLOAT_MANTISSA_BITS );
        result = ( (as_int(x) & FLOAT_EXPONENT_MASK ) >> FLOAT_MANTISSA_BITS) - FLOAT_BIAS - FLOAT_MANTISSA_BITS;
    }

    return result;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARG_LOOP( ilogb, int, float, f32 )

#if defined(cl_khr_fp64)

int SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ilogb, _f64, )( double x )
{
    int result = 0;

    if( SPIRV_BUILTIN(IsNormal, _f64, )( x ) )
    {
        result = ( (as_long(x) & DOUBLE_EXPONENT_MASK ) >> DOUBLE_MANTISSA_BITS) - DOUBLE_BIAS;
    }
    else if( SPIRV_BUILTIN(IsNan, _f64, )( x ) | SPIRV_BUILTIN(IsInf, _f64, )( x ) )
    {
        result = FP_ILOGBNAN;
    }
    else if( x == 0.0 )
    {
        result = FP_ILOGB0;
    }
    else
    {
        x = x * ( 1UL << DOUBLE_MANTISSA_BITS );
        result = ( (as_long(x) & DOUBLE_EXPONENT_MASK ) >> DOUBLE_MANTISSA_BITS) - DOUBLE_BIAS - DOUBLE_MANTISSA_BITS;
    }

    return result;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARG_LOOP( ilogb, int, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE int SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ilogb, _f16, )( half x )
{
    return SPIRV_OCL_BUILTIN(ilogb, _f32, )((float)x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARG_LOOP( ilogb, int, half, f16 )

#endif // defined(cl_khr_fp16)

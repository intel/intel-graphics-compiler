/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

int __attribute__((overloadable)) __spirv_ocl_ilogb( float x )
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

int __attribute__((overloadable)) __spirv_ocl_ilogb( double x )
{
    int result = 0;

    if( __spirv_IsNormal( x ) )
    {
        result = ( (as_long(x) & DOUBLE_EXPONENT_MASK ) >> DOUBLE_MANTISSA_BITS) - DOUBLE_BIAS;
    }
    else if( __spirv_IsNan( x ) | __spirv_IsInf( x ) )
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

INLINE int __attribute__((overloadable)) __spirv_ocl_ilogb( half x )
{
    return __spirv_ocl_ilogb((float)x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARG_LOOP( ilogb, int, half, f16 )

#endif // defined(cl_khr_fp16)


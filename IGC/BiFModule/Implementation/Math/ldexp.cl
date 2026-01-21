/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

float __attribute__((overloadable)) __spirv_ocl_ldexp( float x, int n )
{
    int delta = 0;
    float m0 = 1.0f;
    m0 = ( n < (FLT_MIN_EXP+1) ) ? as_float( ( (FLT_MIN_EXP+1) + FLOAT_BIAS ) << FLOAT_MANTISSA_BITS ) : m0;
    m0 = ( n > (FLT_MAX_EXP-1) ) ? as_float( ( (FLT_MAX_EXP-1) + FLOAT_BIAS ) << FLOAT_MANTISSA_BITS ) : m0;
    delta = ( n < (FLT_MIN_EXP+1) ) ? (FLT_MIN_EXP+1) : 0;
    delta = ( n > (FLT_MAX_EXP-1) ) ? (FLT_MAX_EXP-1) : delta;
    n -= delta;

    float m1 = 1.0f;
    m1 = ( n < (FLT_MIN_EXP+1) ) ? as_float( ( (FLT_MIN_EXP+1) + FLOAT_BIAS ) << FLOAT_MANTISSA_BITS ) : m1;
    m1 = ( n > (FLT_MAX_EXP-1) ) ? as_float( ( (FLT_MAX_EXP-1) + FLOAT_BIAS ) << FLOAT_MANTISSA_BITS ) : m1;
    delta = ( n < (FLT_MIN_EXP+1) ) ? (FLT_MIN_EXP+1) : 0;
    delta = ( n > (FLT_MAX_EXP-1) ) ? (FLT_MAX_EXP-1) : delta;
    n -= delta;

    float mn = as_float( ( n + FLOAT_BIAS ) << FLOAT_MANTISSA_BITS );
    mn = ( n == 0 ) ? 1.0f : mn;
    mn = ( n < (FLT_MIN_EXP+1) ) ? as_float( ( (FLT_MIN_EXP+1) + FLOAT_BIAS ) << FLOAT_MANTISSA_BITS ) : mn;
    mn = ( n > (FLT_MAX_EXP-1) ) ? as_float( ( (FLT_MAX_EXP-1) + FLOAT_BIAS ) << FLOAT_MANTISSA_BITS ) : mn;

    float res = x * mn * m0 * m1;

    return res;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS_VV( ldexp, float, float, int, f32, i32 )

#if defined(cl_khr_fp64)

double __attribute__((overloadable)) __spirv_ocl_ldexp( double x, int n )
{
    int delta = 0;
    double m0 = 1.0;
    m0 = ( n < (DBL_MIN_EXP+1) ) ? as_double( (long)( (DBL_MIN_EXP+1) + DOUBLE_BIAS ) << DOUBLE_MANTISSA_BITS ) : m0;
    m0 = ( n > (DBL_MAX_EXP-1) ) ? as_double( (long)( (DBL_MAX_EXP-1) + DOUBLE_BIAS ) << DOUBLE_MANTISSA_BITS ) : m0;
    delta = ( n < (DBL_MIN_EXP+1) ) ? (DBL_MIN_EXP+1) : 0;
    delta = ( n > (DBL_MAX_EXP-1) ) ? (DBL_MAX_EXP-1) : delta;
    n -= delta;

    double m1 = 1.0;
    m1 = ( n < (DBL_MIN_EXP+1) ) ? as_double( (long)( (DBL_MIN_EXP+1) + DOUBLE_BIAS ) << DOUBLE_MANTISSA_BITS ) : m1;
    m1 = ( n > (DBL_MAX_EXP-1) ) ? as_double( (long)( (DBL_MAX_EXP-1) + DOUBLE_BIAS ) << DOUBLE_MANTISSA_BITS ) : m1;
    delta = ( n < (DBL_MIN_EXP+1) ) ? (DBL_MIN_EXP+1) : 0;
    delta = ( n > (DBL_MAX_EXP-1) ) ? (DBL_MAX_EXP-1) : delta;
    n -= delta;

    double mn = as_double( (long)( n + DOUBLE_BIAS ) << DOUBLE_MANTISSA_BITS );
    mn = ( n == 0 ) ? 1.0 : mn;
    mn = ( n < (DBL_MIN_EXP+1) ) ? as_double( (long)( (DBL_MIN_EXP+1) + DOUBLE_BIAS ) << DOUBLE_MANTISSA_BITS ) : mn;
    mn = ( n > (DBL_MAX_EXP-1) ) ? as_double( (long)( (DBL_MAX_EXP-1) + DOUBLE_BIAS ) << DOUBLE_MANTISSA_BITS ) : mn;

    double res = x * mn * m0 * m1;

    return res;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS_VV( ldexp, double, double, int, f64, i32 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

half __attribute__((overloadable)) __spirv_ocl_ldexp( half x, int n )
{
    float mn = as_float( ( n + FLOAT_BIAS ) << FLOAT_MANTISSA_BITS );
    mn = ( n == 0 ) ? 1.0f : mn;
    mn = ( n < (FLT_MIN_EXP+1) ) ? as_float( ( (FLT_MIN_EXP+1) + FLOAT_BIAS ) << FLOAT_MANTISSA_BITS ) : mn;
    mn = ( n > (FLT_MAX_EXP-1) ) ? as_float( ( (FLT_MAX_EXP-1) + FLOAT_BIAS ) << FLOAT_MANTISSA_BITS ) : mn;

    float res = x * mn;

    return res;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS_VV( ldexp, half, half, int, f16, i32 )

#endif // defined(cl_khr_fp16)


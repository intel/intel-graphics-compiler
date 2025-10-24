/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(hypot, _f32_f32, )( float x, float y )
{
    // Note: This isn't well specified, but apparently conformance
    // tests expect the following behavior:
    // 1) If either input is infinity, return infinity.
    // 2) Else, if either input is NaN, return NaN.
    // 3) Else, if both inputs are zero, return zero.
    // 4) Else, return sqrt( x * x, y * y )

    float result;

    if( __intel_relaxed_isinf( x ) |
        __intel_relaxed_isinf( y ) )
    {
        result = INFINITY;
    }
    else if( __intel_relaxed_isnan( x ) |
             __intel_relaxed_isnan( y ) )
    {
        result = SPIRV_OCL_BUILTIN(nan, _i32, )(0);
    }
    else
    {
        x = SPIRV_OCL_BUILTIN(fabs, _f32, )( x );
        y = SPIRV_OCL_BUILTIN(fabs, _f32, )( y );
        float maxc = x > y ? x : y;
        float minc = x > y ? y : x;
        if( maxc == 0.0f )
        {
            result = 0.0f;
        }
        else
        {
            // Scale all components down by the biggest component.
            // Compute the length of this scaled vector, then scale
            // back up to compute the actual length.  Since this is
            // a vec2 normalize, we have it a bit easier, since we
            // know the other component is the min component.
            float s = minc / maxc;
            float t = SPIRV_OCL_BUILTIN(sqrt, _f32, )( SPIRV_OCL_BUILTIN(mad, _f32_f32_f32, )( s, s, 1.0f ) );
            result = t * maxc;
        }
    }

    return result;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( hypot, float, float, float, f32, f32 )

#if defined(cl_khr_fp64)

double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(hypot, _f64_f64, )( double x, double y )
{
    // Note: This isn't well specified, but apparently conformance
    // tests expect the following behavior:
    // 1) If either input is infinity, return infinity.
    // 2) Else, if either input is NaN, return NaN.
    // 3) Else, if both inputs are zero, return zero.
    // 4) Else, return sqrt( x * x, y * y )

    // Find the biggest absolute component:
    double2 p = (double2)( x, y );
    double2 a = SPIRV_OCL_BUILTIN(fabs, _v2f64, )( p );
    double maxc = a.x > a.y ? a.x : a.y;
    double minc = a.x > a.y ? a.y : a.x;

    double result;
    if( SPIRV_BUILTIN(IsInf, _f64, )(p.x) |
        SPIRV_BUILTIN(IsInf, _f64, )(p.y) )
    {
        result = INFINITY;
    }
    else if( SPIRV_BUILTIN(IsNan, _f64, )( minc ) )
    {
        result = SPIRV_OCL_BUILTIN(nan, _i32, )(0);
    }
    else
    {
        // Scale by the biggest component.
        // Compute the length of this scaled vector, then scale
        // back up to compute the actual length.
        double s = minc / maxc;
        double t = SPIRV_OCL_BUILTIN(sqrt, _f64, )( SPIRV_OCL_BUILTIN(mad, _f64_f64_f64, )( s, s, 1.0 ) );
        result = t * maxc;

        result = ( maxc == 0.0 ) ? 0.0 : result;
    }

    return result;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( hypot, double, double, double, f64, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(hypot, _f16_f16, )( half x, half y )
{
    return (half)SPIRV_OCL_BUILTIN(hypot, _f32_f32, )((float)x, (float)y);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( hypot, half, half, half, f16, f16 )

#endif // defined(cl_khr_fp16)

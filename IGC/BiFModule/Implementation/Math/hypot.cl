/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

float __attribute__((overloadable)) __spirv_ocl_hypot( float x, float y )
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
        result = __spirv_ocl_nan(0);
    }
    else
    {
        x = __spirv_ocl_fabs( x );
        y = __spirv_ocl_fabs( y );
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
            float t = __spirv_ocl_sqrt( __spirv_ocl_mad( s, s, 1.0f ) );
            result = t * maxc;
        }
    }

    return result;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( hypot, float, float, float, f32, f32 )

#if defined(cl_khr_fp64)

double __attribute__((overloadable)) __spirv_ocl_hypot( double x, double y )
{
    // Note: This isn't well specified, but apparently conformance
    // tests expect the following behavior:
    // 1) If either input is infinity, return infinity.
    // 2) Else, if either input is NaN, return NaN.
    // 3) Else, if both inputs are zero, return zero.
    // 4) Else, return sqrt( x * x, y * y )

    // Find the biggest absolute component:
    double2 p = (double2)( x, y );
    double2 a = __spirv_ocl_fabs( p );
    double maxc = a.x > a.y ? a.x : a.y;
    double minc = a.x > a.y ? a.y : a.x;

    double result;
    if( __spirv_IsInf(p.x) |
        __spirv_IsInf(p.y) )
    {
        result = INFINITY;
    }
    else if( __spirv_IsNan( minc ) )
    {
        result = __spirv_ocl_nan(0);
    }
    else
    {
        // Scale by the biggest component.
        // Compute the length of this scaled vector, then scale
        // back up to compute the actual length.
        double s = minc / maxc;
        double t = __spirv_ocl_sqrt( __spirv_ocl_mad( s, s, 1.0 ) );
        result = t * maxc;

        result = ( maxc == 0.0 ) ? 0.0 : result;
    }

    return result;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( hypot, double, double, double, f64, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

half __attribute__((overloadable)) __spirv_ocl_hypot( half x, half y )
{
    return (half)__spirv_ocl_hypot((float)x, (float)y);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( hypot, half, half, half, f16, f16 )

#endif // defined(cl_khr_fp16)


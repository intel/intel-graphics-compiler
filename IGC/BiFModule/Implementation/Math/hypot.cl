/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

float __builtin_spirv_OpenCL_hypot_f32_f32( float x, float y )
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
        result = __builtin_spirv_OpenCL_nan_i32(0u);
    }
    else
    {
        x = __builtin_spirv_OpenCL_fabs_f32( x );
        y = __builtin_spirv_OpenCL_fabs_f32( y );
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
            float t = __builtin_spirv_OpenCL_sqrt_f32( __builtin_spirv_OpenCL_mad_f32_f32_f32( s, s, 1.0f ) );
            result = t * maxc;
        }
    }

    return result;
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpenCL_hypot, float, float, float, f32, f32 )

#if defined(cl_khr_fp64)

double __builtin_spirv_OpenCL_hypot_f64_f64( double x, double y )
{
    // Note: This isn't well specified, but apparently conformance
    // tests expect the following behavior:
    // 1) If either input is infinity, return infinity.
    // 2) Else, if either input is NaN, return NaN.
    // 3) Else, if both inputs are zero, return zero.
    // 4) Else, return sqrt( x * x, y * y )

    // Find the biggest absolute component:
    double2 p = (double2)( x, y );
    double2 a = __builtin_spirv_OpenCL_fabs_v2f64( p );
    double maxc = a.x > a.y ? a.x : a.y;
    double minc = a.x > a.y ? a.y : a.x;

    double result;
    if( __builtin_spirv_OpIsInf_f64(p.x) |
        __builtin_spirv_OpIsInf_f64(p.y) )
    {
        result = INFINITY;
    }
    else if( __builtin_spirv_OpIsNan_f64( minc ) )
    {
        result = __builtin_spirv_OpenCL_nan_i32(0u);
    }
    else
    {
        // Scale by the biggest component.
        // Compute the length of this scaled vector, then scale
        // back up to compute the actual length.
        double s = minc / maxc;
        double t = __builtin_spirv_OpenCL_sqrt_f64( __builtin_spirv_OpenCL_mad_f64_f64_f64( s, s, 1.0 ) );
        result = t * maxc;

        result = ( maxc == 0.0 ) ? 0.0 : result;
    }

    return result;
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpenCL_hypot, double, double, double, f64, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

half __builtin_spirv_OpenCL_hypot_f16_f16( half x, half y )
{
    return (half)__builtin_spirv_OpenCL_hypot_f32_f32(x, y);
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpenCL_hypot, half, half, half, f16, f16 )

#endif // defined(cl_khr_fp16)

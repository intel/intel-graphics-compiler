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

#if defined(cl_khr_fp64)

    #include "../ExternalLibraries/libclc/doubles.cl"

#endif // defined(cl_khr_fp64)

// TODO: I think we should be able to use M_PI_F here instead of FLOAT_PI,
// but this does cause very small differences in the results of atan2(),
// since M_PI_F is rounded (and therefore slightly larger than FLOAT_PI).
// We'll need to re-collect some GITS streams if we want to use M_PI_F
// instead.
#define FLOAT_PI                (as_float(0x40490FDA)) // 3.1415926535897930f

float __builtin_spirv_OpenCL_atan2_f32_f32( float y, float x )
{
    // atan2(y,x) =  atan(y/x)        if  any y, x > 0    atan_yx
    //            =  atan(y/x) + pi   if y >= 0, x < 0    atan_yx + pi
    //            =  atan(y/x) - pi   if y <  0, x < 0    atan_yx - pi
    //            =  pi/2             if y >= 0, x = 0    pi/2
    //            = -pi/2             if y <  0, x = 0    -pi/2
    //  Sign    = (y >= 0) ? 1 : -1;
    //  atan_yx = (x != 0) ? atan(y/x) : Sign*pi/2;
    //  Coeff   = (x < 0)  ? Sign : 0;
    //  dst     = atan_yx + Coeff*pi;

    float result;

    if(__FastRelaxedMath && (!__APIRS))
    {
        // Implemented as:
        //  atan(y/x) for x > 0,
        //  atan(y/x) + M_PI_F for x < 0 and y > 0, and
        //  atan(y/x) -M_PI_F for x < 0 and y < 0.
        float px = __builtin_spirv_OpenCL_atan_f32( y / x );
        float py = px + M_PI_F;
        float ny = px - M_PI_F;

        result = ( y > 0 ) ? py : ny;
        result = ( x > 0 ) ? px : result;
    }
    else
    {
        if( __intel_relaxed_isnan(x) |
            __intel_relaxed_isnan(y) )
        {
            result = __builtin_spirv_OpenCL_nan_i32(0U);
        }
        else
        {
            float signy = __builtin_spirv_OpenCL_copysign_f32_f32(1.0f, y);
            if( y == 0.0f )
            {
                float signx = __builtin_spirv_OpenCL_copysign_f32_f32(1.0f, x);
                float px = signy * 0.0f;
                float nx = signy * FLOAT_PI;
                // In this case, we need to compare signx against
                // 1.0f, not x > 0.0f, since we need to distinguish
                // between x == +0.0f and x == -0.0f.
                result = ( signx == 1.0f ) ? px : nx;
            }
            else if( x == 0.0f )
            {
                result = signy * ( FLOAT_PI * 0.5f );
            }
            else if( __intel_relaxed_isinf( y ) &
                     __intel_relaxed_isinf( x ) )
            {
                float px = signy * ( FLOAT_PI * 0.25f );
                float nx = signy * ( FLOAT_PI * 0.75f );
                result = ( x > 0.0f ) ? px : nx;
            }
            else
            {
                float px = __builtin_spirv_OpenCL_atan_f32( y / x );
                float nx = __builtin_spirv_OpenCL_mad_f32_f32_f32( signy, FLOAT_PI, px );
                result = ( x > 0.0f ) ? px : nx;
            }
        }
    }

    return result;
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpenCL_atan2, float, float, float, f32, f32 )

#if defined(cl_khr_fp64)

INLINE double __builtin_spirv_OpenCL_atan2_f64_f64( double y, double x )
{
        return libclc_atan2_f64_f64(y, x);
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpenCL_atan2, double, double, double, f64, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_atan2_f16_f16( half y, half x )
{
    half result;

    if( __intel_relaxed_isnan((float)x) |
        __intel_relaxed_isnan((float)y) )
    {
        result = __builtin_spirv_OpenCL_nan_i32(0U);
    }
    else
    {
        half signy = __builtin_spirv_OpenCL_copysign_f16_f16(1.0f, y);
        if( y == 0.0f )
        {
            half signx = __builtin_spirv_OpenCL_copysign_f16_f16(1.0f, x);
            half px = signy * 0.0f;
            half nx = signy * FLOAT_PI;
            // In this case, we need to compare signx against
            // 1.0f, not x > 0.0f, since we need to distinguish
            // between x == +0.0f and x == -0.0f.
            result = ( signx == 1.0f ) ? px : nx;
        }
        else if( x == 0.0f )
        {
            result = signy * ( FLOAT_PI * 0.5f );
        }
        else if(__intel_relaxed_isinf((float)y) &
                __intel_relaxed_isinf((float)x))
        {
            half px = signy * ( FLOAT_PI * 0.25f );
            half nx = signy * ( FLOAT_PI * 0.75f );
            result = ( x > 0.0f ) ? px : nx;
        }
        else
        {
            half px = __builtin_spirv_OpenCL_atan_f32( (float)y / (float)x );
            half nx = __builtin_spirv_OpenCL_mad_f32_f32_f32( (float)signy, FLOAT_PI, (float)px );
            result = ( x > 0.0f ) ? px : nx;
        }
    }

    return result;
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpenCL_atan2, half, half, half, f16, f16 )

#endif // defined(cl_khr_fp16)

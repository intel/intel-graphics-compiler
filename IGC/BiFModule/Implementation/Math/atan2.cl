/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

#if defined(cl_khr_fp64)
    #include "../IMF/FP64/atan2_d_la.cl"
#endif // defined(cl_khr_fp64)

// TODO: I think we should be able to use M_PI_F here instead of FLOAT_PI,
// but this does cause very small differences in the results of atan2(),
// since M_PI_F is rounded (and therefore slightly larger than FLOAT_PI).
// We'll need to re-collect some GITS streams if we want to use M_PI_F
// instead.
#define FLOAT_PI                (as_float(0x40490FDA)) // 3.1415926535897930f

float __attribute__((overloadable)) __spirv_ocl_atan2( float y, float x )
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

    if(BIF_FLAG_CTRL_GET(FastRelaxedMath) && (!BIF_FLAG_CTRL_GET(APIRS)))
    {
        // This works in all special points except when (y, x) are both zeroes
        //
        // input;              implementation;        reference;            rel.error
        // y =  0, x = -1;     3.141592741012573;     3.1415926535897931;    2.78e-08
        // y = -0, x = -1;    -3.141592741012573;    -3.1415926535897931;    2.78e-08
        // y =  0, x =  1;    -0.000000000000000;     0.0000000000000000;           0
        // y = -0, x =  1;     0.000000000000000;    -0.0000000000000000;           0
        // y = -1, x =  0;    -1.570796370506287;    -1.5707963267948966;    2.78e-08
        // y = -1, x = -0;    -1.570796370506287;    -1.5707963267948966;    2.78e-08
        // y =  1, x =  0;     1.570796370506287;     1.5707963267948966;    2.78e-08
        // y =  1, x = -0;     1.570796370506287;     1.5707963267948966;    2.78e-08
        float px = __spirv_ocl_atan(y / x);
        float c = (x >= 0.0f) ? 0.0f : __spirv_ocl_copysign(M_PI_F, x * px);
        result = px + c;
        result = (0.0f == x) ? result = __spirv_ocl_copysign(result, y) : result;
        result = (0.0f == y) ? result = __spirv_ocl_copysign(result, -x * y) : result;
    }
    else
    {
        // The LA atan2 implementation (IMF/FP32/atan2_s_la.cl)
        // seems to be slower on Mandelbulb algorithm..
        if( __intel_relaxed_isnan(x) |
            __intel_relaxed_isnan(y) )
        {
            result = __spirv_ocl_nan(0);
        }
        else
        {
            float signy = __spirv_ocl_copysign(1.0f, y);
            if( y == 0.0f )
            {
                float signx = __spirv_ocl_copysign(1.0f, x);
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
                float px = __spirv_ocl_atan( y / x );
                float nx = __spirv_ocl_mad( signy, FLOAT_PI, px );
                result = ( x > 0.0f ) ? px : nx;
            }
        }
    }

    return result;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( atan2, float, float, float, f32, f32 )

#if defined(cl_khr_fp64)

INLINE double __attribute__((overloadable)) __spirv_ocl_atan2( double y, double x )
{
    return __ocl_svml_atan2(y, x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( atan2, double, double, double, f64, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __attribute__((overloadable)) __spirv_ocl_atan2( half y, half x )
{
    half result;

    if( __intel_relaxed_isnan((float)x) |
        __intel_relaxed_isnan((float)y) )
    {
        result = __spirv_ocl_nan(0);
    }
    else
    {
        half signy = __spirv_ocl_copysign((half)(1.0), y);
        if( y == 0.0f )
        {
            half signx = __spirv_ocl_copysign((half)(1.0), x);
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
            half px = __spirv_ocl_atan( (float)y / (float)x );
            half nx = __spirv_ocl_mad( (float)signy, FLOAT_PI, (float)px );
            result = ( x > 0.0f ) ? px : nx;
        }
    }

    return result;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( atan2, half, half, half, f16, f16 )

#endif // defined(cl_khr_fp16)


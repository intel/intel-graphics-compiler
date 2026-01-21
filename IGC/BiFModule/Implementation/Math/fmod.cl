/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE
float __attribute__((overloadable)) __spirv_ocl_fast_fmod( float xx, float yy )
{
    float result = xx - yy * __spirv_ocl_trunc( xx / yy );
    return result;
}

INLINE
float2 __attribute__((overloadable)) __spirv_ocl_fast_fmod( float2 xx, float2 yy )
{
    float2 temp;
    temp.s0 = __spirv_ocl_fast_fmod(xx.s0, yy.s0);
    temp.s1 = __spirv_ocl_fast_fmod(xx.s1, yy.s1);
    return temp;
}

INLINE
float3 __attribute__((overloadable)) __spirv_ocl_fast_fmod( float3 xx, float3 yy )
{
    float3 temp;
    temp.s0 = __spirv_ocl_fast_fmod(xx.s0, yy.s0);
    temp.s1 = __spirv_ocl_fast_fmod(xx.s1, yy.s1);
    temp.s2 = __spirv_ocl_fast_fmod(xx.s2, yy.s2);
    return temp;
}

INLINE
float4 __attribute__((overloadable)) __spirv_ocl_fast_fmod( float4 xx, float4 yy )
{
    float4 temp;
    temp.s0 = __spirv_ocl_fast_fmod(xx.s0, yy.s0);
    temp.s1 = __spirv_ocl_fast_fmod(xx.s1, yy.s1);
    temp.s2 = __spirv_ocl_fast_fmod(xx.s2, yy.s2);
    temp.s3 = __spirv_ocl_fast_fmod(xx.s3, yy.s3);
    return temp;
}

#if defined(cl_khr_fp16)
INLINE
half __attribute__((overloadable)) __spirv_ocl_fast_fmod( half xx, half yy )
{
    return (half)__spirv_ocl_fast_fmod((float)xx, (float)yy);
}

INLINE
half2 __attribute__((overloadable)) __spirv_ocl_fast_fmod( half2 xx, half2 yy )
{
    half2 temp;
    temp.s0 = __spirv_ocl_fast_fmod(xx.s0, yy.s0);
    temp.s1 = __spirv_ocl_fast_fmod(xx.s1, yy.s1);
    return temp;
}

INLINE
half3 __attribute__((overloadable)) __spirv_ocl_fast_fmod( half3 xx, half3 yy )
{
    half3 temp;
    temp.s0 = __spirv_ocl_fast_fmod(xx.s0, yy.s0);
    temp.s1 = __spirv_ocl_fast_fmod(xx.s1, yy.s1);
    temp.s2 = __spirv_ocl_fast_fmod(xx.s2, yy.s2);
    return temp;
}

INLINE
half4 __attribute__((overloadable)) __spirv_ocl_fast_fmod( half4 xx, half4 yy )
{
    half4 temp;
    temp.s0 = __spirv_ocl_fast_fmod(xx.s0, yy.s0);
    temp.s1 = __spirv_ocl_fast_fmod(xx.s1, yy.s1);
    temp.s2 = __spirv_ocl_fast_fmod(xx.s2, yy.s2);
    temp.s3 = __spirv_ocl_fast_fmod(xx.s3, yy.s3);
    return temp;
}
#endif // cl_khr_fp16

#if defined(cl_khr_fp64)
INLINE
double __attribute__((overloadable)) __spirv_ocl_fast_fmod( double xx, double yy )
{
    return __spirv_ocl_fast_fmod(xx, yy);
}

INLINE
double2 __attribute__((overloadable)) __spirv_ocl_fast_fmod( double2 xx, double2 yy )
{
    double2 temp;
    temp.s0 = __spirv_ocl_fast_fmod(xx.s0, yy.s0);
    temp.s1 = __spirv_ocl_fast_fmod(xx.s1, yy.s1);
    return temp;
}

INLINE
double3 __attribute__((overloadable)) __spirv_ocl_fast_fmod( double3 xx, double3 yy )
{
    double3 temp;
    temp.s0 = __spirv_ocl_fast_fmod(xx.s0, yy.s0);
    temp.s1 = __spirv_ocl_fast_fmod(xx.s1, yy.s1);
    temp.s2 = __spirv_ocl_fast_fmod(xx.s2, yy.s2);
    return temp;
}

INLINE
double4 __attribute__((overloadable)) __spirv_ocl_fast_fmod( double4 xx, double4 yy )
{
    double4 temp;
    temp.s0 = __spirv_ocl_fast_fmod(xx.s0, yy.s0);
    temp.s1 = __spirv_ocl_fast_fmod(xx.s1, yy.s1);
    temp.s2 = __spirv_ocl_fast_fmod(xx.s2, yy.s2);
    temp.s3 = __spirv_ocl_fast_fmod(xx.s3, yy.s3);
    return temp;
}
#endif // cl_khr_fp64

static float __intel_fmod_f32_f32( float xx, float yy, bool doFast )
{
    float result;
    if( BIF_FLAG_CTRL_GET(FastRelaxedMath) && doFast )
    {
        return __spirv_ocl_fast_fmod(xx, yy);
    }

    if( __intel_relaxed_isnan(xx) |
        __intel_relaxed_isnan(yy) |
        __intel_relaxed_isinf(xx) |
        yy == 0.0f)
    {
        result = __spirv_ocl_nan(0);
    }
    else if( __intel_relaxed_isinf(yy) |
             (xx == 0.0f) )
    {
        result = xx;
    }
    else if( __spirv_ocl_fabs(xx) == __spirv_ocl_fabs(yy) )
    {
        result = __spirv_ocl_copysign(0.0f, xx);
    }
    else if (__spirv_ocl_fabs(xx) < __spirv_ocl_fabs(yy))
    {
        result = xx;
    }
    else
    {
        float x = __spirv_ocl_fabs(xx);
        float y = __spirv_ocl_fabs(yy);
        int ex = __spirv_ocl_ilogb( x );
        int ey = __spirv_ocl_ilogb( y );
        float xr = x;
        float yr = y;

        if(ex-ey >= 0)
        {
            yr = __spirv_ocl_ldexp( y, -ey );
            xr = __spirv_ocl_ldexp( x, -ex );

            for(int i = ex-ey; i > 0; i--)
            {
                float s = xr - yr;
                xr = ( xr >= yr ) ? s : xr;
                xr = xr + xr;
            }
            float s = xr - yr;
            xr = ( xr >= yr ) ? s : xr;

            xr = __spirv_ocl_ldexp(xr, ey);
        }

        float m = -xr;
        xr = ( xx < 0.0f ) ? m : xr;

        result = xr;
    }

    return result;
}

float __attribute__((overloadable)) __spirv_ocl_fmod( float xx, float yy )
{
    return __intel_fmod_f32_f32( xx, yy, true );
}

// OpFMod is the core version and is identical to OpenCL_fmod except
// it takes the sign from operand 2
INLINE float __builtin_spirv_OpFMod_f32_f32( float x, float y )
{
    return __spirv_ocl_copysign(__spirv_ocl_fmod(x, y), y);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( fmod, float, float, float, f32, f32 )
GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpFMod, float, float, float, f32, f32 )

#if defined(cl_khr_fp64)

double __attribute__((overloadable)) __spirv_ocl_fmod( double xx, double yy )
{
    double result;

    if( __spirv_IsNan(xx) |
        __spirv_IsNan(yy) |
        __spirv_IsInf(xx) |
        yy == 0.0)
    {
        result = __spirv_ocl_nan(0);
    }
    else if( __spirv_IsInf(yy) |
             (xx == 0.0) )
    {
        result = xx;
    }
    else if( __spirv_ocl_fabs(xx) == __spirv_ocl_fabs(yy) )
    {
        result = __spirv_ocl_copysign(0.0, xx);
    }
    else if (__spirv_ocl_fabs(xx) < __spirv_ocl_fabs(yy))
    {
        result = xx;
    }
    else
    {
        double x = __spirv_ocl_fabs(xx);
        double y = __spirv_ocl_fabs(yy);
        int ex = __spirv_ocl_ilogb( x );
        int ey = __spirv_ocl_ilogb( y );
        double xr = x;
        double yr = y;

        if(ex-ey >= 0)
        {
            yr = __spirv_ocl_ldexp( y, -ey );
            xr = __spirv_ocl_ldexp( x, -ex );

            for(int i = ex-ey; i > 0; i--)
            {
                double s = xr - yr;
                xr = ( xr >= yr ) ? s : xr;
                xr = xr + xr;
            }
            double s = xr - yr;
            xr = ( xr >= yr ) ? s : xr;

            xr = __spirv_ocl_ldexp(xr, ey);
        }

        double m = -xr;
        xr = ( xx < 0.0 ) ? m : xr;

        result = xr;
    }

    return result;
}

INLINE double __builtin_spirv_OpFMod_f64_f64( double x, double y )
{
    return __spirv_ocl_copysign(__spirv_ocl_fmod(x, y), y);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( fmod, double, double, double, f64, f64 )
GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpFMod, double, double, double, f64, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __attribute__((overloadable)) __spirv_ocl_fmod( half x, half y )
{
    return __spirv_ocl_fmod((float)x, (float)y);
}

INLINE half __builtin_spirv_OpFMod_f16_f16( half x, half y )
{
    return __spirv_ocl_copysign(__spirv_ocl_fmod(x, y), y);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( fmod, half, half, half, f16, f16 )
GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpFMod, half, half, half, f16, f16 )

#endif // defined(cl_khr_fp16)


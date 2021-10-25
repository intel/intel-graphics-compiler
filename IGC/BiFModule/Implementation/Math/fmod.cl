/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"


INLINE
float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fast_fmod, _f32_f32, )( float xx, float yy )
{
    float result = xx - yy * SPIRV_OCL_BUILTIN(trunc, _f32, )( xx / yy );
    return result;
}

INLINE
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fast_fmod, _v2f32_v2f32, )( float2 xx, float2 yy )
{
    float2 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(fast_fmod, _f32_f32, )(xx.s0, yy.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(fast_fmod, _f32_f32, )(xx.s1, yy.s1);
    return temp;
}

INLINE
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fast_fmod, _v3f32_v3f32, )( float3 xx, float3 yy )
{
    float3 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(fast_fmod, _f32_f32, )(xx.s0, yy.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(fast_fmod, _f32_f32, )(xx.s1, yy.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(fast_fmod, _f32_f32, )(xx.s2, yy.s2);
    return temp;
}

INLINE
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fast_fmod, _v4f32_v4f32, )( float4 xx, float4 yy )
{
    float4 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(fast_fmod, _f32_f32, )(xx.s0, yy.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(fast_fmod, _f32_f32, )(xx.s1, yy.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(fast_fmod, _f32_f32, )(xx.s2, yy.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(fast_fmod, _f32_f32, )(xx.s3, yy.s3);
    return temp;
}


#if defined(cl_khr_fp16)
INLINE
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fast_fmod, _f16_f16, )( half xx, half yy )
{
    return (half)SPIRV_OCL_BUILTIN(fast_fmod, _f32_f32, )((float)xx, (float)yy);
}

INLINE
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fast_fmod, _v2f16_v2f16, )( half2 xx, half2 yy )
{
    half2 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(fast_fmod, _f16_f16, )(xx.s0, yy.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(fast_fmod, _f16_f16, )(xx.s1, yy.s1);
    return temp;
}

INLINE
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fast_fmod, _v3f16_v3f16, )( half3 xx, half3 yy )
{
    half3 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(fast_fmod, _f16_f16, )(xx.s0, yy.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(fast_fmod, _f16_f16, )(xx.s1, yy.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(fast_fmod, _f16_f16, )(xx.s2, yy.s2);
    return temp;
}

INLINE
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fast_fmod, _v4f16_v4f16, )( half4 xx, half4 yy )
{
    half4 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(fast_fmod, _f16_f16, )(xx.s0, yy.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(fast_fmod, _f16_f16, )(xx.s1, yy.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(fast_fmod, _f16_f16, )(xx.s2, yy.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(fast_fmod, _f16_f16, )(xx.s3, yy.s3);
    return temp;
}
#endif // cl_khr_fp16


#if defined(cl_khr_fp64)
INLINE
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fast_fmod, _f64_f64, )( double xx, double yy )
{
    return SPIRV_OCL_BUILTIN(fast_fmod, _f64_f64, )(xx, yy);
}

INLINE
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fast_fmod, _v2f64_v2f64, )( double2 xx, double2 yy )
{
    double2 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(fast_fmod, _f64_f64, )(xx.s0, yy.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(fast_fmod, _f64_f64, )(xx.s1, yy.s1);
    return temp;
}

INLINE
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fast_fmod, _v3f64_v3f64, )( double3 xx, double3 yy )
{
    double3 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(fast_fmod, _f64_f64, )(xx.s0, yy.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(fast_fmod, _f64_f64, )(xx.s1, yy.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(fast_fmod, _f64_f64, )(xx.s2, yy.s2);
    return temp;
}

INLINE
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fast_fmod, _v4f64_v4f64, )( double4 xx, double4 yy )
{
    double4 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(fast_fmod, _f64_f64, )(xx.s0, yy.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(fast_fmod, _f64_f64, )(xx.s1, yy.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(fast_fmod, _f64_f64, )(xx.s2, yy.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(fast_fmod, _f64_f64, )(xx.s3, yy.s3);
    return temp;
}
#endif // cl_khr_fp64

static float __intel_fmod_f32_f32( float xx, float yy, bool doFast )
{
    float result;
    if( __FastRelaxedMath && doFast )
    {
        return SPIRV_OCL_BUILTIN(fast_fmod, _f32_f32, )(xx, yy);
    }

    if( __intel_relaxed_isnan(xx) |
        __intel_relaxed_isnan(yy) |
        __intel_relaxed_isinf(xx) |
        yy == 0.0f)
    {
        result = SPIRV_OCL_BUILTIN(nan, _i32, )(0);
    }
    else if( __intel_relaxed_isinf(yy) |
             (xx == 0.0f) )
    {
        result = xx;
    }
    else if( SPIRV_OCL_BUILTIN(fabs, _f32, )(xx) == SPIRV_OCL_BUILTIN(fabs, _f32, )(yy) )
    {
        result = SPIRV_OCL_BUILTIN(copysign, _f32_f32, )(0.0f, xx);
    }
    else if (SPIRV_OCL_BUILTIN(fabs, _f32, )(xx) < SPIRV_OCL_BUILTIN(fabs, _f32, )(yy))
    {
        result = xx;
    }
    else
    {
        float x = SPIRV_OCL_BUILTIN(fabs, _f32, )(xx);
        float y = SPIRV_OCL_BUILTIN(fabs, _f32, )(yy);
        int ex = SPIRV_OCL_BUILTIN(ilogb, _f32, )( x );
        int ey = SPIRV_OCL_BUILTIN(ilogb, _f32, )( y );
        float xr = x;
        float yr = y;

        if(ex-ey >= 0)
        {
            yr = SPIRV_OCL_BUILTIN(ldexp, _f32_i32, )( y, -ey );
            xr = SPIRV_OCL_BUILTIN(ldexp, _f32_i32, )( x, -ex );

            for(int i = ex-ey; i > 0; i--)
            {
                float s = xr - yr;
                xr = ( xr >= yr ) ? s : xr;
                xr = xr + xr;
            }
            float s = xr - yr;
            xr = ( xr >= yr ) ? s : xr;

            xr = SPIRV_OCL_BUILTIN(ldexp, _f32_i32, )(xr, ey);
        }

        float m = -xr;
        xr = ( xx < 0.0f ) ? m : xr;

        result = xr;
    }

    return result;
}

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmod, _f32_f32, )( float xx, float yy )
{
    return __intel_fmod_f32_f32( xx, yy, true );
}

// OpFMod is the core version and is identical to OpenCL_fmod except
// it takes the sign from operand 2
INLINE float __builtin_spirv_OpFMod_f32_f32( float x, float y )
{
    return SPIRV_OCL_BUILTIN(copysign, _f32_f32, )(SPIRV_OCL_BUILTIN(fmod, _f32_f32, )(x, y), y);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( fmod, float, float, float, f32, f32 )
GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpFMod, float, float, float, f32, f32 )

#if defined(cl_khr_fp64)

double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmod, _f64_f64, )( double xx, double yy )
{
    double result;

    if( SPIRV_BUILTIN(IsNan, _f64, )(xx) |
        SPIRV_BUILTIN(IsNan, _f64, )(yy) |
        SPIRV_BUILTIN(IsInf, _f64, )(xx) |
        yy == 0.0)
    {
        result = SPIRV_OCL_BUILTIN(nan, _i64, )(0);
    }
    else if( SPIRV_BUILTIN(IsInf, _f64, )(yy) |
             (xx == 0.0) )
    {
        result = xx;
    }
    else if( SPIRV_OCL_BUILTIN(fabs, _f64, )(xx) == SPIRV_OCL_BUILTIN(fabs, _f64, )(yy) )
    {
        result = SPIRV_OCL_BUILTIN(copysign, _f64_f64, )(0.0, xx);
    }
    else if (SPIRV_OCL_BUILTIN(fabs, _f64, )(xx) < SPIRV_OCL_BUILTIN(fabs, _f64, )(yy))
    {
        result = xx;
    }
    else
    {
        double x = SPIRV_OCL_BUILTIN(fabs, _f64, )(xx);
        double y = SPIRV_OCL_BUILTIN(fabs, _f64, )(yy);
        int ex = SPIRV_OCL_BUILTIN(ilogb, _f64, )( x );
        int ey = SPIRV_OCL_BUILTIN(ilogb, _f64, )( y );
        double xr = x;
        double yr = y;

        if(ex-ey >= 0)
        {
            yr = SPIRV_OCL_BUILTIN(ldexp, _f64_i32, )( y, -ey );
            xr = SPIRV_OCL_BUILTIN(ldexp, _f64_i32, )( x, -ex );

            for(int i = ex-ey; i > 0; i--)
            {
                double s = xr - yr;
                xr = ( xr >= yr ) ? s : xr;
                xr = xr + xr;
            }
            double s = xr - yr;
            xr = ( xr >= yr ) ? s : xr;

            xr = SPIRV_OCL_BUILTIN(ldexp, _f64_i32, )(xr, ey);
        }

        double m = -xr;
        xr = ( xx < 0.0 ) ? m : xr;

        result = xr;
    }

    return result;
}

INLINE double __builtin_spirv_OpFMod_f64_f64( double x, double y )
{
    return SPIRV_OCL_BUILTIN(copysign, _f64_f64, )(SPIRV_OCL_BUILTIN(fmod, _f64_f64, )(x, y), y);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( fmod, double, double, double, f64, f64 )
GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpFMod, double, double, double, f64, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmod, _f16_f16, )( half x, half y )
{
    return SPIRV_OCL_BUILTIN(fmod, _f32_f32, )((float)x, (float)y);
}

INLINE half __builtin_spirv_OpFMod_f16_f16( half x, half y )
{
    return SPIRV_OCL_BUILTIN(copysign, _f16_f16, )(SPIRV_OCL_BUILTIN(fmod, _f16_f16, )(x, y), y);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( fmod, half, half, half, f16, f16 )
GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpFMod, half, half, half, f16, f16 )

#endif // defined(cl_khr_fp16)

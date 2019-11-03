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


INLINE
float __builtin_spirv_OpenCL_fast_fmod_f32_f32( float xx, float yy )
{
    float result = xx - yy * __builtin_spirv_OpenCL_trunc_f32( xx / yy );
    return result;
}

INLINE
float2 __builtin_spirv_OpenCL_fast_fmod_v2f32_v2f32( float2 xx, float2 yy )
{
    float2 temp;
    temp.s0 = __builtin_spirv_OpenCL_fast_fmod_f32_f32(xx.s0, yy.s0);
    temp.s1 = __builtin_spirv_OpenCL_fast_fmod_f32_f32(xx.s1, yy.s1);
    return temp;
}

INLINE
float3 __builtin_spirv_OpenCL_fast_fmod_v3f32_v3f32( float3 xx, float3 yy )
{
    float3 temp;
    temp.s0 = __builtin_spirv_OpenCL_fast_fmod_f32_f32(xx.s0, yy.s0);
    temp.s1 = __builtin_spirv_OpenCL_fast_fmod_f32_f32(xx.s1, yy.s1);
    temp.s2 = __builtin_spirv_OpenCL_fast_fmod_f32_f32(xx.s2, yy.s2);
    return temp;
}

INLINE
float4 __builtin_spirv_OpenCL_fast_fmod_v4f32_v4f32( float4 xx, float4 yy )
{
    float4 temp;
    temp.s0 = __builtin_spirv_OpenCL_fast_fmod_f32_f32(xx.s0, yy.s0);
    temp.s1 = __builtin_spirv_OpenCL_fast_fmod_f32_f32(xx.s1, yy.s1);
    temp.s2 = __builtin_spirv_OpenCL_fast_fmod_f32_f32(xx.s2, yy.s2);
    temp.s3 = __builtin_spirv_OpenCL_fast_fmod_f32_f32(xx.s3, yy.s3);
    return temp;
}


#if defined(cl_khr_fp16)
INLINE
half __builtin_spirv_OpenCL_fast_fmod_f16_f16( half xx, half yy )
{
    return (half)__builtin_spirv_OpenCL_fast_fmod_f32_f32((float)xx, (float)yy);
}

INLINE
half2 __builtin_spirv_OpenCL_fast_fmod_v2f16_v2f16( half2 xx, half2 yy )
{
    half2 temp;
    temp.s0 = __builtin_spirv_OpenCL_fast_fmod_f16_f16(xx.s0, yy.s0);
    temp.s1 = __builtin_spirv_OpenCL_fast_fmod_f16_f16(xx.s1, yy.s1);
    return temp;
}

INLINE
half3 __builtin_spirv_OpenCL_fast_fmod_v3f16_v3f16( half3 xx, half3 yy )
{
    half3 temp;
    temp.s0 = __builtin_spirv_OpenCL_fast_fmod_f16_f16(xx.s0, yy.s0);
    temp.s1 = __builtin_spirv_OpenCL_fast_fmod_f16_f16(xx.s1, yy.s1);
    temp.s2 = __builtin_spirv_OpenCL_fast_fmod_f16_f16(xx.s2, yy.s2);
    return temp;
}

INLINE
half4 __builtin_spirv_OpenCL_fast_fmod_v4f16_v4f16( half4 xx, half4 yy )
{
    half4 temp;
    temp.s0 = __builtin_spirv_OpenCL_fast_fmod_f16_f16(xx.s0, yy.s0);
    temp.s1 = __builtin_spirv_OpenCL_fast_fmod_f16_f16(xx.s1, yy.s1);
    temp.s2 = __builtin_spirv_OpenCL_fast_fmod_f16_f16(xx.s2, yy.s2);
    temp.s3 = __builtin_spirv_OpenCL_fast_fmod_f16_f16(xx.s3, yy.s3);
    return temp;
}
#endif // cl_khr_fp16


#if defined(cl_khr_fp64)
INLINE
double __builtin_spirv_OpenCL_fast_fmod_f64_f64( double xx, double yy )
{
    return __builtin_spirv_OpenCL_fast_fmod_f64_f64(xx, yy);
}

INLINE
double2 __builtin_spirv_OpenCL_fast_fmod_v2f64_v2f64( double2 xx, double2 yy )
{
    double2 temp;
    temp.s0 = __builtin_spirv_OpenCL_fast_fmod_f64_f64(xx.s0, yy.s0);
    temp.s1 = __builtin_spirv_OpenCL_fast_fmod_f64_f64(xx.s1, yy.s1);
    return temp;
}

INLINE
double3 __builtin_spirv_OpenCL_fast_fmod_v3f64_v3f64( double3 xx, double3 yy )
{
    double3 temp;
    temp.s0 = __builtin_spirv_OpenCL_fast_fmod_f64_f64(xx.s0, yy.s0);
    temp.s1 = __builtin_spirv_OpenCL_fast_fmod_f64_f64(xx.s1, yy.s1);
    temp.s2 = __builtin_spirv_OpenCL_fast_fmod_f64_f64(xx.s2, yy.s2);
    return temp;
}

INLINE
double4 __builtin_spirv_OpenCL_fast_fmod_v4f64_v4f64( double4 xx, double4 yy )
{
    double4 temp;
    temp.s0 = __builtin_spirv_OpenCL_fast_fmod_f64_f64(xx.s0, yy.s0);
    temp.s1 = __builtin_spirv_OpenCL_fast_fmod_f64_f64(xx.s1, yy.s1);
    temp.s2 = __builtin_spirv_OpenCL_fast_fmod_f64_f64(xx.s2, yy.s2);
    temp.s3 = __builtin_spirv_OpenCL_fast_fmod_f64_f64(xx.s3, yy.s3);
    return temp;
}
#endif // cl_khr_fp64

static float __intel_fmod_f32_f32( float xx, float yy, bool doFast )
{
    float result;
    if( __FastRelaxedMath && doFast )
    {
        return __builtin_spirv_OpenCL_fast_fmod_f32_f32(xx, yy);
    }

    if( __intel_relaxed_isnan(xx) |
        __intel_relaxed_isnan(yy) |
        __intel_relaxed_isinf(xx) |
        yy == 0.0f)
    {
        result = __builtin_spirv_OpenCL_nan_i32((uint)0);
    }
    else if( __intel_relaxed_isinf(yy) |
             (xx == 0.0f) )
    {
        result = xx;
    }
    else if( __builtin_spirv_OpenCL_fabs_f32(xx) == __builtin_spirv_OpenCL_fabs_f32(yy) )
    {
        result = __builtin_spirv_OpenCL_copysign_f32_f32(0.0f, xx);
    }
    else if (__builtin_spirv_OpenCL_fabs_f32(xx) < __builtin_spirv_OpenCL_fabs_f32(yy))
    {
        result = xx;
    }
    else
    {
        float x = __builtin_spirv_OpenCL_fabs_f32(xx);
        float y = __builtin_spirv_OpenCL_fabs_f32(yy);
        int ex = __builtin_spirv_OpenCL_ilogb_f32( x );
        int ey = __builtin_spirv_OpenCL_ilogb_f32( y );
        float xr = x;
        float yr = y;

        if(ex-ey >= 0)
        {
            yr = __builtin_spirv_OpenCL_ldexp_f32_i32( y, -ey );
            xr = __builtin_spirv_OpenCL_ldexp_f32_i32( x, -ex );

            for(int i = ex-ey; i > 0; i--)
            {
                float s = xr - yr;
                xr = ( xr >= yr ) ? s : xr;
                xr = xr + xr;
            }
            float s = xr - yr;
            xr = ( xr >= yr ) ? s : xr;

            xr = __builtin_spirv_OpenCL_ldexp_f32_i32(xr, ey);
        }

        float m = -xr;
        xr = ( xx < 0.0f ) ? m : xr;

        result = xr;
    }

    return result;
}

float __builtin_spirv_OpenCL_fmod_f32_f32( float xx, float yy )
{
    return __intel_fmod_f32_f32( xx, yy, true );
}

// OpFMod is the core version and is identical to OpenCL_fmod except
// it takes the sign from operand 2
INLINE float __builtin_spirv_OpFMod_f32_f32( float x, float y )
{
    return __builtin_spirv_OpenCL_copysign_f32_f32(__builtin_spirv_OpenCL_fmod_f32_f32(x, y), y);
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpenCL_fmod, float, float, float, f32, f32 )
GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpFMod, float, float, float, f32, f32 )

#if defined(cl_khr_fp64)

double __builtin_spirv_OpenCL_fmod_f64_f64( double xx, double yy )
{
    double result;

    if( __builtin_spirv_OpIsNan_f64(xx) |
        __builtin_spirv_OpIsNan_f64(yy) |
        __builtin_spirv_OpIsInf_f64(xx) |
        yy == 0.0)
    {
        result = __builtin_spirv_OpenCL_nan_i64((ulong)0);
    }
    else if( __builtin_spirv_OpIsInf_f64(yy) |
             (xx == 0.0) )
    {
        result = xx;
    }
    else if( __builtin_spirv_OpenCL_fabs_f64(xx) == __builtin_spirv_OpenCL_fabs_f64(yy) )
    {
        result = __builtin_spirv_OpenCL_copysign_f64_f64(0.0, xx);
    }
    else if (__builtin_spirv_OpenCL_fabs_f64(xx) < __builtin_spirv_OpenCL_fabs_f64(yy))
    {
        result = xx;
    }
    else
    {
        double x = __builtin_spirv_OpenCL_fabs_f64(xx);
        double y = __builtin_spirv_OpenCL_fabs_f64(yy);
        int ex = __builtin_spirv_OpenCL_ilogb_f64( x );
        int ey = __builtin_spirv_OpenCL_ilogb_f64( y );
        double xr = x;
        double yr = y;

        if(ex-ey >= 0)
        {
            yr = __builtin_spirv_OpenCL_ldexp_f64_i32( y, -ey );
            xr = __builtin_spirv_OpenCL_ldexp_f64_i32( x, -ex );

            for(int i = ex-ey; i > 0; i--)
            {
                double s = xr - yr;
                xr = ( xr >= yr ) ? s : xr;
                xr = xr + xr;
            }
            double s = xr - yr;
            xr = ( xr >= yr ) ? s : xr;

            xr = __builtin_spirv_OpenCL_ldexp_f64_i32(xr, ey);
        }

        double m = -xr;
        xr = ( xx < 0.0 ) ? m : xr;

        result = xr;
    }

    return result;
}

INLINE double __builtin_spirv_OpFMod_f64_f64( double x, double y )
{
    return __builtin_spirv_OpenCL_copysign_f64_f64(__builtin_spirv_OpenCL_fmod_f64_f64(x, y), y);
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpenCL_fmod, double, double, double, f64, f64 )
GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpFMod, double, double, double, f64, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_fmod_f16_f16( half x, half y )
{
    return __builtin_spirv_OpenCL_fmod_f32_f32((float)x, (float)y);
}

INLINE half __builtin_spirv_OpFMod_f16_f16( half x, half y )
{
    return __builtin_spirv_OpenCL_copysign_f16_f16(__builtin_spirv_OpenCL_fmod_f16_f16(x, y), y);
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpenCL_fmod, half, half, half, f16, f16 )
GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpFMod, half, half, half, f16, f16 )

#endif // defined(cl_khr_fp16)

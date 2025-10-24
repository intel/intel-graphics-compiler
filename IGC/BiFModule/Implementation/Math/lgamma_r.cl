/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _f32_p1i32, )( float         x,
                                          __global int* signp )
{
    int     s;
    float   r = SPIRV_OCL_BUILTIN(lgamma_r, _f32_p0i32, )(x, &s);
    signp[0] = s;
    return r;
}

INLINE float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v2f32_p1v2i32, )( float2         x,
                                               __global int2* signp )
{
    int2    s;
    float2  r = SPIRV_OCL_BUILTIN(lgamma_r, _v2f32_p0v2i32, )(x, &s);
    signp[0] = s;
    return r;
}

INLINE float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v3f32_p1v3i32, )( float3         x,
                                               __global int3* signp )
{
    int3    s;
    float3  r = SPIRV_OCL_BUILTIN(lgamma_r, _v3f32_p0v3i32, )(x, &s);
    signp[0] = s;
    return r;
}

INLINE float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v4f32_p1v4i32, )( float4         x,
                                               __global int4* signp )
{
    int4    s;
    float4  r = SPIRV_OCL_BUILTIN(lgamma_r, _v4f32_p0v4i32, )(x, &s);
    signp[0] = s;
    return r;
}

INLINE float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v8f32_p1v8i32, )( float8         x,
                                               __global int8* signp )
{
    int8    s;
    float8  r = SPIRV_OCL_BUILTIN(lgamma_r, _v8f32_p0v8i32, )(x, &s);
    signp[0] = s;
    return r;
}

INLINE float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v16f32_p1v16i32, )( float16         x,
                                                  __global int16* signp )
{
    int16   s;
    float16 r = SPIRV_OCL_BUILTIN(lgamma_r, _v16f32_p0v16i32, )(x, &s);
    signp[0] = s;
    return r;
}

INLINE float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _f32_p3i32, )( float        x,
                                          __local int* signp )
{
    int     s;
    float   r = SPIRV_OCL_BUILTIN(lgamma_r, _f32_p0i32, )(x, &s);
    signp[0] = s;
    return r;
}

INLINE float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v2f32_p3v2i32, )( float2        x,
                                               __local int2* signp )
{
    int2    s;
    float2  r = SPIRV_OCL_BUILTIN(lgamma_r, _v2f32_p0v2i32, )(x, &s);
    signp[0] = s;
    return r;
}

INLINE float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v3f32_p3v3i32, )( float3        x,
                                               __local int3* signp )
{
    int3    s;
    float3  r = SPIRV_OCL_BUILTIN(lgamma_r, _v3f32_p0v3i32, )(x, &s);
    signp[0] = s;
    return r;
}

INLINE float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v4f32_p3v4i32, )( float4        x,
                                               __local int4* signp )
{
    int4    s;
    float4  r = SPIRV_OCL_BUILTIN(lgamma_r, _v4f32_p0v4i32, )(x, &s);
    signp[0] = s;
    return r;
}

INLINE float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v8f32_p3v8i32, )( float8        x,
                                               __local int8* signp )
{
    int8    s;
    float8  r = SPIRV_OCL_BUILTIN(lgamma_r, _v8f32_p0v8i32, )(x, &s);
    signp[0] = s;
    return r;
}

INLINE float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v16f32_p3v16i32, )( float16        x,
                                                  __local int16* signp )
{
    int16   s;
    float16 r = SPIRV_OCL_BUILTIN(lgamma_r, _v16f32_p0v16i32, )(x, &s);
    signp[0] = s;
    return r;
}

INLINE float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _f32_p0i32, )( float          x,
                                          __private int* signp )
{
    int     s;
    float   r;
    if( __intel_relaxed_isnan(x) )
    {
        r = SPIRV_OCL_BUILTIN(nan, _i32, )(0);
        s = 0;
    }
    else
    {
        float g = SPIRV_OCL_BUILTIN(tgamma, _f32, )(x);
        r = __intel_relaxed_isnan(g) ? INFINITY : SPIRV_OCL_BUILTIN(native_log, _f32, )(SPIRV_OCL_BUILTIN(fabs, _f32, )(g));
        s = (x == 0) ? 0 : SPIRV_OCL_BUILTIN(sign, _f32, )(g);
    }
    signp[0] = s;
    return r;
}

INLINE float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v2f32_p0v2i32, )( float2          x,
                                               __private int2* signp )
{
    float2  r;
    const __private float* px = (const __private float*)&x;
    __private int*      sign_scalar = (__private int*)signp;
    __private float*    r_scalar = (__private float*)&r;
    for(uint i = 0; i < 2; i++)
    {
        r_scalar[i] = SPIRV_OCL_BUILTIN(lgamma_r, _f32_p0i32, )( px[i], sign_scalar + i );
    }
    return r;
}

INLINE float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v3f32_p0v3i32, )( float3          x,
                                               __private int3* signp )
{
    float3  r;
    const __private float* px = (const __private float*)&x;
    __private int*      sign_scalar = (__private int*)signp;
    __private float*    r_scalar = (__private float*)&r;
    for(uint i = 0; i < 3; i++)
    {
        r_scalar[i] = SPIRV_OCL_BUILTIN(lgamma_r, _f32_p0i32, )( px[i], sign_scalar + i );
    }
    return r;
}

INLINE float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v4f32_p0v4i32, )( float4          x,
                                               __private int4* signp )
{
    float4  r;
    const __private float* px = (const __private float*)&x;
    __private int*      sign_scalar = (__private int*)signp;
    __private float*    r_scalar = (__private float*)&r;
    for(uint i = 0; i < 4; i++)
    {
        r_scalar[i] = SPIRV_OCL_BUILTIN(lgamma_r, _f32_p0i32, )( px[i], sign_scalar + i );
    }
    return r;
}

INLINE float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v8f32_p0v8i32, )( float8          x,
                                               __private int8* signp )
{
    float8  r;
    const __private float* px = (const __private float*)&x;
    __private int*      sign_scalar = (__private int*)signp;
    __private float*    r_scalar = (__private float*)&r;
    for(uint i = 0; i < 8; i++)
    {
        r_scalar[i] = SPIRV_OCL_BUILTIN(lgamma_r, _f32_p0i32, )( px[i], sign_scalar + i );
    }
    return r;
}

INLINE float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v16f32_p0v16i32, )( float16          x,
                                                  __private int16* signp )
{
    float16 r;
    const __private float* px = (const __private float*)&x;
    __private int*      sign_scalar = (__private int*)signp;
    __private float*    r_scalar = (__private float*)&r;
    for(uint i = 0; i < 16; i++)
    {
        r_scalar[i] = SPIRV_OCL_BUILTIN(lgamma_r, _f32_p0i32, )( px[i], sign_scalar + i );
    }
    return r;
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

INLINE float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _f32_p4i32, )( float          x,
                                          __generic int* signp )
{
    int     s;
    float   r = SPIRV_OCL_BUILTIN(lgamma_r, _f32_p0i32, )(x, &s);
    signp[0] = s;
    return r;
}

INLINE float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v2f32_p4v2i32, )( float2          x,
                                               __generic int2* signp )
{
    int2    s;
    float2  r = SPIRV_OCL_BUILTIN(lgamma_r, _v2f32_p0v2i32, )(x, &s);
    signp[0] = s;
    return r;
}

INLINE float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v3f32_p4v3i32, )( float3          x,
                                               __generic int3* signp )
{
    int3    s;
    float3  r = SPIRV_OCL_BUILTIN(lgamma_r, _v3f32_p0v3i32, )(x, &s);
    signp[0] = s;
    return r;
}

INLINE float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v4f32_p4v4i32, )( float4          x,
                                               __generic int4* signp )
{
    int4    s;
    float4  r = SPIRV_OCL_BUILTIN(lgamma_r, _v4f32_p0v4i32, )(x, &s);
    signp[0] = s;
    return r;
}

INLINE float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v8f32_p4v8i32, )( float8          x,
                                               __generic int8* signp )
{
    int8    s;
    float8  r = SPIRV_OCL_BUILTIN(lgamma_r, _v8f32_p0v8i32, )(x, &s);
    signp[0] = s;
    return r;
}

INLINE float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v16f32_p4v16i32, )( float16          x,
                                                  __generic int16* signp )
{
    int16   s;
    float16 r = SPIRV_OCL_BUILTIN(lgamma_r, _v16f32_p0v16i32, )(x, &s);
    signp[0] = s;
    return r;
}

#endif //#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#if defined(cl_khr_fp16)

INLINE half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _f16_p1i32, )( half          x,
                                         __global int* signp )
{
    return SPIRV_BUILTIN(FConvert, _f16_f32, _Rhalf)( SPIRV_OCL_BUILTIN(lgamma_r, _f32_p1i32, )( SPIRV_BUILTIN(FConvert, _f32_f16, _Rfloat)(x), signp ) );
}

INLINE half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v2f16_p1v2i32, )( half2          x,
                                              __global int2* signp )
{
    return SPIRV_BUILTIN(FConvert, _v2f16_v2f32, _Rhalf2)( SPIRV_OCL_BUILTIN(lgamma_r, _v2f32_p1v2i32, )( SPIRV_BUILTIN(FConvert, _v2f32_v2f16, _Rfloat2)(x), signp ) );
}

INLINE half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v3f16_p1v3i32, )( half3          x,
                                              __global int3* signp )
{
    return SPIRV_BUILTIN(FConvert, _v3f16_v3f32, _Rhalf3)( SPIRV_OCL_BUILTIN(lgamma_r, _v3f32_p1v3i32, )( SPIRV_BUILTIN(FConvert, _v3f32_v3f16, _Rfloat3)(x), signp ) );
}

INLINE half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v4f16_p1v4i32, )( half4          x,
                                              __global int4* signp )
{
    return SPIRV_BUILTIN(FConvert, _v4f16_v4f32, _Rhalf4)( SPIRV_OCL_BUILTIN(lgamma_r, _v4f32_p1v4i32, )( SPIRV_BUILTIN(FConvert, _v4f32_v4f16, _Rfloat4)(x), signp ) );
}

INLINE half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v8f16_p1v8i32, )( half8          x,
                                              __global int8* signp )
{
    return SPIRV_BUILTIN(FConvert, _v8f16_v8f32, _Rhalf8)( SPIRV_OCL_BUILTIN(lgamma_r, _v8f32_p1v8i32, )( SPIRV_BUILTIN(FConvert, _v8f32_v8f16, _Rfloat8)(x), signp ) );
}

INLINE half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v16f16_p1v16i32, )( half16          x,
                                                 __global int16* signp )
{
    return SPIRV_BUILTIN(FConvert, _v16f16_v16f32, _Rhalf16)( SPIRV_OCL_BUILTIN(lgamma_r, _v16f32_p1v16i32, )( SPIRV_BUILTIN(FConvert, _v16f32_v16f16, _Rfloat16)(x), signp ) );
}

INLINE half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _f16_p0i32, )( half           x,
                                         __private int* signp )
{
    return SPIRV_BUILTIN(FConvert, _f16_f32, _Rhalf)( SPIRV_OCL_BUILTIN(lgamma_r, _f32_p0i32, )( SPIRV_BUILTIN(FConvert, _f32_f16, _Rfloat)(x), signp ) );
}

INLINE half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v2f16_p0v2i32, )( half2           x,
                                              __private int2* signp )
{
    return SPIRV_BUILTIN(FConvert, _v2f16_v2f32, _Rhalf2)( SPIRV_OCL_BUILTIN(lgamma_r, _v2f32_p0v2i32, )( SPIRV_BUILTIN(FConvert, _v2f32_v2f16, _Rfloat2)(x), signp ) );
}

INLINE half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v3f16_p0v3i32, )( half3           x,
                                              __private int3* signp )
{
    return SPIRV_BUILTIN(FConvert, _v3f16_v3f32, _Rhalf3)( SPIRV_OCL_BUILTIN(lgamma_r, _v3f32_p0v3i32, )( SPIRV_BUILTIN(FConvert, _v3f32_v3f16, _Rfloat3)(x), signp ) );
}

INLINE half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v4f16_p0v4i32, )( half4           x,
                                              __private int4* signp )
{
    return SPIRV_BUILTIN(FConvert, _v4f16_v4f32, _Rhalf4)( SPIRV_OCL_BUILTIN(lgamma_r, _v4f32_p0v4i32, )( SPIRV_BUILTIN(FConvert, _v4f32_v4f16, _Rfloat4)(x), signp ) );
}

INLINE half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v8f16_p0v8i32, )( half8           x,
                                              __private int8* signp )
{
    return SPIRV_BUILTIN(FConvert, _v8f16_v8f32, _Rhalf8)( SPIRV_OCL_BUILTIN(lgamma_r, _v8f32_p0v8i32, )( SPIRV_BUILTIN(FConvert, _v8f32_v8f16, _Rfloat8)(x), signp ) );
}

INLINE half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v16f16_p0v16i32, )( half16           x,
                                                 __private int16* signp )
{
    return SPIRV_BUILTIN(FConvert, _v16f16_v16f32, _Rhalf16)( SPIRV_OCL_BUILTIN(lgamma_r, _v16f32_p0v16i32, )( SPIRV_BUILTIN(FConvert, _v16f32_v16f16, _Rfloat16)(x), signp ) );
}

INLINE half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _f16_p3i32, )( half         x,
                                         __local int* signp )
{
    return SPIRV_BUILTIN(FConvert, _f16_f32, _Rhalf)( SPIRV_OCL_BUILTIN(lgamma_r, _f32_p3i32, )( SPIRV_BUILTIN(FConvert, _f32_f16, _Rfloat)(x), signp ) );
}

INLINE half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v2f16_p3v2i32, )( half2         x,
                                              __local int2* signp )
{
    return SPIRV_BUILTIN(FConvert, _v2f16_v2f32, _Rhalf2)( SPIRV_OCL_BUILTIN(lgamma_r, _v2f32_p3v2i32, )( SPIRV_BUILTIN(FConvert, _v2f32_v2f16, _Rfloat2)(x), signp ) );
}

INLINE half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v3f16_p3v3i32, )( half3         x,
                                              __local int3* signp )
{
    return SPIRV_BUILTIN(FConvert, _v3f16_v3f32, _Rhalf3)( SPIRV_OCL_BUILTIN(lgamma_r, _v3f32_p3v3i32, )( SPIRV_BUILTIN(FConvert, _v3f32_v3f16, _Rfloat3)(x), signp ) );
}

INLINE half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v4f16_p3v4i32, )( half4         x,
                                              __local int4* signp )
{
    return SPIRV_BUILTIN(FConvert, _v4f16_v4f32, _Rhalf4)( SPIRV_OCL_BUILTIN(lgamma_r, _v4f32_p3v4i32, )( SPIRV_BUILTIN(FConvert, _v4f32_v4f16, _Rfloat4)(x), signp ) );
}

INLINE half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v8f16_p3v8i32, )( half8         x,
                                              __local int8* signp )
{
    return SPIRV_BUILTIN(FConvert, _v8f16_v8f32, _Rhalf8)( SPIRV_OCL_BUILTIN(lgamma_r, _v8f32_p3v8i32, )( SPIRV_BUILTIN(FConvert, _v8f32_v8f16, _Rfloat8)(x), signp ) );
}

INLINE half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v16f16_p3v16i32, )( half16         x,
                                                 __local int16* signp )
{
    return SPIRV_BUILTIN(FConvert, _v16f16_v16f32, _Rhalf16)( SPIRV_OCL_BUILTIN(lgamma_r, _v16f32_p3v16i32, )( SPIRV_BUILTIN(FConvert, _v16f32_v16f16, _Rfloat16)(x), signp ) );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

INLINE half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _f16_p4i32, )( half           x,
                                         __generic int* signp )
{
    return SPIRV_BUILTIN(FConvert, _f16_f32, _Rhalf)( SPIRV_OCL_BUILTIN(lgamma_r, _f32_p4i32, )( SPIRV_BUILTIN(FConvert, _f32_f16, _Rfloat)(x), signp ) );
}

INLINE half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v2f16_p4v2i32, )( half2           x,
                                              __generic int2* signp )
{
    return SPIRV_BUILTIN(FConvert, _v2f16_v2f32, _Rhalf2)( SPIRV_OCL_BUILTIN(lgamma_r, _v2f32_p4v2i32, )( SPIRV_BUILTIN(FConvert, _v2f32_v2f16, _Rfloat2)(x), signp ) );
}

INLINE half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v3f16_p4v3i32, )( half3           x,
                                              __generic int3* signp )
{
    return SPIRV_BUILTIN(FConvert, _v3f16_v3f32, _Rhalf3)( SPIRV_OCL_BUILTIN(lgamma_r, _v3f32_p4v3i32, )( SPIRV_BUILTIN(FConvert, _v3f32_v3f16, _Rfloat3)(x), signp ) );
}

INLINE half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v4f16_p4v4i32, )( half4           x,
                                              __generic int4* signp )
{
    return SPIRV_BUILTIN(FConvert, _v4f16_v4f32, _Rhalf4)( SPIRV_OCL_BUILTIN(lgamma_r, _v4f32_p4v4i32, )( SPIRV_BUILTIN(FConvert, _v4f32_v4f16, _Rfloat4)(x), signp ) );
}

INLINE half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v8f16_p4v8i32, )( half8           x,
                                              __generic int8* signp )
{
    return SPIRV_BUILTIN(FConvert, _v8f16_v8f32, _Rhalf8)( SPIRV_OCL_BUILTIN(lgamma_r, _v8f32_p4v8i32, )( SPIRV_BUILTIN(FConvert, _v8f32_v8f16, _Rfloat8)(x), signp ) );
}

INLINE half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v16f16_p4v16i32, )( half16           x,
                                                 __generic int16* signp )
{
    return SPIRV_BUILTIN(FConvert, _v16f16_v16f32, _Rhalf16)( SPIRV_OCL_BUILTIN(lgamma_r, _v16f32_p4v16i32, )( SPIRV_BUILTIN(FConvert, _v16f32_v16f16, _Rfloat16)(x), signp ) );
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#endif // (cl_khr_fp16)

#if defined(cl_khr_fp64)

double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _f64_p1i32, )( double        x,
                                           __global int* signp )
{
    return SPIRV_BUILTIN(FConvert, _f64_f32, _Rdouble)( SPIRV_OCL_BUILTIN(lgamma_r, _f32_p1i32, )( SPIRV_BUILTIN(FConvert, _f32_f64, _Rfloat)(x), signp ) );
}

double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v2f64_p1v2i32, )( double2        x,
                                                __global int2* signp )
{
    return SPIRV_BUILTIN(FConvert, _v2f64_v2f32, _Rdouble2)( SPIRV_OCL_BUILTIN(lgamma_r, _v2f32_p1v2i32, )( SPIRV_BUILTIN(FConvert, _v2f32_v2f64, _Rfloat2)(x), signp ) );
}

double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v3f64_p1v3i32, )( double3        x,
                                                __global int3* signp )
{
    return SPIRV_BUILTIN(FConvert, _v3f64_v3f32, _Rdouble3)( SPIRV_OCL_BUILTIN(lgamma_r, _v3f32_p1v3i32, )( SPIRV_BUILTIN(FConvert, _v3f32_v3f64, _Rfloat3)(x), signp ) );
}

double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v4f64_p1v4i32, )( double4        x,
                                                __global int4* signp )
{
    return SPIRV_BUILTIN(FConvert, _v4f64_v4f32, _Rdouble4)( SPIRV_OCL_BUILTIN(lgamma_r, _v4f32_p1v4i32, )( SPIRV_BUILTIN(FConvert, _v4f32_v4f64, _Rfloat4)(x), signp ) );
}

double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v8f64_p1v8i32, )( double8        x,
                                                __global int8* signp )
{
    return SPIRV_BUILTIN(FConvert, _v8f64_v8f32, _Rdouble8)( SPIRV_OCL_BUILTIN(lgamma_r, _v8f32_p1v8i32, )( SPIRV_BUILTIN(FConvert, _v8f32_v8f64, _Rfloat8)(x), signp ) );
}

double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v16f64_p1v16i32, )( double16        x,
                                                   __global int16* signp )
{
    return SPIRV_BUILTIN(FConvert, _v16f64_v16f32, _Rdouble16)( SPIRV_OCL_BUILTIN(lgamma_r, _v16f32_p1v16i32, )( SPIRV_BUILTIN(FConvert, _v16f32_v16f64, _Rfloat16)(x), signp ) );
}

double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _f64_p0i32, )( double         x,
                                           __private int* signp )
{
    return SPIRV_BUILTIN(FConvert, _f64_f32, _Rdouble)( SPIRV_OCL_BUILTIN(lgamma_r, _f32_p0i32, )( SPIRV_BUILTIN(FConvert, _f32_f64, _Rfloat)(x), signp ) );
}

double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v2f64_p0v2i32, )( double2         x,
                                                __private int2* signp )
{
    return SPIRV_BUILTIN(FConvert, _v2f64_v2f32, _Rdouble2)( SPIRV_OCL_BUILTIN(lgamma_r, _v2f32_p0v2i32, )( SPIRV_BUILTIN(FConvert, _v2f32_v2f64, _Rfloat2)(x), signp ) );
}

double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v3f64_p0v3i32, )( double3         x,
                                                __private int3* signp )
{
    return SPIRV_BUILTIN(FConvert, _v3f64_v3f32, _Rdouble3)( SPIRV_OCL_BUILTIN(lgamma_r, _v3f32_p0v3i32, )( SPIRV_BUILTIN(FConvert, _v3f32_v3f64, _Rfloat3)(x), signp ) );
}

double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v4f64_p0v4i32, )( double4         x,
                                                __private int4* signp )
{
    return SPIRV_BUILTIN(FConvert, _v4f64_v4f32, _Rdouble4)( SPIRV_OCL_BUILTIN(lgamma_r, _v4f32_p0v4i32, )( SPIRV_BUILTIN(FConvert, _v4f32_v4f64, _Rfloat4)(x), signp ) );
}

double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v8f64_p0v8i32, )( double8         x,
                                                __private int8* signp )
{
    return SPIRV_BUILTIN(FConvert, _v8f64_v8f32, _Rdouble8)( SPIRV_OCL_BUILTIN(lgamma_r, _v8f32_p0v8i32, )( SPIRV_BUILTIN(FConvert, _v8f32_v8f64, _Rfloat8)(x), signp ) );
}

double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v16f64_p0v16i32, )( double16         x,
                                                   __private int16* signp )
{
    return SPIRV_BUILTIN(FConvert, _v16f64_v16f32, _Rdouble16)( SPIRV_OCL_BUILTIN(lgamma_r, _v16f32_p0v16i32, )( SPIRV_BUILTIN(FConvert, _v16f32_v16f64, _Rfloat16)(x), signp ) );
}

double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _f64_p3i32, )( double       x,
                                           __local int* signp )
{
    return SPIRV_BUILTIN(FConvert, _f64_f32, _Rdouble)( SPIRV_OCL_BUILTIN(lgamma_r, _f32_p3i32, )( SPIRV_BUILTIN(FConvert, _f32_f64, _Rfloat)(x), signp ) );
}

double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v2f64_p3v2i32, )( double2       x,
                                                __local int2* signp )
{
    return SPIRV_BUILTIN(FConvert, _v2f64_v2f32, _Rdouble2)( SPIRV_OCL_BUILTIN(lgamma_r, _v2f32_p3v2i32, )( SPIRV_BUILTIN(FConvert, _v2f32_v2f64, _Rfloat2)(x), signp ) );
}

double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v3f64_p3v3i32, )( double3       x,
                                                __local int3* signp )
{
    return SPIRV_BUILTIN(FConvert, _v3f64_v3f32, _Rdouble3)( SPIRV_OCL_BUILTIN(lgamma_r, _v3f32_p3v3i32, )( SPIRV_BUILTIN(FConvert, _v3f32_v3f64, _Rfloat3)(x), signp ) );
}

double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v4f64_p3v4i32, )( double4       x,
                                                __local int4* signp )
{
    return SPIRV_BUILTIN(FConvert, _v4f64_v4f32, _Rdouble4)( SPIRV_OCL_BUILTIN(lgamma_r, _v4f32_p3v4i32, )( SPIRV_BUILTIN(FConvert, _v4f32_v4f64, _Rfloat4)(x), signp ) );
}

double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v8f64_p3v8i32, )( double8       x,
                                                __local int8* signp )
{
    return SPIRV_BUILTIN(FConvert, _v8f64_v8f32, _Rdouble8)( SPIRV_OCL_BUILTIN(lgamma_r, _v8f32_p3v8i32, )( SPIRV_BUILTIN(FConvert, _v8f32_v8f64, _Rfloat8)(x), signp ) );
}

double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v16f64_p3v16i32, )( double16       x,
                                                   __local int16* signp )
{
    return SPIRV_BUILTIN(FConvert, _v16f64_v16f32, _Rdouble16)( SPIRV_OCL_BUILTIN(lgamma_r, _v16f32_p3v16i32, )( SPIRV_BUILTIN(FConvert, _v16f32_v16f64, _Rfloat16)(x), signp ) );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _f64_p4i32, )( double         x,
                                           __generic int* signp )
{
    return SPIRV_BUILTIN(FConvert, _f64_f32, _Rdouble)( SPIRV_OCL_BUILTIN(lgamma_r, _f32_p4i32, )( SPIRV_BUILTIN(FConvert, _f32_f64, _Rfloat)(x), signp ) );
}

double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v2f64_p4v2i32, )( double2         x,
                                                __generic int2* signp )
{
    return SPIRV_BUILTIN(FConvert, _v2f64_v2f32, _Rdouble2)( SPIRV_OCL_BUILTIN(lgamma_r, _v2f32_p4v2i32, )( SPIRV_BUILTIN(FConvert, _v2f32_v2f64, _Rfloat2)(x), signp ) );
}

double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v3f64_p4v3i32, )( double3         x,
                                                __generic int3* signp )
{
    return SPIRV_BUILTIN(FConvert, _v3f64_v3f32, _Rdouble3)( SPIRV_OCL_BUILTIN(lgamma_r, _v3f32_p4v3i32, )( SPIRV_BUILTIN(FConvert, _v3f32_v3f64, _Rfloat3)(x), signp ) );
}

double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v4f64_p4v4i32, )( double4         x,
                                                __generic int4* signp )
{
    return SPIRV_BUILTIN(FConvert, _v4f64_v4f32, _Rdouble4)( SPIRV_OCL_BUILTIN(lgamma_r, _v4f32_p4v4i32, )( SPIRV_BUILTIN(FConvert, _v4f32_v4f64, _Rfloat4)(x), signp ) );
}

double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v8f64_p4v8i32, )( double8         x,
                                                __generic int8* signp )
{
    return SPIRV_BUILTIN(FConvert, _v8f64_v8f32, _Rdouble8)( SPIRV_OCL_BUILTIN(lgamma_r, _v8f32_p4v8i32, )( SPIRV_BUILTIN(FConvert, _v8f32_v8f64, _Rfloat8)(x), signp ) );
}

double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v16f64_p4v16i32, )( double16         x,
                                                   __generic int16* signp )
{
    return SPIRV_BUILTIN(FConvert, _v16f64_v16f32, _Rdouble16)( SPIRV_OCL_BUILTIN(lgamma_r, _v16f32_p4v16i32, )( SPIRV_BUILTIN(FConvert, _v16f32_v16f64, _Rfloat16)(x), signp ) );
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
#endif // defined(cl_khr_fp64)

/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float __attribute__((overloadable)) __spirv_ocl_lgamma_r( float         x,
                                          __global int* signp )
{
    int     s;
    float   r = __spirv_ocl_lgamma_r(x, &s);
    signp[0] = s;
    return r;
}

INLINE float2 __attribute__((overloadable)) __spirv_ocl_lgamma_r( float2         x,
                                               __global int2* signp )
{
    int2    s;
    float2  r = __spirv_ocl_lgamma_r(x, &s);
    signp[0] = s;
    return r;
}

INLINE float3 __attribute__((overloadable)) __spirv_ocl_lgamma_r( float3         x,
                                               __global int3* signp )
{
    int3    s;
    float3  r = __spirv_ocl_lgamma_r(x, &s);
    signp[0] = s;
    return r;
}

INLINE float4 __attribute__((overloadable)) __spirv_ocl_lgamma_r( float4         x,
                                               __global int4* signp )
{
    int4    s;
    float4  r = __spirv_ocl_lgamma_r(x, &s);
    signp[0] = s;
    return r;
}

INLINE float8 __attribute__((overloadable)) __spirv_ocl_lgamma_r( float8         x,
                                               __global int8* signp )
{
    int8    s;
    float8  r = __spirv_ocl_lgamma_r(x, &s);
    signp[0] = s;
    return r;
}

INLINE float16 __attribute__((overloadable)) __spirv_ocl_lgamma_r( float16         x,
                                                  __global int16* signp )
{
    int16   s;
    float16 r = __spirv_ocl_lgamma_r(x, &s);
    signp[0] = s;
    return r;
}

INLINE float __attribute__((overloadable)) __spirv_ocl_lgamma_r( float        x,
                                          __local int* signp )
{
    int     s;
    float   r = __spirv_ocl_lgamma_r(x, &s);
    signp[0] = s;
    return r;
}

INLINE float2 __attribute__((overloadable)) __spirv_ocl_lgamma_r( float2        x,
                                               __local int2* signp )
{
    int2    s;
    float2  r = __spirv_ocl_lgamma_r(x, &s);
    signp[0] = s;
    return r;
}

INLINE float3 __attribute__((overloadable)) __spirv_ocl_lgamma_r( float3        x,
                                               __local int3* signp )
{
    int3    s;
    float3  r = __spirv_ocl_lgamma_r(x, &s);
    signp[0] = s;
    return r;
}

INLINE float4 __attribute__((overloadable)) __spirv_ocl_lgamma_r( float4        x,
                                               __local int4* signp )
{
    int4    s;
    float4  r = __spirv_ocl_lgamma_r(x, &s);
    signp[0] = s;
    return r;
}

INLINE float8 __attribute__((overloadable)) __spirv_ocl_lgamma_r( float8        x,
                                               __local int8* signp )
{
    int8    s;
    float8  r = __spirv_ocl_lgamma_r(x, &s);
    signp[0] = s;
    return r;
}

INLINE float16 __attribute__((overloadable)) __spirv_ocl_lgamma_r( float16        x,
                                                  __local int16* signp )
{
    int16   s;
    float16 r = __spirv_ocl_lgamma_r(x, &s);
    signp[0] = s;
    return r;
}

INLINE float __attribute__((overloadable)) __spirv_ocl_lgamma_r( float          x,
                                          __private int* signp )
{
    int     s;
    float   r;
    if( __intel_relaxed_isnan(x) )
    {
        r = __spirv_ocl_nan(0);
        s = 0;
    }
    else
    {
        float g = __spirv_ocl_tgamma(x);
        r = __intel_relaxed_isnan(g) ? INFINITY : __spirv_ocl_native_log(__spirv_ocl_fabs(g));
        s = (x == 0) ? 0 : __spirv_ocl_sign(g);
    }
    signp[0] = s;
    return r;
}

INLINE float2 __attribute__((overloadable)) __spirv_ocl_lgamma_r( float2          x,
                                               __private int2* signp )
{
    float2  r;
    const __private float* px = (const __private float*)&x;
    __private int*      sign_scalar = (__private int*)signp;
    __private float*    r_scalar = (__private float*)&r;
    for(uint i = 0; i < 2; i++)
    {
        r_scalar[i] = __spirv_ocl_lgamma_r( px[i], sign_scalar + i );
    }
    return r;
}

INLINE float3 __attribute__((overloadable)) __spirv_ocl_lgamma_r( float3          x,
                                               __private int3* signp )
{
    float3  r;
    const __private float* px = (const __private float*)&x;
    __private int*      sign_scalar = (__private int*)signp;
    __private float*    r_scalar = (__private float*)&r;
    for(uint i = 0; i < 3; i++)
    {
        r_scalar[i] = __spirv_ocl_lgamma_r( px[i], sign_scalar + i );
    }
    return r;
}

INLINE float4 __attribute__((overloadable)) __spirv_ocl_lgamma_r( float4          x,
                                               __private int4* signp )
{
    float4  r;
    const __private float* px = (const __private float*)&x;
    __private int*      sign_scalar = (__private int*)signp;
    __private float*    r_scalar = (__private float*)&r;
    for(uint i = 0; i < 4; i++)
    {
        r_scalar[i] = __spirv_ocl_lgamma_r( px[i], sign_scalar + i );
    }
    return r;
}

INLINE float8 __attribute__((overloadable)) __spirv_ocl_lgamma_r( float8          x,
                                               __private int8* signp )
{
    float8  r;
    const __private float* px = (const __private float*)&x;
    __private int*      sign_scalar = (__private int*)signp;
    __private float*    r_scalar = (__private float*)&r;
    for(uint i = 0; i < 8; i++)
    {
        r_scalar[i] = __spirv_ocl_lgamma_r( px[i], sign_scalar + i );
    }
    return r;
}

INLINE float16 __attribute__((overloadable)) __spirv_ocl_lgamma_r( float16          x,
                                                  __private int16* signp )
{
    float16 r;
    const __private float* px = (const __private float*)&x;
    __private int*      sign_scalar = (__private int*)signp;
    __private float*    r_scalar = (__private float*)&r;
    for(uint i = 0; i < 16; i++)
    {
        r_scalar[i] = __spirv_ocl_lgamma_r( px[i], sign_scalar + i );
    }
    return r;
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

INLINE float __attribute__((overloadable)) __spirv_ocl_lgamma_r( float          x,
                                          __generic int* signp )
{
    int     s;
    float   r = __spirv_ocl_lgamma_r(x, &s);
    signp[0] = s;
    return r;
}

INLINE float2 __attribute__((overloadable)) __spirv_ocl_lgamma_r( float2          x,
                                               __generic int2* signp )
{
    int2    s;
    float2  r = __spirv_ocl_lgamma_r(x, &s);
    signp[0] = s;
    return r;
}

INLINE float3 __attribute__((overloadable)) __spirv_ocl_lgamma_r( float3          x,
                                               __generic int3* signp )
{
    int3    s;
    float3  r = __spirv_ocl_lgamma_r(x, &s);
    signp[0] = s;
    return r;
}

INLINE float4 __attribute__((overloadable)) __spirv_ocl_lgamma_r( float4          x,
                                               __generic int4* signp )
{
    int4    s;
    float4  r = __spirv_ocl_lgamma_r(x, &s);
    signp[0] = s;
    return r;
}

INLINE float8 __attribute__((overloadable)) __spirv_ocl_lgamma_r( float8          x,
                                               __generic int8* signp )
{
    int8    s;
    float8  r = __spirv_ocl_lgamma_r(x, &s);
    signp[0] = s;
    return r;
}

INLINE float16 __attribute__((overloadable)) __spirv_ocl_lgamma_r( float16          x,
                                                  __generic int16* signp )
{
    int16   s;
    float16 r = __spirv_ocl_lgamma_r(x, &s);
    signp[0] = s;
    return r;
}

#endif //#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#if defined(cl_khr_fp16)

INLINE half __attribute__((overloadable)) __spirv_ocl_lgamma_r( half          x,
                                         __global int* signp )
{
    return __spirv_FConvert_Rhalf( __spirv_ocl_lgamma_r( __spirv_FConvert_Rfloat(x), signp ) );
}

INLINE half2 __attribute__((overloadable)) __spirv_ocl_lgamma_r( half2          x,
                                              __global int2* signp )
{
    return __spirv_FConvert_Rhalf2( __spirv_ocl_lgamma_r( __spirv_FConvert_Rfloat2(x), signp ) );
}

INLINE half3 __attribute__((overloadable)) __spirv_ocl_lgamma_r( half3          x,
                                              __global int3* signp )
{
    return __spirv_FConvert_Rhalf3( __spirv_ocl_lgamma_r( __spirv_FConvert_Rfloat3(x), signp ) );
}

INLINE half4 __attribute__((overloadable)) __spirv_ocl_lgamma_r( half4          x,
                                              __global int4* signp )
{
    return __spirv_FConvert_Rhalf4( __spirv_ocl_lgamma_r( __spirv_FConvert_Rfloat4(x), signp ) );
}

INLINE half8 __attribute__((overloadable)) __spirv_ocl_lgamma_r( half8          x,
                                              __global int8* signp )
{
    return __spirv_FConvert_Rhalf8( __spirv_ocl_lgamma_r( __spirv_FConvert_Rfloat8(x), signp ) );
}

INLINE half16 __attribute__((overloadable)) __spirv_ocl_lgamma_r( half16          x,
                                                 __global int16* signp )
{
    return __spirv_FConvert_Rhalf16( __spirv_ocl_lgamma_r( __spirv_FConvert_Rfloat16(x), signp ) );
}

INLINE half __attribute__((overloadable)) __spirv_ocl_lgamma_r( half           x,
                                         __private int* signp )
{
    return __spirv_FConvert_Rhalf( __spirv_ocl_lgamma_r( __spirv_FConvert_Rfloat(x), signp ) );
}

INLINE half2 __attribute__((overloadable)) __spirv_ocl_lgamma_r( half2           x,
                                              __private int2* signp )
{
    return __spirv_FConvert_Rhalf2( __spirv_ocl_lgamma_r( __spirv_FConvert_Rfloat2(x), signp ) );
}

INLINE half3 __attribute__((overloadable)) __spirv_ocl_lgamma_r( half3           x,
                                              __private int3* signp )
{
    return __spirv_FConvert_Rhalf3( __spirv_ocl_lgamma_r( __spirv_FConvert_Rfloat3(x), signp ) );
}

INLINE half4 __attribute__((overloadable)) __spirv_ocl_lgamma_r( half4           x,
                                              __private int4* signp )
{
    return __spirv_FConvert_Rhalf4( __spirv_ocl_lgamma_r( __spirv_FConvert_Rfloat4(x), signp ) );
}

INLINE half8 __attribute__((overloadable)) __spirv_ocl_lgamma_r( half8           x,
                                              __private int8* signp )
{
    return __spirv_FConvert_Rhalf8( __spirv_ocl_lgamma_r( __spirv_FConvert_Rfloat8(x), signp ) );
}

INLINE half16 __attribute__((overloadable)) __spirv_ocl_lgamma_r( half16           x,
                                                 __private int16* signp )
{
    return __spirv_FConvert_Rhalf16( __spirv_ocl_lgamma_r( __spirv_FConvert_Rfloat16(x), signp ) );
}

INLINE half __attribute__((overloadable)) __spirv_ocl_lgamma_r( half         x,
                                         __local int* signp )
{
    return __spirv_FConvert_Rhalf( __spirv_ocl_lgamma_r( __spirv_FConvert_Rfloat(x), signp ) );
}

INLINE half2 __attribute__((overloadable)) __spirv_ocl_lgamma_r( half2         x,
                                              __local int2* signp )
{
    return __spirv_FConvert_Rhalf2( __spirv_ocl_lgamma_r( __spirv_FConvert_Rfloat2(x), signp ) );
}

INLINE half3 __attribute__((overloadable)) __spirv_ocl_lgamma_r( half3         x,
                                              __local int3* signp )
{
    return __spirv_FConvert_Rhalf3( __spirv_ocl_lgamma_r( __spirv_FConvert_Rfloat3(x), signp ) );
}

INLINE half4 __attribute__((overloadable)) __spirv_ocl_lgamma_r( half4         x,
                                              __local int4* signp )
{
    return __spirv_FConvert_Rhalf4( __spirv_ocl_lgamma_r( __spirv_FConvert_Rfloat4(x), signp ) );
}

INLINE half8 __attribute__((overloadable)) __spirv_ocl_lgamma_r( half8         x,
                                              __local int8* signp )
{
    return __spirv_FConvert_Rhalf8( __spirv_ocl_lgamma_r( __spirv_FConvert_Rfloat8(x), signp ) );
}

INLINE half16 __attribute__((overloadable)) __spirv_ocl_lgamma_r( half16         x,
                                                 __local int16* signp )
{
    return __spirv_FConvert_Rhalf16( __spirv_ocl_lgamma_r( __spirv_FConvert_Rfloat16(x), signp ) );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

INLINE half __attribute__((overloadable)) __spirv_ocl_lgamma_r( half           x,
                                         __generic int* signp )
{
    return __spirv_FConvert_Rhalf( __spirv_ocl_lgamma_r( __spirv_FConvert_Rfloat(x), signp ) );
}

INLINE half2 __attribute__((overloadable)) __spirv_ocl_lgamma_r( half2           x,
                                              __generic int2* signp )
{
    return __spirv_FConvert_Rhalf2( __spirv_ocl_lgamma_r( __spirv_FConvert_Rfloat2(x), signp ) );
}

INLINE half3 __attribute__((overloadable)) __spirv_ocl_lgamma_r( half3           x,
                                              __generic int3* signp )
{
    return __spirv_FConvert_Rhalf3( __spirv_ocl_lgamma_r( __spirv_FConvert_Rfloat3(x), signp ) );
}

INLINE half4 __attribute__((overloadable)) __spirv_ocl_lgamma_r( half4           x,
                                              __generic int4* signp )
{
    return __spirv_FConvert_Rhalf4( __spirv_ocl_lgamma_r( __spirv_FConvert_Rfloat4(x), signp ) );
}

INLINE half8 __attribute__((overloadable)) __spirv_ocl_lgamma_r( half8           x,
                                              __generic int8* signp )
{
    return __spirv_FConvert_Rhalf8( __spirv_ocl_lgamma_r( __spirv_FConvert_Rfloat8(x), signp ) );
}

INLINE half16 __attribute__((overloadable)) __spirv_ocl_lgamma_r( half16           x,
                                                 __generic int16* signp )
{
    return __spirv_FConvert_Rhalf16( __spirv_ocl_lgamma_r( __spirv_FConvert_Rfloat16(x), signp ) );
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#endif // (cl_khr_fp16)

#if defined(cl_khr_fp64)

double __attribute__((overloadable)) __spirv_ocl_lgamma_r( double        x,
                                           __global int* signp )
{
    return __spirv_FConvert_Rdouble( __spirv_ocl_lgamma_r( __spirv_FConvert_Rfloat(x), signp ) );
}

double2 __attribute__((overloadable)) __spirv_ocl_lgamma_r( double2        x,
                                                __global int2* signp )
{
    return __spirv_FConvert_Rdouble2( __spirv_ocl_lgamma_r( __spirv_FConvert_Rfloat2(x), signp ) );
}

double3 __attribute__((overloadable)) __spirv_ocl_lgamma_r( double3        x,
                                                __global int3* signp )
{
    return __spirv_FConvert_Rdouble3( __spirv_ocl_lgamma_r( __spirv_FConvert_Rfloat3(x), signp ) );
}

double4 __attribute__((overloadable)) __spirv_ocl_lgamma_r( double4        x,
                                                __global int4* signp )
{
    return __spirv_FConvert_Rdouble4( __spirv_ocl_lgamma_r( __spirv_FConvert_Rfloat4(x), signp ) );
}

double8 __attribute__((overloadable)) __spirv_ocl_lgamma_r( double8        x,
                                                __global int8* signp )
{
    return __spirv_FConvert_Rdouble8( __spirv_ocl_lgamma_r( __spirv_FConvert_Rfloat8(x), signp ) );
}

double16 __attribute__((overloadable)) __spirv_ocl_lgamma_r( double16        x,
                                                   __global int16* signp )
{
    return __spirv_FConvert_Rdouble16( __spirv_ocl_lgamma_r( __spirv_FConvert_Rfloat16(x), signp ) );
}

double __attribute__((overloadable)) __spirv_ocl_lgamma_r( double         x,
                                           __private int* signp )
{
    return __spirv_FConvert_Rdouble( __spirv_ocl_lgamma_r( __spirv_FConvert_Rfloat(x), signp ) );
}

double2 __attribute__((overloadable)) __spirv_ocl_lgamma_r( double2         x,
                                                __private int2* signp )
{
    return __spirv_FConvert_Rdouble2( __spirv_ocl_lgamma_r( __spirv_FConvert_Rfloat2(x), signp ) );
}

double3 __attribute__((overloadable)) __spirv_ocl_lgamma_r( double3         x,
                                                __private int3* signp )
{
    return __spirv_FConvert_Rdouble3( __spirv_ocl_lgamma_r( __spirv_FConvert_Rfloat3(x), signp ) );
}

double4 __attribute__((overloadable)) __spirv_ocl_lgamma_r( double4         x,
                                                __private int4* signp )
{
    return __spirv_FConvert_Rdouble4( __spirv_ocl_lgamma_r( __spirv_FConvert_Rfloat4(x), signp ) );
}

double8 __attribute__((overloadable)) __spirv_ocl_lgamma_r( double8         x,
                                                __private int8* signp )
{
    return __spirv_FConvert_Rdouble8( __spirv_ocl_lgamma_r( __spirv_FConvert_Rfloat8(x), signp ) );
}

double16 __attribute__((overloadable)) __spirv_ocl_lgamma_r( double16         x,
                                                   __private int16* signp )
{
    return __spirv_FConvert_Rdouble16( __spirv_ocl_lgamma_r( __spirv_FConvert_Rfloat16(x), signp ) );
}

double __attribute__((overloadable)) __spirv_ocl_lgamma_r( double       x,
                                           __local int* signp )
{
    return __spirv_FConvert_Rdouble( __spirv_ocl_lgamma_r( __spirv_FConvert_Rfloat(x), signp ) );
}

double2 __attribute__((overloadable)) __spirv_ocl_lgamma_r( double2       x,
                                                __local int2* signp )
{
    return __spirv_FConvert_Rdouble2( __spirv_ocl_lgamma_r( __spirv_FConvert_Rfloat2(x), signp ) );
}

double3 __attribute__((overloadable)) __spirv_ocl_lgamma_r( double3       x,
                                                __local int3* signp )
{
    return __spirv_FConvert_Rdouble3( __spirv_ocl_lgamma_r( __spirv_FConvert_Rfloat3(x), signp ) );
}

double4 __attribute__((overloadable)) __spirv_ocl_lgamma_r( double4       x,
                                                __local int4* signp )
{
    return __spirv_FConvert_Rdouble4( __spirv_ocl_lgamma_r( __spirv_FConvert_Rfloat4(x), signp ) );
}

double8 __attribute__((overloadable)) __spirv_ocl_lgamma_r( double8       x,
                                                __local int8* signp )
{
    return __spirv_FConvert_Rdouble8( __spirv_ocl_lgamma_r( __spirv_FConvert_Rfloat8(x), signp ) );
}

double16 __attribute__((overloadable)) __spirv_ocl_lgamma_r( double16       x,
                                                   __local int16* signp )
{
    return __spirv_FConvert_Rdouble16( __spirv_ocl_lgamma_r( __spirv_FConvert_Rfloat16(x), signp ) );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

double __attribute__((overloadable)) __spirv_ocl_lgamma_r( double         x,
                                           __generic int* signp )
{
    return __spirv_FConvert_Rdouble( __spirv_ocl_lgamma_r( __spirv_FConvert_Rfloat(x), signp ) );
}

double2 __attribute__((overloadable)) __spirv_ocl_lgamma_r( double2         x,
                                                __generic int2* signp )
{
    return __spirv_FConvert_Rdouble2( __spirv_ocl_lgamma_r( __spirv_FConvert_Rfloat2(x), signp ) );
}

double3 __attribute__((overloadable)) __spirv_ocl_lgamma_r( double3         x,
                                                __generic int3* signp )
{
    return __spirv_FConvert_Rdouble3( __spirv_ocl_lgamma_r( __spirv_FConvert_Rfloat3(x), signp ) );
}

double4 __attribute__((overloadable)) __spirv_ocl_lgamma_r( double4         x,
                                                __generic int4* signp )
{
    return __spirv_FConvert_Rdouble4( __spirv_ocl_lgamma_r( __spirv_FConvert_Rfloat4(x), signp ) );
}

double8 __attribute__((overloadable)) __spirv_ocl_lgamma_r( double8         x,
                                                __generic int8* signp )
{
    return __spirv_FConvert_Rdouble8( __spirv_ocl_lgamma_r( __spirv_FConvert_Rfloat8(x), signp ) );
}

double16 __attribute__((overloadable)) __spirv_ocl_lgamma_r( double16         x,
                                                   __generic int16* signp )
{
    return __spirv_FConvert_Rdouble16( __spirv_ocl_lgamma_r( __spirv_FConvert_Rfloat16(x), signp ) );
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
#endif // defined(cl_khr_fp64)

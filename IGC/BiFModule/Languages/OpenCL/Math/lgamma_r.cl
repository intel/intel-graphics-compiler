/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE float OVERLOADABLE lgamma_r( float         x,
                             __global int* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _f32_p1i32, )( x, signp );
}

INLINE float2 OVERLOADABLE lgamma_r( float2         x,
                              __global int2* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _v2f32_p1v2i32, )( x, signp );
}

INLINE float3 OVERLOADABLE lgamma_r( float3         x,
                              __global int3* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _v3f32_p1v3i32, )( x, signp );
}

INLINE float4 OVERLOADABLE lgamma_r( float4         x,
                              __global int4* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _v4f32_p1v4i32, )( x, signp );
}

INLINE float8 OVERLOADABLE lgamma_r( float8         x,
                              __global int8* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _v8f32_p1v8i32, )( x, signp );
}

INLINE float16 OVERLOADABLE lgamma_r( float16         x,
                               __global int16* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _v16f32_p1v16i32, )( x, signp );
}

INLINE float OVERLOADABLE lgamma_r( float        x,
                             __local int* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _f32_p3i32, )( x, signp );
}

INLINE float2 OVERLOADABLE lgamma_r( float2        x,
                              __local int2* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _v2f32_p3v2i32, )( x, signp );
}

INLINE float3 OVERLOADABLE lgamma_r( float3        x,
                              __local int3* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _v3f32_p3v3i32, )( x, signp );
}

INLINE float4 OVERLOADABLE lgamma_r( float4        x,
                              __local int4* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _v4f32_p3v4i32, )( x, signp );
}

INLINE float8 OVERLOADABLE lgamma_r( float8        x,
                              __local int8* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _v8f32_p3v8i32, )( x, signp );
}

INLINE float16 OVERLOADABLE lgamma_r( float16        x,
                               __local int16* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _v16f32_p3v16i32, )( x, signp );
}

INLINE float OVERLOADABLE lgamma_r( float          x,
                             __private int* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _f32_p0i32, )( x, signp );
}

INLINE float2 OVERLOADABLE lgamma_r( float2          x,
                              __private int2* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _v2f32_p0v2i32, )( x, signp );
}

INLINE float3 OVERLOADABLE lgamma_r( float3          x,
                              __private int3* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _v3f32_p0v3i32, )( x, signp );
}

INLINE float4 OVERLOADABLE lgamma_r( float4          x,
                              __private int4* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _v4f32_p0v4i32, )( x, signp );
}

INLINE float8 OVERLOADABLE lgamma_r( float8          x,
                              __private int8* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _v8f32_p0v8i32, )( x, signp );
}

INLINE float16 OVERLOADABLE lgamma_r( float16          x,
                               __private int16* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _v16f32_p0v16i32, )( x, signp );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

INLINE float OVERLOADABLE lgamma_r( float          x,
                             __generic int* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _f32_p4i32, )( x, signp );
}

INLINE float2 OVERLOADABLE lgamma_r( float2          x,
                              __generic int2* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _v2f32_p4v2i32, )( x, signp );
}

INLINE float3 OVERLOADABLE lgamma_r( float3          x,
                              __generic int3* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _v3f32_p4v3i32, )( x, signp );
}

INLINE float4 OVERLOADABLE lgamma_r( float4          x,
                              __generic int4* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _v4f32_p4v4i32, )( x, signp );
}

INLINE float8 OVERLOADABLE lgamma_r( float8          x,
                              __generic int8* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _v8f32_p4v8i32, )( x, signp );
}

INLINE float16 OVERLOADABLE lgamma_r( float16          x,
                               __generic int16* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _v16f32_p4v16i32, )( x, signp );
}

#endif //#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#if defined(cl_khr_fp16)

INLINE half OVERLOADABLE lgamma_r( half          x,
                            __global int* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _f16_p1i32, )( x, signp );
}

INLINE half2 OVERLOADABLE lgamma_r( half2          x,
                             __global int2* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _v2f16_p1v2i32, )( x, signp );
}

INLINE half3 OVERLOADABLE lgamma_r( half3          x,
                             __global int3* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _v3f16_p1v3i32, )( x, signp );
}

INLINE half4 OVERLOADABLE lgamma_r( half4          x,
                             __global int4* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _v4f16_p1v4i32, )( x, signp );
}

INLINE half8 OVERLOADABLE lgamma_r( half8          x,
                             __global int8* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _v8f16_p1v8i32, )( x, signp );
}

INLINE half16 OVERLOADABLE lgamma_r( half16          x,
                              __global int16* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _v16f16_p1v16i32, )( x, signp );
}

INLINE half OVERLOADABLE lgamma_r( half           x,
                            __private int* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _f16_p0i32, )( x, signp );
}

INLINE half2 OVERLOADABLE lgamma_r( half2           x,
                             __private int2* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _v2f16_p0v2i32, )( x, signp );
}

INLINE half3 OVERLOADABLE lgamma_r( half3           x,
                             __private int3* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _v3f16_p0v3i32, )( x, signp );
}

INLINE half4 OVERLOADABLE lgamma_r( half4           x,
                             __private int4* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _v4f16_p0v4i32, )( x, signp );
}

INLINE half8 OVERLOADABLE lgamma_r( half8           x,
                             __private int8* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _v8f16_p0v8i32, )( x, signp );
}

INLINE half16 OVERLOADABLE lgamma_r( half16           x,
                              __private int16* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _v16f16_p0v16i32, )( x, signp );
}

INLINE half OVERLOADABLE lgamma_r( half         x,
                            __local int* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _f16_p3i32, )( x, signp );
}

INLINE half2 OVERLOADABLE lgamma_r( half2         x,
                             __local int2* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _v2f16_p3v2i32, )( x, signp );
}

INLINE half3 OVERLOADABLE lgamma_r( half3         x,
                             __local int3* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _v3f16_p3v3i32, )( x, signp );
}

INLINE half4 OVERLOADABLE lgamma_r( half4         x,
                             __local int4* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _v4f16_p3v4i32, )( x, signp );
}

INLINE half8 OVERLOADABLE lgamma_r( half8         x,
                             __local int8* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _v8f16_p3v8i32, )( x, signp );
}

INLINE half16 OVERLOADABLE lgamma_r( half16         x,
                              __local int16* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _v16f16_p3v16i32, )( x, signp );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

INLINE half OVERLOADABLE lgamma_r( half           x,
                            __generic int* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _f16_p4i32, )( x, signp );
}

INLINE half2 OVERLOADABLE lgamma_r( half2           x,
                             __generic int2* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _v2f16_p4v2i32, )( x, signp );
}

INLINE half3 OVERLOADABLE lgamma_r( half3           x,
                             __generic int3* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _v3f16_p4v3i32, )( x, signp );
}

INLINE half4 OVERLOADABLE lgamma_r( half4           x,
                             __generic int4* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _v4f16_p4v4i32, )( x, signp );
}

INLINE half8 OVERLOADABLE lgamma_r( half8           x,
                             __generic int8* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _v8f16_p4v8i32, )( x, signp );
}

INLINE half16 OVERLOADABLE lgamma_r( half16           x,
                              __generic int16* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _v16f16_p4v16i32, )( x, signp );
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#endif // (cl_khr_fp16)

#if defined(cl_khr_fp64)

double OVERLOADABLE lgamma_r( double        x,
                              __global int* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _f64_p1i32, )( x, signp );
}

double2 OVERLOADABLE lgamma_r( double2        x,
                               __global int2* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _v2f64_p1v2i32, )( x, signp );
}

double3 OVERLOADABLE lgamma_r( double3        x,
                               __global int3* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _v3f64_p1v3i32, )( x, signp );
}

double4 OVERLOADABLE lgamma_r( double4        x,
                               __global int4* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _v4f64_p1v4i32, )( x, signp );
}

double8 OVERLOADABLE lgamma_r( double8        x,
                               __global int8* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _v8f64_p1v8i32, )( x, signp );
}

double16 OVERLOADABLE lgamma_r( double16        x,
                                __global int16* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _v16f64_p1v16i32, )( x, signp );
}

double OVERLOADABLE lgamma_r( double         x,
                              __private int* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _f64_p0i32, )( x, signp );
}

double2 OVERLOADABLE lgamma_r( double2         x,
                               __private int2* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _v2f64_p0v2i32, )( x, signp );
}

double3 OVERLOADABLE lgamma_r( double3         x,
                               __private int3* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _v3f64_p0v3i32, )( x, signp );
}

double4 OVERLOADABLE lgamma_r( double4         x,
                               __private int4* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _v4f64_p0v4i32, )( x, signp );
}

double8 OVERLOADABLE lgamma_r( double8         x,
                               __private int8* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _v8f64_p0v8i32, )( x, signp );
}

double16 OVERLOADABLE lgamma_r( double16         x,
                                __private int16* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _v16f64_p0v16i32, )( x, signp );
}

double OVERLOADABLE lgamma_r( double       x,
                              __local int* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _f64_p3i32, )( x, signp );
}

double2 OVERLOADABLE lgamma_r( double2       x,
                               __local int2* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _v2f64_p3v2i32, )( x, signp );
}

double3 OVERLOADABLE lgamma_r( double3       x,
                               __local int3* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _v3f64_p3v3i32, )( x, signp );
}

double4 OVERLOADABLE lgamma_r( double4       x,
                               __local int4* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _v4f64_p3v4i32, )( x, signp );
}

double8 OVERLOADABLE lgamma_r( double8       x,
                               __local int8* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _v8f64_p3v8i32, )( x, signp );
}

double16 OVERLOADABLE lgamma_r( double16       x,
                                __local int16* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _v16f64_p3v16i32, )( x, signp );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

double OVERLOADABLE lgamma_r( double         x,
                              __generic int* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _f64_p4i32, )( x, signp );
}

double2 OVERLOADABLE lgamma_r( double2         x,
                               __generic int2* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _v2f64_p4v2i32, )( x, signp );
}

double3 OVERLOADABLE lgamma_r( double3         x,
                               __generic int3* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _v3f64_p4v3i32, )( x, signp );
}

double4 OVERLOADABLE lgamma_r( double4         x,
                               __generic int4* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _v4f64_p4v4i32, )( x, signp );
}

double8 OVERLOADABLE lgamma_r( double8         x,
                               __generic int8* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _v8f64_p4v8i32, )( x, signp );
}

double16 OVERLOADABLE lgamma_r( double16         x,
                                __generic int16* signp )
{
    return SPIRV_OCL_BUILTIN(lgamma_r, _v16f64_p4v16i32, )( x, signp );
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
#endif // defined(cl_khr_fp64)

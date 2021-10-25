/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

float OVERLOADABLE frexp( float         x,
                          __global int* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _f32_p1i32, )( x, exp );
}

INLINE float2 OVERLOADABLE frexp( float2         x,
                           __global int2* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _v2f32_p1v2i32, )( x, exp );
}

INLINE float3 OVERLOADABLE frexp( float3         x,
                           __global int3* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _v3f32_p1v3i32, )( x, exp );
}

INLINE float4 OVERLOADABLE frexp( float4         x,
                           __global int4* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _v4f32_p1v4i32, )( x, exp );
}

INLINE float8 OVERLOADABLE frexp( float8         x,
                           __global int8* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _v8f32_p1v8i32, )( x, exp );
}

INLINE float16 OVERLOADABLE frexp( float16         x,
                            __global int16* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _v16f32_p1v16i32, )( x, exp );
}

float OVERLOADABLE frexp( float          x,
                          __private int* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _f32_p0i32, )( x, exp );
}

INLINE float2 OVERLOADABLE frexp( float2          x,
                           __private int2* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _v2f32_p0v2i32, )( x, exp );
}

INLINE float3 OVERLOADABLE frexp( float3          x,
                           __private int3* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _v3f32_p0v3i32, )( x, exp );
}

INLINE float4 OVERLOADABLE frexp( float4          x,
                           __private int4* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _v4f32_p0v4i32, )( x, exp );
}

INLINE float8 OVERLOADABLE frexp( float8          x,
                           __private int8* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _v8f32_p0v8i32, )( x, exp );
}

INLINE float16 OVERLOADABLE frexp( float16          x,
                            __private int16* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _v16f32_p0v16i32, )( x, exp );
}

float OVERLOADABLE frexp( float        x,
                          __local int* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _f32_p3i32, )( x, exp );
}


INLINE float2 OVERLOADABLE frexp( float2        x,
                           __local int2* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _v2f32_p3v2i32, )( x, exp );
}

INLINE float3 OVERLOADABLE frexp( float3        x,
                           __local int3* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _v3f32_p3v3i32, )( x, exp );
}

INLINE float4 OVERLOADABLE frexp( float4        x,
                           __local int4* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _v4f32_p3v4i32, )( x, exp );
}

float8 OVERLOADABLE frexp( float8        x,
                           __local int8* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _v8f32_p3v8i32, )( x, exp );
}

float16 OVERLOADABLE frexp( float16        x,
                            __local int16* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _v16f32_p3v16i32, )( x, exp );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

float OVERLOADABLE frexp( float          x,
                          __generic int* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _f32_p4i32, )( x, exp );
}

INLINE float2 OVERLOADABLE frexp( float2          x,
                           __generic int2* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _v2f32_p4v2i32, )( x, exp );
}

INLINE float3 OVERLOADABLE frexp( float3          x,
                           __generic int3* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _v3f32_p4v3i32, )( x, exp );
}

INLINE float4 OVERLOADABLE frexp( float4          x,
                           __generic int4* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _v4f32_p4v4i32, )( x, exp );
}

INLINE float8 OVERLOADABLE frexp( float8          x,
                           __generic int8* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _v8f32_p4v8i32, )( x, exp );
}

INLINE float16 OVERLOADABLE frexp( float16          x,
                            __generic int16* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _v16f32_p4v16i32, )( x, exp );
}

#endif //#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#ifdef cl_khr_fp16

half OVERLOADABLE frexp( half          x,
                         __global int* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _f16_p1i32, )( x, exp );
}

INLINE half2 OVERLOADABLE frexp( half2          x,
                          __global int2* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _v2f16_p1v2i32, )( x, exp );
}

INLINE half3 OVERLOADABLE frexp( half3          x,
                          __global int3* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _v3f16_p1v3i32, )( x, exp );
}

INLINE half4 OVERLOADABLE frexp( half4          x,
                          __global int4* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _v4f16_p1v4i32, )( x, exp );
}

INLINE half8 OVERLOADABLE frexp( half8          x,
                          __global int8* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _v8f16_p1v8i32, )( x, exp );
}

INLINE half16 OVERLOADABLE frexp( half16          x,
                           __global int16* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _v16f16_p1v16i32, )( x, exp );
}

half OVERLOADABLE frexp( half           x,
                         __private int* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _f16_p0i32, )( x, exp );
}

INLINE half2 OVERLOADABLE frexp( half2           x,
                          __private int2* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _v2f16_p0v2i32, )( x, exp );
}

INLINE half3 OVERLOADABLE frexp( half3           x,
                          __private int3* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _v3f16_p0v3i32, )( x, exp );
}

INLINE half4 OVERLOADABLE frexp( half4           x,
                          __private int4* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _v4f16_p0v4i32, )( x, exp );
}

INLINE half8 OVERLOADABLE frexp( half8           x,
                          __private int8* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _v8f16_p0v8i32, )( x, exp );
}

INLINE half16 OVERLOADABLE frexp( half16           x,
                           __private int16* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _v16f16_p0v16i32, )( x, exp );
}

half OVERLOADABLE frexp( half         x,
                         __local int* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _f16_p3i32, )( x, exp );
}

INLINE half2 OVERLOADABLE frexp( half2         x,
                          __local int2* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _v2f16_p3v2i32, )( x, exp );
}

INLINE half3 OVERLOADABLE frexp( half3         x,
                          __local int3* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _v3f16_p3v3i32, )( x, exp );
}

INLINE half4 OVERLOADABLE frexp( half4         x,
                          __local int4* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _v4f16_p3v4i32, )( x, exp );
}

INLINE half8 OVERLOADABLE frexp( half8         x,
                          __local int8* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _v8f16_p3v8i32, )( x, exp );
}

INLINE half16 OVERLOADABLE frexp( half16         x,
                           __local int16* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _v16f16_p3v16i32, )( x, exp );
}
#endif


#if defined(cl_khr_fp16) && (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

half OVERLOADABLE frexp( half           x,
                         __generic int* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _f16_p4i32, )( x, exp );
}

INLINE half2 OVERLOADABLE frexp( half2           x,
                          __generic int2* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _v2f16_p4v2i32, )( x, exp );
}

INLINE half3 OVERLOADABLE frexp( half3           x,
                          __generic int3* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _v3f16_p4v3i32, )( x, exp );
}

INLINE half4 OVERLOADABLE frexp( half4           x,
                          __generic int4* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _v4f16_p4v4i32, )( x, exp );
}

INLINE half8 OVERLOADABLE frexp( half8           x,
                          __generic int8* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _v8f16_p4v8i32, )( x, exp );
}

INLINE half16 OVERLOADABLE frexp( half16           x,
                           __generic int16* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _v16f16_p4v16i32, )( x, exp );
}
#endif //#if defined(cl_khr_fp16) && (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)


#if defined(cl_khr_fp64)

double OVERLOADABLE frexp( double        x,
                           __global int* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _f64_p1i32, )( x, exp );
}

double2 OVERLOADABLE frexp( double2        x,
                            __global int2* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _v2f64_p1v2i32, )( x, exp );
}

double3 OVERLOADABLE frexp( double3        x,
                            __global int3* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _v3f64_p1v3i32, )( x, exp );
}

double4 OVERLOADABLE frexp( double4        x,
                            __global int4* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _v4f64_p1v4i32, )( x, exp );
}

double8 OVERLOADABLE frexp( double8        x,
                            __global int8* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _v8f64_p1v8i32, )( x, exp );
}

double16 OVERLOADABLE frexp( double16        x,
                             __global int16* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _v16f64_p1v16i32, )( x, exp );
}

double OVERLOADABLE frexp( double         x,
                           __private int* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _f64_p0i32, )( x, exp );
}

double2 OVERLOADABLE frexp( double2         x,
                            __private int2* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _v2f64_p0v2i32, )( x, exp );
}

double3 OVERLOADABLE frexp( double3         x,
                            __private int3* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _v3f64_p0v3i32, )( x, exp );
}

double4 OVERLOADABLE frexp( double4         x,
                            __private int4* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _v4f64_p0v4i32, )( x, exp );
}

double8 OVERLOADABLE frexp( double8         x,
                            __private int8* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _v8f64_p0v8i32, )( x, exp );
}

double16 OVERLOADABLE frexp( double16         x,
                             __private int16* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _v16f64_p0v16i32, )( x, exp );
}

double OVERLOADABLE frexp( double       x,
                           __local int* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _f64_p3i32, )( x, exp );
}

double2 OVERLOADABLE frexp( double2       x,
                            __local int2* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _v2f64_p3v2i32, )( x, exp );
}

double3 OVERLOADABLE frexp( double3       x,
                            __local int3* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _v3f64_p3v3i32, )( x, exp );
}

double4 OVERLOADABLE frexp( double4       x,
                            __local int4* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _v4f64_p3v4i32, )( x, exp );
}

double8 OVERLOADABLE frexp( double8       x,
                            __local int8* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _v8f64_p3v8i32, )( x, exp );
}

double16 OVERLOADABLE frexp( double16       x,
                             __local int16* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _v16f64_p3v16i32, )( x, exp );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
double OVERLOADABLE frexp( double       x,
                           __generic int* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _f64_p4i32, )( x, exp );
}

double2 OVERLOADABLE frexp( double2       x,
                            __generic int2* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _v2f64_p4v2i32, )( x, exp );
}

double3 OVERLOADABLE frexp( double3       x,
                            __generic int3* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _v3f64_p4v3i32, )( x, exp );
}

double4 OVERLOADABLE frexp( double4       x,
                            __generic int4* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _v4f64_p4v4i32, )( x, exp );
}

double8 OVERLOADABLE frexp( double8       x,
                            __generic int8* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _v8f64_p4v8i32, )( x, exp );
}

double16 OVERLOADABLE frexp( double16       x,
                             __generic int16* exp )
{
    return SPIRV_OCL_BUILTIN(frexp, _v16f64_p4v16i32, )( x, exp );
}
#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#endif // defined(cl_khr_fp64)

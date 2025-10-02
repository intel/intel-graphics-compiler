/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

float OVERLOADABLE frexp( float         x,
                          __global int* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

INLINE float2 OVERLOADABLE frexp( float2         x,
                           __global int2* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

INLINE float3 OVERLOADABLE frexp( float3         x,
                           __global int3* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

INLINE float4 OVERLOADABLE frexp( float4         x,
                           __global int4* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

INLINE float8 OVERLOADABLE frexp( float8         x,
                           __global int8* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

INLINE float16 OVERLOADABLE frexp( float16         x,
                            __global int16* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

float OVERLOADABLE frexp( float          x,
                          __private int* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

INLINE float2 OVERLOADABLE frexp( float2          x,
                           __private int2* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

INLINE float3 OVERLOADABLE frexp( float3          x,
                           __private int3* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

INLINE float4 OVERLOADABLE frexp( float4          x,
                           __private int4* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

INLINE float8 OVERLOADABLE frexp( float8          x,
                           __private int8* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

INLINE float16 OVERLOADABLE frexp( float16          x,
                            __private int16* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

float OVERLOADABLE frexp( float        x,
                          __local int* exp )
{
    return __spirv_ocl_frexp( x, exp );
}


INLINE float2 OVERLOADABLE frexp( float2        x,
                           __local int2* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

INLINE float3 OVERLOADABLE frexp( float3        x,
                           __local int3* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

INLINE float4 OVERLOADABLE frexp( float4        x,
                           __local int4* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

float8 OVERLOADABLE frexp( float8        x,
                           __local int8* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

float16 OVERLOADABLE frexp( float16        x,
                            __local int16* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

float OVERLOADABLE frexp( float          x,
                          __generic int* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

INLINE float2 OVERLOADABLE frexp( float2          x,
                           __generic int2* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

INLINE float3 OVERLOADABLE frexp( float3          x,
                           __generic int3* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

INLINE float4 OVERLOADABLE frexp( float4          x,
                           __generic int4* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

INLINE float8 OVERLOADABLE frexp( float8          x,
                           __generic int8* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

INLINE float16 OVERLOADABLE frexp( float16          x,
                            __generic int16* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

#endif //#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#ifdef cl_khr_fp16

half OVERLOADABLE frexp( half          x,
                         __global int* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

INLINE half2 OVERLOADABLE frexp( half2          x,
                          __global int2* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

INLINE half3 OVERLOADABLE frexp( half3          x,
                          __global int3* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

INLINE half4 OVERLOADABLE frexp( half4          x,
                          __global int4* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

INLINE half8 OVERLOADABLE frexp( half8          x,
                          __global int8* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

INLINE half16 OVERLOADABLE frexp( half16          x,
                           __global int16* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

half OVERLOADABLE frexp( half           x,
                         __private int* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

INLINE half2 OVERLOADABLE frexp( half2           x,
                          __private int2* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

INLINE half3 OVERLOADABLE frexp( half3           x,
                          __private int3* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

INLINE half4 OVERLOADABLE frexp( half4           x,
                          __private int4* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

INLINE half8 OVERLOADABLE frexp( half8           x,
                          __private int8* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

INLINE half16 OVERLOADABLE frexp( half16           x,
                           __private int16* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

half OVERLOADABLE frexp( half         x,
                         __local int* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

INLINE half2 OVERLOADABLE frexp( half2         x,
                          __local int2* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

INLINE half3 OVERLOADABLE frexp( half3         x,
                          __local int3* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

INLINE half4 OVERLOADABLE frexp( half4         x,
                          __local int4* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

INLINE half8 OVERLOADABLE frexp( half8         x,
                          __local int8* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

INLINE half16 OVERLOADABLE frexp( half16         x,
                           __local int16* exp )
{
    return __spirv_ocl_frexp( x, exp );
}
#endif


#if defined(cl_khr_fp16) && (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

half OVERLOADABLE frexp( half           x,
                         __generic int* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

INLINE half2 OVERLOADABLE frexp( half2           x,
                          __generic int2* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

INLINE half3 OVERLOADABLE frexp( half3           x,
                          __generic int3* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

INLINE half4 OVERLOADABLE frexp( half4           x,
                          __generic int4* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

INLINE half8 OVERLOADABLE frexp( half8           x,
                          __generic int8* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

INLINE half16 OVERLOADABLE frexp( half16           x,
                           __generic int16* exp )
{
    return __spirv_ocl_frexp( x, exp );
}
#endif //#if defined(cl_khr_fp16) && (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)


#if defined(cl_khr_fp64)

double OVERLOADABLE frexp( double        x,
                           __global int* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

double2 OVERLOADABLE frexp( double2        x,
                            __global int2* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

double3 OVERLOADABLE frexp( double3        x,
                            __global int3* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

double4 OVERLOADABLE frexp( double4        x,
                            __global int4* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

double8 OVERLOADABLE frexp( double8        x,
                            __global int8* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

double16 OVERLOADABLE frexp( double16        x,
                             __global int16* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

double OVERLOADABLE frexp( double         x,
                           __private int* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

double2 OVERLOADABLE frexp( double2         x,
                            __private int2* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

double3 OVERLOADABLE frexp( double3         x,
                            __private int3* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

double4 OVERLOADABLE frexp( double4         x,
                            __private int4* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

double8 OVERLOADABLE frexp( double8         x,
                            __private int8* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

double16 OVERLOADABLE frexp( double16         x,
                             __private int16* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

double OVERLOADABLE frexp( double       x,
                           __local int* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

double2 OVERLOADABLE frexp( double2       x,
                            __local int2* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

double3 OVERLOADABLE frexp( double3       x,
                            __local int3* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

double4 OVERLOADABLE frexp( double4       x,
                            __local int4* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

double8 OVERLOADABLE frexp( double8       x,
                            __local int8* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

double16 OVERLOADABLE frexp( double16       x,
                             __local int16* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
double OVERLOADABLE frexp( double       x,
                           __generic int* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

double2 OVERLOADABLE frexp( double2       x,
                            __generic int2* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

double3 OVERLOADABLE frexp( double3       x,
                            __generic int3* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

double4 OVERLOADABLE frexp( double4       x,
                            __generic int4* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

double8 OVERLOADABLE frexp( double8       x,
                            __generic int8* exp )
{
    return __spirv_ocl_frexp( x, exp );
}

double16 OVERLOADABLE frexp( double16       x,
                             __generic int16* exp )
{
    return __spirv_ocl_frexp( x, exp );
}
#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#endif // defined(cl_khr_fp64)

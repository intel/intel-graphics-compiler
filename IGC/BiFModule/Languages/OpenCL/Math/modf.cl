/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE float OVERLOADABLE modf( float           x,
                         __global float* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

INLINE float2 OVERLOADABLE modf( float2           x,
                          __global float2* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

INLINE float3 OVERLOADABLE modf( float3           x,
                          __global float3* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

INLINE float4 OVERLOADABLE modf( float4           x,
                          __global float4* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

INLINE float8 OVERLOADABLE modf( float8           x,
                          __global float8* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

INLINE float16 OVERLOADABLE modf( float16           x,
                           __global float16* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

INLINE float OVERLOADABLE modf( float            x,
                         __private float* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

INLINE float2 OVERLOADABLE modf( float2            x,
                          __private float2* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

INLINE float3 OVERLOADABLE modf( float3            x,
                          __private float3* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

INLINE float4 OVERLOADABLE modf( float4            x,
                          __private float4* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

INLINE float8 OVERLOADABLE modf( float8            x,
                          __private float8* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

INLINE float16 OVERLOADABLE modf( float16            x,
                           __private float16* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

INLINE float OVERLOADABLE modf( float          x,
                         __local float* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

INLINE float2 OVERLOADABLE modf( float2          x,
                          __local float2* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

INLINE float3 OVERLOADABLE modf( float3          x,
                          __local float3* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

INLINE float4 OVERLOADABLE modf( float4          x,
                          __local float4* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

INLINE float8 OVERLOADABLE modf( float8          x,
                          __local float8* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

INLINE float16 OVERLOADABLE modf( float16          x,
                           __local float16* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

INLINE float OVERLOADABLE modf( float            x,
                         __generic float* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

INLINE float2 OVERLOADABLE modf( float2            x,
                          __generic float2* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

INLINE float3 OVERLOADABLE modf( float3            x,
                          __generic float3* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

INLINE float4 OVERLOADABLE modf( float4            x,
                          __generic float4* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

INLINE float8 OVERLOADABLE modf( float8            x,
                          __generic float8* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

INLINE float16 OVERLOADABLE modf( float16            x,
                           __generic float16* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

#endif //#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#ifdef cl_khr_fp16

INLINE half OVERLOADABLE modf( half           x,
                        __global half* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

INLINE half2 OVERLOADABLE modf( half2           x,
                         __global half2* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

INLINE half3 OVERLOADABLE modf( half3           x,
                         __global half3* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

INLINE half4 OVERLOADABLE modf( half4           x,
                         __global half4* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

INLINE half8 OVERLOADABLE modf( half8           x,
                         __global half8* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

INLINE half16 OVERLOADABLE modf( half16           x,
                          __global half16* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

INLINE half OVERLOADABLE modf( half            x,
                        __private half* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

INLINE half2 OVERLOADABLE modf( half2            x,
                         __private half2* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

INLINE half3 OVERLOADABLE modf( half3            x,
                         __private half3* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

INLINE half4 OVERLOADABLE modf( half4            x,
                         __private half4* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

INLINE half8 OVERLOADABLE modf( half8            x,
                         __private half8* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

INLINE half16 OVERLOADABLE modf( half16            x,
                          __private half16* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

INLINE half OVERLOADABLE modf( half          x,
                        __local half* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

INLINE half2 OVERLOADABLE modf( half2          x,
                         __local half2* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

INLINE half3 OVERLOADABLE modf( half3          x,
                         __local half3* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

INLINE half4 OVERLOADABLE modf( half4          x,
                         __local half4* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

INLINE half8 OVERLOADABLE modf( half8          x,
                         __local half8* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

INLINE half16 OVERLOADABLE modf( half16          x,
                          __local half16* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}
#endif


#if defined(cl_khr_fp16) && (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

INLINE half OVERLOADABLE modf( half            x,
                        __generic half* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

INLINE half2 OVERLOADABLE modf( half2            x,
                         __generic half2* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

INLINE half3 OVERLOADABLE modf( half3            x,
                         __generic half3* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

INLINE half4 OVERLOADABLE modf( half4            x,
                         __generic half4* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

INLINE half8 OVERLOADABLE modf( half8            x,
                         __generic half8* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

INLINE half16 OVERLOADABLE modf( half16            x,
                          __generic half16* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}
#endif //#if defined(cl_khr_fp16) && (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#if defined(cl_khr_fp64)

double OVERLOADABLE modf( double           x,
                          __global double* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

double2 OVERLOADABLE modf( double2           x,
                           __global double2* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

double3 OVERLOADABLE modf( double3           x,
                           __global double3* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

double4 OVERLOADABLE modf( double4           x,
                           __global double4* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

double8 OVERLOADABLE modf( double8           x,
                           __global double8* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

double16 OVERLOADABLE modf( double16           x,
                            __global double16* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

double OVERLOADABLE modf( double            x,
                          __private double* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

double2 OVERLOADABLE modf( double2            x,
                           __private double2* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

double3 OVERLOADABLE modf( double3            x,
                           __private double3* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

double4 OVERLOADABLE modf( double4            x,
                           __private double4* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

double8 OVERLOADABLE modf( double8            x,
                           __private double8* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

double16 OVERLOADABLE modf( double16            x,
                            __private double16* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

double OVERLOADABLE modf( double          x,
                          __local double* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

double2 OVERLOADABLE modf( double2        x,
                         __local double2* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

double3 OVERLOADABLE modf( double3          x,
                           __local double3* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

double4 OVERLOADABLE modf( double4          x,
                           __local double4* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

double8 OVERLOADABLE modf( double8          x,
                           __local double8* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

double16 OVERLOADABLE modf( double16          x,
                            __local double16* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

double OVERLOADABLE modf( double            x,
                          __generic double* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

double2 OVERLOADABLE modf( double2            x,
                           __generic double2* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

double3 OVERLOADABLE modf( double3            x,
                           __generic double3* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

double4 OVERLOADABLE modf( double4            x,
                           __generic double4* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

double8 OVERLOADABLE modf( double8            x,
                           __generic double8* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

double16 OVERLOADABLE modf( double16            x,
                            __generic double16* iptr )
{
    return __spirv_ocl_modf( x, iptr );
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
#endif // defined(cl_khr_fp64)

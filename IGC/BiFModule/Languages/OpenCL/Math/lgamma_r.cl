/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE float OVERLOADABLE lgamma_r( float         x,
                             __global int* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

INLINE float2 OVERLOADABLE lgamma_r( float2         x,
                              __global int2* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

INLINE float3 OVERLOADABLE lgamma_r( float3         x,
                              __global int3* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

INLINE float4 OVERLOADABLE lgamma_r( float4         x,
                              __global int4* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

INLINE float8 OVERLOADABLE lgamma_r( float8         x,
                              __global int8* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

INLINE float16 OVERLOADABLE lgamma_r( float16         x,
                               __global int16* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

INLINE float OVERLOADABLE lgamma_r( float        x,
                             __local int* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

INLINE float2 OVERLOADABLE lgamma_r( float2        x,
                              __local int2* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

INLINE float3 OVERLOADABLE lgamma_r( float3        x,
                              __local int3* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

INLINE float4 OVERLOADABLE lgamma_r( float4        x,
                              __local int4* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

INLINE float8 OVERLOADABLE lgamma_r( float8        x,
                              __local int8* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

INLINE float16 OVERLOADABLE lgamma_r( float16        x,
                               __local int16* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

INLINE float OVERLOADABLE lgamma_r( float          x,
                             __private int* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

INLINE float2 OVERLOADABLE lgamma_r( float2          x,
                              __private int2* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

INLINE float3 OVERLOADABLE lgamma_r( float3          x,
                              __private int3* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

INLINE float4 OVERLOADABLE lgamma_r( float4          x,
                              __private int4* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

INLINE float8 OVERLOADABLE lgamma_r( float8          x,
                              __private int8* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

INLINE float16 OVERLOADABLE lgamma_r( float16          x,
                               __private int16* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

INLINE float OVERLOADABLE lgamma_r( float          x,
                             __generic int* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

INLINE float2 OVERLOADABLE lgamma_r( float2          x,
                              __generic int2* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

INLINE float3 OVERLOADABLE lgamma_r( float3          x,
                              __generic int3* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

INLINE float4 OVERLOADABLE lgamma_r( float4          x,
                              __generic int4* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

INLINE float8 OVERLOADABLE lgamma_r( float8          x,
                              __generic int8* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

INLINE float16 OVERLOADABLE lgamma_r( float16          x,
                               __generic int16* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

#endif //#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#if defined(cl_khr_fp16)

INLINE half OVERLOADABLE lgamma_r( half          x,
                            __global int* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

INLINE half2 OVERLOADABLE lgamma_r( half2          x,
                             __global int2* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

INLINE half3 OVERLOADABLE lgamma_r( half3          x,
                             __global int3* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

INLINE half4 OVERLOADABLE lgamma_r( half4          x,
                             __global int4* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

INLINE half8 OVERLOADABLE lgamma_r( half8          x,
                             __global int8* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

INLINE half16 OVERLOADABLE lgamma_r( half16          x,
                              __global int16* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

INLINE half OVERLOADABLE lgamma_r( half           x,
                            __private int* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

INLINE half2 OVERLOADABLE lgamma_r( half2           x,
                             __private int2* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

INLINE half3 OVERLOADABLE lgamma_r( half3           x,
                             __private int3* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

INLINE half4 OVERLOADABLE lgamma_r( half4           x,
                             __private int4* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

INLINE half8 OVERLOADABLE lgamma_r( half8           x,
                             __private int8* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

INLINE half16 OVERLOADABLE lgamma_r( half16           x,
                              __private int16* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

INLINE half OVERLOADABLE lgamma_r( half         x,
                            __local int* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

INLINE half2 OVERLOADABLE lgamma_r( half2         x,
                             __local int2* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

INLINE half3 OVERLOADABLE lgamma_r( half3         x,
                             __local int3* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

INLINE half4 OVERLOADABLE lgamma_r( half4         x,
                             __local int4* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

INLINE half8 OVERLOADABLE lgamma_r( half8         x,
                             __local int8* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

INLINE half16 OVERLOADABLE lgamma_r( half16         x,
                              __local int16* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

INLINE half OVERLOADABLE lgamma_r( half           x,
                            __generic int* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

INLINE half2 OVERLOADABLE lgamma_r( half2           x,
                             __generic int2* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

INLINE half3 OVERLOADABLE lgamma_r( half3           x,
                             __generic int3* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

INLINE half4 OVERLOADABLE lgamma_r( half4           x,
                             __generic int4* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

INLINE half8 OVERLOADABLE lgamma_r( half8           x,
                             __generic int8* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

INLINE half16 OVERLOADABLE lgamma_r( half16           x,
                              __generic int16* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#endif // (cl_khr_fp16)

#if defined(cl_khr_fp64)

double OVERLOADABLE lgamma_r( double        x,
                              __global int* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

double2 OVERLOADABLE lgamma_r( double2        x,
                               __global int2* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

double3 OVERLOADABLE lgamma_r( double3        x,
                               __global int3* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

double4 OVERLOADABLE lgamma_r( double4        x,
                               __global int4* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

double8 OVERLOADABLE lgamma_r( double8        x,
                               __global int8* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

double16 OVERLOADABLE lgamma_r( double16        x,
                                __global int16* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

double OVERLOADABLE lgamma_r( double         x,
                              __private int* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

double2 OVERLOADABLE lgamma_r( double2         x,
                               __private int2* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

double3 OVERLOADABLE lgamma_r( double3         x,
                               __private int3* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

double4 OVERLOADABLE lgamma_r( double4         x,
                               __private int4* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

double8 OVERLOADABLE lgamma_r( double8         x,
                               __private int8* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

double16 OVERLOADABLE lgamma_r( double16         x,
                                __private int16* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

double OVERLOADABLE lgamma_r( double       x,
                              __local int* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

double2 OVERLOADABLE lgamma_r( double2       x,
                               __local int2* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

double3 OVERLOADABLE lgamma_r( double3       x,
                               __local int3* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

double4 OVERLOADABLE lgamma_r( double4       x,
                               __local int4* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

double8 OVERLOADABLE lgamma_r( double8       x,
                               __local int8* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

double16 OVERLOADABLE lgamma_r( double16       x,
                                __local int16* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

double OVERLOADABLE lgamma_r( double         x,
                              __generic int* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

double2 OVERLOADABLE lgamma_r( double2         x,
                               __generic int2* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

double3 OVERLOADABLE lgamma_r( double3         x,
                               __generic int3* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

double4 OVERLOADABLE lgamma_r( double4         x,
                               __generic int4* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

double8 OVERLOADABLE lgamma_r( double8         x,
                               __generic int8* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

double16 OVERLOADABLE lgamma_r( double16         x,
                                __generic int16* signp )
{
    return __spirv_ocl_lgamma_r( x, signp );
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
#endif // defined(cl_khr_fp64)

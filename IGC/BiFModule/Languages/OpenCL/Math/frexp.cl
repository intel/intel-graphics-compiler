/*========================== begin_copyright_notice ============================

Copyright (c) 2016-2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

float OVERLOADABLE frexp( float         x,
                          __global int* exp )
{
    return __builtin_spirv_OpenCL_frexp_f32_p1i32( x, exp );
}

INLINE float2 OVERLOADABLE frexp( float2         x,
                           __global int2* exp )
{
    return __builtin_spirv_OpenCL_frexp_v2f32_p1v2i32( x, exp );
}

INLINE float3 OVERLOADABLE frexp( float3         x,
                           __global int3* exp )
{
    return __builtin_spirv_OpenCL_frexp_v3f32_p1v3i32( x, exp );
}

INLINE float4 OVERLOADABLE frexp( float4         x,
                           __global int4* exp )
{
    return __builtin_spirv_OpenCL_frexp_v4f32_p1v4i32( x, exp );
}

INLINE float8 OVERLOADABLE frexp( float8         x,
                           __global int8* exp )
{
    return __builtin_spirv_OpenCL_frexp_v8f32_p1v8i32( x, exp );
}

INLINE float16 OVERLOADABLE frexp( float16         x,
                            __global int16* exp )
{
    return __builtin_spirv_OpenCL_frexp_v16f32_p1v16i32( x, exp );
}

float OVERLOADABLE frexp( float          x,
                          __private int* exp )
{
    return __builtin_spirv_OpenCL_frexp_f32_p0i32( x, exp );
}

INLINE float2 OVERLOADABLE frexp( float2          x,
                           __private int2* exp )
{
    return __builtin_spirv_OpenCL_frexp_v2f32_p0v2i32( x, exp );
}

INLINE float3 OVERLOADABLE frexp( float3          x,
                           __private int3* exp )
{
    return __builtin_spirv_OpenCL_frexp_v3f32_p0v3i32( x, exp );
}

INLINE float4 OVERLOADABLE frexp( float4          x,
                           __private int4* exp )
{
    return __builtin_spirv_OpenCL_frexp_v4f32_p0v4i32( x, exp );
}

INLINE float8 OVERLOADABLE frexp( float8          x,
                           __private int8* exp )
{
    return __builtin_spirv_OpenCL_frexp_v8f32_p0v8i32( x, exp );
}

INLINE float16 OVERLOADABLE frexp( float16          x,
                            __private int16* exp )
{
    return __builtin_spirv_OpenCL_frexp_v16f32_p0v16i32( x, exp );
}

float OVERLOADABLE frexp( float        x,
                          __local int* exp )
{
    return __builtin_spirv_OpenCL_frexp_f32_p3i32( x, exp );
}


INLINE float2 OVERLOADABLE frexp( float2        x,
                           __local int2* exp )
{
    return __builtin_spirv_OpenCL_frexp_v2f32_p3v2i32( x, exp );
}

INLINE float3 OVERLOADABLE frexp( float3        x,
                           __local int3* exp )
{
    return __builtin_spirv_OpenCL_frexp_v3f32_p3v3i32( x, exp );
}

INLINE float4 OVERLOADABLE frexp( float4        x,
                           __local int4* exp )
{
    return __builtin_spirv_OpenCL_frexp_v4f32_p3v4i32( x, exp );
}

float8 OVERLOADABLE frexp( float8        x,
                           __local int8* exp )
{
    return __builtin_spirv_OpenCL_frexp_v8f32_p3v8i32( x, exp );
}

float16 OVERLOADABLE frexp( float16        x,
                            __local int16* exp )
{
    return __builtin_spirv_OpenCL_frexp_v16f32_p3v16i32( x, exp );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

float OVERLOADABLE frexp( float          x,
                          __generic int* exp )
{
    return __builtin_spirv_OpenCL_frexp_f32_p4i32( x, exp );
}

INLINE float2 OVERLOADABLE frexp( float2          x,
                           __generic int2* exp )
{
    return __builtin_spirv_OpenCL_frexp_v2f32_p4v2i32( x, exp );
}

INLINE float3 OVERLOADABLE frexp( float3          x,
                           __generic int3* exp )
{
    return __builtin_spirv_OpenCL_frexp_v3f32_p4v3i32( x, exp );
}

INLINE float4 OVERLOADABLE frexp( float4          x,
                           __generic int4* exp )
{
    return __builtin_spirv_OpenCL_frexp_v4f32_p4v4i32( x, exp );
}

INLINE float8 OVERLOADABLE frexp( float8          x,
                           __generic int8* exp )
{
    return __builtin_spirv_OpenCL_frexp_v8f32_p4v8i32( x, exp );
}

INLINE float16 OVERLOADABLE frexp( float16          x,
                            __generic int16* exp )
{
    return __builtin_spirv_OpenCL_frexp_v16f32_p4v16i32( x, exp );
}

#endif //#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#ifdef cl_khr_fp16

half OVERLOADABLE frexp( half          x,
                         __global int* exp )
{
    return __builtin_spirv_OpenCL_frexp_f16_p1i32( x, exp );
}

INLINE half2 OVERLOADABLE frexp( half2          x,
                          __global int2* exp )
{
    return __builtin_spirv_OpenCL_frexp_v2f16_p1v2i32( x, exp );
}

INLINE half3 OVERLOADABLE frexp( half3          x,
                          __global int3* exp )
{
    return __builtin_spirv_OpenCL_frexp_v3f16_p1v3i32( x, exp );
}

INLINE half4 OVERLOADABLE frexp( half4          x,
                          __global int4* exp )
{
    return __builtin_spirv_OpenCL_frexp_v4f16_p1v4i32( x, exp );
}

INLINE half8 OVERLOADABLE frexp( half8          x,
                          __global int8* exp )
{
    return __builtin_spirv_OpenCL_frexp_v8f16_p1v8i32( x, exp );
}

INLINE half16 OVERLOADABLE frexp( half16          x,
                           __global int16* exp )
{
    return __builtin_spirv_OpenCL_frexp_v16f16_p1v16i32( x, exp );
}

half OVERLOADABLE frexp( half           x,
                         __private int* exp )
{
    return __builtin_spirv_OpenCL_frexp_f16_p0i32( x, exp );
}

INLINE half2 OVERLOADABLE frexp( half2           x,
                          __private int2* exp )
{
    return __builtin_spirv_OpenCL_frexp_v2f16_p0v2i32( x, exp );
}

INLINE half3 OVERLOADABLE frexp( half3           x,
                          __private int3* exp )
{
    return __builtin_spirv_OpenCL_frexp_v3f16_p0v3i32( x, exp );
}

INLINE half4 OVERLOADABLE frexp( half4           x,
                          __private int4* exp )
{
    return __builtin_spirv_OpenCL_frexp_v4f16_p0v4i32( x, exp );
}

INLINE half8 OVERLOADABLE frexp( half8           x,
                          __private int8* exp )
{
    return __builtin_spirv_OpenCL_frexp_v8f16_p0v8i32( x, exp );
}

INLINE half16 OVERLOADABLE frexp( half16           x,
                           __private int16* exp )
{
    return __builtin_spirv_OpenCL_frexp_v16f16_p0v16i32( x, exp );
}

half OVERLOADABLE frexp( half         x,
                         __local int* exp )
{
    return __builtin_spirv_OpenCL_frexp_f16_p3i32( x, exp );
}

INLINE half2 OVERLOADABLE frexp( half2         x,
                          __local int2* exp )
{
    return __builtin_spirv_OpenCL_frexp_v2f16_p3v2i32( x, exp );
}

INLINE half3 OVERLOADABLE frexp( half3         x,
                          __local int3* exp )
{
    return __builtin_spirv_OpenCL_frexp_v3f16_p3v3i32( x, exp );
}

INLINE half4 OVERLOADABLE frexp( half4         x,
                          __local int4* exp )
{
    return __builtin_spirv_OpenCL_frexp_v4f16_p3v4i32( x, exp );
}

INLINE half8 OVERLOADABLE frexp( half8         x,
                          __local int8* exp )
{
    return __builtin_spirv_OpenCL_frexp_v8f16_p3v8i32( x, exp );
}

INLINE half16 OVERLOADABLE frexp( half16         x,
                           __local int16* exp )
{
    return __builtin_spirv_OpenCL_frexp_v16f16_p3v16i32( x, exp );
}
#endif


#if defined(cl_khr_fp16) && (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

half OVERLOADABLE frexp( half           x,
                         __generic int* exp )
{
    return __builtin_spirv_OpenCL_frexp_f16_p4i32( x, exp );
}

INLINE half2 OVERLOADABLE frexp( half2           x,
                          __generic int2* exp )
{
    return __builtin_spirv_OpenCL_frexp_v2f16_p4v2i32( x, exp );
}

INLINE half3 OVERLOADABLE frexp( half3           x,
                          __generic int3* exp )
{
    return __builtin_spirv_OpenCL_frexp_v3f16_p4v3i32( x, exp );
}

INLINE half4 OVERLOADABLE frexp( half4           x,
                          __generic int4* exp )
{
    return __builtin_spirv_OpenCL_frexp_v4f16_p4v4i32( x, exp );
}

INLINE half8 OVERLOADABLE frexp( half8           x,
                          __generic int8* exp )
{
    return __builtin_spirv_OpenCL_frexp_v8f16_p4v8i32( x, exp );
}

INLINE half16 OVERLOADABLE frexp( half16           x,
                           __generic int16* exp )
{
    return __builtin_spirv_OpenCL_frexp_v16f16_p4v16i32( x, exp );
}
#endif //#if defined(cl_khr_fp16) && (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)


#if defined(cl_khr_fp64)

double OVERLOADABLE frexp( double        x,
                           __global int* exp )
{
    return __builtin_spirv_OpenCL_frexp_f64_p1i32( x, exp );
}

double2 OVERLOADABLE frexp( double2        x,
                            __global int2* exp )
{
    return __builtin_spirv_OpenCL_frexp_v2f64_p1v2i32( x, exp );
}

double3 OVERLOADABLE frexp( double3        x,
                            __global int3* exp )
{
    return __builtin_spirv_OpenCL_frexp_v3f64_p1v3i32( x, exp );
}

double4 OVERLOADABLE frexp( double4        x,
                            __global int4* exp )
{
    return __builtin_spirv_OpenCL_frexp_v4f64_p1v4i32( x, exp );
}

double8 OVERLOADABLE frexp( double8        x,
                            __global int8* exp )
{
    return __builtin_spirv_OpenCL_frexp_v8f64_p1v8i32( x, exp );
}

double16 OVERLOADABLE frexp( double16        x,
                             __global int16* exp )
{
    return __builtin_spirv_OpenCL_frexp_v16f64_p1v16i32( x, exp );
}

double OVERLOADABLE frexp( double         x,
                           __private int* exp )
{
    return __builtin_spirv_OpenCL_frexp_f64_p0i32( x, exp );
}

double2 OVERLOADABLE frexp( double2         x,
                            __private int2* exp )
{
    return __builtin_spirv_OpenCL_frexp_v2f64_p0v2i32( x, exp );
}

double3 OVERLOADABLE frexp( double3         x,
                            __private int3* exp )
{
    return __builtin_spirv_OpenCL_frexp_v3f64_p0v3i32( x, exp );
}

double4 OVERLOADABLE frexp( double4         x,
                            __private int4* exp )
{
    return __builtin_spirv_OpenCL_frexp_v4f64_p0v4i32( x, exp );
}

double8 OVERLOADABLE frexp( double8         x,
                            __private int8* exp )
{
    return __builtin_spirv_OpenCL_frexp_v8f64_p0v8i32( x, exp );
}

double16 OVERLOADABLE frexp( double16         x,
                             __private int16* exp )
{
    return __builtin_spirv_OpenCL_frexp_v16f64_p0v16i32( x, exp );
}

double OVERLOADABLE frexp( double       x,
                           __local int* exp )
{
    return __builtin_spirv_OpenCL_frexp_f64_p3i32( x, exp );
}

double2 OVERLOADABLE frexp( double2       x,
                            __local int2* exp )
{
    return __builtin_spirv_OpenCL_frexp_v2f64_p3v2i32( x, exp );
}

double3 OVERLOADABLE frexp( double3       x,
                            __local int3* exp )
{
    return __builtin_spirv_OpenCL_frexp_v3f64_p3v3i32( x, exp );
}

double4 OVERLOADABLE frexp( double4       x,
                            __local int4* exp )
{
    return __builtin_spirv_OpenCL_frexp_v4f64_p3v4i32( x, exp );
}

double8 OVERLOADABLE frexp( double8       x,
                            __local int8* exp )
{
    return __builtin_spirv_OpenCL_frexp_v8f64_p3v8i32( x, exp );
}

double16 OVERLOADABLE frexp( double16       x,
                             __local int16* exp )
{
    return __builtin_spirv_OpenCL_frexp_v16f64_p3v16i32( x, exp );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
double OVERLOADABLE frexp( double       x,
                           __generic int* exp )
{
    return __builtin_spirv_OpenCL_frexp_f64_p4i32( x, exp );
}

double2 OVERLOADABLE frexp( double2       x,
                            __generic int2* exp )
{
    return __builtin_spirv_OpenCL_frexp_v2f64_p4v2i32( x, exp );
}

double3 OVERLOADABLE frexp( double3       x,
                            __generic int3* exp )
{
    return __builtin_spirv_OpenCL_frexp_v3f64_p4v3i32( x, exp );
}

double4 OVERLOADABLE frexp( double4       x,
                            __generic int4* exp )
{
    return __builtin_spirv_OpenCL_frexp_v4f64_p4v4i32( x, exp );
}

double8 OVERLOADABLE frexp( double8       x,
                            __generic int8* exp )
{
    return __builtin_spirv_OpenCL_frexp_v8f64_p4v8i32( x, exp );
}

double16 OVERLOADABLE frexp( double16       x,
                             __generic int16* exp )
{
    return __builtin_spirv_OpenCL_frexp_v16f64_p4v16i32( x, exp );
}
#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#endif // defined(cl_khr_fp64)

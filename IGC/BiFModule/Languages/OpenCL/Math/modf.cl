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

INLINE float OVERLOADABLE modf( float           x,
                         __global float* iptr )
{
    return __builtin_spirv_OpenCL_modf_f32_p1f32( x, iptr );
}

INLINE float2 OVERLOADABLE modf( float2           x,
                          __global float2* iptr )
{
    return __builtin_spirv_OpenCL_modf_v2f32_p1v2f32( x, iptr );
}

INLINE float3 OVERLOADABLE modf( float3           x,
                          __global float3* iptr )
{
    return __builtin_spirv_OpenCL_modf_v3f32_p1v3f32( x, iptr );
}

INLINE float4 OVERLOADABLE modf( float4           x,
                          __global float4* iptr )
{
    return __builtin_spirv_OpenCL_modf_v4f32_p1v4f32( x, iptr );
}

INLINE float8 OVERLOADABLE modf( float8           x,
                          __global float8* iptr )
{
    return __builtin_spirv_OpenCL_modf_v8f32_p1v8f32( x, iptr );
}

INLINE float16 OVERLOADABLE modf( float16           x,
                           __global float16* iptr )
{
    return __builtin_spirv_OpenCL_modf_v16f32_p1v16f32( x, iptr );
}

INLINE float OVERLOADABLE modf( float            x,
                         __private float* iptr )
{
    return __builtin_spirv_OpenCL_modf_f32_p0f32( x, iptr );
}

INLINE float2 OVERLOADABLE modf( float2            x,
                          __private float2* iptr )
{
    return __builtin_spirv_OpenCL_modf_v2f32_p0v2f32( x, iptr );
}

INLINE float3 OVERLOADABLE modf( float3            x,
                          __private float3* iptr )
{
    return __builtin_spirv_OpenCL_modf_v3f32_p0v3f32( x, iptr );
}

INLINE float4 OVERLOADABLE modf( float4            x,
                          __private float4* iptr )
{
    return __builtin_spirv_OpenCL_modf_v4f32_p0v4f32( x, iptr );
}

INLINE float8 OVERLOADABLE modf( float8            x,
                          __private float8* iptr )
{
    return __builtin_spirv_OpenCL_modf_v8f32_p0v8f32( x, iptr );
}

INLINE float16 OVERLOADABLE modf( float16            x,
                           __private float16* iptr )
{
    return __builtin_spirv_OpenCL_modf_v16f32_p0v16f32( x, iptr );
}

INLINE float OVERLOADABLE modf( float          x,
                         __local float* iptr )
{
    return __builtin_spirv_OpenCL_modf_f32_p3f32( x, iptr );
}

INLINE float2 OVERLOADABLE modf( float2          x,
                          __local float2* iptr )
{
    return __builtin_spirv_OpenCL_modf_v2f32_p3v2f32( x, iptr );
}

INLINE float3 OVERLOADABLE modf( float3          x,
                          __local float3* iptr )
{
    return __builtin_spirv_OpenCL_modf_v3f32_p3v3f32( x, iptr );
}

INLINE float4 OVERLOADABLE modf( float4          x,
                          __local float4* iptr )
{
    return __builtin_spirv_OpenCL_modf_v4f32_p3v4f32( x, iptr );
}

INLINE float8 OVERLOADABLE modf( float8          x,
                          __local float8* iptr )
{
    return __builtin_spirv_OpenCL_modf_v8f32_p3v8f32( x, iptr );
}

INLINE float16 OVERLOADABLE modf( float16          x,
                           __local float16* iptr )
{
    return __builtin_spirv_OpenCL_modf_v16f32_p3v16f32( x, iptr );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

INLINE float OVERLOADABLE modf( float            x,
                         __generic float* iptr )
{
    return __builtin_spirv_OpenCL_modf_f32_p4f32( x, iptr );
}

INLINE float2 OVERLOADABLE modf( float2            x,
                          __generic float2* iptr )
{
    return __builtin_spirv_OpenCL_modf_v2f32_p4v2f32( x, iptr );
}

INLINE float3 OVERLOADABLE modf( float3            x,
                          __generic float3* iptr )
{
    return __builtin_spirv_OpenCL_modf_v3f32_p4v3f32( x, iptr );
}

INLINE float4 OVERLOADABLE modf( float4            x,
                          __generic float4* iptr )
{
    return __builtin_spirv_OpenCL_modf_v4f32_p4v4f32( x, iptr );
}

INLINE float8 OVERLOADABLE modf( float8            x,
                          __generic float8* iptr )
{
    return __builtin_spirv_OpenCL_modf_v8f32_p4v8f32( x, iptr );
}

INLINE float16 OVERLOADABLE modf( float16            x,
                           __generic float16* iptr )
{
    return __builtin_spirv_OpenCL_modf_v16f32_p4v16f32( x, iptr );
}

#endif //#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#ifdef cl_khr_fp16

INLINE half OVERLOADABLE modf( half           x,
                        __global half* iptr )
{
    return __builtin_spirv_OpenCL_modf_f16_p1f16( x, iptr );
}

INLINE half2 OVERLOADABLE modf( half2           x,
                         __global half2* iptr )
{
    return __builtin_spirv_OpenCL_modf_v2f16_p1v2f16( x, iptr );
}

INLINE half3 OVERLOADABLE modf( half3           x,
                         __global half3* iptr )
{
    return __builtin_spirv_OpenCL_modf_v3f16_p1v3f16( x, iptr );
}

INLINE half4 OVERLOADABLE modf( half4           x,
                         __global half4* iptr )
{
    return __builtin_spirv_OpenCL_modf_v4f16_p1v4f16( x, iptr );
}

INLINE half8 OVERLOADABLE modf( half8           x,
                         __global half8* iptr )
{
    return __builtin_spirv_OpenCL_modf_v8f16_p1v8f16( x, iptr );
}

INLINE half16 OVERLOADABLE modf( half16           x,
                          __global half16* iptr )
{
    return __builtin_spirv_OpenCL_modf_v16f16_p1v16f16( x, iptr );
}

INLINE half OVERLOADABLE modf( half            x,
                        __private half* iptr )
{
    return __builtin_spirv_OpenCL_modf_f16_p0f16( x, iptr );
}

INLINE half2 OVERLOADABLE modf( half2            x,
                         __private half2* iptr )
{
    return __builtin_spirv_OpenCL_modf_v2f16_p0v2f16( x, iptr );
}

INLINE half3 OVERLOADABLE modf( half3            x,
                         __private half3* iptr )
{
    return __builtin_spirv_OpenCL_modf_v3f16_p0v3f16( x, iptr );
}

INLINE half4 OVERLOADABLE modf( half4            x,
                         __private half4* iptr )
{
    return __builtin_spirv_OpenCL_modf_v4f16_p0v4f16( x, iptr );
}

INLINE half8 OVERLOADABLE modf( half8            x,
                         __private half8* iptr )
{
    return __builtin_spirv_OpenCL_modf_v8f16_p0v8f16( x, iptr );
}

INLINE half16 OVERLOADABLE modf( half16            x,
                          __private half16* iptr )
{
    return __builtin_spirv_OpenCL_modf_v16f16_p0v16f16( x, iptr );
}

INLINE half OVERLOADABLE modf( half          x,
                        __local half* iptr )
{
    return __builtin_spirv_OpenCL_modf_f16_p3f16( x, iptr );
}

INLINE half2 OVERLOADABLE modf( half2          x,
                         __local half2* iptr )
{
    return __builtin_spirv_OpenCL_modf_v2f16_p3v2f16( x, iptr );
}

INLINE half3 OVERLOADABLE modf( half3          x,
                         __local half3* iptr )
{
    return __builtin_spirv_OpenCL_modf_v3f16_p3v3f16( x, iptr );
}

INLINE half4 OVERLOADABLE modf( half4          x,
                         __local half4* iptr )
{
    return __builtin_spirv_OpenCL_modf_v4f16_p3v4f16( x, iptr );
}

INLINE half8 OVERLOADABLE modf( half8          x,
                         __local half8* iptr )
{
    return __builtin_spirv_OpenCL_modf_v8f16_p3v8f16( x, iptr );
}

INLINE half16 OVERLOADABLE modf( half16          x,
                          __local half16* iptr )
{
    return __builtin_spirv_OpenCL_modf_v16f16_p3v16f16( x, iptr );
}
#endif


#if defined(cl_khr_fp16) && (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

INLINE half OVERLOADABLE modf( half            x,
                        __generic half* iptr )
{
    return __builtin_spirv_OpenCL_modf_f16_p4f16( x, iptr );
}

INLINE half2 OVERLOADABLE modf( half2            x,
                         __generic half2* iptr )
{
    return __builtin_spirv_OpenCL_modf_v2f16_p4v2f16( x, iptr );
}

INLINE half3 OVERLOADABLE modf( half3            x,
                         __generic half3* iptr )
{
    return __builtin_spirv_OpenCL_modf_v3f16_p4v3f16( x, iptr );
}

INLINE half4 OVERLOADABLE modf( half4            x,
                         __generic half4* iptr )
{
    return __builtin_spirv_OpenCL_modf_v4f16_p4v4f16( x, iptr );
}

INLINE half8 OVERLOADABLE modf( half8            x,
                         __generic half8* iptr )
{
    return __builtin_spirv_OpenCL_modf_v8f16_p4v8f16( x, iptr );
}

INLINE half16 OVERLOADABLE modf( half16            x,
                          __generic half16* iptr )
{
    return __builtin_spirv_OpenCL_modf_v16f16_p4v16f16( x, iptr );
}
#endif //#if defined(cl_khr_fp16) && (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#if defined(cl_khr_fp64)

double OVERLOADABLE modf( double           x,
                          __global double* iptr )
{
    return __builtin_spirv_OpenCL_modf_f64_p1f64( x, iptr );
}

double2 OVERLOADABLE modf( double2           x,
                           __global double2* iptr )
{
    return __builtin_spirv_OpenCL_modf_v2f64_p1v2f64( x, iptr );
}

double3 OVERLOADABLE modf( double3           x,
                           __global double3* iptr )
{
    return __builtin_spirv_OpenCL_modf_v3f64_p1v3f64( x, iptr );
}

double4 OVERLOADABLE modf( double4           x,
                           __global double4* iptr )
{
    return __builtin_spirv_OpenCL_modf_v4f64_p1v4f64( x, iptr );
}

double8 OVERLOADABLE modf( double8           x,
                           __global double8* iptr )
{
    return __builtin_spirv_OpenCL_modf_v8f64_p1v8f64( x, iptr );
}

double16 OVERLOADABLE modf( double16           x,
                            __global double16* iptr )
{
    return __builtin_spirv_OpenCL_modf_v16f64_p1v16f64( x, iptr );
}

double OVERLOADABLE modf( double            x,
                          __private double* iptr )
{
    return __builtin_spirv_OpenCL_modf_f64_p0f64( x, iptr );
}

double2 OVERLOADABLE modf( double2            x,
                           __private double2* iptr )
{
    return __builtin_spirv_OpenCL_modf_v2f64_p0v2f64( x, iptr );
}

double3 OVERLOADABLE modf( double3            x,
                           __private double3* iptr )
{
    return __builtin_spirv_OpenCL_modf_v3f64_p0v3f64( x, iptr );
}

double4 OVERLOADABLE modf( double4            x,
                           __private double4* iptr )
{
    return __builtin_spirv_OpenCL_modf_v4f64_p0v4f64( x, iptr );
}

double8 OVERLOADABLE modf( double8            x,
                           __private double8* iptr )
{
    return __builtin_spirv_OpenCL_modf_v8f64_p0v8f64( x, iptr );
}

double16 OVERLOADABLE modf( double16            x,
                            __private double16* iptr )
{
    return __builtin_spirv_OpenCL_modf_v16f64_p0v16f64( x, iptr );
}

double OVERLOADABLE modf( double          x,
                          __local double* iptr )
{
    return __builtin_spirv_OpenCL_modf_f64_p3f64( x, iptr );
}

double2 OVERLOADABLE modf( double2        x,
                         __local double2* iptr )
{
    return __builtin_spirv_OpenCL_modf_v2f64_p3v2f64( x, iptr );
}

double3 OVERLOADABLE modf( double3          x,
                           __local double3* iptr )
{
    return __builtin_spirv_OpenCL_modf_v3f64_p3v3f64( x, iptr );
}

double4 OVERLOADABLE modf( double4          x,
                           __local double4* iptr )
{
    return __builtin_spirv_OpenCL_modf_v4f64_p3v4f64( x, iptr );
}

double8 OVERLOADABLE modf( double8          x,
                           __local double8* iptr )
{
    return __builtin_spirv_OpenCL_modf_v8f64_p3v8f64( x, iptr );
}

double16 OVERLOADABLE modf( double16          x,
                            __local double16* iptr )
{
    return __builtin_spirv_OpenCL_modf_v16f64_p3v16f64( x, iptr );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

double OVERLOADABLE modf( double            x,
                          __generic double* iptr )
{
    return __builtin_spirv_OpenCL_modf_f64_p4f64( x, iptr );
}

double2 OVERLOADABLE modf( double2            x,
                           __generic double2* iptr )
{
    return __builtin_spirv_OpenCL_modf_v2f64_p4v2f64( x, iptr );
}

double3 OVERLOADABLE modf( double3            x,
                           __generic double3* iptr )
{
    return __builtin_spirv_OpenCL_modf_v3f64_p4v3f64( x, iptr );
}

double4 OVERLOADABLE modf( double4            x,
                           __generic double4* iptr )
{
    return __builtin_spirv_OpenCL_modf_v4f64_p4v4f64( x, iptr );
}

double8 OVERLOADABLE modf( double8            x,
                           __generic double8* iptr )
{
    return __builtin_spirv_OpenCL_modf_v8f64_p4v8f64( x, iptr );
}

double16 OVERLOADABLE modf( double16            x,
                            __generic double16* iptr )
{
    return __builtin_spirv_OpenCL_modf_v16f64_p4v16f64( x, iptr );
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
#endif // defined(cl_khr_fp64)

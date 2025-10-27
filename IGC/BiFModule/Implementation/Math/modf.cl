/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float __attribute__((overloadable)) __spirv_ocl_modf( float           x,
                                      __global float* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    return __spirv_ocl_copysign((__intel_relaxed_isinf(x) ? (float)(0.0f) : x - *iptr), x);
}

INLINE float2 __attribute__((overloadable)) __spirv_ocl_modf( float2           x,
                                           __global float2* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    float2 temp = x - *iptr;
    temp.s0 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s0) ? (float)(0.0f) : temp.s0), x.s0);
    temp.s1 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s1) ? (float)(0.0f) : temp.s1), x.s1);
    return temp;
}

INLINE float3 __attribute__((overloadable)) __spirv_ocl_modf( float3           x,
                                           __global float3* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    float3 temp = x - *iptr;
    temp.s0 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s0) ? (float)(0.0f) : temp.s0), x.s0);
    temp.s1 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s1) ? (float)(0.0f) : temp.s1), x.s1);
    temp.s2 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s2) ? (float)(0.0f) : temp.s2), x.s2);
    return temp;
}

INLINE float4 __attribute__((overloadable)) __spirv_ocl_modf( float4           x,
                                           __global float4* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    float4 temp = x - *iptr;
    temp.s0 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s0) ? (float)(0.0f) : temp.s0), x.s0);
    temp.s1 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s1) ? (float)(0.0f) : temp.s1), x.s1);
    temp.s2 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s2) ? (float)(0.0f) : temp.s2), x.s2);
    temp.s3 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s3) ? (float)(0.0f) : temp.s3), x.s3);
    return temp;
}

INLINE float8 __attribute__((overloadable)) __spirv_ocl_modf( float8           x,
                                           __global float8* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    float8 temp = x - *iptr;
    temp.s0 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s0) ? (float)(0.0f) : temp.s0), x.s0);
    temp.s1 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s1) ? (float)(0.0f) : temp.s1), x.s1);
    temp.s2 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s2) ? (float)(0.0f) : temp.s2), x.s2);
    temp.s3 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s3) ? (float)(0.0f) : temp.s3), x.s3);
    temp.s4 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s4) ? (float)(0.0f) : temp.s4), x.s4);
    temp.s5 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s5) ? (float)(0.0f) : temp.s5), x.s5);
    temp.s6 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s6) ? (float)(0.0f) : temp.s6), x.s6);
    temp.s7 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s7) ? (float)(0.0f) : temp.s7), x.s7);
    return temp;
}

INLINE float16 __attribute__((overloadable)) __spirv_ocl_modf( float16           x,
                                              __global float16* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    float16 temp = x - *iptr;
    temp.s0 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s0) ? (float)(0.0f) : temp.s0), x.s0);
    temp.s1 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s1) ? (float)(0.0f) : temp.s1), x.s1);
    temp.s2 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s2) ? (float)(0.0f) : temp.s2), x.s2);
    temp.s3 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s3) ? (float)(0.0f) : temp.s3), x.s3);
    temp.s4 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s4) ? (float)(0.0f) : temp.s4), x.s4);
    temp.s5 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s5) ? (float)(0.0f) : temp.s5), x.s5);
    temp.s6 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s6) ? (float)(0.0f) : temp.s6), x.s6);
    temp.s7 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s7) ? (float)(0.0f) : temp.s7), x.s7);
    temp.s8 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s8) ? (float)(0.0f) : temp.s8), x.s8);
    temp.s9 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s9) ? (float)(0.0f) : temp.s9), x.s9);
    temp.sa = __spirv_ocl_copysign((__intel_relaxed_isinf(x.sa) ? (float)(0.0f) : temp.sa), x.sa);
    temp.sb = __spirv_ocl_copysign((__intel_relaxed_isinf(x.sb) ? (float)(0.0f) : temp.sb), x.sb);
    temp.sc = __spirv_ocl_copysign((__intel_relaxed_isinf(x.sc) ? (float)(0.0f) : temp.sc), x.sc);
    temp.sd = __spirv_ocl_copysign((__intel_relaxed_isinf(x.sd) ? (float)(0.0f) : temp.sd), x.sd);
    temp.se = __spirv_ocl_copysign((__intel_relaxed_isinf(x.se) ? (float)(0.0f) : temp.se), x.se);
    temp.sf = __spirv_ocl_copysign((__intel_relaxed_isinf(x.sf) ? (float)(0.0f) : temp.sf), x.sf);
    return temp;
}

INLINE float __attribute__((overloadable)) __spirv_ocl_modf( float            x,
                                      __private float* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    return __spirv_ocl_copysign((__intel_relaxed_isinf(x) ? (float)(0.0f) : x - *iptr), x);
}

INLINE float2 __attribute__((overloadable)) __spirv_ocl_modf( float2            x,
                                           __private float2* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    float2 temp = x - *iptr;
    temp.s0 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s0) ? (float)(0.0f) : temp.s0), x.s0);
    temp.s1 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s1) ? (float)(0.0f) : temp.s1), x.s1);
    return temp;
}

INLINE float3 __attribute__((overloadable)) __spirv_ocl_modf( float3            x,
                                           __private float3* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    float3 temp = x - *iptr;
    temp.s0 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s0) ? (float)(0.0f) : temp.s0), x.s0);
    temp.s1 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s1) ? (float)(0.0f) : temp.s1), x.s1);
    temp.s2 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s2) ? (float)(0.0f) : temp.s2), x.s2);
    return temp;
}

INLINE float4 __attribute__((overloadable)) __spirv_ocl_modf( float4            x,
                                           __private float4* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    float4 temp = x - *iptr;
    temp.s0 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s0) ? (float)(0.0f) : temp.s0), x.s0);
    temp.s1 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s1) ? (float)(0.0f) : temp.s1), x.s1);
    temp.s2 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s2) ? (float)(0.0f) : temp.s2), x.s2);
    temp.s3 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s3) ? (float)(0.0f) : temp.s3), x.s3);
    return temp;
}

INLINE float8 __attribute__((overloadable)) __spirv_ocl_modf( float8            x,
                                           __private float8* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    float8 temp = x - *iptr;
    temp.s0 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s0) ? (float)(0.0f) : temp.s0), x.s0);
    temp.s1 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s1) ? (float)(0.0f) : temp.s1), x.s1);
    temp.s2 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s2) ? (float)(0.0f) : temp.s2), x.s2);
    temp.s3 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s3) ? (float)(0.0f) : temp.s3), x.s3);
    temp.s4 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s4) ? (float)(0.0f) : temp.s4), x.s4);
    temp.s5 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s5) ? (float)(0.0f) : temp.s5), x.s5);
    temp.s6 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s6) ? (float)(0.0f) : temp.s6), x.s6);
    temp.s7 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s7) ? (float)(0.0f) : temp.s7), x.s7);
    return temp;
}

INLINE float16 __attribute__((overloadable)) __spirv_ocl_modf( float16            x,
                                              __private float16* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    float16 temp = x - *iptr;
    temp.s0 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s0) ? (float)(0.0f) : temp.s0), x.s0);
    temp.s1 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s1) ? (float)(0.0f) : temp.s1), x.s1);
    temp.s2 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s2) ? (float)(0.0f) : temp.s2), x.s2);
    temp.s3 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s3) ? (float)(0.0f) : temp.s3), x.s3);
    temp.s4 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s4) ? (float)(0.0f) : temp.s4), x.s4);
    temp.s5 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s5) ? (float)(0.0f) : temp.s5), x.s5);
    temp.s6 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s6) ? (float)(0.0f) : temp.s6), x.s6);
    temp.s7 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s7) ? (float)(0.0f) : temp.s7), x.s7);
    temp.s8 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s8) ? (float)(0.0f) : temp.s8), x.s8);
    temp.s9 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s9) ? (float)(0.0f) : temp.s9), x.s9);
    temp.sa = __spirv_ocl_copysign((__intel_relaxed_isinf(x.sa) ? (float)(0.0f) : temp.sa), x.sa);
    temp.sb = __spirv_ocl_copysign((__intel_relaxed_isinf(x.sb) ? (float)(0.0f) : temp.sb), x.sb);
    temp.sc = __spirv_ocl_copysign((__intel_relaxed_isinf(x.sc) ? (float)(0.0f) : temp.sc), x.sc);
    temp.sd = __spirv_ocl_copysign((__intel_relaxed_isinf(x.sd) ? (float)(0.0f) : temp.sd), x.sd);
    temp.se = __spirv_ocl_copysign((__intel_relaxed_isinf(x.se) ? (float)(0.0f) : temp.se), x.se);
    temp.sf = __spirv_ocl_copysign((__intel_relaxed_isinf(x.sf) ? (float)(0.0f) : temp.sf), x.sf);
    return temp;
}

INLINE float __attribute__((overloadable)) __spirv_ocl_modf( float          x,
                                      __local float* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    return __spirv_ocl_copysign((__intel_relaxed_isinf(x) ? (float)(0.0f) : x - *iptr), x);
}

INLINE float2 __attribute__((overloadable)) __spirv_ocl_modf( float2          x,
                                           __local float2* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    float2 temp = x - *iptr;
    temp.s0 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s0) ? (float)(0.0f) : temp.s0), x.s0);
    temp.s1 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s1) ? (float)(0.0f) : temp.s1), x.s1);
    return temp;
}

INLINE float3 __attribute__((overloadable)) __spirv_ocl_modf( float3          x,
                                           __local float3* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    float3 temp = x - *iptr;
    temp.s0 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s0) ? (float)(0.0f) : temp.s0), x.s0);
    temp.s1 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s1) ? (float)(0.0f) : temp.s1), x.s1);
    temp.s2 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s2) ? (float)(0.0f) : temp.s2), x.s2);
    return temp;
}

INLINE float4 __attribute__((overloadable)) __spirv_ocl_modf( float4          x,
                                           __local float4* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    float4 temp = x - *iptr;
    temp.s0 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s0) ? (float)(0.0f) : temp.s0), x.s0);
    temp.s1 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s1) ? (float)(0.0f) : temp.s1), x.s1);
    temp.s2 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s2) ? (float)(0.0f) : temp.s2), x.s2);
    temp.s3 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s3) ? (float)(0.0f) : temp.s3), x.s3);
    return temp;
}

INLINE float8 __attribute__((overloadable)) __spirv_ocl_modf( float8          x,
                                           __local float8* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    float8 temp = x - *iptr;
    temp.s0 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s0) ? (float)(0.0f) : temp.s0), x.s0);
    temp.s1 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s1) ? (float)(0.0f) : temp.s1), x.s1);
    temp.s2 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s2) ? (float)(0.0f) : temp.s2), x.s2);
    temp.s3 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s3) ? (float)(0.0f) : temp.s3), x.s3);
    temp.s4 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s4) ? (float)(0.0f) : temp.s4), x.s4);
    temp.s5 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s5) ? (float)(0.0f) : temp.s5), x.s5);
    temp.s6 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s6) ? (float)(0.0f) : temp.s6), x.s6);
    temp.s7 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s7) ? (float)(0.0f) : temp.s7), x.s7);
    return temp;
}

INLINE float16 __attribute__((overloadable)) __spirv_ocl_modf( float16          x,
                                              __local float16* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    float16 temp = x - *iptr;
    temp.s0 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s0) ? (float)(0.0f) : temp.s0), x.s0);
    temp.s1 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s1) ? (float)(0.0f) : temp.s1), x.s1);
    temp.s2 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s2) ? (float)(0.0f) : temp.s2), x.s2);
    temp.s3 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s3) ? (float)(0.0f) : temp.s3), x.s3);
    temp.s4 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s4) ? (float)(0.0f) : temp.s4), x.s4);
    temp.s5 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s5) ? (float)(0.0f) : temp.s5), x.s5);
    temp.s6 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s6) ? (float)(0.0f) : temp.s6), x.s6);
    temp.s7 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s7) ? (float)(0.0f) : temp.s7), x.s7);
    temp.s8 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s8) ? (float)(0.0f) : temp.s8), x.s8);
    temp.s9 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s9) ? (float)(0.0f) : temp.s9), x.s9);
    temp.sa = __spirv_ocl_copysign((__intel_relaxed_isinf(x.sa) ? (float)(0.0f) : temp.sa), x.sa);
    temp.sb = __spirv_ocl_copysign((__intel_relaxed_isinf(x.sb) ? (float)(0.0f) : temp.sb), x.sb);
    temp.sc = __spirv_ocl_copysign((__intel_relaxed_isinf(x.sc) ? (float)(0.0f) : temp.sc), x.sc);
    temp.sd = __spirv_ocl_copysign((__intel_relaxed_isinf(x.sd) ? (float)(0.0f) : temp.sd), x.sd);
    temp.se = __spirv_ocl_copysign((__intel_relaxed_isinf(x.se) ? (float)(0.0f) : temp.se), x.se);
    temp.sf = __spirv_ocl_copysign((__intel_relaxed_isinf(x.sf) ? (float)(0.0f) : temp.sf), x.sf);
    return temp;
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

INLINE float __attribute__((overloadable)) __spirv_ocl_modf( float            x,
                                      __generic float* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    return __spirv_ocl_copysign((__intel_relaxed_isinf(x) ? (float)(0.0f) : x - *iptr), x);
}

INLINE float2 __attribute__((overloadable)) __spirv_ocl_modf( float2            x,
                                           __generic float2* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    float2 temp = x - *iptr;
    temp.s0 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s0) ? (float)(0.0f) : temp.s0), x.s0);
    temp.s1 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s1) ? (float)(0.0f) : temp.s1), x.s1);
    return temp;
}

INLINE float3 __attribute__((overloadable)) __spirv_ocl_modf( float3            x,
                                           __generic float3* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    float3 temp = x - *iptr;
    temp.s0 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s0) ? (float)(0.0f) : temp.s0), x.s0);
    temp.s1 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s1) ? (float)(0.0f) : temp.s1), x.s1);
    temp.s2 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s2) ? (float)(0.0f) : temp.s2), x.s2);
    return temp;
}

INLINE float4 __attribute__((overloadable)) __spirv_ocl_modf( float4            x,
                                           __generic float4* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    float4 temp = x - *iptr;
    temp.s0 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s0) ? (float)(0.0f) : temp.s0), x.s0);
    temp.s1 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s1) ? (float)(0.0f) : temp.s1), x.s1);
    temp.s2 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s2) ? (float)(0.0f) : temp.s2), x.s2);
    temp.s3 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s3) ? (float)(0.0f) : temp.s3), x.s3);
    return temp;
}

INLINE float8 __attribute__((overloadable)) __spirv_ocl_modf( float8            x,
                                           __generic float8* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    float8 temp = x - *iptr;
    temp.s0 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s0) ? (float)(0.0f) : temp.s0), x.s0);
    temp.s1 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s1) ? (float)(0.0f) : temp.s1), x.s1);
    temp.s2 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s2) ? (float)(0.0f) : temp.s2), x.s2);
    temp.s3 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s3) ? (float)(0.0f) : temp.s3), x.s3);
    temp.s4 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s4) ? (float)(0.0f) : temp.s4), x.s4);
    temp.s5 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s5) ? (float)(0.0f) : temp.s5), x.s5);
    temp.s6 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s6) ? (float)(0.0f) : temp.s6), x.s6);
    temp.s7 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s7) ? (float)(0.0f) : temp.s7), x.s7);
    return temp;
}

INLINE float16 __attribute__((overloadable)) __spirv_ocl_modf( float16            x,
                                              __generic float16* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    float16 temp = x - *iptr;
    temp.s0 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s0) ? (float)(0.0f) : temp.s0), x.s0);
    temp.s1 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s1) ? (float)(0.0f) : temp.s1), x.s1);
    temp.s2 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s2) ? (float)(0.0f) : temp.s2), x.s2);
    temp.s3 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s3) ? (float)(0.0f) : temp.s3), x.s3);
    temp.s4 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s4) ? (float)(0.0f) : temp.s4), x.s4);
    temp.s5 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s5) ? (float)(0.0f) : temp.s5), x.s5);
    temp.s6 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s6) ? (float)(0.0f) : temp.s6), x.s6);
    temp.s7 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s7) ? (float)(0.0f) : temp.s7), x.s7);
    temp.s8 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s8) ? (float)(0.0f) : temp.s8), x.s8);
    temp.s9 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s9) ? (float)(0.0f) : temp.s9), x.s9);
    temp.sa = __spirv_ocl_copysign((__intel_relaxed_isinf(x.sa) ? (float)(0.0f) : temp.sa), x.sa);
    temp.sb = __spirv_ocl_copysign((__intel_relaxed_isinf(x.sb) ? (float)(0.0f) : temp.sb), x.sb);
    temp.sc = __spirv_ocl_copysign((__intel_relaxed_isinf(x.sc) ? (float)(0.0f) : temp.sc), x.sc);
    temp.sd = __spirv_ocl_copysign((__intel_relaxed_isinf(x.sd) ? (float)(0.0f) : temp.sd), x.sd);
    temp.se = __spirv_ocl_copysign((__intel_relaxed_isinf(x.se) ? (float)(0.0f) : temp.se), x.se);
    temp.sf = __spirv_ocl_copysign((__intel_relaxed_isinf(x.sf) ? (float)(0.0f) : temp.sf), x.sf);
    return temp;
}

#endif //#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#ifdef cl_khr_fp16

INLINE half __attribute__((overloadable)) __spirv_ocl_modf( half           x,
                                     __global half* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    return __spirv_ocl_copysign((__intel_relaxed_isinf(x) ? (half)(0.0f) : x - *iptr), x);
}

INLINE half2 __attribute__((overloadable)) __spirv_ocl_modf( half2           x,
                                          __global half2* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    half2 temp = x - *iptr;
    temp.s0 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s0) ? (half)(0.0f) : temp.s0), x.s0);
    temp.s1 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s1) ? (half)(0.0f) : temp.s1), x.s1);
    return temp;
}

INLINE half3 __attribute__((overloadable)) __spirv_ocl_modf( half3           x,
                                          __global half3* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    half3 temp = x - *iptr;
    temp.s0 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s0) ? (half)(0.0f) : temp.s0), x.s0);
    temp.s1 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s1) ? (half)(0.0f) : temp.s1), x.s1);
    temp.s2 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s2) ? (half)(0.0f) : temp.s2), x.s2);
    return temp;
}

INLINE half4 __attribute__((overloadable)) __spirv_ocl_modf( half4           x,
                                          __global half4* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    half4 temp = x - *iptr;
    temp.s0 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s0) ? (half)(0.0f) : temp.s0), x.s0);
    temp.s1 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s1) ? (half)(0.0f) : temp.s1), x.s1);
    temp.s2 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s2) ? (half)(0.0f) : temp.s2), x.s2);
    temp.s3 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s3) ? (half)(0.0f) : temp.s3), x.s3);
    return temp;
}

INLINE half8 __attribute__((overloadable)) __spirv_ocl_modf( half8           x,
                                          __global half8* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    half8 temp = x - *iptr;
    temp.s0 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s0) ? (half)(0.0f) : temp.s0), x.s0);
    temp.s1 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s1) ? (half)(0.0f) : temp.s1), x.s1);
    temp.s2 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s2) ? (half)(0.0f) : temp.s2), x.s2);
    temp.s3 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s3) ? (half)(0.0f) : temp.s3), x.s3);
    temp.s4 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s4) ? (half)(0.0f) : temp.s4), x.s4);
    temp.s5 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s5) ? (half)(0.0f) : temp.s5), x.s5);
    temp.s6 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s6) ? (half)(0.0f) : temp.s6), x.s6);
    temp.s7 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s7) ? (half)(0.0f) : temp.s7), x.s7);
    return temp;
}

INLINE half16 __attribute__((overloadable)) __spirv_ocl_modf( half16           x,
                                             __global half16* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    half16 temp = x - *iptr;
    temp.s0 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s0) ? (half)(0.0f) : temp.s0), x.s0);
    temp.s1 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s1) ? (half)(0.0f) : temp.s1), x.s1);
    temp.s2 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s2) ? (half)(0.0f) : temp.s2), x.s2);
    temp.s3 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s3) ? (half)(0.0f) : temp.s3), x.s3);
    temp.s4 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s4) ? (half)(0.0f) : temp.s4), x.s4);
    temp.s5 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s5) ? (half)(0.0f) : temp.s5), x.s5);
    temp.s6 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s6) ? (half)(0.0f) : temp.s6), x.s6);
    temp.s7 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s7) ? (half)(0.0f) : temp.s7), x.s7);
    temp.s8 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s8) ? (half)(0.0f) : temp.s8), x.s8);
    temp.s9 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s9) ? (half)(0.0f) : temp.s9), x.s9);
    temp.sa = __spirv_ocl_copysign((__intel_relaxed_isinf(x.sa) ? (half)(0.0f) : temp.sa), x.sa);
    temp.sb = __spirv_ocl_copysign((__intel_relaxed_isinf(x.sb) ? (half)(0.0f) : temp.sb), x.sb);
    temp.sc = __spirv_ocl_copysign((__intel_relaxed_isinf(x.sc) ? (half)(0.0f) : temp.sc), x.sc);
    temp.sd = __spirv_ocl_copysign((__intel_relaxed_isinf(x.sd) ? (half)(0.0f) : temp.sd), x.sd);
    temp.se = __spirv_ocl_copysign((__intel_relaxed_isinf(x.se) ? (half)(0.0f) : temp.se), x.se);
    temp.sf = __spirv_ocl_copysign((__intel_relaxed_isinf(x.sf) ? (half)(0.0f) : temp.sf), x.sf);
    return temp;
}

INLINE half __attribute__((overloadable)) __spirv_ocl_modf( half            x,
                                     __private half* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    return __spirv_ocl_copysign((__intel_relaxed_isinf(x) ? (half)(0.0f) : x - *iptr), x);
}

INLINE half2 __attribute__((overloadable)) __spirv_ocl_modf( half2            x,
                                          __private half2* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    half2 temp = x - *iptr;
    temp.s0 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s0) ? (half)(0.0f) : temp.s0), x.s0);
    temp.s1 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s1) ? (half)(0.0f) : temp.s1), x.s1);
    return temp;
}

INLINE half3 __attribute__((overloadable)) __spirv_ocl_modf( half3            x,
                                          __private half3* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    half3 temp = x - *iptr;
    temp.s0 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s0) ? (half)(0.0f) : temp.s0), x.s0);
    temp.s1 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s1) ? (half)(0.0f) : temp.s1), x.s1);
    temp.s2 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s2) ? (half)(0.0f) : temp.s2), x.s2);
    return temp;
}

INLINE half4 __attribute__((overloadable)) __spirv_ocl_modf( half4            x,
                                          __private half4* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    half4 temp = x - *iptr;
    temp.s0 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s0) ? (half)(0.0f) : temp.s0), x.s0);
    temp.s1 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s1) ? (half)(0.0f) : temp.s1), x.s1);
    temp.s2 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s2) ? (half)(0.0f) : temp.s2), x.s2);
    temp.s3 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s3) ? (half)(0.0f) : temp.s3), x.s3);
    return temp;
}

INLINE half8 __attribute__((overloadable)) __spirv_ocl_modf( half8            x,
                                          __private half8* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    half8 temp = x - *iptr;
    temp.s0 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s0) ? (half)(0.0f) : temp.s0), x.s0);
    temp.s1 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s1) ? (half)(0.0f) : temp.s1), x.s1);
    temp.s2 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s2) ? (half)(0.0f) : temp.s2), x.s2);
    temp.s3 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s3) ? (half)(0.0f) : temp.s3), x.s3);
    temp.s4 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s4) ? (half)(0.0f) : temp.s4), x.s4);
    temp.s5 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s5) ? (half)(0.0f) : temp.s5), x.s5);
    temp.s6 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s6) ? (half)(0.0f) : temp.s6), x.s6);
    temp.s7 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s7) ? (half)(0.0f) : temp.s7), x.s7);
    return temp;
}

INLINE half16 __attribute__((overloadable)) __spirv_ocl_modf( half16            x,
                                             __private half16* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    half16 temp = x - *iptr;
    temp.s0 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s0) ? (half)(0.0f) : temp.s0), x.s0);
    temp.s1 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s1) ? (half)(0.0f) : temp.s1), x.s1);
    temp.s2 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s2) ? (half)(0.0f) : temp.s2), x.s2);
    temp.s3 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s3) ? (half)(0.0f) : temp.s3), x.s3);
    temp.s4 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s4) ? (half)(0.0f) : temp.s4), x.s4);
    temp.s5 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s5) ? (half)(0.0f) : temp.s5), x.s5);
    temp.s6 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s6) ? (half)(0.0f) : temp.s6), x.s6);
    temp.s7 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s7) ? (half)(0.0f) : temp.s7), x.s7);
    temp.s8 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s8) ? (half)(0.0f) : temp.s8), x.s8);
    temp.s9 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s9) ? (half)(0.0f) : temp.s9), x.s9);
    temp.sa = __spirv_ocl_copysign((__intel_relaxed_isinf(x.sa) ? (half)(0.0f) : temp.sa), x.sa);
    temp.sb = __spirv_ocl_copysign((__intel_relaxed_isinf(x.sb) ? (half)(0.0f) : temp.sb), x.sb);
    temp.sc = __spirv_ocl_copysign((__intel_relaxed_isinf(x.sc) ? (half)(0.0f) : temp.sc), x.sc);
    temp.sd = __spirv_ocl_copysign((__intel_relaxed_isinf(x.sd) ? (half)(0.0f) : temp.sd), x.sd);
    temp.se = __spirv_ocl_copysign((__intel_relaxed_isinf(x.se) ? (half)(0.0f) : temp.se), x.se);
    temp.sf = __spirv_ocl_copysign((__intel_relaxed_isinf(x.sf) ? (half)(0.0f) : temp.sf), x.sf);
    return temp;
}

INLINE half __attribute__((overloadable)) __spirv_ocl_modf( half          x,
                                     __local half* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    return __spirv_ocl_copysign((__intel_relaxed_isinf(x) ? (half)(0.0f) : x - *iptr), x);
}

INLINE half2 __attribute__((overloadable)) __spirv_ocl_modf( half2          x,
                                          __local half2* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    half2 temp = x - *iptr;
    temp.s0 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s0) ? (half)(0.0f) : temp.s0), x.s0);
    temp.s1 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s1) ? (half)(0.0f) : temp.s1), x.s1);
    return temp;
}

INLINE half3 __attribute__((overloadable)) __spirv_ocl_modf( half3          x,
                                          __local half3* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    half3 temp = x - *iptr;
    temp.s0 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s0) ? (half)(0.0f) : temp.s0), x.s0);
    temp.s1 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s1) ? (half)(0.0f) : temp.s1), x.s1);
    temp.s2 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s2) ? (half)(0.0f) : temp.s2), x.s2);
    return temp;
}

INLINE half4 __attribute__((overloadable)) __spirv_ocl_modf( half4          x,
                                          __local half4* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    half4 temp = x - *iptr;
    temp.s0 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s0) ? (half)(0.0f) : temp.s0), x.s0);
    temp.s1 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s1) ? (half)(0.0f) : temp.s1), x.s1);
    temp.s2 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s2) ? (half)(0.0f) : temp.s2), x.s2);
    temp.s3 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s3) ? (half)(0.0f) : temp.s3), x.s3);
    return temp;
}

INLINE half8 __attribute__((overloadable)) __spirv_ocl_modf( half8          x,
                                          __local half8* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    half8 temp = x - *iptr;
    temp.s0 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s0) ? (half)(0.0f) : temp.s0), x.s0);
    temp.s1 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s1) ? (half)(0.0f) : temp.s1), x.s1);
    temp.s2 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s2) ? (half)(0.0f) : temp.s2), x.s2);
    temp.s3 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s3) ? (half)(0.0f) : temp.s3), x.s3);
    temp.s4 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s4) ? (half)(0.0f) : temp.s4), x.s4);
    temp.s5 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s5) ? (half)(0.0f) : temp.s5), x.s5);
    temp.s6 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s6) ? (half)(0.0f) : temp.s6), x.s6);
    temp.s7 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s7) ? (half)(0.0f) : temp.s7), x.s7);
    return temp;
}

INLINE half16 __attribute__((overloadable)) __spirv_ocl_modf( half16          x,
                                             __local half16* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    half16 temp = x - *iptr;
    temp.s0 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s0) ? (half)(0.0f) : temp.s0), x.s0);
    temp.s1 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s1) ? (half)(0.0f) : temp.s1), x.s1);
    temp.s2 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s2) ? (half)(0.0f) : temp.s2), x.s2);
    temp.s3 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s3) ? (half)(0.0f) : temp.s3), x.s3);
    temp.s4 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s4) ? (half)(0.0f) : temp.s4), x.s4);
    temp.s5 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s5) ? (half)(0.0f) : temp.s5), x.s5);
    temp.s6 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s6) ? (half)(0.0f) : temp.s6), x.s6);
    temp.s7 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s7) ? (half)(0.0f) : temp.s7), x.s7);
    temp.s8 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s8) ? (half)(0.0f) : temp.s8), x.s8);
    temp.s9 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s9) ? (half)(0.0f) : temp.s9), x.s9);
    temp.sa = __spirv_ocl_copysign((__intel_relaxed_isinf(x.sa) ? (half)(0.0f) : temp.sa), x.sa);
    temp.sb = __spirv_ocl_copysign((__intel_relaxed_isinf(x.sb) ? (half)(0.0f) : temp.sb), x.sb);
    temp.sc = __spirv_ocl_copysign((__intel_relaxed_isinf(x.sc) ? (half)(0.0f) : temp.sc), x.sc);
    temp.sd = __spirv_ocl_copysign((__intel_relaxed_isinf(x.sd) ? (half)(0.0f) : temp.sd), x.sd);
    temp.se = __spirv_ocl_copysign((__intel_relaxed_isinf(x.se) ? (half)(0.0f) : temp.se), x.se);
    temp.sf = __spirv_ocl_copysign((__intel_relaxed_isinf(x.sf) ? (half)(0.0f) : temp.sf), x.sf);
    return temp;
}
#endif


#if defined(cl_khr_fp16) && (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

INLINE half __attribute__((overloadable)) __spirv_ocl_modf( half            x,
                                     __generic half* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    return __spirv_ocl_copysign((__intel_relaxed_isinf(x) ? (half)(0.0f) : x - *iptr), x);
}

INLINE half2 __attribute__((overloadable)) __spirv_ocl_modf( half2            x,
                                          __generic half2* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    half2 temp = x - *iptr;
    temp.s0 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s0) ? (half)(0.0f) : temp.s0), x.s0);
    temp.s1 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s1) ? (half)(0.0f) : temp.s1), x.s1);
    return temp;
}

INLINE half3 __attribute__((overloadable)) __spirv_ocl_modf( half3            x,
                                          __generic half3* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    half3 temp = x - *iptr;
    temp.s0 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s0) ? (half)(0.0f) : temp.s0), x.s0);
    temp.s1 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s1) ? (half)(0.0f) : temp.s1), x.s1);
    temp.s2 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s2) ? (half)(0.0f) : temp.s2), x.s2);
    return temp;
}

INLINE half4 __attribute__((overloadable)) __spirv_ocl_modf( half4            x,
                                          __generic half4* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    half4 temp = x - *iptr;
    temp.s0 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s0) ? (half)(0.0f) : temp.s0), x.s0);
    temp.s1 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s1) ? (half)(0.0f) : temp.s1), x.s1);
    temp.s2 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s2) ? (half)(0.0f) : temp.s2), x.s2);
    temp.s3 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s3) ? (half)(0.0f) : temp.s3), x.s3);
    return temp;
}

INLINE half8 __attribute__((overloadable)) __spirv_ocl_modf( half8            x,
                                          __generic half8* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    half8 temp = x - *iptr;
    temp.s0 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s0) ? (half)(0.0f) : temp.s0), x.s0);
    temp.s1 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s1) ? (half)(0.0f) : temp.s1), x.s1);
    temp.s2 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s2) ? (half)(0.0f) : temp.s2), x.s2);
    temp.s3 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s3) ? (half)(0.0f) : temp.s3), x.s3);
    temp.s4 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s4) ? (half)(0.0f) : temp.s4), x.s4);
    temp.s5 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s5) ? (half)(0.0f) : temp.s5), x.s5);
    temp.s6 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s6) ? (half)(0.0f) : temp.s6), x.s6);
    temp.s7 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s7) ? (half)(0.0f) : temp.s7), x.s7);
    return temp;
}

INLINE half16 __attribute__((overloadable)) __spirv_ocl_modf( half16            x,
                                             __generic half16* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    half16 temp = x - *iptr;
    temp.s0 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s0) ? (half)(0.0f) : temp.s0), x.s0);
    temp.s1 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s1) ? (half)(0.0f) : temp.s1), x.s1);
    temp.s2 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s2) ? (half)(0.0f) : temp.s2), x.s2);
    temp.s3 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s3) ? (half)(0.0f) : temp.s3), x.s3);
    temp.s4 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s4) ? (half)(0.0f) : temp.s4), x.s4);
    temp.s5 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s5) ? (half)(0.0f) : temp.s5), x.s5);
    temp.s6 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s6) ? (half)(0.0f) : temp.s6), x.s6);
    temp.s7 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s7) ? (half)(0.0f) : temp.s7), x.s7);
    temp.s8 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s8) ? (half)(0.0f) : temp.s8), x.s8);
    temp.s9 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s9) ? (half)(0.0f) : temp.s9), x.s9);
    temp.sa = __spirv_ocl_copysign((__intel_relaxed_isinf(x.sa) ? (half)(0.0f) : temp.sa), x.sa);
    temp.sb = __spirv_ocl_copysign((__intel_relaxed_isinf(x.sb) ? (half)(0.0f) : temp.sb), x.sb);
    temp.sc = __spirv_ocl_copysign((__intel_relaxed_isinf(x.sc) ? (half)(0.0f) : temp.sc), x.sc);
    temp.sd = __spirv_ocl_copysign((__intel_relaxed_isinf(x.sd) ? (half)(0.0f) : temp.sd), x.sd);
    temp.se = __spirv_ocl_copysign((__intel_relaxed_isinf(x.se) ? (half)(0.0f) : temp.se), x.se);
    temp.sf = __spirv_ocl_copysign((__intel_relaxed_isinf(x.sf) ? (half)(0.0f) : temp.sf), x.sf);
    return temp;
}
#endif //#if defined(cl_khr_fp16) && (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#if defined(cl_khr_fp64)

double __attribute__((overloadable)) __spirv_ocl_modf( double           x,
                                       __global double* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    return __spirv_ocl_copysign((__intel_relaxed_isinf(x) ? 0.0 : x - *iptr), x);
}

double2 __attribute__((overloadable)) __spirv_ocl_modf( double2           x,
                                            __global double2* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    double2 temp = x - *iptr;
    temp.s0 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s0) ? 0.0 : temp.s0), x.s0);
    temp.s1 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s1) ? 0.0 : temp.s1), x.s1);
    return temp;
}

double3 __attribute__((overloadable)) __spirv_ocl_modf( double3           x,
                                            __global double3* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    double3 temp = x - *iptr;
    temp.s0 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s0) ? 0.0 : temp.s0), x.s0);
    temp.s1 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s1) ? 0.0 : temp.s1), x.s1);
    temp.s2 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s2) ? 0.0 : temp.s2), x.s2);
    return temp;
}

double4 __attribute__((overloadable)) __spirv_ocl_modf( double4           x,
                                            __global double4* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    double4 temp = x - *iptr;
    temp.s0 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s0) ? 0.0 : temp.s0), x.s0);
    temp.s1 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s1) ? 0.0 : temp.s1), x.s1);
    temp.s2 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s2) ? 0.0 : temp.s2), x.s2);
    temp.s3 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s3) ? 0.0 : temp.s3), x.s3);
    return temp;
}

double8 __attribute__((overloadable)) __spirv_ocl_modf( double8           x,
                                            __global double8* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    double8 temp = x - *iptr;
    temp.s0 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s0) ? 0.0 : temp.s0), x.s0);
    temp.s1 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s1) ? 0.0 : temp.s1), x.s1);
    temp.s2 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s2) ? 0.0 : temp.s2), x.s2);
    temp.s3 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s3) ? 0.0 : temp.s3), x.s3);
    temp.s4 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s4) ? 0.0 : temp.s4), x.s4);
    temp.s5 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s5) ? 0.0 : temp.s5), x.s5);
    temp.s6 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s6) ? 0.0 : temp.s6), x.s6);
    temp.s7 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s7) ? 0.0 : temp.s7), x.s7);
    return temp;
}

double16 __attribute__((overloadable)) __spirv_ocl_modf( double16           x,
                                               __global double16* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    double16 temp = x - *iptr;
    temp.s0 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s0) ? 0.0 : temp.s0), x.s0);
    temp.s1 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s1) ? 0.0 : temp.s1), x.s1);
    temp.s2 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s2) ? 0.0 : temp.s2), x.s2);
    temp.s3 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s3) ? 0.0 : temp.s3), x.s3);
    temp.s4 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s4) ? 0.0 : temp.s4), x.s4);
    temp.s5 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s5) ? 0.0 : temp.s5), x.s5);
    temp.s6 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s6) ? 0.0 : temp.s6), x.s6);
    temp.s7 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s7) ? 0.0 : temp.s7), x.s7);
    temp.s8 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s8) ? 0.0 : temp.s8), x.s8);
    temp.s9 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s9) ? 0.0 : temp.s9), x.s9);
    temp.sa = __spirv_ocl_copysign((__intel_relaxed_isinf(x.sa) ? 0.0 : temp.sa), x.sa);
    temp.sb = __spirv_ocl_copysign((__intel_relaxed_isinf(x.sb) ? 0.0 : temp.sb), x.sb);
    temp.sc = __spirv_ocl_copysign((__intel_relaxed_isinf(x.sc) ? 0.0 : temp.sc), x.sc);
    temp.sd = __spirv_ocl_copysign((__intel_relaxed_isinf(x.sd) ? 0.0 : temp.sd), x.sd);
    temp.se = __spirv_ocl_copysign((__intel_relaxed_isinf(x.se) ? 0.0 : temp.se), x.se);
    temp.sf = __spirv_ocl_copysign((__intel_relaxed_isinf(x.sf) ? 0.0 : temp.sf), x.sf);
    return temp;
}

double __attribute__((overloadable)) __spirv_ocl_modf( double            x,
                                       __private double* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    return __spirv_ocl_copysign((__intel_relaxed_isinf(x) ? 0.0 : x - *iptr), x);
}

double2 __attribute__((overloadable)) __spirv_ocl_modf( double2            x,
                                            __private double2* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    double2 temp = x - *iptr;
    temp.s0 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s0) ? 0.0 : temp.s0), x.s0);
    temp.s1 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s1) ? 0.0 : temp.s1), x.s1);
    return temp;
}

double3 __attribute__((overloadable)) __spirv_ocl_modf( double3            x,
                                            __private double3* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    double3 temp = x - *iptr;
    temp.s0 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s0) ? 0.0 : temp.s0), x.s0);
    temp.s1 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s1) ? 0.0 : temp.s1), x.s1);
    temp.s2 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s2) ? 0.0 : temp.s2), x.s2);
    return temp;
}

double4 __attribute__((overloadable)) __spirv_ocl_modf( double4            x,
                                            __private double4* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    double4 temp = x - *iptr;
    temp.s0 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s0) ? 0.0 : temp.s0), x.s0);
    temp.s1 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s1) ? 0.0 : temp.s1), x.s1);
    temp.s2 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s2) ? 0.0 : temp.s2), x.s2);
    temp.s3 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s3) ? 0.0 : temp.s3), x.s3);
    return temp;
}

double8 __attribute__((overloadable)) __spirv_ocl_modf( double8            x,
                                            __private double8* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    double8 temp = x - *iptr;
    temp.s0 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s0) ? 0.0 : temp.s0), x.s0);
    temp.s1 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s1) ? 0.0 : temp.s1), x.s1);
    temp.s2 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s2) ? 0.0 : temp.s2), x.s2);
    temp.s3 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s3) ? 0.0 : temp.s3), x.s3);
    temp.s4 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s4) ? 0.0 : temp.s4), x.s4);
    temp.s5 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s5) ? 0.0 : temp.s5), x.s5);
    temp.s6 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s6) ? 0.0 : temp.s6), x.s6);
    temp.s7 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s7) ? 0.0 : temp.s7), x.s7);
    return temp;
}

double16 __attribute__((overloadable)) __spirv_ocl_modf( double16            x,
                                               __private double16* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    double16 temp = x - *iptr;
    temp.s0 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s0) ? 0.0 : temp.s0), x.s0);
    temp.s1 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s1) ? 0.0 : temp.s1), x.s1);
    temp.s2 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s2) ? 0.0 : temp.s2), x.s2);
    temp.s3 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s3) ? 0.0 : temp.s3), x.s3);
    temp.s4 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s4) ? 0.0 : temp.s4), x.s4);
    temp.s5 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s5) ? 0.0 : temp.s5), x.s5);
    temp.s6 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s6) ? 0.0 : temp.s6), x.s6);
    temp.s7 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s7) ? 0.0 : temp.s7), x.s7);
    temp.s8 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s8) ? 0.0 : temp.s8), x.s8);
    temp.s9 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s9) ? 0.0 : temp.s9), x.s9);
    temp.sa = __spirv_ocl_copysign((__intel_relaxed_isinf(x.sa) ? 0.0 : temp.sa), x.sa);
    temp.sb = __spirv_ocl_copysign((__intel_relaxed_isinf(x.sb) ? 0.0 : temp.sb), x.sb);
    temp.sc = __spirv_ocl_copysign((__intel_relaxed_isinf(x.sc) ? 0.0 : temp.sc), x.sc);
    temp.sd = __spirv_ocl_copysign((__intel_relaxed_isinf(x.sd) ? 0.0 : temp.sd), x.sd);
    temp.se = __spirv_ocl_copysign((__intel_relaxed_isinf(x.se) ? 0.0 : temp.se), x.se);
    temp.sf = __spirv_ocl_copysign((__intel_relaxed_isinf(x.sf) ? 0.0 : temp.sf), x.sf);
    return temp;
}

double __attribute__((overloadable)) __spirv_ocl_modf( double          x,
                                       __local double* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    return __spirv_ocl_copysign((__intel_relaxed_isinf(x) ? 0.0 : x - *iptr), x);
}

double2 __attribute__((overloadable)) __spirv_ocl_modf( double2        x,
                                          __local double2* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    double2 temp = x - *iptr;
    temp.s0 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s0) ? 0.0 : temp.s0), x.s0);
    temp.s1 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s1) ? 0.0 : temp.s1), x.s1);
    return temp;
}

double3 __attribute__((overloadable)) __spirv_ocl_modf( double3          x,
                                            __local double3* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    double3 temp = x - *iptr;
    temp.s0 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s0) ? 0.0 : temp.s0), x.s0);
    temp.s1 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s1) ? 0.0 : temp.s1), x.s1);
    temp.s2 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s2) ? 0.0 : temp.s2), x.s2);
    return temp;
}

double4 __attribute__((overloadable)) __spirv_ocl_modf( double4          x,
                                            __local double4* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    double4 temp = x - *iptr;
    temp.s0 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s0) ? 0.0 : temp.s0), x.s0);
    temp.s1 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s1) ? 0.0 : temp.s1), x.s1);
    temp.s2 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s2) ? 0.0 : temp.s2), x.s2);
    temp.s3 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s3) ? 0.0 : temp.s3), x.s3);
    return temp;
}

double8 __attribute__((overloadable)) __spirv_ocl_modf( double8          x,
                                            __local double8* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    double8 temp = x - *iptr;
    temp.s0 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s0) ? 0.0 : temp.s0), x.s0);
    temp.s1 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s1) ? 0.0 : temp.s1), x.s1);
    temp.s2 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s2) ? 0.0 : temp.s2), x.s2);
    temp.s3 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s3) ? 0.0 : temp.s3), x.s3);
    temp.s4 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s4) ? 0.0 : temp.s4), x.s4);
    temp.s5 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s5) ? 0.0 : temp.s5), x.s5);
    temp.s6 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s6) ? 0.0 : temp.s6), x.s6);
    temp.s7 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s7) ? 0.0 : temp.s7), x.s7);
    return temp;
}

double16 __attribute__((overloadable)) __spirv_ocl_modf( double16          x,
                                               __local double16* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    double16 temp = x - *iptr;
    temp.s0 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s0) ? 0.0 : temp.s0), x.s0);
    temp.s1 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s1) ? 0.0 : temp.s1), x.s1);
    temp.s2 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s2) ? 0.0 : temp.s2), x.s2);
    temp.s3 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s3) ? 0.0 : temp.s3), x.s3);
    temp.s4 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s4) ? 0.0 : temp.s4), x.s4);
    temp.s5 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s5) ? 0.0 : temp.s5), x.s5);
    temp.s6 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s6) ? 0.0 : temp.s6), x.s6);
    temp.s7 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s7) ? 0.0 : temp.s7), x.s7);
    temp.s8 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s8) ? 0.0 : temp.s8), x.s8);
    temp.s9 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s9) ? 0.0 : temp.s9), x.s9);
    temp.sa = __spirv_ocl_copysign((__intel_relaxed_isinf(x.sa) ? 0.0 : temp.sa), x.sa);
    temp.sb = __spirv_ocl_copysign((__intel_relaxed_isinf(x.sb) ? 0.0 : temp.sb), x.sb);
    temp.sc = __spirv_ocl_copysign((__intel_relaxed_isinf(x.sc) ? 0.0 : temp.sc), x.sc);
    temp.sd = __spirv_ocl_copysign((__intel_relaxed_isinf(x.sd) ? 0.0 : temp.sd), x.sd);
    temp.se = __spirv_ocl_copysign((__intel_relaxed_isinf(x.se) ? 0.0 : temp.se), x.se);
    temp.sf = __spirv_ocl_copysign((__intel_relaxed_isinf(x.sf) ? 0.0 : temp.sf), x.sf);
    return temp;
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

double __attribute__((overloadable)) __spirv_ocl_modf( double            x,
                                       __generic double* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    return __spirv_ocl_copysign((__intel_relaxed_isinf(x) ? 0.0 : x - *iptr), x);
}

double2 __attribute__((overloadable)) __spirv_ocl_modf( double2            x,
                                            __generic double2* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    double2 temp = x - *iptr;
    temp.s0 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s0) ? 0.0 : temp.s0), x.s0);
    temp.s1 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s1) ? 0.0 : temp.s1), x.s1);
    return temp;
}

double3 __attribute__((overloadable)) __spirv_ocl_modf( double3            x,
                                            __generic double3* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    double3 temp = x - *iptr;
    temp.s0 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s0) ? 0.0 : temp.s0), x.s0);
    temp.s1 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s1) ? 0.0 : temp.s1), x.s1);
    temp.s2 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s2) ? 0.0 : temp.s2), x.s2);
    return temp;
}

double4 __attribute__((overloadable)) __spirv_ocl_modf( double4            x,
                                            __generic double4* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    double4 temp = x - *iptr;
    temp.s0 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s0) ? 0.0 : temp.s0), x.s0);
    temp.s1 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s1) ? 0.0 : temp.s1), x.s1);
    temp.s2 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s2) ? 0.0 : temp.s2), x.s2);
    temp.s3 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s3) ? 0.0 : temp.s3), x.s3);
    return temp;
}

double8 __attribute__((overloadable)) __spirv_ocl_modf( double8            x,
                                            __generic double8* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    double8 temp = x - *iptr;
    temp.s0 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s0) ? 0.0 : temp.s0), x.s0);
    temp.s1 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s1) ? 0.0 : temp.s1), x.s1);
    temp.s2 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s2) ? 0.0 : temp.s2), x.s2);
    temp.s3 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s3) ? 0.0 : temp.s3), x.s3);
    temp.s4 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s4) ? 0.0 : temp.s4), x.s4);
    temp.s5 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s5) ? 0.0 : temp.s5), x.s5);
    temp.s6 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s6) ? 0.0 : temp.s6), x.s6);
    temp.s7 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s7) ? 0.0 : temp.s7), x.s7);
    return temp;
}

double16 __attribute__((overloadable)) __spirv_ocl_modf( double16            x,
                                               __generic double16* iptr )
{
    *iptr = __spirv_ocl_trunc(x);
    double16 temp = x - *iptr;
    temp.s0 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s0) ? 0.0 : temp.s0), x.s0);
    temp.s1 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s1) ? 0.0 : temp.s1), x.s1);
    temp.s2 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s2) ? 0.0 : temp.s2), x.s2);
    temp.s3 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s3) ? 0.0 : temp.s3), x.s3);
    temp.s4 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s4) ? 0.0 : temp.s4), x.s4);
    temp.s5 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s5) ? 0.0 : temp.s5), x.s5);
    temp.s6 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s6) ? 0.0 : temp.s6), x.s6);
    temp.s7 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s7) ? 0.0 : temp.s7), x.s7);
    temp.s8 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s8) ? 0.0 : temp.s8), x.s8);
    temp.s9 = __spirv_ocl_copysign((__intel_relaxed_isinf(x.s9) ? 0.0 : temp.s9), x.s9);
    temp.sa = __spirv_ocl_copysign((__intel_relaxed_isinf(x.sa) ? 0.0 : temp.sa), x.sa);
    temp.sb = __spirv_ocl_copysign((__intel_relaxed_isinf(x.sb) ? 0.0 : temp.sb), x.sb);
    temp.sc = __spirv_ocl_copysign((__intel_relaxed_isinf(x.sc) ? 0.0 : temp.sc), x.sc);
    temp.sd = __spirv_ocl_copysign((__intel_relaxed_isinf(x.sd) ? 0.0 : temp.sd), x.sd);
    temp.se = __spirv_ocl_copysign((__intel_relaxed_isinf(x.se) ? 0.0 : temp.se), x.se);
    temp.sf = __spirv_ocl_copysign((__intel_relaxed_isinf(x.sf) ? 0.0 : temp.sf), x.sf);
    return temp;
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
#endif // defined(cl_khr_fp64)

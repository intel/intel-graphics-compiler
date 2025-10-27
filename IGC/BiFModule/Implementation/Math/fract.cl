/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float __attribute__((overloadable)) __spirv_ocl_fract( float x,
                                       __global float* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( 0 ), (int)(__intel_relaxed_isnan( x )) );
    float temp = x - *iptr;
    temp = __spirv_ocl_select( (float)__spirv_ocl_fmin( temp, (float)(0x1.fffffep-1f)), (float)__spirv_ocl_copysign((float)0.0f, x), (int)__intel_relaxed_isinf(x));
    return __spirv_ocl_select( temp, __spirv_ocl_nan(0), (int)(__intel_relaxed_isnan(x)) );
}

INLINE float2 __attribute__((overloadable)) __spirv_ocl_fract( float2 x,
                                            __global float2* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (int2)0 ), __convert_int2(__spirv_IsNan( x )) );
    float2 temp = x - *iptr;
    temp = __spirv_ocl_select( (float2)__spirv_ocl_fmin( temp, (float2)(0x1.fffffep-1f)), (float2)__spirv_ocl_copysign((float2)0.0f, x), __convert_int2(__spirv_IsInf(x)));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((int2)0), __convert_int2(__spirv_IsNan(x)) );
}

INLINE float3 __attribute__((overloadable)) __spirv_ocl_fract( float3 x,
                                            __global float3* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (int3)0 ), __convert_int3(__spirv_IsNan( x )) );
    float3 temp = x - *iptr;
    temp = __spirv_ocl_select( (float3)__spirv_ocl_fmin( temp, (float3)(0x1.fffffep-1f)), (float3)__spirv_ocl_copysign((float3)0.0f, x), __convert_int3(__spirv_IsInf(x)));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((int3)0), __convert_int3(__spirv_IsNan(x)) );
}

INLINE float4 __attribute__((overloadable)) __spirv_ocl_fract( float4 x,
                                            __global float4* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (int4)0 ), __convert_int4(__spirv_IsNan( x )) );
    float4 temp = x - *iptr;
    temp = __spirv_ocl_select( (float4)__spirv_ocl_fmin( temp, (float4)(0x1.fffffep-1f)), (float4)__spirv_ocl_copysign((float4)0.0f, x), __convert_int4(__spirv_IsInf(x)));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((int4)0), __convert_int4(__spirv_IsNan(x)) );
}

INLINE float8 __attribute__((overloadable)) __spirv_ocl_fract( float8 x,
                                            __global float8* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (int8)0 ), __convert_int8(__spirv_IsNan( x )) );
    float8 temp = x - *iptr;
    temp = __spirv_ocl_select( (float8)__spirv_ocl_fmin( temp, (float8)(0x1.fffffep-1f)), (float8)__spirv_ocl_copysign((float8)0.0f, x), __convert_int8(__spirv_IsInf(x)));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((int8)0), __convert_int8(__spirv_IsNan(x)) );
}

INLINE float16 __attribute__((overloadable)) __spirv_ocl_fract( float16 x,
                                               __global float16* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (int16)0 ), __convert_int16(__spirv_IsNan( x )) );
    float16 temp = x - *iptr;
    temp = __spirv_ocl_select( (float16)__spirv_ocl_fmin( temp, (float16)(0x1.fffffep-1f)), (float16)__spirv_ocl_copysign((float16)0.0f, x), __convert_int16(__spirv_IsInf(x)));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((int16)0), __convert_int16(__spirv_IsNan(x)) );
}

INLINE float __attribute__((overloadable)) __spirv_ocl_fract( float x,
                                       __private float* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( 0 ), (int)(__intel_relaxed_isnan( x )) );
    float temp = x - *iptr;
    temp = __spirv_ocl_select( (float)__spirv_ocl_fmin( temp, (float)(0x1.fffffep-1f)), (float)__spirv_ocl_copysign((float)0.0f, x), (int)__intel_relaxed_isinf(x));
    return __spirv_ocl_select( temp, __spirv_ocl_nan(0), (int)(__intel_relaxed_isnan(x)) );
}

INLINE float2 __attribute__((overloadable)) __spirv_ocl_fract( float2 x,
                                            __private float2* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (int2)0 ), __convert_int2(__spirv_IsNan( x )) );
    float2 temp = x - *iptr;
    temp = __spirv_ocl_select( (float2)__spirv_ocl_fmin( temp, (float2)(0x1.fffffep-1f)), (float2)__spirv_ocl_copysign((float2)0.0f, x), __convert_int2(__spirv_IsInf(x)));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((int2)0), __convert_int2(__spirv_IsNan(x)) );
}

INLINE float3 __attribute__((overloadable)) __spirv_ocl_fract( float3 x,
                                            __private float3* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (int3)0 ), __convert_int3(__spirv_IsNan( x )) );
    float3 temp = x - *iptr;
    temp = __spirv_ocl_select( (float3)__spirv_ocl_fmin( temp, (float3)(0x1.fffffep-1f)), (float3)__spirv_ocl_copysign((float3)0.0f, x), __convert_int3(__spirv_IsInf(x)));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((int3)0), __convert_int3(__spirv_IsNan(x)) );
}

INLINE float4 __attribute__((overloadable)) __spirv_ocl_fract( float4 x,
                                            __private float4* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (int4)0 ), __convert_int4(__spirv_IsNan( x )) );
    float4 temp = x - *iptr;
    temp = __spirv_ocl_select( (float4)__spirv_ocl_fmin( temp, (float4)(0x1.fffffep-1f)), (float4)__spirv_ocl_copysign((float4)0.0f, x), __convert_int4(__spirv_IsInf(x)));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((int4)0), __convert_int4(__spirv_IsNan(x)) );
}

INLINE float8 __attribute__((overloadable)) __spirv_ocl_fract( float8 x,
                                            __private float8* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (int8)0 ), __convert_int8(__spirv_IsNan( x )) );
    float8 temp = x - *iptr;
    temp = __spirv_ocl_select( (float8)__spirv_ocl_fmin( temp, (float8)(0x1.fffffep-1f)), (float8)__spirv_ocl_copysign((float8)0.0f, x), __convert_int8(__spirv_IsInf(x)));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((int8)0), __convert_int8(__spirv_IsNan(x)) );
}

INLINE float16 __attribute__((overloadable)) __spirv_ocl_fract( float16 x,
                                               __private float16* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (int16)0 ), __convert_int16(__spirv_IsNan( x )) );
    float16 temp = x - *iptr;
    temp = __spirv_ocl_select( (float16)__spirv_ocl_fmin( temp, (float16)(0x1.fffffep-1f)), (float16)__spirv_ocl_copysign((float16)0.0f, x), __convert_int16(__spirv_IsInf(x)));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((int16)0), __convert_int16(__spirv_IsNan(x)) );
}

INLINE float __attribute__((overloadable)) __spirv_ocl_fract( float x,
                                       __local float* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (int)0 ), (int)(__intel_relaxed_isnan( x )) );
    float temp = x - *iptr;
    temp = __spirv_ocl_select( (float)__spirv_ocl_fmin( temp, (float)(0x1.fffffep-1f)), (float)__spirv_ocl_copysign((float)0.0f, x), (int)__intel_relaxed_isinf(x));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((int)0), (int)(__intel_relaxed_isnan(x)) );
}

INLINE float2 __attribute__((overloadable)) __spirv_ocl_fract( float2 x,
                                            __local float2* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (int2)0 ), __convert_int2(__spirv_IsNan( x )) );
    float2 temp = x - *iptr;
    temp = __spirv_ocl_select( (float2)__spirv_ocl_fmin( temp, (float2)(0x1.fffffep-1f)), (float2)__spirv_ocl_copysign((float2)0.0f, x), __convert_int2(__spirv_IsInf(x)));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((int2)0), __convert_int2(__spirv_IsNan(x)) );
}

INLINE float3 __attribute__((overloadable)) __spirv_ocl_fract( float3 x,
                                            __local float3* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (int3)0 ), __convert_int3(__spirv_IsNan( x )) );
    float3 temp = x - *iptr;
    temp = __spirv_ocl_select( (float3)__spirv_ocl_fmin( temp, (float3)(0x1.fffffep-1f)), (float3)__spirv_ocl_copysign((float3)0.0f, x), __convert_int3(__spirv_IsInf(x)));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((int3)0), __convert_int3(__spirv_IsNan(x)) );
}

INLINE float4 __attribute__((overloadable)) __spirv_ocl_fract( float4 x,
                                            __local float4* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (int4)0 ), __convert_int4(__spirv_IsNan( x )) );
    float4 temp = x - *iptr;
    temp = __spirv_ocl_select( (float4)__spirv_ocl_fmin( temp, (float4)(0x1.fffffep-1f)), (float4)__spirv_ocl_copysign((float4)0.0f, x), __convert_int4(__spirv_IsInf(x)));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((int4)0), __convert_int4(__spirv_IsNan(x)) );
}

INLINE float8 __attribute__((overloadable)) __spirv_ocl_fract( float8 x,
                                            __local float8* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (int8)0 ), __convert_int8(__spirv_IsNan( x )) );
    float8 temp = x - *iptr;
    temp = __spirv_ocl_select( (float8)__spirv_ocl_fmin( temp, (float8)(0x1.fffffep-1f)), (float8)__spirv_ocl_copysign((float8)0.0f, x), __convert_int8(__spirv_IsInf(x)));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((int8)0), __convert_int8(__spirv_IsNan(x)) );
}

INLINE float16 __attribute__((overloadable)) __spirv_ocl_fract( float16 x,
                                               __local float16* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (int16)0 ), __convert_int16(__spirv_IsNan( x )) );
    float16 temp = x - *iptr;
    temp = __spirv_ocl_select( (float16)__spirv_ocl_fmin( temp, (float16)(0x1.fffffep-1f)), (float16)__spirv_ocl_copysign((float16)0.0f, x), __convert_int16(__spirv_IsInf(x)));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((int16)0), __convert_int16(__spirv_IsNan(x)) );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

INLINE float __attribute__((overloadable)) __spirv_ocl_fract( float x,
                                       __generic float* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (int)0 ), (int)(__intel_relaxed_isnan( x )) );
    float temp = x - *iptr;
    temp = __spirv_ocl_select( (float)__spirv_ocl_fmin( temp, (float)(0x1.fffffep-1f)), (float)__spirv_ocl_copysign((float)0.0f, x), (int)__intel_relaxed_isinf(x));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((int)0), (int)(__intel_relaxed_isnan(x)) );
}

INLINE float2 __attribute__((overloadable)) __spirv_ocl_fract( float2 x,
                                            __generic float2* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (int2)0 ), __convert_int2(__spirv_IsNan( x )) );
    float2 temp = x - *iptr;
    temp = __spirv_ocl_select( (float2)__spirv_ocl_fmin( temp, (float2)(0x1.fffffep-1f)), (float2)__spirv_ocl_copysign((float2)0.0f, x), __convert_int2(__spirv_IsInf(x)));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((int2)0), __convert_int2(__spirv_IsNan(x)) );
}

INLINE float3 __attribute__((overloadable)) __spirv_ocl_fract( float3 x,
                                            __generic float3* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (int3)0 ), __convert_int3(__spirv_IsNan( x )) );
    float3 temp = x - *iptr;
    temp = __spirv_ocl_select( (float3)__spirv_ocl_fmin( temp, (float3)(0x1.fffffep-1f)), (float3)__spirv_ocl_copysign((float3)0.0f, x), __convert_int3(__spirv_IsInf(x)));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((int3)0), __convert_int3(__spirv_IsNan(x)) );
}

INLINE float4 __attribute__((overloadable)) __spirv_ocl_fract( float4 x,
                                            __generic float4* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (int4)0 ), __convert_int4(__spirv_IsNan( x )) );
    float4 temp = x - *iptr;
    temp = __spirv_ocl_select( (float4)__spirv_ocl_fmin( temp, (float4)(0x1.fffffep-1f)), (float4)__spirv_ocl_copysign((float4)0.0f, x), __convert_int4(__spirv_IsInf(x)));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((int4)0), __convert_int4(__spirv_IsNan(x)) );
}

INLINE float8 __attribute__((overloadable)) __spirv_ocl_fract( float8 x,
                                            __generic float8* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (int8)0 ), __convert_int8(__spirv_IsNan( x )) );
    float8 temp = x - *iptr;
    temp = __spirv_ocl_select( (float8)__spirv_ocl_fmin( temp, (float8)(0x1.fffffep-1f)), (float8)__spirv_ocl_copysign((float8)0.0f, x), __convert_int8(__spirv_IsInf(x)));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((int8)0), __convert_int8(__spirv_IsNan(x)) );
}

INLINE float16 __attribute__((overloadable)) __spirv_ocl_fract( float16 x,
                                               __generic float16* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (int16)0 ), __convert_int16(__spirv_IsNan( x )) );
    float16 temp = x - *iptr;
    temp = __spirv_ocl_select( (float16)__spirv_ocl_fmin( temp, (float16)(0x1.fffffep-1f)), (float16)__spirv_ocl_copysign((float16)0.0f, x), __convert_int16(__spirv_IsInf(x)));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((int16)0), __convert_int16(__spirv_IsNan(x)) );
}

#endif //#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)


#ifdef cl_khr_fp16
INLINE half __attribute__((overloadable)) __spirv_ocl_fract( half x,
                                      __global half* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (short)0 ), (short)(__spirv_IsNan( x )) );
    half temp = x - *iptr;
    temp = __spirv_ocl_select( (half)__spirv_ocl_fmin( temp, (half)(0x1.ffcp-1h)), (half)__spirv_ocl_copysign((half)0.0f, x), (short)__spirv_IsInf(x));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((short)0), (short)(__spirv_IsNan(x)) );
}

INLINE half2 __attribute__((overloadable)) __spirv_ocl_fract( half2 x,
                                           __global half2* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (short2)0 ), __convert_short2(__spirv_IsNan( x )) );
    half2 temp = x - *iptr;
    temp = __spirv_ocl_select( (half2)__spirv_ocl_fmin( temp, (half2)(0x1.ffcp-1h)), (half2)__spirv_ocl_copysign((half2)0.0f, x), __convert_short2(__spirv_IsInf(x)));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((short2)0), __convert_short2(__spirv_IsNan(x)) );
}

INLINE half3 __attribute__((overloadable)) __spirv_ocl_fract( half3 x,
                                           __global half3* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (short3)0 ), __convert_short3(__spirv_IsNan( x )) );
    half3 temp = x - *iptr;
    temp = __spirv_ocl_select( (half3)__spirv_ocl_fmin( temp, (half3)(0x1.ffcp-1h)), (half3)__spirv_ocl_copysign((half3)0.0f, x), __convert_short3(__spirv_IsInf(x)));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((short3)0), __convert_short3(__spirv_IsNan(x)) );
}

INLINE half4 __attribute__((overloadable)) __spirv_ocl_fract( half4 x,
                                           __global half4* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (short4)0 ), __convert_short4(__spirv_IsNan( x )) );
    half4 temp = x - *iptr;
    temp = __spirv_ocl_select( (half4)__spirv_ocl_fmin( temp, (half4)(0x1.ffcp-1h)), (half4)__spirv_ocl_copysign((half4)0.0f, x), __convert_short4(__spirv_IsInf(x)));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((short4)0), __convert_short4(__spirv_IsNan(x)) );
}

INLINE half8 __attribute__((overloadable)) __spirv_ocl_fract( half8 x,
                                           __global half8* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (short8)0 ), __convert_short8(__spirv_IsNan( x )) );
    half8 temp = x - *iptr;
    temp = __spirv_ocl_select( (half8)__spirv_ocl_fmin( temp, (half8)(0x1.ffcp-1h)), (half8)__spirv_ocl_copysign((half8)0.0f, x), __convert_short8(__spirv_IsInf(x)));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((short8)0), __convert_short8(__spirv_IsNan(x)) );
}

INLINE half16 __attribute__((overloadable)) __spirv_ocl_fract( half16 x,
                                              __global half16* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (short16)0 ), __convert_short16(__spirv_IsNan( x )) );
    half16 temp = x - *iptr;
    temp = __spirv_ocl_select( (half16)__spirv_ocl_fmin( temp, (half16)(0x1.ffcp-1h)), (half16)__spirv_ocl_copysign((half16)0.0f, x), __convert_short16(__spirv_IsInf(x)));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((short16)0), __convert_short16(__spirv_IsNan(x)) );
}

INLINE half __attribute__((overloadable)) __spirv_ocl_fract( half x,
                                      __private half* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (short)0 ), (short)(__spirv_IsNan( x )) );
    half temp = x - *iptr;
    temp = __spirv_ocl_select( (half)__spirv_ocl_fmin( temp, (half)(0x1.ffcp-1h)), (half)__spirv_ocl_copysign((half)0.0f, x), (short)__spirv_IsInf(x));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((short)0), (short)(__spirv_IsNan(x)) );
}

INLINE half2 __attribute__((overloadable)) __spirv_ocl_fract( half2 x,
                                           __private half2* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (short2)0 ), __convert_short2(__spirv_IsNan( x )) );
    half2 temp = x - *iptr;
    temp = __spirv_ocl_select( (half2)__spirv_ocl_fmin( temp, (half2)(0x1.ffcp-1h)), (half2)__spirv_ocl_copysign((half2)0.0f, x), __convert_short2(__spirv_IsInf(x)));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((short2)0), __convert_short2(__spirv_IsNan(x)) );
}

INLINE half3 __attribute__((overloadable)) __spirv_ocl_fract( half3 x,
                                           __private half3* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (short3)0 ), __convert_short3(__spirv_IsNan( x )) );
    half3 temp = x - *iptr;
    temp = __spirv_ocl_select( (half3)__spirv_ocl_fmin( temp, (half3)(0x1.ffcp-1h)), (half3)__spirv_ocl_copysign((half3)0.0f, x), __convert_short3(__spirv_IsInf(x)));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((short3)0), __convert_short3(__spirv_IsNan(x)) );
}

INLINE half4 __attribute__((overloadable)) __spirv_ocl_fract( half4 x,
                                           __private half4* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (short4)0 ), __convert_short4(__spirv_IsNan( x )) );
    half4 temp = x - *iptr;
    temp = __spirv_ocl_select( (half4)__spirv_ocl_fmin( temp, (half4)(0x1.ffcp-1h)), (half4)__spirv_ocl_copysign((half4)0.0f, x), __convert_short4(__spirv_IsInf(x)));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((short4)0), __convert_short4(__spirv_IsNan(x)) );
}

INLINE half8 __attribute__((overloadable)) __spirv_ocl_fract( half8 x,
                                           __private half8* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (short8)0 ), __convert_short8(__spirv_IsNan( x )) );
    half8 temp = x - *iptr;
    temp = __spirv_ocl_select( (half8)__spirv_ocl_fmin( temp, (half8)(0x1.ffcp-1h)), (half8)__spirv_ocl_copysign((half8)0.0f, x), __convert_short8(__spirv_IsInf(x)));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((short8)0), __convert_short8(__spirv_IsNan(x)) );
}

INLINE half16 __attribute__((overloadable)) __spirv_ocl_fract( half16 x,
                                              __private half16* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (short16)0 ), __convert_short16(__spirv_IsNan( x )) );
    half16 temp = x - *iptr;
    temp = __spirv_ocl_select( (half16)__spirv_ocl_fmin( temp, (half16)(0x1.ffcp-1h)), (half16)__spirv_ocl_copysign((half16)0.0f, x), __convert_short16(__spirv_IsInf(x)));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((short16)0), __convert_short16(__spirv_IsNan(x)) );
}

INLINE half __attribute__((overloadable)) __spirv_ocl_fract( half x,
                                      __local half* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (short)0 ), (short)(__spirv_IsNan( x )) );
    half temp = x - *iptr;
    temp = __spirv_ocl_select( (half)__spirv_ocl_fmin( temp, (half)(0x1.ffcp-1h)), (half)__spirv_ocl_copysign((half)0.0f, x), (short)__spirv_IsInf(x));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((short)0), (short)(__spirv_IsNan(x)) );
}

INLINE half2 __attribute__((overloadable)) __spirv_ocl_fract( half2 x,
                                           __local half2* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (short2)0 ), __convert_short2(__spirv_IsNan( x )) );
    half2 temp = x - *iptr;
    temp = __spirv_ocl_select( (half2)__spirv_ocl_fmin( temp, (half2)(0x1.ffcp-1h)), (half2)__spirv_ocl_copysign((half2)0.0f, x), __convert_short2(__spirv_IsInf(x)));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((short2)0), __convert_short2(__spirv_IsNan(x)) );
}

INLINE half3 __attribute__((overloadable)) __spirv_ocl_fract( half3 x,
                                           __local half3* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (short3)0 ), __convert_short3(__spirv_IsNan( x )) );
    half3 temp = x - *iptr;
    temp = __spirv_ocl_select( (half3)__spirv_ocl_fmin( temp, (half3)(0x1.ffcp-1h)), (half3)__spirv_ocl_copysign((half3)0.0f, x), __convert_short3(__spirv_IsInf(x)));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((short3)0), __convert_short3(__spirv_IsNan(x)) );
}

INLINE half4 __attribute__((overloadable)) __spirv_ocl_fract( half4 x,
                                           __local half4* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (short4)0 ), __convert_short4(__spirv_IsNan( x )) );
    half4 temp = x - *iptr;
    temp = __spirv_ocl_select( (half4)__spirv_ocl_fmin( temp, (half4)(0x1.ffcp-1h)), (half4)__spirv_ocl_copysign((half4)0.0f, x), __convert_short4(__spirv_IsInf(x)));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((short4)0), __convert_short4(__spirv_IsNan(x)) );
}

INLINE half8 __attribute__((overloadable)) __spirv_ocl_fract( half8 x,
                                           __local half8* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (short8)0 ), __convert_short8(__spirv_IsNan( x )) );
    half8 temp = x - *iptr;
    temp = __spirv_ocl_select( (half8)__spirv_ocl_fmin( temp, (half8)(0x1.ffcp-1h)), (half8)__spirv_ocl_copysign((half8)0.0f, x), __convert_short8(__spirv_IsInf(x)));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((short8)0), __convert_short8(__spirv_IsNan(x)) );
}

INLINE half16 __attribute__((overloadable)) __spirv_ocl_fract( half16 x,
                                              __local half16* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (short16)0 ), __convert_short16(__spirv_IsNan( x )) );
    half16 temp = x - *iptr;
    temp = __spirv_ocl_select( (half16)__spirv_ocl_fmin( temp, (half16)(0x1.ffcp-1h)), (half16)__spirv_ocl_copysign((half16)0.0f, x), __convert_short16(__spirv_IsInf(x)));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((short16)0), __convert_short16(__spirv_IsNan(x)) );
}
#endif

#if defined(cl_khr_fp16) && (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

INLINE half __attribute__((overloadable)) __spirv_ocl_fract( half x,
                                      __generic half* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (short)0 ), (short)(__spirv_IsNan( x )) );
    half temp = x - *iptr;
    temp = __spirv_ocl_select( (half)__spirv_ocl_fmin( temp, (half)(0x1.ffcp-1h)), (half)__spirv_ocl_copysign((half)0.0f, x), (short)__spirv_IsInf(x));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((short)0), (short)(__spirv_IsNan(x)) );
}

INLINE half2 __attribute__((overloadable)) __spirv_ocl_fract( half2 x,
                                           __generic half2* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (short2)0 ), __convert_short2(__spirv_IsNan( x )) );
    half2 temp = x - *iptr;
    temp = __spirv_ocl_select( (half2)__spirv_ocl_fmin( temp, (half2)(0x1.ffcp-1h)), (half2)__spirv_ocl_copysign((half2)0.0f, x), __convert_short2(__spirv_IsInf(x)));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((short2)0), __convert_short2(__spirv_IsNan(x)) );
}

INLINE half3 __attribute__((overloadable)) __spirv_ocl_fract( half3 x,
                                           __generic half3* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (short3)0 ), __convert_short3(__spirv_IsNan( x )) );
    half3 temp = x - *iptr;
    temp = __spirv_ocl_select( (half3)__spirv_ocl_fmin( temp, (half3)(0x1.ffcp-1h)), (half3)__spirv_ocl_copysign((half3)0.0f, x), __convert_short3(__spirv_IsInf(x)));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((short3)0), __convert_short3(__spirv_IsNan(x)) );
}

INLINE half4 __attribute__((overloadable)) __spirv_ocl_fract( half4 x,
                                           __generic half4* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (short4)0 ), __convert_short4(__spirv_IsNan( x )) );
    half4 temp = x - *iptr;
    temp = __spirv_ocl_select( (half4)__spirv_ocl_fmin( temp, (half4)(0x1.ffcp-1h)), (half4)__spirv_ocl_copysign((half4)0.0f, x), __convert_short4(__spirv_IsInf(x)));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((short4)0), __convert_short4(__spirv_IsNan(x)) );
}

INLINE half8 __attribute__((overloadable)) __spirv_ocl_fract( half8 x,
                                           __generic half8* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (short8)0 ), __convert_short8(__spirv_IsNan( x )) );
    half8 temp = x - *iptr;
    temp = __spirv_ocl_select( (half8)__spirv_ocl_fmin( temp, (half8)(0x1.ffcp-1h)), (half8)__spirv_ocl_copysign((half8)0.0f, x), __convert_short8(__spirv_IsInf(x)));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((short8)0), __convert_short8(__spirv_IsNan(x)) );
}

INLINE half16 __attribute__((overloadable)) __spirv_ocl_fract( half16 x,
                                              __generic half16* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (short16)0 ), __convert_short16(__spirv_IsNan( x )) );
    half16 temp = x - *iptr;
    temp = __spirv_ocl_select( (half16)__spirv_ocl_fmin( temp, (half16)(0x1.ffcp-1h)), (half16)__spirv_ocl_copysign((half16)0.0f, x), __convert_short16(__spirv_IsInf(x)));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((short16)0), __convert_short16(__spirv_IsNan(x)) );
}
#endif //if defined(cl_khr_fp16) && (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#if defined(cl_khr_fp64)

double __attribute__((overloadable)) __spirv_ocl_fract( double x,
                                        __global double* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (long)0 ), (long)(__spirv_IsNan( x )) );
    double temp = x - *iptr;
    temp = __spirv_ocl_select( __spirv_ocl_fmin( temp, 0x1.fffffffffffffp-1), __spirv_ocl_copysign(0.0, x), (long)__spirv_IsInf(x));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((long)0), (long)(__spirv_IsNan(x)) );
}

double2 __attribute__((overloadable)) __spirv_ocl_fract( double2 x,
                                             __global double2* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (long2)0 ), __convert_long2(__spirv_IsNan( x )) );
    double2 temp = x - *iptr;
    temp = __spirv_ocl_select( (double2)__spirv_ocl_fmin( temp, (double2)(0x1.fffffffffffffp-1)), (double2)__spirv_ocl_copysign((double2)0.0, x), __convert_long2(__spirv_IsInf(x)));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((long2)0), __convert_long2(__spirv_IsNan(x)) );
}

double3 __attribute__((overloadable)) __spirv_ocl_fract( double3 x,
                                             __global double3* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (long3)0 ), __convert_long3(__spirv_IsNan( x )) );
    double3 temp = x - *iptr;
    temp = __spirv_ocl_select( (double3)__spirv_ocl_fmin( temp, (double3)(0x1.fffffffffffffp-1)), (double3)__spirv_ocl_copysign((double3)0.0, x), __convert_long3(__spirv_IsInf(x)));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((long3)0), __convert_long3(__spirv_IsNan(x)) );
}

double4 __attribute__((overloadable)) __spirv_ocl_fract( double4 x,
                                             __global double4* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (long4)0 ), __convert_long4(__spirv_IsNan( x )) );
    double4 temp = x - *iptr;
    temp = __spirv_ocl_select( (double4)__spirv_ocl_fmin( temp, (double4)(0x1.fffffffffffffp-1)), (double4)__spirv_ocl_copysign((double4)0.0, x), __convert_long4(__spirv_IsInf(x)));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((long4)0), __convert_long4(__spirv_IsNan(x)) );
}

double8 __attribute__((overloadable)) __spirv_ocl_fract( double8 x,
                                             __global double8* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (long8)0 ), __convert_long8(__spirv_IsNan( x )) );
    double8 temp = x - *iptr;
    temp = __spirv_ocl_select( (double8)__spirv_ocl_fmin( temp, (double8)(0x1.fffffffffffffp-1)), (double8)__spirv_ocl_copysign((double8)0.0, x), __convert_long8(__spirv_IsInf(x)));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((long8)0), __convert_long8(__spirv_IsNan(x)) );
}

double16 __attribute__((overloadable)) __spirv_ocl_fract( double16 x,
                                                __global double16* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (long16)0 ), __convert_long16(__spirv_IsNan( x )) );
    double16 temp = x - *iptr;
    temp = __spirv_ocl_select( (double16)__spirv_ocl_fmin( temp, (double16)(0x1.fffffffffffffp-1)), (double16)__spirv_ocl_copysign((double16)0.0, x), __convert_long16(__spirv_IsInf(x)));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((long16)0), __convert_long16(__spirv_IsNan(x)) );
}

double __attribute__((overloadable)) __spirv_ocl_fract( double x,
                                        __private double* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (long)0 ), (long)(__spirv_IsNan( x )) );
    double temp = x - *iptr;
    temp = __spirv_ocl_select( (double)__spirv_ocl_fmin( temp, (double)(0x1.fffffffffffffp-1)), (double)__spirv_ocl_copysign((double)0.0, x), (long)__spirv_IsInf(x));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((long)0), (long)(__spirv_IsNan(x)) );
}

double2 __attribute__((overloadable)) __spirv_ocl_fract( double2 x,
                                             __private double2* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (long2)0 ), __convert_long2(__spirv_IsNan( x )) );
    double2 temp = x - *iptr;
    temp = __spirv_ocl_select( (double2)__spirv_ocl_fmin( temp, (double2)(0x1.fffffffffffffp-1)), (double2)__spirv_ocl_copysign((double2)0.0, x), __convert_long2(__spirv_IsInf(x)));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((long2)0), __convert_long2(__spirv_IsNan(x)) );
}

double3 __attribute__((overloadable)) __spirv_ocl_fract( double3 x,
                                             __private double3* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (long3)0 ), __convert_long3(__spirv_IsNan( x )) );
    double3 temp = x - *iptr;
    temp = __spirv_ocl_select( (double3)__spirv_ocl_fmin( temp, (double3)(0x1.fffffffffffffp-1)), (double3)__spirv_ocl_copysign((double3)0.0, x), __convert_long3(__spirv_IsInf(x)));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((long3)0), __convert_long3(__spirv_IsNan(x)) );
}

double4 __attribute__((overloadable)) __spirv_ocl_fract( double4 x,
                                             __private double4* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (long4)0 ), __convert_long4(__spirv_IsNan( x )) );
    double4 temp = x - *iptr;
    temp = __spirv_ocl_select( (double4)__spirv_ocl_fmin( temp, (double4)(0x1.fffffffffffffp-1)), (double4)__spirv_ocl_copysign((double4)0.0, x), __convert_long4(__spirv_IsInf(x)));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((long4)0), __convert_long4(__spirv_IsNan(x)) );
}

double8 __attribute__((overloadable)) __spirv_ocl_fract( double8 x,
                                             __private double8* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (long8)0 ), __convert_long8(__spirv_IsNan( x )) );
    double8 temp = x - *iptr;
    temp = __spirv_ocl_select( (double8)__spirv_ocl_fmin( temp, (double8)(0x1.fffffffffffffp-1)), (double8)__spirv_ocl_copysign((double8)0.0, x), __convert_long8(__spirv_IsInf(x)));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((long8)0), __convert_long8(__spirv_IsNan(x)) );
}

double16 __attribute__((overloadable)) __spirv_ocl_fract( double16 x,
                                                __private double16* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (long16)0 ), __convert_long16(__spirv_IsNan( x )) );
    double16 temp = x - *iptr;
    temp = __spirv_ocl_select( (double16)__spirv_ocl_fmin( temp, (double16)(0x1.fffffffffffffp-1)), (double16)__spirv_ocl_copysign((double16)0.0, x), __convert_long16(__spirv_IsInf(x)));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((long16)0), __convert_long16(__spirv_IsNan(x)) );
}

double __attribute__((overloadable)) __spirv_ocl_fract( double x,
                                        __local double* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (long)0 ), (long)(__spirv_IsNan( x )) );
    double temp = x - *iptr;
    temp = __spirv_ocl_select( (double)__spirv_ocl_fmin( temp, (double)(0x1.fffffffffffffp-1)), (double)__spirv_ocl_copysign((double)0.0, x), (long)__spirv_IsInf(x));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((long)0), (long)(__spirv_IsNan(x)) );
}

double2 __attribute__((overloadable)) __spirv_ocl_fract( double2 x,
                                             __local double2* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (long2)0 ), __convert_long2(__spirv_IsNan( x )) );
    double2 temp = x - *iptr;
    temp = __spirv_ocl_select( (double2)__spirv_ocl_fmin( temp, (double2)(0x1.fffffffffffffp-1)), (double2)__spirv_ocl_copysign((double2)0.0, x), __convert_long2(__spirv_IsInf(x)));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((long2)0), __convert_long2(__spirv_IsNan(x)) );
}

double3 __attribute__((overloadable)) __spirv_ocl_fract( double3 x,
                                             __local double3* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (long3)0 ), __convert_long3(__spirv_IsNan( x )) );
    double3 temp = x - *iptr;
    temp = __spirv_ocl_select( (double3)__spirv_ocl_fmin( temp, (double3)(0x1.fffffffffffffp-1)), (double3)__spirv_ocl_copysign((double3)0.0, x), __convert_long3(__spirv_IsInf(x)));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((long3)0), __convert_long3(__spirv_IsNan(x)) );
}

double4 __attribute__((overloadable)) __spirv_ocl_fract( double4 x,
                                             __local double4* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (long4)0 ), __convert_long4(__spirv_IsNan( x )) );
    double4 temp = x - *iptr;
    temp = __spirv_ocl_select( (double4)__spirv_ocl_fmin( temp, (double4)(0x1.fffffffffffffp-1)), (double4)__spirv_ocl_copysign((double4)0.0, x), __convert_long4(__spirv_IsInf(x)));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((long4)0), __convert_long4(__spirv_IsNan(x)) );
}

double8 __attribute__((overloadable)) __spirv_ocl_fract( double8 x,
                                             __local double8* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (long8)0 ), __convert_long8(__spirv_IsNan( x )) );
    double8 temp = x - *iptr;
    temp = __spirv_ocl_select( (double8)__spirv_ocl_fmin( temp, (double8)(0x1.fffffffffffffp-1)), (double8)__spirv_ocl_copysign((double8)0.0, x), __convert_long8(__spirv_IsInf(x)));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((long8)0), __convert_long8(__spirv_IsNan(x)) );
}

double16 __attribute__((overloadable)) __spirv_ocl_fract( double16 x,
                                                __local double16* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (long16)0 ), __convert_long16(__spirv_IsNan( x )) );
    double16 temp = x - *iptr;
    temp = __spirv_ocl_select( (double16)__spirv_ocl_fmin( temp, (double16)(0x1.fffffffffffffp-1)), (double16)__spirv_ocl_copysign((double16)0.0, x), __convert_long16(__spirv_IsInf(x)));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((long16)0), __convert_long16(__spirv_IsNan(x)) );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

double __attribute__((overloadable)) __spirv_ocl_fract( double x,
                                        __generic double* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (long)0 ), (long)(__spirv_IsNan( x )) );
    double temp = x - *iptr;
    temp = __spirv_ocl_select( (double)__spirv_ocl_fmin( temp, (double)(0x1.fffffffffffffp-1)), (double)__spirv_ocl_copysign((double)0.0, x), (long)__spirv_IsInf(x));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((long)0), (long)(__spirv_IsNan(x)) );
}

double2 __attribute__((overloadable)) __spirv_ocl_fract( double2 x,
                                             __generic double2* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (long2)0 ), __convert_long2(__spirv_IsNan( x )) );
    double2 temp = x - *iptr;
    temp = __spirv_ocl_select( (double2)__spirv_ocl_fmin( temp, (double2)(0x1.fffffffffffffp-1)), (double2)__spirv_ocl_copysign((double2)0.0, x), __convert_long2(__spirv_IsInf(x)));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((long2)0), __convert_long2(__spirv_IsNan(x)) );
}

double3 __attribute__((overloadable)) __spirv_ocl_fract( double3 x,
                                             __generic double3* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (long3)0 ), __convert_long3(__spirv_IsNan( x )) );
    double3 temp = x - *iptr;
    temp = __spirv_ocl_select( (double3)__spirv_ocl_fmin( temp, (double3)(0x1.fffffffffffffp-1)), (double3)__spirv_ocl_copysign((double3)0.0, x), __convert_long3(__spirv_IsInf(x)));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((long3)0), __convert_long3(__spirv_IsNan(x)) );
}

double4 __attribute__((overloadable)) __spirv_ocl_fract( double4 x,
                                             __generic double4* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (long4)0 ), __convert_long4(__spirv_IsNan( x )) );
    double4 temp = x - *iptr;
    temp = __spirv_ocl_select( (double4)__spirv_ocl_fmin( temp, (double4)(0x1.fffffffffffffp-1)), (double4)__spirv_ocl_copysign((double4)0.0, x), __convert_long4(__spirv_IsInf(x)));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((long4)0), __convert_long4(__spirv_IsNan(x)) );
}

double8 __attribute__((overloadable)) __spirv_ocl_fract( double8 x,
                                             __generic double8* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (long8)0 ), __convert_long8(__spirv_IsNan( x )) );
    double8 temp = x - *iptr;
    temp = __spirv_ocl_select( (double8)__spirv_ocl_fmin( temp, (double8)(0x1.fffffffffffffp-1)), (double8)__spirv_ocl_copysign((double8)0.0, x), __convert_long8(__spirv_IsInf(x)));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((long8)0), __convert_long8(__spirv_IsNan(x)) );
}

double16 __attribute__((overloadable)) __spirv_ocl_fract( double16 x,
                                                __generic double16* iptr )
{
    *iptr = __spirv_ocl_select( __spirv_ocl_floor( x ), __spirv_ocl_nan( (long16)0 ), __convert_long16(__spirv_IsNan( x )) );
    double16 temp = x - *iptr;
    temp = __spirv_ocl_select( (double16)__spirv_ocl_fmin( temp, (double16)(0x1.fffffffffffffp-1)), (double16)__spirv_ocl_copysign((double16)0.0, x), __convert_long16(__spirv_IsInf(x)));
    return __spirv_ocl_select( temp, __spirv_ocl_nan((long16)0), __convert_long16(__spirv_IsNan(x)) );
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
#endif // defined(cl_khr_fp64)

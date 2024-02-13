/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _f32_p1f32, )( float x,
                                       __global float* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _f32_f32_i32, )( SPIRV_OCL_BUILTIN(floor, _f32, )( x ), SPIRV_OCL_BUILTIN(nan, _i32, )( 0 ), (int)(__intel_relaxed_isnan( x )) );
    float temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _f32_f32_i32, )( (float)SPIRV_OCL_BUILTIN(fmin, _f32_f32, )( temp, (float)(0x1.fffffep-1f)), (float)SPIRV_OCL_BUILTIN(copysign, _f32_f32, )((float)0.0f, x), (int)__intel_relaxed_isinf(x));
    return SPIRV_OCL_BUILTIN(select, _f32_f32_i32, )( temp, SPIRV_OCL_BUILTIN(nan, _i32, )(0), (int)(__intel_relaxed_isnan(x)) );
}

INLINE float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v2f32_p1v2f32, )( float2 x,
                                            __global float2* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _v2f32_v2f32_v2i32, )( SPIRV_OCL_BUILTIN(floor, _v2f32, )( x ), SPIRV_OCL_BUILTIN(nan, _v2i32, )( (int2)0 ), __convert_int2(SPIRV_BUILTIN(IsNan, _v2f32, )( x )) );
    float2 temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _v2f32_v2f32_v2i32, )( (float2)SPIRV_OCL_BUILTIN(fmin, _v2f32_v2f32, )( temp, (float2)(0x1.fffffep-1f)), (float2)SPIRV_OCL_BUILTIN(copysign, _v2f32_v2f32, )((float2)0.0f, x), __convert_int2(SPIRV_BUILTIN(IsInf, _v2f32, )(x)));
    return SPIRV_OCL_BUILTIN(select, _v2f32_v2f32_v2i32, )( temp, SPIRV_OCL_BUILTIN(nan, _v2i32, )((int2)0), __convert_int2(SPIRV_BUILTIN(IsNan, _v2f32, )(x)) );
}

INLINE float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v3f32_p1v3f32, )( float3 x,
                                            __global float3* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _v3f32_v3f32_v3i32, )( SPIRV_OCL_BUILTIN(floor, _v3f32, )( x ), SPIRV_OCL_BUILTIN(nan, _v3i32, )( (int3)0 ), __convert_int3(SPIRV_BUILTIN(IsNan, _v3f32, )( x )) );
    float3 temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _v3f32_v3f32_v3i32, )( (float3)SPIRV_OCL_BUILTIN(fmin, _v3f32_v3f32, )( temp, (float3)(0x1.fffffep-1f)), (float3)SPIRV_OCL_BUILTIN(copysign, _v3f32_v3f32, )((float3)0.0f, x), __convert_int3(SPIRV_BUILTIN(IsInf, _v3f32, )(x)));
    return SPIRV_OCL_BUILTIN(select, _v3f32_v3f32_v3i32, )( temp, SPIRV_OCL_BUILTIN(nan, _v3i32, )((int3)0), __convert_int3(SPIRV_BUILTIN(IsNan, _v3f32, )(x)) );
}

INLINE float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v4f32_p1v4f32, )( float4 x,
                                            __global float4* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _v4f32_v4f32_v4i32, )( SPIRV_OCL_BUILTIN(floor, _v4f32, )( x ), SPIRV_OCL_BUILTIN(nan, _v4i32, )( (int4)0 ), __convert_int4(SPIRV_BUILTIN(IsNan, _v4f32, )( x )) );
    float4 temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _v4f32_v4f32_v4i32, )( (float4)SPIRV_OCL_BUILTIN(fmin, _v4f32_v4f32, )( temp, (float4)(0x1.fffffep-1f)), (float4)SPIRV_OCL_BUILTIN(copysign, _v4f32_v4f32, )((float4)0.0f, x), __convert_int4(SPIRV_BUILTIN(IsInf, _v4f32, )(x)));
    return SPIRV_OCL_BUILTIN(select, _v4f32_v4f32_v4i32, )( temp, SPIRV_OCL_BUILTIN(nan, _v4i32, )((int4)0), __convert_int4(SPIRV_BUILTIN(IsNan, _v4f32, )(x)) );
}

INLINE float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v8f32_p1v8f32, )( float8 x,
                                            __global float8* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _v8f32_v8f32_v8i32, )( SPIRV_OCL_BUILTIN(floor, _v8f32, )( x ), SPIRV_OCL_BUILTIN(nan, _v8i32, )( (int8)0 ), __convert_int8(SPIRV_BUILTIN(IsNan, _v8f32, )( x )) );
    float8 temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _v8f32_v8f32_v8i32, )( (float8)SPIRV_OCL_BUILTIN(fmin, _v8f32_v8f32, )( temp, (float8)(0x1.fffffep-1f)), (float8)SPIRV_OCL_BUILTIN(copysign, _v8f32_v8f32, )((float8)0.0f, x), __convert_int8(SPIRV_BUILTIN(IsInf, _v8f32, )(x)));
    return SPIRV_OCL_BUILTIN(select, _v8f32_v8f32_v8i32, )( temp, SPIRV_OCL_BUILTIN(nan, _v8i32, )((int8)0), __convert_int8(SPIRV_BUILTIN(IsNan, _v8f32, )(x)) );
}

INLINE float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v16f32_p1v16f32, )( float16 x,
                                               __global float16* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _v16f32_v16f32_v16i32, )( SPIRV_OCL_BUILTIN(floor, _v16f32, )( x ), SPIRV_OCL_BUILTIN(nan, _v16i32, )( (int16)0 ), __convert_int16(SPIRV_BUILTIN(IsNan, _v16f32, )( x )) );
    float16 temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _v16f32_v16f32_v16i32, )( (float16)SPIRV_OCL_BUILTIN(fmin, _v16f32_v16f32, )( temp, (float16)(0x1.fffffep-1f)), (float16)SPIRV_OCL_BUILTIN(copysign, _v16f32_v16f32, )((float16)0.0f, x), __convert_int16(SPIRV_BUILTIN(IsInf, _v16f32, )(x)));
    return SPIRV_OCL_BUILTIN(select, _v16f32_v16f32_v16i32, )( temp, SPIRV_OCL_BUILTIN(nan, _v16i32, )((int16)0), __convert_int16(SPIRV_BUILTIN(IsNan, _v16f32, )(x)) );
}

INLINE float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _f32_p0f32, )( float x,
                                       __private float* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _f32_f32_i32, )( SPIRV_OCL_BUILTIN(floor, _f32, )( x ), SPIRV_OCL_BUILTIN(nan, _i32, )( 0 ), (int)(__intel_relaxed_isnan( x )) );
    float temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _f32_f32_i32, )( (float)SPIRV_OCL_BUILTIN(fmin, _f32_f32, )( temp, (float)(0x1.fffffep-1f)), (float)SPIRV_OCL_BUILTIN(copysign, _f32_f32, )((float)0.0f, x), (int)__intel_relaxed_isinf(x));
    return SPIRV_OCL_BUILTIN(select, _f32_f32_i32, )( temp, SPIRV_OCL_BUILTIN(nan, _i32, )(0), (int)(__intel_relaxed_isnan(x)) );
}

INLINE float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v2f32_p0v2f32, )( float2 x,
                                            __private float2* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _v2f32_v2f32_v2i32, )( SPIRV_OCL_BUILTIN(floor, _v2f32, )( x ), SPIRV_OCL_BUILTIN(nan, _v2i32, )( (int2)0 ), __convert_int2(SPIRV_BUILTIN(IsNan, _v2f32, )( x )) );
    float2 temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _v2f32_v2f32_v2i32, )( (float2)SPIRV_OCL_BUILTIN(fmin, _v2f32_v2f32, )( temp, (float2)(0x1.fffffep-1f)), (float2)SPIRV_OCL_BUILTIN(copysign, _v2f32_v2f32, )((float2)0.0f, x), __convert_int2(SPIRV_BUILTIN(IsInf, _v2f32, )(x)));
    return SPIRV_OCL_BUILTIN(select, _v2f32_v2f32_v2i32, )( temp, SPIRV_OCL_BUILTIN(nan, _v2i32, )((int2)0), __convert_int2(SPIRV_BUILTIN(IsNan, _v2f32, )(x)) );
}

INLINE float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v3f32_p0v3f32, )( float3 x,
                                            __private float3* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _v3f32_v3f32_v3i32, )( SPIRV_OCL_BUILTIN(floor, _v3f32, )( x ), SPIRV_OCL_BUILTIN(nan, _v3i32, )( (int3)0 ), __convert_int3(SPIRV_BUILTIN(IsNan, _v3f32, )( x )) );
    float3 temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _v3f32_v3f32_v3i32, )( (float3)SPIRV_OCL_BUILTIN(fmin, _v3f32_v3f32, )( temp, (float3)(0x1.fffffep-1f)), (float3)SPIRV_OCL_BUILTIN(copysign, _v3f32_v3f32, )((float3)0.0f, x), __convert_int3(SPIRV_BUILTIN(IsInf, _v3f32, )(x)));
    return SPIRV_OCL_BUILTIN(select, _v3f32_v3f32_v3i32, )( temp, SPIRV_OCL_BUILTIN(nan, _v3i32, )((int3)0), __convert_int3(SPIRV_BUILTIN(IsNan, _v3f32, )(x)) );
}

INLINE float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v4f32_p0v4f32, )( float4 x,
                                            __private float4* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _v4f32_v4f32_v4i32, )( SPIRV_OCL_BUILTIN(floor, _v4f32, )( x ), SPIRV_OCL_BUILTIN(nan, _v4i32, )( (int4)0 ), __convert_int4(SPIRV_BUILTIN(IsNan, _v4f32, )( x )) );
    float4 temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _v4f32_v4f32_v4i32, )( (float4)SPIRV_OCL_BUILTIN(fmin, _v4f32_v4f32, )( temp, (float4)(0x1.fffffep-1f)), (float4)SPIRV_OCL_BUILTIN(copysign, _v4f32_v4f32, )((float4)0.0f, x), __convert_int4(SPIRV_BUILTIN(IsInf, _v4f32, )(x)));
    return SPIRV_OCL_BUILTIN(select, _v4f32_v4f32_v4i32, )( temp, SPIRV_OCL_BUILTIN(nan, _v4i32, )((int4)0), __convert_int4(SPIRV_BUILTIN(IsNan, _v4f32, )(x)) );
}

INLINE float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v8f32_p0v8f32, )( float8 x,
                                            __private float8* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _v8f32_v8f32_v8i32, )( SPIRV_OCL_BUILTIN(floor, _v8f32, )( x ), SPIRV_OCL_BUILTIN(nan, _v8i32, )( (int8)0 ), __convert_int8(SPIRV_BUILTIN(IsNan, _v8f32, )( x )) );
    float8 temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _v8f32_v8f32_v8i32, )( (float8)SPIRV_OCL_BUILTIN(fmin, _v8f32_v8f32, )( temp, (float8)(0x1.fffffep-1f)), (float8)SPIRV_OCL_BUILTIN(copysign, _v8f32_v8f32, )((float8)0.0f, x), __convert_int8(SPIRV_BUILTIN(IsInf, _v8f32, )(x)));
    return SPIRV_OCL_BUILTIN(select, _v8f32_v8f32_v8i32, )( temp, SPIRV_OCL_BUILTIN(nan, _v8i32, )((int8)0), __convert_int8(SPIRV_BUILTIN(IsNan, _v8f32, )(x)) );
}

INLINE float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v16f32_p0v16f32, )( float16 x,
                                               __private float16* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _v16f32_v16f32_v16i32, )( SPIRV_OCL_BUILTIN(floor, _v16f32, )( x ), SPIRV_OCL_BUILTIN(nan, _v16i32, )( (int16)0 ), __convert_int16(SPIRV_BUILTIN(IsNan, _v16f32, )( x )) );
    float16 temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _v16f32_v16f32_v16i32, )( (float16)SPIRV_OCL_BUILTIN(fmin, _v16f32_v16f32, )( temp, (float16)(0x1.fffffep-1f)), (float16)SPIRV_OCL_BUILTIN(copysign, _v16f32_v16f32, )((float16)0.0f, x), __convert_int16(SPIRV_BUILTIN(IsInf, _v16f32, )(x)));
    return SPIRV_OCL_BUILTIN(select, _v16f32_v16f32_v16i32, )( temp, SPIRV_OCL_BUILTIN(nan, _v16i32, )((int16)0), __convert_int16(SPIRV_BUILTIN(IsNan, _v16f32, )(x)) );
}

INLINE float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _f32_p3f32, )( float x,
                                       __local float* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _f32_f32_i32, )( SPIRV_OCL_BUILTIN(floor, _f32, )( x ), SPIRV_OCL_BUILTIN(nan, _i32, )( (int)0 ), (int)(__intel_relaxed_isnan( x )) );
    float temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _f32_f32_i32, )( (float)SPIRV_OCL_BUILTIN(fmin, _f32_f32, )( temp, (float)(0x1.fffffep-1f)), (float)SPIRV_OCL_BUILTIN(copysign, _f32_f32, )((float)0.0f, x), (int)__intel_relaxed_isinf(x));
    return SPIRV_OCL_BUILTIN(select, _f32_f32_i32, )( temp, SPIRV_OCL_BUILTIN(nan, _i32, )((int)0), (int)(__intel_relaxed_isnan(x)) );
}

INLINE float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v2f32_p3v2f32, )( float2 x,
                                            __local float2* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _v2f32_v2f32_v2i32, )( SPIRV_OCL_BUILTIN(floor, _v2f32, )( x ), SPIRV_OCL_BUILTIN(nan, _v2i32, )( (int2)0 ), __convert_int2(SPIRV_BUILTIN(IsNan, _v2f32, )( x )) );
    float2 temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _v2f32_v2f32_v2i32, )( (float2)SPIRV_OCL_BUILTIN(fmin, _v2f32_v2f32, )( temp, (float2)(0x1.fffffep-1f)), (float2)SPIRV_OCL_BUILTIN(copysign, _v2f32_v2f32, )((float2)0.0f, x), __convert_int2(SPIRV_BUILTIN(IsInf, _v2f32, )(x)));
    return SPIRV_OCL_BUILTIN(select, _v2f32_v2f32_v2i32, )( temp, SPIRV_OCL_BUILTIN(nan, _v2i32, )((int2)0), __convert_int2(SPIRV_BUILTIN(IsNan, _v2f32, )(x)) );
}

INLINE float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v3f32_p3v3f32, )( float3 x,
                                            __local float3* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _v3f32_v3f32_v3i32, )( SPIRV_OCL_BUILTIN(floor, _v3f32, )( x ), SPIRV_OCL_BUILTIN(nan, _v3i32, )( (int3)0 ), __convert_int3(SPIRV_BUILTIN(IsNan, _v3f32, )( x )) );
    float3 temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _v3f32_v3f32_v3i32, )( (float3)SPIRV_OCL_BUILTIN(fmin, _v3f32_v3f32, )( temp, (float3)(0x1.fffffep-1f)), (float3)SPIRV_OCL_BUILTIN(copysign, _v3f32_v3f32, )((float3)0.0f, x), __convert_int3(SPIRV_BUILTIN(IsInf, _v3f32, )(x)));
    return SPIRV_OCL_BUILTIN(select, _v3f32_v3f32_v3i32, )( temp, SPIRV_OCL_BUILTIN(nan, _v3i32, )((int3)0), __convert_int3(SPIRV_BUILTIN(IsNan, _v3f32, )(x)) );
}

INLINE float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v4f32_p3v4f32, )( float4 x,
                                            __local float4* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _v4f32_v4f32_v4i32, )( SPIRV_OCL_BUILTIN(floor, _v4f32, )( x ), SPIRV_OCL_BUILTIN(nan, _v4i32, )( (int4)0 ), __convert_int4(SPIRV_BUILTIN(IsNan, _v4f32, )( x )) );
    float4 temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _v4f32_v4f32_v4i32, )( (float4)SPIRV_OCL_BUILTIN(fmin, _v4f32_v4f32, )( temp, (float4)(0x1.fffffep-1f)), (float4)SPIRV_OCL_BUILTIN(copysign, _v4f32_v4f32, )((float4)0.0f, x), __convert_int4(SPIRV_BUILTIN(IsInf, _v4f32, )(x)));
    return SPIRV_OCL_BUILTIN(select, _v4f32_v4f32_v4i32, )( temp, SPIRV_OCL_BUILTIN(nan, _v4i32, )((int4)0), __convert_int4(SPIRV_BUILTIN(IsNan, _v4f32, )(x)) );
}

INLINE float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v8f32_p3v8f32, )( float8 x,
                                            __local float8* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _v8f32_v8f32_v8i32, )( SPIRV_OCL_BUILTIN(floor, _v8f32, )( x ), SPIRV_OCL_BUILTIN(nan, _v8i32, )( (int8)0 ), __convert_int8(SPIRV_BUILTIN(IsNan, _v8f32, )( x )) );
    float8 temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _v8f32_v8f32_v8i32, )( (float8)SPIRV_OCL_BUILTIN(fmin, _v8f32_v8f32, )( temp, (float8)(0x1.fffffep-1f)), (float8)SPIRV_OCL_BUILTIN(copysign, _v8f32_v8f32, )((float8)0.0f, x), __convert_int8(SPIRV_BUILTIN(IsInf, _v8f32, )(x)));
    return SPIRV_OCL_BUILTIN(select, _v8f32_v8f32_v8i32, )( temp, SPIRV_OCL_BUILTIN(nan, _v8i32, )((int8)0), __convert_int8(SPIRV_BUILTIN(IsNan, _v8f32, )(x)) );
}

INLINE float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v16f32_p3v16f32, )( float16 x,
                                               __local float16* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _v16f32_v16f32_v16i32, )( SPIRV_OCL_BUILTIN(floor, _v16f32, )( x ), SPIRV_OCL_BUILTIN(nan, _v16i32, )( (int16)0 ), __convert_int16(SPIRV_BUILTIN(IsNan, _v16f32, )( x )) );
    float16 temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _v16f32_v16f32_v16i32, )( (float16)SPIRV_OCL_BUILTIN(fmin, _v16f32_v16f32, )( temp, (float16)(0x1.fffffep-1f)), (float16)SPIRV_OCL_BUILTIN(copysign, _v16f32_v16f32, )((float16)0.0f, x), __convert_int16(SPIRV_BUILTIN(IsInf, _v16f32, )(x)));
    return SPIRV_OCL_BUILTIN(select, _v16f32_v16f32_v16i32, )( temp, SPIRV_OCL_BUILTIN(nan, _v16i32, )((int16)0), __convert_int16(SPIRV_BUILTIN(IsNan, _v16f32, )(x)) );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

INLINE float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _f32_p4f32, )( float x,
                                       __generic float* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _f32_f32_i32, )( SPIRV_OCL_BUILTIN(floor, _f32, )( x ), SPIRV_OCL_BUILTIN(nan, _i32, )( (int)0 ), (int)(__intel_relaxed_isnan( x )) );
    float temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _f32_f32_i32, )( (float)SPIRV_OCL_BUILTIN(fmin, _f32_f32, )( temp, (float)(0x1.fffffep-1f)), (float)SPIRV_OCL_BUILTIN(copysign, _f32_f32, )((float)0.0f, x), (int)__intel_relaxed_isinf(x));
    return SPIRV_OCL_BUILTIN(select, _f32_f32_i32, )( temp, SPIRV_OCL_BUILTIN(nan, _i32, )((int)0), (int)(__intel_relaxed_isnan(x)) );
}

INLINE float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v2f32_p4v2f32, )( float2 x,
                                            __generic float2* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _v2f32_v2f32_v2i32, )( SPIRV_OCL_BUILTIN(floor, _v2f32, )( x ), SPIRV_OCL_BUILTIN(nan, _v2i32, )( (int2)0 ), __convert_int2(SPIRV_BUILTIN(IsNan, _v2f32, )( x )) );
    float2 temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _v2f32_v2f32_v2i32, )( (float2)SPIRV_OCL_BUILTIN(fmin, _v2f32_v2f32, )( temp, (float2)(0x1.fffffep-1f)), (float2)SPIRV_OCL_BUILTIN(copysign, _v2f32_v2f32, )((float2)0.0f, x), __convert_int2(SPIRV_BUILTIN(IsInf, _v2f32, )(x)));
    return SPIRV_OCL_BUILTIN(select, _v2f32_v2f32_v2i32, )( temp, SPIRV_OCL_BUILTIN(nan, _v2i32, )((int2)0), __convert_int2(SPIRV_BUILTIN(IsNan, _v2f32, )(x)) );
}

INLINE float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v3f32_p4v3f32, )( float3 x,
                                            __generic float3* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _v3f32_v3f32_v3i32, )( SPIRV_OCL_BUILTIN(floor, _v3f32, )( x ), SPIRV_OCL_BUILTIN(nan, _v3i32, )( (int3)0 ), __convert_int3(SPIRV_BUILTIN(IsNan, _v3f32, )( x )) );
    float3 temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _v3f32_v3f32_v3i32, )( (float3)SPIRV_OCL_BUILTIN(fmin, _v3f32_v3f32, )( temp, (float3)(0x1.fffffep-1f)), (float3)SPIRV_OCL_BUILTIN(copysign, _v3f32_v3f32, )((float3)0.0f, x), __convert_int3(SPIRV_BUILTIN(IsInf, _v3f32, )(x)));
    return SPIRV_OCL_BUILTIN(select, _v3f32_v3f32_v3i32, )( temp, SPIRV_OCL_BUILTIN(nan, _v3i32, )((int3)0), __convert_int3(SPIRV_BUILTIN(IsNan, _v3f32, )(x)) );
}

INLINE float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v4f32_p4v4f32, )( float4 x,
                                            __generic float4* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _v4f32_v4f32_v4i32, )( SPIRV_OCL_BUILTIN(floor, _v4f32, )( x ), SPIRV_OCL_BUILTIN(nan, _v4i32, )( (int4)0 ), __convert_int4(SPIRV_BUILTIN(IsNan, _v4f32, )( x )) );
    float4 temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _v4f32_v4f32_v4i32, )( (float4)SPIRV_OCL_BUILTIN(fmin, _v4f32_v4f32, )( temp, (float4)(0x1.fffffep-1f)), (float4)SPIRV_OCL_BUILTIN(copysign, _v4f32_v4f32, )((float4)0.0f, x), __convert_int4(SPIRV_BUILTIN(IsInf, _v4f32, )(x)));
    return SPIRV_OCL_BUILTIN(select, _v4f32_v4f32_v4i32, )( temp, SPIRV_OCL_BUILTIN(nan, _v4i32, )((int4)0), __convert_int4(SPIRV_BUILTIN(IsNan, _v4f32, )(x)) );
}

INLINE float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v8f32_p4v8f32, )( float8 x,
                                            __generic float8* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _v8f32_v8f32_v8i32, )( SPIRV_OCL_BUILTIN(floor, _v8f32, )( x ), SPIRV_OCL_BUILTIN(nan, _v8i32, )( (int8)0 ), __convert_int8(SPIRV_BUILTIN(IsNan, _v8f32, )( x )) );
    float8 temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _v8f32_v8f32_v8i32, )( (float8)SPIRV_OCL_BUILTIN(fmin, _v8f32_v8f32, )( temp, (float8)(0x1.fffffep-1f)), (float8)SPIRV_OCL_BUILTIN(copysign, _v8f32_v8f32, )((float8)0.0f, x), __convert_int8(SPIRV_BUILTIN(IsInf, _v8f32, )(x)));
    return SPIRV_OCL_BUILTIN(select, _v8f32_v8f32_v8i32, )( temp, SPIRV_OCL_BUILTIN(nan, _v8i32, )((int8)0), __convert_int8(SPIRV_BUILTIN(IsNan, _v8f32, )(x)) );
}

INLINE float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v16f32_p4v16f32, )( float16 x,
                                               __generic float16* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _v16f32_v16f32_v16i32, )( SPIRV_OCL_BUILTIN(floor, _v16f32, )( x ), SPIRV_OCL_BUILTIN(nan, _v16i32, )( (int16)0 ), __convert_int16(SPIRV_BUILTIN(IsNan, _v16f32, )( x )) );
    float16 temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _v16f32_v16f32_v16i32, )( (float16)SPIRV_OCL_BUILTIN(fmin, _v16f32_v16f32, )( temp, (float16)(0x1.fffffep-1f)), (float16)SPIRV_OCL_BUILTIN(copysign, _v16f32_v16f32, )((float16)0.0f, x), __convert_int16(SPIRV_BUILTIN(IsInf, _v16f32, )(x)));
    return SPIRV_OCL_BUILTIN(select, _v16f32_v16f32_v16i32, )( temp, SPIRV_OCL_BUILTIN(nan, _v16i32, )((int16)0), __convert_int16(SPIRV_BUILTIN(IsNan, _v16f32, )(x)) );
}

#endif //#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)


#ifdef cl_khr_fp16
INLINE half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _f16_p1f16, )( half x,
                                      __global half* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _f16_f16_i16, )( SPIRV_OCL_BUILTIN(floor, _f16, )( x ), SPIRV_OCL_BUILTIN(nan, _i16, )( (short)0 ), (short)(SPIRV_BUILTIN(IsNan, _f16, )( x )) );
    half temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _f16_f16_i16, )( (half)SPIRV_OCL_BUILTIN(fmin, _f16_f16, )( temp, (half)(0x1.fffffep-1f)), (half)SPIRV_OCL_BUILTIN(copysign, _f16_f16, )((half)0.0f, x), (short)SPIRV_BUILTIN(IsInf, _f16, )(x));
    return SPIRV_OCL_BUILTIN(select, _f16_f16_i16, )( temp, SPIRV_OCL_BUILTIN(nan, _i16, )((short)0), (short)(SPIRV_BUILTIN(IsNan, _f16, )(x)) );
}

INLINE half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v2f16_p1v2f16, )( half2 x,
                                           __global half2* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _v2f16_v2f16_v2i16, )( SPIRV_OCL_BUILTIN(floor, _v2f16, )( x ), SPIRV_OCL_BUILTIN(nan, _v2i16, )( (short2)0 ), __convert_short2(SPIRV_BUILTIN(IsNan, _v2f16, )( x )) );
    half2 temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _v2f16_v2f16_v2i16, )( (half2)SPIRV_OCL_BUILTIN(fmin, _v2f16_v2f16, )( temp, (half2)(0x1.fffffep-1f)), (half2)SPIRV_OCL_BUILTIN(copysign, _v2f16_v2f16, )((half2)0.0f, x), __convert_short2(SPIRV_BUILTIN(IsInf, _v2f16, )(x)));
    return SPIRV_OCL_BUILTIN(select, _v2f16_v2f16_v2i16, )( temp, SPIRV_OCL_BUILTIN(nan, _v2i16, )((short2)0), __convert_short2(SPIRV_BUILTIN(IsNan, _v2f16, )(x)) );
}

INLINE half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v3f16_p1v3f16, )( half3 x,
                                           __global half3* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _v3f16_v3f16_v3i16, )( SPIRV_OCL_BUILTIN(floor, _v3f16, )( x ), SPIRV_OCL_BUILTIN(nan, _v3i16, )( (short3)0 ), __convert_short3(SPIRV_BUILTIN(IsNan, _v3f16, )( x )) );
    half3 temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _v3f16_v3f16_v3i16, )( (half3)SPIRV_OCL_BUILTIN(fmin, _v3f16_v3f16, )( temp, (half3)(0x1.fffffep-1f)), (half3)SPIRV_OCL_BUILTIN(copysign, _v3f16_v3f16, )((half3)0.0f, x), __convert_short3(SPIRV_BUILTIN(IsInf, _v3f16, )(x)));
    return SPIRV_OCL_BUILTIN(select, _v3f16_v3f16_v3i16, )( temp, SPIRV_OCL_BUILTIN(nan, _v3i16, )((short3)0), __convert_short3(SPIRV_BUILTIN(IsNan, _v3f16, )(x)) );
}

INLINE half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v4f16_p1v4f16, )( half4 x,
                                           __global half4* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _v4f16_v4f16_v4i16, )( SPIRV_OCL_BUILTIN(floor, _v4f16, )( x ), SPIRV_OCL_BUILTIN(nan, _v4i16, )( (short4)0 ), __convert_short4(SPIRV_BUILTIN(IsNan, _v4f16, )( x )) );
    half4 temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _v4f16_v4f16_v4i16, )( (half4)SPIRV_OCL_BUILTIN(fmin, _v4f16_v4f16, )( temp, (half4)(0x1.fffffep-1f)), (half4)SPIRV_OCL_BUILTIN(copysign, _v4f16_v4f16, )((half4)0.0f, x), __convert_short4(SPIRV_BUILTIN(IsInf, _v4f16, )(x)));
    return SPIRV_OCL_BUILTIN(select, _v4f16_v4f16_v4i16, )( temp, SPIRV_OCL_BUILTIN(nan, _v4i16, )((short4)0), __convert_short4(SPIRV_BUILTIN(IsNan, _v4f16, )(x)) );
}

INLINE half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v8f16_p1v8f16, )( half8 x,
                                           __global half8* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _v8f16_v8f16_v8i16, )( SPIRV_OCL_BUILTIN(floor, _v8f16, )( x ), SPIRV_OCL_BUILTIN(nan, _v8i16, )( (short8)0 ), __convert_short8(SPIRV_BUILTIN(IsNan, _v8f16, )( x )) );
    half8 temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _v8f16_v8f16_v8i16, )( (half8)SPIRV_OCL_BUILTIN(fmin, _v8f16_v8f16, )( temp, (half8)(0x1.fffffep-1f)), (half8)SPIRV_OCL_BUILTIN(copysign, _v8f16_v8f16, )((half8)0.0f, x), __convert_short8(SPIRV_BUILTIN(IsInf, _v8f16, )(x)));
    return SPIRV_OCL_BUILTIN(select, _v8f16_v8f16_v8i16, )( temp, SPIRV_OCL_BUILTIN(nan, _v8i16, )((short8)0), __convert_short8(SPIRV_BUILTIN(IsNan, _v8f16, )(x)) );
}

INLINE half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v16f16_p1v16f16, )( half16 x,
                                              __global half16* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _v16f16_v16f16_v16i16, )( SPIRV_OCL_BUILTIN(floor, _v16f16, )( x ), SPIRV_OCL_BUILTIN(nan, _v16i16, )( (short16)0 ), __convert_short16(SPIRV_BUILTIN(IsNan, _v16f16, )( x )) );
    half16 temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _v16f16_v16f16_v16i16, )( (half16)SPIRV_OCL_BUILTIN(fmin, _v16f16_v16f16, )( temp, (half16)(0x1.fffffep-1f)), (half16)SPIRV_OCL_BUILTIN(copysign, _v16f16_v16f16, )((half16)0.0f, x), __convert_short16(SPIRV_BUILTIN(IsInf, _v16f16, )(x)));
    return SPIRV_OCL_BUILTIN(select, _v16f16_v16f16_v16i16, )( temp, SPIRV_OCL_BUILTIN(nan, _v16i16, )((short16)0), __convert_short16(SPIRV_BUILTIN(IsNan, _v16f16, )(x)) );
}

INLINE half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _f16_p0f16, )( half x,
                                      __private half* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _f16_f16_i16, )( SPIRV_OCL_BUILTIN(floor, _f16, )( x ), SPIRV_OCL_BUILTIN(nan, _i16, )( (short)0 ), (short)(SPIRV_BUILTIN(IsNan, _f16, )( x )) );
    half temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _f16_f16_i16, )( (half)SPIRV_OCL_BUILTIN(fmin, _f16_f16, )( temp, (half)(0x1.fffffep-1f)), (half)SPIRV_OCL_BUILTIN(copysign, _f16_f16, )((half)0.0f, x), (short)SPIRV_BUILTIN(IsInf, _f16, )(x));
    return SPIRV_OCL_BUILTIN(select, _f16_f16_i16, )( temp, SPIRV_OCL_BUILTIN(nan, _i16, )((short)0), (short)(SPIRV_BUILTIN(IsNan, _f16, )(x)) );
}

INLINE half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v2f16_p0v2f16, )( half2 x,
                                           __private half2* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _v2f16_v2f16_v2i16, )( SPIRV_OCL_BUILTIN(floor, _v2f16, )( x ), SPIRV_OCL_BUILTIN(nan, _v2i16, )( (short2)0 ), __convert_short2(SPIRV_BUILTIN(IsNan, _v2f16, )( x )) );
    half2 temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _v2f16_v2f16_v2i16, )( (half2)SPIRV_OCL_BUILTIN(fmin, _v2f16_v2f16, )( temp, (half2)(0x1.fffffep-1f)), (half2)SPIRV_OCL_BUILTIN(copysign, _v2f16_v2f16, )((half2)0.0f, x), __convert_short2(SPIRV_BUILTIN(IsInf, _v2f16, )(x)));
    return SPIRV_OCL_BUILTIN(select, _v2f16_v2f16_v2i16, )( temp, SPIRV_OCL_BUILTIN(nan, _v2i16, )((short2)0), __convert_short2(SPIRV_BUILTIN(IsNan, _v2f16, )(x)) );
}

INLINE half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v3f16_p0v3f16, )( half3 x,
                                           __private half3* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _v3f16_v3f16_v3i16, )( SPIRV_OCL_BUILTIN(floor, _v3f16, )( x ), SPIRV_OCL_BUILTIN(nan, _v3i16, )( (short3)0 ), __convert_short3(SPIRV_BUILTIN(IsNan, _v3f16, )( x )) );
    half3 temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _v3f16_v3f16_v3i16, )( (half3)SPIRV_OCL_BUILTIN(fmin, _v3f16_v3f16, )( temp, (half3)(0x1.fffffep-1f)), (half3)SPIRV_OCL_BUILTIN(copysign, _v3f16_v3f16, )((half3)0.0f, x), __convert_short3(SPIRV_BUILTIN(IsInf, _v3f16, )(x)));
    return SPIRV_OCL_BUILTIN(select, _v3f16_v3f16_v3i16, )( temp, SPIRV_OCL_BUILTIN(nan, _v3i16, )((short3)0), __convert_short3(SPIRV_BUILTIN(IsNan, _v3f16, )(x)) );
}

INLINE half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v4f16_p0v4f16, )( half4 x,
                                           __private half4* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _v4f16_v4f16_v4i16, )( SPIRV_OCL_BUILTIN(floor, _v4f16, )( x ), SPIRV_OCL_BUILTIN(nan, _v4i16, )( (short4)0 ), __convert_short4(SPIRV_BUILTIN(IsNan, _v4f16, )( x )) );
    half4 temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _v4f16_v4f16_v4i16, )( (half4)SPIRV_OCL_BUILTIN(fmin, _v4f16_v4f16, )( temp, (half4)(0x1.fffffep-1f)), (half4)SPIRV_OCL_BUILTIN(copysign, _v4f16_v4f16, )((half4)0.0f, x), __convert_short4(SPIRV_BUILTIN(IsInf, _v4f16, )(x)));
    return SPIRV_OCL_BUILTIN(select, _v4f16_v4f16_v4i16, )( temp, SPIRV_OCL_BUILTIN(nan, _v4i16, )((short4)0), __convert_short4(SPIRV_BUILTIN(IsNan, _v4f16, )(x)) );
}

INLINE half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v8f16_p0v8f16, )( half8 x,
                                           __private half8* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _v8f16_v8f16_v8i16, )( SPIRV_OCL_BUILTIN(floor, _v8f16, )( x ), SPIRV_OCL_BUILTIN(nan, _v8i16, )( (short8)0 ), __convert_short8(SPIRV_BUILTIN(IsNan, _v8f16, )( x )) );
    half8 temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _v8f16_v8f16_v8i16, )( (half8)SPIRV_OCL_BUILTIN(fmin, _v8f16_v8f16, )( temp, (half8)(0x1.fffffep-1f)), (half8)SPIRV_OCL_BUILTIN(copysign, _v8f16_v8f16, )((half8)0.0f, x), __convert_short8(SPIRV_BUILTIN(IsInf, _v8f16, )(x)));
    return SPIRV_OCL_BUILTIN(select, _v8f16_v8f16_v8i16, )( temp, SPIRV_OCL_BUILTIN(nan, _v8i16, )((short8)0), __convert_short8(SPIRV_BUILTIN(IsNan, _v8f16, )(x)) );
}

INLINE half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v16f16_p0v16f16, )( half16 x,
                                              __private half16* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _v16f16_v16f16_v16i16, )( SPIRV_OCL_BUILTIN(floor, _v16f16, )( x ), SPIRV_OCL_BUILTIN(nan, _v16i16, )( (short16)0 ), __convert_short16(SPIRV_BUILTIN(IsNan, _v16f16, )( x )) );
    half16 temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _v16f16_v16f16_v16i16, )( (half16)SPIRV_OCL_BUILTIN(fmin, _v16f16_v16f16, )( temp, (half16)(0x1.fffffep-1f)), (half16)SPIRV_OCL_BUILTIN(copysign, _v16f16_v16f16, )((half16)0.0f, x), __convert_short16(SPIRV_BUILTIN(IsInf, _v16f16, )(x)));
    return SPIRV_OCL_BUILTIN(select, _v16f16_v16f16_v16i16, )( temp, SPIRV_OCL_BUILTIN(nan, _v16i16, )((short16)0), __convert_short16(SPIRV_BUILTIN(IsNan, _v16f16, )(x)) );
}

INLINE half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _f16_p3f16, )( half x,
                                      __local half* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _f16_f16_i16, )( SPIRV_OCL_BUILTIN(floor, _f16, )( x ), SPIRV_OCL_BUILTIN(nan, _i16, )( (short)0 ), (short)(SPIRV_BUILTIN(IsNan, _f16, )( x )) );
    half temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _f16_f16_i16, )( (half)SPIRV_OCL_BUILTIN(fmin, _f16_f16, )( temp, (half)(0x1.fffffep-1f)), (half)SPIRV_OCL_BUILTIN(copysign, _f16_f16, )((half)0.0f, x), (short)SPIRV_BUILTIN(IsInf, _f16, )(x));
    return SPIRV_OCL_BUILTIN(select, _f16_f16_i16, )( temp, SPIRV_OCL_BUILTIN(nan, _i16, )((short)0), (short)(SPIRV_BUILTIN(IsNan, _f16, )(x)) );
}

INLINE half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v2f16_p3v2f16, )( half2 x,
                                           __local half2* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _v2f16_v2f16_v2i16, )( SPIRV_OCL_BUILTIN(floor, _v2f16, )( x ), SPIRV_OCL_BUILTIN(nan, _v2i16, )( (short2)0 ), __convert_short2(SPIRV_BUILTIN(IsNan, _v2f16, )( x )) );
    half2 temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _v2f16_v2f16_v2i16, )( (half2)SPIRV_OCL_BUILTIN(fmin, _v2f16_v2f16, )( temp, (half2)(0x1.fffffep-1f)), (half2)SPIRV_OCL_BUILTIN(copysign, _v2f16_v2f16, )((half2)0.0f, x), __convert_short2(SPIRV_BUILTIN(IsInf, _v2f16, )(x)));
    return SPIRV_OCL_BUILTIN(select, _v2f16_v2f16_v2i16, )( temp, SPIRV_OCL_BUILTIN(nan, _v2i16, )((short2)0), __convert_short2(SPIRV_BUILTIN(IsNan, _v2f16, )(x)) );
}

INLINE half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v3f16_p3v3f16, )( half3 x,
                                           __local half3* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _v3f16_v3f16_v3i16, )( SPIRV_OCL_BUILTIN(floor, _v3f16, )( x ), SPIRV_OCL_BUILTIN(nan, _v3i16, )( (short3)0 ), __convert_short3(SPIRV_BUILTIN(IsNan, _v3f16, )( x )) );
    half3 temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _v3f16_v3f16_v3i16, )( (half3)SPIRV_OCL_BUILTIN(fmin, _v3f16_v3f16, )( temp, (half3)(0x1.fffffep-1f)), (half3)SPIRV_OCL_BUILTIN(copysign, _v3f16_v3f16, )((half3)0.0f, x), __convert_short3(SPIRV_BUILTIN(IsInf, _v3f16, )(x)));
    return SPIRV_OCL_BUILTIN(select, _v3f16_v3f16_v3i16, )( temp, SPIRV_OCL_BUILTIN(nan, _v3i16, )((short3)0), __convert_short3(SPIRV_BUILTIN(IsNan, _v3f16, )(x)) );
}

INLINE half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v4f16_p3v4f16, )( half4 x,
                                           __local half4* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _v4f16_v4f16_v4i16, )( SPIRV_OCL_BUILTIN(floor, _v4f16, )( x ), SPIRV_OCL_BUILTIN(nan, _v4i16, )( (short4)0 ), __convert_short4(SPIRV_BUILTIN(IsNan, _v4f16, )( x )) );
    half4 temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _v4f16_v4f16_v4i16, )( (half4)SPIRV_OCL_BUILTIN(fmin, _v4f16_v4f16, )( temp, (half4)(0x1.fffffep-1f)), (half4)SPIRV_OCL_BUILTIN(copysign, _v4f16_v4f16, )((half4)0.0f, x), __convert_short4(SPIRV_BUILTIN(IsInf, _v4f16, )(x)));
    return SPIRV_OCL_BUILTIN(select, _v4f16_v4f16_v4i16, )( temp, SPIRV_OCL_BUILTIN(nan, _v4i16, )((short4)0), __convert_short4(SPIRV_BUILTIN(IsNan, _v4f16, )(x)) );
}

INLINE half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v8f16_p3v8f16, )( half8 x,
                                           __local half8* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _v8f16_v8f16_v8i16, )( SPIRV_OCL_BUILTIN(floor, _v8f16, )( x ), SPIRV_OCL_BUILTIN(nan, _v8i16, )( (short8)0 ), __convert_short8(SPIRV_BUILTIN(IsNan, _v8f16, )( x )) );
    half8 temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _v8f16_v8f16_v8i16, )( (half8)SPIRV_OCL_BUILTIN(fmin, _v8f16_v8f16, )( temp, (half8)(0x1.fffffep-1f)), (half8)SPIRV_OCL_BUILTIN(copysign, _v8f16_v8f16, )((half8)0.0f, x), __convert_short8(SPIRV_BUILTIN(IsInf, _v8f16, )(x)));
    return SPIRV_OCL_BUILTIN(select, _v8f16_v8f16_v8i16, )( temp, SPIRV_OCL_BUILTIN(nan, _v8i16, )((short8)0), __convert_short8(SPIRV_BUILTIN(IsNan, _v8f16, )(x)) );
}

INLINE half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v16f16_p3v16f16, )( half16 x,
                                              __local half16* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _v16f16_v16f16_v16i16, )( SPIRV_OCL_BUILTIN(floor, _v16f16, )( x ), SPIRV_OCL_BUILTIN(nan, _v16i16, )( (short16)0 ), __convert_short16(SPIRV_BUILTIN(IsNan, _v16f16, )( x )) );
    half16 temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _v16f16_v16f16_v16i16, )( (half16)SPIRV_OCL_BUILTIN(fmin, _v16f16_v16f16, )( temp, (half16)(0x1.fffffep-1f)), (half16)SPIRV_OCL_BUILTIN(copysign, _v16f16_v16f16, )((half16)0.0f, x), __convert_short16(SPIRV_BUILTIN(IsInf, _v16f16, )(x)));
    return SPIRV_OCL_BUILTIN(select, _v16f16_v16f16_v16i16, )( temp, SPIRV_OCL_BUILTIN(nan, _v16i16, )((short16)0), __convert_short16(SPIRV_BUILTIN(IsNan, _v16f16, )(x)) );
}
#endif

#if defined(cl_khr_fp16) && (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

INLINE half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _f16_p4f16, )( half x,
                                      __generic half* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _f16_f16_i16, )( SPIRV_OCL_BUILTIN(floor, _f16, )( x ), SPIRV_OCL_BUILTIN(nan, _i16, )( (short)0 ), (short)(SPIRV_BUILTIN(IsNan, _f16, )( x )) );
    half temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _f16_f16_i16, )( (half)SPIRV_OCL_BUILTIN(fmin, _f16_f16, )( temp, (half)(0x1.fffffep-1f)), (half)SPIRV_OCL_BUILTIN(copysign, _f16_f16, )((half)0.0f, x), (short)SPIRV_BUILTIN(IsInf, _f16, )(x));
    return SPIRV_OCL_BUILTIN(select, _f16_f16_i16, )( temp, SPIRV_OCL_BUILTIN(nan, _i16, )((short)0), (short)(SPIRV_BUILTIN(IsNan, _f16, )(x)) );
}

INLINE half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v2f16_p4v2f16, )( half2 x,
                                           __generic half2* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _v2f16_v2f16_v2i16, )( SPIRV_OCL_BUILTIN(floor, _v2f16, )( x ), SPIRV_OCL_BUILTIN(nan, _v2i16, )( (short2)0 ), __convert_short2(SPIRV_BUILTIN(IsNan, _v2f16, )( x )) );
    half2 temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _v2f16_v2f16_v2i16, )( (half2)SPIRV_OCL_BUILTIN(fmin, _v2f16_v2f16, )( temp, (half2)(0x1.fffffep-1f)), (half2)SPIRV_OCL_BUILTIN(copysign, _v2f16_v2f16, )((half2)0.0f, x), __convert_short2(SPIRV_BUILTIN(IsInf, _v2f16, )(x)));
    return SPIRV_OCL_BUILTIN(select, _v2f16_v2f16_v2i16, )( temp, SPIRV_OCL_BUILTIN(nan, _v2i16, )((short2)0), __convert_short2(SPIRV_BUILTIN(IsNan, _v2f16, )(x)) );
}

INLINE half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v3f16_p4v3f16, )( half3 x,
                                           __generic half3* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _v3f16_v3f16_v3i16, )( SPIRV_OCL_BUILTIN(floor, _v3f16, )( x ), SPIRV_OCL_BUILTIN(nan, _v3i16, )( (short3)0 ), __convert_short3(SPIRV_BUILTIN(IsNan, _v3f16, )( x )) );
    half3 temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _v3f16_v3f16_v3i16, )( (half3)SPIRV_OCL_BUILTIN(fmin, _v3f16_v3f16, )( temp, (half3)(0x1.fffffep-1f)), (half3)SPIRV_OCL_BUILTIN(copysign, _v3f16_v3f16, )((half3)0.0f, x), __convert_short3(SPIRV_BUILTIN(IsInf, _v3f16, )(x)));
    return SPIRV_OCL_BUILTIN(select, _v3f16_v3f16_v3i16, )( temp, SPIRV_OCL_BUILTIN(nan, _v3i16, )((short3)0), __convert_short3(SPIRV_BUILTIN(IsNan, _v3f16, )(x)) );
}

INLINE half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v4f16_p4v4f16, )( half4 x,
                                           __generic half4* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _v4f16_v4f16_v4i16, )( SPIRV_OCL_BUILTIN(floor, _v4f16, )( x ), SPIRV_OCL_BUILTIN(nan, _v4i16, )( (short4)0 ), __convert_short4(SPIRV_BUILTIN(IsNan, _v4f16, )( x )) );
    half4 temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _v4f16_v4f16_v4i16, )( (half4)SPIRV_OCL_BUILTIN(fmin, _v4f16_v4f16, )( temp, (half4)(0x1.fffffep-1f)), (half4)SPIRV_OCL_BUILTIN(copysign, _v4f16_v4f16, )((half4)0.0f, x), __convert_short4(SPIRV_BUILTIN(IsInf, _v4f16, )(x)));
    return SPIRV_OCL_BUILTIN(select, _v4f16_v4f16_v4i16, )( temp, SPIRV_OCL_BUILTIN(nan, _v4i16, )((short4)0), __convert_short4(SPIRV_BUILTIN(IsNan, _v4f16, )(x)) );
}

INLINE half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v8f16_p4v8f16, )( half8 x,
                                           __generic half8* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _v8f16_v8f16_v8i16, )( SPIRV_OCL_BUILTIN(floor, _v8f16, )( x ), SPIRV_OCL_BUILTIN(nan, _v8i16, )( (short8)0 ), __convert_short8(SPIRV_BUILTIN(IsNan, _v8f16, )( x )) );
    half8 temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _v8f16_v8f16_v8i16, )( (half8)SPIRV_OCL_BUILTIN(fmin, _v8f16_v8f16, )( temp, (half8)(0x1.fffffep-1f)), (half8)SPIRV_OCL_BUILTIN(copysign, _v8f16_v8f16, )((half8)0.0f, x), __convert_short8(SPIRV_BUILTIN(IsInf, _v8f16, )(x)));
    return SPIRV_OCL_BUILTIN(select, _v8f16_v8f16_v8i16, )( temp, SPIRV_OCL_BUILTIN(nan, _v8i16, )((short8)0), __convert_short8(SPIRV_BUILTIN(IsNan, _v8f16, )(x)) );
}

INLINE half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v16f16_p4v16f16, )( half16 x,
                                              __generic half16* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _v16f16_v16f16_v16i16, )( SPIRV_OCL_BUILTIN(floor, _v16f16, )( x ), SPIRV_OCL_BUILTIN(nan, _v16i16, )( (short16)0 ), __convert_short16(SPIRV_BUILTIN(IsNan, _v16f16, )( x )) );
    half16 temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _v16f16_v16f16_v16i16, )( (half16)SPIRV_OCL_BUILTIN(fmin, _v16f16_v16f16, )( temp, (half16)(0x1.fffffep-1f)), (half16)SPIRV_OCL_BUILTIN(copysign, _v16f16_v16f16, )((half16)0.0f, x), __convert_short16(SPIRV_BUILTIN(IsInf, _v16f16, )(x)));
    return SPIRV_OCL_BUILTIN(select, _v16f16_v16f16_v16i16, )( temp, SPIRV_OCL_BUILTIN(nan, _v16i16, )((short16)0), __convert_short16(SPIRV_BUILTIN(IsNan, _v16f16, )(x)) );
}
#endif //if defined(cl_khr_fp16) && (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#if defined(cl_khr_fp64)

double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _f64_p1f64, )( double x,
                                        __global double* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _f64_f64_i64, )( SPIRV_OCL_BUILTIN(floor, _f64, )( x ), SPIRV_OCL_BUILTIN(nan, _i64, )( (long)0 ), (long)(SPIRV_BUILTIN(IsNan, _f64, )( x )) );
    double temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _f64_f64_i64, )( SPIRV_OCL_BUILTIN(fmin, _f64_f64, )( temp, 0x1.fffffffffffffp-1), SPIRV_OCL_BUILTIN(copysign, _f64_f64, )(0.0, x), (long)SPIRV_BUILTIN(IsInf, _f64, )(x));
    return SPIRV_OCL_BUILTIN(select, _f64_f64_i64, )( temp, SPIRV_OCL_BUILTIN(nan, _i64, )((long)0), (long)(SPIRV_BUILTIN(IsNan, _f64, )(x)) );
}

double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v2f64_p1v2f64, )( double2 x,
                                             __global double2* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _v2f64_v2f64_v2i64, )( SPIRV_OCL_BUILTIN(floor, _v2f64, )( x ), SPIRV_OCL_BUILTIN(nan, _v2i64, )( (long2)0 ), __convert_long2(SPIRV_BUILTIN(IsNan, _v2f64, )( x )) );
    double2 temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _v2f64_v2f64_v2i64, )( (double2)SPIRV_OCL_BUILTIN(fmin, _v2f64_v2f64, )( temp, (double2)(0x1.fffffffffffffp-1)), (double2)SPIRV_OCL_BUILTIN(copysign, _v2f64_v2f64, )((double2)0.0, x), __convert_long2(SPIRV_BUILTIN(IsInf, _v2f64, )(x)));
    return SPIRV_OCL_BUILTIN(select, _v2f64_v2f64_v2i64, )( temp, SPIRV_OCL_BUILTIN(nan, _v2i64, )((long2)0), __convert_long2(SPIRV_BUILTIN(IsNan, _v2f64, )(x)) );
}

double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v3f64_p1v3f64, )( double3 x,
                                             __global double3* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _v3f64_v3f64_v3i64, )( SPIRV_OCL_BUILTIN(floor, _v3f64, )( x ), SPIRV_OCL_BUILTIN(nan, _v3i64, )( (long3)0 ), __convert_long3(SPIRV_BUILTIN(IsNan, _v3f64, )( x )) );
    double3 temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _v3f64_v3f64_v3i64, )( (double3)SPIRV_OCL_BUILTIN(fmin, _v3f64_v3f64, )( temp, (double3)(0x1.fffffffffffffp-1)), (double3)SPIRV_OCL_BUILTIN(copysign, _v3f64_v3f64, )((double3)0.0, x), __convert_long3(SPIRV_BUILTIN(IsInf, _v3f64, )(x)));
    return SPIRV_OCL_BUILTIN(select, _v3f64_v3f64_v3i64, )( temp, SPIRV_OCL_BUILTIN(nan, _v3i64, )((long3)0), __convert_long3(SPIRV_BUILTIN(IsNan, _v3f64, )(x)) );
}

double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v4f64_p1v4f64, )( double4 x,
                                             __global double4* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _v4f64_v4f64_v4i64, )( SPIRV_OCL_BUILTIN(floor, _v4f64, )( x ), SPIRV_OCL_BUILTIN(nan, _v4i64, )( (long4)0 ), __convert_long4(SPIRV_BUILTIN(IsNan, _v4f64, )( x )) );
    double4 temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _v4f64_v4f64_v4i64, )( (double4)SPIRV_OCL_BUILTIN(fmin, _v4f64_v4f64, )( temp, (double4)(0x1.fffffffffffffp-1)), (double4)SPIRV_OCL_BUILTIN(copysign, _v4f64_v4f64, )((double4)0.0, x), __convert_long4(SPIRV_BUILTIN(IsInf, _v4f64, )(x)));
    return SPIRV_OCL_BUILTIN(select, _v4f64_v4f64_v4i64, )( temp, SPIRV_OCL_BUILTIN(nan, _v4i64, )((long4)0), __convert_long4(SPIRV_BUILTIN(IsNan, _v4f64, )(x)) );
}

double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v8f64_p1v8f64, )( double8 x,
                                             __global double8* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _v8f64_v8f64_v8i64, )( SPIRV_OCL_BUILTIN(floor, _v8f64, )( x ), SPIRV_OCL_BUILTIN(nan, _v8i64, )( (long8)0 ), __convert_long8(SPIRV_BUILTIN(IsNan, _v8f64, )( x )) );
    double8 temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _v8f64_v8f64_v8i64, )( (double8)SPIRV_OCL_BUILTIN(fmin, _v8f64_v8f64, )( temp, (double8)(0x1.fffffffffffffp-1)), (double8)SPIRV_OCL_BUILTIN(copysign, _v8f64_v8f64, )((double8)0.0, x), __convert_long8(SPIRV_BUILTIN(IsInf, _v8f64, )(x)));
    return SPIRV_OCL_BUILTIN(select, _v8f64_v8f64_v8i64, )( temp, SPIRV_OCL_BUILTIN(nan, _v8i64, )((long8)0), __convert_long8(SPIRV_BUILTIN(IsNan, _v8f64, )(x)) );
}

double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v16f64_p1v16f64, )( double16 x,
                                                __global double16* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _v16f64_v16f64_v16i64, )( SPIRV_OCL_BUILTIN(floor, _v16f64, )( x ), SPIRV_OCL_BUILTIN(nan, _v16i64, )( (long16)0 ), __convert_long16(SPIRV_BUILTIN(IsNan, _v16f64, )( x )) );
    double16 temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _v16f64_v16f64_v16i64, )( (double16)SPIRV_OCL_BUILTIN(fmin, _v16f64_v16f64, )( temp, (double16)(0x1.fffffffffffffp-1)), (double16)SPIRV_OCL_BUILTIN(copysign, _v16f64_v16f64, )((double16)0.0, x), __convert_long16(SPIRV_BUILTIN(IsInf, _v16f64, )(x)));
    return SPIRV_OCL_BUILTIN(select, _v16f64_v16f64_v16i64, )( temp, SPIRV_OCL_BUILTIN(nan, _v16i64, )((long16)0), __convert_long16(SPIRV_BUILTIN(IsNan, _v16f64, )(x)) );
}

double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _f64_p0f64, )( double x,
                                        __private double* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _f64_f64_i64, )( SPIRV_OCL_BUILTIN(floor, _f64, )( x ), SPIRV_OCL_BUILTIN(nan, _i64, )( (long)0 ), (long)(SPIRV_BUILTIN(IsNan, _f64, )( x )) );
    double temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _f64_f64_i64, )( (double)SPIRV_OCL_BUILTIN(fmin, _f64_f64, )( temp, (double)(0x1.fffffffffffffp-1)), (double)SPIRV_OCL_BUILTIN(copysign, _f64_f64, )((double)0.0, x), (long)SPIRV_BUILTIN(IsInf, _f64, )(x));
    return SPIRV_OCL_BUILTIN(select, _f64_f64_i64, )( temp, SPIRV_OCL_BUILTIN(nan, _i64, )((long)0), (long)(SPIRV_BUILTIN(IsNan, _f64, )(x)) );
}

double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v2f64_p0v2f64, )( double2 x,
                                             __private double2* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _v2f64_v2f64_v2i64, )( SPIRV_OCL_BUILTIN(floor, _v2f64, )( x ), SPIRV_OCL_BUILTIN(nan, _v2i64, )( (long2)0 ), __convert_long2(SPIRV_BUILTIN(IsNan, _v2f64, )( x )) );
    double2 temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _v2f64_v2f64_v2i64, )( (double2)SPIRV_OCL_BUILTIN(fmin, _v2f64_v2f64, )( temp, (double2)(0x1.fffffffffffffp-1)), (double2)SPIRV_OCL_BUILTIN(copysign, _v2f64_v2f64, )((double2)0.0, x), __convert_long2(SPIRV_BUILTIN(IsInf, _v2f64, )(x)));
    return SPIRV_OCL_BUILTIN(select, _v2f64_v2f64_v2i64, )( temp, SPIRV_OCL_BUILTIN(nan, _v2i64, )((long2)0), __convert_long2(SPIRV_BUILTIN(IsNan, _v2f64, )(x)) );
}

double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v3f64_p0v3f64, )( double3 x,
                                             __private double3* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _v3f64_v3f64_v3i64, )( SPIRV_OCL_BUILTIN(floor, _v3f64, )( x ), SPIRV_OCL_BUILTIN(nan, _v3i64, )( (long3)0 ), __convert_long3(SPIRV_BUILTIN(IsNan, _v3f64, )( x )) );
    double3 temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _v3f64_v3f64_v3i64, )( (double3)SPIRV_OCL_BUILTIN(fmin, _v3f64_v3f64, )( temp, (double3)(0x1.fffffffffffffp-1)), (double3)SPIRV_OCL_BUILTIN(copysign, _v3f64_v3f64, )((double3)0.0, x), __convert_long3(SPIRV_BUILTIN(IsInf, _v3f64, )(x)));
    return SPIRV_OCL_BUILTIN(select, _v3f64_v3f64_v3i64, )( temp, SPIRV_OCL_BUILTIN(nan, _v3i64, )((long3)0), __convert_long3(SPIRV_BUILTIN(IsNan, _v3f64, )(x)) );
}

double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v4f64_p0v4f64, )( double4 x,
                                             __private double4* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _v4f64_v4f64_v4i64, )( SPIRV_OCL_BUILTIN(floor, _v4f64, )( x ), SPIRV_OCL_BUILTIN(nan, _v4i64, )( (long4)0 ), __convert_long4(SPIRV_BUILTIN(IsNan, _v4f64, )( x )) );
    double4 temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _v4f64_v4f64_v4i64, )( (double4)SPIRV_OCL_BUILTIN(fmin, _v4f64_v4f64, )( temp, (double4)(0x1.fffffffffffffp-1)), (double4)SPIRV_OCL_BUILTIN(copysign, _v4f64_v4f64, )((double4)0.0, x), __convert_long4(SPIRV_BUILTIN(IsInf, _v4f64, )(x)));
    return SPIRV_OCL_BUILTIN(select, _v4f64_v4f64_v4i64, )( temp, SPIRV_OCL_BUILTIN(nan, _v4i64, )((long4)0), __convert_long4(SPIRV_BUILTIN(IsNan, _v4f64, )(x)) );
}

double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v8f64_p0v8f64, )( double8 x,
                                             __private double8* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _v8f64_v8f64_v8i64, )( SPIRV_OCL_BUILTIN(floor, _v8f64, )( x ), SPIRV_OCL_BUILTIN(nan, _v8i64, )( (long8)0 ), __convert_long8(SPIRV_BUILTIN(IsNan, _v8f64, )( x )) );
    double8 temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _v8f64_v8f64_v8i64, )( (double8)SPIRV_OCL_BUILTIN(fmin, _v8f64_v8f64, )( temp, (double8)(0x1.fffffffffffffp-1)), (double8)SPIRV_OCL_BUILTIN(copysign, _v8f64_v8f64, )((double8)0.0, x), __convert_long8(SPIRV_BUILTIN(IsInf, _v8f64, )(x)));
    return SPIRV_OCL_BUILTIN(select, _v8f64_v8f64_v8i64, )( temp, SPIRV_OCL_BUILTIN(nan, _v8i64, )((long8)0), __convert_long8(SPIRV_BUILTIN(IsNan, _v8f64, )(x)) );
}

double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v16f64_p0v16f64, )( double16 x,
                                                __private double16* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _v16f64_v16f64_v16i64, )( SPIRV_OCL_BUILTIN(floor, _v16f64, )( x ), SPIRV_OCL_BUILTIN(nan, _v16i64, )( (long16)0 ), __convert_long16(SPIRV_BUILTIN(IsNan, _v16f64, )( x )) );
    double16 temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _v16f64_v16f64_v16i64, )( (double16)SPIRV_OCL_BUILTIN(fmin, _v16f64_v16f64, )( temp, (double16)(0x1.fffffffffffffp-1)), (double16)SPIRV_OCL_BUILTIN(copysign, _v16f64_v16f64, )((double16)0.0, x), __convert_long16(SPIRV_BUILTIN(IsInf, _v16f64, )(x)));
    return SPIRV_OCL_BUILTIN(select, _v16f64_v16f64_v16i64, )( temp, SPIRV_OCL_BUILTIN(nan, _v16i64, )((long16)0), __convert_long16(SPIRV_BUILTIN(IsNan, _v16f64, )(x)) );
}

double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _f64_p3f64, )( double x,
                                        __local double* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _f64_f64_i64, )( SPIRV_OCL_BUILTIN(floor, _f64, )( x ), SPIRV_OCL_BUILTIN(nan, _i64, )( (long)0 ), (long)(SPIRV_BUILTIN(IsNan, _f64, )( x )) );
    double temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _f64_f64_i64, )( (double)SPIRV_OCL_BUILTIN(fmin, _f64_f64, )( temp, (double)(0x1.fffffffffffffp-1)), (double)SPIRV_OCL_BUILTIN(copysign, _f64_f64, )((double)0.0, x), (long)SPIRV_BUILTIN(IsInf, _f64, )(x));
    return SPIRV_OCL_BUILTIN(select, _f64_f64_i64, )( temp, SPIRV_OCL_BUILTIN(nan, _i64, )((long)0), (long)(SPIRV_BUILTIN(IsNan, _f64, )(x)) );
}

double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v2f64_p3v2f64, )( double2 x,
                                             __local double2* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _v2f64_v2f64_v2i64, )( SPIRV_OCL_BUILTIN(floor, _v2f64, )( x ), SPIRV_OCL_BUILTIN(nan, _v2i64, )( (long2)0 ), __convert_long2(SPIRV_BUILTIN(IsNan, _v2f64, )( x )) );
    double2 temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _v2f64_v2f64_v2i64, )( (double2)SPIRV_OCL_BUILTIN(fmin, _v2f64_v2f64, )( temp, (double2)(0x1.fffffffffffffp-1)), (double2)SPIRV_OCL_BUILTIN(copysign, _v2f64_v2f64, )((double2)0.0, x), __convert_long2(SPIRV_BUILTIN(IsInf, _v2f64, )(x)));
    return SPIRV_OCL_BUILTIN(select, _v2f64_v2f64_v2i64, )( temp, SPIRV_OCL_BUILTIN(nan, _v2i64, )((long2)0), __convert_long2(SPIRV_BUILTIN(IsNan, _v2f64, )(x)) );
}

double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v3f64_p3v3f64, )( double3 x,
                                             __local double3* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _v3f64_v3f64_v3i64, )( SPIRV_OCL_BUILTIN(floor, _v3f64, )( x ), SPIRV_OCL_BUILTIN(nan, _v3i64, )( (long3)0 ), __convert_long3(SPIRV_BUILTIN(IsNan, _v3f64, )( x )) );
    double3 temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _v3f64_v3f64_v3i64, )( (double3)SPIRV_OCL_BUILTIN(fmin, _v3f64_v3f64, )( temp, (double3)(0x1.fffffffffffffp-1)), (double3)SPIRV_OCL_BUILTIN(copysign, _v3f64_v3f64, )((double3)0.0, x), __convert_long3(SPIRV_BUILTIN(IsInf, _v3f64, )(x)));
    return SPIRV_OCL_BUILTIN(select, _v3f64_v3f64_v3i64, )( temp, SPIRV_OCL_BUILTIN(nan, _v3i64, )((long3)0), __convert_long3(SPIRV_BUILTIN(IsNan, _v3f64, )(x)) );
}

double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v4f64_p3v4f64, )( double4 x,
                                             __local double4* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _v4f64_v4f64_v4i64, )( SPIRV_OCL_BUILTIN(floor, _v4f64, )( x ), SPIRV_OCL_BUILTIN(nan, _v4i64, )( (long4)0 ), __convert_long4(SPIRV_BUILTIN(IsNan, _v4f64, )( x )) );
    double4 temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _v4f64_v4f64_v4i64, )( (double4)SPIRV_OCL_BUILTIN(fmin, _v4f64_v4f64, )( temp, (double4)(0x1.fffffffffffffp-1)), (double4)SPIRV_OCL_BUILTIN(copysign, _v4f64_v4f64, )((double4)0.0, x), __convert_long4(SPIRV_BUILTIN(IsInf, _v4f64, )(x)));
    return SPIRV_OCL_BUILTIN(select, _v4f64_v4f64_v4i64, )( temp, SPIRV_OCL_BUILTIN(nan, _v4i64, )((long4)0), __convert_long4(SPIRV_BUILTIN(IsNan, _v4f64, )(x)) );
}

double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v8f64_p3v8f64, )( double8 x,
                                             __local double8* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _v8f64_v8f64_v8i64, )( SPIRV_OCL_BUILTIN(floor, _v8f64, )( x ), SPIRV_OCL_BUILTIN(nan, _v8i64, )( (long8)0 ), __convert_long8(SPIRV_BUILTIN(IsNan, _v8f64, )( x )) );
    double8 temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _v8f64_v8f64_v8i64, )( (double8)SPIRV_OCL_BUILTIN(fmin, _v8f64_v8f64, )( temp, (double8)(0x1.fffffffffffffp-1)), (double8)SPIRV_OCL_BUILTIN(copysign, _v8f64_v8f64, )((double8)0.0, x), __convert_long8(SPIRV_BUILTIN(IsInf, _v8f64, )(x)));
    return SPIRV_OCL_BUILTIN(select, _v8f64_v8f64_v8i64, )( temp, SPIRV_OCL_BUILTIN(nan, _v8i64, )((long8)0), __convert_long8(SPIRV_BUILTIN(IsNan, _v8f64, )(x)) );
}

double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v16f64_p3v16f64, )( double16 x,
                                                __local double16* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _v16f64_v16f64_v16i64, )( SPIRV_OCL_BUILTIN(floor, _v16f64, )( x ), SPIRV_OCL_BUILTIN(nan, _v16i64, )( (long16)0 ), __convert_long16(SPIRV_BUILTIN(IsNan, _v16f64, )( x )) );
    double16 temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _v16f64_v16f64_v16i64, )( (double16)SPIRV_OCL_BUILTIN(fmin, _v16f64_v16f64, )( temp, (double16)(0x1.fffffffffffffp-1)), (double16)SPIRV_OCL_BUILTIN(copysign, _v16f64_v16f64, )((double16)0.0, x), __convert_long16(SPIRV_BUILTIN(IsInf, _v16f64, )(x)));
    return SPIRV_OCL_BUILTIN(select, _v16f64_v16f64_v16i64, )( temp, SPIRV_OCL_BUILTIN(nan, _v16i64, )((long16)0), __convert_long16(SPIRV_BUILTIN(IsNan, _v16f64, )(x)) );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _f64_p4f64, )( double x,
                                        __generic double* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _f64_f64_i64, )( SPIRV_OCL_BUILTIN(floor, _f64, )( x ), SPIRV_OCL_BUILTIN(nan, _i64, )( (long)0 ), (long)(SPIRV_BUILTIN(IsNan, _f64, )( x )) );
    double temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _f64_f64_i64, )( (double)SPIRV_OCL_BUILTIN(fmin, _f64_f64, )( temp, (double)(0x1.fffffffffffffp-1)), (double)SPIRV_OCL_BUILTIN(copysign, _f64_f64, )((double)0.0, x), (long)SPIRV_BUILTIN(IsInf, _f64, )(x));
    return SPIRV_OCL_BUILTIN(select, _f64_f64_i64, )( temp, SPIRV_OCL_BUILTIN(nan, _i64, )((long)0), (long)(SPIRV_BUILTIN(IsNan, _f64, )(x)) );
}

double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v2f64_p4v2f64, )( double2 x,
                                             __generic double2* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _v2f64_v2f64_v2i64, )( SPIRV_OCL_BUILTIN(floor, _v2f64, )( x ), SPIRV_OCL_BUILTIN(nan, _v2i64, )( (long2)0 ), __convert_long2(SPIRV_BUILTIN(IsNan, _v2f64, )( x )) );
    double2 temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _v2f64_v2f64_v2i64, )( (double2)SPIRV_OCL_BUILTIN(fmin, _v2f64_v2f64, )( temp, (double2)(0x1.fffffffffffffp-1)), (double2)SPIRV_OCL_BUILTIN(copysign, _v2f64_v2f64, )((double2)0.0, x), __convert_long2(SPIRV_BUILTIN(IsInf, _v2f64, )(x)));
    return SPIRV_OCL_BUILTIN(select, _v2f64_v2f64_v2i64, )( temp, SPIRV_OCL_BUILTIN(nan, _v2i64, )((long2)0), __convert_long2(SPIRV_BUILTIN(IsNan, _v2f64, )(x)) );
}

double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v3f64_p4v3f64, )( double3 x,
                                             __generic double3* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _v3f64_v3f64_v3i64, )( SPIRV_OCL_BUILTIN(floor, _v3f64, )( x ), SPIRV_OCL_BUILTIN(nan, _v3i64, )( (long3)0 ), __convert_long3(SPIRV_BUILTIN(IsNan, _v3f64, )( x )) );
    double3 temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _v3f64_v3f64_v3i64, )( (double3)SPIRV_OCL_BUILTIN(fmin, _v3f64_v3f64, )( temp, (double3)(0x1.fffffffffffffp-1)), (double3)SPIRV_OCL_BUILTIN(copysign, _v3f64_v3f64, )((double3)0.0, x), __convert_long3(SPIRV_BUILTIN(IsInf, _v3f64, )(x)));
    return SPIRV_OCL_BUILTIN(select, _v3f64_v3f64_v3i64, )( temp, SPIRV_OCL_BUILTIN(nan, _v3i64, )((long3)0), __convert_long3(SPIRV_BUILTIN(IsNan, _v3f64, )(x)) );
}

double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v4f64_p4v4f64, )( double4 x,
                                             __generic double4* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _v4f64_v4f64_v4i64, )( SPIRV_OCL_BUILTIN(floor, _v4f64, )( x ), SPIRV_OCL_BUILTIN(nan, _v4i64, )( (long4)0 ), __convert_long4(SPIRV_BUILTIN(IsNan, _v4f64, )( x )) );
    double4 temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _v4f64_v4f64_v4i64, )( (double4)SPIRV_OCL_BUILTIN(fmin, _v4f64_v4f64, )( temp, (double4)(0x1.fffffffffffffp-1)), (double4)SPIRV_OCL_BUILTIN(copysign, _v4f64_v4f64, )((double4)0.0, x), __convert_long4(SPIRV_BUILTIN(IsInf, _v4f64, )(x)));
    return SPIRV_OCL_BUILTIN(select, _v4f64_v4f64_v4i64, )( temp, SPIRV_OCL_BUILTIN(nan, _v4i64, )((long4)0), __convert_long4(SPIRV_BUILTIN(IsNan, _v4f64, )(x)) );
}

double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v8f64_p4v8f64, )( double8 x,
                                             __generic double8* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _v8f64_v8f64_v8i64, )( SPIRV_OCL_BUILTIN(floor, _v8f64, )( x ), SPIRV_OCL_BUILTIN(nan, _v8i64, )( (long8)0 ), __convert_long8(SPIRV_BUILTIN(IsNan, _v8f64, )( x )) );
    double8 temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _v8f64_v8f64_v8i64, )( (double8)SPIRV_OCL_BUILTIN(fmin, _v8f64_v8f64, )( temp, (double8)(0x1.fffffffffffffp-1)), (double8)SPIRV_OCL_BUILTIN(copysign, _v8f64_v8f64, )((double8)0.0, x), __convert_long8(SPIRV_BUILTIN(IsInf, _v8f64, )(x)));
    return SPIRV_OCL_BUILTIN(select, _v8f64_v8f64_v8i64, )( temp, SPIRV_OCL_BUILTIN(nan, _v8i64, )((long8)0), __convert_long8(SPIRV_BUILTIN(IsNan, _v8f64, )(x)) );
}

double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v16f64_p4v16f64, )( double16 x,
                                                __generic double16* iptr )
{
    *iptr = SPIRV_OCL_BUILTIN(select, _v16f64_v16f64_v16i64, )( SPIRV_OCL_BUILTIN(floor, _v16f64, )( x ), SPIRV_OCL_BUILTIN(nan, _v16i64, )( (long16)0 ), __convert_long16(SPIRV_BUILTIN(IsNan, _v16f64, )( x )) );
    double16 temp = x - *iptr;
    temp = SPIRV_OCL_BUILTIN(select, _v16f64_v16f64_v16i64, )( (double16)SPIRV_OCL_BUILTIN(fmin, _v16f64_v16f64, )( temp, (double16)(0x1.fffffffffffffp-1)), (double16)SPIRV_OCL_BUILTIN(copysign, _v16f64_v16f64, )((double16)0.0, x), __convert_long16(SPIRV_BUILTIN(IsInf, _v16f64, )(x)));
    return SPIRV_OCL_BUILTIN(select, _v16f64_v16f64_v16i64, )( temp, SPIRV_OCL_BUILTIN(nan, _v16i64, )((long16)0), __convert_long16(SPIRV_BUILTIN(IsNan, _v16f64, )(x)) );
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
#endif // defined(cl_khr_fp64)

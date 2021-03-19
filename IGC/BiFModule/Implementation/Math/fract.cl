/*========================== begin_copyright_notice ============================

Copyright (c) 2000-2021 Intel Corporation

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
#include "../../Headers/spirv.h"

INLINE float __builtin_spirv_OpenCL_fract_f32_p1f32( float x,
                                       __global float* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_f32_f32_i32( __builtin_spirv_OpenCL_floor_f32( x ), __builtin_spirv_OpenCL_nan_i32( (uint)0 ), (uint)(__intel_relaxed_isnan( x )) );
    float temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_f32_f32_i32( (float)__builtin_spirv_OpenCL_fmin_f32_f32( temp, (float)(0x1.fffffep-1f)), (float)__builtin_spirv_OpenCL_copysign_f32_f32((float)0.0f, x), (int)__intel_relaxed_isinf(x));
    return __builtin_spirv_OpenCL_select_f32_f32_i32( temp, __builtin_spirv_OpenCL_nan_i32((uint)0), (int)(__intel_relaxed_isnan(x)) );
}

INLINE float2 __builtin_spirv_OpenCL_fract_v2f32_p1v2f32( float2 x,
                                            __global float2* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v2f32_v2f32_v2i32( __builtin_spirv_OpenCL_floor_v2f32( x ), __builtin_spirv_OpenCL_nan_v2i32( (uint2)0 ), __convert_uint2(__builtin_spirv_OpIsNan_v2f32( x )) );
    float2 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v2f32_v2f32_v2i32( (float2)__builtin_spirv_OpenCL_fmin_v2f32_v2f32( temp, (float2)(0x1.fffffep-1f)), (float2)__builtin_spirv_OpenCL_copysign_v2f32_v2f32((float2)0.0f, x), __convert_uint2(__builtin_spirv_OpIsInf_v2f32(x)));
    return __builtin_spirv_OpenCL_select_v2f32_v2f32_v2i32( temp, __builtin_spirv_OpenCL_nan_v2i32((uint2)0), __convert_uint2(__builtin_spirv_OpIsNan_v2f32(x)) );
}

INLINE float3 __builtin_spirv_OpenCL_fract_v3f32_p1v3f32( float3 x,
                                            __global float3* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v3f32_v3f32_v3i32( __builtin_spirv_OpenCL_floor_v3f32( x ), __builtin_spirv_OpenCL_nan_v3i32( (uint3)0 ), __convert_uint3(__builtin_spirv_OpIsNan_v3f32( x )) );
    float3 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v3f32_v3f32_v3i32( (float3)__builtin_spirv_OpenCL_fmin_v3f32_v3f32( temp, (float3)(0x1.fffffep-1f)), (float3)__builtin_spirv_OpenCL_copysign_v3f32_v3f32((float3)0.0f, x), __convert_uint3(__builtin_spirv_OpIsInf_v3f32(x)));
    return __builtin_spirv_OpenCL_select_v3f32_v3f32_v3i32( temp, __builtin_spirv_OpenCL_nan_v3i32((uint3)0), __convert_uint3(__builtin_spirv_OpIsNan_v3f32(x)) );
}

INLINE float4 __builtin_spirv_OpenCL_fract_v4f32_p1v4f32( float4 x,
                                            __global float4* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v4f32_v4f32_v4i32( __builtin_spirv_OpenCL_floor_v4f32( x ), __builtin_spirv_OpenCL_nan_v4i32( (uint4)0 ), __convert_uint4(__builtin_spirv_OpIsNan_v4f32( x )) );
    float4 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v4f32_v4f32_v4i32( (float4)__builtin_spirv_OpenCL_fmin_v4f32_v4f32( temp, (float4)(0x1.fffffep-1f)), (float4)__builtin_spirv_OpenCL_copysign_v4f32_v4f32((float4)0.0f, x), __convert_uint4(__builtin_spirv_OpIsInf_v4f32(x)));
    return __builtin_spirv_OpenCL_select_v4f32_v4f32_v4i32( temp, __builtin_spirv_OpenCL_nan_v4i32((uint4)0), __convert_uint4(__builtin_spirv_OpIsNan_v4f32(x)) );
}

INLINE float8 __builtin_spirv_OpenCL_fract_v8f32_p1v8f32( float8 x,
                                            __global float8* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v8f32_v8f32_v8i32( __builtin_spirv_OpenCL_floor_v8f32( x ), __builtin_spirv_OpenCL_nan_v8i32( (uint8)0 ), __convert_uint8(__builtin_spirv_OpIsNan_v8f32( x )) );
    float8 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v8f32_v8f32_v8i32( (float8)__builtin_spirv_OpenCL_fmin_v8f32_v8f32( temp, (float8)(0x1.fffffep-1f)), (float8)__builtin_spirv_OpenCL_copysign_v8f32_v8f32((float8)0.0f, x), __convert_uint8(__builtin_spirv_OpIsInf_v8f32(x)));
    return __builtin_spirv_OpenCL_select_v8f32_v8f32_v8i32( temp, __builtin_spirv_OpenCL_nan_v8i32((uint8)0), __convert_uint8(__builtin_spirv_OpIsNan_v8f32(x)) );
}

INLINE float16 __builtin_spirv_OpenCL_fract_v16f32_p1v16f32( float16 x,
                                               __global float16* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v16f32_v16f32_v16i32( __builtin_spirv_OpenCL_floor_v16f32( x ), __builtin_spirv_OpenCL_nan_v16i32( (uint16)0 ), __convert_uint16(__builtin_spirv_OpIsNan_v16f32( x )) );
    float16 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v16f32_v16f32_v16i32( (float16)__builtin_spirv_OpenCL_fmin_v16f32_v16f32( temp, (float16)(0x1.fffffep-1f)), (float16)__builtin_spirv_OpenCL_copysign_v16f32_v16f32((float16)0.0f, x), __convert_uint16(__builtin_spirv_OpIsInf_v16f32(x)));
    return __builtin_spirv_OpenCL_select_v16f32_v16f32_v16i32( temp, __builtin_spirv_OpenCL_nan_v16i32((uint16)0), __convert_uint16(__builtin_spirv_OpIsNan_v16f32(x)) );
}

INLINE float __builtin_spirv_OpenCL_fract_f32_p0f32( float x,
                                       __private float* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_f32_f32_i32( __builtin_spirv_OpenCL_floor_f32( x ), __builtin_spirv_OpenCL_nan_i32( (uint)0 ), (int)(__intel_relaxed_isnan( x )) );
    float temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_f32_f32_i32( (float)__builtin_spirv_OpenCL_fmin_f32_f32( temp, (float)(0x1.fffffep-1f)), (float)__builtin_spirv_OpenCL_copysign_f32_f32((float)0.0f, x), (int)__intel_relaxed_isinf(x));
    return __builtin_spirv_OpenCL_select_f32_f32_i32( temp, __builtin_spirv_OpenCL_nan_i32((uint)0), (int)(__intel_relaxed_isnan(x)) );
}

INLINE float2 __builtin_spirv_OpenCL_fract_v2f32_p0v2f32( float2 x,
                                            __private float2* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v2f32_v2f32_v2i32( __builtin_spirv_OpenCL_floor_v2f32( x ), __builtin_spirv_OpenCL_nan_v2i32( (uint2)0 ), __convert_uint2(__builtin_spirv_OpIsNan_v2f32( x )) );
    float2 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v2f32_v2f32_v2i32( (float2)__builtin_spirv_OpenCL_fmin_v2f32_v2f32( temp, (float2)(0x1.fffffep-1f)), (float2)__builtin_spirv_OpenCL_copysign_v2f32_v2f32((float2)0.0f, x), __convert_uint2(__builtin_spirv_OpIsInf_v2f32(x)));
    return __builtin_spirv_OpenCL_select_v2f32_v2f32_v2i32( temp, __builtin_spirv_OpenCL_nan_v2i32((uint2)0), __convert_uint2(__builtin_spirv_OpIsNan_v2f32(x)) );
}

INLINE float3 __builtin_spirv_OpenCL_fract_v3f32_p0v3f32( float3 x,
                                            __private float3* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v3f32_v3f32_v3i32( __builtin_spirv_OpenCL_floor_v3f32( x ), __builtin_spirv_OpenCL_nan_v3i32( (uint3)0 ), __convert_uint3(__builtin_spirv_OpIsNan_v3f32( x )) );
    float3 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v3f32_v3f32_v3i32( (float3)__builtin_spirv_OpenCL_fmin_v3f32_v3f32( temp, (float3)(0x1.fffffep-1f)), (float3)__builtin_spirv_OpenCL_copysign_v3f32_v3f32((float3)0.0f, x), __convert_uint3(__builtin_spirv_OpIsInf_v3f32(x)));
    return __builtin_spirv_OpenCL_select_v3f32_v3f32_v3i32( temp, __builtin_spirv_OpenCL_nan_v3i32((uint3)0), __convert_uint3(__builtin_spirv_OpIsNan_v3f32(x)) );
}

INLINE float4 __builtin_spirv_OpenCL_fract_v4f32_p0v4f32( float4 x,
                                            __private float4* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v4f32_v4f32_v4i32( __builtin_spirv_OpenCL_floor_v4f32( x ), __builtin_spirv_OpenCL_nan_v4i32( (uint4)0 ), __convert_uint4(__builtin_spirv_OpIsNan_v4f32( x )) );
    float4 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v4f32_v4f32_v4i32( (float4)__builtin_spirv_OpenCL_fmin_v4f32_v4f32( temp, (float4)(0x1.fffffep-1f)), (float4)__builtin_spirv_OpenCL_copysign_v4f32_v4f32((float4)0.0f, x), __convert_uint4(__builtin_spirv_OpIsInf_v4f32(x)));
    return __builtin_spirv_OpenCL_select_v4f32_v4f32_v4i32( temp, __builtin_spirv_OpenCL_nan_v4i32((uint4)0), __convert_uint4(__builtin_spirv_OpIsNan_v4f32(x)) );
}

INLINE float8 __builtin_spirv_OpenCL_fract_v8f32_p0v8f32( float8 x,
                                            __private float8* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v8f32_v8f32_v8i32( __builtin_spirv_OpenCL_floor_v8f32( x ), __builtin_spirv_OpenCL_nan_v8i32( (uint8)0 ), __convert_uint8(__builtin_spirv_OpIsNan_v8f32( x )) );
    float8 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v8f32_v8f32_v8i32( (float8)__builtin_spirv_OpenCL_fmin_v8f32_v8f32( temp, (float8)(0x1.fffffep-1f)), (float8)__builtin_spirv_OpenCL_copysign_v8f32_v8f32((float8)0.0f, x), __convert_uint8(__builtin_spirv_OpIsInf_v8f32(x)));
    return __builtin_spirv_OpenCL_select_v8f32_v8f32_v8i32( temp, __builtin_spirv_OpenCL_nan_v8i32((uint8)0), __convert_uint8(__builtin_spirv_OpIsNan_v8f32(x)) );
}

INLINE float16 __builtin_spirv_OpenCL_fract_v16f32_p0v16f32( float16 x,
                                               __private float16* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v16f32_v16f32_v16i32( __builtin_spirv_OpenCL_floor_v16f32( x ), __builtin_spirv_OpenCL_nan_v16i32( (uint16)0 ), __convert_uint16(__builtin_spirv_OpIsNan_v16f32( x )) );
    float16 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v16f32_v16f32_v16i32( (float16)__builtin_spirv_OpenCL_fmin_v16f32_v16f32( temp, (float16)(0x1.fffffep-1f)), (float16)__builtin_spirv_OpenCL_copysign_v16f32_v16f32((float16)0.0f, x), __convert_uint16(__builtin_spirv_OpIsInf_v16f32(x)));
    return __builtin_spirv_OpenCL_select_v16f32_v16f32_v16i32( temp, __builtin_spirv_OpenCL_nan_v16i32((uint16)0), __convert_uint16(__builtin_spirv_OpIsNan_v16f32(x)) );
}

INLINE float __builtin_spirv_OpenCL_fract_f32_p3f32( float x,
                                       __local float* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_f32_f32_i32( __builtin_spirv_OpenCL_floor_f32( x ), __builtin_spirv_OpenCL_nan_i32( (uint)0 ), (int)(__intel_relaxed_isnan( x )) );
    float temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_f32_f32_i32( (float)__builtin_spirv_OpenCL_fmin_f32_f32( temp, (float)(0x1.fffffep-1f)), (float)__builtin_spirv_OpenCL_copysign_f32_f32((float)0.0f, x), (int)__intel_relaxed_isinf(x));
    return __builtin_spirv_OpenCL_select_f32_f32_i32( temp, __builtin_spirv_OpenCL_nan_i32((uint)0), (int)(__intel_relaxed_isnan(x)) );
}

INLINE float2 __builtin_spirv_OpenCL_fract_v2f32_p3v2f32( float2 x,
                                            __local float2* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v2f32_v2f32_v2i32( __builtin_spirv_OpenCL_floor_v2f32( x ), __builtin_spirv_OpenCL_nan_v2i32( (uint2)0 ), __convert_uint2(__builtin_spirv_OpIsNan_v2f32( x )) );
    float2 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v2f32_v2f32_v2i32( (float2)__builtin_spirv_OpenCL_fmin_v2f32_v2f32( temp, (float2)(0x1.fffffep-1f)), (float2)__builtin_spirv_OpenCL_copysign_v2f32_v2f32((float2)0.0f, x), __convert_uint2(__builtin_spirv_OpIsInf_v2f32(x)));
    return __builtin_spirv_OpenCL_select_v2f32_v2f32_v2i32( temp, __builtin_spirv_OpenCL_nan_v2i32((uint2)0), __convert_uint2(__builtin_spirv_OpIsNan_v2f32(x)) );
}

INLINE float3 __builtin_spirv_OpenCL_fract_v3f32_p3v3f32( float3 x,
                                            __local float3* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v3f32_v3f32_v3i32( __builtin_spirv_OpenCL_floor_v3f32( x ), __builtin_spirv_OpenCL_nan_v3i32( (uint3)0 ), __convert_uint3(__builtin_spirv_OpIsNan_v3f32( x )) );
    float3 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v3f32_v3f32_v3i32( (float3)__builtin_spirv_OpenCL_fmin_v3f32_v3f32( temp, (float3)(0x1.fffffep-1f)), (float3)__builtin_spirv_OpenCL_copysign_v3f32_v3f32((float3)0.0f, x), __convert_uint3(__builtin_spirv_OpIsInf_v3f32(x)));
    return __builtin_spirv_OpenCL_select_v3f32_v3f32_v3i32( temp, __builtin_spirv_OpenCL_nan_v3i32((uint3)0), __convert_uint3(__builtin_spirv_OpIsNan_v3f32(x)) );
}

INLINE float4 __builtin_spirv_OpenCL_fract_v4f32_p3v4f32( float4 x,
                                            __local float4* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v4f32_v4f32_v4i32( __builtin_spirv_OpenCL_floor_v4f32( x ), __builtin_spirv_OpenCL_nan_v4i32( (uint4)0 ), __convert_uint4(__builtin_spirv_OpIsNan_v4f32( x )) );
    float4 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v4f32_v4f32_v4i32( (float4)__builtin_spirv_OpenCL_fmin_v4f32_v4f32( temp, (float4)(0x1.fffffep-1f)), (float4)__builtin_spirv_OpenCL_copysign_v4f32_v4f32((float4)0.0f, x), __convert_uint4(__builtin_spirv_OpIsInf_v4f32(x)));
    return __builtin_spirv_OpenCL_select_v4f32_v4f32_v4i32( temp, __builtin_spirv_OpenCL_nan_v4i32((uint4)0), __convert_uint4(__builtin_spirv_OpIsNan_v4f32(x)) );
}

INLINE float8 __builtin_spirv_OpenCL_fract_v8f32_p3v8f32( float8 x,
                                            __local float8* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v8f32_v8f32_v8i32( __builtin_spirv_OpenCL_floor_v8f32( x ), __builtin_spirv_OpenCL_nan_v8i32( (uint8)0 ), __convert_uint8(__builtin_spirv_OpIsNan_v8f32( x )) );
    float8 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v8f32_v8f32_v8i32( (float8)__builtin_spirv_OpenCL_fmin_v8f32_v8f32( temp, (float8)(0x1.fffffep-1f)), (float8)__builtin_spirv_OpenCL_copysign_v8f32_v8f32((float8)0.0f, x), __convert_uint8(__builtin_spirv_OpIsInf_v8f32(x)));
    return __builtin_spirv_OpenCL_select_v8f32_v8f32_v8i32( temp, __builtin_spirv_OpenCL_nan_v8i32((uint8)0), __convert_uint8(__builtin_spirv_OpIsNan_v8f32(x)) );
}

INLINE float16 __builtin_spirv_OpenCL_fract_v16f32_p3v16f32( float16 x,
                                               __local float16* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v16f32_v16f32_v16i32( __builtin_spirv_OpenCL_floor_v16f32( x ), __builtin_spirv_OpenCL_nan_v16i32( (uint16)0 ), __convert_uint16(__builtin_spirv_OpIsNan_v16f32( x )) );
    float16 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v16f32_v16f32_v16i32( (float16)__builtin_spirv_OpenCL_fmin_v16f32_v16f32( temp, (float16)(0x1.fffffep-1f)), (float16)__builtin_spirv_OpenCL_copysign_v16f32_v16f32((float16)0.0f, x), __convert_uint16(__builtin_spirv_OpIsInf_v16f32(x)));
    return __builtin_spirv_OpenCL_select_v16f32_v16f32_v16i32( temp, __builtin_spirv_OpenCL_nan_v16i32((uint16)0), __convert_uint16(__builtin_spirv_OpIsNan_v16f32(x)) );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

INLINE float __builtin_spirv_OpenCL_fract_f32_p4f32( float x,
                                       __generic float* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_f32_f32_i32( __builtin_spirv_OpenCL_floor_f32( x ), __builtin_spirv_OpenCL_nan_i32( (uint)0 ), (int)(__intel_relaxed_isnan( x )) );
    float temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_f32_f32_i32( (float)__builtin_spirv_OpenCL_fmin_f32_f32( temp, (float)(0x1.fffffep-1f)), (float)__builtin_spirv_OpenCL_copysign_f32_f32((float)0.0f, x), (int)__intel_relaxed_isinf(x));
    return __builtin_spirv_OpenCL_select_f32_f32_i32( temp, __builtin_spirv_OpenCL_nan_i32((uint)0), (int)(__intel_relaxed_isnan(x)) );
}

INLINE float2 __builtin_spirv_OpenCL_fract_v2f32_p4v2f32( float2 x,
                                            __generic float2* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v2f32_v2f32_v2i32( __builtin_spirv_OpenCL_floor_v2f32( x ), __builtin_spirv_OpenCL_nan_v2i32( (uint2)0 ), __convert_uint2(__builtin_spirv_OpIsNan_v2f32( x )) );
    float2 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v2f32_v2f32_v2i32( (float2)__builtin_spirv_OpenCL_fmin_v2f32_v2f32( temp, (float2)(0x1.fffffep-1f)), (float2)__builtin_spirv_OpenCL_copysign_v2f32_v2f32((float2)0.0f, x), __convert_uint2(__builtin_spirv_OpIsInf_v2f32(x)));
    return __builtin_spirv_OpenCL_select_v2f32_v2f32_v2i32( temp, __builtin_spirv_OpenCL_nan_v2i32((uint2)0), __convert_uint2(__builtin_spirv_OpIsNan_v2f32(x)) );
}

INLINE float3 __builtin_spirv_OpenCL_fract_v3f32_p4v3f32( float3 x,
                                            __generic float3* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v3f32_v3f32_v3i32( __builtin_spirv_OpenCL_floor_v3f32( x ), __builtin_spirv_OpenCL_nan_v3i32( (uint3)0 ), __convert_uint3(__builtin_spirv_OpIsNan_v3f32( x )) );
    float3 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v3f32_v3f32_v3i32( (float3)__builtin_spirv_OpenCL_fmin_v3f32_v3f32( temp, (float3)(0x1.fffffep-1f)), (float3)__builtin_spirv_OpenCL_copysign_v3f32_v3f32((float3)0.0f, x), __convert_uint3(__builtin_spirv_OpIsInf_v3f32(x)));
    return __builtin_spirv_OpenCL_select_v3f32_v3f32_v3i32( temp, __builtin_spirv_OpenCL_nan_v3i32((uint3)0), __convert_uint3(__builtin_spirv_OpIsNan_v3f32(x)) );
}

INLINE float4 __builtin_spirv_OpenCL_fract_v4f32_p4v4f32( float4 x,
                                            __generic float4* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v4f32_v4f32_v4i32( __builtin_spirv_OpenCL_floor_v4f32( x ), __builtin_spirv_OpenCL_nan_v4i32( (uint4)0 ), __convert_uint4(__builtin_spirv_OpIsNan_v4f32( x )) );
    float4 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v4f32_v4f32_v4i32( (float4)__builtin_spirv_OpenCL_fmin_v4f32_v4f32( temp, (float4)(0x1.fffffep-1f)), (float4)__builtin_spirv_OpenCL_copysign_v4f32_v4f32((float4)0.0f, x), __convert_uint4(__builtin_spirv_OpIsInf_v4f32(x)));
    return __builtin_spirv_OpenCL_select_v4f32_v4f32_v4i32( temp, __builtin_spirv_OpenCL_nan_v4i32((uint4)0), __convert_uint4(__builtin_spirv_OpIsNan_v4f32(x)) );
}

INLINE float8 __builtin_spirv_OpenCL_fract_v8f32_p4v8f32( float8 x,
                                            __generic float8* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v8f32_v8f32_v8i32( __builtin_spirv_OpenCL_floor_v8f32( x ), __builtin_spirv_OpenCL_nan_v8i32( (uint8)0 ), __convert_uint8(__builtin_spirv_OpIsNan_v8f32( x )) );
    float8 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v8f32_v8f32_v8i32( (float8)__builtin_spirv_OpenCL_fmin_v8f32_v8f32( temp, (float8)(0x1.fffffep-1f)), (float8)__builtin_spirv_OpenCL_copysign_v8f32_v8f32((float8)0.0f, x), __convert_uint8(__builtin_spirv_OpIsInf_v8f32(x)));
    return __builtin_spirv_OpenCL_select_v8f32_v8f32_v8i32( temp, __builtin_spirv_OpenCL_nan_v8i32((uint8)0), __convert_uint8(__builtin_spirv_OpIsNan_v8f32(x)) );
}

INLINE float16 __builtin_spirv_OpenCL_fract_v16f32_p4v16f32( float16 x,
                                               __generic float16* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v16f32_v16f32_v16i32( __builtin_spirv_OpenCL_floor_v16f32( x ), __builtin_spirv_OpenCL_nan_v16i32( (uint16)0 ), __convert_uint16(__builtin_spirv_OpIsNan_v16f32( x )) );
    float16 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v16f32_v16f32_v16i32( (float16)__builtin_spirv_OpenCL_fmin_v16f32_v16f32( temp, (float16)(0x1.fffffep-1f)), (float16)__builtin_spirv_OpenCL_copysign_v16f32_v16f32((float16)0.0f, x), __convert_uint16(__builtin_spirv_OpIsInf_v16f32(x)));
    return __builtin_spirv_OpenCL_select_v16f32_v16f32_v16i32( temp, __builtin_spirv_OpenCL_nan_v16i32((uint16)0), __convert_uint16(__builtin_spirv_OpIsNan_v16f32(x)) );
}

#endif //#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)


#ifdef cl_khr_fp16
INLINE half __builtin_spirv_OpenCL_fract_f16_p1f16( half x,
                                      __global half* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_f16_f16_i16( __builtin_spirv_OpenCL_floor_f16( x ), __builtin_spirv_OpenCL_nan_i16( (ushort)0 ), (short)(__builtin_spirv_OpIsNan_f16( x )) );
    half temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_f16_f16_i16( (half)__builtin_spirv_OpenCL_fmin_f16_f16( temp, (half)(0x1.fffffep-1f)), (half)__builtin_spirv_OpenCL_copysign_f16_f16((half)0.0f, x), (short)__builtin_spirv_OpIsInf_f16(x));
    return __builtin_spirv_OpenCL_select_f16_f16_i16( temp, __builtin_spirv_OpenCL_nan_i16((ushort)0), (short)(__builtin_spirv_OpIsNan_f16(x)) );
}

INLINE half2 __builtin_spirv_OpenCL_fract_v2f16_p1v2f16( half2 x,
                                           __global half2* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v2f16_v2f16_v2i16( __builtin_spirv_OpenCL_floor_v2f16( x ), __builtin_spirv_OpenCL_nan_v2i16( (ushort2)0 ), __convert_ushort2(__builtin_spirv_OpIsNan_v2f16( x )) );
    half2 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v2f16_v2f16_v2i16( (half2)__builtin_spirv_OpenCL_fmin_v2f16_v2f16( temp, (half2)(0x1.fffffep-1f)), (half2)__builtin_spirv_OpenCL_copysign_v2f16_v2f16((half2)0.0f, x), __convert_ushort2(__builtin_spirv_OpIsInf_v2f16(x)));
    return __builtin_spirv_OpenCL_select_v2f16_v2f16_v2i16( temp, __builtin_spirv_OpenCL_nan_v2i16((ushort2)0), __convert_ushort2(__builtin_spirv_OpIsNan_v2f16(x)) );
}

INLINE half3 __builtin_spirv_OpenCL_fract_v3f16_p1v3f16( half3 x,
                                           __global half3* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v3f16_v3f16_v3i16( __builtin_spirv_OpenCL_floor_v3f16( x ), __builtin_spirv_OpenCL_nan_v3i16( (ushort3)0 ), __convert_ushort3(__builtin_spirv_OpIsNan_v3f16( x )) );
    half3 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v3f16_v3f16_v3i16( (half3)__builtin_spirv_OpenCL_fmin_v3f16_v3f16( temp, (half3)(0x1.fffffep-1f)), (half3)__builtin_spirv_OpenCL_copysign_v3f16_v3f16((half3)0.0f, x), __convert_ushort3(__builtin_spirv_OpIsInf_v3f16(x)));
    return __builtin_spirv_OpenCL_select_v3f16_v3f16_v3i16( temp, __builtin_spirv_OpenCL_nan_v3i16((ushort3)0), __convert_ushort3(__builtin_spirv_OpIsNan_v3f16(x)) );
}

INLINE half4 __builtin_spirv_OpenCL_fract_v4f16_p1v4f16( half4 x,
                                           __global half4* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v4f16_v4f16_v4i16( __builtin_spirv_OpenCL_floor_v4f16( x ), __builtin_spirv_OpenCL_nan_v4i16( (ushort4)0 ), __convert_ushort4(__builtin_spirv_OpIsNan_v4f16( x )) );
    half4 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v4f16_v4f16_v4i16( (half4)__builtin_spirv_OpenCL_fmin_v4f16_v4f16( temp, (half4)(0x1.fffffep-1f)), (half4)__builtin_spirv_OpenCL_copysign_v4f16_v4f16((half4)0.0f, x), __convert_ushort4(__builtin_spirv_OpIsInf_v4f16(x)));
    return __builtin_spirv_OpenCL_select_v4f16_v4f16_v4i16( temp, __builtin_spirv_OpenCL_nan_v4i16((ushort4)0), __convert_ushort4(__builtin_spirv_OpIsNan_v4f16(x)) );
}

INLINE half8 __builtin_spirv_OpenCL_fract_v8f16_p1v8f16( half8 x,
                                           __global half8* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v8f16_v8f16_v8i16( __builtin_spirv_OpenCL_floor_v8f16( x ), __builtin_spirv_OpenCL_nan_v8i16( (ushort8)0 ), __convert_ushort8(__builtin_spirv_OpIsNan_v8f16( x )) );
    half8 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v8f16_v8f16_v8i16( (half8)__builtin_spirv_OpenCL_fmin_v8f16_v8f16( temp, (half8)(0x1.fffffep-1f)), (half8)__builtin_spirv_OpenCL_copysign_v8f16_v8f16((half8)0.0f, x), __convert_ushort8(__builtin_spirv_OpIsInf_v8f16(x)));
    return __builtin_spirv_OpenCL_select_v8f16_v8f16_v8i16( temp, __builtin_spirv_OpenCL_nan_v8i16((ushort8)0), __convert_ushort8(__builtin_spirv_OpIsNan_v8f16(x)) );
}

INLINE half16 __builtin_spirv_OpenCL_fract_v16f16_p1v16f16( half16 x,
                                              __global half16* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v16f16_v16f16_v16i16( __builtin_spirv_OpenCL_floor_v16f16( x ), __builtin_spirv_OpenCL_nan_v16i16( (ushort16)0 ), __convert_ushort16(__builtin_spirv_OpIsNan_v16f16( x )) );
    half16 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v16f16_v16f16_v16i16( (half16)__builtin_spirv_OpenCL_fmin_v16f16_v16f16( temp, (half16)(0x1.fffffep-1f)), (half16)__builtin_spirv_OpenCL_copysign_v16f16_v16f16((half16)0.0f, x), __convert_ushort16(__builtin_spirv_OpIsInf_v16f16(x)));
    return __builtin_spirv_OpenCL_select_v16f16_v16f16_v16i16( temp, __builtin_spirv_OpenCL_nan_v16i16((ushort16)0), __convert_ushort16(__builtin_spirv_OpIsNan_v16f16(x)) );
}

INLINE half __builtin_spirv_OpenCL_fract_f16_p0f16( half x,
                                      __private half* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_f16_f16_i16( __builtin_spirv_OpenCL_floor_f16( x ), __builtin_spirv_OpenCL_nan_i16( (ushort)0 ), (short)(__builtin_spirv_OpIsNan_f16( x )) );
    half temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_f16_f16_i16( (half)__builtin_spirv_OpenCL_fmin_f16_f16( temp, (half)(0x1.fffffep-1f)), (half)__builtin_spirv_OpenCL_copysign_f16_f16((half)0.0f, x), (short)__builtin_spirv_OpIsInf_f16(x));
    return __builtin_spirv_OpenCL_select_f16_f16_i16( temp, __builtin_spirv_OpenCL_nan_i16((ushort)0), (short)(__builtin_spirv_OpIsNan_f16(x)) );
}

INLINE half2 __builtin_spirv_OpenCL_fract_v2f16_p0v2f16( half2 x,
                                           __private half2* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v2f16_v2f16_v2i16( __builtin_spirv_OpenCL_floor_v2f16( x ), __builtin_spirv_OpenCL_nan_v2i16( (ushort2)0 ), __convert_ushort2(__builtin_spirv_OpIsNan_v2f16( x )) );
    half2 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v2f16_v2f16_v2i16( (half2)__builtin_spirv_OpenCL_fmin_v2f16_v2f16( temp, (half2)(0x1.fffffep-1f)), (half2)__builtin_spirv_OpenCL_copysign_v2f16_v2f16((half2)0.0f, x), __convert_ushort2(__builtin_spirv_OpIsInf_v2f16(x)));
    return __builtin_spirv_OpenCL_select_v2f16_v2f16_v2i16( temp, __builtin_spirv_OpenCL_nan_v2i16((ushort2)0), __convert_ushort2(__builtin_spirv_OpIsNan_v2f16(x)) );
}

INLINE half3 __builtin_spirv_OpenCL_fract_v3f16_p0v3f16( half3 x,
                                           __private half3* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v3f16_v3f16_v3i16( __builtin_spirv_OpenCL_floor_v3f16( x ), __builtin_spirv_OpenCL_nan_v3i16( (ushort3)0 ), __convert_ushort3(__builtin_spirv_OpIsNan_v3f16( x )) );
    half3 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v3f16_v3f16_v3i16( (half3)__builtin_spirv_OpenCL_fmin_v3f16_v3f16( temp, (half3)(0x1.fffffep-1f)), (half3)__builtin_spirv_OpenCL_copysign_v3f16_v3f16((half3)0.0f, x), __convert_ushort3(__builtin_spirv_OpIsInf_v3f16(x)));
    return __builtin_spirv_OpenCL_select_v3f16_v3f16_v3i16( temp, __builtin_spirv_OpenCL_nan_v3i16((ushort3)0), __convert_ushort3(__builtin_spirv_OpIsNan_v3f16(x)) );
}

INLINE half4 __builtin_spirv_OpenCL_fract_v4f16_p0v4f16( half4 x,
                                           __private half4* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v4f16_v4f16_v4i16( __builtin_spirv_OpenCL_floor_v4f16( x ), __builtin_spirv_OpenCL_nan_v4i16( (ushort4)0 ), __convert_ushort4(__builtin_spirv_OpIsNan_v4f16( x )) );
    half4 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v4f16_v4f16_v4i16( (half4)__builtin_spirv_OpenCL_fmin_v4f16_v4f16( temp, (half4)(0x1.fffffep-1f)), (half4)__builtin_spirv_OpenCL_copysign_v4f16_v4f16((half4)0.0f, x), __convert_ushort4(__builtin_spirv_OpIsInf_v4f16(x)));
    return __builtin_spirv_OpenCL_select_v4f16_v4f16_v4i16( temp, __builtin_spirv_OpenCL_nan_v4i16((ushort4)0), __convert_ushort4(__builtin_spirv_OpIsNan_v4f16(x)) );
}

INLINE half8 __builtin_spirv_OpenCL_fract_v8f16_p0v8f16( half8 x,
                                           __private half8* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v8f16_v8f16_v8i16( __builtin_spirv_OpenCL_floor_v8f16( x ), __builtin_spirv_OpenCL_nan_v8i16( (ushort8)0 ), __convert_ushort8(__builtin_spirv_OpIsNan_v8f16( x )) );
    half8 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v8f16_v8f16_v8i16( (half8)__builtin_spirv_OpenCL_fmin_v8f16_v8f16( temp, (half8)(0x1.fffffep-1f)), (half8)__builtin_spirv_OpenCL_copysign_v8f16_v8f16((half8)0.0f, x), __convert_ushort8(__builtin_spirv_OpIsInf_v8f16(x)));
    return __builtin_spirv_OpenCL_select_v8f16_v8f16_v8i16( temp, __builtin_spirv_OpenCL_nan_v8i16((ushort8)0), __convert_ushort8(__builtin_spirv_OpIsNan_v8f16(x)) );
}

INLINE half16 __builtin_spirv_OpenCL_fract_v16f16_p0v16f16( half16 x,
                                              __private half16* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v16f16_v16f16_v16i16( __builtin_spirv_OpenCL_floor_v16f16( x ), __builtin_spirv_OpenCL_nan_v16i16( (ushort16)0 ), __convert_ushort16(__builtin_spirv_OpIsNan_v16f16( x )) );
    half16 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v16f16_v16f16_v16i16( (half16)__builtin_spirv_OpenCL_fmin_v16f16_v16f16( temp, (half16)(0x1.fffffep-1f)), (half16)__builtin_spirv_OpenCL_copysign_v16f16_v16f16((half16)0.0f, x), __convert_ushort16(__builtin_spirv_OpIsInf_v16f16(x)));
    return __builtin_spirv_OpenCL_select_v16f16_v16f16_v16i16( temp, __builtin_spirv_OpenCL_nan_v16i16((ushort16)0), __convert_ushort16(__builtin_spirv_OpIsNan_v16f16(x)) );
}

INLINE half __builtin_spirv_OpenCL_fract_f16_p3f16( half x,
                                      __local half* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_f16_f16_i16( __builtin_spirv_OpenCL_floor_f16( x ), __builtin_spirv_OpenCL_nan_i16( (ushort)0 ), (short)(__builtin_spirv_OpIsNan_f16( x )) );
    half temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_f16_f16_i16( (half)__builtin_spirv_OpenCL_fmin_f16_f16( temp, (half)(0x1.fffffep-1f)), (half)__builtin_spirv_OpenCL_copysign_f16_f16((half)0.0f, x), (short)__builtin_spirv_OpIsInf_f16(x));
    return __builtin_spirv_OpenCL_select_f16_f16_i16( temp, __builtin_spirv_OpenCL_nan_i16((ushort)0), (short)(__builtin_spirv_OpIsNan_f16(x)) );
}

INLINE half2 __builtin_spirv_OpenCL_fract_v2f16_p3v2f16( half2 x,
                                           __local half2* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v2f16_v2f16_v2i16( __builtin_spirv_OpenCL_floor_v2f16( x ), __builtin_spirv_OpenCL_nan_v2i16( (ushort2)0 ), __convert_ushort2(__builtin_spirv_OpIsNan_v2f16( x )) );
    half2 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v2f16_v2f16_v2i16( (half2)__builtin_spirv_OpenCL_fmin_v2f16_v2f16( temp, (half2)(0x1.fffffep-1f)), (half2)__builtin_spirv_OpenCL_copysign_v2f16_v2f16((half2)0.0f, x), __convert_ushort2(__builtin_spirv_OpIsInf_v2f16(x)));
    return __builtin_spirv_OpenCL_select_v2f16_v2f16_v2i16( temp, __builtin_spirv_OpenCL_nan_v2i16((ushort2)0), __convert_ushort2(__builtin_spirv_OpIsNan_v2f16(x)) );
}

INLINE half3 __builtin_spirv_OpenCL_fract_v3f16_p3v3f16( half3 x,
                                           __local half3* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v3f16_v3f16_v3i16( __builtin_spirv_OpenCL_floor_v3f16( x ), __builtin_spirv_OpenCL_nan_v3i16( (ushort3)0 ), __convert_ushort3(__builtin_spirv_OpIsNan_v3f16( x )) );
    half3 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v3f16_v3f16_v3i16( (half3)__builtin_spirv_OpenCL_fmin_v3f16_v3f16( temp, (half3)(0x1.fffffep-1f)), (half3)__builtin_spirv_OpenCL_copysign_v3f16_v3f16((half3)0.0f, x), __convert_ushort3(__builtin_spirv_OpIsInf_v3f16(x)));
    return __builtin_spirv_OpenCL_select_v3f16_v3f16_v3i16( temp, __builtin_spirv_OpenCL_nan_v3i16((ushort3)0), __convert_ushort3(__builtin_spirv_OpIsNan_v3f16(x)) );
}

INLINE half4 __builtin_spirv_OpenCL_fract_v4f16_p3v4f16( half4 x,
                                           __local half4* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v4f16_v4f16_v4i16( __builtin_spirv_OpenCL_floor_v4f16( x ), __builtin_spirv_OpenCL_nan_v4i16( (ushort4)0 ), __convert_ushort4(__builtin_spirv_OpIsNan_v4f16( x )) );
    half4 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v4f16_v4f16_v4i16( (half4)__builtin_spirv_OpenCL_fmin_v4f16_v4f16( temp, (half4)(0x1.fffffep-1f)), (half4)__builtin_spirv_OpenCL_copysign_v4f16_v4f16((half4)0.0f, x), __convert_ushort4(__builtin_spirv_OpIsInf_v4f16(x)));
    return __builtin_spirv_OpenCL_select_v4f16_v4f16_v4i16( temp, __builtin_spirv_OpenCL_nan_v4i16((ushort4)0), __convert_ushort4(__builtin_spirv_OpIsNan_v4f16(x)) );
}

INLINE half8 __builtin_spirv_OpenCL_fract_v8f16_p3v8f16( half8 x,
                                           __local half8* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v8f16_v8f16_v8i16( __builtin_spirv_OpenCL_floor_v8f16( x ), __builtin_spirv_OpenCL_nan_v8i16( (ushort8)0 ), __convert_ushort8(__builtin_spirv_OpIsNan_v8f16( x )) );
    half8 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v8f16_v8f16_v8i16( (half8)__builtin_spirv_OpenCL_fmin_v8f16_v8f16( temp, (half8)(0x1.fffffep-1f)), (half8)__builtin_spirv_OpenCL_copysign_v8f16_v8f16((half8)0.0f, x), __convert_ushort8(__builtin_spirv_OpIsInf_v8f16(x)));
    return __builtin_spirv_OpenCL_select_v8f16_v8f16_v8i16( temp, __builtin_spirv_OpenCL_nan_v8i16((ushort8)0), __convert_ushort8(__builtin_spirv_OpIsNan_v8f16(x)) );
}

INLINE half16 __builtin_spirv_OpenCL_fract_v16f16_p3v16f16( half16 x,
                                              __local half16* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v16f16_v16f16_v16i16( __builtin_spirv_OpenCL_floor_v16f16( x ), __builtin_spirv_OpenCL_nan_v16i16( (ushort16)0 ), __convert_ushort16(__builtin_spirv_OpIsNan_v16f16( x )) );
    half16 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v16f16_v16f16_v16i16( (half16)__builtin_spirv_OpenCL_fmin_v16f16_v16f16( temp, (half16)(0x1.fffffep-1f)), (half16)__builtin_spirv_OpenCL_copysign_v16f16_v16f16((half16)0.0f, x), __convert_ushort16(__builtin_spirv_OpIsInf_v16f16(x)));
    return __builtin_spirv_OpenCL_select_v16f16_v16f16_v16i16( temp, __builtin_spirv_OpenCL_nan_v16i16((ushort16)0), __convert_ushort16(__builtin_spirv_OpIsNan_v16f16(x)) );
}
#endif

#if defined(cl_khr_fp16) && (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

INLINE half __builtin_spirv_OpenCL_fract_f16_p4f16( half x,
                                      __generic half* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_f16_f16_i16( __builtin_spirv_OpenCL_floor_f16( x ), __builtin_spirv_OpenCL_nan_i16( (ushort)0 ), (short)(__builtin_spirv_OpIsNan_f16( x )) );
    half temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_f16_f16_i16( (half)__builtin_spirv_OpenCL_fmin_f16_f16( temp, (half)(0x1.fffffep-1f)), (half)__builtin_spirv_OpenCL_copysign_f16_f16((half)0.0f, x), (short)__builtin_spirv_OpIsInf_f16(x));
    return __builtin_spirv_OpenCL_select_f16_f16_i16( temp, __builtin_spirv_OpenCL_nan_i16((ushort)0), (short)(__builtin_spirv_OpIsNan_f16(x)) );
}

INLINE half2 __builtin_spirv_OpenCL_fract_v2f16_p4v2f16( half2 x,
                                           __generic half2* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v2f16_v2f16_v2i16( __builtin_spirv_OpenCL_floor_v2f16( x ), __builtin_spirv_OpenCL_nan_v2i16( (ushort2)0 ), __convert_ushort2(__builtin_spirv_OpIsNan_v2f16( x )) );
    half2 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v2f16_v2f16_v2i16( (half2)__builtin_spirv_OpenCL_fmin_v2f16_v2f16( temp, (half2)(0x1.fffffep-1f)), (half2)__builtin_spirv_OpenCL_copysign_v2f16_v2f16((half2)0.0f, x), __convert_ushort2(__builtin_spirv_OpIsInf_v2f16(x)));
    return __builtin_spirv_OpenCL_select_v2f16_v2f16_v2i16( temp, __builtin_spirv_OpenCL_nan_v2i16((ushort2)0), __convert_ushort2(__builtin_spirv_OpIsNan_v2f16(x)) );
}

INLINE half3 __builtin_spirv_OpenCL_fract_v3f16_p4v3f16( half3 x,
                                           __generic half3* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v3f16_v3f16_v3i16( __builtin_spirv_OpenCL_floor_v3f16( x ), __builtin_spirv_OpenCL_nan_v3i16( (ushort3)0 ), __convert_ushort3(__builtin_spirv_OpIsNan_v3f16( x )) );
    half3 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v3f16_v3f16_v3i16( (half3)__builtin_spirv_OpenCL_fmin_v3f16_v3f16( temp, (half3)(0x1.fffffep-1f)), (half3)__builtin_spirv_OpenCL_copysign_v3f16_v3f16((half3)0.0f, x), __convert_ushort3(__builtin_spirv_OpIsInf_v3f16(x)));
    return __builtin_spirv_OpenCL_select_v3f16_v3f16_v3i16( temp, __builtin_spirv_OpenCL_nan_v3i16((ushort3)0), __convert_ushort3(__builtin_spirv_OpIsNan_v3f16(x)) );
}

INLINE half4 __builtin_spirv_OpenCL_fract_v4f16_p4v4f16( half4 x,
                                           __generic half4* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v4f16_v4f16_v4i16( __builtin_spirv_OpenCL_floor_v4f16( x ), __builtin_spirv_OpenCL_nan_v4i16( (ushort4)0 ), __convert_ushort4(__builtin_spirv_OpIsNan_v4f16( x )) );
    half4 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v4f16_v4f16_v4i16( (half4)__builtin_spirv_OpenCL_fmin_v4f16_v4f16( temp, (half4)(0x1.fffffep-1f)), (half4)__builtin_spirv_OpenCL_copysign_v4f16_v4f16((half4)0.0f, x), __convert_ushort4(__builtin_spirv_OpIsInf_v4f16(x)));
    return __builtin_spirv_OpenCL_select_v4f16_v4f16_v4i16( temp, __builtin_spirv_OpenCL_nan_v4i16((ushort4)0), __convert_ushort4(__builtin_spirv_OpIsNan_v4f16(x)) );
}

INLINE half8 __builtin_spirv_OpenCL_fract_v8f16_p4v8f16( half8 x,
                                           __generic half8* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v8f16_v8f16_v8i16( __builtin_spirv_OpenCL_floor_v8f16( x ), __builtin_spirv_OpenCL_nan_v8i16( (ushort8)0 ), __convert_ushort8(__builtin_spirv_OpIsNan_v8f16( x )) );
    half8 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v8f16_v8f16_v8i16( (half8)__builtin_spirv_OpenCL_fmin_v8f16_v8f16( temp, (half8)(0x1.fffffep-1f)), (half8)__builtin_spirv_OpenCL_copysign_v8f16_v8f16((half8)0.0f, x), __convert_ushort8(__builtin_spirv_OpIsInf_v8f16(x)));
    return __builtin_spirv_OpenCL_select_v8f16_v8f16_v8i16( temp, __builtin_spirv_OpenCL_nan_v8i16((ushort8)0), __convert_ushort8(__builtin_spirv_OpIsNan_v8f16(x)) );
}

INLINE half16 __builtin_spirv_OpenCL_fract_v16f16_p4v16f16( half16 x,
                                              __generic half16* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v16f16_v16f16_v16i16( __builtin_spirv_OpenCL_floor_v16f16( x ), __builtin_spirv_OpenCL_nan_v16i16( (ushort16)0 ), __convert_ushort16(__builtin_spirv_OpIsNan_v16f16( x )) );
    half16 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v16f16_v16f16_v16i16( (half16)__builtin_spirv_OpenCL_fmin_v16f16_v16f16( temp, (half16)(0x1.fffffep-1f)), (half16)__builtin_spirv_OpenCL_copysign_v16f16_v16f16((half16)0.0f, x), __convert_ushort16(__builtin_spirv_OpIsInf_v16f16(x)));
    return __builtin_spirv_OpenCL_select_v16f16_v16f16_v16i16( temp, __builtin_spirv_OpenCL_nan_v16i16((ushort16)0), __convert_ushort16(__builtin_spirv_OpIsNan_v16f16(x)) );
}
#endif //if defined(cl_khr_fp16) && (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#if defined(cl_khr_fp64)

double __builtin_spirv_OpenCL_fract_f64_p1f64( double x,
                                        __global double* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_f64_f64_i64( __builtin_spirv_OpenCL_floor_f64( x ), __builtin_spirv_OpenCL_nan_i64( (ulong)0 ), (long)(__builtin_spirv_OpIsNan_f64( x )) );
    double temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_f64_f64_i64( __builtin_spirv_OpenCL_fmin_f64_f64( temp, 0x1.fffffffffffffp-1), __builtin_spirv_OpenCL_copysign_f64_f64(0.0, x), (long)__builtin_spirv_OpIsInf_f64(x));
    return __builtin_spirv_OpenCL_select_f64_f64_i64( temp, __builtin_spirv_OpenCL_nan_i64((ulong)0), (long)(__builtin_spirv_OpIsNan_f64(x)) );
}

double2 __builtin_spirv_OpenCL_fract_v2f64_p1v2f64( double2 x,
                                             __global double2* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v2f64_v2f64_v2i64( __builtin_spirv_OpenCL_floor_v2f64( x ), __builtin_spirv_OpenCL_nan_v2i64( (ulong2)0 ), __convert_ulong2(__builtin_spirv_OpIsNan_v2f64( x )) );
    double2 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v2f64_v2f64_v2i64( (double2)__builtin_spirv_OpenCL_fmin_v2f64_v2f64( temp, (double2)(0x1.fffffffffffffp-1)), (double2)__builtin_spirv_OpenCL_copysign_v2f64_v2f64((double2)0.0, x), __convert_ulong2(__builtin_spirv_OpIsInf_v2f64(x)));
    return __builtin_spirv_OpenCL_select_v2f64_v2f64_v2i64( temp, __builtin_spirv_OpenCL_nan_v2i64((ulong2)0), __convert_ulong2(__builtin_spirv_OpIsNan_v2f64(x)) );
}

double3 __builtin_spirv_OpenCL_fract_v3f64_p1v3f64( double3 x,
                                             __global double3* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v3f64_v3f64_v3i64( __builtin_spirv_OpenCL_floor_v3f64( x ), __builtin_spirv_OpenCL_nan_v3i64( (ulong3)0 ), __convert_ulong3(__builtin_spirv_OpIsNan_v3f64( x )) );
    double3 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v3f64_v3f64_v3i64( (double3)__builtin_spirv_OpenCL_fmin_v3f64_v3f64( temp, (double3)(0x1.fffffffffffffp-1)), (double3)__builtin_spirv_OpenCL_copysign_v3f64_v3f64((double3)0.0, x), __convert_ulong3(__builtin_spirv_OpIsInf_v3f64(x)));
    return __builtin_spirv_OpenCL_select_v3f64_v3f64_v3i64( temp, __builtin_spirv_OpenCL_nan_v3i64((ulong3)0), __convert_ulong3(__builtin_spirv_OpIsNan_v3f64(x)) );
}

double4 __builtin_spirv_OpenCL_fract_v4f64_p1v4f64( double4 x,
                                             __global double4* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v4f64_v4f64_v4i64( __builtin_spirv_OpenCL_floor_v4f64( x ), __builtin_spirv_OpenCL_nan_v4i64( (ulong4)0 ), __convert_ulong4(__builtin_spirv_OpIsNan_v4f64( x )) );
    double4 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v4f64_v4f64_v4i64( (double4)__builtin_spirv_OpenCL_fmin_v4f64_v4f64( temp, (double4)(0x1.fffffffffffffp-1)), (double4)__builtin_spirv_OpenCL_copysign_v4f64_v4f64((double4)0.0, x), __convert_ulong4(__builtin_spirv_OpIsInf_v4f64(x)));
    return __builtin_spirv_OpenCL_select_v4f64_v4f64_v4i64( temp, __builtin_spirv_OpenCL_nan_v4i64((ulong4)0), __convert_ulong4(__builtin_spirv_OpIsNan_v4f64(x)) );
}

double8 __builtin_spirv_OpenCL_fract_v8f64_p1v8f64( double8 x,
                                             __global double8* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v8f64_v8f64_v8i64( __builtin_spirv_OpenCL_floor_v8f64( x ), __builtin_spirv_OpenCL_nan_v8i64( (ulong8)0 ), __convert_ulong8(__builtin_spirv_OpIsNan_v8f64( x )) );
    double8 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v8f64_v8f64_v8i64( (double8)__builtin_spirv_OpenCL_fmin_v8f64_v8f64( temp, (double8)(0x1.fffffffffffffp-1)), (double8)__builtin_spirv_OpenCL_copysign_v8f64_v8f64((double8)0.0, x), __convert_ulong8(__builtin_spirv_OpIsInf_v8f64(x)));
    return __builtin_spirv_OpenCL_select_v8f64_v8f64_v8i64( temp, __builtin_spirv_OpenCL_nan_v8i64((ulong8)0), __convert_ulong8(__builtin_spirv_OpIsNan_v8f64(x)) );
}

double16 __builtin_spirv_OpenCL_fract_v16f64_p1v16f64( double16 x,
                                                __global double16* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v16f64_v16f64_v16i64( __builtin_spirv_OpenCL_floor_v16f64( x ), __builtin_spirv_OpenCL_nan_v16i64( (ulong16)0 ), __convert_ulong16(__builtin_spirv_OpIsNan_v16f64( x )) );
    double16 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v16f64_v16f64_v16i64( (double16)__builtin_spirv_OpenCL_fmin_v16f64_v16f64( temp, (double16)(0x1.fffffffffffffp-1)), (double16)__builtin_spirv_OpenCL_copysign_v16f64_v16f64((double16)0.0, x), __convert_ulong16(__builtin_spirv_OpIsInf_v16f64(x)));
    return __builtin_spirv_OpenCL_select_v16f64_v16f64_v16i64( temp, __builtin_spirv_OpenCL_nan_v16i64((ulong16)0), __convert_ulong16(__builtin_spirv_OpIsNan_v16f64(x)) );
}

double __builtin_spirv_OpenCL_fract_f64_p0f64( double x,
                                        __private double* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_f64_f64_i64( __builtin_spirv_OpenCL_floor_f64( x ), __builtin_spirv_OpenCL_nan_i64( (ulong)0 ), (ulong)(__builtin_spirv_OpIsNan_f64( x )) );
    double temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_f64_f64_i64( (double)__builtin_spirv_OpenCL_fmin_f64_f64( temp, (double)(0x1.fffffffffffffp-1)), (double)__builtin_spirv_OpenCL_copysign_f64_f64((double)0.0, x), (long)__builtin_spirv_OpIsInf_f64(x));
    return __builtin_spirv_OpenCL_select_f64_f64_i64( temp, __builtin_spirv_OpenCL_nan_i64((ulong)0), (ulong)(__builtin_spirv_OpIsNan_f64(x)) );
}

double2 __builtin_spirv_OpenCL_fract_v2f64_p0v2f64( double2 x,
                                             __private double2* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v2f64_v2f64_v2i64( __builtin_spirv_OpenCL_floor_v2f64( x ), __builtin_spirv_OpenCL_nan_v2i64( (ulong2)0 ), __convert_ulong2(__builtin_spirv_OpIsNan_v2f64( x )) );
    double2 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v2f64_v2f64_v2i64( (double2)__builtin_spirv_OpenCL_fmin_v2f64_v2f64( temp, (double2)(0x1.fffffffffffffp-1)), (double2)__builtin_spirv_OpenCL_copysign_v2f64_v2f64((double2)0.0, x), __convert_ulong2(__builtin_spirv_OpIsInf_v2f64(x)));
    return __builtin_spirv_OpenCL_select_v2f64_v2f64_v2i64( temp, __builtin_spirv_OpenCL_nan_v2i64((ulong2)0), __convert_ulong2(__builtin_spirv_OpIsNan_v2f64(x)) );
}

double3 __builtin_spirv_OpenCL_fract_v3f64_p0v3f64( double3 x,
                                             __private double3* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v3f64_v3f64_v3i64( __builtin_spirv_OpenCL_floor_v3f64( x ), __builtin_spirv_OpenCL_nan_v3i64( (ulong3)0 ), __convert_ulong3(__builtin_spirv_OpIsNan_v3f64( x )) );
    double3 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v3f64_v3f64_v3i64( (double3)__builtin_spirv_OpenCL_fmin_v3f64_v3f64( temp, (double3)(0x1.fffffffffffffp-1)), (double3)__builtin_spirv_OpenCL_copysign_v3f64_v3f64((double3)0.0, x), __convert_ulong3(__builtin_spirv_OpIsInf_v3f64(x)));
    return __builtin_spirv_OpenCL_select_v3f64_v3f64_v3i64( temp, __builtin_spirv_OpenCL_nan_v3i64((ulong3)0), __convert_ulong3(__builtin_spirv_OpIsNan_v3f64(x)) );
}

double4 __builtin_spirv_OpenCL_fract_v4f64_p0v4f64( double4 x,
                                             __private double4* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v4f64_v4f64_v4i64( __builtin_spirv_OpenCL_floor_v4f64( x ), __builtin_spirv_OpenCL_nan_v4i64( (ulong4)0 ), __convert_ulong4(__builtin_spirv_OpIsNan_v4f64( x )) );
    double4 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v4f64_v4f64_v4i64( (double4)__builtin_spirv_OpenCL_fmin_v4f64_v4f64( temp, (double4)(0x1.fffffffffffffp-1)), (double4)__builtin_spirv_OpenCL_copysign_v4f64_v4f64((double4)0.0, x), __convert_ulong4(__builtin_spirv_OpIsInf_v4f64(x)));
    return __builtin_spirv_OpenCL_select_v4f64_v4f64_v4i64( temp, __builtin_spirv_OpenCL_nan_v4i64((ulong4)0), __convert_ulong4(__builtin_spirv_OpIsNan_v4f64(x)) );
}

double8 __builtin_spirv_OpenCL_fract_v8f64_p0v8f64( double8 x,
                                             __private double8* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v8f64_v8f64_v8i64( __builtin_spirv_OpenCL_floor_v8f64( x ), __builtin_spirv_OpenCL_nan_v8i64( (ulong8)0 ), __convert_ulong8(__builtin_spirv_OpIsNan_v8f64( x )) );
    double8 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v8f64_v8f64_v8i64( (double8)__builtin_spirv_OpenCL_fmin_v8f64_v8f64( temp, (double8)(0x1.fffffffffffffp-1)), (double8)__builtin_spirv_OpenCL_copysign_v8f64_v8f64((double8)0.0, x), __convert_ulong8(__builtin_spirv_OpIsInf_v8f64(x)));
    return __builtin_spirv_OpenCL_select_v8f64_v8f64_v8i64( temp, __builtin_spirv_OpenCL_nan_v8i64((ulong8)0), __convert_ulong8(__builtin_spirv_OpIsNan_v8f64(x)) );
}

double16 __builtin_spirv_OpenCL_fract_v16f64_p0v16f64( double16 x,
                                                __private double16* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v16f64_v16f64_v16i64( __builtin_spirv_OpenCL_floor_v16f64( x ), __builtin_spirv_OpenCL_nan_v16i64( (ulong16)0 ), __convert_ulong16(__builtin_spirv_OpIsNan_v16f64( x )) );
    double16 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v16f64_v16f64_v16i64( (double16)__builtin_spirv_OpenCL_fmin_v16f64_v16f64( temp, (double16)(0x1.fffffffffffffp-1)), (double16)__builtin_spirv_OpenCL_copysign_v16f64_v16f64((double16)0.0, x), __convert_ulong16(__builtin_spirv_OpIsInf_v16f64(x)));
    return __builtin_spirv_OpenCL_select_v16f64_v16f64_v16i64( temp, __builtin_spirv_OpenCL_nan_v16i64((ulong16)0), __convert_ulong16(__builtin_spirv_OpIsNan_v16f64(x)) );
}

double __builtin_spirv_OpenCL_fract_f64_p3f64( double x,
                                        __local double* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_f64_f64_i64( __builtin_spirv_OpenCL_floor_f64( x ), __builtin_spirv_OpenCL_nan_i64( (ulong)0 ), (long)(__builtin_spirv_OpIsNan_f64( x )) );
    double temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_f64_f64_i64( (double)__builtin_spirv_OpenCL_fmin_f64_f64( temp, (double)(0x1.fffffffffffffp-1)), (double)__builtin_spirv_OpenCL_copysign_f64_f64((double)0.0, x), (long)__builtin_spirv_OpIsInf_f64(x));
    return __builtin_spirv_OpenCL_select_f64_f64_i64( temp, __builtin_spirv_OpenCL_nan_i64((ulong)0), (long)(__builtin_spirv_OpIsNan_f64(x)) );
}

double2 __builtin_spirv_OpenCL_fract_v2f64_p3v2f64( double2 x,
                                             __local double2* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v2f64_v2f64_v2i64( __builtin_spirv_OpenCL_floor_v2f64( x ), __builtin_spirv_OpenCL_nan_v2i64( (ulong2)0 ), __convert_ulong2(__builtin_spirv_OpIsNan_v2f64( x )) );
    double2 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v2f64_v2f64_v2i64( (double2)__builtin_spirv_OpenCL_fmin_v2f64_v2f64( temp, (double2)(0x1.fffffffffffffp-1)), (double2)__builtin_spirv_OpenCL_copysign_v2f64_v2f64((double2)0.0, x), __convert_ulong2(__builtin_spirv_OpIsInf_v2f64(x)));
    return __builtin_spirv_OpenCL_select_v2f64_v2f64_v2i64( temp, __builtin_spirv_OpenCL_nan_v2i64((ulong2)0), __convert_ulong2(__builtin_spirv_OpIsNan_v2f64(x)) );
}

double3 __builtin_spirv_OpenCL_fract_v3f64_p3v3f64( double3 x,
                                             __local double3* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v3f64_v3f64_v3i64( __builtin_spirv_OpenCL_floor_v3f64( x ), __builtin_spirv_OpenCL_nan_v3i64( (ulong3)0 ), __convert_ulong3(__builtin_spirv_OpIsNan_v3f64( x )) );
    double3 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v3f64_v3f64_v3i64( (double3)__builtin_spirv_OpenCL_fmin_v3f64_v3f64( temp, (double3)(0x1.fffffffffffffp-1)), (double3)__builtin_spirv_OpenCL_copysign_v3f64_v3f64((double3)0.0, x), __convert_ulong3(__builtin_spirv_OpIsInf_v3f64(x)));
    return __builtin_spirv_OpenCL_select_v3f64_v3f64_v3i64( temp, __builtin_spirv_OpenCL_nan_v3i64((ulong3)0), __convert_ulong3(__builtin_spirv_OpIsNan_v3f64(x)) );
}

double4 __builtin_spirv_OpenCL_fract_v4f64_p3v4f64( double4 x,
                                             __local double4* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v4f64_v4f64_v4i64( __builtin_spirv_OpenCL_floor_v4f64( x ), __builtin_spirv_OpenCL_nan_v4i64( (ulong4)0 ), __convert_ulong4(__builtin_spirv_OpIsNan_v4f64( x )) );
    double4 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v4f64_v4f64_v4i64( (double4)__builtin_spirv_OpenCL_fmin_v4f64_v4f64( temp, (double4)(0x1.fffffffffffffp-1)), (double4)__builtin_spirv_OpenCL_copysign_v4f64_v4f64((double4)0.0, x), __convert_ulong4(__builtin_spirv_OpIsInf_v4f64(x)));
    return __builtin_spirv_OpenCL_select_v4f64_v4f64_v4i64( temp, __builtin_spirv_OpenCL_nan_v4i64((ulong4)0), __convert_ulong4(__builtin_spirv_OpIsNan_v4f64(x)) );
}

double8 __builtin_spirv_OpenCL_fract_v8f64_p3v8f64( double8 x,
                                             __local double8* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v8f64_v8f64_v8i64( __builtin_spirv_OpenCL_floor_v8f64( x ), __builtin_spirv_OpenCL_nan_v8i64( (ulong8)0 ), __convert_ulong8(__builtin_spirv_OpIsNan_v8f64( x )) );
    double8 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v8f64_v8f64_v8i64( (double8)__builtin_spirv_OpenCL_fmin_v8f64_v8f64( temp, (double8)(0x1.fffffffffffffp-1)), (double8)__builtin_spirv_OpenCL_copysign_v8f64_v8f64((double8)0.0, x), __convert_ulong8(__builtin_spirv_OpIsInf_v8f64(x)));
    return __builtin_spirv_OpenCL_select_v8f64_v8f64_v8i64( temp, __builtin_spirv_OpenCL_nan_v8i64((ulong8)0), __convert_ulong8(__builtin_spirv_OpIsNan_v8f64(x)) );
}

double16 __builtin_spirv_OpenCL_fract_v16f64_p3v16f64( double16 x,
                                                __local double16* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v16f64_v16f64_v16i64( __builtin_spirv_OpenCL_floor_v16f64( x ), __builtin_spirv_OpenCL_nan_v16i64( (ulong16)0 ), __convert_ulong16(__builtin_spirv_OpIsNan_v16f64( x )) );
    double16 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v16f64_v16f64_v16i64( (double16)__builtin_spirv_OpenCL_fmin_v16f64_v16f64( temp, (double16)(0x1.fffffffffffffp-1)), (double16)__builtin_spirv_OpenCL_copysign_v16f64_v16f64((double16)0.0, x), __convert_ulong16(__builtin_spirv_OpIsInf_v16f64(x)));
    return __builtin_spirv_OpenCL_select_v16f64_v16f64_v16i64( temp, __builtin_spirv_OpenCL_nan_v16i64((ulong16)0), __convert_ulong16(__builtin_spirv_OpIsNan_v16f64(x)) );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

double __builtin_spirv_OpenCL_fract_f64_p4f64( double x,
                                        __generic double* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_f64_f64_i64( __builtin_spirv_OpenCL_floor_f64( x ), __builtin_spirv_OpenCL_nan_i64( (ulong)0 ), (long)(__builtin_spirv_OpIsNan_f64( x )) );
    double temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_f64_f64_i64( (double)__builtin_spirv_OpenCL_fmin_f64_f64( temp, (double)(0x1.fffffffffffffp-1)), (double)__builtin_spirv_OpenCL_copysign_f64_f64((double)0.0, x), (long)__builtin_spirv_OpIsInf_f64(x));
    return __builtin_spirv_OpenCL_select_f64_f64_i64( temp, __builtin_spirv_OpenCL_nan_i64((ulong)0), (long)(__builtin_spirv_OpIsNan_f64(x)) );
}

double2 __builtin_spirv_OpenCL_fract_v2f64_p4v2f64( double2 x,
                                             __generic double2* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v2f64_v2f64_v2i64( __builtin_spirv_OpenCL_floor_v2f64( x ), __builtin_spirv_OpenCL_nan_v2i64( (ulong2)0 ), __convert_ulong2(__builtin_spirv_OpIsNan_v2f64( x )) );
    double2 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v2f64_v2f64_v2i64( (double2)__builtin_spirv_OpenCL_fmin_v2f64_v2f64( temp, (double2)(0x1.fffffffffffffp-1)), (double2)__builtin_spirv_OpenCL_copysign_v2f64_v2f64((double2)0.0, x), __convert_ulong2(__builtin_spirv_OpIsInf_v2f64(x)));
    return __builtin_spirv_OpenCL_select_v2f64_v2f64_v2i64( temp, __builtin_spirv_OpenCL_nan_v2i64((ulong2)0), __convert_ulong2(__builtin_spirv_OpIsNan_v2f64(x)) );
}

double3 __builtin_spirv_OpenCL_fract_v3f64_p4v3f64( double3 x,
                                             __generic double3* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v3f64_v3f64_v3i64( __builtin_spirv_OpenCL_floor_v3f64( x ), __builtin_spirv_OpenCL_nan_v3i64( (ulong3)0 ), __convert_ulong3(__builtin_spirv_OpIsNan_v3f64( x )) );
    double3 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v3f64_v3f64_v3i64( (double3)__builtin_spirv_OpenCL_fmin_v3f64_v3f64( temp, (double3)(0x1.fffffffffffffp-1)), (double3)__builtin_spirv_OpenCL_copysign_v3f64_v3f64((double3)0.0, x), __convert_ulong3(__builtin_spirv_OpIsInf_v3f64(x)));
    return __builtin_spirv_OpenCL_select_v3f64_v3f64_v3i64( temp, __builtin_spirv_OpenCL_nan_v3i64((ulong3)0), __convert_ulong3(__builtin_spirv_OpIsNan_v3f64(x)) );
}

double4 __builtin_spirv_OpenCL_fract_v4f64_p4v4f64( double4 x,
                                             __generic double4* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v4f64_v4f64_v4i64( __builtin_spirv_OpenCL_floor_v4f64( x ), __builtin_spirv_OpenCL_nan_v4i64( (ulong4)0 ), __convert_ulong4(__builtin_spirv_OpIsNan_v4f64( x )) );
    double4 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v4f64_v4f64_v4i64( (double4)__builtin_spirv_OpenCL_fmin_v4f64_v4f64( temp, (double4)(0x1.fffffffffffffp-1)), (double4)__builtin_spirv_OpenCL_copysign_v4f64_v4f64((double4)0.0, x), __convert_ulong4(__builtin_spirv_OpIsInf_v4f64(x)));
    return __builtin_spirv_OpenCL_select_v4f64_v4f64_v4i64( temp, __builtin_spirv_OpenCL_nan_v4i64((ulong4)0), __convert_ulong4(__builtin_spirv_OpIsNan_v4f64(x)) );
}

double8 __builtin_spirv_OpenCL_fract_v8f64_p4v8f64( double8 x,
                                             __generic double8* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v8f64_v8f64_v8i64( __builtin_spirv_OpenCL_floor_v8f64( x ), __builtin_spirv_OpenCL_nan_v8i64( (ulong8)0 ), __convert_ulong8(__builtin_spirv_OpIsNan_v8f64( x )) );
    double8 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v8f64_v8f64_v8i64( (double8)__builtin_spirv_OpenCL_fmin_v8f64_v8f64( temp, (double8)(0x1.fffffffffffffp-1)), (double8)__builtin_spirv_OpenCL_copysign_v8f64_v8f64((double8)0.0, x), __convert_ulong8(__builtin_spirv_OpIsInf_v8f64(x)));
    return __builtin_spirv_OpenCL_select_v8f64_v8f64_v8i64( temp, __builtin_spirv_OpenCL_nan_v8i64((ulong8)0), __convert_ulong8(__builtin_spirv_OpIsNan_v8f64(x)) );
}

double16 __builtin_spirv_OpenCL_fract_v16f64_p4v16f64( double16 x,
                                                __generic double16* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v16f64_v16f64_v16i64( __builtin_spirv_OpenCL_floor_v16f64( x ), __builtin_spirv_OpenCL_nan_v16i64( (ulong16)0 ), __convert_ulong16(__builtin_spirv_OpIsNan_v16f64( x )) );
    double16 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v16f64_v16f64_v16i64( (double16)__builtin_spirv_OpenCL_fmin_v16f64_v16f64( temp, (double16)(0x1.fffffffffffffp-1)), (double16)__builtin_spirv_OpenCL_copysign_v16f64_v16f64((double16)0.0, x), __convert_ulong16(__builtin_spirv_OpIsInf_v16f64(x)));
    return __builtin_spirv_OpenCL_select_v16f64_v16f64_v16i64( temp, __builtin_spirv_OpenCL_nan_v16i64((ulong16)0), __convert_ulong16(__builtin_spirv_OpIsNan_v16f64(x)) );
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
#endif // defined(cl_khr_fp64)

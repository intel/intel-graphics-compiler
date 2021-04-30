/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float __builtin_spirv_OpenCL_fast_normalize_f32(float p ){
    return __builtin_spirv_OpenCL_normalize_f32(p);
}

INLINE float2 __builtin_spirv_OpenCL_fast_normalize_v2f32(float2 p ){
    float l2 = SPIRV_BUILTIN(Dot, _v2f32_v2f32, )( p, p );
    float2 n = p * __builtin_spirv_OpenCL_native_rsqrt_f32( l2 );;
    return l2 == 0.0f ? p : n;
}

INLINE float3 __builtin_spirv_OpenCL_fast_normalize_v3f32(float3 p ){
    float l2 = SPIRV_BUILTIN(Dot, _v3f32_v3f32, )( p, p );
    float3 n = p * __builtin_spirv_OpenCL_native_rsqrt_f32( l2 );;
    return l2 == 0.0f ? p : n;
}

INLINE float4 __builtin_spirv_OpenCL_fast_normalize_v4f32(float4 p ){
    float l2 = SPIRV_BUILTIN(Dot, _v4f32_v4f32, )( p, p );
    float4 n = p * __builtin_spirv_OpenCL_native_rsqrt_f32( l2 );;
    return l2 == 0.0f ? p : n;
}

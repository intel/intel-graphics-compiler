/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fast_normalize, _f32, )(float p ){
    return SPIRV_OCL_BUILTIN(normalize, _f32, )(p);
}

INLINE float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fast_normalize, _v2f32, )(float2 p ){
    float l2 = SPIRV_BUILTIN(Dot, _v2f32_v2f32, )( p, p );
    float2 n = p * SPIRV_OCL_BUILTIN(native_rsqrt, _f32, )( l2 );;
    return l2 == 0.0f ? p : n;
}

INLINE float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fast_normalize, _v3f32, )(float3 p ){
    float l2 = SPIRV_BUILTIN(Dot, _v3f32_v3f32, )( p, p );
    float3 n = p * SPIRV_OCL_BUILTIN(native_rsqrt, _f32, )( l2 );;
    return l2 == 0.0f ? p : n;
}

INLINE float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fast_normalize, _v4f32, )(float4 p ){
    float l2 = SPIRV_BUILTIN(Dot, _v4f32_v4f32, )( p, p );
    float4 n = p * SPIRV_OCL_BUILTIN(native_rsqrt, _f32, )( l2 );;
    return l2 == 0.0f ? p : n;
}

/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_powr, _f32_f32, )(float x, float y ){
    return SPIRV_OCL_BUILTIN(powr, _f32_f32, )(x, y);
}

INLINE float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_powr, _v2f32_v2f32, )(float2 x, float2 y ){
    return SPIRV_OCL_BUILTIN(powr, _v2f32_v2f32, )(x, y);
}

INLINE float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_powr, _v3f32_v3f32, )(float3 x, float3 y ){
    return SPIRV_OCL_BUILTIN(powr, _v3f32_v3f32, )(x, y);
}

INLINE float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_powr, _v4f32_v4f32, )(float4 x, float4 y ){
    return SPIRV_OCL_BUILTIN(powr, _v4f32_v4f32, )(x, y);
}

INLINE float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_powr, _v8f32_v8f32, )(float8 x, float8 y ){
    return SPIRV_OCL_BUILTIN(powr, _v8f32_v8f32, )(x, y);
}

INLINE float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_powr, _v16f32_v16f32, )(float16 x, float16 y ){
    return SPIRV_OCL_BUILTIN(powr, _v16f32_v16f32, )(x, y);
}

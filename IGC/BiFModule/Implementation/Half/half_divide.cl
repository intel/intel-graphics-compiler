/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_divide, _f32_f32, )(float x, float y ){
    return x / y;
}

INLINE float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_divide, _v2f32_v2f32, )(float2 x, float2 y ){
    return x / y;
}

INLINE float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_divide, _v3f32_v3f32, )(float3 x, float3 y ){
    return x / y;
}

INLINE float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_divide, _v4f32_v4f32, )(float4 x, float4 y ){
    return x / y;
}

INLINE float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_divide, _v8f32_v8f32, )(float8 x, float8 y ){
    return x / y;
}

INLINE float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_divide, _v16f32_v16f32, )(float16 x, float16 y ){
    return x / y;
}

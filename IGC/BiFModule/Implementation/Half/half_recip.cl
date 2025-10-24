/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_recip, _f32, )(float x ){
    return SPIRV_OCL_BUILTIN(native_recip, _f32, )(x);
}

INLINE float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_recip, _v2f32, )(float2 x ){
    return SPIRV_OCL_BUILTIN(native_recip, _v2f32, )(x);
}

INLINE float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_recip, _v3f32, )(float3 x ){
    return SPIRV_OCL_BUILTIN(native_recip, _v3f32, )(x);
}

INLINE float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_recip, _v4f32, )(float4 x ){
    return SPIRV_OCL_BUILTIN(native_recip, _v4f32, )(x);
}

INLINE float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_recip, _v8f32, )(float8 x ){
    return SPIRV_OCL_BUILTIN(native_recip, _v8f32, )(x);
}

INLINE float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_recip, _v16f32, )(float16 x ){
    return SPIRV_OCL_BUILTIN(native_recip, _v16f32, )(x);
}

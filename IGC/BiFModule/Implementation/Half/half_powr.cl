/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float __builtin_spirv_OpenCL_half_powr_f32_f32(float x, float y ){
    return __builtin_spirv_OpenCL_powr_f32_f32(x, y);
}

INLINE float2 __builtin_spirv_OpenCL_half_powr_v2f32_v2f32(float2 x, float2 y ){
    return __builtin_spirv_OpenCL_powr_v2f32_v2f32(x, y);
}

INLINE float3 __builtin_spirv_OpenCL_half_powr_v3f32_v3f32(float3 x, float3 y ){
    return __builtin_spirv_OpenCL_powr_v3f32_v3f32(x, y);
}

INLINE float4 __builtin_spirv_OpenCL_half_powr_v4f32_v4f32(float4 x, float4 y ){
    return __builtin_spirv_OpenCL_powr_v4f32_v4f32(x, y);
}

INLINE float8 __builtin_spirv_OpenCL_half_powr_v8f32_v8f32(float8 x, float8 y ){
    return __builtin_spirv_OpenCL_powr_v8f32_v8f32(x, y);
}

INLINE float16 __builtin_spirv_OpenCL_half_powr_v16f32_v16f32(float16 x, float16 y ){
    return __builtin_spirv_OpenCL_powr_v16f32_v16f32(x, y);
}

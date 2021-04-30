/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float __builtin_spirv_OpenCL_half_exp_f32(float x ){
    return __builtin_spirv_OpenCL_native_exp_f32(x);
}

INLINE float2 __builtin_spirv_OpenCL_half_exp_v2f32(float2 x ){
    return __builtin_spirv_OpenCL_native_exp_v2f32(x);
}

INLINE float3 __builtin_spirv_OpenCL_half_exp_v3f32(float3 x ){
    return __builtin_spirv_OpenCL_native_exp_v3f32(x);
}

INLINE float4 __builtin_spirv_OpenCL_half_exp_v4f32(float4 x ){
    return __builtin_spirv_OpenCL_native_exp_v4f32(x);
}

INLINE float8 __builtin_spirv_OpenCL_half_exp_v8f32(float8 x ){
    return __builtin_spirv_OpenCL_native_exp_v8f32(x);
}

INLINE float16 __builtin_spirv_OpenCL_half_exp_v16f32(float16 x ){
    return __builtin_spirv_OpenCL_native_exp_v16f32(x);
}

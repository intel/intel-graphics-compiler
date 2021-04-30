/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float __builtin_spirv_OpenCL_fast_length_f32(float p ){
    return __builtin_spirv_OpenCL_fabs_f32(p);
}

INLINE float __builtin_spirv_OpenCL_fast_length_v2f32(float2 p ){
    return __builtin_spirv_OpenCL_native_sqrt_f32( SPIRV_BUILTIN(Dot, _v2f32_v2f32, )( p, p ) );
}

INLINE float __builtin_spirv_OpenCL_fast_length_v3f32(float3 p ){
    return __builtin_spirv_OpenCL_native_sqrt_f32( SPIRV_BUILTIN(Dot, _v3f32_v3f32, )( p, p ) );
}

INLINE float __builtin_spirv_OpenCL_fast_length_v4f32(float4 p ){
    return __builtin_spirv_OpenCL_native_sqrt_f32( SPIRV_BUILTIN(Dot, _v4f32_v4f32, )( p, p ) );
}

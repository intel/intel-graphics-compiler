/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float __builtin_spirv_OpenCL_fast_distance_f32_f32(float p0, float p1 ){
    return __builtin_spirv_OpenCL_fast_length_f32( p0 - p1 );
}

INLINE float __builtin_spirv_OpenCL_fast_distance_v2f32_v2f32(float2 p0, float2 p1 ){
    return __builtin_spirv_OpenCL_fast_length_v2f32( p0 - p1 );
}

INLINE float __builtin_spirv_OpenCL_fast_distance_v3f32_v3f32(float3 p0, float3 p1 ){
    return __builtin_spirv_OpenCL_fast_length_v3f32( p0 - p1 );
}

INLINE float __builtin_spirv_OpenCL_fast_distance_v4f32_v4f32(float4 p0, float4 p1 ){
    return __builtin_spirv_OpenCL_fast_length_v4f32( p0 - p1 );
}

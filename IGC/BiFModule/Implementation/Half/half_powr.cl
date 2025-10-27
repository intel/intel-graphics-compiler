/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float __attribute__((overloadable)) __spirv_ocl_half_powr(float x, float y ){
    return __spirv_ocl_powr(x, y);
}

INLINE float2 __attribute__((overloadable)) __spirv_ocl_half_powr(float2 x, float2 y ){
    return __spirv_ocl_powr(x, y);
}

INLINE float3 __attribute__((overloadable)) __spirv_ocl_half_powr(float3 x, float3 y ){
    return __spirv_ocl_powr(x, y);
}

INLINE float4 __attribute__((overloadable)) __spirv_ocl_half_powr(float4 x, float4 y ){
    return __spirv_ocl_powr(x, y);
}

INLINE float8 __attribute__((overloadable)) __spirv_ocl_half_powr(float8 x, float8 y ){
    return __spirv_ocl_powr(x, y);
}

INLINE float16 __attribute__((overloadable)) __spirv_ocl_half_powr(float16 x, float16 y ){
    return __spirv_ocl_powr(x, y);
}

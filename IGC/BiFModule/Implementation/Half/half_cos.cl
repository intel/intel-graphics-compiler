/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float __attribute__((overloadable)) __spirv_ocl_half_cos(float x ){
    return __spirv_ocl_cos(x);
}

INLINE float2 __attribute__((overloadable)) __spirv_ocl_half_cos(float2 x ){
    return __spirv_ocl_cos(x);
}

INLINE float3 __attribute__((overloadable)) __spirv_ocl_half_cos(float3 x ){
    return __spirv_ocl_cos(x);
}

INLINE float4 __attribute__((overloadable)) __spirv_ocl_half_cos(float4 x ){
    return __spirv_ocl_cos(x);
}

INLINE float8 __attribute__((overloadable)) __spirv_ocl_half_cos(float8 x ){
    return __spirv_ocl_cos(x);
}

INLINE float16 __attribute__((overloadable)) __spirv_ocl_half_cos(float16 x ){
    return __spirv_ocl_cos(x);
}

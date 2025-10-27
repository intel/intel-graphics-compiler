/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float __attribute__((overloadable)) __spirv_ocl_half_exp(float x ){
    return __spirv_ocl_native_exp(x);
}

INLINE float2 __attribute__((overloadable)) __spirv_ocl_half_exp(float2 x ){
    return __spirv_ocl_native_exp(x);
}

INLINE float3 __attribute__((overloadable)) __spirv_ocl_half_exp(float3 x ){
    return __spirv_ocl_native_exp(x);
}

INLINE float4 __attribute__((overloadable)) __spirv_ocl_half_exp(float4 x ){
    return __spirv_ocl_native_exp(x);
}

INLINE float8 __attribute__((overloadable)) __spirv_ocl_half_exp(float8 x ){
    return __spirv_ocl_native_exp(x);
}

INLINE float16 __attribute__((overloadable)) __spirv_ocl_half_exp(float16 x ){
    return __spirv_ocl_native_exp(x);
}

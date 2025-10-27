/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float __attribute__((overloadable)) __spirv_ocl_fast_normalize(float p ){
    return __spirv_ocl_normalize(p);
}

INLINE float2 __attribute__((overloadable)) __spirv_ocl_fast_normalize(float2 p ){
    float l2 = __spirv_Dot( p, p );
    float2 n = p * __spirv_ocl_native_rsqrt( l2 );;
    return l2 == 0.0f ? p : n;
}

INLINE float3 __attribute__((overloadable)) __spirv_ocl_fast_normalize(float3 p ){
    float l2 = __spirv_Dot( p, p );
    float3 n = p * __spirv_ocl_native_rsqrt( l2 );;
    return l2 == 0.0f ? p : n;
}

INLINE float4 __attribute__((overloadable)) __spirv_ocl_fast_normalize(float4 p ){
    float l2 = __spirv_Dot( p, p );
    float4 n = p * __spirv_ocl_native_rsqrt( l2 );;
    return l2 == 0.0f ? p : n;
}

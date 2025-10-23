/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float __attribute__((overloadable)) __spirv_ocl_fast_length(float p ){
    return __spirv_ocl_fabs(p);
}

INLINE float __attribute__((overloadable)) __spirv_ocl_fast_length(float2 p ){
    return __spirv_ocl_native_sqrt( __spirv_Dot( p, p ) );
}

INLINE float __attribute__((overloadable)) __spirv_ocl_fast_length(float3 p ){
    return __spirv_ocl_native_sqrt( __spirv_Dot( p, p ) );
}

INLINE float __attribute__((overloadable)) __spirv_ocl_fast_length(float4 p ){
    return __spirv_ocl_native_sqrt( __spirv_Dot( p, p ) );
}

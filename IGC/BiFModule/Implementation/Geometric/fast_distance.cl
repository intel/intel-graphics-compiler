/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float __attribute__((overloadable)) __spirv_ocl_fast_distance(float p0, float p1 ){
    return __spirv_ocl_fast_length( p0 - p1 );
}

INLINE float __attribute__((overloadable)) __spirv_ocl_fast_distance(float2 p0, float2 p1 ){
    return __spirv_ocl_fast_length( p0 - p1 );
}

INLINE float __attribute__((overloadable)) __spirv_ocl_fast_distance(float3 p0, float3 p1 ){
    return __spirv_ocl_fast_length( p0 - p1 );
}

INLINE float __attribute__((overloadable)) __spirv_ocl_fast_distance(float4 p0, float4 p1 ){
    return __spirv_ocl_fast_length( p0 - p1 );
}

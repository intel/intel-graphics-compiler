/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "include/BiF_Definitions.cl"
#include "spirv_bfloat.h"

INLINE bfloat __attribute__((overloadable)) __spirv_ocl_distance(bfloat p0, bfloat p1 ){
    return __spirv_ocl_length( p0 - p1 );
}

INLINE bfloat __attribute__((overloadable)) __spirv_ocl_distance(bfloat2 p0, bfloat2 p1 ){
    return __spirv_ocl_length( p0 - p1 );
}

INLINE bfloat __attribute__((overloadable)) __spirv_ocl_distance(bfloat3 p0, bfloat3 p1 ){
    return __spirv_ocl_length( p0 - p1 );
}

INLINE bfloat __attribute__((overloadable)) __spirv_ocl_distance(bfloat4 p0, bfloat4 p1 ){
    return __spirv_ocl_length( p0 - p1 );
}

/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "include/BiF_Definitions.cl"
#include "spirv_bfloat.h"

INLINE bfloat __attribute__((overloadable)) __spirv_ocl_smoothstep(bfloat edge0, bfloat edge1, bfloat x ){
    bfloat div = (x - edge0) / (edge1 - edge0);
    bfloat temp = __spirv_ocl_fclamp( div, (bfloat)0.0f, (bfloat)1.0f );
    return temp * temp * __spirv_ocl_mad( (bfloat)-2.0f, temp, (bfloat)3.0f );
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_3ARGS( smoothstep, bfloat, bfloat, )

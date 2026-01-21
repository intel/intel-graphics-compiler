/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "include/BiF_Definitions.cl"
#include "spirv_bfloat.h"

INLINE bfloat __attribute__((overloadable)) __spirv_ocl_step(bfloat edge, bfloat x ){
    return x < edge ? (bfloat)0.0f : (bfloat)1.0f;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS( step, bfloat, bfloat, )

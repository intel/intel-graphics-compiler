/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "include/BiF_Definitions.cl"
#include "spirv_bfloat.h"

INLINE bfloat __attribute__((overloadable)) __spirv_ocl_sign( bfloat x )
{
    bfloat result = x;
    result = ( x > (bfloat)0.0f ) ? (bfloat) 1.0f : result;
    result = ( x < (bfloat)0.0f ) ? (bfloat)-1.0f : result;
    result = __intel_relaxed_isnan(x) ? (bfloat)0.0f : result;
    return result ;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( sign, bfloat, bfloat, )

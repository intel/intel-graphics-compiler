/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "include/BiF_Definitions.cl"
#include "spirv_bfloat.h"

INLINE bfloat __attribute__((overloadable)) __spirv_ocl_native_cos( bfloat x )
{
    return as_bfloat(__builtin_bf16_cos(as_ushort(x)));
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( native_cos, bfloat, bfloat, )

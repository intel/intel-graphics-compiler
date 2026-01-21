/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "include/BiF_Definitions.cl"
#include "spirv_bfloat.h"

INLINE bfloat __attribute__((overloadable)) __spirv_ocl_native_log( bfloat x )
{
    bfloat bf16_M_LN_2 = (bfloat)M_LN2_F;
    bfloat bf16_log2_x = __spirv_ocl_native_log2(x);
    bfloat result = as_bfloat(__builtin_bf16_mul(as_ushort(bf16_log2_x), as_ushort(bf16_M_LN_2)));
    return result;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( native_log, bfloat, bfloat, )

/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "include/BiF_Definitions.cl"
#include "spirv_bfloat.h"

INLINE bfloat __attribute__((overloadable)) __spirv_ocl_native_exp( bfloat x )
{
    // Regular multiplication is upconverted to float by clang.
    // return __spirv_ocl_native_exp2(x * (bfloat)M_LOG2E_F);
    bfloat bf16_LOG2E = (bfloat)M_LOG2E_F;
    bfloat result = as_bfloat(__builtin_bf16_mul(as_ushort(x), as_ushort(bf16_LOG2E)));
    result = __spirv_ocl_native_exp2(result);
    return result;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( native_exp, bfloat, bfloat, )

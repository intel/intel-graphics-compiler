/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "include/BiF_Definitions.cl"
#include "spirv_bfloat.h"

#define _M_LOG2_10      (as_float(0x40549A78))          // 3.321928094887362f

INLINE bfloat __attribute__((overloadable)) __spirv_ocl_native_exp10( bfloat x )
{
    // Regular multiplication is upconverted to float by clang.
    // return __spirv_ocl_native_exp2( x * (bfloat)(_M_LOG2_10) );
    bfloat bf16_LOG2_10 = (bfloat)_M_LOG2_10;
    bfloat result = as_bfloat(__builtin_bf16_mul(as_ushort(x), as_ushort(bf16_LOG2_10)));
    result = __spirv_ocl_native_exp2(result);
    return result;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( native_exp10, bfloat, bfloat, )

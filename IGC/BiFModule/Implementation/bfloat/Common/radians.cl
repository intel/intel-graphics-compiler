/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "include/BiF_Definitions.cl"
#include "spirv_bfloat.h"

INLINE bfloat __attribute__((overloadable)) __spirv_ocl_radians(bfloat d ){
    return as_bfloat(__builtin_bf16_mul(as_ushort(d), as_ushort(PI_OVER_ONE_EIGHTY_BFLT)));
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( radians, bfloat, bfloat, )

/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "include/BiF_Definitions.cl"
#include "spirv_bfloat.h"

INLINE bfloat __attribute__((overloadable)) __spirv_ocl_fma( bfloat a, bfloat b, bfloat c )
{
    return as_bfloat(__builtin_bf16_mad(as_ushort(a), as_ushort(b), as_ushort(c)));
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_3ARGS( fma, bfloat, bfloat, )

/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "include/BiF_Definitions.cl"
#include "spirv_bfloat.h"

INLINE bfloat __attribute__((overloadable)) __spirv_ocl_fabs( bfloat x )
{
    ushort mask = 0x7FFF;
    ushort temp = as_ushort(x) & mask;
    return as_bfloat(temp);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( fabs, bfloat, bfloat, )

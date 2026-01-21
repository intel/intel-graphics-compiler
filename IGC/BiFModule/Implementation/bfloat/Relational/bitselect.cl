/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "include/BiF_Definitions.cl"
#include "spirv_bfloat.h"

INLINE
bfloat __attribute__((overloadable)) __spirv_ocl_bitselect( bfloat a, bfloat b, bfloat c )
{
    return as_bfloat( __spirv_ocl_bitselect(as_short(a), as_short(b), as_short(c)) );
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_3ARGS( bitselect, bfloat, bfloat, )

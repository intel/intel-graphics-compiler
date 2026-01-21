/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "include/BiF_Definitions.cl"
#include "spirv_bfloat.h"

INLINE bfloat __attribute__((overloadable)) __spirv_ocl_select( bfloat a, bfloat b, short c )
{
    return c ? b : a;
}

static INLINE OVERLOADABLE bfloat __intel_vector_select_helper( bfloat a, bfloat b, short c )
{
    return c < 0 ? b : a;
}

GENERATE_VECTOR_FUNCTIONS_3ARGS_SELECT( __intel_vector_select_helper, bfloat, short, , )

/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "include/BiF_Definitions.cl"
#include "spirv_bfloat.h"

INLINE bfloat __attribute__((overloadable)) __spirv_ocl_mad( bfloat a, bfloat b, bfloat c )
{
    // If -cl-mad-enable flag set later optimizations will decide how to combine it into mad
    if(BIF_FLAG_CTRL_GET(MadEnable))
    {
        // Clang upconverts to float
        // return a * b + c;
        return as_bfloat(__builtin_bf16_add(__builtin_bf16_mul(as_ushort(a), as_ushort(b)), as_ushort(c)));
    }
    else
    {
        return __spirv_ocl_fma(a,b,c);
    }
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_3ARGS( mad, bfloat, bfloat, )

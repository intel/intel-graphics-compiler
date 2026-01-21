/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/


INLINE bfloat __attribute__((overloadable)) __spirv_ocl_pow( bfloat x, bfloat y )
{
    return __spirv_ocl_pow((float)x, (float)y);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( pow, bfloat, bfloat, bfloat, , )

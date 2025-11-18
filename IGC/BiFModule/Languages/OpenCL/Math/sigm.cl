/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

float OVERLOADABLE sigm( float x )
{
    return __spirv_ocl_sigm( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( sigm, float, float )

#if defined(cl_khr_fp16)

INLINE half OVERLOADABLE sigm( half x )
{
    return __spirv_ocl_sigm( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( sigm, half, half )

#endif // defined(cl_khr_fp16)

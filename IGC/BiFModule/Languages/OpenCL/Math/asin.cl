/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

GENERATE_VECTOR_FUNCTIONS_1ARG( asin, float, float )

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE asin( double x )
{
    return SPIRV_OCL_BUILTIN(asin, _f64, )( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( asin, double, double )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

GENERATE_VECTOR_FUNCTIONS_1ARG( asin, half, half )

#endif // defined(cl_khr_fp16)

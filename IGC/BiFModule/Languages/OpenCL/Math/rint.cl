/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

GENERATE_VECTOR_FUNCTIONS_1ARG( rint, float, float )

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE rint( double x )
{
    return SPIRV_OCL_BUILTIN(rint, _f64, )( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( rint, double, double )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

GENERATE_VECTOR_FUNCTIONS_1ARG( rint, half, half )

#endif // defined(cl_khr_fp16)

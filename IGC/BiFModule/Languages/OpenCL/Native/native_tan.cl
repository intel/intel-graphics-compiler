/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

GENERATE_VECTOR_FUNCTIONS_1ARG( native_tan, float, float )

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE native_tan( double x )
{
    return SPIRV_OCL_BUILTIN(native_tan, _f64, )( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( native_tan, double, double )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

GENERATE_VECTOR_FUNCTIONS_1ARG( native_tan, half, half )

#endif // defined(cl_khr_fp16)

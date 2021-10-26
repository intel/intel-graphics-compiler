/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

GENERATE_VECTOR_FUNCTIONS_2ARGS( native_powr, float, float )

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE native_powr( double x, double y )
{
    return SPIRV_OCL_BUILTIN(native_powr, _f64_f64, )( x, y );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( native_powr, double, double )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

GENERATE_VECTOR_FUNCTIONS_2ARGS( native_powr, half, half )

#endif // defined(cl_khr_fp16)

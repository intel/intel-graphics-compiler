/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

float OVERLOADABLE tgamma( float x )
{
    return SPIRV_OCL_BUILTIN(tgamma, _f32, )( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( tgamma, float, float )

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE tgamma( double x )
{
    return SPIRV_OCL_BUILTIN(tgamma, _f64, )( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( tgamma, double, double )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half OVERLOADABLE tgamma( half x )
{
    return SPIRV_OCL_BUILTIN(tgamma, _f16, )( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( tgamma, half, half )

#endif // defined(cl_khr_fp16)

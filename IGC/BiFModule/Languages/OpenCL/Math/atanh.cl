/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

float OVERLOADABLE atanh( float x )
{
    return SPIRV_OCL_BUILTIN(atanh, _f32, )( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( atanh, float, float )

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE atanh( double x )
{
    return SPIRV_OCL_BUILTIN(atanh, _f64, )( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( atanh, double, double )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half OVERLOADABLE atanh( half x )
{
    return SPIRV_OCL_BUILTIN(atanh, _f16, )( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( atanh, half, half )

#endif // defined(cl_khr_fp16)

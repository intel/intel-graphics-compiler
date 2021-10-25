/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

#if defined(cl_khr_fp16)

INLINE
half OVERLOADABLE sqrt_cr( half a )
{
    return SPIRV_OCL_BUILTIN(sqrt_cr, _f16, )( a );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( sqrt_cr, half, half )

#endif // define(cl_khr_fp16)

float OVERLOADABLE sqrt_cr( float a )
{
    return SPIRV_OCL_BUILTIN(sqrt_cr, _f32, )( a );
}

#ifdef cl_fp64_basic_ops

INLINE double OVERLOADABLE sqrt_cr( double x )
{
    return SPIRV_OCL_BUILTIN(sqrt_cr, _f64, )( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( sqrt_cr, double, double )

#endif // cl_fp64_basic_ops

GENERATE_VECTOR_FUNCTIONS_1ARG( sqrt_cr, float, float )

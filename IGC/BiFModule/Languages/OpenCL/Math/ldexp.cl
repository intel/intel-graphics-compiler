/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

float OVERLOADABLE ldexp( float x, int n )
{
    return SPIRV_OCL_BUILTIN(ldexp, _f32_i32, )( x, n );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV( ldexp, float, float, int )

#if defined(cl_khr_fp64)

double OVERLOADABLE ldexp( double x, int n )
{
    return SPIRV_OCL_BUILTIN(ldexp, _f64_i32, )( x, n );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV( ldexp, double, double, int )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

half OVERLOADABLE ldexp( half x, int n )
{
    return SPIRV_OCL_BUILTIN(ldexp, _f16_i32, )( x, n );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV( ldexp, half, half, int )

#endif // defined(cl_khr_fp16)

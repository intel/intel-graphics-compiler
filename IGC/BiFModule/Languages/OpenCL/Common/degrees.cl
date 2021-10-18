/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

float OVERLOADABLE degrees( float r )
{
    return SPIRV_OCL_BUILTIN(degrees, _f32, )( r );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( degrees, float, float )

#if defined(cl_khr_fp64)

double OVERLOADABLE degrees( double r )
{
    return SPIRV_OCL_BUILTIN(degrees, _f64, )( r );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( degrees, double, double )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

half OVERLOADABLE degrees( half r )
{
    return SPIRV_OCL_BUILTIN(degrees, _f16, )( r );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( degrees, half, half )

#endif // defined(cl_khr_fp16)

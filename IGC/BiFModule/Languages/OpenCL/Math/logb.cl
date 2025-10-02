/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

float OVERLOADABLE logb( float x )
{
    return __spirv_ocl_logb( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( logb, float, float )

#if defined(cl_khr_fp64)

double OVERLOADABLE logb( double x )
{
    return __spirv_ocl_logb( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( logb, double, double )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half OVERLOADABLE logb( half x )
{
    return __spirv_ocl_logb( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( logb, half, half )

#endif // defined(cl_khr_fp16)

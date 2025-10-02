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
    return __spirv_ocl_sqrt_cr( a );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( sqrt_cr, half, half )

#endif // define(cl_khr_fp16)

float OVERLOADABLE sqrt_cr( float a )
{
    return __spirv_ocl_sqrt_cr( a );
}

#ifdef cl_fp64_basic_ops

INLINE double OVERLOADABLE sqrt_cr( double x )
{
    return __spirv_ocl_sqrt_cr( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( sqrt_cr, double, double )

#endif // cl_fp64_basic_ops

GENERATE_VECTOR_FUNCTIONS_1ARG( sqrt_cr, float, float )

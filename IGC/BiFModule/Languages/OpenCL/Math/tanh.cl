/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

float OVERLOADABLE tanh( float x )
{
    return __spirv_ocl_tanh( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( tanh, float, float )

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE tanh( double x )
{
    return __spirv_ocl_tanh( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( tanh, double, double )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half OVERLOADABLE tanh( half x )
{
    return __spirv_ocl_tanh( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( tanh, half, half )

#endif // defined(cl_khr_fp16)

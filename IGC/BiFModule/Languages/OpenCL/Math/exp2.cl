/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE float OVERLOADABLE exp2( float x )
{
    return __spirv_ocl_exp2( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( exp2, float, float )

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE exp2( double x )
{
    return __spirv_ocl_exp2( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( exp2, double, double )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half OVERLOADABLE exp2( half x )
{
    return __spirv_ocl_exp2( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( exp2, half, half )

#endif // defined(cl_khr_fp16)

/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE float OVERLOADABLE sign( float x )
{
    return __spirv_ocl_sign( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( sign, float, float )

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE sign( double x )
{
    return __spirv_ocl_sign( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( sign, double, double )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half OVERLOADABLE sign( half x )
{
    return __spirv_ocl_sign( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( sign, half, half )

#endif // defined(cl_khr_fp16)

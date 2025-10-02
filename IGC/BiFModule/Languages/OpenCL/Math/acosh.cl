/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

float OVERLOADABLE acosh( float x )
{
    return __spirv_ocl_acosh( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( acosh, float, float )

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE acosh( double x )
{
    return __spirv_ocl_acosh( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( acosh, double, double )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half OVERLOADABLE acosh( half x )
{
    return __spirv_ocl_acosh( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( acosh, half, half )

#endif // defined(cl_khr_fp16)

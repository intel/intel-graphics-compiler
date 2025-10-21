/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

float OVERLOADABLE tan( float x )
{
    return __spirv_ocl_tan( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( tan, float, float )

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE tan( double x )
{
    return __spirv_ocl_tan( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( tan, double, double )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half OVERLOADABLE tan( half x )
{
    return __spirv_ocl_tan( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( tan, half, half )

#endif // defined(cl_khr_fp16)

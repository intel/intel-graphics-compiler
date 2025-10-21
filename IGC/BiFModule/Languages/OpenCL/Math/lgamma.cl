/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE float OVERLOADABLE lgamma( float x )
{
    return __spirv_ocl_lgamma( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( lgamma, float, float )

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE lgamma( double x )
{
    return __spirv_ocl_lgamma( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( lgamma, double, double )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half OVERLOADABLE lgamma( half x )
{
    return __spirv_ocl_lgamma( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( lgamma, half, half )

#endif // defined(cl_khr_fp16)

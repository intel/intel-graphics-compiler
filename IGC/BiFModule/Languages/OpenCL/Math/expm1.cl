/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE float OVERLOADABLE expm1( float a )
{
    return __spirv_ocl_expm1( a );
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( expm1, float, float )

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE expm1( double x )
{
    return __spirv_ocl_expm1( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( expm1, double, double )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half OVERLOADABLE expm1( half x )
{
    return __spirv_ocl_expm1( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( expm1, half, half )

#endif // defined(cl_khr_fp16)

/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE float OVERLOADABLE atanpi( float x )
{
    return __spirv_ocl_atanpi( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( atanpi, float, float )

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE atanpi( double x )
{
    return __spirv_ocl_atanpi( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( atanpi, double, double )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half OVERLOADABLE atanpi( half x )
{
    return __spirv_ocl_atanpi( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( atanpi, half, half )

#endif // defined(cl_khr_fp16)

/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE float OVERLOADABLE asinpi( float x )
{
    return __spirv_ocl_asinpi( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( asinpi, float, float )

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE asinpi( double x )
{
    return __spirv_ocl_asinpi( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( asinpi, double, double )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half OVERLOADABLE asinpi( half x )
{
    return __spirv_ocl_asinpi( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( asinpi, half, half )

#endif // defined(cl_khr_fp16)

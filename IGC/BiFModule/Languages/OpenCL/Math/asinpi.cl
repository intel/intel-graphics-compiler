/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE float OVERLOADABLE asinpi( float x )
{
    return SPIRV_OCL_BUILTIN(asinpi, _f32, )( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( asinpi, float, float )

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE asinpi( double x )
{
    return SPIRV_OCL_BUILTIN(asinpi, _f64, )( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( asinpi, double, double )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half OVERLOADABLE asinpi( half x )
{
    return SPIRV_OCL_BUILTIN(asinpi, _f16, )( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( asinpi, half, half )

#endif // defined(cl_khr_fp16)

/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE float OVERLOADABLE acospi( float x )
{
    return SPIRV_OCL_BUILTIN(acospi, _f32, )( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( acospi, float, float )

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE acospi( double x )
{
    return SPIRV_OCL_BUILTIN(acospi, _f64, )( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( acospi, double, double )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half OVERLOADABLE acospi( half x )
{
    return SPIRV_OCL_BUILTIN(acospi, _f16, )( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( acospi, half, half )

#endif // defined(cl_khr_fp16)

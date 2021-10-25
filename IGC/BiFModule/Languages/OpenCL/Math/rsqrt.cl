/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE float OVERLOADABLE rsqrt( float x )
{
    return SPIRV_OCL_BUILTIN(rsqrt, _f32, )( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( rsqrt, float, float )

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE rsqrt( double x )
{
    return SPIRV_OCL_BUILTIN(rsqrt, _f64, )( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( rsqrt, double, double )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half OVERLOADABLE rsqrt( half x )
{
    return SPIRV_OCL_BUILTIN(rsqrt, _f16, )( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( rsqrt, half, half )

#endif // defined(cl_khr_fp16)

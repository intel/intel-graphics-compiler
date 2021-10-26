/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE float OVERLOADABLE native_exp( float x )
{
    return SPIRV_OCL_BUILTIN(native_exp, _f32, )( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( native_exp, float, float )

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE native_exp( double x )
{
    return SPIRV_OCL_BUILTIN(native_exp, _f64, )( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( native_exp, double, double )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half OVERLOADABLE native_exp( half x )
{
    return SPIRV_OCL_BUILTIN(native_exp, _f16, )( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( native_exp, half, half )

#endif // defined(cl_khr_fp16)

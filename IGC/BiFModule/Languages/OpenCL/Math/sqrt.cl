/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE float OVERLOADABLE sqrt( float x )
{
    return SPIRV_OCL_BUILTIN(sqrt, _f32, )( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( sqrt, float, float )

#ifdef cl_fp64_basic_ops

INLINE double OVERLOADABLE sqrt( double x )
{
    return SPIRV_OCL_BUILTIN(sqrt, _f64, )( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( sqrt, double, double )

#endif // cl_fp64_basic_ops

#ifdef cl_khr_fp16

INLINE half OVERLOADABLE sqrt( half x )
{
    return SPIRV_OCL_BUILTIN(sqrt, _f16, )( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( sqrt, half, half )

#endif // cl_khr_fp16

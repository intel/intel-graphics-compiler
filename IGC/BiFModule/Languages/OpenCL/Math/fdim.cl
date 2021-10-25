/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE float OVERLOADABLE fdim( float x, float y )
{
    return SPIRV_OCL_BUILTIN(fdim, _f32_f32, )( x, y );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( fdim, float, float )

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE fdim( double x, double y )
{
    return SPIRV_OCL_BUILTIN(fdim, _f64_f64, )( x, y );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( fdim, double, double )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half OVERLOADABLE fdim( half x, half y )
{
    return SPIRV_OCL_BUILTIN(fdim, _f16_f16, )( x, y );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( fdim, half, half )

#endif // defined(cl_khr_fp16)

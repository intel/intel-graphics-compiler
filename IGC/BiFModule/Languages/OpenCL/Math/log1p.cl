/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE float OVERLOADABLE log1p( float a )
{
    return SPIRV_OCL_BUILTIN(log1p, _f32, )( a );
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( log1p, float, float )

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE log1p( double a )
{
    return SPIRV_OCL_BUILTIN(log1p, _f64, )( a );
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( log1p, double, double )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half OVERLOADABLE log1p( half x )
{
    return SPIRV_OCL_BUILTIN(log1p, _f16, )( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( log1p, half, half )

#endif // defined(cl_khr_fp16)

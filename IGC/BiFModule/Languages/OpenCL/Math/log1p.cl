/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE float OVERLOADABLE log1p( float a )
{
    return __builtin_spirv_OpenCL_log1p_f32( a );
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( log1p, float, float )

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE log1p( double a )
{
    return __builtin_spirv_OpenCL_log1p_f64( a );
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( log1p, double, double )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half OVERLOADABLE log1p( half x )
{
    return __builtin_spirv_OpenCL_log1p_f16( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( log1p, half, half )

#endif // defined(cl_khr_fp16)

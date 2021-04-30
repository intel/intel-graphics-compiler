/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

float OVERLOADABLE erfc( float x )
{
    return __builtin_spirv_OpenCL_erfc_f32( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( erfc, float, float )

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE erfc( double x )
{
    return __builtin_spirv_OpenCL_erfc_f64( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( erfc, double, double )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half OVERLOADABLE erfc( half x )
{
    return __builtin_spirv_OpenCL_erfc_f16( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( erfc, half, half )

#endif // defined(cl_khr_fp16)

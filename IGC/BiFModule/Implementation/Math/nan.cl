/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float __attribute__((overloadable)) __spirv_ocl_nan( int nancode )
{
    return as_float( FLOAT_QUIET_NAN );
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( nan, float, int, i32 )

#if defined(cl_khr_fp64)

INLINE double __attribute__((overloadable)) __spirv_ocl_nan( long nancode )
{
    return as_double( DOUBLE_QUIET_NAN );
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( nan, double, long, i64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __attribute__((overloadable)) __spirv_ocl_nan( short nancode )
{
    return as_half( HALF_QUIET_NAN );
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( nan, half, short, i16 )

#endif // defined(cl_khr_fp16)

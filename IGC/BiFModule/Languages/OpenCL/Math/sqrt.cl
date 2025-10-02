/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE float OVERLOADABLE sqrt( float x )
{
    return __spirv_ocl_sqrt( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( sqrt, float, float )

#ifdef cl_fp64_basic_ops

INLINE double OVERLOADABLE sqrt( double x )
{
    return __spirv_ocl_sqrt( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( sqrt, double, double )

#endif // cl_fp64_basic_ops

#ifdef cl_khr_fp16

INLINE half OVERLOADABLE sqrt( half x )
{
    return __spirv_ocl_sqrt( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( sqrt, half, half )

#endif // cl_khr_fp16

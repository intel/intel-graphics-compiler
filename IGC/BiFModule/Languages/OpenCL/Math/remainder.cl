/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE float OVERLOADABLE remainder( float x, float y )
{
    return __spirv_ocl_remainder( x, y );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( remainder, float, float, float )

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE remainder( double x, double y )
{
    return __spirv_ocl_remainder( x, y );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( remainder, double, double, double )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half OVERLOADABLE remainder( half y, half x )
{
    return __spirv_ocl_remainder( y, x );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( remainder, half, half, half )

#endif // defined(cl_khr_fp16)

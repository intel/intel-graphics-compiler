/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE float OVERLOADABLE pow( float x, float y )
{
    return __spirv_ocl_pow( x, y );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( pow, float, float, float )

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE pow( double x, double y )
{
    return __spirv_ocl_pow( x, y );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( pow, double, double, double )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half OVERLOADABLE pow( half x, half y )
{
    return __spirv_ocl_pow( x, y );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( pow, half, half, half )

#endif // defined(cl_khr_fp16)

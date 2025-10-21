/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE float OVERLOADABLE native_divide( float x, float y )
{
    return __spirv_ocl_native_divide( x, y );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( native_divide, float, float )

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE native_divide( double x, double y )
{
    return __spirv_ocl_native_divide( x, y );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( native_divide, double, double )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half OVERLOADABLE native_divide( half x, half y )
{
    return __spirv_ocl_native_divide( x, y );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( native_divide, half, half )

#endif // defined(cl_khr_fp16)

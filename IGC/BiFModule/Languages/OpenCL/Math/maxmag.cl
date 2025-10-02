/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE float OVERLOADABLE maxmag( float x, float y )
{
    return __spirv_ocl_maxmag( x, y );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( maxmag, float, float )

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE maxmag( double x, double y )
{
    return __spirv_ocl_maxmag( x, y );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( maxmag, double, double )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half OVERLOADABLE maxmag( half x, half y )
{
    return __spirv_ocl_maxmag( x, y );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( maxmag, half, half )

#endif // defined(cl_khr_fp16)

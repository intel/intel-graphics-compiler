/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE float OVERLOADABLE copysign( float x, float y )
{
    return __spirv_ocl_copysign( x, y );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( copysign, float, float )

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE copysign( double x, double y )
{
    return __spirv_ocl_copysign( x, y );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( copysign, double, double )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half OVERLOADABLE copysign( half x, half y )
{
    return __spirv_ocl_copysign( x, y );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( copysign, half, half )

#endif // defined(cl_khr_fp16)

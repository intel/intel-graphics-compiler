/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE float OVERLOADABLE powr( float x, float y )
{
    return __spirv_ocl_powr( x, y );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( powr, float, float, float )

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE powr( double x, double y )
{
    return __spirv_ocl_powr( x, y );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( powr, double, double, double )

# endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half OVERLOADABLE powr( half x, half y )
{
    return __spirv_ocl_powr( x, y );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( powr, half, half, half )

#endif // defined(cl_khr_fp16)

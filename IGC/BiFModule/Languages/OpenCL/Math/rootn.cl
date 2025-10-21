/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

float OVERLOADABLE rootn( float x, int n )
{
    return __spirv_ocl_rootn( x, n );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( rootn, float, float, int )

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE rootn( double y, int x )
{
    return __spirv_ocl_rootn( y, x );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( rootn, double, double, int )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half OVERLOADABLE rootn( half y, int x )
{
    return __spirv_ocl_rootn( y, x );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( rootn, half, half, int )

#endif // defined(cl_khr_fp16)

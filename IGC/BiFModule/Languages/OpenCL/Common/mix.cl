/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE float OVERLOADABLE mix( float x, float y, float a )
{
    return __spirv_ocl_mix( x, y, a );
}

GENERATE_VECTOR_FUNCTIONS_3ARGS( mix, float, float )
GENERATE_VECTOR_FUNCTIONS_3ARGS_VVS( mix, float, float, float )

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE mix( double x, double y, double a )
{
    return __spirv_ocl_mix( x, y, a );
}

GENERATE_VECTOR_FUNCTIONS_3ARGS( mix, double, double )
GENERATE_VECTOR_FUNCTIONS_3ARGS_VVS( mix, double, double, double )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half OVERLOADABLE mix( half x, half y, half a )
{
    return __spirv_ocl_mix( x, y, a );
}

GENERATE_VECTOR_FUNCTIONS_3ARGS( mix, half, half )
GENERATE_VECTOR_FUNCTIONS_3ARGS_VVS( mix, half, half, half )

#endif // defined(cl_khr_fp16)

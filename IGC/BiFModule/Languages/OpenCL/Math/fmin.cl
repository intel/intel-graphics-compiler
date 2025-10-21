/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE float OVERLOADABLE fmin( float x, float y )
{
    return __spirv_ocl_fmin( x, y );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( fmin, float, float )
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS( fmin, float, float, float )

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE fmin( double x, double y )
{
    return __spirv_ocl_fmin( x, y );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( fmin, double, double )
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS( fmin, double, double, double )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half OVERLOADABLE fmin( half x, half y )
{
    return __spirv_ocl_fmin( x, y );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( fmin, half, half )
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS( fmin, half, half, half )

#endif // defined(cl_khr_fp16)

/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE float OVERLOADABLE fma( float a, float b, float c )
{
    return __spirv_ocl_fma( a, b, c );
}

GENERATE_VECTOR_FUNCTIONS_3ARGS( fma, float, float )

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE fma( double a, double b, double c )
{
    return __spirv_ocl_fma( a, b, c );
}

GENERATE_VECTOR_FUNCTIONS_3ARGS( fma, double, double )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

GENERATE_VECTOR_FUNCTIONS_3ARGS( fma, half, half )

#endif // defined(cl_khr_fp16)

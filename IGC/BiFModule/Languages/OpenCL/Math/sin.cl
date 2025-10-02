/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE float OVERLOADABLE sin( float x )
{
    return __spirv_ocl_sin( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( sin, float, float )

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE sin( double x )
{
    return __spirv_ocl_sin( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( sin, double, double )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half OVERLOADABLE sin( half x )
{
    return __spirv_ocl_sin( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( sin, half, half )

#endif // defined(cl_khr_fp16)

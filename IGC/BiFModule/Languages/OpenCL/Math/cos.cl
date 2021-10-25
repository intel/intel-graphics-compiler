/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE float OVERLOADABLE cos( float x )
{
    return SPIRV_OCL_BUILTIN(cos, _f32, )( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( cos, float, float )

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE cos( double x )
{
    return SPIRV_OCL_BUILTIN(cos, _f64, )( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( cos, double, double )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half OVERLOADABLE cos( half x )
{
    return SPIRV_OCL_BUILTIN(cos, _f16, )( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( cos, half, half )

#endif // defined(cl_khr_fp16)

/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE float OVERLOADABLE step( float edge, float x )
{
    return SPIRV_OCL_BUILTIN(step, _f32_f32, )( edge, x );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( step, float, float )
GENERATE_VECTOR_FUNCTIONS_2ARGS_SV( step, float, float, float )

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE step( double edge, double x )
{
    return SPIRV_OCL_BUILTIN(step, _f64_f64, )( edge, x );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( step, double, double )
GENERATE_VECTOR_FUNCTIONS_2ARGS_SV( step, double, double, double )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half OVERLOADABLE step( half edge, half x )
{
    return SPIRV_OCL_BUILTIN(step, _f16_f16, )( edge, x );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( step, half, half )
GENERATE_VECTOR_FUNCTIONS_2ARGS_SV( step, half, half, half )

#endif // defined(cl_khr_fp16)

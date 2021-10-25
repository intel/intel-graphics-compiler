/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

float OVERLOADABLE hypot( float x, float y )
{
    return SPIRV_OCL_BUILTIN(hypot, _f32_f32, )( x, y );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( hypot, float, float, float )

#if defined(cl_khr_fp64)

double OVERLOADABLE hypot( double x, double y )
{
    return SPIRV_OCL_BUILTIN(hypot, _f64_f64, )( x, y );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( hypot, double, double, double )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

half OVERLOADABLE hypot( half x, half y )
{
    // __builtin_spirv_OpenCL_hypot_f16_f16 is not precise enough, so we use f32
    return SPIRV_OCL_BUILTIN(hypot, _f16_f16, )( x, y );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( hypot, half, half, half )

#endif // defined(cl_khr_fp16)

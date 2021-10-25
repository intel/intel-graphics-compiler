/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

// TODO: I think we should be able to use M_PI_F here instead of FLOAT_PI,
// but this does cause very small differences in the results of atan2(),
// since M_PI_F is rounded (and therefore slightly larger than FLOAT_PI).
// We'll need to re-collect some GITS streams if we want to use M_PI_F
// instead.
#define FLOAT_PI                (as_float(0x40490FDA)) // 3.1415926535897930f

float OVERLOADABLE atan2( float y, float x )
{
    return SPIRV_OCL_BUILTIN(atan2, _f32_f32, )( y, x );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( atan2, float, float, float )

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE atan2( double y, double x )
{
    return SPIRV_OCL_BUILTIN(atan2, _f64_f64, )( y, x );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( atan2, double, double, double )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half OVERLOADABLE atan2( half y, half x )
{
    return SPIRV_OCL_BUILTIN(atan2, _f16_f16, )( y, x );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( atan2, half, half, half )

#endif // defined(cl_khr_fp16)

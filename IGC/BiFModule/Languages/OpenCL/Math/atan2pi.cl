/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE float OVERLOADABLE atan2pi( float x, float y )
{
    return SPIRV_OCL_BUILTIN(atan2pi, _f32_f32, )( x, y );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( atan2pi, float, float )

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE atan2pi( double x, double y )
{
    return SPIRV_OCL_BUILTIN(atan2pi, _f64_f64, )( x, y );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( atan2pi, double, double )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half OVERLOADABLE atan2pi( half x, half y )
{
    return SPIRV_OCL_BUILTIN(atan2pi, _f16_f16, )( x, y );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( atan2pi, half, half )

#endif // defined(cl_khr_fp16)

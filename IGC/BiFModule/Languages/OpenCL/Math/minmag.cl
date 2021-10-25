/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE float OVERLOADABLE minmag( float x, float y )
{
    return SPIRV_OCL_BUILTIN(minmag, _f32_f32, )( x, y );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( minmag, float, float )

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE minmag( double x, double y )
{
    return SPIRV_OCL_BUILTIN(minmag, _f64_f64, )( x, y );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( minmag, double, double )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half OVERLOADABLE minmag( half x, half y )
{
    return SPIRV_OCL_BUILTIN(minmag, _f16_f16, )( x, y );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( minmag, half, half )

#endif // defined(cl_khr_fp16)

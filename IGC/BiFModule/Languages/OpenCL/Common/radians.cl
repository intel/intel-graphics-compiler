/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE float OVERLOADABLE radians( float d )
{
    return SPIRV_OCL_BUILTIN(radians, _f32, )( d );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( radians, float, float )

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE radians( double d )
{
    return SPIRV_OCL_BUILTIN(radians, _f64, )( d );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( radians, double, double )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half OVERLOADABLE radians( half d )
{
    return SPIRV_OCL_BUILTIN(radians, _f16, )( d );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( radians, half, half )

#endif // defined(cl_khr_fp16)

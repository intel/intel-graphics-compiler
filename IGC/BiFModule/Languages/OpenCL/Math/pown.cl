/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE float OVERLOADABLE pown( float x, int y )
{
    return SPIRV_OCL_BUILTIN(pown, _f32_i32, )( x, y );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( pown, float, float, int )

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE pown( double x, int y )
{
    return SPIRV_OCL_BUILTIN(pown, _f64_i32, )( x, y );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( pown, double, double, int )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half OVERLOADABLE pown( half x, int y )
{
    return SPIRV_OCL_BUILTIN(pown, _f16_i32, )( x, y );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( pown, half, half, int )

#endif // defined(cl_khr_fp16)

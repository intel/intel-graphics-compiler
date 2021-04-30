/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE int OVERLOADABLE isunordered( float x, float y )
{
    return SPIRV_BUILTIN(Unordered, _f32_f32, )( x, y );
}

static INLINE int OVERLOADABLE __intel_vector_isunordered_helper( float x, float y )
{
    return (x != x) | (y != y) ? -1 : 0;
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_EXPLICIT( isunordered, __intel_vector_isunordered_helper, int, float )

#if defined(cl_khr_fp64)

INLINE int OVERLOADABLE isunordered( double x, double y )
{
    return SPIRV_BUILTIN(Unordered, _f64_f64, )( x, y );
}

static INLINE long OVERLOADABLE __intel_vector_isunordered_helper( double x, double y )
{
    return (x != x) | (y != y) ? -1 : 0;
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_EXPLICIT( isunordered, __intel_vector_isunordered_helper, long, double )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE int OVERLOADABLE isunordered( half x, half y )
{
    return SPIRV_BUILTIN(Unordered, _f16_f16, )( x, y );
}

static INLINE short OVERLOADABLE __intel_vector_isunordered_helper( half x, half y )
{
    return (x != x) | (y != y) ? -1 : 0;
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_EXPLICIT( isunordered, __intel_vector_isunordered_helper, short, half )

#endif // defined(cl_khr_fp16)

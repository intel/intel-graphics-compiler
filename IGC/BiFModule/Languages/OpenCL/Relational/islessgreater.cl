/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE int OVERLOADABLE islessgreater( float x, float y )
{
    return SPIRV_BUILTIN(LessOrGreater, _f32_f32, )( x, y );
}

static INLINE int OVERLOADABLE __intel_vector_islessgreater_helper( float x, float y )
{
    return (x < y) | (x > y) ? -1 : 0;
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_EXPLICIT( islessgreater, __intel_vector_islessgreater_helper, int, float )

#if defined(cl_khr_fp64)

INLINE int OVERLOADABLE islessgreater( double x, double y )
{
    return SPIRV_BUILTIN(LessOrGreater, _f64_f64, )( x, y );
}

static INLINE long OVERLOADABLE __intel_vector_islessgreater_helper( double x, double y )
{
    return (x < y) | (x > y) ? -1 : 0;
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_EXPLICIT( islessgreater, __intel_vector_islessgreater_helper, long, double )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE int OVERLOADABLE islessgreater( half x, half y )
{
    return SPIRV_BUILTIN(LessOrGreater, _f16_f16, )( x, y );
}

static INLINE short OVERLOADABLE __intel_vector_islessgreater_helper( half x, half y )
{
    return (x < y) | (x > y) ? -1 : 0;
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_EXPLICIT( islessgreater, __intel_vector_islessgreater_helper, short, half )

#endif // defined(cl_khr_fp16)

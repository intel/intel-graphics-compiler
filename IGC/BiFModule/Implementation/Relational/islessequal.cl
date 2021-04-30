/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"

INLINE int OVERLOADABLE islessequal( float x, float y )
{
    return (x <= y) ? 1 : 0;
}

static INLINE int OVERLOADABLE __intel_vector_islessequal_helper( float x, float y )
{
    return (x <= y) ? -1 : 0;
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_EXPLICIT( islessequal, __intel_vector_islessequal_helper, int, float )

#if defined(cl_khr_fp64)

INLINE int OVERLOADABLE islessequal( double x, double y )
{
    return (x <= y) ? 1 : 0;
}

static INLINE long OVERLOADABLE __intel_vector_islessequal_helper( double x, double y )
{
    return (x <= y) ? -1 : 0;
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_EXPLICIT( islessequal, __intel_vector_islessequal_helper, long, double )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE int OVERLOADABLE islessequal( half x, half y )
{
    return (x <= y) ? 1 : 0;
}

static INLINE short OVERLOADABLE __intel_vector_islessequal_helper( half x, half y )
{
    return (x <= y) ? -1 : 0;
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_EXPLICIT( islessequal, __intel_vector_islessequal_helper, short, half )

#endif // defined(cl_khr_fp16)

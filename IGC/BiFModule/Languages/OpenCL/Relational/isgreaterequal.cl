/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE int OVERLOADABLE isgreaterequal( float x, float y )
{
    return (x >= y) ? 1 : 0;
}

static INLINE int OVERLOADABLE __intel_vector_isgreaterequal_helper( float x, float y )
{
    return (x >= y) ? -1 : 0;
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_EXPLICIT( isgreaterequal, __intel_vector_isgreaterequal_helper, int, float )

#if defined(cl_khr_fp64)

INLINE int OVERLOADABLE isgreaterequal( double x, double y )
{
    return (x >= y) ? 1 : 0;
}

static INLINE long OVERLOADABLE __intel_vector_isgreaterequal_helper( double x, double y )
{
    return (x >= y) ? -1 : 0;
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_EXPLICIT( isgreaterequal, __intel_vector_isgreaterequal_helper, long, double )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE int OVERLOADABLE isgreaterequal( half x, half y )
{
    return (x >= y) ? 1 : 0;
}

static INLINE short OVERLOADABLE __intel_vector_isgreaterequal_helper( half x, half y )
{
    return (x >= y) ? -1 : 0;
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_EXPLICIT( isgreaterequal, __intel_vector_isgreaterequal_helper, short, half )

#endif // defined(cl_khr_fp16)

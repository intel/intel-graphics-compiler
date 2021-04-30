/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE int OVERLOADABLE signbit( float x )
{
    return as_int( x ) & FLOAT_SIGN_MASK ? 1 : 0;
}

static INLINE int OVERLOADABLE __intel_vector_signbit_helper( float x )
{
    return as_int( x ) & FLOAT_SIGN_MASK ? -1 : 0;
}

GENERATE_VECTOR_FUNCTIONS_1ARG_EXPLICIT( signbit, __intel_vector_signbit_helper, int, float )

#if defined(cl_khr_fp64)

INLINE int OVERLOADABLE signbit( double x )
{
    return as_long( x ) & DOUBLE_SIGN_MASK ? 1 : 0;
}

static INLINE long OVERLOADABLE __intel_vector_signbit_helper( double x )
{
    return as_long( x ) & DOUBLE_SIGN_MASK ? -1 : 0;
}

GENERATE_VECTOR_FUNCTIONS_1ARG_EXPLICIT( signbit, __intel_vector_signbit_helper, long, double )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE int OVERLOADABLE signbit( half x )
{
    return as_short( x ) & HALF_SIGN_MASK ? 1 : 0;
}

static INLINE short OVERLOADABLE __intel_vector_signbit_helper( half x )
{
    return as_short( x ) & HALF_SIGN_MASK ? -1 : 0;
}

GENERATE_VECTOR_FUNCTIONS_1ARG_EXPLICIT( signbit, __intel_vector_signbit_helper, short, half )

#endif // defined(cl_khr_fp16)

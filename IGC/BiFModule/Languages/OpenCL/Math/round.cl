/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE INLINE float OVERLOADABLE round( float x )
{
    return SPIRV_OCL_BUILTIN(round, _f32, )( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( round, float, float )

#if defined(cl_khr_fp64)

INLINE INLINE double OVERLOADABLE round( double x )
{
    return SPIRV_OCL_BUILTIN(round, _f64, )( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( round, double, double )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE INLINE half OVERLOADABLE round( half x )
{
    return SPIRV_OCL_BUILTIN(round, _f16, )( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( round, half, half )

#endif // defined(cl_khr_fp16)

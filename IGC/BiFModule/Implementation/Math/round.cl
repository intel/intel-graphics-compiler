/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE INLINE float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(round, _f32, )( float x )
{
    float delta = as_float(0x3EFFFFFF); // one bit less than 0.5f
    float nd = x - delta;
    float pd = x + delta;
    x = ( x < 0 ) ? nd : pd;
    return SPIRV_OCL_BUILTIN(trunc, _f32, )(x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( round, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE INLINE double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(round, _f64, )( double x )
{
    double delta = as_double(0x3FDFFFFFFFFFFFFFl);   // one bit less than 0.5
    double nd = x - delta;
    double pd = x + delta;
    x = ( x < 0 ) ? nd : pd;
    return SPIRV_OCL_BUILTIN(trunc, _f64, )(x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( round, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE INLINE half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(round, _f16, )( half x )
{
    return SPIRV_OCL_BUILTIN(round, _f32, )((float)x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( round, half, half, f16 )

#endif // defined(cl_khr_fp16)

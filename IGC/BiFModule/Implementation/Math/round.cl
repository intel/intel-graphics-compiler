/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE INLINE float __attribute__((overloadable)) __spirv_ocl_round( float x )
{
    float delta = as_float(0x3EFFFFFF); // one bit less than 0.5f
    float nd = x - delta;
    float pd = x + delta;
    x = ( x < 0 ) ? nd : pd;
    return __spirv_ocl_trunc(x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( round, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE INLINE double __attribute__((overloadable)) __spirv_ocl_round( double x )
{
    double delta = as_double(0x3FDFFFFFFFFFFFFFl);   // one bit less than 0.5
    double nd = x - delta;
    double pd = x + delta;
    x = ( x < 0 ) ? nd : pd;
    return __spirv_ocl_trunc(x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( round, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE INLINE half __attribute__((overloadable)) __spirv_ocl_round( half x )
{
    return __spirv_ocl_round((float)x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( round, half, half, f16 )

#endif // defined(cl_khr_fp16)


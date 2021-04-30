/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE INLINE float __builtin_spirv_OpenCL_round_f32( float x )
{
    float delta = as_float(0x3EFFFFFF); // one bit less than 0.5f
    float nd = x - delta;
    float pd = x + delta;
    x = ( x < 0 ) ? nd : pd;
    return __builtin_spirv_OpenCL_trunc_f32(x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_round, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE INLINE double __builtin_spirv_OpenCL_round_f64( double x )
{
    double delta = as_double(0x3FDFFFFFFFFFFFFFl);   // one bit less than 0.5
    double nd = x - delta;
    double pd = x + delta;
    x = ( x < 0 ) ? nd : pd;
    return __builtin_spirv_OpenCL_trunc_f64(x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_round, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE INLINE half __builtin_spirv_OpenCL_round_f16( half x )
{
    return __builtin_spirv_OpenCL_round_f32(x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_round, half, half, f16 )

#endif // defined(cl_khr_fp16)

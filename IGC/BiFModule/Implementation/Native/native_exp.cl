/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float __builtin_spirv_OpenCL_native_exp_f32( float x )
{
    return __builtin_spirv_OpenCL_native_exp2_f32(x * M_LOG2E_F);
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_native_exp, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __builtin_spirv_OpenCL_native_exp_f64( double x )
{
    return __builtin_spirv_OpenCL_native_exp2_f32((float)x * M_LOG2E_F);
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_native_exp, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_native_exp_f16( half x )
{
    return __builtin_spirv_OpenCL_native_exp2_f16(x * M_LOG2E_H);
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_native_exp, half, half, f16 )

#endif // defined(cl_khr_fp16)

/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float __attribute__((overloadable)) __spirv_ocl_native_exp( float x )
{
    return __spirv_ocl_native_exp2(x * M_LOG2E_F);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( native_exp, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __attribute__((overloadable)) __spirv_ocl_native_exp( double x )
{
    return __spirv_ocl_native_exp2((float)x * M_LOG2E_F);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( native_exp, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __attribute__((overloadable)) __spirv_ocl_native_exp( half x )
{
    return __spirv_ocl_native_exp2(x * M_LOG2E_H);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( native_exp, half, half, f16 )

#endif // defined(cl_khr_fp16)


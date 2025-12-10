/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( native_cos, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __attribute__((overloadable)) __spirv_ocl_native_cos( double x )
{
    float f = (float)x;
    return __spirv_ocl_native_cos(f);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( native_cos, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( native_cos, half, half, f16 )

#endif // defined(cl_khr_fp16)

#if defined(IGC_SPV_INTEL_bfloat16_arithmetic)
INLINE bfloat __attribute__((overloadable)) __spirv_ocl_native_cos( bfloat x )
{
    return as_bfloat(__builtin_bf16_cos(as_ushort(x)));
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( native_cos, bfloat, bfloat, )
#endif // defined(IGC_SPV_INTEL_bfloat16_arithmetic)

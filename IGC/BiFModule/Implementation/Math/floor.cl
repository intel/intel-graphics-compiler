/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( floor, float, float, f32 )

#if defined(cl_khr_fp64)

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( floor, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( floor, half, half, f16 )

#endif // defined(cl_khr_fp16)

#if defined(IGC_SPV_INTEL_bfloat16_arithmetic)
INLINE bfloat __attribute__((overloadable)) __spirv_ocl_floor( bfloat x )
{
    return __spirv_ocl_floor((float)x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( floor, bfloat, bfloat, )
#endif // defined(IGC_SPV_INTEL_bfloat16_arithmetic)

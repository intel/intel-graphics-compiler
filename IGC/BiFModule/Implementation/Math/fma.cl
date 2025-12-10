/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_3ARGS( fma, float, float, f32 )

#if defined(cl_khr_fp64)

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_3ARGS( fma, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_3ARGS( fma, half, half, f16 )

#endif // defined(cl_khr_fp16)

#if defined(IGC_SPV_INTEL_bfloat16_arithmetic)
INLINE bfloat __attribute__((overloadable)) __spirv_ocl_fma( bfloat a, bfloat b, bfloat c )
{
    return as_bfloat(__builtin_bf16_mad(as_ushort(a), as_ushort(b), as_ushort(c)));
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_3ARGS( fma, bfloat, bfloat, )
#endif // defined(IGC_SPV_INTEL_bfloat16_arithmetic)

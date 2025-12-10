/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

float __attribute__((overloadable)) __spirv_ocl_sigm( float x )
{
    float result;

    if( __intel_relaxed_isnan(x) )
    {
        result = __spirv_ocl_nan(0);
    }
    else if(BIF_FLAG_CTRL_GET(HasNativeSigm))
    {
        result = __builtin_IB_sigmf(x);
    }
    else
    {
        float nexp = __intel_exp_for_tanh( -x, 0.0f);
        result = 1 / (1.0f + nexp);
    }

    return result;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARG_LOOP( sigm, float, float, f32 )

#if defined(cl_khr_fp16)

INLINE half __attribute__((overloadable)) __spirv_ocl_sigm( half x )
{
    if(BIF_FLAG_CTRL_GET(HasNativeSigm))
    {
        return __builtin_IB_sigmh(x);
    }
    return __spirv_ocl_sigm((float)x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARG_LOOP( sigm, half, half, f16 )

#endif // defined(cl_khr_fp16)


// SPV_INTEL_sigmoid extension
float __attribute__((overloadable)) __spirv_FSigmoidINTEL( float x )
{
    return __spirv_ocl_sigm(x);
}
SPIRV_GENERATE_VECTOR_FUNCTIONS_1ARG( FSigmoidINTEL, float, float, f32 )

#if defined(cl_khr_fp16)
half __attribute__((overloadable)) __spirv_FSigmoidINTEL( half x )
{
    return __spirv_ocl_sigm(x);
}
SPIRV_GENERATE_VECTOR_FUNCTIONS_1ARG( FSigmoidINTEL, half, half, f16 )
#endif // defined(cl_khr_fp16)

// bfloat16 - short is used to represent bfloat16, it is changed later
// in BfloatBuiltinsResolution to bfloat type.

ushort __builtin_bf16_sigm(ushort) __attribute__((overloadable));

short __attribute__((overloadable)) __spirv_FSigmoidINTEL( short x )
{
    if(BIF_FLAG_CTRL_GET(HasNativeSigm))
    {
        return __builtin_bf16_sigm(x);
    }
    return __spirv_ocl_sigm(__builtin_IB_bftof_1(x));
}
SPIRV_GENERATE_VECTOR_FUNCTIONS_1ARG( FSigmoidINTEL, short, short, bf16 )

/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

// This is a workaround for unexpected behaviour of SPIRV-LLVM Translator:
// OpExtInst %float fmax_common  -->  _Z11fmax_commonff
// Whereas expected (clang-consistent) behaviour is:
// OpExtInst %float fmax_common  -->  _Z3maxff
// It doesn't affect functionallity, it's just naming matter.

INLINE float OVERLOADABLE fmax_common(float x, float y)
{
    return SPIRV_OCL_BUILTIN(fmax_common, _f32_f32, )(x, y);
}

GENERATE_VECTOR_FUNCTIONS_2ARGS(fmax_common, float, float)
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS(fmax_common, float, float, float)

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE fmax_common(double x, double y)
{
    return SPIRV_OCL_BUILTIN(fmax_common, _f64_f64, )(x, y);
}

GENERATE_VECTOR_FUNCTIONS_2ARGS(fmax_common, double, double)
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS(fmax_common, double, double, double)

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half OVERLOADABLE fmax_common(half x, half y)
{
    return SPIRV_OCL_BUILTIN(fmax_common, _f16_f16, )(x, y);
}

GENERATE_VECTOR_FUNCTIONS_2ARGS(fmax_common, half, half)
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS(fmax_common, half, half, half)

#endif // defined(cl_khr_fp16)

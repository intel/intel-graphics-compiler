/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_exp, _f32, )( float x )
{
    return SPIRV_OCL_BUILTIN(native_exp2, _f32, )(x * M_LOG2E_F);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( native_exp, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_exp, _f64, )( double x )
{
    return SPIRV_OCL_BUILTIN(native_exp2, _f32, )((float)x * M_LOG2E_F);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( native_exp, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_exp, _f16, )( half x )
{
    return SPIRV_OCL_BUILTIN(native_exp2, _f16, )(x * M_LOG2E_H);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( native_exp, half, half, f16 )

#endif // defined(cl_khr_fp16)

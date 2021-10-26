/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sqrt, _f32, )( float x )
{
    return SPIRV_OCL_BUILTIN(native_sqrt, _f32, )(x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( sqrt, float, float, f32 )

#ifdef cl_fp64_basic_ops

INLINE double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sqrt, _f64, )( double x )
{
    return SPIRV_OCL_BUILTIN(native_sqrt, _f64, )(x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( sqrt, double, double, f64 )

#endif // cl_fp64_basic_ops

#if defined(cl_khr_fp16)

INLINE half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sqrt, _f16, )( half x )
{
    return SPIRV_OCL_BUILTIN(native_sqrt, _f32, )((float)x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( sqrt, half, half, f16 )

#endif // defined(cl_khr_fp16)

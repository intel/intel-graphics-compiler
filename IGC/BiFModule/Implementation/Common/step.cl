/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(step, _f32_f32, )(float edge, float x ){
    return x < edge ? 0.0f : 1.0f;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS( step, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(step, _f64_f64, )(double edge, double x ){
    return x < edge ? 0.0 : 1.0;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS( step, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(step, _f16_f16, )(half edge, half x ){
    return x < edge ? (half)0.0f : (half)1.0f;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS( step, half, half, f16 )

#endif // defined(cl_khr_fp16)

/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( native_cos, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_cos, _f64, )( double x )
{
    float f = (float)x;
    return SPIRV_OCL_BUILTIN(native_cos, _f32, )(f);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( native_cos, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( native_cos, half, half, f16 )

#endif // defined(cl_khr_fp16)

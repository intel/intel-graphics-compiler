/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( native_log2, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_log2, _f64, )( double x )
{
    float f = (float)x;
    return SPIRV_OCL_BUILTIN(native_log2, _f32, )(f);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( native_log2, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( native_log2, half, half, f16 )

#endif // defined(cl_khr_fp16)

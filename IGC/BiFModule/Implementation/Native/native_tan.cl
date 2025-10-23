/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( native_tan, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __attribute__((overloadable)) __spirv_ocl_native_tan( double x )
{
    return __spirv_ocl_native_tan((float)x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( native_tan, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( native_tan, half, half, f16 )

#endif // defined(cl_khr_fp16)

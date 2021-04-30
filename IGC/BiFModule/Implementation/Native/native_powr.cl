/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

GENERATE_VECTOR_FUNCTIONS_2ARGS( __builtin_spirv_OpenCL_native_powr, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __builtin_spirv_OpenCL_native_powr_f64_f64( double x, double y )
{
    return __builtin_spirv_OpenCL_native_powr_f32_f32((float)x, (float)y);
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( __builtin_spirv_OpenCL_native_powr, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

GENERATE_VECTOR_FUNCTIONS_2ARGS( __builtin_spirv_OpenCL_native_powr, half, half, f16 )

#endif // defined(cl_khr_fp16)

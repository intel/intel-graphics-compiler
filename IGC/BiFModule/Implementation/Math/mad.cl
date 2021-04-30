/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float __builtin_spirv_OpenCL_mad_f32_f32_f32( float a, float b, float c )
{
    return __builtin_spirv_OpenCL_fma_f32_f32_f32(a,b,c);
}

GENERATE_VECTOR_FUNCTIONS_3ARGS( __builtin_spirv_OpenCL_mad, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __builtin_spirv_OpenCL_mad_f64_f64_f64( double a, double b, double c )
{
    return __builtin_spirv_OpenCL_fma_f64_f64_f64(a,b,c);
}

GENERATE_VECTOR_FUNCTIONS_3ARGS( __builtin_spirv_OpenCL_mad, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

GENERATE_VECTOR_FUNCTIONS_3ARGS( __builtin_spirv_OpenCL_mad, half, half, f16 )

#endif // defined(cl_khr_fp16)

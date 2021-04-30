/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float __builtin_spirv_OpenCL_smoothstep_f32_f32_f32(float edge0, float edge1, float x ){
    float div = (x - edge0) / (edge1 - edge0);
    float temp = __builtin_spirv_OpenCL_fclamp_f32_f32_f32( div, 0.0f, 1.0f );
    return temp * temp * __builtin_spirv_OpenCL_mad_f32_f32_f32( -2.0f, temp, 3.0f );
}

GENERATE_VECTOR_FUNCTIONS_3ARGS( __builtin_spirv_OpenCL_smoothstep, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __builtin_spirv_OpenCL_smoothstep_f64_f64_f64(double edge0, double edge1, double x ){
    double div = (x - edge0) / (edge1 - edge0);
    double temp = __builtin_spirv_OpenCL_fclamp_f64_f64_f64( div, 0.0, 1.0 );
    return temp * temp * __builtin_spirv_OpenCL_mad_f64_f64_f64( -2.0, temp, 3.0 );
}

GENERATE_VECTOR_FUNCTIONS_3ARGS( __builtin_spirv_OpenCL_smoothstep, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_smoothstep_f16_f16_f16(half edge0, half edge1, half x ){
    half div = (x - edge0) / (edge1 - edge0);
    half temp = __builtin_spirv_OpenCL_fclamp_f16_f16_f16( div, (half)0.0f, (half)1.0f );
    return temp * temp * __builtin_spirv_OpenCL_mad_f16_f16_f16( (half)-2.0f, temp, (half)3.0f );
}

GENERATE_VECTOR_FUNCTIONS_3ARGS( __builtin_spirv_OpenCL_smoothstep, half, half, f16 )

#endif // defined(cl_khr_fp16)

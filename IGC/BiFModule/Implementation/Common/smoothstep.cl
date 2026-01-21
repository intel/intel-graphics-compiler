/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float __attribute__((overloadable)) __spirv_ocl_smoothstep(float edge0, float edge1, float x ){
    float div = (x - edge0) / (edge1 - edge0);
    float temp = __spirv_ocl_fclamp( div, 0.0f, 1.0f );
    return temp * temp * __spirv_ocl_mad( -2.0f, temp, 3.0f );
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_3ARGS( smoothstep, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __attribute__((overloadable)) __spirv_ocl_smoothstep(double edge0, double edge1, double x ){
    double div = (x - edge0) / (edge1 - edge0);
    double temp = __spirv_ocl_fclamp( div, 0.0, 1.0 );
    return temp * temp * __spirv_ocl_mad( -2.0, temp, 3.0 );
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_3ARGS( smoothstep, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __attribute__((overloadable)) __spirv_ocl_smoothstep(half edge0, half edge1, half x ){
    half div = (x - edge0) / (edge1 - edge0);
    half temp = __spirv_ocl_fclamp( div, (half)0.0f, (half)1.0f );
    return temp * temp * __spirv_ocl_mad( (half)-2.0f, temp, (half)3.0f );
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_3ARGS( smoothstep, half, half, f16 )

#endif // defined(cl_khr_fp16)


/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

float __builtin_spirv_OpenCL_degrees_f32(float r ){
    return ONE_EIGHTY_OVER_PI_FLT * r;
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_degrees, float, float, f32 )

#if defined(cl_khr_fp64)

double __builtin_spirv_OpenCL_degrees_f64(double r ){
    return ONE_EIGHTY_OVER_PI_DBL * r;
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_degrees, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

half __builtin_spirv_OpenCL_degrees_f16(half r ){
    return ONE_EIGHTY_OVER_PI_HLF * r;
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_degrees, half, half, f16 )

#endif // defined(cl_khr_fp16)

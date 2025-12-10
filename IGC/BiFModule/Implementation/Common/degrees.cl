/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

float __attribute__((overloadable)) __spirv_ocl_degrees(float r ){
    return ONE_EIGHTY_OVER_PI_FLT * r;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( degrees, float, float, f32 )

#if defined(cl_khr_fp64)

double __attribute__((overloadable)) __spirv_ocl_degrees(double r ){
    return ONE_EIGHTY_OVER_PI_DBL * r;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( degrees, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

half __attribute__((overloadable)) __spirv_ocl_degrees(half r ){
    return ONE_EIGHTY_OVER_PI_HLF * r;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( degrees, half, half, f16 )

#endif // defined(cl_khr_fp16)

#if defined(IGC_SPV_INTEL_bfloat16_arithmetic)
bfloat __attribute__((overloadable)) __spirv_ocl_degrees(bfloat r ){
    return as_bfloat(__builtin_bf16_mul(as_ushort(r), as_ushort((bfloat)ONE_EIGHTY_OVER_PI_FLT)));
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( degrees, bfloat, bfloat, )
#endif // defined(IGC_SPV_INTEL_bfloat16_arithmetic)

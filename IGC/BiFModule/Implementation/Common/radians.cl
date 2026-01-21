/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float __attribute__((overloadable)) __spirv_ocl_radians(float d ){
    return PI_OVER_ONE_EIGHTY_FLT * d;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( radians, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __attribute__((overloadable)) __spirv_ocl_radians(double d ){
    return PI_OVER_ONE_EIGHTY_DBL * d;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( radians, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __attribute__((overloadable)) __spirv_ocl_radians(half d ){
    return PI_OVER_ONE_EIGHTY_HLF * d;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( radians, half, half, f16 )

#endif // defined(cl_khr_fp16)


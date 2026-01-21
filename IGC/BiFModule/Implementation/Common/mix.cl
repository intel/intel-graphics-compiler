/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float __attribute__((overloadable)) __spirv_ocl_mix(float x, float y, float a ){
    return __spirv_ocl_mad( ( y - x ), a, x );
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_3ARGS( mix, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __attribute__((overloadable)) __spirv_ocl_mix(double x, double y, double a ){
    return __spirv_ocl_mad( ( y - x ), a, x );
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_3ARGS( mix, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __attribute__((overloadable)) __spirv_ocl_mix(half x, half y, half a ){
    return __spirv_ocl_mad( ( y - x ), a, x );
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_3ARGS( mix, half, half, f16 )

#endif // defined(cl_khr_fp16)


/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float __builtin_spirv_OpenCL_mix_f32_f32_f32(float x, float y, float a ){
    return __builtin_spirv_OpenCL_mad_f32_f32_f32( ( y - x ), a, x );
}

GENERATE_VECTOR_FUNCTIONS_3ARGS( __builtin_spirv_OpenCL_mix, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __builtin_spirv_OpenCL_mix_f64_f64_f64(double x, double y, double a ){
    return __builtin_spirv_OpenCL_mad_f64_f64_f64( ( y - x ), a, x );
}

GENERATE_VECTOR_FUNCTIONS_3ARGS( __builtin_spirv_OpenCL_mix, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_mix_f16_f16_f16(half x, half y, half a ){
    return __builtin_spirv_OpenCL_mad_f16_f16_f16( ( y - x ), a, x );
}

GENERATE_VECTOR_FUNCTIONS_3ARGS( __builtin_spirv_OpenCL_mix, half, half, f16 )

#endif // defined(cl_khr_fp16)

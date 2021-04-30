/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float __builtin_spirv_OpenCL_fmax_common_f32_f32(float x, float y) {
    return __builtin_IB_fmax(x, y);
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( __builtin_spirv_OpenCL_fmax_common, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __builtin_spirv_OpenCL_fmax_common_f64_f64(double x, double y) {
    return __builtin_IB_dmax(x, y);
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( __builtin_spirv_OpenCL_fmax_common, double, double, f64 )

#endif // defined(cl_khr_fp64)

#ifdef cl_khr_fp16

INLINE half __builtin_spirv_OpenCL_fmax_common_f16_f16(half x, half y) {
    return __builtin_IB_HMAX(x, y);
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( __builtin_spirv_OpenCL_fmax_common, half, half, f16 )

#endif // defined(cl_khr_fp16)

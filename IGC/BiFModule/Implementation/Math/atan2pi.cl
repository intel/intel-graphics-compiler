/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float __builtin_spirv_OpenCL_atan2pi_f32_f32( float x, float y )
{
    return M_1_PI_F * __builtin_spirv_OpenCL_atan2_f32_f32(x, y);
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( __builtin_spirv_OpenCL_atan2pi, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __builtin_spirv_OpenCL_atan2pi_f64_f64( double x, double y )
{
    return M_1_PI * __builtin_spirv_OpenCL_atan2_f64_f64(x, y);
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( __builtin_spirv_OpenCL_atan2pi, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_atan2pi_f16_f16( half x, half y )
{
    return M_1_PI_H * __builtin_spirv_OpenCL_atan2_f16_f16(x, y);
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( __builtin_spirv_OpenCL_atan2pi, half, half, f16 )

#endif // defined(cl_khr_fp16)

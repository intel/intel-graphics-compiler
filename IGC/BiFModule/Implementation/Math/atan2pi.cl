/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float __attribute__((overloadable)) __spirv_ocl_atan2pi( float x, float y )
{
    return M_1_PI_F * __spirv_ocl_atan2(x, y);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS( atan2pi, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __attribute__((overloadable)) __spirv_ocl_atan2pi( double x, double y )
{
    return M_1_PI * __spirv_ocl_atan2(x, y);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS( atan2pi, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __attribute__((overloadable)) __spirv_ocl_atan2pi( half x, half y )
{
    return M_1_PI_H * __spirv_ocl_atan2(x, y);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS( atan2pi, half, half, f16 )

#endif // defined(cl_khr_fp16)


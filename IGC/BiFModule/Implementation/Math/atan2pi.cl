/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atan2pi, _f32_f32, )( float x, float y )
{
    return M_1_PI_F * SPIRV_OCL_BUILTIN(atan2, _f32_f32, )(x, y);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS( atan2pi, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atan2pi, _f64_f64, )( double x, double y )
{
    return M_1_PI * SPIRV_OCL_BUILTIN(atan2, _f64_f64, )(x, y);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS( atan2pi, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atan2pi, _f16_f16, )( half x, half y )
{
    return M_1_PI_H * SPIRV_OCL_BUILTIN(atan2, _f16_f16, )(x, y);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS( atan2pi, half, half, f16 )

#endif // defined(cl_khr_fp16)

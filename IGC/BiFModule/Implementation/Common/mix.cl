/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(mix, _f32_f32_f32, )(float x, float y, float a ){
    return SPIRV_OCL_BUILTIN(mad, _f32_f32_f32, )( ( y - x ), a, x );
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_3ARGS( mix, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(mix, _f64_f64_f64, )(double x, double y, double a ){
    return SPIRV_OCL_BUILTIN(mad, _f64_f64_f64, )( ( y - x ), a, x );
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_3ARGS( mix, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(mix, _f16_f16_f16, )(half x, half y, half a ){
    return SPIRV_OCL_BUILTIN(mad, _f16_f16_f16, )( ( y - x ), a, x );
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_3ARGS( mix, half, half, f16 )

#endif // defined(cl_khr_fp16)

/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_divide, _f32_f32, )( float x, float y )
{
    return x * SPIRV_OCL_BUILTIN(native_recip, _f32, )( y );
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS( native_divide, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_divide, _f64_f64, )( double x, double y )
{
    return x * SPIRV_OCL_BUILTIN(native_recip, _f64, )( y );
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS( native_divide, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_divide, _f16_f16, )( half x, half y )
{
    return x * SPIRV_OCL_BUILTIN(native_recip, _f16, )( y );
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS( native_divide, half, half, f16 )

#endif // defined(cl_khr_fp16)

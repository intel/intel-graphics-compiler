/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float __builtin_spirv_OpenCL_native_divide_f32_f32( float x, float y )
{
    return x * __builtin_spirv_OpenCL_native_recip_f32( y );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( __builtin_spirv_OpenCL_native_divide, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __builtin_spirv_OpenCL_native_divide_f64_f64( double x, double y )
{
    return x * __builtin_spirv_OpenCL_native_recip_f64( y );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( __builtin_spirv_OpenCL_native_divide, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_native_divide_f16_f16( half x, half y )
{
    return x * __builtin_spirv_OpenCL_native_recip_f16( y );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( __builtin_spirv_OpenCL_native_divide, half, half, f16 )

#endif // defined(cl_khr_fp16)

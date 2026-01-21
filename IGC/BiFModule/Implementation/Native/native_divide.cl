/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float __attribute__((overloadable)) __spirv_ocl_native_divide( float x, float y )
{
    return x * __spirv_ocl_native_recip( y );
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS( native_divide, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __attribute__((overloadable)) __spirv_ocl_native_divide( double x, double y )
{
    return x * __spirv_ocl_native_recip( y );
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS( native_divide, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __attribute__((overloadable)) __spirv_ocl_native_divide( half x, half y )
{
    return x * __spirv_ocl_native_recip( y );
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS( native_divide, half, half, f16 )

#endif // defined(cl_khr_fp16)


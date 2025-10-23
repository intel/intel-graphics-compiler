/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float __attribute__((overloadable)) __spirv_ocl_fmax( float x, float y )
{
    return __builtin_IB_fmax(x, y);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS( fmax, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __attribute__((overloadable)) __spirv_ocl_fmax( double x, double y )
{
    return __builtin_IB_dmax(x, y);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS( fmax, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __attribute__((overloadable)) __spirv_ocl_fmax( half x, half y )
{
    return __builtin_IB_HMAX(x, y);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS( fmax, half, half, f16 )

#endif // defined(cl_khr_fp16)

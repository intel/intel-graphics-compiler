/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( acos, float, float, f32 )

#if defined(cl_khr_fp64)
    #include "../IMF/FP64/acos_d_la.cl"
#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp64)

INLINE double __attribute__((overloadable)) __spirv_ocl_acos( double x )
{
    return __ocl_svml_acos(x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( acos, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( acos, half, half, f16 )

#endif // defined(cl_khr_fp16)


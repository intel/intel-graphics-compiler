/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"
#include "../IMF/FP32/cbrt_s_la.cl"

#if defined(cl_khr_fp64)
    #include "../IMF/FP64/cbrt_d_la.cl"
#endif

INLINE float __attribute__((overloadable)) __spirv_ocl_cbrt( float x )
{
    return __ocl_svml_cbrtf(x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( cbrt, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __attribute__((overloadable)) __spirv_ocl_cbrt( double x )
{
    return __ocl_svml_cbrt(x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( cbrt, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __attribute__((overloadable)) __spirv_ocl_cbrt( half x )
{
    return __spirv_ocl_cbrt((float)x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( cbrt, half, half, f16 )

#endif // defined(cl_khr_fp16)


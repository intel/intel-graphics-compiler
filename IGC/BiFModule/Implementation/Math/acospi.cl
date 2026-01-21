/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"
#include "../IMF/FP32/acospi_s_la.cl"

#if defined(cl_khr_fp64)
    #include "../IMF/FP64/acospi_d_la.cl"
#endif // defined(cl_khr_fp64)

INLINE float __attribute__((overloadable)) __spirv_ocl_acospi( float x )
{
    return __ocl_svml_acospif(x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( acospi, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __attribute__((overloadable)) __spirv_ocl_acospi( double x )
{
    return __ocl_svml_acospi(x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( acospi, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __attribute__((overloadable)) __spirv_ocl_acospi( half x )
{
    return M_1_PI_H * __spirv_ocl_acos(x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( acospi, half, half, f16 )

#endif // defined(cl_khr_fp16)


/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

#if defined(cl_khr_fp64)
    #include "../IMF/FP64/asin_d_la.cl"
#endif //defined(cl_khr_fp64)

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( asin, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __attribute__((overloadable)) __spirv_ocl_asin( double x )
{
    return __ocl_svml_asin(x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( asin, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( asin, half, half, f16 )

#endif // defined(cl_khr_fp16)

#if defined(IGC_SPV_INTEL_bfloat16_arithmetic)
INLINE bfloat __attribute__((overloadable)) __spirv_ocl_asin( bfloat x )
{
    return __spirv_ocl_asin((float)x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( asin, bfloat, bfloat, )
#endif // defined(IGC_SPV_INTEL_bfloat16_arithmetic)

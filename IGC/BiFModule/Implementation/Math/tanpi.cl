/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"
#include "../IMF/FP32/tanpi_s_la.cl"

#if defined(cl_khr_fp64)
    #include "../IMF/FP64/tanpi_d_la.cl"
#endif // defined(cl_khr_fp64)

INLINE float __attribute__((overloadable)) __spirv_ocl_tanpi( float x )
{
    bool useNative = BIF_FLAG_CTRL_GET(FastRelaxedMath) && (!BIF_FLAG_CTRL_GET(APIRS));

    if(useNative)
    {
        return __spirv_ocl_native_tan(x * M_PI_F);
    }
    else
    {
        return __ocl_svml_tanpif(x);
    }
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARG_LOOP( tanpi, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __attribute__((overloadable)) __spirv_ocl_tanpi( double x )
{
    return __ocl_svml_tanpi(x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARG_LOOP( tanpi, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __attribute__((overloadable)) __spirv_ocl_tanpi( half x )
{
    return __spirv_ocl_tanpi((float)x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARG_LOOP( tanpi, half, half, f16 )

#endif // defined(cl_khr_fp16)


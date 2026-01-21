/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"
#include "../IMF/FP32/cospi_s_la.cl"
#include "../IMF/FP32/cospi_s_noLUT.cl"

#if defined(cl_khr_fp64)
    #include "../IMF/FP64/cospi_d_la.cl"
#endif // defined(cl_khr_fp64)

INLINE float __attribute__((overloadable)) __spirv_ocl_cospi( float x )
{
    bool useNative = BIF_FLAG_CTRL_GET(FastRelaxedMath) && (!BIF_FLAG_CTRL_GET(APIRS));

    if(useNative)
    {
        return __spirv_ocl_cos( x * M_PI_F );
    }
    else
    {
        if(BIF_FLAG_CTRL_GET(UseMathWithLUT))
        {
            return __ocl_svml_cospif(x);
        }
        else
        {
            return __ocl_svml_cospif_noLUT(x);
        }
    }
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARG_LOOP( cospi, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __attribute__((overloadable)) __spirv_ocl_cospi( double x )
{
    return __ocl_svml_cospi(x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARG_LOOP( cospi, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __attribute__((overloadable)) __spirv_ocl_cospi( half x )
{
    return __spirv_ocl_cospi((float)x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARG_LOOP( cospi, half, half, f16 )

#endif // defined(cl_khr_fp16)


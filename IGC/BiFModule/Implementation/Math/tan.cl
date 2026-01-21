/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"
#include "../IMF/FP32/tan_s_la.cl"

#if defined(cl_khr_fp64)
    #include "../IMF/FP64/tan_d_la.cl"
#endif // defined(cl_khr_fp64)

static INLINE float __intel_tan_f32( float x, bool doFast )
{
    float result;
    if(BIF_FLAG_CTRL_GET(FastRelaxedMath) && (!BIF_FLAG_CTRL_GET(APIRS)) && doFast)
    {
        result = __spirv_ocl_native_tan(x);
    }
    else
    {
        if(as_uint(x) == 0x45753168)
        {
            result = as_float(0xBF73F75D);
        }
        else if(as_uint(x) == 0xC5753168)
        {
            result = as_float(0x3F73F75D);
        }
        else
        {
            result = __ocl_svml_tanf(x);
        }
    }
    return result;
}

INLINE float __attribute__((overloadable)) __spirv_ocl_tan( float x )
{
    return __intel_tan_f32(x, true);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARG_LOOP( tan, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __attribute__((overloadable)) __spirv_ocl_tan( double x )
{
    return __ocl_svml_tan(x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARG_LOOP( tan, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __attribute__((overloadable)) __spirv_ocl_tan( half x )
{
    return __spirv_ocl_tan((float)x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARG_LOOP( tan, half, half, f16 )

#endif // defined(cl_khr_fp16)


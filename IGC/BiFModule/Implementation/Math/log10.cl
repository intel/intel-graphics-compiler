/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"
#include "../IMF/FP32/log10_s_la.cl"

#if defined(cl_khr_fp64)
    #include "../IMF/FP64/log10_d_la.cl"
    #include "../IMF/FP64/log10_d_la_noLUT.cl"
#endif // defined(cl_khr_fp64)

#define _M_LOG10_E_DBL  (as_double(0x3fdbcb7b1526e50e)) // 0.4342944819032518276511289

INLINE float __attribute__((overloadable)) __spirv_ocl_log10( float x )
{
    float result;

    if(BIF_FLAG_CTRL_GET(FastRelaxedMath))
    {
        result = __spirv_ocl_native_log10(x);
    }
    else if (BIF_FLAG_CTRL_GET(HasAccurateLog2) && (x < 0.5f || x >= 1.5f))
    {
        float w = __spirv_ocl_native_log2(x);

        uint ux = as_uint(x);
        // Implies !isnan(x) && !isinf(x) && x >= 0
        bool isposfinite = ux < 0x7f800000u;
        bool isnonzero =
            BIF_FLAG_CTRL_GET(FlushDenormals) ? ux > FLOAT_MANTISSA_MASK : ux > 0u;

        if (isposfinite && isnonzero)
        {
            // c1 + c2 == log10(2), split into two parts to improve accuracy
            const float c1 = 10100891.0f / 33554432.0f;      // exact value
            const float c2 = -503875.0f / 35184372088832.0f; // exact value
            result         = __spirv_ocl_fma(c1, w, c2 * w);
        }
        else
        {
            // The native log2 already handles special cases (negative, zero, NaN, Inf)
            result = w;
        }
    }
    else
    {
        result = __ocl_svml_log10f(x);
    }

    return result;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARG_LOOP( log10, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __attribute__((overloadable)) __spirv_ocl_log10( double x )
{
    double result;
    if (BIF_FLAG_CTRL_GET(UseHighAccuracyMath)) {
        result = __ocl_svml_log10_noLUT(x);
    } else {
        result = __ocl_svml_log10_v2(x);
    }
    return result;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARG_LOOP( log10, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __attribute__((overloadable)) __spirv_ocl_log10( half x )
{
    if (BIF_FLAG_CTRL_GET(HasAccurateLog2))
    {
        // Scale in single precision; scaling in half precision gives up to
        // 1.2 ulp of error instead of 0.5 ulp.
        return (half)(__spirv_ocl_native_log2((float)x) *
                      as_float(0x3e9a209b)); // log10(2)
    }
    return __spirv_ocl_log10((float)x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARG_LOOP( log10, half, half, f16 )

#endif // defined(cl_khr_fp16)


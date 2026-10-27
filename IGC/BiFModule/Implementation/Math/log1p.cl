/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"
#include "../IMF/FP32/log1p_s_la.cl"

#if defined(cl_khr_fp64)
    #include "../IMF/FP64/log1p_d_la.cl"
#endif // defined(cl_khr_fp64)

INLINE float __attribute__((overloadable)) __spirv_ocl_log1p( float x )
{
    if (BIF_FLAG_CTRL_GET(FastRelaxedMath))
    {
        return __spirv_ocl_native_log(1.0f + x);
    }

    if (!BIF_FLAG_CTRL_GET(HasAccurateLog2))
    {
        return __ocl_svml_log1pf(x);
    }

    // For x >= 0.5 or x < -0.5, 1 + x is outside [0.5, 1.5), so the native
    // log2 is accurate enough for OpenCL.
    if (x >= 0.5f || x < -0.5f)
    {
        float xs   = 1.0f + x;
        float a    = __spirv_ocl_fmax(x, 1.0f);
        float b    = __spirv_ocl_fmin(x, 1.0f);
        float bh   = xs - a;
        float xs_l = b - bh; // low part of 1 + x

        // xs_l / xs
        float lcorr = xs_l * __spirv_ocl_native_recip(xs);

        // fixup for special inputs
        uint lcorru = as_uint(lcorr);
        lcorru      = (lcorru & 0x40000000u) ? 0u : lcorru;
        lcorr       = as_float(lcorru);

        return __spirv_ocl_fma(__spirv_ocl_native_log2(xs), M_LN2_F, lcorr);
    }

    // Polynomial approximation of log(1 + x) for -0.5 <= x < 0.5
    float poly = __spirv_ocl_fma(x, as_float(0xbe3819e7), as_float(0x3e3c333a));
    poly       = __spirv_ocl_fma(poly, x, as_float(0xbd85760b));
    poly       = __spirv_ocl_fma(poly, x, as_float(0x3da5f1df));
    poly       = __spirv_ocl_fma(poly, x, as_float(0xbe05bacb));
    poly       = __spirv_ocl_fma(poly, x, as_float(0x3e16928b));
    poly       = __spirv_ocl_fma(poly, x, as_float(0xbe2a3ba5));
    poly       = __spirv_ocl_fma(poly, x, as_float(0x3e4c89a6));
    poly       = __spirv_ocl_fma(poly, x, as_float(0xbe8001c5));
    poly       = __spirv_ocl_fma(poly, x, as_float(0x3eaaab68));
    poly       = __spirv_ocl_fma(poly, x, as_float(0xbefffffc));
    poly       = poly * x;
    return __spirv_ocl_fma(poly, x, x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARG_LOOP( log1p, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __attribute__((overloadable)) __spirv_ocl_log1p( double x )
{
    return __ocl_svml_log1p(x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARG_LOOP( log1p, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __attribute__((overloadable)) __spirv_ocl_log1p( half x )
{
    if (BIF_FLAG_CTRL_GET(HasAccurateLog2))
    {
        // log2(1 + x) * ln(2) in single precision
        return (half)__spirv_ocl_native_log(1.0f + (float)x);
    }

    return __spirv_ocl_log1p((float)x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARG_LOOP( log1p, half, half, f16 )

#endif // defined(cl_khr_fp16)


/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

#define USE_IMF_TGAMMA_IMPL 1

#ifdef USE_IMF_TGAMMA_IMPL
#include "../IMF/FP32/tgamma_s_noFP64.cl"
#endif // USE_IMF_TGAMMA_IMPL

#define SQRT_2PI                (as_float(0x40206C98)) // 2.5066282746310007f

// Computes the gamma functions using a Lanczos approximation:
static float __intel_gamma(float z)
{
    float p0 = as_float(0x3f800000);    // 1.0f
    float p1 = as_float(0x42985c35);    // 76.180092f
    float p2 = as_float(0xc2ad02b9);    // -86.505318f
    float p3 = as_float(0x41c01ce0);    // 24.014099
    float p4 = as_float(0xbf9da9a4);    // -1.2317395
    float p5 = as_float(0x3a9e6b99);    // 1.2086510e-3f
    float p6 = as_float(0xb6b508c1);    // -5.3952394e-6f
    float g = 5.0f; // number of coefficients - 2

    z -= 1;

    float x = p0;
    x += p1 / (z + 1);
    x += p2 / (z + 2);
    x += p3 / (z + 3);
    x += p4 / (z + 4);
    x += p5 / (z + 5);
    x += p6 / (z + 6);

    float t = z + g + 0.5f;
    return SQRT_2PI * __spirv_ocl_pow(t, z + 0.5f) * __spirv_ocl_exp(-t) * x;
}

float __attribute__((overloadable)) __spirv_ocl_tgamma( float x )
{
#if USE_IMF_TGAMMA_IMPL
    return __ocl_svml_tgammaf(x);
#else // USE_IMF_TGAMMA_IMPL
    float ret;
    if ( (x < 0.0f) & (x == __spirv_ocl_floor(x))) {
        ret = __spirv_ocl_nan(0)
    } else {
        float y = 1.0f - x;
        float z = ( x < 0.5f ) ? y : x;
        // Note: z >= 0.5f.
        float g = __intel_gamma(z);

        ret = ( x < 0.5f ) ?
            M_PI_F / ( __spirv_ocl_sinpi(x) * g ) :
            g;

        // Special handling for -0.0f.
        // It may be possible to restrict this to renderscript only,
        // but for now we'll apply it across the board to stay on
        // the safe side, since this built-in is used infrequently.
        ret = ( as_uint(x) == FLOAT_SIGN_MASK ) ? -INFINITY : ret;
    }
    return ret;
#endif // USE_IMF_TGAMMA_IMPL
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARG_LOOP( tgamma, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __attribute__((overloadable)) __spirv_ocl_tgamma( double x )
{
    return libclc_tgamma_f64(x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARG_LOOP( tgamma, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __attribute__((overloadable)) __spirv_ocl_tgamma( half x )
{
    return __spirv_ocl_tgamma((float)x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARG_LOOP( tgamma, half, half, f16 )

#endif // defined(cl_khr_fp16)


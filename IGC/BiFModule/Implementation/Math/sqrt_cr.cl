/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

extern __constant int __Native64Bit;

#if defined(cl_khr_fp16)

INLINE
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sqrt_cr, _f16, )( half a )
{
    return (half)SPIRV_OCL_BUILTIN(sqrt_cr, _f32, )((float)a);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( sqrt_cr, half, half, f16 )

#endif // define(cl_khr_fp16)

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sqrt_cr, _f32, )( float a )
{
    if (!__CRMacros)
    {
        typedef union binary32
        {
            uint u;
            int  s;
            float f;
        } binary32;

        binary32 fa, y0, onehalf, H0, H1, S0, S1, S, d0, e0;
        int aExp, sExp;
        fa.f = a;
        aExp = (fa.u >> 23) & 0xff;
        uint significand = fa.u & 0x7fffff;

        if (aExp == 0 & significand == 0) {
            /* return +/-zero for +/-zero */
            S.u = fa.u;
        }
        else if (aExp == 0xff) { /* NaN and Inf */
            if ((fa.u & 0x7fffff) == 0) { /* Inf */
                S.u = (fa.u & 0x80000000) ? 0xffc00000 : 0x7f800000;
            } else { /* NaN */
                S.u = fa.u | 0x400000; /* Quiet signalling NaN */
            }
        } else {
            if (fa.u & 0x80000000) { /* Negative normals/denormals */
                if (__FlushDenormals & (aExp == 0))
                    S.u = fa.u & 0x80000000;
                else
                    /* return qNaN for negative normal/denormal values */
                    S.u = 0xffc00000;
            } else if (__FlushDenormals & (aExp == 0)) {
                S.u = 0; // positive denorms
            } else { /* Positive normals/denormals */
                bool denorm = (aExp == 0);

                if (denorm & !__FlushDenormals) {
                    fa.f = SPIRV_OCL_BUILTIN(ldexp, _f32_i32, )(fa.f, 126);
                }
                else {
                    // Scale a to [1/2, 2)
                    fa.u = (fa.u & 0x00ffffff) | 0x3f000000;
                }

                // Initial approximation
                y0.f = SPIRV_OCL_BUILTIN(rsqrt, _f32, )(fa.f);
                onehalf.u = 0x3f000000;
                // Step(1), H0 = 1/2y0
                H0.f = onehalf.f * y0.f;
                // Step(2), S0 = a*y0
                S0.f = fa.f * y0.f;
                // Step(3), d0 = 1/2 - S0*H0
                d0.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(-S0.f, H0.f, onehalf.f);
                // Step(4), H1 = H0 + d0*H0
                H1.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(d0.f, H0.f, H0.f);
                // Step(5), S1 = S0 + d0*S0
                S1.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(d0.f, S0.f, S0.f);
                // Step(6), e0 = a - S1*S1
                e0.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(-S1.f, S1.f, fa.f);
                // Step(7), S = S1 + e0*H1
                S.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(e0.f, H1.f, S1.f);

                if (denorm & !__FlushDenormals) {
                    S.f = SPIRV_OCL_BUILTIN(ldexp, _f32_i32, )(S.f, -126/2);
                }
                else {
                    // Adjust exponent
                    sExp = ((aExp - FLOAT_BIAS) >> 1) + FLOAT_BIAS;
                    S.u = (S.u & 0x007fffff) | (sExp << 23);
                }
            }
        }
        return S.f;
    }
    else
    {
        return FSQRT_IEEE(a);
    }
}

#ifdef cl_fp64_basic_ops

INLINE double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sqrt_cr, _f64, )( double x )
{
        return SPIRV_OCL_BUILTIN(sqrt, _f64, )(x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( sqrt_cr, double, double, f64 )

#endif // cl_fp64_basic_ops

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( sqrt_cr, float, float, f32 )

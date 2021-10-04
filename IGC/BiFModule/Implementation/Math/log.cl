/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"
#include "../IMF/FP32/ln_s_la.cl"
#include "../IMF/FP64/ln_d_la_noLUT.cl"

#if defined(cl_khr_fp64)
    #include "../IMF/FP64/ln_d_la.cl"
#endif // defined(cl_khr_fp64)

float __builtin_spirv_OpenCL_log_f32( float x )
{
#if 0
    // This version is ever so slightly faster (<1%) than the version below,
    // however it is almost a full ULP less precise in some cases, so we'll
    // stick with the full expansion for now.
    return __builtin_spirv_OpenCL_log2_f32(x) * M_LN2_F;
#else
    float result;

    if(__FastRelaxedMath)
    {
        result = __builtin_spirv_OpenCL_native_log_f32(x);
    }
    //  Denorm checking is to work-around a llvm issue that demote
    //  "(float) x > 0.0f"  to " (half)x > (half)0.0f" (log(half).
    //  This causes the inaccurate result with -cl-denorms-are-zero.
    else if( __intel_relaxed_isfinite(x) &
             ((!__FlushDenormals & (x > 0.0f)) |
              ( __FlushDenormals & (as_int(x) > 0x7FFFFF))) )
    //else if( __intel_relaxed_isfinite(x) & ( x > 0.0f ) )
    {
        if(__UseMathWithLUT)
        {
            result = __ocl_svml_logf(x);
        }
        else
        {
        // We already know that we're positive and finite, so
        // we can use this very cheap check for normal vs.
        // subnormal inputs:
        float s = x * ( 1 << FLOAT_MANTISSA_BITS );
        float e = ( x < FLT_MIN ) ? -FLOAT_MANTISSA_BITS : 0.0f;
        x = ( x < FLT_MIN ) ? s : x;

        const int   magic = 0x3f2aaaab;
        int iX = as_int(x) - magic;
        int iR = ( iX & FLOAT_MANTISSA_MASK ) + magic;

        e += iX >> FLOAT_MANTISSA_BITS;

        float sR = as_float(iR) - 1.0f;

        float sP = as_float(0xbe0402c8);
        sP = __builtin_spirv_OpenCL_fma_f32_f32_f32( sP, sR, as_float(0x3e0f335d));
        sP = __builtin_spirv_OpenCL_fma_f32_f32_f32( sP, sR, as_float(0xbdf9889e));
        sP = __builtin_spirv_OpenCL_fma_f32_f32_f32( sP, sR, as_float(0x3e0f6b8c));
        sP = __builtin_spirv_OpenCL_fma_f32_f32_f32( sP, sR, as_float(0xbe2acee6));
        sP = __builtin_spirv_OpenCL_fma_f32_f32_f32( sP, sR, as_float(0x3e4ce814));
        sP = __builtin_spirv_OpenCL_fma_f32_f32_f32( sP, sR, as_float(0xbe7fff78));
        sP = __builtin_spirv_OpenCL_fma_f32_f32_f32( sP, sR, as_float(0x3eaaaa83));
        sP = __builtin_spirv_OpenCL_fma_f32_f32_f32( sP, sR, as_float(0xbf000000));

        sP = sP * sR;
        sP = __builtin_spirv_OpenCL_fma_f32_f32_f32( sP, sR, sR);

        sP = __builtin_spirv_OpenCL_fma_f32_f32_f32( e, as_float(0x35bfbe8e), sP);
        sP = __builtin_spirv_OpenCL_fma_f32_f32_f32( e, as_float(0x3f317200), sP);

        result = sP;
        }
    }
    else
    {
        // If we get here, we're either infinity, NaN, or negative.
        // The native log2 handles all of these cases.  Note, we don't
        // have to multiply by M_LN2_F, since the result in
        // these cases is NaN or +/- infinity, therefore the multiply
        // is irrelevant and unnecessary.
        result = __builtin_spirv_OpenCL_native_log2_f32(x);
    }

    return result;
#endif
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_log, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __builtin_spirv_OpenCL_log_f64( double x )
{
    double result;
    if (__UseMathWithLUT) {
        result = __ocl_svml_log(x);
    } else {
        result = __ocl_svml_log_noLUT(x);
    }
    return result;
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_log, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_log_f16( half x )
{
    return (half)__builtin_spirv_OpenCL_log_f32((float)x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_log, half, half, f16 )

#endif // defined(cl_khr_fp16)

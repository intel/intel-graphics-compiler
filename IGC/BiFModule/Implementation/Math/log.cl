/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"


#if defined(cl_khr_fp64)

    #include "../ExternalLibraries/libclc/doubles.cl"

#endif // defined(cl_khr_fp64)

/*
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//    Copyright (C) 1996-2010 Intel Corporation. All Rights Reserved.
//
*/

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
        return libclc_log_f64(x);
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

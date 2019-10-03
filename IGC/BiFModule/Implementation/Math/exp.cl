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

float __builtin_spirv_OpenCL_exp_f32(float x)
{
    if (__FastRelaxedMath)
    {
        return __builtin_spirv_OpenCL_native_exp_f32(x);
    }
    else
    {
        // e^x = 2^(log2(e^x)) = 2^(x * log2(e))
        // We'll compute 2^(x * log2(e)) by splitting x * log2(e)
        //   into a whole part and fractional part.

        // Compute the whole part of x * log2(e)
        // This part is easy!
        float w = __builtin_spirv_OpenCL_trunc_f32( x * M_LOG2E_F );

        // Compute the fractional part of x * log2(e)
        // We have to do this carefully, so we don't lose precision.
        // Compute as:
        //   fract( x * log2(e) ) = ( x - w * C1 - w * C2 ) * log2(e)
        // C1 is the "Cephes Constant", and is close to 1/log2(e)
        // C2 is the difference between the "Cephes Constant" and 1/log2(e)
        const float C1 = as_float( 0x3F317200 );    // 0.693145751953125
        const float C2 = as_float( 0x35BFBE8E );    // 0.000001428606765330187
        float f = x;
        f = __builtin_spirv_OpenCL_fma_f32_f32_f32( w, -C1, f );
        f = __builtin_spirv_OpenCL_fma_f32_f32_f32( w, -C2, f );
        f = f * M_LOG2E_F;

        w = __builtin_spirv_OpenCL_native_exp2_f32( w );   // this should be exact
        f = __builtin_spirv_OpenCL_native_exp2_f32( f );   // this should be close enough

        float res = w * f;
        res = ( x < as_float( 0xC2D20000 ) ) ? as_float( 0x00000000 ) : res;
        res = ( x > as_float( 0x42D20000 ) ) ? as_float( 0x7F800000 ) : res;

        return res;
    }
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_exp, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __builtin_spirv_OpenCL_exp_f64( double x )
{
        return libclc_exp_f64( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_exp, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_exp_f16( half x )
{
    return __builtin_spirv_OpenCL_exp_f32((float)x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_exp, half, half, f16 )

#endif // defined(cl_khr_fp16)

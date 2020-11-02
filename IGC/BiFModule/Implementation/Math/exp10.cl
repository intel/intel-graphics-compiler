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
    #include "../IMF/FP64/exp10_d_la.cl"
#endif // defined(cl_khr_fp64)

float __builtin_spirv_OpenCL_exp10_f32( float x )
{
    if(__FastRelaxedMath)
    {
        return __builtin_spirv_OpenCL_native_exp10_f32(x);
    }
    else
    {
        // 10^x = 2^(log2(10^x)) = 2^(x * log2(10))
        // We'll compute 2^(x * log2(10)) by splitting x * log2(10)
        //   into a whole part and fractional part.
        const float log2_10 = as_float( 0x40549A78 );

        // Compute the whole part of x * log2(10)
        // This part is easy!
        float w = __builtin_spirv_OpenCL_trunc_f32( x * log2_10 );

        // Compute the fractional part of x * log2(10)
        // We have to do this carefully, so we don't lose precision.
        // Compute as:
        //   fract( x * log2(10) ) = ( x - w * C1 - w * C2 ) * log2(10)
        // C1 is the "Cephes Constant", and is close to 1/log2(10)
        // C2 is the difference between the "Cephes Constant" and 1/log2(10)
        const float C1 = as_float( 0x3E9A0000 ); // 0.30078125
        const float C2 = as_float( 0x39826A14 ); // 0.00024874567
        float f = x;
        f = __builtin_spirv_OpenCL_fma_f32_f32_f32( w, -C1, f );
        f = __builtin_spirv_OpenCL_fma_f32_f32_f32( w, -C2, f );
        f = f * log2_10;

        w = __builtin_spirv_OpenCL_native_exp2_f32( w );   // this should be exact
        f = __builtin_spirv_OpenCL_native_exp2_f32( f );   // this should be close enough

        float res = w * f;
        res = ( x < as_float( 0xC2800000 ) ) ? as_float( 0x00000000 ) : res;
        res = ( x > as_float( 0x42200000 ) ) ? as_float( 0x7F800000 ) : res;

        return res;
    }
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_exp10, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __builtin_spirv_OpenCL_exp10_f64( double x )
{
    return __ocl_svml_exp10(x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_exp10, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_exp10_f16( half x )
{
    return __builtin_spirv_OpenCL_exp10_f32((float)x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_exp10, half, half, f16 )

#endif // defined(cl_khr_fp16)

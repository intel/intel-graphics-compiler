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
        // 10^x = 2^(log2(10^x)) = 2^(x * log2(10))
        // We'll compute 2^(x * log2(10)) by splitting x * log2(10)
        //   into a whole part and fractional part.
        const double log2_10 = as_double( 0x400A934F0979A371UL ); // Closest value to log2(10)
        const double log2_10_diff = as_double( 0x3CA7F2495FB7FA75UL ); // infinite precise log2(10) - log2_10

        double2 x2 = (double2)(log2_10, log2_10_diff);
        double2 y2 = (double2)(x, 0.0);

        volatile double res2 = x2.x * y2.x;
        double err = __builtin_spirv_OpenCL_fma_f64_f64_f64(x2.x, y2.x, -res2);
        double2 prod = (double2)(res2, err);

        prod.y = __builtin_spirv_OpenCL_fma_f64_f64_f64(x2.x, y2.y, prod.y);
        prod.y = __builtin_spirv_OpenCL_fma_f64_f64_f64(x2.y, y2.x, prod.y);

        double sum  = prod.x + prod.y;
        double err2 = prod.y - (sum - prod.x);

        double w = __builtin_spirv_OpenCL_trunc_f64(sum);

        // Now to emulate the subtraction....

        double2 x3 = (double2)(sum, err2);
        double2 y3 = (double2)(-w, 0.0);

        double sum1   = x3.x + y3.x;
        double ydiff = sum1 - x3.x;
        double err1   = (x3.x - (sum1 - ydiff)) + (y3.x - ydiff);

        double sum2   = x3.y + y3.y;
        double ydiff2 = sum2 - x3.y;
        double err2_1   = (x3.y - (sum2 - ydiff2)) + (y3.y - ydiff2);

        err1 += sum2;

        double sum3 = sum1 + err1;
        err1 -= (sum3 - sum1);
        err1 += err2_1;

        double f = sum3 + err1;

        double w2 = __builtin_spirv_OpenCL_exp2_f64( w );   // calculation with the whole part
        double f2 = __builtin_spirv_OpenCL_exp2_f64( f );   // calculation with the fractional part

        double res = w2 * f2;

        res = ( x < as_double( 0xC0912C0000000000UL ) )  ? as_double( 0x0000000000000000UL ) : res;
        res = ( x >= as_double( 0x4090040000000000UL ) ) ? as_double( 0x7FF0000000000000UL ) : res;

        return res;
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

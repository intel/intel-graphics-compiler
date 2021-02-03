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

float __builtin_spirv_OpenCL_nextafter_f32_f32( float x, float y )
{
    const int maxneg = FLOAT_SIGN_MASK;

    // The way this algorithm works is as follows:
    //
    // - Treat the incoming float as an integer.  Note that the integer
    //   representation of floats are ordered if the integer is interpreted
    //   as being sign-magnitude encoded.
    // - Convert each incoming float from sign-magnitude to twos-complement
    //   encoded, so we can use usual comparison and math operations on them.
    // - Based on the twos complement encoding of the integer representation
    //   of each float, either add or subtract one from the twos-complement
    //   encoding of the integer representation of x.  This gives a twos-
    //   complement encoding of the result.
    // - Convert from the twos-complement encoding of the result back
    //   to a sign-magnitude encoding of the result, and interpret as
    //   a float.  We're done!  Well, almost.
    // - Handle two special-cases:
    //    - When the two floats are equal then there is no delta.
    //    - When either float is NaN the result is NaN.
    //
    // The code is written so it does not need flow control.

    int smix = as_int(x);
    int nx = maxneg - smix;
    int tcix = ( smix < 0 ) ? nx : smix;

    int smiy = as_int(y);
    int ny = maxneg - smiy;
    int tciy = ( smiy < 0 ) ? ny : smiy;

    int delta = ( tcix < tciy ) ? 1 : -1;

    int tcir = tcix + delta;
    int nr = maxneg - tcir;
    int smir = ( tcir < 0 ) ? nr : tcir;

    float result = as_float(smir);
    result = (tcix == tciy) ? y : result;

    {
        float n = __builtin_spirv_OpenCL_nan_i32(0u);
        int test = __intel_relaxed_isnan(x) | __intel_relaxed_isnan(y);
        result = test ? n : result;
    }

    return result;
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( __builtin_spirv_OpenCL_nextafter, float, float, f32 )

#if defined(cl_khr_fp64)

double __builtin_spirv_OpenCL_nextafter_f64_f64( double x, double y )
{
    const long maxneg = DOUBLE_SIGN_MASK;

    long smix = as_long(x);
    long nx = maxneg - smix;
    long tcix = ( smix < 0 ) ? nx : smix;

    long smiy = as_long(y);
    long ny = maxneg - smiy;
    long tciy = ( smiy < 0 ) ? ny : smiy;

    long delta = ( tcix < tciy ) ? 1 : -1;

    long tcir = tcix + delta;
    long nr = maxneg - tcir;
    long smir = ( tcir < 0 ) ? nr : tcir;

    double result = as_double(smir);
    result = (tcix == tciy) ? y : result;

    {
        double n = __builtin_spirv_OpenCL_nan_i64(0ul);
        int test = __builtin_spirv_OpIsNan_f64(x) | __builtin_spirv_OpIsNan_f64(y);
        result = test ? n : result;
    }

    return result;
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( __builtin_spirv_OpenCL_nextafter, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

half __builtin_spirv_OpenCL_nextafter_f16_f16( half x, half y )
{
    const short maxneg = HALF_SIGN_MASK;

    short smix = as_short(x);
    short nx = maxneg - smix;
    short tcix = ( smix < 0 ) ? nx : smix;

    short smiy = as_short(y);
    short ny = maxneg - smiy;
    short tciy = ( smiy < 0 ) ? ny : smiy;

    short delta = ( tcix < tciy ) ? 1 : -1;

    short tcir = tcix + delta;
    short nr = maxneg - tcir;
    short smir = ( tcir < 0 ) ? nr : tcir;

    half result = as_half(smir);
    result = (tcix == tciy) ? y : result;

    {
        half n = __builtin_spirv_OpenCL_nan_i32(0u);
        int test = __builtin_spirv_OpIsNan_f16(x) | __builtin_spirv_OpIsNan_f16(y);
        result = test ? n : result;
    }

    return result;
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( __builtin_spirv_OpenCL_nextafter, half, half, f16 )

#endif // defined(cl_khr_fp16)

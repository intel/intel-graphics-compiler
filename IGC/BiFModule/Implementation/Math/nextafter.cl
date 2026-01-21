/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

float __attribute__((overloadable)) __spirv_ocl_nextafter( float x, float y )
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
        float n = __spirv_ocl_nan(0);
        int test = __intel_relaxed_isnan(x) | __intel_relaxed_isnan(y);
        result = test ? n : result;
    }

    return result;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS( nextafter, float, float, f32 )

#if defined(cl_khr_fp64)

double __attribute__((overloadable)) __spirv_ocl_nextafter( double x, double y )
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
        double n = __spirv_ocl_nan(0);
        int test = __spirv_IsNan(x) | __spirv_IsNan(y);
        result = test ? n : result;
    }

    return result;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS( nextafter, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

half __attribute__((overloadable)) __spirv_ocl_nextafter( half x, half y )
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
        half n = __spirv_ocl_nan(0);
        int test = __spirv_IsNan(x) | __spirv_IsNan(y);
        result = test ? n : result;
    }

    return result;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS( nextafter, half, half, f16 )

#endif // defined(cl_khr_fp16)


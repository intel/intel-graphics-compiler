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

#include "spirv.h"

INLINE double __builtin_spirv_OpenCL_fclamp_f64_f64_f64(double x, double minval, double maxval ){
    return __builtin_spirv_OpenCL_fmin_f64_f64(__builtin_spirv_OpenCL_fmax_f64_f64(x, minval), maxval);
}

INLINE
double __builtin_spirv_OpenCL_ceil_f64(double x ){
    //First part of the algorithm performs rounding towards zero by truncating bits in
    //the fractional part of the number.This is done by finding out the position of the
    //fractional bits of the mantissa and masking them out with zeros.

    double roundedToZeroVal = __builtin_spirv_OpenCL_trunc_f64(x);
    unsigned high32Bit = (int)(as_ulong( x ) >> 32);
    ulong fraction = as_ulong(x) - as_ulong(roundedToZeroVal);    // getting fraction

    // Second part is to calculate exponent adjustment based on sign of source

    uint temp5 = (uint)(as_ulong( fraction ));
    uint temp6 = (uint)(as_ulong( fraction ) >> 32);
    uint sign = high32Bit & 0x80000000;
    uint expBias = (sign == 0) ? 0x3ff00000 : 0;
    uint orDst = temp5 | temp6;
    ulong signAdjustedVal = (orDst == 0) ? 0 : (expBias);
    double output = as_double( signAdjustedVal << 32 ) + as_double( roundedToZeroVal );
    return output;
}

INLINE
double __builtin_spirv_OpenCL_fabs_f64(double x ){
    double neg = -x;
    return (x >= 0) ?  x : neg;
}

INLINE
double __builtin_spirv_OpenCL_floor_f64(double x ){
    //First part of the algorithm performs rounding towards zero by truncating bits in
    //the fractional part of the number.This is done by finding out the position of the
    //fractional bits of the mantissa and masking them out with zeros.

    double roundedToZeroVal = __builtin_spirv_OpenCL_trunc_f64(x);

    ulong fraction = as_ulong(x) - as_ulong( roundedToZeroVal );       // getting fraction

    // Second part is to calculate exponent adjustment based on sign of source

    uint temp5 = (uint)( fraction );
    uint temp6 = (uint)(fraction >> 32);
    unsigned high32Bit = (uint)(as_ulong( x ) >> 32);
    uint sign = high32Bit & 0x80000000;
    uint expBias = (sign != 0) ? 0xbff00000 : 0;
    uint orDst = temp5 | temp6;
    ulong signAdjustedVal = (orDst == 0) ? 0 : (expBias);
    double output = as_double( signAdjustedVal << 32 ) + as_double( roundedToZeroVal );
    return output;
}

INLINE double __builtin_spirv_OpenCL_native_sqrt_f64(double x ){
        return __builtin_IB_native_sqrtd(x);
}

INLINE double __builtin_spirv_OpenCL_native_rsqrt_f64(double x ){
      return (1 / __builtin_IB_native_sqrtd(x));
}

INLINE
double FDIV_IEEE_DOUBLE( double a,
                           double b )
{
    return __builtin_IB_ieee_divide_f64(a, b);
}

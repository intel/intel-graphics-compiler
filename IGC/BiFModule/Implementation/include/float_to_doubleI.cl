/*========================== begin_copyright_notice ============================

Copyright (c) 2016-2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

============================= end_copyright_notice ===========================*/

#ifndef __FLOAT_TO_DOUBLEI_CL__
#define __FLOAT_TO_DOUBLEI_CL__

#include "BiF_Definitions.cl"

unsigned int FloatToDoubleI(float single, __private unsigned int* lo)
#define FLOAT_BIAS              (127)
{
    unsigned int signbit   = as_int(single) & FLOAT_SIGN_MASK;
             int expF      = ((as_int(single) & FLOAT_EXPONENT_MASK) >> FLOAT_MANTISSA_BITS) - FLOAT_BIAS;
    unsigned int fracFbits = as_int(single) & FLOAT_MANTISSA_MASK;
    unsigned int hi, lz, expDbits, fracDbits_hi, fracDbits_lo;
    int expD;

    if(expF == -FLOAT_BIAS)
    {
        if(fracFbits == 0)
        {   // ZERO
            expD      = 0;
            fracFbits = 0;
        }
        else
        {   // DENORMAL
            lz        = __builtin_spirv_OpenCL_clz_i32(fracFbits) - (FLOAT_SIGN_BITS + FLOAT_EXPONENT_BITS);
            expD      = (expF - lz) + DOUBLE_BIAS;
            fracFbits = (fracFbits << (lz + 1)) & FLOAT_MANTISSA_MASK; // Add one for the implicit bit and strip off the implicit bit
        }
    }
    else if(expF == FLOAT_BIAS + 1)
    {   // INF OR NAN
        expD = DOUBLE_BIAS + 1 + DOUBLE_BIAS;
    }
    else
    {
      expD = expF + DOUBLE_BIAS;
    }
    expDbits = expD << (FLOAT_BITS - FLOAT_SIGN_BITS - DOUBLE_EXPONENT_BITS);

    /*
    lz = __builtin_spirv_OpenCL_clz_i32(as_int(__builtin_spirv_OpenCL_fabs_f32(single))) + 1;
    lz = (lz > 32) ? 32 : lz;
    expD = (expF == -FLOAT_BIAS) ?
                            ((fracFbits == 0)              ?  -DOUBLE_BIAS     : expF - lz) :
                            ((expF      == FLOAT_BIAS + 1) ?   DOUBLE_BIAS + 1 : expF);
    fracFbits = (expF == -FLOAT_BIAS) ?
                            ((fracFbits == 0)              ? 0               : fracFbits << lz) :
                            fracFbits;
    */

    fracDbits_hi = (fracFbits >> (FLOAT_MANTISSA_BITS - (FLOAT_BITS - FLOAT_SIGN_BITS - DOUBLE_EXPONENT_BITS)));
    fracDbits_lo = (fracFbits << (FLOAT_BITS - (FLOAT_MANTISSA_BITS - (FLOAT_BITS - FLOAT_SIGN_BITS - DOUBLE_EXPONENT_BITS))));

    hi = signbit | expDbits | fracDbits_hi;
    *lo = fracDbits_lo;

    return hi;
}

#endif // __FLOAT_TO_DOUBLEI_CL__

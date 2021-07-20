/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/*========================== begin_copyright_notice ============================

Copyright (c) 2015 Advanced Micro Devices, Inc.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

============================= end_copyright_notice ===========================*/

#include "../../include/BiF_Definitions.cl"
#include "../../../Headers/spirv.h"
#include "tables.cl"
#include "math.h"

/*
   Algorithm:

   Based on:
   Ping-Tak Peter Tang
   "Table-driven implementation of the logarithm function in IEEE
   floating-point arithmetic"
   ACM Transactions on Mathematical Software (TOMS)
   Volume 16, Issue 4 (December 1990)


   x very close to 1.0 is handled differently, for x everywhere else
   a brief explanation is given below

   x = (2^m)*A
   x = (2^m)*(G+g) with (1 <= G < 2) and (g <= 2^(-8))
   x = (2^m)*2*(G/2+g/2)
   x = (2^m)*2*(F+f) with (0.5 <= F < 1) and (f <= 2^(-9))

   Y = (2^(-1))*(2^(-m))*(2^m)*A
   Now, range of Y is: 0.5 <= Y < 1

   F = 0x80 + (first 7 mantissa bits) + (8th mantissa bit)
   Now, range of F is: 128 <= F <= 256
   F = F / 256
   Now, range of F is: 0.5 <= F <= 1

   f = -(Y-F), with (f <= 2^(-9))

   log(x) = m*log(2) + log(2) + log(F-f)
   log(x) = m*log(2) + log(2) + log(F) + log(1-(f/F))
   log(x) = m*log(2) + log(2*F) + log(1-r)

   r = (f/F), with (r <= 2^(-8))
   r = f*(1/F) with (1/F) precomputed to avoid division

   log(x) = m*log(2) + log(G) - poly

   log(G) is precomputed
   poly = (r + (r^2)/2 + (r^3)/3 + (r^4)/4) + (r^5)/5))

   log(2) and log(G) need to be maintained in extra precision
   to avoid losing precision in the calculations


   For x close to 1.0, we employ the following technique to
   ensure faster convergence.

   log(x) = log((1+s)/(1-s)) = 2*s + (2/3)*s^3 + (2/5)*s^5 + (2/7)*s^7
   x = ((1+s)/(1-s))
   x = 1 + r
   s = r/(2+r)

*/

INLINE float libclc_log2_f32(float x)
{
    const float LOG2E = 0x1.715476p+0f;      // 1.4426950408889634
    const float LOG2E_HEAD = 0x1.700000p+0f; // 1.4375
    const float LOG2E_TAIL = 0x1.547652p-8f; // 0.00519504072

    uint xi = as_uint(x);
    uint ax = xi & EXSIGNBIT_SP32;

    // Calculations for |x-1| < 2^-4
    float r = x - 1.0f;
    int near1 = fabs(r) < 0x1.0p-4f;
    float u2 = MATH_DIVIDE(r, 2.0f + r);
    float corr = u2 * r;
    float u = u2 + u2;
    float v = u * u;
    float znear1, z1, z2;

    // 2/(5 * 2^5), 2/(3 * 2^3)
    z2 = mad(u, mad(v, 0x1.99999ap-7f, 0x1.555556p-4f)*v, -corr);

    z1 = as_float(as_int(r) & 0xffff0000);
    z2 = z2 + (r - z1);
    znear1 = mad(z1, LOG2E_HEAD, mad(z2, LOG2E_HEAD, mad(z1, LOG2E_TAIL, z2*LOG2E_TAIL)));

    // Calculations for x not near 1
    int m = (int)(xi >> EXPSHIFTBITS_SP32) - EXPBIAS_SP32;

    // Normalize subnormal
    uint xis = as_uint(as_float(xi | 0x3f800000) - 1.0f);
    int ms = (int)(xis >> EXPSHIFTBITS_SP32) - 253;
    int c = m == -127;
    m = c ? ms : m;
    uint xin = c ? xis : xi;

    float mf = (float)m;
    uint indx = (xin & 0x007f0000) + ((xin & 0x00008000) << 1);

    // F - Y
    float f = as_float(0x3f000000 | indx) - as_float(0x3f000000 | (xin & MANTBITS_SP32));

    indx = indx >> 16;
    r = f * USE_TABLE(log_inv_tbl, indx);

    // 1/3,  1/2
    float poly = mad(mad(r, 0x1.555556p-2f, 0.5f), r*r, r);

    float2 tv = USE_TABLE(log2_tbl, indx);
    z1 = tv.s0 + mf;
    z2 = mad(poly, -LOG2E, tv.s1);

    float z = z1 + z2;
    z = near1 ? znear1 : z;

    // Corner cases
    z = ax >= PINFBITPATT_SP32 ? x : z;
    z = xi != ax ? as_float(QNANBITPATT_SP32) : z;
    z = ax == 0 ? as_float(NINFBITPATT_SP32) : z;

    return z;
}


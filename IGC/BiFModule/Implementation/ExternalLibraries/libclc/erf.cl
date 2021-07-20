/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/*========================== begin_copyright_notice ============================

Copyright (c) 2014 Advanced Micro Devices, Inc.

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

/*========================== begin_copyright_notice ============================

Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.

Developed at SunPro, a Sun Microsystems, Inc. business.
Permission to use, copy, modify, and distribute this
software is freely granted, provided that this notice
is preserved.

============================= end_copyright_notice ===========================*/

#include "../../include/BiF_Definitions.cl"
#include "../../../Headers/spirv.h"
#include "math.h"

#define erx   8.4506291151e-01f        /* 0x3f58560b */

// Coefficients for approximation to  erf on [00.84375]

#define efx   1.2837916613e-01f        /* 0x3e0375d4 */
#define efx8  1.0270333290e+00f        /* 0x3f8375d4 */

#define pp0   1.2837916613e-01f        /* 0x3e0375d4 */
#define pp1  -3.2504209876e-01f        /* 0xbea66beb */
#define pp2  -2.8481749818e-02f        /* 0xbce9528f */
#define pp3  -5.7702702470e-03f        /* 0xbbbd1489 */
#define pp4  -2.3763017452e-05f        /* 0xb7c756b1 */
#define qq1   3.9791721106e-01f        /* 0x3ecbbbce */
#define qq2   6.5022252500e-02f        /* 0x3d852a63 */
#define qq3   5.0813062117e-03f        /* 0x3ba68116 */
#define qq4   1.3249473704e-04f        /* 0x390aee49 */
#define qq5  -3.9602282413e-06f        /* 0xb684e21a */

// Coefficients for approximation to  erf  in [0.843751.25]

#define pa0  -2.3621185683e-03f        /* 0xbb1acdc6 */
#define pa1   4.1485610604e-01f        /* 0x3ed46805 */
#define pa2  -3.7220788002e-01f        /* 0xbebe9208 */
#define pa3   3.1834661961e-01f        /* 0x3ea2fe54 */
#define pa4  -1.1089469492e-01f        /* 0xbde31cc2 */
#define pa5   3.5478305072e-02f        /* 0x3d1151b3 */
#define pa6  -2.1663755178e-03f        /* 0xbb0df9c0 */
#define qa1   1.0642088205e-01f        /* 0x3dd9f331 */
#define qa2   5.4039794207e-01f        /* 0x3f0a5785 */
#define qa3   7.1828655899e-02f        /* 0x3d931ae7 */
#define qa4   1.2617121637e-01f        /* 0x3e013307 */
#define qa5   1.3637083583e-02f        /* 0x3c5f6e13 */
#define qa6   1.1984500103e-02f        /* 0x3c445aa3 */

// Coefficients for approximation to  erfc in [1.251/0.35]

#define ra0  -9.8649440333e-03f        /* 0xbc21a093 */
#define ra1  -6.9385856390e-01f        /* 0xbf31a0b7 */
#define ra2  -1.0558626175e+01f        /* 0xc128f022 */
#define ra3  -6.2375331879e+01f        /* 0xc2798057 */
#define ra4  -1.6239666748e+02f        /* 0xc322658c */
#define ra5  -1.8460508728e+02f        /* 0xc3389ae7 */
#define ra6  -8.1287437439e+01f        /* 0xc2a2932b */
#define ra7  -9.8143291473e+00f        /* 0xc11d077e */
#define sa1   1.9651271820e+01f        /* 0x419d35ce */
#define sa2   1.3765776062e+02f        /* 0x4309a863 */
#define sa3   4.3456588745e+02f        /* 0x43d9486f */
#define sa4   6.4538726807e+02f        /* 0x442158c9 */
#define sa5   4.2900814819e+02f        /* 0x43d6810b */
#define sa6   1.0863500214e+02f        /* 0x42d9451f */
#define sa7   6.5702495575e+00f        /* 0x40d23f7c */
#define sa8  -6.0424413532e-02f        /* 0xbd777f97 */

// Coefficients for approximation to  erfc in [1/.3528]

#define rb0  -9.8649431020e-03f        /* 0xbc21a092 */
#define rb1  -7.9928326607e-01f        /* 0xbf4c9dd4 */
#define rb2  -1.7757955551e+01f        /* 0xc18e104b */
#define rb3  -1.6063638306e+02f        /* 0xc320a2ea */
#define rb4  -6.3756646729e+02f        /* 0xc41f6441 */
#define rb5  -1.0250950928e+03f        /* 0xc480230b */
#define rb6  -4.8351919556e+02f        /* 0xc3f1c275 */
#define sb1   3.0338060379e+01f        /* 0x41f2b459 */
#define sb2   3.2579251099e+02f        /* 0x43a2e571 */
#define sb3   1.5367296143e+03f        /* 0x44c01759 */
#define sb4   3.1998581543e+03f        /* 0x4547fdbb */
#define sb5   2.5530502930e+03f        /* 0x451f90ce */
#define sb6   4.7452853394e+02f        /* 0x43ed43a7 */
#define sb7  -2.2440952301e+01f        /* 0xc1b38712 */

INLINE float libclc_erf_f32(float x) {
    int hx = as_uint(x);
    int ix = hx & 0x7fffffff;
    float absx = as_float(ix);

    float x2 = absx * absx;
    float t = 1.0f / x2;
    float tt = absx - 1.0f;
    t = absx < 1.25f ? tt : t;
    t = absx < 0.84375f ? x2 : t;

    float u, v, tu, tv;

    // |x| < 6
    u = mad(t, mad(t, mad(t, mad(t, mad(t, mad(t, rb6, rb5), rb4), rb3), rb2), rb1), rb0);
    v = mad(t, mad(t, mad(t, mad(t, mad(t, mad(t, sb7, sb6), sb5), sb4), sb3), sb2), sb1);

    tu = mad(t, mad(t, mad(t, mad(t, mad(t, mad(t, mad(t, ra7, ra6), ra5), ra4), ra3), ra2), ra1), ra0);
    tv = mad(t, mad(t, mad(t, mad(t, mad(t, mad(t, mad(t, sa8, sa7), sa6), sa5), sa4), sa3), sa2), sa1);
    u = absx < 0x1.6db6dcp+1f ? tu : u;
    v = absx < 0x1.6db6dcp+1f ? tv : v;

    tu = mad(t, mad(t, mad(t, mad(t, mad(t, mad(t, pa6, pa5), pa4), pa3), pa2), pa1), pa0);
    tv = mad(t, mad(t, mad(t, mad(t, mad(t, qa6, qa5), qa4), qa3), qa2), qa1);
    u = absx < 1.25f ? tu : u;
    v = absx < 1.25f ? tv : v;

    tu = mad(t, mad(t, mad(t, mad(t, pp4, pp3), pp2), pp1), pp0);
    tv = mad(t, mad(t, mad(t, mad(t, qq5, qq4), qq3), qq2), qq1);
    u = absx < 0.84375f ? tu : u;
    v = absx < 0.84375f ? tv : v;

    v = mad(t, v, 1.0f);
    float q = MATH_DIVIDE(u, v);

    float ret = 1.0f;

    // |x| < 6
    float z = as_float(ix & 0xfffff000);
    float r = exp(mad(-z, z, -0.5625f)) * exp(mad(z-absx, z+absx, q));
    r = 1.0f - MATH_DIVIDE(r,  absx);
    ret = absx < 6.0f ? r : ret;

    r = erx + q;
    ret = absx < 1.25f ? r : ret;

    ret = as_float((hx & 0x80000000) | as_int(ret));

    r = mad(x, q, x);
    ret = absx < 0.84375f ? r : ret;

    // Prevent underflow
    r = 0.125f * mad(8.0f, x, efx8 * x);
    ret = absx < 0x1.0p-28f ? r : ret;

    ret = isnan(x) ? x : ret;

    return ret;
}

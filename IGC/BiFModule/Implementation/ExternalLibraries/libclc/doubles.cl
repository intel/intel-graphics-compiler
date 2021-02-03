
/*
 * Copyright (c) 2014 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "math.h"

double __builtin_spirv_OpenCL_exp_f64(double);
double __builtin_spirv_OpenCL_log_f64(double);
double __builtin_spirv_OpenCL_sinpi_f64(double);

/*################################## libclc_lgamma_r_f64 ###############################################*/

// ====================================================
// Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
//
// Developed at SunPro, a Sun Microsystems, Inc. business.
// Permission to use, copy, modify, and distribute this
// software is freely granted, provided that this notice
// is preserved.
// ====================================================

// lgamma_r(x, i)
// Reentrant version of the __builtin_spirv_OpenCL_log_f64arithm of the Gamma function
// with user provide pointer for the sign of Gamma(x).
//
// Method:
//   1. Argument Reduction for 0 < x <= 8
//      Since gamma(1+s)=s*gamma(s), for x in [0,8], we may
//      reduce x to a number in [1.5,2.5] by
//              lgamma(1+s) = __builtin_spirv_OpenCL_log_f64(s) + lgamma(s)
//      for example,
//              lgamma(7.3) = __builtin_spirv_OpenCL_log_f64(6.3) + lgamma(6.3)
//                          = __builtin_spirv_OpenCL_log_f64(6.3*5.3) + lgamma(5.3)
//                          = __builtin_spirv_OpenCL_log_f64(6.3*5.3*4.3*3.3*2.3) + lgamma(2.3)
//   2. Polynomial approximation of lgamma around its
//      minimun ymin=1.461632144968362245 to maintain monotonicity.
//      On [ymin-0.23, ymin+0.27] (i.e., [1.23164,1.73163]), use
//              Let z = x-ymin;
//              lgamma(x) = -1.214862905358496078218 + z^2*poly(z)
//      where
//              poly(z) is a 14 degree polynomial.
//   2. Rational approximation in the primary interval [2,3]
//      We use the following approximation:
//              s = x-2.0;
//              lgamma(x) = 0.5*s + s*P(s)/Q(s)
//      with accuracy
//              |P/Q - (lgamma(x)-0.5s)| < 2**-61.71
//      Our algorithms are based on the following observation
//
//                             zeta(2)-1    2    zeta(3)-1    3
// lgamma(2+s) = s*(1-Euler) + --------- * s  -  --------- * s  + ...
//                                 2                 3
//
//      where Euler = 0.5771... is the Euler constant, which is very
//      close to 0.5.
//
//   3. For x>=8, we have
//      lgamma(x)~(x-0.5)__builtin_spirv_OpenCL_log_f64(x)-x+0.5*__builtin_spirv_OpenCL_log_f64(2pi)+1/(12x)-1/(360x**3)+....
//      (better formula:
//         lgamma(x)~(x-0.5)*(__builtin_spirv_OpenCL_log_f64(x)-1)-.5*(__builtin_spirv_OpenCL_log_f64(2pi)-1) + ...)
//      Let z = 1/x, then we approximation
//              f(z) = lgamma(x) - (x-0.5)(__builtin_spirv_OpenCL_log_f64(x)-1)
//      by
//                                  3       5             11
//              w = w0 + w1*z + w2*z  + w3*z  + ... + w6*z
//      where
//              |w - f(z)| < 2**-58.74
//
//   4. For negative x, since (G is gamma function)
//              -x*G(-x)*G(x) = pi/sin(pi*x),
//      we have
//              G(x) = pi/(sin(pi*x)*(-x)*G(-x))
//      since G(-x) is positive, sign(G(x)) = sign(sin(pi*x)) for x<0
//      Hence, for x<0, signgam = sign(sin(pi*x)) and
//              lgamma(x) = __builtin_spirv_OpenCL_log_f64(|Gamma(x)|)
//                        = __builtin_spirv_OpenCL_log_f64(pi/(|x*sin(pi*x)|)) - lgamma(-x);
//      Note: one should avoid compute pi*(-x) directly in the
//            computation of sin(pi*(-x)).
//
//   5. Special Cases
//              lgamma(2+s) ~ s*(1-Euler) for tiny s
//              lgamma(1)=lgamma(2)=0
//              lgamma(x) ~ -__builtin_spirv_OpenCL_log_f64(x) for tiny x
//              lgamma(0) = lgamma(inf) = inf
//              lgamma(-integer) = +-inf
//
#define PI 3.14159265358979311600e+00   /* 0x400921FB, 0x54442D18 */

#define A0 7.72156649015328655494e-02   /* 0x3FB3C467, 0xE37DB0C8 */
#define A1 3.22467033424113591611e-01   /* 0x3FD4A34C, 0xC4A60FAD */
#define A2 6.73523010531292681824e-02   /* 0x3FB13E00, 0x1A5562A7 */
#define A3 2.05808084325167332806e-02   /* 0x3F951322, 0xAC92547B */
#define A4 7.38555086081402883957e-03   /* 0x3F7E404F, 0xB68FEFE8 */
#define A5 2.89051383673415629091e-03   /* 0x3F67ADD8, 0xCCB7926B */
#define A6 1.19270763183362067845e-03   /* 0x3F538A94, 0x116F3F5D */
#define A7 5.10069792153511336608e-04   /* 0x3F40B6C6, 0x89B99C00 */
#define A8 2.20862790713908385557e-04   /* 0x3F2CF2EC, 0xED10E54D */
#define A9 1.08011567247583939954e-04   /* 0x3F1C5088, 0x987DFB07 */
#define A10 2.52144565451257326939e-05  /* 0x3EFA7074, 0x428CFA52 */
#define A11 4.48640949618915160150e-05  /* 0x3F07858E, 0x90A45837 */

#define TC 1.46163214496836224576e+00   /* 0x3FF762D8, 0x6356BE3F */
#define TF -1.21486290535849611461e-01  /* 0xBFBF19B9, 0xBCC38A42 */
#define TT -3.63867699703950536541e-18  /* 0xBC50C7CA, 0xA48A971F */

#define T0 4.83836122723810047042e-01   /* 0x3FDEF72B, 0xC8EE38A2 */
#define T1 -1.47587722994593911752e-01  /* 0xBFC2E427, 0x8DC6C509 */
#define T2 6.46249402391333854778e-02   /* 0x3FB08B42, 0x94D5419B */
#define T3 -3.27885410759859649565e-02  /* 0xBFA0C9A8, 0xDF35B713 */
#define T4 1.79706750811820387126e-02   /* 0x3F9266E7, 0x970AF9EC */
#define T5 -1.03142241298341437450e-02  /* 0xBF851F9F, 0xBA91EC6A */
#define T6 6.10053870246291332635e-03   /* 0x3F78FCE0, 0xE370E344 */
#define T7 -3.68452016781138256760e-03  /* 0xBF6E2EFF, 0xB3E914D7 */
#define T8 2.25964780900612472250e-03   /* 0x3F6282D3, 0x2E15C915 */
#define T9 -1.40346469989232843813e-03  /* 0xBF56FE8E, 0xBF2D1AF1 */
#define T10 8.81081882437654011382e-04  /* 0x3F4CDF0C, 0xEF61A8E9 */
#define T11 -5.38595305356740546715e-04 /* 0xBF41A610, 0x9C73E0EC */
#define T12 3.15632070903625950361e-04  /* 0x3F34AF6D, 0x6C0EBBF7 */
#define T13 -3.12754168375120860518e-04 /* 0xBF347F24, 0xECC38C38 */
#define T14 3.35529192635519073543e-04  /* 0x3F35FD3E, 0xE8C2D3F4 */

#define U0 -7.72156649015328655494e-02  /* 0xBFB3C467, 0xE37DB0C8 */
#define U1 6.32827064025093366517e-01   /* 0x3FE4401E, 0x8B005DFF */
#define U2 1.45492250137234768737e+00   /* 0x3FF7475C, 0xD119BD6F */
#define U3 9.77717527963372745603e-01   /* 0x3FEF4976, 0x44EA8450 */
#define U4 2.28963728064692451092e-01   /* 0x3FCD4EAE, 0xF6010924 */
#define U5 1.33810918536787660377e-02   /* 0x3F8B678B, 0xBF2BAB09 */

#define V1 2.45597793713041134822e+00   /* 0x4003A5D7, 0xC2BD619C */
#define V2 2.12848976379893395361e+00   /* 0x40010725, 0xA42B18F5 */
#define V3 7.69285150456672783825e-01   /* 0x3FE89DFB, 0xE45050AF */
#define V4 1.04222645593369134254e-01   /* 0x3FBAAE55, 0xD6537C88 */
#define V5 3.21709242282423911810e-03   /* 0x3F6A5ABB, 0x57D0CF61 */

#define S0 -7.72156649015328655494e-02  /* 0xBFB3C467, 0xE37DB0C8 */
#define S1 2.14982415960608852501e-01   /* 0x3FCB848B, 0x36E20878 */
#define S2 3.25778796408930981787e-01   /* 0x3FD4D98F, 0x4F139F59 */
#define S3 1.46350472652464452805e-01   /* 0x3FC2BB9C, 0xBEE5F2F7 */
#define S4 2.66422703033638609560e-02   /* 0x3F9B481C, 0x7E939961 */
#define S5 1.84028451407337715652e-03   /* 0x3F5E26B6, 0x7368F239 */
#define S6 3.19475326584100867617e-05   /* 0x3F00BFEC, 0xDD17E945 */

#define R1 1.39200533467621045958e+00   /* 0x3FF645A7, 0x62C4AB74 */
#define R2 7.21935547567138069525e-01   /* 0x3FE71A18, 0x93D3DCDC */
#define R3 1.71933865632803078993e-01   /* 0x3FC601ED, 0xCCFBDF27 */
#define R4 1.86459191715652901344e-02   /* 0x3F9317EA, 0x742ED475 */
#define R5 7.77942496381893596434e-04   /* 0x3F497DDA, 0xCA41A95B */
#define R6 7.32668430744625636189e-06   /* 0x3EDEBAF7, 0xA5B38140 */

#define W0 4.18938533204672725052e-01   /* 0x3FDACFE3, 0x90C97D69 */
#define W1 8.33333333333329678849e-02   /* 0x3FB55555, 0x5555553B */
#define W2 -2.77777777728775536470e-03  /* 0xBF66C16C, 0x16B02E5C */
#define W3 7.93650558643019558500e-04   /* 0x3F4A019F, 0x98CF38B6 */
#define W4 -5.95187557450339963135e-04  /* 0xBF4380CB, 0x8C0FE741 */
#define W5 8.36339918996282139126e-04   /* 0x3F4B67BA, 0x4CDAD5D1 */
#define W6 -1.63092934096575273989e-03  /* 0xBF5AB89D, 0x0B9E43E4 */

INLINE double libclc_lgamma_r_f64(double x, private int* ip) {
    ulong ux = as_ulong(x);
    ulong ax = ux & EXSIGNBIT_DP64;
    double absx = as_double(ax);

    if (ax >= 0x7ff0000000000000UL) {
        // +-Inf, NaN
        *ip = 1;
        return absx;
    }

    if (absx < 0x1.0p-70) {
        *ip = ax == ux ? 1 : -1;
        return -__builtin_spirv_OpenCL_log_f64(absx);
    }

    // Handle rest of range
    double r;

    if (absx < 2.0) {
        int i = 0;
        double y = 2.0 - absx;

        int c = absx < 0x1.bb4c3p+0;
        double t = absx - TC;
        i = c ? 1 : i;
        y = c ? t : y;

        c = absx < 0x1.3b4c4p+0;
        t = absx - 1.0;
        i = c ? 2 : i;
        y = c ? t : y;

        c = absx <= 0x1.cccccp-1;
        t = -__builtin_spirv_OpenCL_log_f64(absx);
        r = c ? t : 0.0;
        t = 1.0 - absx;
        i = c ? 0 : i;
        y = c ? t : y;

        c = absx < 0x1.76944p-1;
        t = absx - (TC - 1.0);
        i = c ? 1 : i;
        y = c ? t : y;

        c = absx < 0x1.da661p-3;
        i = c ? 2 : i;
        y = c ? absx : y;

        double p, q;

        switch (i) {
        case 0:
            p = __builtin_spirv_OpenCL_fma_f64_f64_f64(y, __builtin_spirv_OpenCL_fma_f64_f64_f64(y, __builtin_spirv_OpenCL_fma_f64_f64_f64(y, __builtin_spirv_OpenCL_fma_f64_f64_f64(y, A11, A10), A9), A8), A7);
            p = __builtin_spirv_OpenCL_fma_f64_f64_f64(y, __builtin_spirv_OpenCL_fma_f64_f64_f64(y, __builtin_spirv_OpenCL_fma_f64_f64_f64(y, __builtin_spirv_OpenCL_fma_f64_f64_f64(y, p, A6), A5), A4), A3);
            p = __builtin_spirv_OpenCL_fma_f64_f64_f64(y, __builtin_spirv_OpenCL_fma_f64_f64_f64(y, __builtin_spirv_OpenCL_fma_f64_f64_f64(y, p, A2), A1), A0);
            r = __builtin_spirv_OpenCL_fma_f64_f64_f64(y, p - 0.5, r);
            break;
        case 1:
            p = __builtin_spirv_OpenCL_fma_f64_f64_f64(y, __builtin_spirv_OpenCL_fma_f64_f64_f64(y, __builtin_spirv_OpenCL_fma_f64_f64_f64(y, __builtin_spirv_OpenCL_fma_f64_f64_f64(y, T14, T13), T12), T11), T10);
            p = __builtin_spirv_OpenCL_fma_f64_f64_f64(y, __builtin_spirv_OpenCL_fma_f64_f64_f64(y, __builtin_spirv_OpenCL_fma_f64_f64_f64(y, __builtin_spirv_OpenCL_fma_f64_f64_f64(y, __builtin_spirv_OpenCL_fma_f64_f64_f64(y, p, T9), T8), T7), T6), T5);
            p = __builtin_spirv_OpenCL_fma_f64_f64_f64(y, __builtin_spirv_OpenCL_fma_f64_f64_f64(y, __builtin_spirv_OpenCL_fma_f64_f64_f64(y, __builtin_spirv_OpenCL_fma_f64_f64_f64(y, __builtin_spirv_OpenCL_fma_f64_f64_f64(y, p, T4), T3), T2), T1), T0);
            p = __builtin_spirv_OpenCL_fma_f64_f64_f64(y * y, p, -TT);
            r += (TF + p);
            break;
        case 2:
            p = y * __builtin_spirv_OpenCL_fma_f64_f64_f64(y, __builtin_spirv_OpenCL_fma_f64_f64_f64(y, __builtin_spirv_OpenCL_fma_f64_f64_f64(y, __builtin_spirv_OpenCL_fma_f64_f64_f64(y, __builtin_spirv_OpenCL_fma_f64_f64_f64(y, U5, U4), U3), U2), U1), U0);
            q = __builtin_spirv_OpenCL_fma_f64_f64_f64(y, __builtin_spirv_OpenCL_fma_f64_f64_f64(y, __builtin_spirv_OpenCL_fma_f64_f64_f64(y, __builtin_spirv_OpenCL_fma_f64_f64_f64(y, __builtin_spirv_OpenCL_fma_f64_f64_f64(y, V5, V4), V3), V2), V1), 1.0);
            r += __builtin_spirv_OpenCL_fma_f64_f64_f64(-0.5, y, p / q);
        }
    }
    else if (absx < 8.0) {
        int i = absx;
        double y = absx - (double)i;
        double p = y * __builtin_spirv_OpenCL_fma_f64_f64_f64(y, __builtin_spirv_OpenCL_fma_f64_f64_f64(y, __builtin_spirv_OpenCL_fma_f64_f64_f64(y, __builtin_spirv_OpenCL_fma_f64_f64_f64(y, __builtin_spirv_OpenCL_fma_f64_f64_f64(y, __builtin_spirv_OpenCL_fma_f64_f64_f64(y, S6, S5), S4), S3), S2), S1), S0);
        double q = __builtin_spirv_OpenCL_fma_f64_f64_f64(y, __builtin_spirv_OpenCL_fma_f64_f64_f64(y, __builtin_spirv_OpenCL_fma_f64_f64_f64(y, __builtin_spirv_OpenCL_fma_f64_f64_f64(y, __builtin_spirv_OpenCL_fma_f64_f64_f64(y, __builtin_spirv_OpenCL_fma_f64_f64_f64(y, R6, R5), R4), R3), R2), R1), 1.0);
        r = __builtin_spirv_OpenCL_fma_f64_f64_f64(0.5, y, p / q);
        double z = 1.0;
        // lgamma(1+s) = __builtin_spirv_OpenCL_log_f64(s) + lgamma(s)
        double y6 = y + 6.0;
        double y5 = y + 5.0;
        double y4 = y + 4.0;
        double y3 = y + 3.0;
        double y2 = y + 2.0;
        z *= i > 6 ? y6 : 1.0;
        z *= i > 5 ? y5 : 1.0;
        z *= i > 4 ? y4 : 1.0;
        z *= i > 3 ? y3 : 1.0;
        z *= i > 2 ? y2 : 1.0;
        r += __builtin_spirv_OpenCL_log_f64(z);
    }
    else {
        double z = 1.0 / absx;
        double z2 = z * z;
        double w = __builtin_spirv_OpenCL_fma_f64_f64_f64(z, __builtin_spirv_OpenCL_fma_f64_f64_f64(z2, __builtin_spirv_OpenCL_fma_f64_f64_f64(z2, __builtin_spirv_OpenCL_fma_f64_f64_f64(z2, __builtin_spirv_OpenCL_fma_f64_f64_f64(z2, __builtin_spirv_OpenCL_fma_f64_f64_f64(z2, W6, W5), W4), W3), W2), W1), W0);
        r = (absx - 0.5) * (__builtin_spirv_OpenCL_log_f64(absx) - 1.0) + w;
    }

    if (x < 0.0) {
        double t = __builtin_spirv_OpenCL_sinpi_f64(x);
        r = __builtin_spirv_OpenCL_log_f64(PI / __builtin_spirv_OpenCL_fabs_f64(t * x)) - r;
        r = t == 0.0 ? as_double(PINFBITPATT_DP64) : r;
        *ip = t < 0.0 ? -1 : 1;
    }
    else
        *ip = 1;

    return r;
}

/*################################## libclc_lgamma_f64 ###############################################*/

INLINE double libclc_lgamma_f64(double x) {
    int s;
    return libclc_lgamma_r_f64(x, &s);
}

#undef PI

#undef A0
#undef A1
#undef A2
#undef A3
#undef A4
#undef A5
#undef A6
#undef A7
#undef A8
#undef A9
#undef A10
#undef A11

#undef TC
#undef TF
#undef TT

#undef T0
#undef T1
#undef T2
#undef T3
#undef T4
#undef T5
#undef T6
#undef T7
#undef T8
#undef T9
#undef T10
#undef T11
#undef T12
#undef T13
#undef T14

#undef U0
#undef U1
#undef U2
#undef U3
#undef U4
#undef U5

#undef V1
#undef V2
#undef V3
#undef V4
#undef V5

#undef S0
#undef S1
#undef S2
#undef S3
#undef S4
#undef S5
#undef S6

#undef R1
#undef R2
#undef R3
#undef R4
#undef R5
#undef R6

#undef W0
#undef W1
#undef W2
#undef W3
#undef W4
#undef W5
#undef W6

/*################################## libclc_tgamma_f64 ###############################################*/

INLINE double libclc_tgamma_f64(double x) {
    const double pi = 3.1415926535897932384626433832795;
    double ax = __builtin_spirv_OpenCL_fabs_f64(x);
    double lg = libclc_lgamma_f64(ax);
    double g = __builtin_spirv_OpenCL_exp_f64(lg);

    if (x < 0.0) {
        double z = __builtin_spirv_OpenCL_sinpi_f64(x);
        g = g * ax * z;
        g = pi / g;
        g = g == 0 ? as_double(PINFBITPATT_DP64) : g;
        g = z == 0 ? as_double(QNANBITPATT_DP64) : g;
    }

    return g;
}
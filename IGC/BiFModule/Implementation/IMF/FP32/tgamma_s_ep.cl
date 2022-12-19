/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF

/*
//++
//
//  ALGORITHM DESCRIPTION
//  ---------------------
// The method consists of three cases.
//
// If       2 <= x < OVERFLOW_BOUNDARY
// else if  0 < x < 2
// else if  -(i+1) <  x < -i, i = 0...43
//
// Case 2 <= x < OVERFLOW_BOUNDARY
// -------------------------------
//   Here we use algorithm based on the recursive formula
//   GAMMA(x+1) = x*GAMMA(x). For that we subdivide interval
//   [2; OVERFLOW_BOUNDARY] into intervals [8*n; 8*(n+1)] and
//   approximate GAMMA(x) by polynomial of 22th degree on each
//   [8*n; 8*n+1], recursive formula is used to expand GAMMA(x)
//   to [8*n; 8*n+1]. In other words we need to find n, i and r
//   such that x = 8 * n + i + r where n and i are integer numbers
//   and r is fractional part of x. So GAMMA(x) = GAMMA(8*n+i+r) =
//   = (x-1)*(x-2)*...*(x-i)*GAMMA(x-i) =
//   = (x-1)*(x-2)*...*(x-i)*GAMMA(8*n+r) ~
//   ~ (x-1)*(x-2)*...*(x-i)*P12n(r).
//
//   Step 1: Reduction
//   -----------------
//    N = [x] with truncate
//    r = x - N, note 0 <= r < 1
//
//    n = N & ~0x7 - index of table that contains coefficient of
//                   polynomial approximation
//    i = N & 0x7  - is used in recursive formula
//
//
//   Step 2: Approximation
//   ---------------------
//    We use factorized minimax approximation polynomials
//    P12n(r) = A12*(r^2+C01(n)*r+C00(n))*
//              *(r^2+C11(n)*r+C10(n))*...*(r^2+C51(n)*r+C50(n))
//
//   Step 3: Recursion
//   -----------------
//    In case when i > 0 we need to multiply P12n(r) by product
//    R(i,x)=(x-1)*(x-2)*...*(x-i). To reduce number of fp-instructions
//    we can calculate R as follow:
//    R(i,x) = ((x-1)*(x-2))*((x-3)*(x-4))*...*((x-(i-1))*(x-i)) if i is
//    even or R = ((x-1)*(x-2))*((x-3)*(x-4))*...*((x-(i-2))*(x-(i-1)))*
//    *(i-1) if i is odd. In both cases we need to calculate
//    R2(i,x) = (x^2-3*x+2)*(x^2-7*x+12)*...*(x^2+x+2*j*(2*j-1)) =
//    = ((x^2-x)+2*(1-x))*((x^2-x)+6*(2-x))*...*((x^2-x)+2*(2*j-1)*(j-x)) =
//    = (RA+2*RB)*(RA+6*(1-RB))*...*(RA+2*(2*j-1)*(j-1+RB))
//    where j = 1..[i/2], RA = x^2-x, RB = 1-x.
//
//   Step 4: Reconstruction
//   ----------------------
//    Reconstruction is just simple multiplication i.e.
//    GAMMA(x) = P12n(r)*R(i,x)
//
// Case 0 < x < 2
// --------------
//    To calculate GAMMA(x) on this interval we do following
//        if 1.0  <= x < 1.25  than  GAMMA(x) = P7(x-1)
//        if 1.25 <= x < 1.5   than  GAMMA(x) = P7(x-x_min) where
//              x_min is point of local minimum on [1; 2] interval.
//        if 1.5  <= x < 1.75  than  GAMMA(x) = P7(x-1)
//        if 1.75 <= x < 2.0   than  GAMMA(x) = P7(x-1)
//    and
//        if 0 < x < 1 than GAMMA(x) = GAMMA(x+1)/x
//
// Case -(i+1) <  x < -i, i = 0...43
// ----------------------------------
//    Here we use the fact that GAMMA(-x) = PI/(x*GAMMA(x)*sin(PI*x)) and
//    so we need to calculate GAMMA(x), sin(PI*x)/PI. Calculation of
//    GAMMA(x) is described above.
//
//   Step 1: Reduction
//   -----------------
//    Note that period of sin(PI*x) is 2 and range reduction for
//    sin(PI*x) is like to range reduction for GAMMA(x)
//    i.e rs = x - round(x) and |rs| <= 0.5.
//
//   Step 2: Approximation
//   ---------------------
//    To approximate sin(PI*x)/PI = sin(PI*(2*n+rs))/PI =
//    = (-1)^n*sin(PI*rs)/PI Taylor series is used.
//    sin(PI*rs)/PI ~ S17(rs).
//
//   Step 3: Division
//   ----------------
//    1/x and 1/(GAMMA(x)*S12(rs))
//--
*/

//
// Static data section:
//

// negative values underflow range
static __constant float __stgamma_ep__neg_underflow[2] = { 43.0, -43.0 };

// negative values "half" overflow range - multiply by 1/An
static __constant float __stgamma_ep__neg_half_overflow[2] = { 40.0, -40.0 };

// overflow boundary (35.04010009765625)
static __constant unsigned int __stgamma_ep__overflowf_boundary[] = {
    0x420c2910, //       35.0401001
};

// point of local minium (0.461632144968362356785)
static __constant unsigned int __stgamma_ep__local_minimumf[] = {
    0x3eec5b0c, //      0.461632133
};

static __constant unsigned int __stgamma_ep__tgammaf_A_table[] = {
    0x404f7e8e, //       3.24209929
    0x3ffd9a9c, //        1.9812808
    0x3ece7348, //      0.403223276
    0xbfc61856, //      -1.54761767
    0xc0819138, //       -4.0489769
    0xc0f8b8d2, //      -7.77256107
    0x4070be98, //       3.76163292
    0x40834492, //        4.1021204
    0x40a47c71, //        5.1401906
    0x40e300cb, //        7.0938468
    0x412b18a4, //       10.6935158
    0x419a0368, //       19.2516632
    0x3710d6c4, //   8.63307287e-06
    //
    0x4031115d, //       2.76668477
    0x4012293a, //       2.28376627
    0x3faa2c14, //       1.32947016
    0xbdde9083, //     -0.108674072
    0xc008779a, //       -2.1322999
    0xc0a495c1, //      -5.14328051
    0x4002a105, //       2.04107785
    0x40196098, //       2.39652061
    0x404ce483, //       3.20144725
    0x4095cb95, //       4.68110132
    0x40eb998f, //       7.36249495
    0x41503404, //       13.0126991
    0x3f37b6dd, //      0.717634022
    //
    0x40009880, //       2.00930786
    0x3fd51b17, //       1.66488922
    0x3f758f9a, //      0.959222436
    0xbe1ddf37, //      -0.15417181
    0xbfe5b4c3, //      -1.79457891
    0xc08adf72, //      -4.33977604
    0x3f8a703f, //       1.08155048
    0x3fa9dbb6, //       1.32701755
    0x3ff317fc, //       1.89916945
    0x403f19a4, //       2.98593998
    0x40a028f0, //       5.00499725
    0x4114ea11, //       9.30714512
    0x4f4d95ba, //   3.44914176e+09
    //
    0x3fd7bdea, //       1.68548322
    0x3fb156c6, //       1.38546062
    0x3f44a1e6, //      0.768095374
    0xbe59cea3, //     -0.212702319
    0xbfd5e56e, //      -1.67106414
    0xc07d6f4c, //      -3.95991802
    0x3f43cd7a, //      0.764854074
    0x3f763ef6, //      0.961898208
    0x3fb6d4f7, //       1.42837417
    0x401514f5, //       2.32940412
    0x4080bdbd, //       4.02316141
    0x40f4a0ee, //       7.64464474
    0x6194eadb, //   3.43380155e+20
    //
    0x3fbf659d, //       1.49528849
    0x3f9c0ad4, //       1.21908045
    0x3f2673b7, //      0.650203168
    0xbe82926e, //      -0.25502342
    0xbfcd5ec8, //      -1.60445499
    0xc06ec4cb, //      -3.73076129
    0x3f1ac297, //      0.604531705
    0x3f463b08, //      0.774338245
    0x3f97218a, //       1.18071103
    0x3ffcb6e2, //       1.97433114
    0x405e7ba8, //       3.47629738
    0x40d60a70, //       6.68877411
    0x757fa655, //   3.24074539e+32
    //
    0x3faee727, //       1.36642921
    0x3f8d86c6, //       1.10567546
    0x3f118ef6, //      0.568587661
    0xbe928f65, //     -0.286250263
    0xbfc7da28, //       -1.5613451
    0xc064b2c7, //       -3.5734117
    0x3f01b457, //      0.506658018
    0x3f289d72, //      0.658652425
    0x3f834429, //       1.02551758
    0x3fdfbe3e, //       1.74799323
    0x4047cb08, //       3.12176704
    0x40c1dd04, //       6.05822945
    0x3f800000, //                1
};

// sin(pi*x)/pi
static __constant unsigned int __stgamma_ep__tgammaf_sin_table[] = {
    0xb60a2594, //  -2.05854758e-06
    0x3487e4c9, //   2.53121726e-07
    0x42aa0ebd, //       85.0287857
    0xc137d86a, //      -11.4903355
    0x43672c7a, //       231.173737
    0x3ffd8afd, //       1.98080409
    0x42229bc3, //       40.6521111
    0xc11eacd2, //      -9.91719246
};

static __constant unsigned int __stgamma_ep__tgammaf_A100_table[] = {
    0x3f800000, //                1
    0xbf13c466, //     -0.577215552
    0x3f7d3247, //      0.989048421
    0xbf684007, //      -0.90722698
    0x3f7a42bd, //      0.977580845
    0xbf71b908, //     -0.944229603
    0x3f4b7ff7, //      0.794921339
    0xbecc84b2, //     -0.399449885
};

static __constant unsigned int __stgamma_ep__tgammaf_A125_table[] = {
    0x3f62b6e4, //      0.885603189
    0xaf74351d, //  -2.22105404e-10
    0x3edb62a3, //      0.428486913
    0xbe05d6f4, //     -0.130702794
    0x3e24b501, //      0.160846725
    0xbdbf3fc2, //    -0.0933833271
    0x3d791ba9, //     0.0608173944
    0xbda1091b, //    -0.0786306486
};

static __constant unsigned int __stgamma_ep__tgammaf_A150_table[] = {
    0x3f7fe30a, //      0.999558091
    0xbf122cb9, //     -0.570994914
    0x3f732aac, //      0.949869871
    0xbf42da14, //     -0.761140108
    0x3f1e4866, //      0.618292212
    0xbeace98e, //     -0.337719381
    0x3dfa101f, //      0.122101061
    0xbca3985b, //    -0.0199701097
};

static __constant unsigned int __stgamma_ep__tgammaf_A175_table[] = {
    0x3f7f70f3, //      0.997817218
    0xbf0e0d77, //     -0.554892957
    0x3f62b540, //      0.885578156
    0xbf1e1256, //     -0.617467284
    0x3ed9316d, //      0.424205214
    0xbe378738, //     -0.179226756
    0x3d4b71bf, //     0.0496690236
    0xbbba3543, //   -0.00568261882
};

// Right shifter

static __constant unsigned __stgamma_ep__two_23h[] = { 0x4b000000 };    // 2^23

// Special values

static __constant unsigned int __stgamma_ep__own_large_value_32[] = { 0x71800000, 0xf1800000 }; // +2^100,-2^100

static __constant unsigned int __stgamma_ep__own_small_value_32[] = { 0x0d800000, 0x8d800000 }; // +2^(-100),-2^(-100)

// constants

static __constant unsigned int __stgamma_ep__zeros[] = { 0x00000000, 0x80000000 };
static __constant unsigned int __stgamma_ep__ones[] = { 0x3f800000, 0xbf800000 };
static __constant unsigned int __stgamma_ep__infs[] = { 0x7f800000, 0xff800000 };
static __constant float __stgamma_ep_twos[] = { 2.0, -2.0 };
static __constant float __stgamma_ep_ones_5[] = { 1.5, -1.5 };
static __constant float __stgamma_ep_ones_25[] = { 1.25, -1.25 };
static __constant float __stgamma_ep_ones_75[] = { 1.75, -1.75 };

#pragma float_control(precise,on)

__attribute__((always_inline))
inline int __internal_stgamma_ep_cout (float *a, float *r)
{
    int nRet = 0;
    int t = 0, i = 0, j = 0, irsign = 0;
    int ix = 0, iabsx_n = 0, iabsx_t = 0, ixsign = 0, ixexp = 0;
    float dix = 0.0f, diabsx_n = 0.0f, diabsx_t = 0.0f;
    float tv = 0.0f;
    float x = 0.0f;
    float absx = 0.0f, res = 0.0f;

    float resf = 0.0f;
    float s = 0.0f, r2 = 0.0f, r3 = 0.0f, rrr = 0.0f;
    float curabsx = 0.0f;
    float p = 0.0f, pr = 0.0f;
    __constant float *A;
    __constant float *Af;
    x = *(a);
    absx = x;
    res = ((__constant float *) __stgamma_ep__zeros)[0];

    // get arg sign
    ixsign = (((_iml_sp_union_t *) & x)->hex[0] >> 31);
    // get arg exponent
    ixexp = ((((_iml_sp_union_t *) & x)->hex[0] >> 23) & 0xFF);

    // normal values
    if (ixexp != 0xFF)
    {
        // create absolute value
        (((_iml_sp_union_t *) & absx)->hex[0] = (((_iml_sp_union_t *) & absx)->hex[0] & 0x7FFFFFFF) | ((_iml_uint32_t) (0) << 31));

        ix = *((int *) (&absx));

        // if x == 0 - zero divide exception
        if (x == ((__constant float *) __stgamma_ep__zeros)[0])
        {
            *r = (((__constant float *) __stgamma_ep__ones)[(ixsign)] / ((__constant float *) __stgamma_ep__zeros)[0]);
            nRet = 2;
            return nRet;
        }

        if (ix <= 0x00200000)   // if |x| < denorm_overflow
        {
            {
                float tz = ((__constant float *) __stgamma_ep__own_large_value_32)[0];
                ((*r)) = (((__constant float *) __stgamma_ep__own_large_value_32)[(ixsign)] * tz);
            };  // raise overflow
            nRet = 3;
            return nRet;
        }

        // singularity at negative integer points
        if (ixsign)
        {
            // if |x| >= 2^23 - only integer values
            if (ixexp >= 0x00000096)
            {
                {
                    float tz = ((__constant float *) __stgamma_ep__zeros)[0];
                    ((*r)) = (((__constant float *) __stgamma_ep__zeros)[(0)] / tz);
                };
                nRet = 1;
                return nRet;
            }   // if(ixexp >= 0x00000096)
            else
            {
                // get integer value of arg (truncated)
                tv = absx + (*(__constant float *) __stgamma_ep__two_23h);
                diabsx_t = tv - (*(__constant float *) __stgamma_ep__two_23h);
                iabsx_t = (0x000fffff) & (*((int *) (&tv)));
                if (diabsx_t > absx)
                {
                    diabsx_t -= ((__constant float *) __stgamma_ep__ones)[0];
                    iabsx_t -= 1;
                }
            }   // else if(ixexp >= 0x00000096)

            // if arg - integer then singularity
            if (absx == diabsx_t)
            {
                {
                    float tz = ((__constant float *) __stgamma_ep__zeros)[0];
                    ((*r)) = (((__constant float *) __stgamma_ep__zeros)[(0)] / tz);
                };
                nRet = 1;
                return nRet;
            }

            // if arg < -185.0 then underflow (values rounded to zero)
            if (x < __stgamma_ep__neg_underflow[1])
            {
                (*r) = (((__constant float *) __stgamma_ep__own_small_value_32)[((~iabsx_t) & 1)] * ((__constant float *) __stgamma_ep__own_small_value_32)[0]);    // raise underflow and inexact
                nRet = 4;
                return nRet;
            }
        }   // if(ixsign)

        // big positive values overflow domain (res rounded to INF)
        if (x >= (*((__constant float *) __stgamma_ep__overflowf_boundary)))
        {
            {
                float tz = ((__constant float *) __stgamma_ep__own_large_value_32)[0];
                ((*r)) = (((__constant float *) __stgamma_ep__own_large_value_32)[(0)] * tz);
            };  // raise overflow and inexact
            nRet = 3;
            return nRet;
        }

        // compute sin(Pi*x)/x for negative values
        if (ixsign)
        {
            // get rounded to nearest abs arg
            tv = absx + (*(__constant float *) __stgamma_ep__two_23h);
            diabsx_n = tv - (*(__constant float *) __stgamma_ep__two_23h);
            iabsx_n = (0x000fffff) & (*((int *) (&tv)));

            rrr = absx - diabsx_n;  // reduced argument

            if (rrr < 0)
                rrr = (-rrr);   //IML_ABS_DP(rrr);  // remove sign

            r2 = rrr * rrr; // rrr^2

            // Tailor series
            s = rrr +
                rrr *
                ((r2 * (((__constant float *) __stgamma_ep__tgammaf_sin_table)[0] + r2 * ((__constant float *) __stgamma_ep__tgammaf_sin_table)[1])) *
                 (((__constant float *) __stgamma_ep__tgammaf_sin_table)[2] +
                  r2 * (r2 +
                        ((__constant float *) __stgamma_ep__tgammaf_sin_table)[3])) * (((__constant float *) __stgamma_ep__tgammaf_sin_table)[4] +
                                                                                       r2 * (r2 +
                                                                                             ((__constant float *)
                                                                                              __stgamma_ep__tgammaf_sin_table)[5])) *
                 (((__constant float *) __stgamma_ep__tgammaf_sin_table)[6] + r2 * (r2 + ((__constant float *) __stgamma_ep__tgammaf_sin_table)[7])));
        }   // if(ixsign)

        // get truncated integer argument
        tv = absx + (*(__constant float *) __stgamma_ep__two_23h);
        diabsx_t = tv - (*(__constant float *) __stgamma_ep__two_23h);
        iabsx_t = (0x000fffff) & (*((int *) (&tv)));

        if (diabsx_t > absx)
        {
            diabsx_t -= ((__constant float *) __stgamma_ep__ones)[0];
            iabsx_t -= 1;
        }
        // get result sign
        irsign = ((iabsx_t + 1) & 1);
        // if x > 2.0 - simple polynomials
        if (absx >= __stgamma_ep_twos[0])
        {
            t = iabsx_t & (~0x7);   // index of table of coefficient
            i = iabsx_t & (0x7);    // used in recursive formula
            // for 2 <= x < 8 - shift index
            if (iabsx_t < 8)
                i = i - 2;

            rrr = absx - diabsx_t;  // reduced argument
            A = &(((__constant float *) __stgamma_ep__tgammaf_A_table)[t + (t >> 1) + (t >> 3)]);   // table address
            r2 = rrr * rrr; // rrr^2

            // factorized polynomial
            p = A[12] * (r2 + A[0] * rrr + A[6 + 0]) *
                (r2 + A[1] * rrr + A[6 + 1]) *
                (r2 + A[2] * rrr + A[6 + 2]) * (r2 + A[3] * rrr + A[6 + 3]) * (r2 + A[4] * rrr + A[6 + 4]) * (r2 + A[5] * rrr + A[6 + 5]);

            // if no recursion - p = 1.0
            pr = ((__constant float *) __stgamma_ep__ones)[0];
            // if i > 0 - recursies
            if (i)
            {
                for (j = 1; j <= i; j++)
                {
                    pr *= (absx - j);
                }
            }

            if (ixsign) // for negatives rrr = 1/(x*s*gamma*recursies)
            {
                //[40; 48] for negatives

                unsigned int __stgamma_ep__tgamma_A40_inv[] = {
                    0xEDBCC440, 0x368954EA  // 1/An
                };

                double resd = (double) ((__constant float *) __stgamma_ep__ones)[0] / ((double) absx * s * p * pr);
                if (x < __stgamma_ep__neg_half_overflow[1])
                {
                    resd *= (*((double *) __stgamma_ep__tgamma_A40_inv));
                }
                // set sign
                if (irsign)
                    resd = -resd;
                (*r) = (float) resd;
            }
            else    // for positives rrr = gamma*recursies
            {
                (*r) = p * pr;
            }

            return nRet;
        }   // if(absx >= _VSTATIC(twos)[0])
        else
        {
            // if |x| < 1 - calculate gamma(x+1)
            if (absx < ((__constant float *) __stgamma_ep__ones)[0])
            {
                curabsx = absx + (((__constant float *) __stgamma_ep__ones)[0]);
            }
            else
            {
                curabsx = absx;
            }

            // splitted intervals:
            // x >= 1.75
            if (curabsx >= __stgamma_ep_ones_75[0])
            {
                rrr = curabsx - (((__constant float *) __stgamma_ep__ones)[0]);
                A = ((__constant float *) __stgamma_ep__tgammaf_A175_table);
            }
            else if (curabsx >= __stgamma_ep_ones_5[0]) // x >= 1.5
            {
                rrr = curabsx - (((__constant float *) __stgamma_ep__ones)[0]);
                A = ((__constant float *) __stgamma_ep__tgammaf_A150_table);
            }
            else if (curabsx >= __stgamma_ep_ones_25[0])    // 1.5 > x >= 1.25
            {
                rrr = curabsx - ((((__constant float *) __stgamma_ep__ones)[0]) + (*((__constant float *) __stgamma_ep__local_minimumf)));
                A = ((__constant float *) __stgamma_ep__tgammaf_A125_table);
            }
            else if (curabsx < __stgamma_ep_ones_25[0]) // 0 < x < 1.25
            {
                rrr = curabsx - (((__constant float *) __stgamma_ep__ones)[0]);
                A = ((__constant float *) __stgamma_ep__tgammaf_A100_table);
            }

            if (ixexp)  // for normal values - compute whole polynomial
            {
                p = A[0] + rrr * (A[1] + rrr * (A[2] + rrr * (A[3] + rrr * (A[4] + rrr * (A[5] + rrr * (A[6] + rrr * (A[7])))))));
            }
            else    // for denormal - return just A[0]
            {
                p = A[0];
            }

            if (absx < ((__constant float *) __stgamma_ep__ones)[0])    // |x| < 1.0
            {
                if (ixsign) // if x < 0 then rrr = 1/(s*p)
                {
                    resf = (((__constant float *) __stgamma_ep__ones)[0]) / (s * p);
                    if (irsign)
                        resf = -resf;
                }
                else    // if x > 0 then rrr = p/x
                {
                    resf = p / (absx);
                }
            }   // if(absx < ones[0]) // |x| < 1.0
            else    // |x| > 1.0
            {
                if (ixsign) // rrr = 1/(x*s*p);
                {
                    resf = (((__constant float *) __stgamma_ep__ones)[0]) / ((absx) * s * p);
                }
                else    // rrr = p
                {
                    resf = p;
                }
            }   // else if(absx < ones[0]) // |x| < 1.0

            (*r) = (float) resf;
            return nRet;
        }   // else if(absx >= _VSTATIC(twos)[0])
    }   // if (ixexp != IML_EXPINF_32)
    else    // INF or NAN
    {
        // Singularity at negative INF
        if (ixsign && (!((((_iml_sp_union_t *) & x)->hex[0] & 0x007FFFFF) != 0)))
        {
            {
                float tz = ((__constant float *) __stgamma_ep__zeros)[0];
                ((*r)) = (((__constant float *) __stgamma_ep__zeros)[(1)] / tz);
            };
            nRet = 1;
            return nRet;
        }
        else
        {
            (*r) = x + x;   // raise invalid on SNaN
            return nRet;
        }
    }   // else if (ixexp != IML_EXPINF_32)
}   // inline int __internal_stgamma_ep_cout (float *a, float *r)

float __ocl_svml_tgammaf (float a)
{
    float va1;
    float vr1;
    float r;

    va1 = a;
    __internal_stgamma_ep_cout (&va1, &vr1);
    r = vr1;

    return r;
}

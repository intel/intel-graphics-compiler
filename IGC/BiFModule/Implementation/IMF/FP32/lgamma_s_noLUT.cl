/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF

/*
//++
//  ALGORITHM DESCRIPTION
//  ---------------------
//
//  Case 2^13 <= x < OVERFLOW_BOUNDARY
//  ----------------------------------
//    Here we use algorithm based on the Stirling formula:
//      ln(GAMMA(x)) = ln(sqrt(2*Pi)) + (x-0.5)*ln(x) - x
//
//  Case 1 < x < 2^13
//  -----------------
//    To calculate ln(GAMMA(x)) for such arguments we use polynomial
//    approximation on following intervals: [1.0; 1.25), [1.25; 1.5),
//    [1.5, 1.75), [1.75; 2), [2; 4), [2^i; 2^(i+1)), i=1..8
//
//    Following variants of approximation and argument reduction are used:
//     1. [1.0; 1.25)
//        ln(GAMMA(x)) ~ (x-1.0)*P7(x)
//
//     2. [1.25; 1.5)
//        ln(GAMMA(x)) ~ ln(GAMMA(x0))+(x-x0)*P7(x-x0),
//        where x0 - point of local minimum on [1;2] rounded to nearest double
//        precision number.
//
//     3. [1.5; 1.75)
//        ln(GAMMA(x)) ~ P8(x)
//
//     4. [1.75; 2.0)
//        ln(GAMMA(x)) ~ (x-2)*P7(x)
//
//     5. [2; 4)
//        ln(GAMMA(x)) ~ (x-2)*P10(x)
//
//     6. [2^i; 2^(i+1)), i=2..8
//        ln(GAMMA(x)) ~ P10((x-2^i)/2^i)
//
//  Case -9 < x < 1
//  ---------------
//    Here we use the recursive formula:
//    ln(GAMMA(x)) = ln(GAMMA(x+1)) - ln(x)
//
//    Using this formula we reduce argument to base interval [1.0; 2.0]
//
//  Case -2^13 < x < -9
//  --------------------
//    Here we use the formula:
//    ln(GAMMA(x)) = ln(Pi/(|x|*GAMMA(|x|)*sin(Pi*|x|))) =
//    = -ln(|x|) - ln((GAMMA(|x|)) - ln(sin(Pi*r)/(Pi*r)) - ln(|r|)
//    where r = x - rounded_to_nearest(x), i.e |r| <= 0.5 and
//    ln(sin(Pi*r)/(Pi*r)) is approximated by 8-degree polynomial of r^2
//
//  Case x < -2^13
//  --------------
//    Here we use algorithm based on the Stirling formula:
//    ln(GAMMA(x)) = -ln(sqrt(2*Pi)) + (|x|-0.5)ln(x) - |x| -
//    - ln(sin(Pi*r)/(Pi*r)) - ln(|r|)
//    where r = x - rounded_to_nearest(x).
//
//  Neighbourhoods of negative roots
//  --------------------------------
//    Here we use polynomial approximation
//    ln(GAMMA(x-x0)) = ln(GAMMA(x0)) + (x-x0)*P14(x-x0),
//    where x0 is a root of ln(GAMMA(x)) rounded to nearest double
//    precision number.
//
//
//  Claculation of logarithm
//  ------------------------
//    Consider  x = 2^N * xf so
//    ln(x) = ln(frcpa(x)*x/frcpa(x))
//          = ln(1/frcpa(x)) + ln(frcpa(x)*x)
//
//    frcpa(x) = 2^(-N) * frcpa(xf)
//
//    ln(1/frcpa(x)) = -ln(2^(-N)) - ln(frcpa(xf))
//                   = N*ln(2) - ln(frcpa(xf))
//                   = N*ln(2) + ln(1/frcpa(xf))
//
//    ln(x) = ln(1/frcpa(x)) + ln(frcpa(x)*x) =
//          = N*ln(2) + ln(1/frcpa(xf)) + ln(frcpa(x)*x)
//          = N*ln(2) + T + ln(frcpa(x)*x)
//
//    Let r = 1 - frcpa(x)*x, note that r is quite small by
//    absolute value so
//
//    ln(x) = N*ln(2) + T + ln(1+r) ~ N*ln(2) + T + Series(r),
//    where T - is precomputed tabular value,
//    Series(r) = (P3*r + P2)*r^2 + (P1*r + 1)
//
//--
*/

// SPIRV intrinsics used
// double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fabs, _f64, ) (double);
// float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log, _f32, ) (float);
// float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (float, float, float);
// double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (double, double, double);

//
// Static data section:
//
//
// Positive args path of lgamma's data:
//

static __constant unsigned int __slgamma_ep__LM[] = {
    0x6356BE3F, 0x3FF762D8
};  // The point of local minimum on [1;2]

// [1,1.25)
static __constant unsigned int __slgamma_ep__C0[] = {
    0x21765550, 0x4004F394  // A0
        , 0xBC14D340, 0x3FAC8CDF, 0xFC0EE4DC, 0xBFE22092    // A8,A7
        , 0x7D9BF558, 0x4027CB00, 0xCA309876, 0xC02C6004    // A4,A3
        , 0xC3A02F10, 0x40048D48, 0xFD8B4838, 0xC01B63D2    // A6,A5
        , 0x5B1371AE, 0x40288FCC, 0xF1F3E423, 0xC01F4732    // A2,A1
};

// [1.25,1.5)
static __constant unsigned int __slgamma_ep__C1[] = {
    0xBCC38A42, 0xBFBF19B9  // A0
        , 0x35A6171A, 0x3F838E0D, 0xD61313B7, 0xBF831BBB    // A8,A7
        , 0x196425D0, 0x3FB08B40, 0xA53EB830, 0xBFC2E427    // A4,A3
        , 0xDC20D6C3, 0x3F9285DD, 0x9C223044, 0xBFA0C90C    // A6,A5
        , 0xC8F5287C, 0x3FDEF72B, 0xAEBC1DFC, 0x3D890B3D    // A2,A1
};

// [1.5,1.75)
static __constant unsigned int __slgamma_ep__C2[] = {
    0xF9DFA0CC, 0x4001DB08  // A0
        , 0xEB31047F, 0x3F65D5A7, 0x9BFA7FDE, 0xBFA44EAC    // A8,A7
        , 0xE7A663D8, 0x40051FEF, 0xE00A2522, 0xC012A5CF    // A4,A3
        , 0x3AB00E08, 0x3FD0E158, 0x95883BA5, 0xBFF084AF    // A6,A5
        , 0x877AE0A2, 0x40185982, 0xB73B57B7, 0xC015F83D    // A2,A1
};

// [1.75,2.0)
static __constant unsigned int __slgamma_ep__C3[] = {
    0x38D6CF0A, 0x4000A0CB  // A0
        , 0x032EB39A, 0x3F4A9222, 0x87EEA5A3, 0xBF8CBC95    // A8,A7
        , 0x0783BE49, 0x3FF79540, 0x418B8A25, 0xC00851BC    // A4,A3
        , 0x783E8C5B, 0x3FBBC992, 0x65E89B29, 0xBFDFA67E    // A6,A5
        , 0xF02FAF88, 0x4012B408, 0xE7CB0C39, 0xC013284C    // A2,A1
};

// [2;4)
static __constant unsigned int __slgamma_ep__C4[] = {
    0x38B9355F, 0xBEB2CC7A, 0x1833BF4C, 0x3F035F2D  // A10,A9
        , 0x7FD27785, 0xBFF51BAA, 0x5B6CDEFF, 0x3FFC9D5D    // A2,A1
        , 0xF9CB46C7, 0xBF421676, 0xFA1436C6, 0x3F7437F2    // A8,A7
        , 0x1DE592FE, 0xBFD7A704, 0xFEE8BD29, 0x3FE9F107    // A4,A3
        , 0x9FB224AB, 0xBF9E1C28, 0x445C9460, 0x3FBF7422    // A6,A5
        , 0xD66F8D8A, 0xBFF01E76    // A0
};

// [4; 2^13)

static __constant unsigned int __slgamma_ep__A[] = {
    0x40800000  // 00:    4.0
        , 0x41000000    // 01:    8.0
        , 0x41800000    // 02:   16.0
        , 0x42000000    // 03:   32.0
        , 0x42800000    // 04:   64.0
        , 0x43000000    // 05:  128.0
        , 0x43800000    // 06:  256.0
        , 0x44000000    // 07:  512.0
        , 0x44800000    // 08: 1024.0
        , 0x45000000    // 09: 2048.0
        , 0x45800000    // 10: 4096.0
};

/* [4; 2^13) */

static __constant unsigned int __slgamma_ep__B[] = {
    0x3E800000  // 00: 1.0/   4.0
        , 0x3E000000    // 01: 1.0/   8.0
        , 0x3D800000    // 02: 1.0/  16.0
        , 0x3D000000    // 03: 1.0/  32.0
        , 0x3C800000    // 04: 1.0/  64.0
        , 0x3C000000    // 05: 1.0/ 128.0
        , 0x3B800000    // 06: 1.0/ 256.0
        , 0x3B000000    // 07: 1.0/ 512.0
        , 0x3A800000    // 08: 1.0/1024.0
        , 0x3A000000    // 09: 1.0/2048.0
        , 0x39800000    // 10: 1.0/4096.0
};

// [4; 2^13)

static __constant unsigned int __slgamma_ep__C5_0[] = {
// 00: [4;8)
    0x9000EB5C, 0x3FCB8CC6, 0xA0C2C641, 0xBFD41997  // A6,A5
        , 0xFA0EA462, 0x3FFCAB0B    // A0
// 01: [8;16)
        , 0xDE0A364C, 0x3FD51EE4, 0x98A16E4B, 0xBFE00D7F    // A6,A5
        , 0xF327E9E4, 0x40210CE1    // A0
// 02: [16;32)
        , 0x6742D252, 0x3FE24F60, 0xD12574EC, 0xBFEC81D7    // A6,A5
        , 0xA63A9C27, 0x403BE636    // A0
// 03: [32;64)
        , 0x9DD542B4, 0x3FF1029A, 0x209D3B25, 0xBFFAD37C    // A6,A5
        , 0xFD9BE7EA, 0x405385E6    // A0
// 04: [64;128)
        , 0x7D26B523, 0x400062D9, 0x529FF023, 0xC00A03E1    // A6,A5
        , 0x51E566CE, 0x4069204C    // A0
// 05: [128;256)
        , 0xB38FD501, 0x40101476, 0xB387C0FC, 0xC0199DE7    // A6,A5
        , 0xEC83D759, 0x407EB8DA    // A0
// 06: [256;512)
        , 0x8D65125A, 0x401FDB00, 0x6E665581, 0xC0296B50    // A6,A5
        , 0x3107EF66, 0x409226D9    // A0
// 07: [512;1024)
        , 0xAF3E7B2D, 0x402FB3EA, 0x42AD8E0D, 0xC0395211    // A6,A5
        , 0xF072792E, 0x40A4EFA4    // A0
// 08: [1024;2048)
        , 0xC66B2563, 0x403FA024, 0xF250E691, 0xC0494569    // A6,A5
        , 0xC9235BB8, 0x40B7B747    // A0
// 09: [2048;4096)
        , 0xD6DA512C, 0x404F9607, 0x2EDDB4BC, 0xC0593F0B    // A6,A5
        , 0xC5F16DE2, 0x40CA7E29    // A0
// 10: [4096;8192)
        , 0xF613D98D, 0x405F90C5, 0x30E50AAF, 0xC0693BD1    // A6,A5
        , 0x238B190C, 0x40DD4495    // A0
};

// [4; 2^13)
static __constant unsigned int __slgamma_ep__C5[] = {
// 00: [4;8)
    0x8451C0CD, 0x3F6BBBD6, 0x272A16F7, 0xBF966EC3  // A10,A9
        , 0xA39AD769, 0x40022A24, 0xDF49C8C5, 0x4014190E    // A2,A1
        , 0x016EE241, 0x3FB130FD, 0x6E635248, 0xBFC151B4    // A8,A7
        , 0x1965B5FE, 0x3FDE8F61, 0xEB265E3D, 0xBFEB5110    // A4,A3
// 01: [8;16)
        , 0x3508626A, 0x3F736EF9, 0xADF58AF1, 0xBF9FE5DB    // A10,A9
        , 0xC5192058, 0x40110A9F, 0xA6F96B29, 0x40302008    // A2,A1
        , 0x0CE1E4B5, 0x3FB8E74E, 0x78873656, 0xBFC9B5DA    // A8,A7
        , 0xF10022DC, 0x3FE99D0D, 0x388F9484, 0xBFF829C0    // A4,A3
// 02: [16;32)
        , 0x6D7E9269, 0x3F7FFF9D, 0x249AEDB1, 0xBFAA780A    // A10,A9
        , 0x07AEA080, 0x402082A8, 0x68408013, 0x4045ED98    // A2,A1
        , 0x4C2F99B7, 0x3FC4E1E5, 0x6FFF1490, 0xBFD5DE2D    // A8,A7
        , 0x9584AE87, 0x3FF75FC8, 0xDD886CAE, 0xC006B4BA    // A4,A3
// 03: [32;64)
        , 0x75841A5F, 0x3F8CE543, 0xCFFA1BE2, 0xBFB801AB    // A10,A9
        , 0xB1815BDA, 0x403040A8, 0x17D24B7A, 0x405B99A9    // A2,A1
        , 0x81BFFA03, 0x3FD30CAB, 0x61ECF48B, 0xBFE41AEF    // A8,A7
        , 0x136BEC43, 0x400650CC, 0x46E8292B, 0xC0160220    // A4,A3
// 04: [64;128)
        , 0x22CAA8B8, 0x3F9B69BD, 0x75B7A213, 0xBFC6D488    // A10,A9
        , 0xCCAA2F6D, 0x40402028, 0xEB3CBE0F, 0x40709AAC    // A2,A1
        , 0x5924761E, 0x3FE22C6A, 0xF224523D, 0xBFF342F5    // A8,A7
        , 0x5CCA331F, 0x4015CD40, 0x0482C769, 0xC025AAD1    // A4,A3
// 05: [128;256)
        , 0xD0E40D06, 0x3FAAAD9C, 0x505D80CB, 0xBFD63FC8    // A10,A9
        , 0xD56C2648, 0x40501008, 0x4B0F4376, 0x40836479    // A2,A1
        , 0x26E00284, 0x3FF1BE01, 0xF6F7F7CA, 0xC002D8E3    // A8,A7
        , 0x7E95D860, 0x40258C75, 0xFD398011, 0xC0357FA8    // A4,A3
// 06: [256;512)
        , 0x59D49FEB, 0x3FBA4DAC, 0xD1C43A77, 0xBFE5F476    // A10,A9
        , 0xD890C7C6, 0x40600800, 0xAAEC8EF0, 0x40962C42    // A2,A1
        , 0xECF19B89, 0x40018680, 0x96FB7BA4, 0xC012A3EB    // A8,A7
        , 0xDD3B60F9, 0x40356C4C, 0xBF18F440, 0xC0456A34    // A4,A3
// 07: [512;1024)
        , 0xF6225A5A, 0x3FCA1B54, 0xBA10E048, 0xBFF5CD67    // A10,A9
        , 0xD94C58C2, 0x407003FE, 0x4ACBCD22, 0x40A8F30B    // A2,A1
        , 0x5EB66D8C, 0x40116A13, 0x1CED527E, 0xC022891B    // A8,A7
        , 0x17FDD8BC, 0x40455C46, 0x729E59C4, 0xC0555F82    // A4,A3
// 08: [1024;2048)
        , 0x095C6EC9, 0x3FD9FFF9, 0xB25D76C9, 0xC005B88C    // A10,A9
        , 0x58FA734D, 0x408001FE, 0xBAABB0F3, 0x40BBB953    // A2,A1
        , 0x9FEB5D87, 0x40215B2F, 0x9DEA5058, 0xC0327B53    // A8,A7
        , 0xB3E8D64D, 0x40555444, 0x26F9FC8A, 0xC0655A2B    // A4,A3
// 09: [2048;4096)
        , 0xA1C3D6B1, 0x3FE9F065, 0xFAE8D78D, 0xC015ACF6    // A10,A9
        , 0x383DD2B7, 0x409000FE, 0x1E8BCB8B, 0x40CE7F5C    // A2,A1
        , 0xE5DB2EBE, 0x40315324, 0x4EF70D18, 0xC0427419    // A8,A7
        , 0x53FF2207, 0x40655043, 0xE1BFE7B6, 0xC075577F    // A4,A3
// 10: [4096;8192)
        , 0xC6B1C70D, 0x3FF9E6FB, 0xAF76F85D, 0xC025A62D    // A10,A9
        , 0x2F61EBE8, 0x40A0007E, 0x3FB5F6C3, 0x40E0A2A2    // A2,A1
        , 0xC0A0141A, 0x40414E9B, 0xF2B69D43, 0xC0527030    // A8,A7
        , 0x7717B45B, 0x40754E41, 0x447258E5, 0xC085562A    // A4,A3
};

//
// Main path of lgamma's data:
//

static __constant float __slgamma_ep__TWO_23H[2] = { 12582912.0, -12582912.0 };

// ln(sqrt(2*Pi)) = 0.9189385332046727417803297364056176398618

static __constant unsigned int __slgamma_ep__LN_SQRT_TWO_PI[] = { 0xc864beb5, 0x3fed67f1 };

// Maximal positive number

static __constant int __slgamma_ep__PBIG[] = { 0x7f7fffff, 0x7f7fffff };

// polynomial approximation of ln(sin(Pi*r)/(Pi*r)), |r| <= 0.5

static __constant unsigned int __slgamma_ep__S16[] = { 0xA486E820, 0xBFD58731 };

static __constant unsigned int __slgamma_ep__S14[] = { 0xC28E15A9, 0xBFA4452C };

static __constant unsigned int __slgamma_ep__S08[] = { 0xE1B86C4F, 0xBFD013F6 };

static __constant unsigned int __slgamma_ep__S06[] = { 0x9F7A341F, 0xBFD5B3F1 };

static __constant unsigned int __slgamma_ep__S12[] = { 0x5252E778, 0xBFC86A0D };

static __constant unsigned int __slgamma_ep__S10[] = { 0xC9EE284B, 0xBFC93E08 };

static __constant unsigned int __slgamma_ep__S04[] = { 0x555C9EDD, 0xBFE15132 };

static __constant unsigned int __slgamma_ep__S02[] = { 0x62480E35, 0xBFFA51A6 };

// Left root polynomials.

static __constant unsigned int __slgamma_ep__LRP[] = {
// near -2.7476826467274126919
    0xC31314FF, 0xC034185A, 0x3C28DFE3, 0x4023267F  // R3,R2
        , 0xA904B194, 0xBFFEA12D, 0x30BA7689, 0x3CA8FB85    // R1,R0
// near -3.9552942848585979085
        , 0x9E70C888, 0xC0AD2535, 0xAEA1B8C6, 0x406F76DE    // R3,R2
        , 0x966C5644, 0xC034B99D, 0x36980B58, 0xBCBDDC03    // R1,R0
};

// Right root polynomials.

static __constant unsigned int __slgamma_ep__RRP[] = {
// near -2.4570247382208005860
    0x058D9592, 0x3FF694A6, 0xB003A92B, 0x40136EEB  // R3,R2
        , 0x66AF5360, 0x3FF83FE9, 0x6D1FE86D, 0x3C90323B    // R1,R0
// near -3.1435808883499798405
        , 0x1268DA38, 0x405C1137, 0x977D2C23, 0x4039D4D2    // R3,R2
        , 0x5F2FAC62, 0x401F20A6, 0xE3AE7A62, 0x3CDE9605    // R1,R0
};

// Left root and it's bounds.

static __constant unsigned int __slgamma_ep__LRIB[] = {
    0xA9328A1D, 0xC005FB43, 0x0A1BD901, 0xC005FB41  // -2.7476826467274126919
        , 0x6B0527E5, 0xC005FB3E, 0xD0D6F455, 0xC00FA47B, 0x547C2FE5, 0xC00FA471    // -3.9552942848585979085
    , 0x80000000, 0xC00FA45D
};

// Right root and it's bounds.

static __constant unsigned int __slgamma_ep__RRIB[] = {
    0x3517A988, 0xC003A7FF, 0x9600F86C, 0xC003A7FC  // -2.4570247382208005860
        , 0xF6EA4750, 0xC003A7F9, 0x5BB50ACB, 0xC0092610, 0xBC9E59AF, 0xC009260D    // -3.1435808883499798405
    , 0x1D87A893, 0xC009260B
};

//! ===========================================================================
//! @brief 64-bit natural logarithm value computation routine
//!
//! @param[in] arg     Argument
//! @return            Returns 64-bit natural logarithm
//! ===========================================================================
__attribute__((always_inline))
static inline double __slgamma_ep_own_log_fp64 (double arg)
{
    int denorm_scale_exp = 0;
    double result = 0.0;
    double R = 0.0, R2 = 0.0, R4 = 0.0, d_expon = 0.0;
    double P1819 = 0.0, P1617 = 0.0, P1415 = 0.0, P1213 = 0.0, P1011 = 0.0;
    double P89 = 0.0, P67 = 0.0, P45 = 0.0, P23 = 0.0, P01 = 0.0, P1619 = 0.0, P1215 = 0.0, P811 = 0.0;
    double P47 = 0.0, P03 = 0.0, P1219 = 0.0, P819 = 0.0, P419 = 0.0, P019 = 0.0;
    double poly = 0.0, res = 0.0;
    union
    {
        unsigned long w;
        unsigned int w32[2];
        int s32[2];
        double f;
    } x, expon, expon_r, one, l2;
    union
    {
        unsigned long w;
        unsigned int w32[2];
        int s32[2];
        double f;
    } c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, c10, c11, c12, c13, c14, c15, c16, c17, c18, c19;
    union
    {
        unsigned long w;
        unsigned int w32[2];
        int s32[2];
        double f;
    } denorm_scale, _res;

    denorm_scale.w = 0x43B0000000000000ull;
    x.f = arg;

    if ((x.w == 0x0uL) || (x.w >= 0x7ff0000000000000uL))
    {
        if ((x.w & 0x7fffffffffffffff) == 0x0uL)
        {
            _res.w = 0xfff0000000000000uL;
            result = _res.f;
            return result;
        }
        else if (x.w > 0x8000000000000000uL)
        {
            _res.w = x.w | 0xfff8000000000000uL;
            result = _res.f;
            return result;
        }
        else
        {
            if (x.w > 0x7ff0000000000000uL)
            {
                _res.f = x.f + x.f;
            }
            else
            {
                _res.w = x.w;
            }
            result = _res.f;
            return result;
        }
    }   // if ((x.w == 0x0uL) || (x.w >= 0x7ff0000000000000uL))

    if (x.w <= 0x000fffffffffffffuL)
    {
        x.f *= denorm_scale.f;
        denorm_scale_exp = 60;
    }

    expon.w = x.w + 0x000AAAAAAAAAAAAAull;
    expon.w >>= 52;
    expon_r.w = expon.w << 52;

    one.w = 0x3FF0000000000000ull;
    x.w = (x.w + one.w) - expon_r.w;

    R = x.f - one.f;

    c19.w = 0x3fb66f75676ae3eaull;
    c18.w = 0xbfc65a6d34a6dd3dull;
    P1819 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) ((c19.f), (x.f), (c18.f));

    c17.w = 0x3fa49f86632433feull;
    c16.w = 0xbfb5ea03fef4c746ull;
    P1617 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) ((c17.f), (x.f), (c16.f));

    R2 = R * R;

    c15.w = 0x3faf2a14615c2bb3ull;
    c14.w = 0xbfb062accb1ad8aaull;
    P1415 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) ((c15.f), (R), (c14.f));

    c13.w = 0x3fb1038ce60c1b2full;
    c12.w = 0xbfb2406abbb6c334ull;
    P1213 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) ((c13.f), (R), (c12.f));

    c11.w = 0x3fb3b219a9287c7full;
    c10.w = 0xbfb555d0d4781fd1ull;
    P1011 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) ((c11.f), (R), (c10.f));

    c9.w = 0x3fb745c847eeb960ull;
    c8.w = 0xbfb99995585870b8ull;
    P89 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) ((c9.f), (R), (c8.f));

    c7.w = 0x3fbc71c758cfdb39ull;
    c6.w = 0xbfc000000b3d2e0full;
    P67 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) ((c7.f), (R), (c6.f));

    P1619 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) ((P1819), (R2), (P1617));

    R4 = R2 * R2;
    P1215 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) ((P1415), (R2), (P1213));

    c5.w = 0x3fc2492491d4fd71ull;
    c4.w = 0xbfc555555534c686ull;
    P45 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) ((c5.f), (R), (c4.f));

    c3.w = 0x3fc99999999a7fc1ull;
    c2.w = 0xbfd0000000001596ull;
    P23 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) ((c3.f), (R), (c2.f));

    P811 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) ((P1011), (R2), (P89));
    P1219 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) ((P1619), (R4), (P1215));

    c1.w = 0x3fd55555555554fcull;
    c0.w = 0xbfdffffffffffff8ull;
    P01 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) ((c1.f), (R), (c0.f));

    P47 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) ((P67), (R2), (P45));

    P819 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) ((P1219), (R4), (P811));
    P03 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) ((P23), (R2), (P01));

    P419 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) ((P819), (R4), (P47));

    expon.w -= 0x3FF;

    expon.s32[0] -= denorm_scale_exp;
    P019 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) ((P419), (R4), (P03));

    d_expon = (double) expon.s32[0];

    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) ((R2), (P019), (R));

    l2.w = 0x3FE62E42FEFA39EFull;
    res = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) ((d_expon), (l2.f), (poly));

    result = res;

    return result;
}   // static inline double _VSTATIC(own_log_fp64) (double arg)

//! ===========================================================================
//! @brief 32-bit nearbyint computation routine
//!
//! @param[in] sA      Argument
//! @return            Returns 32-bit nearbyint
//! ===========================================================================
__attribute__((always_inline))
static inline float __slgamma_ep_own_nearbyint_fp32 (float sA)
{
    float _rnd_s2p23 = 0.0f;
    float _rnd_sSignMask = 0.0f;
    float _rnd_sSign = 0.0f;
    float _rnd_sAbsArg = 0.0f;
    float _rnd_sRes_ub = 0.0f;
    float _rnd_sRange = 0.0f;
    unsigned int _rnd_i2p23 = 0x4b000000u;
    unsigned int _rnd_iSignMask = 0x80000000u;
    _rnd_s2p23 = (*((float *) &(_rnd_i2p23)));
    _rnd_sSignMask = (*((float *) &(_rnd_iSignMask)));
    (*((unsigned int *) &(_rnd_sSign))) = ((*((unsigned int *) &(sA))) & (*((unsigned int *) &(_rnd_sSignMask))));
    (*((unsigned int *) &(_rnd_sAbsArg))) = (~((*((unsigned int *) &(_rnd_sSignMask)))) & (*((unsigned int *) &(sA))));
    (*((unsigned int *) &(_rnd_sRange))) = ((unsigned int) (-(int) (_rnd_sAbsArg > _rnd_s2p23)));
    _rnd_sRes_ub = (_rnd_sAbsArg + _rnd_s2p23);
    _rnd_sRes_ub = (_rnd_sRes_ub - _rnd_s2p23);
    (*((unsigned int *) &(_rnd_sRes_ub))) = ((*((unsigned int *) &(_rnd_sRes_ub))) | (*((unsigned int *) &(_rnd_sSign))));
    (*((unsigned int *) &(_rnd_sRes_ub))) =
        (((~(*((unsigned int *) &(_rnd_sRange)))) & (*((unsigned int *) &(_rnd_sRes_ub)))) |
         ((*((unsigned int *) &(_rnd_sRange))) & (*((unsigned int *) &(sA)))));
    return _rnd_sRes_ub;
}   // static inline float _VSTATIC(own_nearbyint_fp32)( float sA )

//! ===========================================================================
//! @brief 32-bit ceil computation routine
//!
//! @param[in] sA      Argument
//! @return            Returns 32-bit ceil
//! ===========================================================================
__attribute__((always_inline))
static inline float __slgamma_ep_own_ceilf_fp32 (float sA)
{
    float _rnd_s2p23 = 0.0f;
    float _rnd_sSignMask = 0.0f;
    float _rnd_sSign = 0.0f;
    float _rnd_sIsGreater = 0.0f;
    float _rnd_sOne = 0.0f;
    float _rnd_sAddOne = 0.0f;
    float _rnd_sAbsArg = 0.0f;
    float _rnd_sRes_ub = 0.0f;
    float _rnd_sRange = 0.0f;
    unsigned int _rnd_i2p23 = 0x4b000000u;
    unsigned int _rnd_iSignMask = 0x80000000u;
    unsigned int _rnd_iOne = 0x3f800000u;
    _rnd_sSignMask = (*((float *) &(_rnd_iSignMask)));
    (*((unsigned int *) &(_rnd_sSign))) = ((*((unsigned int *) &(sA))) & (*((unsigned int *) &(_rnd_sSignMask))));
    (*((unsigned int *) &(_rnd_sAbsArg))) = (~((*((unsigned int *) &(_rnd_sSignMask)))) & (*((unsigned int *) &(sA))));
    _rnd_s2p23 = (*((float *) &(_rnd_i2p23)));
    (*((unsigned int *) &(_rnd_sRange))) = ((unsigned int) (-(signed int) (_rnd_sAbsArg > _rnd_s2p23)));
    _rnd_sRes_ub = (_rnd_sAbsArg + _rnd_s2p23);
    _rnd_sRes_ub = (_rnd_sRes_ub - _rnd_s2p23);
    (*((unsigned int *) &(_rnd_sRes_ub))) = ((*((unsigned int *) &(_rnd_sRes_ub))) | (*((unsigned int *) &(_rnd_sSign))));
    (*((unsigned int *) &(_rnd_sIsGreater))) = ((unsigned int) (-(signed int) (sA > _rnd_sRes_ub)));
    _rnd_sOne = (*((float *) &(_rnd_iOne)));
    (*((unsigned int *) &(_rnd_sAddOne))) = ((*((unsigned int *) &(_rnd_sOne))) & (*((unsigned int *) &(_rnd_sIsGreater))));
    _rnd_sRes_ub = _rnd_sRes_ub + _rnd_sAddOne;
    (*((unsigned int *) &(_rnd_sRes_ub))) =
        (((~(*((unsigned int *) &(_rnd_sRange)))) & (*((unsigned int *) &(_rnd_sRes_ub)))) |
         ((*((unsigned int *) &(_rnd_sRange))) & (*((unsigned int *) &(sA)))));
    return _rnd_sRes_ub;
}   // static inline float _VSTATIC(own_ceilf_fp32)( float sA )

//! ===========================================================================
//! @brief 32-bit Gergo Nemes fast lgamma approximation
//!
//! @param[in] arg     Argument
//! @return            Returns 32-bit lgamma
//! ===========================================================================
__attribute__((always_inline))
static inline float __slgamma_ep_own_lgamma_fast_fp32 (float arg)
{

    unsigned int __slgamma_ep_own_log2pif = 0x3feb3f8eu;
    float result = 0.5f * ((*(float *) &(__slgamma_ep_own_log2pif)) - SPIRV_OCL_BUILTIN(log, _f32, ) (arg));
    result = result + (arg * (SPIRV_OCL_BUILTIN(log, _f32, ) (arg + (1.0f / ((12.0f * arg) - (1.0f / (10.0f * arg))))) - 1.0f));
    return result;
}   // static inline float _VSTATIC(own_lgamma_fast_fp32) (float arg)

//! ===========================================================================
//! @brief 64-bit Gergo Nemes fast lgamma approximation
//!
//! @param[in] arg     Argument
//! @return            Returns 64-bit lgamma
//! ===========================================================================
__attribute__((always_inline))
static inline double __slgamma_ep_own_lgamma_fast_fp64 (double arg)
{

    unsigned int __slgamma_ep_own_log2pi[] = { 0xc864beb5u, 0x3ffd67f1u };
    double result = 0.5 * ((*(double *) __slgamma_ep_own_log2pi) - __slgamma_ep_own_log_fp64 (arg));
    result = result + (arg * (__slgamma_ep_own_log_fp64 (arg + (1.0 / ((12.0 * arg) - (1.0 / (10.0 * arg))))) - 1.0));
    return result;
}   // static inline double _VSTATIC(own_lgamma_fast_fp64) (double arg)

//! ===========================================================================
//! @brief 64-bit lgamma approximation for positive arguments in [0.0; 2^13)
//!
//! @param[in] arg     Argument
//! @return            Returns 64-bit lgamma
//! ===========================================================================
__attribute__((always_inline))
static inline double __slgamma_ep_own_lgamma_pos_fp64 (double arg)
{
    double x = 0.0, result = 0.0, y = 0.0, p = 0.0, y2 = 0.0, lx = 0.0;
    unsigned int ix = 0;
    int i = 0;

    if (arg < 1.0)
    {
        x = arg + 1.0;
        lx = __slgamma_ep_own_log_fp64 (arg);
    }
    else
    {
        x = arg;
        lx = 0.0;
    }

    ix = ((((_iml_dp_union_t *) & x)->dwords.hi_dword));

    if ((x == 1.0) || (x == 2.0))
    {
        // lgamma(1) = +0
        // lgamma(2) = +0
        result = 0.0;
    }
    else if (x >= 4.0)  // 4 <= x < 2^13:
    {

        result = __slgamma_ep_own_lgamma_fast_fp64 (x);
    }   // else if ( x >= 4.0 ) // 4 <= x < 2^13:
    else if (x > 2.0)
    {
        //
        // 2 < x < 4:
        // --------------
        // ln(GAMMA(x)) ~ (x-2)*P10(x)
        //
        y2 = x * x;
        p = ((((((__constant double *) __slgamma_ep__C4)[1] * y2
                + ((__constant double *) __slgamma_ep__C4)[5]) * y2
               + ((__constant double *) __slgamma_ep__C4)[9]) * y2
              + ((__constant double *) __slgamma_ep__C4)[7]) * y2 + ((__constant double *) __slgamma_ep__C4)[3]) * x;
        result = ((((((__constant double *) __slgamma_ep__C4)[0] * y2
                     + ((__constant double *) __slgamma_ep__C4)[4]) * y2
                    + ((__constant double *) __slgamma_ep__C4)[8]) * y2
                   + ((__constant double *) __slgamma_ep__C4)[6]) * y2
                  + ((__constant double *) __slgamma_ep__C4)[2]) * y2 + ((__constant double *) __slgamma_ep__C4)[10];
        result = result + p;
        p = x - 2.0;
        result = result * p;
    }   // else if ( x > 2.0 )
    else if (x >= 1.75)
    {
        //
        // 1.75 <= x < 2:
        // -----------------
        // ln(GAMMA(x)) ~ P8(x)
        //
        y2 = x * x;
        p = (((((__constant double *) __slgamma_ep__C3)[2] * y2
               + ((__constant double *) __slgamma_ep__C3)[6]) * y2
              + ((__constant double *) __slgamma_ep__C3)[4]) * y2 + ((__constant double *) __slgamma_ep__C3)[8]) * x;
        result = (((((__constant double *) __slgamma_ep__C3)[1] * y2
                    + ((__constant double *) __slgamma_ep__C3)[5]) * y2
                   + ((__constant double *) __slgamma_ep__C3)[3]) * y2
                  + ((__constant double *) __slgamma_ep__C3)[7]) * y2 + ((__constant double *) __slgamma_ep__C3)[0];
        result = result + p;
    }   // else if ( x >= 1.75 )
    else if (x >= 1.5)
    {
        //
        // 1.5 <= x < 1.75:
        // -----------------
        // ln(GAMMA(x)) ~ P8(x)
        //
        y2 = x * x;
        p = (((((__constant double *) __slgamma_ep__C2)[2] * y2
               + ((__constant double *) __slgamma_ep__C2)[6]) * y2
              + ((__constant double *) __slgamma_ep__C2)[4]) * y2 + ((__constant double *) __slgamma_ep__C2)[8]) * x;
        result = (((((__constant double *) __slgamma_ep__C2)[1] * y2
                    + ((__constant double *) __slgamma_ep__C2)[5]) * y2
                   + ((__constant double *) __slgamma_ep__C2)[3]) * y2
                  + ((__constant double *) __slgamma_ep__C2)[7]) * y2 + ((__constant double *) __slgamma_ep__C2)[0];
        result = result + p;
    }   // else if ( x >= 1.5 )
    else if (x >= 1.25)
    {
        //
        // 1.25 <= x < 1.5:
        // -----------------
        // ln(GAMMA(x)) ~ ln(GAMMA(x0))+(x-x0)*P7(x-x0),
        // where x0 - point of local minimum on [1;2]
        // rounded to nearest double precision number.
        //
        y = x - (*(__constant double *) __slgamma_ep__LM);
        y2 = y * y;
        p = (((((__constant double *) __slgamma_ep__C1)[2] * y2
               + ((__constant double *) __slgamma_ep__C1)[6]) * y2
              + ((__constant double *) __slgamma_ep__C1)[4]) * y2 + ((__constant double *) __slgamma_ep__C1)[8]) * y;
        result = (((((__constant double *) __slgamma_ep__C1)[1] * y2
                    + ((__constant double *) __slgamma_ep__C1)[5]) * y2
                   + ((__constant double *) __slgamma_ep__C1)[3]) * y2
                  + ((__constant double *) __slgamma_ep__C1)[7]) * y2 + ((__constant double *) __slgamma_ep__C1)[0];
        result = result + p;
    }   // else if ( x >= 1.25 )
    else    // if ( x >= 1.0 )
    {
        //
        // 1 <= x < 1.25:
        // -----------------
        // ln(GAMMA(x)) ~ P8(x)
        //
        y2 = x * x;
        p = (((((__constant double *) __slgamma_ep__C0)[2] * y2
               + ((__constant double *) __slgamma_ep__C0)[6]) * y2
              + ((__constant double *) __slgamma_ep__C0)[4]) * y2 + ((__constant double *) __slgamma_ep__C0)[8]) * x;
        result = (((((__constant double *) __slgamma_ep__C0)[1] * y2
                    + ((__constant double *) __slgamma_ep__C0)[5]) * y2
                   + ((__constant double *) __slgamma_ep__C0)[3]) * y2
                  + ((__constant double *) __slgamma_ep__C0)[7]) * y2 + ((__constant double *) __slgamma_ep__C0)[0];
        result = result + p;
    }   // else // if ( x >= 1.0 )

    result = result - lx;

    return result;
}   // static inline double _VSTATIC(own_lgamma_pos_fp64) (double arg)

//! ===========================================================================
//! @brief 32-bit lgamma approximation
//!
//! @param[in] a     Argument pointer
//! @param[in] r     Result pointer
//! @return          Computation status
//! ===========================================================================

__attribute__((always_inline))
inline int __internal_slgamma_ep_cout (float *a, float *r)
{

    float x = (*a);
    float f = 0.0f, f1 = 0.0f;
    double ldx = 0.0, t0 = 0.0, t1 = 0.0, t2 = 0.0, result = 0.0, y = 0.0, y2 = 0.0;
    double p = 0.0, r1 = 0.0, r2 = 0.0;
    signed int ix = ((signed int) (((_iml_sp_union_t *) & x)->hex[0]));
    unsigned int iabsx = ((((_iml_sp_union_t *) & x)->hex[0]) & 0x7fffffff);
    unsigned int xsig = (((_iml_sp_union_t *) & x)->hex[0] >> 31);
    unsigned int exp_x = ((((_iml_sp_union_t *) & x)->hex[0] >> 23) & 0xFF);
    int i = 0, intx = 0;
    int signgam = 1;
    int nRet = 0;

    if (exp_x == 0xFF)
    {
        // For x==+/-infinity: return +infinity.
        // For x==NaN: raise invalid, return QNaN.
        (*r) = x * x;
    }
    else if (iabsx == 0x00000000u)
    {
        // x==+/-0
        if (xsig)
        {
            signgam = -1;
        }
        // Return +Infinity. Raise Zero divide.
        f = 0.0f;
        (*r) = 1.0f / f;
        nRet = 2;
    }
    else if (ix > 0x7C44AF8E)
    {
        // OVERFLOW_BOUNDARY <= x <= +Max_Double
        // Return +Infinity. Raise Inexact and Overflow.
        (*r) = (*(__constant double *) __slgamma_ep__PBIG) * (*(__constant double *) __slgamma_ep__PBIG);
        nRet = 3;
    }
    else
    {

        if (x >= 4.0f)  // x>4 fast Sergio Nemes branch
        {
            (*r) = __slgamma_ep_own_lgamma_fast_fp32 (x);
        }
        else if (!xsig) // x>=0 positive arguments domain
        {
            if (iabsx < 0x46000000u)
            {
                ldx = x;
                (*r) = __slgamma_ep_own_lgamma_pos_fp64 (ldx);
            }
            else    // if ( iabsx <= 0x7C44AF8Eu)
            {
                //
                // 2^13 <= x < OVERFLOW_BOUNDARY:
                // -----------------------------
                // Here we use algorithm based on the Stirling formula:
                // ln(GAMMA(x)) = ln(sqrt(2*Pi)) + (x-0.5)ln(x) - x
                //
                ldx = x;
                result = (*(__constant double *) __slgamma_ep__LN_SQRT_TWO_PI);
                t1 = ldx - 0.5;
                t2 = __slgamma_ep_own_log_fp64 (x);
                t1 = t1 * t2;
                result = result + t1;
                result = result - ldx;
                (*r) = (float) result;
            }
        }   // if (!xsig) // x>=0 positive arguments domain
        else    // x<0 negative arguments domain
        {
            f = __slgamma_ep_own_nearbyint_fp32 (x);

            intx = (x == f);    /* Is x an integer? */

            if (!intx && (x > (__slgamma_ep__TWO_23H[1])) && !(((int) __slgamma_ep_own_ceilf_fp32 (x)) & 1))
            {
                // x belongs to interval (-2*k-1;-2*k) for some integer k>0.
                signgam = -1;
            }

            if (intx)
            {
                // x is negative integer
                // Return +Infinity. Raise Zero divide
                f = 0.0f;
                (*r) = 1.0f / f;
                nRet = 2;
            }
            else if (iabsx < 0x40000000u)
            {
                //
                // -2 < x < 0:
                // ------------
                // Reduction to interval [1;2)
                // using the formula
                // lgammaf(x) = lgammaf(x+1) - ln(abs(x))
                //
                y = x;
                p = 1.0;
                while (y < 1.0)
                {
                    p = p * y;
                    y = y + 1.0;
                }

                p = SPIRV_OCL_BUILTIN(fabs, _f64, ) (p);
                p = __slgamma_ep_own_log_fp64 ((double) p);
                result = __slgamma_ep_own_lgamma_pos_fp64 (y);
                result = result - p;
                (*r) = (float) result;
            }   // else if ( iabsx < 0x40000000u )
            else if (iabsx < 0x40800000u)
            {
                //
                // -4 < x < -2:
                // --------------
                // At first we check if x is near 1.0 of 2.0 roots on [-i-1;-i)
                // and if so then we compute lgammaf(x) with special polynomials:
                // ln(GAMMA(x-x0)) = ln(GAMMA(x0)) + (x-x0)*P14(x-x0),
                // where x0 is corresponding root (rounded to double precision).
                // Else we use argument reduction to interval [1;2)
                // using the formula
                // lgammaf(x) = lgammaf(x+1) - ln(abs(x))
                //
                f = (-2.5) - x;
                f = f + (__slgamma_ep__TWO_23H[0]);
                i = ((((_iml_sp_union_t *) & f)->hex[0]) & 0x00000001); // i=trunc(2-x)
                f = f - (__slgamma_ep__TWO_23H[0]);

                if ((((__constant double *) __slgamma_ep__LRIB)[3 * i + 0] < x) && (x < ((__constant double *) __slgamma_ep__LRIB)[3 * i + 2]))
                {
                    // x is near left root
                    y = x;
                    y = y - ((__constant double *) __slgamma_ep__LRIB)[3 * i + 1];
                    result = ((((__constant double *) __slgamma_ep__LRP)[4 * i + 0] * y
                               + ((__constant double *) __slgamma_ep__LRP)[4 * i + 1]) * y
                              + ((__constant double *) __slgamma_ep__LRP)[4 * i + 2]) * y + ((__constant double *) __slgamma_ep__LRP)[4 * i + 3];
                    result = result;
                }
                else if ((((__constant double *) __slgamma_ep__RRIB)[3 * i + 0] < x) && (x < ((__constant double *) __slgamma_ep__RRIB)[3 * i + 2]))
                {
                    // x is near right root
                    y = x;
                    y = y - ((__constant double *) __slgamma_ep__RRIB)[3 * i + 1];
                    result = ((((__constant double *) __slgamma_ep__RRP)[4 * i + 0] * y
                               + ((__constant double *) __slgamma_ep__RRP)[4 * i + 1]) * y
                              + ((__constant double *) __slgamma_ep__RRP)[4 * i + 2]) * y + ((__constant double *) __slgamma_ep__RRP)[4 * i + 3];
                    result = result;
                }
                else
                {
                    // x is far away from both roots.
                    y = x;
                    p = 1.0;
                    while (y < 1.0)
                    {
                        p = p * y;
                        y = y + 1.0;
                    }
                    p = SPIRV_OCL_BUILTIN(fabs, _f64, ) (p);
                    p = __slgamma_ep_own_log_fp64 ((double) p);
                    result = __slgamma_ep_own_lgamma_pos_fp64 (y);
                    result = result - p;
                }
                (*r) = (float) result;
            }   // else if ( iabsx < 0x40800000u )
            else if (iabsx < 0x41100000u)
            {
                // -9 < x < -4
                // x is far away from both roots.
                y = x;
                p = 1.0;
                while (y < 1.0)
                {
                    p = p * y;
                    y = y + 1.0;
                }

                p = SPIRV_OCL_BUILTIN(fabs, _f64, ) (p);
                p = __slgamma_ep_own_log_fp64 ((double) p);
                result = __slgamma_ep_own_lgamma_pos_fp64 (y);
                result = result - p;
                (*r) = (float) result;
            }   // else if ( iabsx < 0x41100000u )
            else if (iabsx < 0x46000000u)
            {
                //
                // -2^13 < x < -9:
                // ---------------
                // Here we use the formula:
                // ln(GAMMA(-x)) = ln(Pi/(x*GAMMA(x)*sin(Pi*x))) =
                // = -ln(x) - ln((GAMMA(x)) - ln(sin(Pi*r1)/(Pi*r1)) - ln(|r1|)
                // where r1 = x - rounded_to_nearest_integer(x), i.e |r1| <= 0.5
                // and ln(sin(Pi*r1)/(Pi*r1)) is approximated by
                // 8-degree polynomial of r1^2.
                //
                f = (__slgamma_ep__TWO_23H[0]) - x;
                f = f - (__slgamma_ep__TWO_23H[0]); // f=round(-x)
                ldx = x;
                ldx = -ldx;
                r1 = f;
                r1 = ldx - r1;
                r2 = r1 * r1;

                result = __slgamma_ep_own_log_fp64 (-x);
                result = -result;
                y = __slgamma_ep_own_lgamma_pos_fp64 (ldx);
                result = result - y;

                t1 = r2 * r2;
                t0 = ((((*(__constant double *) __slgamma_ep__S14) * t1
                        + (*(__constant double *) __slgamma_ep__S10)) * t1
                       + (*(__constant double *) __slgamma_ep__S06)) * t1 + (*(__constant double *) __slgamma_ep__S02)) * r2;
                p = ((((*(__constant double *) __slgamma_ep__S16) * t1
                       + (*(__constant double *) __slgamma_ep__S12)) * t1
                      + (*(__constant double *) __slgamma_ep__S08)) * t1 + (*(__constant double *) __slgamma_ep__S04)) * t1;
                p = p + t0;

                result = result - p;

                r1 = SPIRV_OCL_BUILTIN(fabs, _f64, ) (r1);
                y = __slgamma_ep_own_log_fp64 ((double) r1);
                result = result - y;
                (*r) = (float) result;
            }   // else if ( iabsx < 0x46000000u )
            else
            {
                //
                // Case x < -2^13:
                // ---------------
                // Here we use algorithm based on the Stirling formula:
                // ln(GAMMA(x)) = -ln(sqrt(2*Pi)) + (x-0.5)ln(|x|) -
                // - x - ln(sin(Pi*r1)/(Pi*r1)) - ln(|r1|)
                // where r1 = x - rounded_to_nearest_integer(x).
                //
                result = (*(__constant double *) __slgamma_ep__LN_SQRT_TWO_PI);
                ldx = x;
                t1 = ldx - 0.5;
                t2 = -ldx;
                t2 = __slgamma_ep_own_log_fp64 (-x);
                t1 = t1 * t2;
                result = t1 - result;

                result = result - ldx;

                f = (__slgamma_ep__TWO_23H[0]);
                f = f - x;
                f = f - (__slgamma_ep__TWO_23H[0]);

                r1 = -ldx;
                r1 = r1 - f;
                r2 = r1 * r1;

                p = ((((((((*(__constant double *) __slgamma_ep__S16) * r2
                           + (*(__constant double *) __slgamma_ep__S14)) * r2
                          + (*(__constant double *) __slgamma_ep__S12)) * r2
                         + (*(__constant double *) __slgamma_ep__S10)) * r2
                        + (*(__constant double *) __slgamma_ep__S08)) * r2
                       + (*(__constant double *) __slgamma_ep__S06)) * r2
                      + (*(__constant double *) __slgamma_ep__S04)) * r2 + (*(__constant double *) __slgamma_ep__S02)) * r2;

                result = result - p;

                r1 = SPIRV_OCL_BUILTIN(fabs, _f64, ) (r1);
                y = __slgamma_ep_own_log_fp64 ((double) r1);
                result = result - y;
                (*r) = (float) result;
            }   // else if ( intx )
        }   // else // x<0 negative arguments domain
    }   // else if ( exp_x == IML_EXPINF_32 )

    return nRet;
}   // inline int __internal_slgamma_ep_cout (float *a, float *r)

float __ocl_svml_lgammaf_noLUT (float a)
{
    float va1;
    float vr1;
    float r;

    va1 = a;
    __internal_slgamma_ep_cout (&va1, &vr1);
    r = vr1;

    return r;
}

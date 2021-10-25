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
//        where x0 - point of local minimum on [1;2] rounded to nearest
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
//    where x0 is a root of ln(GAMMA(x)) rounded to nearest
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

typedef union
{
    uint hex;
    float fp;
} _iml_lg_sp_union_t;

__constant float __slgamma_ep__TWO_23H[2] = { 12582912.0, -12582912.0 };

static __constant unsigned int __slgamma_ep___p1[] = {
    0xbf13c468, // p[0] =       -0.577215672
    0x45e1ce83, // p[1] =       7225.81396
    0x46ce12fa, // p[2] =       26377.4883
    0x471654f6, // p[3] =       38484.9609
    0x46df1a7e, // p[4] =       28557.2461
    0x4630deb0, // p[5] =       11319.6719
    0x450f2d6a, // p[6] =       2290.83838
    0x4349cfaf, // p[7] =       201.811264
    0x409e3f5e, // p[8] =       4.94523525
};

static __constant unsigned int __slgamma_ep___q1[] = {
    0x46094625, // q[0] =       8785.53613
    0x470dff47, // q[1] =       36351.2773
    0x4770ab39, // q[2] =       61611.2227
    0x4756d11a, // q[3] =       54993.1016
    0x46d7efbe, // q[4] =       27639.8711
    0x45f1d60e, // q[5] =       7738.75684
    0x448b2aa3, // q[6] =        1113.3324
    0x4286f6d9, // q[7] =       67.4821243
    0x3f800000, // q[8] =                1
};

static __constant unsigned int __slgamma_ep___p2[] = {
    0x3ed87730, // p[0] =        0.422784328
    0x4a3ba0f4, // p[1] =          3074109
    0x4a9bd7cb, // p[2] =        5106661.5
    0x4a4bbea4, // p[3] =          3338153
    0x4984d666, // p[4] =       1088204.75
    0x48347653, // p[5] =       184793.297
    0x46724bc1, // p[6] =       15506.9385
    0x44079a7d, // p[7] =       542.413879
    0x409f2ffd, // p[8] =       4.97460794
};

static __constant unsigned int __slgamma_ep___q2[] = {
    0x4b1176a8, // q[0] =          9533096
    0x4b880313, // q[1] =         17827366
    0x4b4d7d87, // q[2] =         13467015
    0x4aa0c3f8, // q[3] =          5267964
    0x498ac20f, // q[4] =       1136705.88
    0x48021198, // q[5] =       133190.375
    0x45f2a865, // q[6] =       7765.04932
    0x43370868, // q[7] =       183.032837
    0x3f800000, // q[8] =                1
};

static __constant unsigned int __slgamma_ep___p3[] = {
    0x3fe55860, // p[0] =       1.79175949
    0x530287dd, // p[1] =   5.60625156e+11
    0x52e563ff, // p[2] =   4.92612583e+11
    0x521e92b7, // p[3] =   1.70266575e+11
    0x50db1347, // p[4] =   2.94037893e+10
    0x4f1ec0c1, // p[5] =   2.66343245e+09
    0x4ce7b23d, // p[6] =        121475560
    0x4a141ef5, // p[7] =       2426813.25
    0x46666416, // p[8] =       14745.0215
};

static __constant unsigned int __slgamma_ep___q3[] = {
    0x52cfd4fe, // q[0] =   4.46315823e+11
    0x529f237e, // q[1] =    3.4174763e+11
    0x51bd64f9, // q[2] =   1.01680357e+11
    0x505dd21b, // q[3] =   1.48861368e+10
    0x4e859e4d, // q[4] =   1.12087206e+09
    0x4c1dc2b8, // q[5] =         41356000
    0x491c19c9, // q[6] =       639388.562
    0x4528287c, // q[7] =       2690.53027
    0xbf800000, // q[8] =               -1
};

static __constant unsigned int __slgamma_ep___s[] = {
    0xbfd28d33, //      -1.64493406
    0xbf0a8993, //     -0.541161716
    0xbead9f8d, //     -0.339107901
    0xbe809fb7, //     -0.251218528
    0xbe49f046, //     -0.197205633
    0xbe43506b, //     -0.190736458
    0xbd222966, //    -0.0395902619
    0xbeac398d, //     -0.336376578
};

//! ===========================================================================
//! @brief Absolute value computation routine
//!
//! @param[in] x       Argument
//! @return            Returns absolute value
//! ===========================================================================
__attribute__((always_inline))
inline float __slgamma_ep_own_abs_fp32 (float x)
{
    return (x > 0.0f) ? x : ((x == 0.0f) ? 0.0f : (-x));
}   // static inline float _VSTATIC(own_abs_fp32)(float x)

//! ===========================================================================
//! @brief 32-bit natural logarithm value computation routine
//!
//! @param[in] a       Argument
//! @return            Returns 32-bit natural logarithm
//! ===========================================================================
__attribute__((always_inline))
inline float __slgamma_ep_own_log_fp32 (float a)
{
    unsigned int iHiDelta = 0x0u;
    unsigned int iLoRange = 0x0u;
    unsigned int iBrkValue = 0x0u;
    unsigned int iOffExpoMask = 0x0u;
    unsigned int iRangeMask = 0x0u;
    unsigned int iX = 0x0u;
    unsigned int iXTest = 0x0u;
    signed int iN = 0x0;
    unsigned int iR = 0x0u;
    unsigned int vm = 0x0u;
    int denorm_scale_exp = 0;

    float r = 0.0f;
    float sOne = 0.0f;
    float sLn2 = 0.0f;
    float sPoly[8] = { 0.0f };
    float sP = 0.0f;
    float va1 = 0.0f;
    float vr1 = 0.0f;
    float sN = 0.0f;
    float sR = 0.0f;

    struct
    {
        unsigned int sPoly[8];
        unsigned int iHiDelta;
        unsigned int iLoRange;
        unsigned int iBrkValue;
        unsigned int iOffExpoMask;
        unsigned int sOne;
        unsigned int sLn2;
        unsigned int sInfs[2];
        unsigned int sOnes[2];
        unsigned int sZeros[2];
    } own_log_fp32_data = {
        {0xbf000000u, 0x3eaaaa94u, 0xbe80058eu, 0x3e4ce190u, 0xbe28ad37u, 0x3e0fcb12u, 0xbe1ad9e3u, 0x3e0d84edu},
        0x00800000u, 0x01000000u, 0x3f2aaaabu, 0x007fffffu, 0x3f800000u, 0x3f317218u,
        {0x7f800000u, 0xff800000u}, {0x3f800000u, 0xbf800000u}, {0x00000000u, 0x80000000u}
    };

    va1 = a;

    /* argument is denormalized or [+/-]0 */
    if (as_uint (va1) <= 0x007fffff)
    {
        /* Scale */
        unsigned int _denorm_scale = 0x4D000000;    // 2^27
        float denorm_scale = *(float *) &_denorm_scale;
        va1 *= denorm_scale;
        denorm_scale_exp = 27;
    }

    iHiDelta = (own_log_fp32_data.iHiDelta);
    iLoRange = (own_log_fp32_data.iLoRange);

    iX = as_uint (va1);

    iXTest = (iX + iHiDelta);
    iRangeMask = ((unsigned int) (-(signed int) ((signed int) iXTest < (signed int) iLoRange)));
    iBrkValue = (own_log_fp32_data.iBrkValue);
    iOffExpoMask = (own_log_fp32_data.iOffExpoMask);

    iX = (iX - iBrkValue);
    iR = (iX & iOffExpoMask);
    iN = ((signed int) iX >> (23));
    iN = iN - denorm_scale_exp;
    iR = (iR + iBrkValue);
    sN = ((float) ((int) (iN)));
    sR = as_float (iR);
    vm = iRangeMask;

    sOne = as_float (own_log_fp32_data.sOne);
    sR = (sR - sOne);

    sPoly[7] = as_float (own_log_fp32_data.sPoly[7]);
    sPoly[6] = as_float (own_log_fp32_data.sPoly[6]);
    sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) ((sPoly[7]), (sR), (sPoly[6]));

    sPoly[5] = as_float (own_log_fp32_data.sPoly[5]);
    sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) ((sP), (sR), (sPoly[5]));

    sPoly[4] = as_float (own_log_fp32_data.sPoly[4]);
    sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) ((sP), (sR), (sPoly[4]));

    sPoly[3] = as_float (own_log_fp32_data.sPoly[3]);
    sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) ((sP), (sR), (sPoly[3]));

    sPoly[2] = as_float (own_log_fp32_data.sPoly[2]);
    sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) ((sP), (sR), (sPoly[2]));

    sPoly[1] = as_float (own_log_fp32_data.sPoly[1]);
    sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) ((sP), (sR), (sPoly[1]));

    sPoly[0] = as_float (own_log_fp32_data.sPoly[0]);
    sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) ((sP), (sR), (sPoly[0]));

    sP = (sP * sR);
    sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) ((sP), (sR), (sR));

    sLn2 = as_float (own_log_fp32_data.sLn2);
    vr1 = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) ((sN), (sLn2), (sP));

    r = vr1;

    return r;
}   // static inline float _VSTATIC(own_log_fp32) (float a)

//! ===========================================================================
//! @brief 32-bit nearbyint computation routine
//!
//! @param[in] sA      Argument
//! @return            Returns 32-bit nearbyint
//! ===========================================================================
__attribute__((always_inline))
inline float __slgamma_ep_own_nearbyint_fp32 (float sA)
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
inline float __slgamma_ep_own_ceilf_fp32 (float sA)
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
inline float __slgamma_ep_own_lgamma_fast_fp32 (float arg)
{

    unsigned int __slgamma_ep_own_log2pif = 0x3feb3f8eu;
    float result = 0.5f * ((*(float *) &(__slgamma_ep_own_log2pif)) - SPIRV_OCL_BUILTIN(log, _f32, ) (arg));
    result = result + (arg * (SPIRV_OCL_BUILTIN(log, _f32, ) (arg + (1.0f / ((12.0f * arg) - (1.0f / (10.0f * arg))))) - 1.0f));
    return result;
}   // static inline float _VSTATIC(own_lgamma_fast_fp32) (float arg)

//! ===========================================================================
//! @brief 32-bit lgamma approximation for positive arguments in [0.0; 2^13)
//!
//! @param[in] arg     Argument
//! @return            Returns 64-bit lgamma
//!
//! ===========================================================================
__attribute__((always_inline))
inline float __slgamma_ep_own_lgamma_pos_fp32 (float arg)
{
    float result;

    if (arg >= 12.0f)
    {
        result = __slgamma_ep_own_lgamma_fast_fp32 (arg);
    }
    else
    {
        // ===========================================================================
        //   W. J. Cody and K. E. Hillstrom, 'Chebyshev Approximations for
        //   the Natural Logarithm of the Gamma Function,' Math. Comp. 21,
        //   1967, pp. 198-203.
        // ===========================================================================
        __constant float *_p1 = (__constant float *) __slgamma_ep___p1;
        __constant float *_q1 = (__constant float *) __slgamma_ep___q1;
        __constant float *_p2 = (__constant float *) __slgamma_ep___p2;
        __constant float *_q2 = (__constant float *) __slgamma_ep___q2;
        __constant float *_p3 = (__constant float *) __slgamma_ep___p3;
        __constant float *_q3 = (__constant float *) __slgamma_ep___q3;

        unsigned int _brk0p6 = 0x3f2e0000;  // ~0.68
        unsigned int _xsmall = 0x257ff2d6;  // ~0.22e-15
        float brk0p6 = *(float *) &_brk0p6;
        float xsmall = *(float *) &_xsmall;

        float qq, pp, rr, rrr, c0;
        __constant float *_p, *_q;

        if (arg >= 4.0)
        {
            rr = arg - 4.0f;
            c0 = 0.0f;
            _p = _p3;
            _q = _q3;
            rrr = 1.0;
        }
        else if (arg >= 1.5)
        {
            rr = arg - 2.0f;
            c0 = 0.0f;
            _p = _p2;
            _q = _q2;
            rrr = rr;
        }
        else if (arg > brk0p6)
        {
            rr = arg - 1.0f;
            c0 = 0.0f;
            _p = _p1;
            _q = _q1;
            rrr = rr;
        }
        else if (arg > 0.5)
        {
            rr = arg - 1.0f;
            c0 = -SPIRV_OCL_BUILTIN(log, _f32, ) (arg);
            _p = _p2;
            _q = _q2;
            rrr = rr;
        }
        else if (arg > xsmall)
        {
            rr = arg;
            c0 = -SPIRV_OCL_BUILTIN(log, _f32, ) (arg);
            _p = _p1;
            _q = _q1;
            rrr = rr;
        }
        else    // arg <= xsmall
        {
            rr = 0.0;
            c0 = -SPIRV_OCL_BUILTIN(log, _f32, ) (arg);
            _p = _p1;
            _q = _q1;
            rrr = rr;
        }

        pp = _p[1] + rr * (_p[2] + rr * (_p[3] + rr * (_p[4] + rr * (_p[5] + rr * (_p[6] + rr * (_p[7] + rr * _p[8]))))));
        qq = _q[0] + rr * (_q[1] + rr * (_q[2] + rr * (_q[3] + rr * (_q[4] + rr * (_q[5] + rr * (_q[6] + rr * (_q[7] + rr * _q[8])))))));
        result = c0 + rrr * (_p[0] + rr * (pp / qq));
    }
    return result;
}

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
    float ldx = 0.0f, t0 = 0.0f, t1 = 0.0f, t2 = 0.0f, result = 0.0f, y = 0.0f, y2 = 0.0f;
    float p = 0.0f, r1 = 0.0f, r2 = 0.0f;
    signed int ix = ((signed int) (((_iml_lg_sp_union_t *) & x)->hex));
    unsigned int iabsx = ((((_iml_lg_sp_union_t *) & x)->hex) & 0x7fffffff);
    unsigned int xsig = (((_iml_lg_sp_union_t *) & x)->hex >> 31);
    unsigned int exp_x = ((((_iml_lg_sp_union_t *) & x)->hex >> 23) & 0xFF);
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
        unsigned int __big = 0x7f7fffff;
        float _big = *(float *) &__big;

        // OVERFLOW_BOUNDARY <= x <= +Max
        // Return +Infinity. Raise Inexact and Overflow.
        (*r) = _big * _big;
        nRet = 3;
    }
    else if (x > 0.0f)
    {
        (*r) = __slgamma_ep_own_lgamma_pos_fp32 (x);
    }
    else    // x < 0.0f
    {
        __constant float *_s = (__constant float *) __slgamma_ep___s;
        unsigned int __ln_sqrt_2_pi = 0x3f6b3f8e;   //      0.918938518
        float _ln_sqrt_2_pi = *(float *) &__ln_sqrt_2_pi;

        f = __slgamma_ep_own_nearbyint_fp32 (x);
        intx = (x == f);    /* Is x an integer? */
        if (!intx && (x > (__slgamma_ep__TWO_23H[1])) && !(((int) __slgamma_ep_own_ceilf_fp32 (x)) & 1))
        {
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
        else if (x > -12.0)
        {
            // x > -12
            y = x;
            p = 1.0f;
            while (y < 1.0f)
            {
                p = p * y;
                y = y + 1.0f;
            }

            p = SPIRV_OCL_BUILTIN(fabs, _f32, ) (p);
            p = SPIRV_OCL_BUILTIN(log, _f32, ) (p);
            result = __slgamma_ep_own_lgamma_pos_fp32 (y);
            result = result - p;
            (*r) = result;
        }
        else if (x > -8192.0f)
        {
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

            result = SPIRV_OCL_BUILTIN(log, _f32, ) (-x);
            result = -result;
            y = __slgamma_ep_own_lgamma_pos_fp32 (ldx);
            result = result - y;

            p = (((((((_s[7] * r2 + _s[6]) * r2 + _s[5]) * r2 + _s[4]) * r2 + _s[3]) * r2 + _s[2]) * r2 + _s[1]) * r2 + _s[0]) * r2;

            result = result - p;

            r1 = SPIRV_OCL_BUILTIN(fabs, _f32, ) (r1);
            y = SPIRV_OCL_BUILTIN(log, _f32, ) (r1);
            result = result - y;
            (*r) = result;
        }
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
            result = _ln_sqrt_2_pi;
            ldx = x;
            t1 = ldx - 0.5f;
            t2 = -ldx;
            t2 = SPIRV_OCL_BUILTIN(log, _f32, ) (-x);
            t1 = t1 * t2;
            result = t1 - result;

            result = result - ldx;

            f = (__slgamma_ep__TWO_23H[0]);
            f = f - x;
            f = f - (__slgamma_ep__TWO_23H[0]);

            r1 = -ldx;
            r1 = r1 - f;
            r2 = r1 * r1;

            p = (((((((_s[7] * r2 + _s[6]) * r2 + _s[5]) * r2 + _s[4]) * r2 + _s[3]) * r2 + _s[2]) * r2 + _s[1]) * r2 + _s[0]) * r2;

            result = result - p;

            r1 = SPIRV_OCL_BUILTIN(fabs, _f32, ) (r1);
            y = SPIRV_OCL_BUILTIN(log, _f32, ) (r1);
            result = result - y;
            (*r) = result;
        }
    }   // else if ( exp_x == IML_EXPINF_32 )

    return nRet;
}   // _VAPI_COUT_SCOPE int _VAPI_COUT_NAME( __constant float* a, float* r)

float __ocl_svml_lgammaf (float a)
{
    float va1;
    float vr1;
    unsigned int vm;

    float r;

    va1 = a;;

    __internal_slgamma_ep_cout (&va1, &vr1);
    r = vr1;;

    return r;
}

/*===================== begin_copyright_notice ==================================

Copyright (c) 2022 Intel Corporation

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

#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF
static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dsin_ha_nolut_INV_PI64 = { 0x3fd45f306dc9c883uL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dsin_ha_nolut_SHIFTER = { 0x4328000000000000uL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dsin_ha_nolut_PI1_BITS = { 0xc00921fb54442d18uL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dsin_ha_nolut_PI2_BITS = { 0xbca1a62633000000uL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dsin_ha_nolut_PI3_BITS = { 0xbaa45c06e0e68948uL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dsin_ha_nolut_two = { 0x4000000000000000uL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dsin_ha_nolut_half = { 0x3fe0000000000000uL };

// cos(x) polynomial. |x|<pi/4
static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dsin_ha_nolut_c_coeff[] = {
    {0x3ff0000000000000uL}, {0xbfe0000000000000uL}, {0x3fa555555555554buL},
    {0xbf56C16C16C14E4AuL}, {0x3efA01A019C687A9uL}, {0xbe927E4F7D8E4BF3uL},
    {0x3e21EE9CCB7C6DBFuL}, {0xbda8F9F637C8424CuL},
};

// sin(x) polynomial, |x|<pi/4
static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dsin_ha_nolut_s_coeff[] = {
    {0}, {0}, {0xbfc5555555555543uL}, {0x3f8111111110f3e0uL},
    {0xbf2a01a019ba9d47uL}, {0x3ec71de35310c44buL}, {0xbe5ae5e382f0f31cuL},
    {0x3de5d877cbbe8daauL},
};

static __constant unsigned long __dsin_ha_nolut_AbsMask = 0x7fffffffffffffffuL;
static __constant unsigned long __dsin_ha_nolut_zero = 0x0000000000000000uL;
///////
// 2^1152/(2*Pi)
static __constant unsigned int __dsin_ha_nolut_InvPi_tbl[] = { 0, 0,
    0x28BE60DBu, 0x9391054Au, 0x7F09D5F4u, 0x7D4D3770u,
    0x36D8A566u, 0x4F10E410u, 0x7F9458EAu, 0xF7AEF158u,
    0x6DC91B8Eu, 0x909374B8u, 0x01924BBAu, 0x82746487u,
    0x3F877AC7u, 0x2C4A69CFu, 0xBA208D7Du, 0x4BAED121u,
    0x3A671C09u, 0xAD17DF90u, 0x4E64758Eu, 0x60D4CE7Du,
    0x272117E2u, 0xEF7E4A0Eu, 0xC7FE25FFu, 0xF7816603u,
    0xFBCBC462u, 0xD6829B47u, 0xDB4D9FB3u, 0xC9F2C26Du,
    0xD3D18FD9u, 0xA797FA8Bu, 0x5D49EEB1u, 0xFAF97C5Eu,
    0xCF41CE7Du, 0xE294A4BAu, 0x9AFED7ECu, 0x47E35742u,
    0x1580CC11u, 0xBF1EDAEAu, 0, 0, 0, 0
};

// (2*pi)*2^61
static __constant unsigned int __dsin_ha_nolut_two_pi[2] = { 0x2168C235u, 0xC90FDAA2u };

// unsigned 32-bit shift
// signed 32-bit shift
// unsigned 64-bit shift
// signed 64-bit shift
// reduce argument to (-2*pi/(2^kbits), 2*pi/(2^kbits)) range
static inline double __dsin_ha_nolut_trig_reduction (double x, int kbits, double *py_low, int *pinterv)
{
    unsigned long ix, sgn_x, abs_x, mx, R;
    unsigned long S[2], P2, P3, P4, P5, L, L2, Lh, P23, P23_l, Msk;
    long e_yl, e_yh;
    volatile unsigned int T[7];
    unsigned int mant[2], Sh, Sl;
    int Syl;
    int expon_x, j, shift, shift2, scale, interv = 0;
    int cond;
    double yl, res;
    union
    {
        unsigned long w;
        unsigned int w32[2];
        int s32[2];
        double f;
    } ixd;
    ixd.f = x;
    ix = ixd.w;
    abs_x = ix & 0x7fffffffffffffffuL;
    sgn_x = ix ^ abs_x;
    // biased exponent
    expon_x = (((unsigned int) ((unsigned int) (abs_x >> 32))) >> (20));
    // mantissa
    mant[0] = (unsigned int) abs_x;
    mant[1] = (((unsigned int) (abs_x >> 32)) & 0xfffffu) | 0x100000u;
    // eliminate smaller |x|, as well as Inf/NaN
    cond = ((unsigned) (expon_x - 0x400) >= (0x7ff - 0x400));
    if (cond)
    {
        *py_low = 0;
        *pinterv = 0;
        return x;
    }
    // starting table index for argument reduction
    // j >= 1 for expon_x >= 20+0x3ff
    expon_x = expon_x + 12 - 0x3ff;
    j = (((unsigned int) (expon_x)) >> (5));
    // look up table values
    T[0] = __dsin_ha_nolut_InvPi_tbl[j];
    T[1] = __dsin_ha_nolut_InvPi_tbl[j + 1];
    T[2] = __dsin_ha_nolut_InvPi_tbl[j + 2];
    T[3] = __dsin_ha_nolut_InvPi_tbl[j + 3];
    T[4] = __dsin_ha_nolut_InvPi_tbl[j + 4];
    T[5] = __dsin_ha_nolut_InvPi_tbl[j + 5];
    T[6] = __dsin_ha_nolut_InvPi_tbl[j + 6];
    // shift in [0, 31]
    shift = expon_x - (j << 5);
    // shift left
    if (shift)
    {
        shift2 = 32 - shift;
        T[0] = (T[0] << shift) | (((unsigned int) (T[1])) >> (shift2));
        T[1] = (T[1] << shift) | (((unsigned int) (T[2])) >> (shift2));
        T[2] = (T[2] << shift) | (((unsigned int) (T[3])) >> (shift2));
        T[3] = (T[3] << shift) | (((unsigned int) (T[4])) >> (shift2));
        T[4] = (T[4] << shift) | (((unsigned int) (T[5])) >> (shift2));
        T[5] = (T[5] << shift) | (((unsigned int) (T[6])) >> (shift2));
    }
    // InvPi*mant_x
    S[0] = (((unsigned long) ((unsigned int) (T[3]))) * ((unsigned int) (mant[0])));
    P4 = (((unsigned long) ((unsigned int) (T[4]))) * ((unsigned int) (mant[1])));
    L = (((unsigned long) ((unsigned int) (T[4]))) * ((unsigned int) (mant[0])));
    L2 = (((unsigned long) ((unsigned int) (T[5]))) * ((unsigned int) (mant[1])));
    L += L2;
    if (L < L2)
        S[0] += 0x100000000uL;
    P2 = (((unsigned long) ((unsigned int) (T[2]))) * ((unsigned int) (mant[0])));
    S[1] =
        (((unsigned long) ((unsigned int) (T[1]))) * ((unsigned int) (mant[0]))) +
        (((unsigned long) (((unsigned int) T[0]) * ((unsigned int) mant[0]))) << 32);
    S[0] += P4;
    if (S[0] < P4)
        S[1]++;
    Lh = (((unsigned long) (L)) >> (32));
    L <<= 32;
    S[0] += Lh;
    if (S[0] < Lh)
        S[1]++;
    P3 = (((unsigned long) ((unsigned int) (T[3]))) * ((unsigned int) (mant[1])));
    S[1] =
        S[1] + (((unsigned long) ((unsigned int) (T[2]))) * ((unsigned int) (mant[1]))) +
        (((unsigned long) (((unsigned int) T[1]) * ((unsigned int) mant[1]))) << 32);
    // accumulate terms
    P23 = P2 + P3;
    // add carry
    if (P23 < P3)
        S[1] += 0x100000000uL;
    S[1] += (((unsigned long) (P23)) >> (32));
    P23_l = P23 << 32;
    S[0] += P23_l;
    if (S[0] < P23_l)
        S[1]++;
    if (kbits)
    {
        shift2 = 32 - kbits;
        interv = (((unsigned long) ((((unsigned long) (S[1])) >> (32)))) >> (shift2));
        S[1] = (S[1] << kbits) | (((unsigned long) ((((unsigned long) (S[0])) >> (32)))) >> (shift2));
        S[0] = (S[0] << kbits) | (((unsigned long) ((((unsigned long) (L)) >> (32)))) >> (shift2));
        L <<= kbits;
    }
    // round intev to nearest
    Msk = (((long) (S[1])) >> (63));
    interv = interv - (int) Msk;
    S[1] ^= Msk;
    S[0] ^= Msk;
    L ^= Msk;
    // apply sign to interv, then correction to sign of fraction
    sgn_x = (((long) (sgn_x)) >> (63));
    *pinterv = (interv ^ (int) sgn_x) - (int) sgn_x;
    sgn_x = (sgn_x ^ Msk) & 0x8000000000000000uL;
    scale = -64 - kbits;
    // normalization: leading bit of S[1] should be 1
    while (((long) S[1]) > 0)
    {
        scale--;
        S[1] = (S[1] << 1) | (((unsigned long) (S[0])) >> (63));
        S[0] = (S[0] << 1) | (((unsigned long) (L)) >> (63));
        L <<= 1;
    }
    // multiply by 2*Pi*(2^61)
    Sh = (unsigned int) (((unsigned long) (S[1])) >> (32));
    Sl = (unsigned int) S[1];
    R = (((unsigned long) ((unsigned int) (Sh))) * ((unsigned int) (__dsin_ha_nolut_two_pi[1])));
    P2 = (((unsigned long) ((unsigned int) (Sh))) * ((unsigned int) (__dsin_ha_nolut_two_pi[0])));
    P3 = (((unsigned long) ((unsigned int) (Sl))) * ((unsigned int) (__dsin_ha_nolut_two_pi[1])));
    // accumulate terms
    P23 = P2 + P3;
    // add carry
    if (P23 < P3)
        R++;
    // R is result*2^(scale+3)
    R += (((unsigned long) (P23)) >> (32));
    scale += 3;
    // normalize
    if (((long) R) < 0)
    {
        R = (((unsigned long) (R)) >> (1));
        scale++;
    }
    // round upper 53 bits
    Syl = (unsigned int) R;
    R += (unsigned long) (1 << 9);
    // determine y_low
    Syl <<= (32 - 10);
    Syl = (((int) (Syl)) >> (32 - 10));
    // SINT32 to double conversion
    yl = (double) Syl;
    // adjust exponent of yl
    e_yl = (long) scale;
    e_yl = ((e_yl + 0x3ff) << 52) | sgn_x;
    // y_low
    *py_low = yl * (*(double *) &e_yl);
    // exponent of high part
    e_yh = (unsigned long) (scale + 62 - 1 + 0x3ff);
    e_yh = (e_yh << 52);
    // high part of result
    R = e_yh + (((unsigned long) (R)) >> (10));
    *(unsigned long *) &res = sgn_x ^ R;
    return res;
}

inline int __ocl_svml_internal_dsin_ha_noLUT (double *a, double *pres)
{
    int nRet = 0;
    double xin = *a;
    unsigned leading_xh;
    int cond;
    int index;
    double dN, Rh, Rm, R, dNP2_h, R2, dNP3_h;
    double mNP2_l, mNP3_l, Rl, R3;
    union
    {
        unsigned long w;
        unsigned int w32[2];
        int s32[2];
        double f;
    } x, x0, res[2], dS, sin_res;
    double s_poly, c_poly, R_Rl, R2h, R2l, Ch;
    unsigned long R_sgn, C_sgn, lindex, sgn_x;
    union
    {
        unsigned long w;
        unsigned int w32[2];
        int s32[2];
        double f;
    } c7, c6, c5, c4, c3, c2, c_poly2, s_poly2;
    x0.f = xin;
    x.f = __spirv_ocl_fabs (xin);    // x.w = x0.w & (_VSTATIC(AbsMask));
    sgn_x = x0.w ^ x.w;
    // redirect special cases
    leading_xh = ((unsigned) x.w32[1]); // & 0x7fffffff;
    cond = ((unsigned) (leading_xh) >= (0x41300000 - 0));
    if (cond)
        goto SIN_SPECIAL;
    // _VSTATIC(SHIFTER) + x*(1/pi)
    dS.f = __spirv_ocl_fma(x.f,(__dsin_ha_nolut_INV_PI64).f,(__dsin_ha_nolut_SHIFTER).f);
    // N ~ x*(1/pi)
    dN = dS.f - (__dsin_ha_nolut_SHIFTER).f;
    R_sgn = (dS.w >> 1);
    R_sgn <<= 63;
    R_sgn ^= sgn_x;
    lindex = dS.w & 1;
    // Rh = x - N*PI1
    Rh = __spirv_ocl_fma(dN,(__dsin_ha_nolut_PI1_BITS).f,x.f);
    // Rm = Rh - N*PI2
    Rm = __spirv_ocl_fma(dN,(__dsin_ha_nolut_PI2_BITS).f,Rh);//FENCE(Rh-dNP2);
    // R = Rm - N*PI3
    R = __spirv_ocl_fma(dN,(__dsin_ha_nolut_PI3_BITS).f,Rm);//FENCE(Rm-dNP3);
    // (N*PI2)_high
    dNP2_h = Rh - Rm;
    // R^2
    R2 = R * R;
    // (N*PI3)_high
    dNP3_h = Rm - R;
    // -(N*PI2)_l
    mNP2_l = __spirv_ocl_fma(dN,(__dsin_ha_nolut_PI2_BITS).f,dNP2_h);//FENCE(dNP2_h-dNP2);
    // -(N*PI3)_l
    mNP3_l = __spirv_ocl_fma(dN,(__dsin_ha_nolut_PI3_BITS).f,dNP3_h);//FENCE(dNP3_h-dNP3);
    // Rl=-(N*PI2)_l-(N*PI3)_l
    Rl = mNP2_l + mNP3_l;
//SINCOS_MAIN_PATH:
    // R*Rl
    R_Rl = R * Rl;
    c7.w = (lindex == 0) ? __dsin_ha_nolut_s_coeff[7].w : __dsin_ha_nolut_c_coeff[7].w;
    c6.w = (lindex == 0) ? __dsin_ha_nolut_s_coeff[6].w : __dsin_ha_nolut_c_coeff[6].w;
    c5.w = (lindex == 0) ? __dsin_ha_nolut_s_coeff[5].w : __dsin_ha_nolut_c_coeff[5].w;
    c4.w = (lindex == 0) ? __dsin_ha_nolut_s_coeff[4].w : __dsin_ha_nolut_c_coeff[4].w;
    c3.w = (lindex == 0) ? __dsin_ha_nolut_s_coeff[3].w : __dsin_ha_nolut_c_coeff[3].w;
    c2.w = (lindex == 0) ? __dsin_ha_nolut_s_coeff[2].w : __dsin_ha_nolut_c_coeff[2].w;
    c_poly = __spirv_ocl_fma(R2,c7.f,c6.f);
    c_poly = __spirv_ocl_fma(R2,c_poly,c5.f);
    c_poly = __spirv_ocl_fma(R2,c_poly,c4.f);
    c_poly = __spirv_ocl_fma(R2,c_poly,c3.f);
    c_poly = __spirv_ocl_fma (R2, c_poly, c2.f) * R2;
    // 2.0 - R^2
    Ch = __spirv_ocl_fma((-R),R,(__dsin_ha_nolut_two).f);
    // (-R^2)_high
    R2h = Ch - (__dsin_ha_nolut_two).f;
    // (R^2)_low
    R2l = __spirv_ocl_fma(R,R,R2h);
    // 0.5*R2l+ R*Rl
    R2l = __spirv_ocl_fma(R2l,(__dsin_ha_nolut_half).f,R_Rl);
    c_poly2.f = __spirv_ocl_fma(R2,c_poly,(-R2l));
    c_poly2.f = __spirv_ocl_fma(Ch,(__dsin_ha_nolut_half).f,c_poly2.f);
    s_poly2.f = __spirv_ocl_fma(R,c_poly,Rl);//s_poly=DP_FMA(R3,s_poly,Rl);
    s_poly2.f = s_poly2.f + R;
    // hopefully, a SEL will be generated
    sin_res.w = (lindex == 0) ? s_poly2.w : c_poly2.w;
    sin_res.w ^= R_sgn;
    *pres = sin_res.f;
    return nRet;
  SIN_SPECIAL:
    // Inf/NaN?
    if (leading_xh >= 0x7ff00000)
    {
        // NaN?
        if ((x.w & (__dsin_ha_nolut_AbsMask)) > 0x7ff0000000000000uL)
        {
            *pres = x.f * x.f;
            return nRet;
        }
        res[0].w = 0xfff8000000000000uL;
        *pres = res[0].f;
        nRet = 1;
        return nRet;
    }
    R = __dsin_ha_nolut_trig_reduction (x.f, 2, &Rl, &index);
    R2 = R * R;
    lindex = (unsigned long) index;
    lindex = (index >> 1);
    lindex <<= 63;
    R_sgn = (lindex);
    lindex = index & 1;
    R_sgn ^= sgn_x;
    // goto SINCOS_MAIN_PATH;
    // R^3
    R3 = R2 * R;
    // add low part of initial reduction (for large arguments)
    //Rl = FENCE(Rl + xlow);
    // R*Rl
    R_Rl = R * Rl;
    c_poly = __spirv_ocl_fma(R2,(__dsin_ha_nolut_c_coeff[7]).f,(__dsin_ha_nolut_c_coeff[6]).f);
    s_poly = __spirv_ocl_fma(R2,(__dsin_ha_nolut_s_coeff[7]).f,(__dsin_ha_nolut_s_coeff[6]).f);
    c_poly = __spirv_ocl_fma(R2,c_poly,(__dsin_ha_nolut_c_coeff[5]).f);
    s_poly = __spirv_ocl_fma(R2,s_poly,(__dsin_ha_nolut_s_coeff[5]).f);
    c_poly = __spirv_ocl_fma(R2,c_poly,(__dsin_ha_nolut_c_coeff[4]).f);
    s_poly = __spirv_ocl_fma(R2,s_poly,(__dsin_ha_nolut_s_coeff[4]).f);
    c_poly = __spirv_ocl_fma(R2,c_poly,(__dsin_ha_nolut_c_coeff[3]).f);
    s_poly = __spirv_ocl_fma(R2,s_poly,(__dsin_ha_nolut_s_coeff[3]).f);
    c_poly = __spirv_ocl_fma (R2, c_poly, (__dsin_ha_nolut_c_coeff[2]).f) * R2;
    s_poly = __spirv_ocl_fma(R2,s_poly,(__dsin_ha_nolut_s_coeff[2]).f);
    // 2.0 - R^2
    Ch = __spirv_ocl_fma((-R),R,(__dsin_ha_nolut_two).f);
    // (-R^2)_high
    R2h = Ch - (__dsin_ha_nolut_two).f;
    // (R^2)_low
    R2l = __spirv_ocl_fma(R,R,R2h);
    // 0.5*R2l+ R*Rl
    R2l = __spirv_ocl_fma(R2l,(__dsin_ha_nolut_half).f,R_Rl);
    c_poly = __spirv_ocl_fma(R2,c_poly,(-R2l));
    res[0].f = __spirv_ocl_fma(Ch,(__dsin_ha_nolut_half).f,c_poly);
    s_poly = __spirv_ocl_fma(R3,s_poly,Rl);
    res[1].f = s_poly + R;
    // hopefully, a SEL will be generated
    //sin_res.f = (index == 0) ? res[0].f : res[1].f;
    lindex = 0 - lindex;
    sin_res.w = res[1].w ^ ((res[1].w ^ res[0].w) & lindex);
    (sin_res).w ^= R_sgn;
    *pres = sin_res.f;
    return nRet;
}

double __ocl_svml_sin_ha_noLUT (double a)
{
    double r;
    __ocl_svml_internal_dsin_ha_noLUT (&a, &r);
    return r;
}

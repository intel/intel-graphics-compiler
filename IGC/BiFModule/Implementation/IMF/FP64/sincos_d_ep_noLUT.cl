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
} __dsincos_ep_INV_PI64 = { 0x3fd45f306dc9c883UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dsincos_ep_SHIFTER = { 0x4328000000000000UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dsincos_ep_PI1_BITS = { 0x400921fb54442d18UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dsincos_ep_PI2_BITS = { 0x3ca1a62633000000UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dsincos_ep_PI3_BITS = { 0x3aa45c06e0e68948UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dsincos_ep_two = { 0x4000000000000000UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dsincos_ep_half = { 0x3fe0000000000000UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dsincos_ep_c_coeff[] = {
    {0x3feffffffff97c47UL}, {0xbfdffffffbdee95eUL}, {0x3fa55553a875b0c5UL},
    {0xbf56c078624df59aUL}, {0x3ef9906ffd58bf6dUL}, {0},
};

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dsincos_ep_s_coeff[] = {
    {0xbe2a670b2f4764e4UL}, {0xbfc55553fe6f4534UL}, {0x3f8110630f1a5f72UL},
    {0xbf299076bb901430UL},
};

static __constant unsigned long __dsincos_ep_AbsMask = 0x7fffffffffffffffUL;
static __constant unsigned long __dsincos_ep_zero = 0x0000000000000000UL;
static __constant unsigned int __dsincos_ep_InvPi_tbl[] = { 0, 0,
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
static __constant unsigned int __dsincos_ep_two_pi[2] = { 0x2168C235u, 0xC90FDAA2u };

__attribute__((always_inline))
static inline double __dsincos_ep_trig_noLUT_reduction (double x, int kbits, double *py_low, int *pinterv)
{
    unsigned long ix, sgn_x, abs_x, mx, R;
    unsigned long S[2], P2, P3, P4, P5, L, L2, Lh, P23, P23_l, Msk;
    long e_yl, e_yh;
    unsigned int mant[2], T[7], Sh, Sl;
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
    abs_x = ix & 0x7fffffffffffffffUL;
    sgn_x = ix ^ abs_x;
    expon_x = (((unsigned int) ((unsigned int) (abs_x >> 32))) >> (20));
    mant[0] = (unsigned int) abs_x;
    mant[1] = (((unsigned int) (abs_x >> 32)) & 0xfffffu) | 0x100000u;
    cond = ((unsigned) (expon_x - 0x400) >= (0x7ff - 0x400));
    if (cond)
    {
        *py_low = 0;
        *pinterv = 0;
        return x;
    }
    expon_x = expon_x + 12 - 0x3ff;
    j = (((unsigned int) (expon_x)) >> (5));
    T[0] = __dsincos_ep_InvPi_tbl[j];
    T[1] = __dsincos_ep_InvPi_tbl[j + 1];
    T[2] = __dsincos_ep_InvPi_tbl[j + 2];
    T[3] = __dsincos_ep_InvPi_tbl[j + 3];
    T[4] = __dsincos_ep_InvPi_tbl[j + 4];
    T[5] = __dsincos_ep_InvPi_tbl[j + 5];
    T[6] = __dsincos_ep_InvPi_tbl[j + 6];
    shift = expon_x - (j << 5);
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
    S[0] = (((unsigned long) ((unsigned int) (T[3]))) * ((unsigned int) (mant[0])));
    P4 = (((unsigned long) ((unsigned int) (T[4]))) * ((unsigned int) (mant[1])));
    L = (((unsigned long) ((unsigned int) (T[4]))) * ((unsigned int) (mant[0])));
    L2 = (((unsigned long) ((unsigned int) (T[5]))) * ((unsigned int) (mant[1])));
    L += L2;
    if (L < L2)
        S[0] += 0x100000000UL;
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
    P23 = P2 + P3;
    if (P23 < P3)
        S[1] += 0x100000000UL;
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
    Msk = (((long) (S[1])) >> (63));
    interv = interv - (int) Msk;
    S[1] ^= Msk;
    S[0] ^= Msk;
    L ^= Msk;
    sgn_x = (((long) (sgn_x)) >> (63));
    *pinterv = (interv ^ (int) sgn_x) - (int) sgn_x;
    sgn_x = (sgn_x ^ Msk) & 0x8000000000000000UL;
    scale = -64 - kbits;
    while (((long) S[1]) > 0)
    {
        scale--;
        S[1] = (S[1] << 1) | (((unsigned long) (S[0])) >> (63));
        S[0] = (S[0] << 1) | (((unsigned long) (L)) >> (63));
        L <<= 1;
    }
    Sh = (unsigned int) (((unsigned long) (S[1])) >> (32));
    Sl = (unsigned int) S[1];
    R = (((unsigned long) ((unsigned int) (Sh))) * ((unsigned int) (__dsincos_ep_two_pi[1])));
    P2 = (((unsigned long) ((unsigned int) (Sh))) * ((unsigned int) (__dsincos_ep_two_pi[0])));
    P3 = (((unsigned long) ((unsigned int) (Sl))) * ((unsigned int) (__dsincos_ep_two_pi[1])));
    P23 = P2 + P3;
    if (P23 < P3)
        R++;
    R += (((unsigned long) (P23)) >> (32));
    scale += 3;
    if (((long) R) < 0)
    {
        R = (((unsigned long) (R)) >> (1));
        scale++;
    }
    Syl = (unsigned int) R;
    R += (unsigned long) (1 << 9);
    Syl <<= (32 - 10);
    Syl = (((int) (Syl)) >> (32 - 10));
    yl = (double) Syl;
    e_yl = (long) scale;
    e_yl = ((e_yl + 0x3ff) << 52) | sgn_x;
    *py_low = yl * (*(double *) &e_yl);
    e_yh = (unsigned long) (scale + 62 - 1 + 0x3ff);
    e_yh = (e_yh << 52);
    R = e_yh + (((unsigned long) (R)) >> (10));
    *(unsigned long *) &res = sgn_x ^ R;
    return res;
}

__attribute__((always_inline))
inline int __internal_dsincos_ep_nolut_cout (double *a, double *psin, double *pcos)
{
    int nRet = 0;
    double xin = *a;
    unsigned leading_xh;
    int cond;
    int idx;
    double dN, Rh, dNP2, dNP3, Rm, R, dNP2_h, R2, dNP3_h;
    double mNP2_l, mNP3_l, Rl, R3;
    union
    {
        unsigned long w;
        unsigned int w32[2];
        int s32[2];
        double f;
    } x, x0, res[2], dS, sin_res, cos_res;
    double s_poly, c_poly, R_Rl, R2h, R2l, Ch;
    unsigned long sgn_x, R_sgn, C_sgn, index, lindex;
    x0.f = xin;
    x.w = x0.w & (__dsincos_ep_AbsMask);
    sgn_x = (x0).w ^ (x).w;
    leading_xh = ((unsigned) x.w32[1]);
    cond = ((unsigned) (leading_xh) >= (0x41300000 - 0));
    if (cond)
        goto SINCOS_SPECIAL;
    dS.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (x.f, (__dsincos_ep_INV_PI64).f, (__dsincos_ep_SHIFTER).f);
    dN = dS.f - (__dsincos_ep_SHIFTER).f;
    C_sgn = (dS).w << 63;
    R_sgn = ((dS).w >> 1) << 63;
    index = dS.w & 1;
    R = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (-dN, (__dsincos_ep_PI1_BITS).f, x.f);
    R = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (-dN, (__dsincos_ep_PI2_BITS).f, R);
    R2 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (R, R, 0.0);
  SINCOS_MAIN_PATH:
    c_poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (R2, (__dsincos_ep_c_coeff[4]).f, (__dsincos_ep_c_coeff[3]).f);
    s_poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (R2, (__dsincos_ep_s_coeff[3]).f, (__dsincos_ep_s_coeff[2]).f);
    c_poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (R2, c_poly, (__dsincos_ep_c_coeff[2]).f);
    c_poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (R2, c_poly, (__dsincos_ep_c_coeff[1]).f);
    s_poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (R2, s_poly, (__dsincos_ep_s_coeff[1]).f);
    c_poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (R2, c_poly, (__dsincos_ep_c_coeff[0]).f);
    s_poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (R2, s_poly, (__dsincos_ep_s_coeff[0]).f);
    s_poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (R, s_poly, R);
    sin_res.f = (index == 0) ? s_poly : c_poly;
    cos_res.f = (index == 0) ? c_poly : s_poly;
    sin_res.w ^= (sgn_x ^ R_sgn);
    cos_res.w ^= (C_sgn ^ R_sgn);
    *pcos = cos_res.f;
    *psin = sin_res.f;
    return nRet;
  SINCOS_SPECIAL:
    if (leading_xh >= 0x7ff00000)
    {
        if (x.w == 0x7ff0000000000000UL)
        {
            res[0].w = 0xfff8000000000000UL;
            *pcos = res[0].f;
            *psin = res[0].f;
            nRet = 1;
            return nRet;
        }
        *psin = *pcos = xin + xin;
        return nRet;
    }
    R = __dsincos_ep_trig_noLUT_reduction (x.f, 2, &Rl, &idx);
    R2 = R * R;
    lindex = index = (unsigned long) idx;
    lindex <<= 63;
    C_sgn = (lindex);
    lindex = (index >> 1);
    lindex <<= 63;
    R_sgn = (lindex);
    index &= 1;
    goto SINCOS_MAIN_PATH;
}

void __ocl_svml_sincos_ep_noLUT (double a,  double *b, __global double *c)
{
    double va1;
    double vr1;
    double vr2;
    va1 = a;
    __internal_dsincos_ep_nolut_cout (&va1, &vr1, &vr2);
    ((double *) b)[0] = vr1;
    ((double *) c)[0] = vr2;
    return;
}

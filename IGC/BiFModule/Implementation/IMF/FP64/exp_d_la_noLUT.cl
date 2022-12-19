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
} __dexp_la_nolut_p_L2E = { 0x3ff71547652B82FEUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dexp_la_nolut_p_Shifter = { 0x43280000000007feUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dexp_la_nolut_p_NL2H = { 0xbfe62e42fefa39efUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dexp_la_nolut_p_NL2L = { 0xbc7abc9e3b39803fUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dexp_la_nolut_p_c0 = { 0x3fdffffffffffe76UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dexp_la_nolut_p_c1 = { 0x3fc5555555555462UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dexp_la_nolut_p_c2 = { 0x3fa55555556228ceUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dexp_la_nolut_p_c3 = { 0x3f811111111ac486UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dexp_la_nolut_p_c4 = { 0x3f56c16b8144bd5bUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dexp_la_nolut_p_c5 = { 0x3f2a019f7560fba3UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dexp_la_nolut_p_c6 = { 0x3efa072e44b58159UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dexp_la_nolut_p_c7 = { 0x3ec722bccc270959UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dexp_la_nolut_p_one = { 0x3ff0000000000000UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dexp_la_nolut_thres = { 0x4086232A00000000UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dexp_la_nolut_min_norm = { 0x0010000000000000UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dexp_la_nolut_Inf = { 0x7ff0000000000000UL };

__attribute__((always_inline))
inline int __internal_dexp_nolut_cout (double *a, double *r)
{
    int nRet = 0;
    double x = *a;
    union
    {
        unsigned long w;
        unsigned int w32[2];
        int s32[2];
        double f;
    } xi, zero, res_special, scale;
    union
    {
        unsigned long w;
        unsigned int w32[2];
        int s32[2];
        double f;
    } idx, T, Tlr;
    double N, R, R0, poly, res;
    int expon32, mask32, mask_h;
    unsigned int xa32, sgn_x, expon_corr;
    idx.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (x, __dexp_la_nolut_p_L2E.f, __dexp_la_nolut_p_Shifter.f);
    N = idx.f - __dexp_la_nolut_p_Shifter.f;
    mask32 = idx.w32[0] << 31;
    expon32 = idx.w32[0] << (20 + 31 - 32);
    R0 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (__dexp_la_nolut_p_NL2H.f, N, x);
    R = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (__dexp_la_nolut_p_NL2L.f, N, R0);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (__dexp_la_nolut_p_c7.f, R, __dexp_la_nolut_p_c6.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dexp_la_nolut_p_c5.f);
    mask32 = mask32 >> 31;
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dexp_la_nolut_p_c4.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dexp_la_nolut_p_c3.f);
    mask_h = mask32 & 0x000EA09E;
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dexp_la_nolut_p_c2.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dexp_la_nolut_p_c1.f);
    T.w32[1] = expon32 ^ mask_h;
    T.w32[0] = mask32 & 0x667F3BCD;
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dexp_la_nolut_p_c0.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dexp_la_nolut_p_one.f);
    poly = poly * R;
    if (SPIRV_OCL_BUILTIN(fabs, _f64, ) (x) >= __dexp_la_nolut_thres.f)
        goto EXP_SPECIAL_PATH;
    res = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (T.f, poly, T.f);
    *r = res;
    return nRet;
  EXP_SPECIAL_PATH:
    xi.f = x;
    xa32 = xi.w32[1] & 0x7fffffffu;
    sgn_x = xa32 ^ xi.w32[1];
    if (xa32 < 0x40879127u)
    {
        expon_corr = sgn_x ? 0x08000000u : 0xF8000000u;
        scale.w = sgn_x ? 0x37f0000000000000UL : 0x47f0000000000000UL;
        T.w32[1] += expon_corr;
        res = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (T.f, poly, T.f);
        res *= scale.f;
    }
    else
    {
        res_special.w = sgn_x ? 0x0000000000000000UL : 0x7ff0000000000000UL;
        xi.w32[1] = xa32;
        res_special.f = (xi.w <= 0x7ff0000000000000UL) ? res_special.f : x;
        zero.w = 0;
        res = res_special.f + zero.f;
    }
    nRet = (res < __dexp_la_nolut_min_norm.f) ? 4 : nRet;
    nRet = (res == __dexp_la_nolut_Inf.f) ? 3 : nRet;
    *r = res;
    return nRet;
}

double __ocl_svml_exp_noLUT (double a)
{
    double va1;
    double vr1;
    double r;
    va1 = a;
    __internal_dexp_nolut_cout (&va1, &vr1);
    r = vr1;
    return r;
}

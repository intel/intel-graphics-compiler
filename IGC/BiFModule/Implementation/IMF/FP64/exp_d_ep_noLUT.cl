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
    unsigned int w;
    float f;
} __dexp_ep_p_L2Ef = { 0x3fB8AA3Bu };

static __constant union
{
    unsigned int w;
    float f;
} __dexp_ep_p_Shifterf0 = { 0x4b4003ffu };

static __constant union
{
    unsigned int w;
    float f;
} __dexp_ep_p_fthres = { 0x4431195c };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dexp_ep_p_NL2H = { 0xbfe62e42fefa39efUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dexp_ep_p_NL2L = { 0xbc7abc9e3b39803fUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dexp_ep_c6 = { 0x3f56dd9818211af0UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dexp_ep_c5 = { 0x3f8126fababd1cf2UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dexp_ep_c4 = { 0x3fa55541c4c8cb89UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dexp_ep_c3 = { 0x3fc55540432ea07bUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dexp_ep_c2 = { 0x3fe00000090aa64aUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dexp_ep_c1 = { 0x3ff000000a208385UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dexp_ep_c0 = { 0xbdd63f26cce7780fUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dexp_ep_min_norm = { 0x0010000000000000UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dexp_ep_Inf = { 0x7ff0000000000000UL };

__attribute__((always_inline))
inline int __internal_dexp_ep_nolut_cout (double *a, double *r)
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
    } T;
    double N, R, R0, poly, res;
    int expon32, mask32, mask_h;
    unsigned int xa32, sgn_x, expon_corr;
    union
    {
        unsigned int w;
        float f;
    } idx;
    float fN, xf;
    xf = (float) x;
    idx.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (xf, __dexp_ep_p_L2Ef.f, __dexp_ep_p_Shifterf0.f);
    fN = idx.f - __dexp_ep_p_Shifterf0.f;
    N = (double) fN;
    T.w32[1] = idx.w << 20;
    T.w32[0] = 0;
    R = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (__dexp_ep_p_NL2H.f, N, x);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (__dexp_ep_c6.f, R, __dexp_ep_c5.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dexp_ep_c4.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dexp_ep_c3.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dexp_ep_c2.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dexp_ep_c1.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dexp_ep_c0.f);
    if (SPIRV_OCL_BUILTIN(fabs, _f32, ) (xf) > __dexp_ep_p_fthres.f)
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
    nRet = (res < __dexp_ep_min_norm.f) ? 4 : nRet;
    nRet = (res == __dexp_ep_Inf.f) ? 3 : nRet;
    *r = res;
    return nRet;
}

double __ocl_svml_exp_ep_noLUT (double a)
{
    double va1;
    double vr1;
    double r;
    va1 = a;
    __internal_dexp_ep_nolut_cout (&va1, &vr1);
    r = vr1;
    return r;
}

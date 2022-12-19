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
} __dln_ep___c9 = { 0xbfc11e70a5c9b8f8UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dln_ep___c8 = { 0x3fc20827ee8835feUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dln_ep___c7 = { 0xbfbedf6494cc1f86UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dln_ep___c6 = { 0x3fc1e531c40397e0UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dln_ep___c5 = { 0xbfc55d72615e74d3UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dln_ep___c4 = { 0x3fc99dac1eadbf8eUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dln_ep___c3 = { 0xbfcfffcff3489b95UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dln_ep___c2 = { 0x3fd5554dfa222c07UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dln_ep___c1 = { 0xbfe00000145aea06UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dln_ep___c0 = { 0x3e30eba0cce2e0b0UL };

__attribute__((always_inline))
inline int __internal_dln_ep_nolut_cout (double *a, double *r)
{
    int nRet = 0;
    union
    {
        unsigned long w;
        unsigned int w32[2];
        int s32[2];
        double f;
    } x, expon, expon_r, one, l2;
    double R, d_expon;
    union
    {
        unsigned long w;
        unsigned int w32[2];
        int s32[2];
        double f;
    } denorm_scale;
    double poly, res;
    union
    {
        unsigned long w;
        unsigned int w32[2];
        int s32[2];
        double f;
    } _res;
    int denorm_scale_exp;
    x.f = *a;
    if ((x.w == 0x0uL) || (x.w >= 0x7ff0000000000000uL))
    {
        if ((x.w & 0x7fffffffffffffff) == 0x0uL)
        {
            nRet = 2;
            _res.w = 0xfff0000000000000uL;
            *r = _res.f;
            return nRet;
        }
        else if (x.w > 0x8000000000000000uL)
        {
            nRet = 1;
            _res.w = x.w | 0xfff8000000000000uL;
            *r = _res.f;
            return nRet;
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
            *r = _res.f;
            return nRet;
        }
    }
    denorm_scale.w = 0x43B0000000000000ull;
    denorm_scale_exp = (x.w <= 0x000fffffffffffffuL) ? (60 + 0x3FF) : 0x3FF;
    x.f = (x.w <= 0x000fffffffffffffuL) ? (x.f * denorm_scale.f) : x.f;
    expon.w = x.w + 0x000AAAAAAAAAAAAAull;
    expon.w >>= 52;
    expon_r.w = expon.w << 52;
    one.w = 0x3FF0000000000000ull;
    x.w = (x.w + one.w) - expon_r.w;
    R = x.f - one.f;
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (__dln_ep___c9.f, R, __dln_ep___c8.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dln_ep___c7.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dln_ep___c6.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dln_ep___c5.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dln_ep___c4.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dln_ep___c3.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dln_ep___c2.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dln_ep___c1.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dln_ep___c0.f);
    expon.s32[0] -= denorm_scale_exp;
    d_expon = (double) expon.s32[0];
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, R);
    l2.w = 0x3FE62E42FEFA39EFull;
    res = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (d_expon, l2.f, poly);
    *r = res;
    return nRet;
}

double __ocl_svml_log_ep_noLUT (double a)
{
    double va1;
    double vr1;
    double r;
    va1 = a;
    __internal_dln_ep_nolut_cout (&va1, &vr1);
    r = vr1;
    return r;
}

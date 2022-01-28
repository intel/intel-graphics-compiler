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
} __dln_la___c19 = { 0xbfb6e22682c05596UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dln_la___c18 = { 0x3fb6c694b21a9875UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dln_la___c17 = { 0xbfa68f0acee35e2dUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dln_la___c16 = { 0x3fa9474ccd075ce5UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dln_la___c15 = { 0xbfb0750f4f9c34f9UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dln_la___c14 = { 0x3fb16608748ab72dUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dln_la___c13 = { 0xbfb23e2ec341eba0UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dln_la___c12 = { 0x3fb3aa521d980cd0UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dln_la___c11 = { 0xbfb555fa23866d76UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dln_la___c10 = { 0x3fb74629a554d880UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dln_la___c9 = { 0xbfb999938abcf213UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dln_la___c8 = { 0x3fbc71c472fb2195UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dln_la___c7 = { 0xbfc00000112830d9UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dln_la___c6 = { 0x3fc24924982c2697UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dln_la___c5 = { 0xbfc55555551fbbdbUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dln_la___c4 = { 0x3fc99999998c68b5UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dln_la___c3 = { 0xbfd0000000002697UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dln_la___c2 = { 0x3fd5555555555b0eUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dln_la___c1 = { 0xbfdffffffffffff0UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dln_la___c0 = { 0xbc8a30cfded694ffUL };

__attribute__((always_inline))
inline int __internal_dln_nolut_cout (double *a, double *r)
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
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (__dln_la___c19.f, R, __dln_la___c18.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dln_la___c17.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dln_la___c16.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dln_la___c15.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dln_la___c14.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dln_la___c13.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dln_la___c12.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dln_la___c11.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dln_la___c10.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dln_la___c9.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dln_la___c8.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dln_la___c7.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dln_la___c6.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dln_la___c5.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dln_la___c4.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dln_la___c3.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dln_la___c2.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dln_la___c1.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dln_la___c0.f);
    expon.s32[0] -= denorm_scale_exp;
    d_expon = (double) expon.s32[0];
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, R);
    l2.w = 0x3FE62E42FEFA39EFull;
    res = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (d_expon, l2.f, poly);
    *r = res;
    return nRet;
}

double __ocl_svml_log_noLUT (double a)
{
    double va1;
    double vr1;
    double r;
    va1 = a;
    __internal_dln_nolut_cout (&va1, &vr1);
    r = vr1;
    return r;
}

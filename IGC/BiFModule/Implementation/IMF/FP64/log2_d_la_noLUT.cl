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

__attribute__((always_inline))
inline int __internal_dlog2_nolut_cout (double *a, double *r)
{
    int nRet = 0;
    union
    {
        unsigned long w;
        unsigned int w32[2];
        int s32[2];
        double f;
    } x, expon, expon_r, one, l2;
    double R, R2, R4, d_expon;
    union
    {
        unsigned long w;
        unsigned int w32[2];
        int s32[2];
        double f;
    } c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, c10, c11, c12, c13, c14, c15, c16, c17, c18, c19, denorm_scale;
    double P1819, P1617, P1415, P1213, P1011, P89, P67, P45, P23, P01, P1619, P1215, P811, P47, P03, P1219, P819, P419, P019, poly, res;
    union
    {
        unsigned long w;
        unsigned int w32[2];
        int s32[2];
        double f;
    } _res;
    int denorm_scale_exp = 0;
    denorm_scale.w = 0x43B0000000000000ull;
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
    P1819 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (c19.f, x.f, c18.f);
    c17.w = 0x3fa49f86632433feull;
    c16.w = 0xbfb5ea03fef4c746ull;
    P1617 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (c17.f, x.f, c16.f);
    R2 = R * R;
    c15.w = 0x3faf2a14615c2bb3ull;
    c14.w = 0xbfb062accb1ad8aaull;
    P1415 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (c15.f, R, c14.f);
    c13.w = 0x3fb1038ce60c1b2full;
    c12.w = 0xbfb2406abbb6c334ull;
    P1213 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (c13.f, R, c12.f);
    c11.w = 0x3fb3b219a9287c7full;
    c10.w = 0xbfb555d0d4781fd1ull;
    P1011 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (c11.f, R, c10.f);
    c9.w = 0x3fb745c847eeb960ull;
    c8.w = 0xbfb99995585870b8ull;
    P89 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (c9.f, R, c8.f);
    c7.w = 0x3fbc71c758cfdb39ull;
    c6.w = 0xbfc000000b3d2e0full;
    P67 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (c7.f, R, c6.f);
    P1619 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (P1819, R2, P1617);
    R4 = R2 * R2;
    P1215 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (P1415, R2, P1213);
    c5.w = 0x3fc2492491d4fd71ull;
    c4.w = 0xbfc555555534c686ull;
    P45 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (c5.f, R, c4.f);
    c3.w = 0x3fc99999999a7fc1ull;
    c2.w = 0xbfd0000000001596ull;
    P23 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (c3.f, R, c2.f);
    P811 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (P1011, R2, P89);
    P1219 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (P1619, R4, P1215);
    c1.w = 0x3fd55555555554fcull;
    c0.w = 0xbfdffffffffffff8ull;
    P01 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (c1.f, R, c0.f);
    P47 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (P67, R2, P45);
    P819 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (P1219, R4, P811);
    P03 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (P23, R2, P01);
    P419 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (P819, R4, P47);
    expon.w -= 0x3FF;
    expon.s32[0] -= denorm_scale_exp;
    P019 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (P419, R4, P03);
    d_expon = (double) expon.s32[0];
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (R2, P019, R);
    l2.w = 0x3ff71547652b82feull;
    res = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (l2.f, poly, d_expon);
    *r = res;
    return nRet;
}

double __ocl_svml_log2_noLUT (double a)
{
    double va1;
    double vr1;
    double r;
    va1 = a;
    __internal_dlog2_nolut_cout (&va1, &vr1);
    r = vr1;
    return r;
}

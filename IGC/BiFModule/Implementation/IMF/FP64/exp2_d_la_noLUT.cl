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
} __dexp2_la_Shifter = { 0x43380000000003ffuL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dexp2_la_Shifter0 = { 0x43380000000007feuL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dexp2_la_c11 = { 0x3dfea1c678ded0efuL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dexp2_la_c10 = { 0x3e3e6228be5a9ffduL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dexp2_la_c9 = { 0x3e7b524ca9ff39ccuL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dexp2_la_c8 = { 0x3eb62bfc2c7be078uL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dexp2_la_c7 = { 0x3eeffcbfc7e3f872uL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dexp2_la_c6 = { 0x3f2430913112cae8uL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dexp2_la_c5 = { 0x3f55d87fe78a0586uL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dexp2_la_c4 = { 0x3f83b2ab6fb9f1a3uL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dexp2_la_c3 = { 0x3fac6b08d704a0dbuL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dexp2_la_c2 = { 0x3fcebfbdff82c5aeuL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dexp2_la_c1 = { 0x3fc8b90bfbe8e7bcuL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dexp2_la_thres = { 0x408ff00000000000uL };

__attribute__((always_inline))
inline int __ocl_svml_internal_dexp2_noLUT (double *a, double *r)
{
    int nRet = 0;
    double x = *a;
    union
    {
        unsigned long w;
        unsigned int w32[2];
        int s32[2];
        double f;
    } dS, xin, res, T, T2, expon;
    double dN, R, poly, High, Rh, Rl;
    unsigned int xa32;
    int sN, sN2;
    dS.f = x + __dexp2_la_Shifter.f;
    dN = dS.f - __dexp2_la_Shifter.f;
    R = x - dN;
    poly = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (__dexp2_la_c11.f, R, __dexp2_la_c10.f);
    poly = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (poly, R, __dexp2_la_c9.f);
    poly = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (poly, R, __dexp2_la_c8.f);
    poly = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (poly, R, __dexp2_la_c7.f);
    poly = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (poly, R, __dexp2_la_c6.f);
    poly = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (poly, R, __dexp2_la_c5.f);
    High = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (R, 0.5, 1.0);
    poly = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (poly, R, __dexp2_la_c4.f);
    poly = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (poly, R, __dexp2_la_c3.f);
    poly = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (poly, R, __dexp2_la_c2.f);
    poly = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (poly, R, __dexp2_la_c1.f);
    res.f = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (poly, R, High);
    if (SPIRV_OCL_BUILTIN (fabs, _f64,) (x) >= __dexp2_la_thres.f)
        goto EXP2_SPECIAL;
    // final scaling
    //res.w32[1] += (dS.w32[0] << 20);
    expon.w = dS.w << 52;
    res.f *= expon.f;
    *r = res.f;
    return nRet;
  EXP2_SPECIAL:
    xin.f = x;
    xa32 = xin.w32[1] & 0x7fffffffuL;
    if (xa32 >= 0x7ff00000u)
    {
        if (xin.w == 0xfff0000000000000uL)
        {
            *r = 0.0f;
            return nRet;
        }
        else    // NaN or +Inf
        {
            *r = x + x;
            return nRet;
        }
    }
    x = SPIRV_OCL_BUILTIN (fmin, _f64_f64,) (x, 1536.0);
    x = SPIRV_OCL_BUILTIN (fmax, _f64_f64,) (x, -1536.0);
    dS.f = x + __dexp2_la_Shifter0.f;
    sN = dS.w32[0];
    // fix res.f for very large |x|
    res.f = SPIRV_OCL_BUILTIN (fmin, _f64_f64,) (res.f, 2.0);
    res.f = SPIRV_OCL_BUILTIN (fmax, _f64_f64,) (res.f, 0.5);
    // split the scaling coefficients
    sN2 = sN >> 1;
    sN -= sN2;
    T.w = (sN /*+ 0x3ff */ );
    T.w <<= 52;
    T2.w = (sN2 /*+ 0x3ff */ );
    T2.w <<= 52;
    res.f *= T.f;
    res.f *= T2.f;
    nRet = (res.w < 0x0010000000000000uL) ? 4 : nRet;
    nRet = (res.w == 0x7ff0000000000000uL) ? 3 : nRet;
    *r = res.f;
    return nRet;
}

double __ocl_svml_exp2_noLUT (double a)
{
    double r;
    __ocl_svml_internal_dexp2_noLUT (&a, &r);
    return r;
}

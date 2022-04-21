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
} __dcbrt_la___c18 = { 0xbf5258530f7103eeuL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dcbrt_la___c17 = { 0x3f6f30182507f509uL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dcbrt_la___c16 = { 0xbf7986b7eabb0f78uL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dcbrt_la___c15 = { 0x3f7d87b2b4a9762auL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dcbrt_la___c14 = { 0xbf7efbebaa4d1f28uL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dcbrt_la___c13 = { 0x3f80bd903b4ac117uL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dcbrt_la___c12 = { 0xbf82a96763bdeb5buL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dcbrt_la___c11 = { 0x3f85095e87087a1buL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dcbrt_la___c10 = { 0xbf87f17e129ff05cuL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dcbrt_la___c9 = { 0x3f8b9fee9509eb5duL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dcbrt_la___c8 = { 0xbf9036dc67cd7b0duL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dcbrt_la___c7 = { 0x3f93750a6fcca6b6uL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dcbrt_la___c6 = { 0xbf98090d6918910duL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dcbrt_la___c5 = { 0x3f9ee71136c830fduL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dcbrt_la___c4 = { 0xbfa511e8d2ad4e91uL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dcbrt_la___c3 = { 0x3faf9add3c0aff15uL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dcbrt_la___c2 = { 0xbfbc71c71c71c90cuL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dcbrt_la___c1 = { 0x3fd5555555555572uL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dcbrt_la___c0 = { 0x3c5338db1759ca85uL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dcbrt_la___maxnum = { 0x7fefffffffffffffuL };

inline int __ocl_svml_internal_dcbrt_noLUT (double *a, double *pres)
{
    int nRet = 0;
    double xin = *a;
    int_double xa, res;
    unsigned long sgn_x, ecorr, expon, k, j, P;
    double dR;
    int_double poly, scale, two_j, mant, r_expon;
    xa.f = xin;
    sgn_x = xa.w & 0x8000000000000000uL;
    // |xin|
    xa.w ^= sgn_x;
    // will scale denormals by 2^69
    scale.w = (xa.w < 0x0010000000000000uL) ? 0x4440000000000000uL : 0x3ff0000000000000uL;
    // final exponent correction
    ecorr = (xa.w < 0x0010000000000000uL) ? 0x2aa - 23 : 0x2aa;
    xa.f = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (xa.f, scale.f, 0.0);
    // input exponent (expon = 3*k+j)
    expon = (xa.w + 0x0008000000000000uL) >> 52;
    // (2^32+2)/3  * exponent
    P = (unsigned long) expon *(unsigned long) 0x55555556uL;
    k = P >> 32;
    j = expon - k - k - k;
    r_expon.w = (k + ecorr) << 52;
    // correction for xin==0.0
    r_expon.w = (!xa.w) ? 0 : r_expon.w;
    // mantissa
    mant.w = (xa.w - (expon << 52)) + 0x3ff0000000000000uL;
    // reduced argument
    dR = mant.f - 1;
    // polynomial evaluation
    poly.f = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (__dcbrt_la___c18.f, dR, __dcbrt_la___c17.f);
    poly.f = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (poly.f, dR, __dcbrt_la___c16.f);
    poly.f = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (poly.f, dR, __dcbrt_la___c15.f);
    poly.f = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (poly.f, dR, __dcbrt_la___c14.f);
    poly.f = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (poly.f, dR, __dcbrt_la___c13.f);
    poly.f = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (poly.f, dR, __dcbrt_la___c12.f);
    poly.f = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (poly.f, dR, __dcbrt_la___c11.f);
    poly.f = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (poly.f, dR, __dcbrt_la___c10.f);
    poly.f = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (poly.f, dR, __dcbrt_la___c9.f);
    poly.f = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (poly.f, dR, __dcbrt_la___c8.f);
    poly.f = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (poly.f, dR, __dcbrt_la___c7.f);
    poly.f = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (poly.f, dR, __dcbrt_la___c6.f);
    poly.f = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (poly.f, dR, __dcbrt_la___c5.f);
    poly.f = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (poly.f, dR, __dcbrt_la___c4.f);
    poly.f = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (poly.f, dR, __dcbrt_la___c3.f);
    poly.f = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (poly.f, dR, __dcbrt_la___c2.f);
    poly.f = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (poly.f, dR, __dcbrt_la___c1.f);
    poly.f = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (poly.f, dR, __dcbrt_la___c0.f);
    // 2^j
    two_j.w = (!j) ? 0x3ff0000000000000uL : 0x3ff428A2F98D728BuL;
    two_j.w = (j <= 1) ? two_j.w : 0x3ff965FEA53D6E3DuL;
    // attach exponent
    two_j.f = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (two_j.f, r_expon.f, 0.0);
    res.f = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (two_j.f, poly.f, two_j.f);
    // fixup for Inf/NaN
    res.f = (xa.f <= __dcbrt_la___maxnum.f) ? res.f : (xa.f + xa.f);
    // set sign
    res.w ^= sgn_x;
    *pres = res.f;
    return nRet;
}

double __ocl_svml_cbrt_noLUT (double a)
{
    double r;
    __ocl_svml_internal_dcbrt_noLUT (&a, &r);
    return r;
}

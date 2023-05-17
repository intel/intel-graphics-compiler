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
} __scbrt_la___c9 = { 0x3bcfaf07u };

static __constant union
{
    unsigned int w;
    float f;
} __scbrt_la___c8 = { 0xbc771231u };

static __constant union
{
    unsigned int w;
    float f;
} __scbrt_la___c7 = { 0x3ca35241u };

static __constant union
{
    unsigned int w;
    float f;
} __scbrt_la___c6 = { 0xbcc266f9u };

static __constant union
{
    unsigned int w;
    float f;
} __scbrt_la___c5 = { 0x3cf6f381u };

static __constant union
{
    unsigned int w;
    float f;
} __scbrt_la___c4 = { 0xbd2880e9u };

static __constant union
{
    unsigned int w;
    float f;
} __scbrt_la___c3 = { 0x3d7cd740u };

static __constant union
{
    unsigned int w;
    float f;
} __scbrt_la___c2 = { 0xbde38e56u };

static __constant union
{
    unsigned int w;
    float f;
} __scbrt_la___c1 = { 0x3eaaaaabu };

static __constant union
{
    unsigned int w;
    float f;
} __scbrt_la___c0 = { 0x30144e9bu };

static __constant union
{
    unsigned int w;
    float f;
} __scbrt_la___maxnum = { 0x7f7fffffu };

__attribute__((always_inline))
inline int __ocl_svml_internal_scbrt_noLUT (float *a, float *pres)
{
    int nRet = 0;
    float xin = *a;
    int_float xa, res;
    unsigned int sgn_x, ecorr, expon, k, j, P;
    float dR;
    int_float poly, pl, scale, two_j, two_jl, mant, r_expon;
    xa.f = xin;
    sgn_x = xa.w & 0x80000000;
    // |xin|
    xa.w ^= sgn_x;
    // will scale denormals by 2^69
    scale.w = (xa.w < 0x00800000) ? 0x62000000u : 0x3f800000u;
    // final exponent correction
    ecorr = (xa.w < 0x00800000) ? 85 - 23 : 85;
    xa.f = SPIRV_OCL_BUILTIN (fma, _f32_f32_f32,) (xa.f, scale.f, 0.0f);
    // input exponent (expon = 3*k+j); will subtract 1 from bias (so it is divisible by 3)
    expon = (xa.w + 0xffc00000) >> 23;
    // (2^32+2)/3  * exponent
    P = (unsigned long) expon *(unsigned long) 0x5556uL;
    k = P >> 16;
    j = expon - k - k - k;
    r_expon.w = (k + ecorr) << 23;
    // correction for xin==0.0
    r_expon.w = (!xa.w) ? 0 : r_expon.w;
    // mantissa
    mant.w = (xa.w - (expon << 23)) + 0x3f000000u;
    // reduced argument, range [-0.25,0.5]
    dR = mant.f - 1.0f;
    // polynomial evaluation
    poly.f = SPIRV_OCL_BUILTIN (fma, _f32_f32_f32,) (__scbrt_la___c9.f, dR, __scbrt_la___c8.f);
    poly.f = SPIRV_OCL_BUILTIN (fma, _f32_f32_f32,) (poly.f, dR, __scbrt_la___c7.f);
    poly.f = SPIRV_OCL_BUILTIN (fma, _f32_f32_f32,) (poly.f, dR, __scbrt_la___c6.f);
    poly.f = SPIRV_OCL_BUILTIN (fma, _f32_f32_f32,) (poly.f, dR, __scbrt_la___c5.f);
    poly.f = SPIRV_OCL_BUILTIN (fma, _f32_f32_f32,) (poly.f, dR, __scbrt_la___c4.f);
    poly.f = SPIRV_OCL_BUILTIN (fma, _f32_f32_f32,) (poly.f, dR, __scbrt_la___c3.f);
    poly.f = SPIRV_OCL_BUILTIN (fma, _f32_f32_f32,) (poly.f, dR, __scbrt_la___c2.f);
    poly.f = SPIRV_OCL_BUILTIN (fma, _f32_f32_f32,) (poly.f, dR, __scbrt_la___c1.f);
    poly.f = SPIRV_OCL_BUILTIN (fma, _f32_f32_f32,) (poly.f, dR, __scbrt_la___c0.f);
    // 2^j
    two_j.w = (!j) ? 0x3f800000u : 0x3FA14518u;
    two_j.w = (j <= 1) ? two_j.w : 0x3FCB2FF5u;
    // (2^j)_low/(2^j)_high
    two_jl.w = (!j) ? 0x0uL : 0xB223C16Cu;
    two_jl.w = (j <= 1) ? two_jl.w : 0x31D34318u;
    // attach exponent
    two_j.f = SPIRV_OCL_BUILTIN (fma, _f32_f32_f32,) (two_j.f, r_expon.f, 0.0f);
    pl.f = SPIRV_OCL_BUILTIN (fma, _f32_f32_f32,) (two_jl.f, poly.f, two_jl.f);
    poly.f = poly.f + pl.f;
    res.f = SPIRV_OCL_BUILTIN (fma, _f32_f32_f32,) (two_j.f, poly.f, two_j.f);
    // fixup for Inf/NaN
    res.f = (xa.f <= __scbrt_la___maxnum.f) ? res.f : (xa.f + xa.f);
    // set sign
    res.w ^= sgn_x;
    *pres = res.f;
    return nRet;
}

float __ocl_svml_cbrtf_noLUT (float a)
{
    float r;
    __ocl_svml_internal_scbrt_noLUT (&a, &r);
    return r;
}

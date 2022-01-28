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
} __serf_la___c7 = { 0xb6be1896u };

static __constant union
{
    unsigned int w;
    float f;
} __serf_la___c6 = { 0x38c10b13u };

static __constant union
{
    unsigned int w;
    float f;
} __serf_la___c5 = { 0xba52ecddu };

static __constant union
{
    unsigned int w;
    float f;
} __serf_la___c4 = { 0x3ba980dcu };

static __constant union
{
    unsigned int w;
    float f;
} __serf_la___c3 = { 0xbcdbd9a1u };

static __constant union
{
    unsigned int w;
    float f;
} __serf_la___c2 = { 0x3de71341u };

static __constant union
{
    unsigned int w;
    float f;
} __serf_la___c1 = { 0xbec09385u };

static __constant union
{
    unsigned int w;
    float f;
} __serf_la___c0 = { 0x3e0375d3u };

static __constant union
{
    unsigned int w;
    float f;
} __serf_la___r12 = { 0xb6924ad6u };

static __constant union
{
    unsigned int w;
    float f;
} __serf_la___r11 = { 0xb65c0d04u };

static __constant union
{
    unsigned int w;
    float f;
} __serf_la___r10 = { 0x38802491u };

static __constant union
{
    unsigned int w;
    float f;
} __serf_la___r9 = { 0xb8c69ea5u };

static __constant union
{
    unsigned int w;
    float f;
} __serf_la___r8 = { 0xb8c7c125u };

static __constant union
{
    unsigned int w;
    float f;
} __serf_la___r7 = { 0x3a2ef775u };

static __constant union
{
    unsigned int w;
    float f;
} __serf_la___r6 = { 0xbad8c682u };

static __constant union
{
    unsigned int w;
    float f;
} __serf_la___r5 = { 0x3b33fa8du };

static __constant union
{
    unsigned int w;
    float f;
} __serf_la___r4 = { 0xbb55960au };

static __constant union
{
    unsigned int w;
    float f;
} __serf_la___r3 = { 0x3b350662u };

static __constant union
{
    unsigned int w;
    float f;
} __serf_la___r2 = { 0xbad34f26u };

static __constant union
{
    unsigned int w;
    float f;
} __serf_la___r1 = { 0x3a19a9a7u };

static __constant union
{
    unsigned int w;
    float f;
} __serf_la___r0 = { 0x3f7ff968u };

__attribute__((always_inline))
inline int __internal_serf_nolut_cout (float *a, float *pres)
{
    int nRet = 0;
    float xin = *a;
    union
    {
        unsigned int w;
        float f;
    } x, xa, res;
    unsigned int sgn_x;
    float dR, dR2;
    union
    {
        unsigned int w;
        float f;
    } poly;
    xa.f = xin;
    sgn_x = xa.w & 0x80000000;
    xa.w ^= sgn_x;
    if (xa.f < 1.5f)
    {
        dR = xa.f;
        dR2 = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (dR, dR, 0.0f);
        poly.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (__serf_la___c7.f, dR2, __serf_la___c6.f);
        poly.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly.f, dR2, __serf_la___c5.f);
        poly.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly.f, dR2, __serf_la___c4.f);
        poly.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly.f, dR2, __serf_la___c3.f);
        poly.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly.f, dR2, __serf_la___c2.f);
        poly.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly.f, dR2, __serf_la___c1.f);
        poly.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly.f, dR2, __serf_la___c0.f);
        res.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly.f, dR, dR);
        res.w ^= sgn_x;
        *pres = res.f;
    }
    else
    {
        dR = (xa.f > 4.0f) ? 4.0f : xa.f;
        dR = (xa.w <= 0x7f800000) ? dR : xa.f;
        dR = dR - 2.75f;
        poly.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (__serf_la___r12.f, dR, __serf_la___r11.f);
        poly.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly.f, dR, __serf_la___r10.f);
        poly.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly.f, dR, __serf_la___r9.f);
        poly.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly.f, dR, __serf_la___r8.f);
        poly.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly.f, dR, __serf_la___r7.f);
        poly.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly.f, dR, __serf_la___r6.f);
        poly.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly.f, dR, __serf_la___r5.f);
        poly.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly.f, dR, __serf_la___r4.f);
        poly.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly.f, dR, __serf_la___r3.f);
        poly.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly.f, dR, __serf_la___r2.f);
        poly.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly.f, dR, __serf_la___r1.f);
        res.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly.f, dR, __serf_la___r0.f);
        res.w ^= sgn_x;
        *pres = res.f;
    }
    return nRet;
}

float __ocl_svml_erff_noLUT (float a)
{
    float va1;
    float vr1;
    float r;
    va1 = a;
    __internal_serf_nolut_cout (&va1, &vr1);
    r = vr1;
    return r;
}

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
} __serf_ep___b5 = { 0x3605524cu };

static __constant union
{
    unsigned int w;
    float f;
} __serf_ep___b4 = { 0x39953450u };

static __constant union
{
    unsigned int w;
    float f;
} __serf_ep___b3 = { 0x3b7e8d75u };

static __constant union
{
    unsigned int w;
    float f;
} __serf_ep___b2 = { 0x3d5983e4u };

static __constant union
{
    unsigned int w;
    float f;
} __serf_ep___b1 = { 0x3e4635acu };

static __constant union
{
    unsigned int w;
    float f;
} __serf_ep___b0 = { 0x3f906ebau };

static __constant union
{
    unsigned int w;
    float f;
} __serf_ep___a5 = { 0x381cf31fu };

static __constant union
{
    unsigned int w;
    float f;
} __serf_ep___a4 = { 0x3a9b6bd9u };

static __constant union
{
    unsigned int w;
    float f;
} __serf_ep___a3 = { 0x3c792ec0u };

static __constant union
{
    unsigned int w;
    float f;
} __serf_ep___a2 = { 0x3dec40c3u };

static __constant union
{
    unsigned int w;
    float f;
} __serf_ep___a1 = { 0x3f013f71u };

static __constant union
{
    unsigned int w;
    float f;
} __serf_ep___a0 = { 0x3f800000u };

__attribute__((always_inline))
inline int __internal_serf_ep_nolut_cout (float *a, float *pres)
{
    int nRet = 0;
    float xin = *a;
    union
    {
        unsigned int w;
        float f;
    } x, xa, res;
    int iexpon;
    unsigned int sgn_x;
    float dR, dR2;
    union
    {
        unsigned int w;
        float f;
    } apoly, bpoly, Y;
    xa.f = xin;
    sgn_x = xa.w & 0x80000000;
    xa.w ^= sgn_x;
    dR = (xa.f > 4.0f) ? 4.0f : xa.f;
    dR2 = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (dR, dR, 0.0f);
    dR = (xa.w > 0x7f800000UL) ? xa.f : dR;
    bpoly.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (__serf_ep___b5.f, dR2, __serf_ep___b4.f);
    apoly.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (__serf_ep___a5.f, dR2, __serf_ep___a4.f);
    bpoly.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (bpoly.f, dR2, __serf_ep___b3.f);
    apoly.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (apoly.f, dR2, __serf_ep___a3.f);
    bpoly.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (bpoly.f, dR2, __serf_ep___b2.f);
    apoly.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (apoly.f, dR2, __serf_ep___a2.f);
    bpoly.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (bpoly.f, dR2, __serf_ep___b1.f);
    apoly.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (apoly.f, dR2, __serf_ep___a1.f);
    bpoly.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (bpoly.f, dR2, __serf_ep___b0.f);
    apoly.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (apoly.f, dR2, __serf_ep___a0.f);
    Y.f = 1.0f / apoly.f;
    bpoly.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (bpoly.f, dR, 0.0f);
    res.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (bpoly.f, Y.f, 0.0f);
    res.f = (res.f > 1.0f) ? 1.0f : res.f;
    res.f = (xa.w <= 0x7f800000) ? res.f : (xa.f + xa.f);
    res.w ^= sgn_x;
    *pres = res.f;
    return nRet;
}

float __ocl_svml_erff_ep_noLUT (float a)
{
    float va1;
    float vr1;
    float r;
    va1 = a;
    __internal_serf_ep_nolut_cout (&va1, &vr1);
    r = vr1;
    return r;
}

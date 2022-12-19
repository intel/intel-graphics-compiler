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
} __derf_ep___b13 = { 0xbbf04820f8782ea7UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_ep___b12 = { 0x3c78c6a6ee56a902UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_ep___b11 = { 0x3d27d9f72ac50c4eUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_ep___b10 = { 0x3d95b9d8ba22f87fUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_ep___b9 = { 0x3ded59e0b9dba72cUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_ep___b8 = { 0x3e40ec0d169c5661UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_ep___b7 = { 0x3e88ca50312635b8UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_ep___b6 = { 0x3ed1666b61a7daa8UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_ep___b5 = { 0x3f0eab6025e6fe88UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_ep___b4 = { 0x3f4c99b8f2e353d0UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_ep___b3 = { 0x3f7c3fbd04a37ceeUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_ep___b2 = { 0x3fb0f935cf0c418cUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_ep___b1 = { 0x3fcc0406ecc19ca4UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_ep___b0 = { 0x3ff20dd750429b6eUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_ep___a13 = { 0xbc3dfa7e57cc8417UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_ep___a12 = { 0x3ce0a482679e10e7UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_ep___a11 = { 0x3d63909d10c4a13bUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_ep___a10 = { 0x3dc2a830735cd361UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_ep___a9 = { 0x3e17313932e30be7UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_ep___a8 = { 0x3e653467a0d8d5e2UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_ep___a7 = { 0x3eae22653b5c366aUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_ep___a6 = { 0x3ef0f7db44b1d85bUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_ep___a5 = { 0x3f2e53e7405862b5UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_ep___a4 = { 0x3f6538c5ce02a52dUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_ep___a3 = { 0x3f968ff8f52c8468UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_ep___a2 = { 0x3fc13830f163e2e9UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_ep___a1 = { 0x3fe0dfad7312eba5UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_ep___a0 = { 0x3ff0000000000000UL };

__attribute__((always_inline))
inline int __internal_derf_ep_nolut_cout (double *a, double *pres)
{
    int nRet = 0;
    double xin = *a;
    union
    {
        unsigned long w;
        unsigned int w32[2];
        int s32[2];
        double f;
    } x, xa, res;
    unsigned long sgn_x;
    float fy, fa;
    double dR, dR2, eps;
    union
    {
        unsigned long w;
        unsigned int w32[2];
        int s32[2];
        double f;
    } apoly, bpoly, Y, Q, Ql;
    xa.f = xin;
    sgn_x = xa.w & 0x8000000000000000UL;
    xa.w ^= sgn_x;
    dR = (6.0 < xa.f) ? 6.0 : xa.f;
    dR2 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dR, dR, 0.0);
    dR = (xa.w > 0x7ff0000000000000UL) ? xa.f : dR;
    bpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (__derf_ep___b13.f, dR2, __derf_ep___b12.f);
    apoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (__derf_ep___a13.f, dR2, __derf_ep___a12.f);
    bpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (bpoly.f, dR2, __derf_ep___b11.f);
    apoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (apoly.f, dR2, __derf_ep___a11.f);
    bpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (bpoly.f, dR2, __derf_ep___b10.f);
    apoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (apoly.f, dR2, __derf_ep___a10.f);
    bpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (bpoly.f, dR2, __derf_ep___b9.f);
    apoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (apoly.f, dR2, __derf_ep___a9.f);
    bpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (bpoly.f, dR2, __derf_ep___b8.f);
    apoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (apoly.f, dR2, __derf_ep___a8.f);
    bpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (bpoly.f, dR2, __derf_ep___b7.f);
    apoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (apoly.f, dR2, __derf_ep___a7.f);
    bpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (bpoly.f, dR2, __derf_ep___b6.f);
    apoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (apoly.f, dR2, __derf_ep___a6.f);
    bpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (bpoly.f, dR2, __derf_ep___b5.f);
    apoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (apoly.f, dR2, __derf_ep___a5.f);
    bpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (bpoly.f, dR2, __derf_ep___b4.f);
    apoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (apoly.f, dR2, __derf_ep___a4.f);
    bpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (bpoly.f, dR2, __derf_ep___b3.f);
    apoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (apoly.f, dR2, __derf_ep___a3.f);
    bpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (bpoly.f, dR2, __derf_ep___b2.f);
    apoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (apoly.f, dR2, __derf_ep___a2.f);
    bpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (bpoly.f, dR2, __derf_ep___b1.f);
    apoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (apoly.f, dR2, __derf_ep___a1.f);
    bpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (bpoly.f, dR2, __derf_ep___b0.f);
    apoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (apoly.f, dR2, __derf_ep___a0.f);
    fa = (float) apoly.f;
    fy = 1.0f / fa;
    Y.f = (double) fy;
    eps = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (-Y.f, apoly.f, 1.0);
    eps = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (eps, eps, eps);
    Y.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (Y.f, eps, Y.f);
    Y.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (Y.f, bpoly.f, 0.0);
    res.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (Y.f, dR, 0.0);
    res.w ^= sgn_x;
    *pres = res.f;
    return nRet;
}

double __ocl_svml_erf_ep_noLUT (double a)
{
    double va1;
    double vr1;
    double r;
    va1 = a;
    __internal_derf_ep_nolut_cout (&va1, &vr1);
    r = vr1;
    return r;
}

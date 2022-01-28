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
} __stanh_la_dnc4 = { 0x3eab52446f7c07feUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __stanh_la_dnc3 = { 0x3f35f1a23fd26ab7UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __stanh_la_dnc2 = { 0x3f9aa5668f92c8dfUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __stanh_la_dnc1 = { 0x3fddeaea531ea139UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __stanh_la_dnc0 = { 0x3ff000000341b1a6UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __stanh_la_ddc4 = { 0x3e4ea0ffff85bec4UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __stanh_la_ddc3 = { 0x3ef6473867555aacUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __stanh_la_ddc2 = { 0x3f6cf2bffba084deUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __stanh_la_ddc1 = { 0x3fc12b2ada806e60UL + (1UL << 26) };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __stanh_la_ddc0 = { 0x3ff0000000000000UL };

__attribute__((always_inline))
inline int __internal_stanh_nolut_cout (float *a, float *r)
{
    int nRet = 0;
    float xin = *a;
    union
    {
        unsigned long w;
        unsigned int w32[2];
        int s32[2];
        double f;
    } dpoly, npoly, R2;
    union
    {
        unsigned int w;
        float f;
    } x0, xa, fR2, fR3, y, q, diff, fdpoly, fnpoly;
    unsigned int sgn_x;
    x0.f = xin;
    xa.w = x0.w & 0x7fffffff;
    sgn_x = xa.w ^ x0.w;
    fR2.f = xa.f * xa.f;
    R2.f = fR2.f;
    npoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (R2.f, __stanh_la_dnc4.f, __stanh_la_dnc3.f);
    dpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (R2.f, __stanh_la_ddc4.f, __stanh_la_ddc3.f);
    npoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (npoly.f, R2.f, __stanh_la_dnc2.f);
    dpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dpoly.f, R2.f, __stanh_la_ddc2.f);
    npoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (npoly.f, R2.f, __stanh_la_dnc1.f);
    npoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (npoly.f, R2.f, __stanh_la_dnc0.f);
    dpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dpoly.f, R2.f, __stanh_la_ddc1.f);
    fnpoly.f = (float) npoly.f;
    fdpoly.f = (float) dpoly.f;
    fR3.f = fR2.f * xa.f;
    fdpoly.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (fdpoly.f, fR3.f, xa.f);
    y.f = 1.0f / (fnpoly.f);
    q.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (y.f, fdpoly.f, 0.0f);
    diff.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (q.f, (-fnpoly.f), fdpoly.f);
    y.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (y.f, diff.f, q.f);
    y.f = (xa.f > 8.015625f) ? 1.0f : y.f;
    y.w ^= sgn_x;
    *r = y.f;
    return nRet;
}

float __ocl_svml_tanhf_noLUT (float a)
{
    float va1;
    float vr1;
    float r;
    va1 = a;
    __internal_stanh_nolut_cout (&va1, &vr1);
    r = vr1;
    return r;
}

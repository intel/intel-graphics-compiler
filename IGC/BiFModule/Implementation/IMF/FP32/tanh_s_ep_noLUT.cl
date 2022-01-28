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
} __stanh_ep_nc2 = { 0x3c520a84u };

static __constant union
{
    unsigned int w;
    float f;
} __stanh_ep_nc1 = { 0x3edef102u };

static __constant union
{
    unsigned int w;
    float f;
} __stanh_ep_nc0 = { 0x3f800000u };

static __constant union
{
    unsigned int w;
    float f;
} __stanh_ep_dc2 = { 0x3a2fc8e6u };

static __constant union
{
    unsigned int w;
    float f;
} __stanh_ep_dc1 = { 0x3dd1c060u };

static __constant union
{
    unsigned int w;
    float f;
} __stanh_ep_dc0 = { 0xb859e195u };

__attribute__((always_inline))
inline int __internal_stanh_ep_nolut_cout (float *a, float *r)
{
    int nRet = 0;
    float xin = *a;
    union
    {
        unsigned int w;
        float f;
    } dpoly, npoly, R2;
    union
    {
        unsigned int w;
        float f;
    } x, x0, xa, y;
    unsigned int sgn_x;
    x0.f = xin;
    xa.w = x0.w & 0x7fffffff;
    sgn_x = xa.w ^ x0.w;
    R2.f = xa.f * xa.f;
    npoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (R2.f, __stanh_ep_nc2.f, __stanh_ep_nc1.f);
    dpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (R2.f, __stanh_ep_dc2.f, __stanh_ep_dc1.f);
    npoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (npoly.f, R2.f, __stanh_ep_nc0.f);
    dpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dpoly.f, R2.f, __stanh_ep_dc0.f);
    dpoly.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (dpoly.f, xa.f, xa.f);
    y.f = 1.0f / (npoly.f);
    y.f = y.f * dpoly.f;
    y.f = (xa.f >= 5.0f) ? 1.0f : y.f;
    y.w ^= sgn_x;
    *r = y.f;
    return nRet;
}

float __ocl_svml_tanhf_ep_noLUT (float a)
{
    float va1;
    float vr1;
    float r;
    va1 = a;
    __internal_stanh_ep_nolut_cout (&va1, &vr1);
    r = vr1;
    return r;
}

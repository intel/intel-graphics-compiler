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
} __satan_ep_c4 = { 0xbca5054fu };

static __constant union
{
    unsigned int w;
    float f;
} __satan_ep_c3 = { 0x3e49099du };

static __constant union
{
    unsigned int w;
    float f;
} __satan_ep_c2 = { 0xbecbaf63u };

static __constant union
{
    unsigned int w;
    float f;
} __satan_ep_c1 = { 0x3bef4e52u };

static __constant union
{
    unsigned int w;
    float f;
} __satan_ep_c0 = { 0x3f7ff759u };

__attribute__((always_inline))
inline int __internal_satan_ep_nolut_cout (float *pxin, float *pres)
{
    int nRet = 0;
    float xin = *pxin;
    {
        union
        {
            unsigned int w;
            float f;
        } x, xa, hcorr, ya, R0, sres;
        int sgn_x, smask, sgn_r, diff;
        float poly, R;
        x.f = xin;
        xa.w = x.w & 0x7fffffffu;
        sgn_x = x.w ^ xa.w;
        ya.f = 1.0f / (xa.f);
        diff = ya.w - xa.w;
        smask = ((int) diff) >> 31;
        hcorr.w = smask & 0xbfc90FDB;
        sgn_r = sgn_x ^ (smask & 0x80000000u);
        R0.w = xa.w + (diff & smask);
        R = R0.f;
        poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (__satan_ep_c4.f, R, __satan_ep_c3.f);
        poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly, R, __satan_ep_c2.f);
        poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly, R, __satan_ep_c1.f);
        poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly, R, __satan_ep_c0.f);
        sres.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly, R0.f, hcorr.f);
        sres.w = sres.w ^ sgn_r;
        *pres = sres.f;
    }
    return nRet;
}

float __ocl_svml_atanf_ep_noLUT (float a)
{
    float va1;
    float vr1;
    float r;
    va1 = a;
    __internal_satan_ep_nolut_cout (&va1, &vr1);
    r = vr1;
    return r;
}

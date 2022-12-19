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
#pragma float_control(precise,on)
static __constant union
{
    unsigned int w;
    float f;
} __stan_ha_invpi_s = { 0x3f22F983u };

static __constant union
{
    unsigned int w;
    float f;
} __stan_ha_two19 = { 0x48c00000u };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __stan_ha_Shifter = { 0x4338000000000000uL };

static __constant union
{
    unsigned int w;
    float f;
} __stan_ha_fShifter = { 0x4b400000u };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __stan_ha_NPI2_h = { 0xbff921fb54442d18uL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __stan_ha_NPI2_l = { 0xbc91a62633000000uL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __stan_ha_dc4 = { 0xbf30473bc9951ab8uL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __stan_ha_dc3 = { 0xbf612b43c92fe3c3uL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __stan_ha_dc2 = { 0xbf96c2a27abdad92uL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __stan_ha_dc1 = { 0xbfd55553d0e3f982uL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __stan_ha_dc0 = { 0x3feffffffd92bc1euL };

// coefficients for long path (merged paths are best avoided on Gen)
static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __stan_ha_c8 = { 0xbf7801a36251f374uL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __stan_ha_c7 = { 0x3d76d2c40e25ae0duL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __stan_ha_c6 = { 0xbf94860d8c43e703uL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __stan_ha_c5 = { 0xbd5905d5ff78631cuL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __stan_ha_c4 = { 0xbfb60db1fa2da8f0uL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __stan_ha_c3 = { 0x3d1d91347a4fb378uL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __stan_ha_c2 = { 0xbfe0c15107186d25uL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __stan_ha_c1 = { 0xbcc01a020cf548a2uL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __stan_ha_c0 = { 0x3fe45f306c3e4616uL };

// for each entry k (0..15), store
//  (2^(8*k)/(2*pi) mod 1)*2^(-8*k+2) in high-low double precision
static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __stan_ha_reduction_tbl[] = {
    {0x3fe45f306e000000uL}, {0xbdfb1bbead603d8buL},
    {0x3fe45f306dc9c883uL}, {0xbc86b01ec5417056uL},
    {0x3f87cc1b727220a9uL}, {0x3c23f84eafa3ea6auL},
    {0x3ef836e4e44152a0uL}, {0xbb8ec54170565912uL},
    {0x3e8b727220a94fe1uL}, {0x3b1d5f47d4d37703uL},
    {0x3e027220a94fe13buL}, {0xbaa05c1596447e49uL},
    {0x3d8220a94fe13abfuL}, {0xba2c1596447e493buL},
    {0x3cb529fc2757d1f5uL}, {0x394a6ee06db14acduL},
    {0x3c729fc2757d1f53uL}, {0x391377036d8a5665uL},
    {0x3bffc2757d1f534euL}, {0xb881f924eb53361euL},
    {0x3b43abe8fa9a6ee0uL}, {0x37eb6c52b3278872uL},
    {0x3b0abe8fa9a6ee07uL}, {0xb79275a99b0ef1bfuL},
    {0x3a8e8fa9a6ee06dbuL}, {0x3704acc9e21c8210uL},
    {0x39ff534ddc0db629uL}, {0x369664f10e41080euL},
    {0x39734ddc0db6295auL}, {0xb61b0ef1bef7fac6uL},
    {0x38ebb81b6c52b328uL}, {0xb58de37defff0eceuL},
    {0x387c0db6295993c4uL}, {0x350c820ff0d954bbuL},
};

// 1.0/(pi/2), for main path
static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __stan_ha_invpi_h = { 0x3fe45f306e000000uL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __stan_ha_invpi_l = { 0xbdfb1bbead603d8buL };

inline int __ocl_svml_internal_stan_ha_noLUT (float *a, float *pres)
{
    int nRet = 0;
    float xin = *a;
    union
    {
        unsigned int w;
        float f;
    } x0, x, fS, fN;
    unsigned int sgn_x, expon;
    int k;
    double xd, dN, dR, dRh, dRl, dR2;
    double dQ, dB, dRcp, eps;
    unsigned int R_sgn;
    float fRcp, fB;
    union
    {
        unsigned long w;
        unsigned int w32[2];
        int s32[2];
        double f;
    } S, poly, dA;
    union
    {
        unsigned int w;
        float f;
    } res;
    x0.f = xin;
    x.f = SPIRV_OCL_BUILTIN (fabs, _f32,) (xin);
    sgn_x = x0.w ^ x.w;
    // convert to DP
    xd = (double) x.f;
    if (x.f < __stan_ha_two19.f)
    {
        // main path: expon < 23
        // Shifter + (int)(x*2/pi)
        fS.f = SPIRV_OCL_BUILTIN (fma, _f32_f32_f32,) (x.f, __stan_ha_invpi_s.f, __stan_ha_fShifter.f);
        // (int)(x*2/pi)
        fN.f = fS.f - __stan_ha_fShifter.f;
        dN = (double) fN.f;
        // reduced argument
        dR = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (dN, __stan_ha_NPI2_h.f, xd);
        dR = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (dN, __stan_ha_NPI2_l.f, dR);
        R_sgn = fS.w << 31;
        dR2 = dR * dR;
        poly.f = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (dR2, (__stan_ha_dc4).f, (__stan_ha_dc3).f);
        poly.f = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (dR2, poly.f, (__stan_ha_dc2).f);
        poly.f = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (dR2, poly.f, (__stan_ha_dc1).f);
        poly.f = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (dR2, poly.f, (__stan_ha_dc0).f);
        dA.f = R_sgn ? poly.f : dR;
        dB = R_sgn ? dR : poly.f;
        fB = (float) dB;
        fRcp = 1.0f / fB;
        dRcp = (double) fRcp;
        // refine dRcp ~ 1/fB
        eps = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (dRcp, -dB, 1.0);
        dRcp = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (dRcp, eps, dRcp);
        dA.f = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (dA.f, dRcp, 0.0);
        res.f = (float) dA.f;
        res.w ^= (R_sgn ^ sgn_x);
    }
    else
    {
        // |x|=Inf or NaN?
        if (x.w >= 0x7f800000)
        {
            if (x.w == 0x7f800000)
            {
                res.w = 0xffc00000;
                nRet = 1;
            }
            else
                res.f = xin + xin;
            *pres = res.f;
            return nRet;
        }
        // large arguments
        // biased exponent
        expon = x.w >> 23;
        // table index
        k = (expon - (23 - 8 + 0x7f)) >> 3;
        k += k;
        //k += 2;
        dRh = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (xd, __stan_ha_reduction_tbl[k].f, 0.0);
        dRl = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (xd, __stan_ha_reduction_tbl[k].f, (-dRh));
        // low part of (int)(x*1/pi)
        S.f = __stan_ha_Shifter.f + dRh;
        dN = S.f - __stan_ha_Shifter.f;
        // reduced argument
        dR = dRh - dN;
        dR = dR + dRl;
        dR = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (xd, __stan_ha_reduction_tbl[k + 1].f, dR);
        R_sgn = (S.w32[0]) << 31;
        poly.f = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (dR, __stan_ha_c8.f, __stan_ha_c7.f);
        poly.f = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (dR, poly.f, __stan_ha_c6.f);
        poly.f = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (dR, poly.f, __stan_ha_c5.f);
        poly.f = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (dR, poly.f, __stan_ha_c4.f);
        poly.f = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (dR, poly.f, __stan_ha_c3.f);
        poly.f = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (dR, poly.f, __stan_ha_c2.f);
        poly.f = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (dR, poly.f, __stan_ha_c1.f);
        poly.f = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (dR, poly.f, __stan_ha_c0.f);
        dA.f = R_sgn ? poly.f : dR;
        dB = R_sgn ? dR : poly.f;
        fB = (float) dB;
        fRcp = 1.0f / fB;
        dRcp = (double) fRcp;
        dQ = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (dA.f, dRcp, 0.0);
        // refine quotient
        eps = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (dQ, -dB, dA.f);
        res.f = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (dRcp, eps, dQ);
        res.w ^= (R_sgn ^ sgn_x);
    }
    *pres = res.f;
    return nRet;
}

float __ocl_svml_tanf_ha_noLUT (float a)
{
    float r;
    __ocl_svml_internal_stan_ha_noLUT (&a, &r);
    return r;
}

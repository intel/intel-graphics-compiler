/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF

#pragma float_control(precise,on)
static __constant int_double __ssincos_la_dp_reduction_tbl[] = {
    {0x3fe45f306e000000UL}, {0xbdfb1bbead603d8bUL},
    {0x3fe45f306dc9c883UL}, {0xbc86b01ec5417056UL},
    {0x3f87cc1b727220a9UL}, {0x3c23f84eafa3ea6aUL},
    {0x3ef836e4e44152a0UL}, {0xbb8ec54170565912UL},
    {0x3e8b727220a94fe1UL}, {0x3b1d5f47d4d37703UL},
    {0x3e027220a94fe13bUL}, {0xbaa05c1596447e49UL},
    {0x3d8220a94fe13abfUL}, {0xba2c1596447e493bUL},
    {0x3cb529fc2757d1f5UL}, {0x394a6ee06db14acdUL},
    {0x3c729fc2757d1f53UL}, {0x391377036d8a5665UL},
    {0x3bffc2757d1f534eUL}, {0xb881f924eb53361eUL},
    {0x3b43abe8fa9a6ee0UL}, {0x37eb6c52b3278872UL},
    {0x3b0abe8fa9a6ee07UL}, {0xb79275a99b0ef1bfUL},
    {0x3a8e8fa9a6ee06dbUL}, {0x3704acc9e21c8210UL},
    {0x39ff534ddc0db629UL}, {0x369664f10e41080eUL},
    {0x39734ddc0db6295aUL}, {0xb61b0ef1bef7fac6UL},
    {0x38ebb81b6c52b328UL}, {0xb58de37defff0eceUL},
    {0x387c0db6295993c4UL}, {0x350c820ff0d954bbUL},
};

static __constant int_double __ssincos_la_dp_invpi_h = { 0x3fe45f306e000000UL };
static __constant int_double __ssincos_la_dp_invpi_l = { 0xbdfb1bbead603d8bUL };

static __constant unsigned int __ssincos_la_dp_AbsMask = 0x7fffffffu;
static __constant int_double __ssincos_la_dp_Shifter = { 0x4338000000000000UL };

static __constant int_double __ssincos_la_dp_s_coeff[] = {
    {0x3fe243f6a5f0c8e5UL}, {0xbfe4abbb99e5a6dbUL},
    {0x3fb465ec65afe9deUL}, {0xbf72d9bb7b168b2bUL},
};

static __constant int_double __ssincos_la_dp_c_coeff[] = {
    {0x3feFFFFFFFF97C47UL}, {0xbff3BD3CC7323529UL},
    {0x3fd03C1DC1BAFA81UL}, {0xbf955C57B06D183CUL},
    {0x3f4D9C364E89E2CCUL}
};

static __constant int_float __ssincos_la_dp_invpi_s = { 0x3f22F983u };
static __constant int_float __ssincos_la_dp_two19 = { 0x49000000u };
static __constant int_float __ssincos_la_dp_fShifter = { 0x4b400000u };

static __constant int_float __ssincos_la_dp_Pi2_h = { 0xbfc90fdbu };
static __constant int_float __ssincos_la_dp_Pi2_m = { 0x333bbd2eu };
static __constant int_float __ssincos_la_dp_Pi2_l = { 0x26f72cedu };

static __constant int_float __ssincos_la_dp_fc3 = { 0xbab212bfu };
static __constant int_float __ssincos_la_dp_fc2 = { 0x3d2a9e76u };
static __constant int_float __ssincos_la_dp_fc1 = { 0xbeffffd0u };
static __constant int_float __ssincos_la_dp_fc0 = { 0x3f800000u };

static __constant int_float __ssincos_la_dp_fs3 = { 0xb94c83b6u };
static __constant int_float __ssincos_la_dp_fs2 = { 0x3c088318u };
static __constant int_float __ssincos_la_dp_fs1 = { 0xbe2aaaa0u };
static __constant int_float __ssincos_la_dp_fs0 = { 0x3f800000u };

__attribute__((always_inline))
inline int __internal_ssincos_cout_dp (float *a, float *psin, float *pcos)
{
    int nRet = 0;
    float xin = *a;
    unsigned int sgn_x, expon;
    int k, index, c_sgn;
    double xd, dN, dR, dRh, dRl, dR2;
    unsigned int R_sgn;
    int_double S, s_poly, c_poly;
    int_float fres0, fres1, sin_res, cos_res, x0, x, fS, fN;
    float fR, fR2, fspoly, fcpoly;

    x0.f = xin;
    x.w = x0.w & __ssincos_la_dp_AbsMask;
    sgn_x = x0.w ^ x.w;

    if (x.f < __ssincos_la_dp_two19.f)
    {

        fS.f = __spirv_ocl_fma(x.f,__ssincos_la_dp_invpi_s.f,__ssincos_la_dp_fShifter.f);

        fN.f = fS.f - __ssincos_la_dp_fShifter.f;

        fR = __spirv_ocl_fma(fN.f,__ssincos_la_dp_Pi2_h.f,x.f);
        fR = __spirv_ocl_fma(fN.f,__ssincos_la_dp_Pi2_m.f,fR);
        fR = __spirv_ocl_fma(fN.f,__ssincos_la_dp_Pi2_l.f,fR);

        R_sgn = (fS.w >> 1) << 31;
        index = fS.w << 31;

        fR2 = fR * fR;

        fspoly = __spirv_ocl_fma(fR2,__ssincos_la_dp_fs3.f,__ssincos_la_dp_fs2.f);
        fcpoly = __spirv_ocl_fma(fR2,__ssincos_la_dp_fc3.f,__ssincos_la_dp_fc2.f);
        fspoly = __spirv_ocl_fma(fspoly,fR2,__ssincos_la_dp_fs1.f);
        fcpoly = __spirv_ocl_fma(fcpoly,fR2,__ssincos_la_dp_fc1.f);
        fspoly = __spirv_ocl_fma(fspoly,fR2,0.0f);
        fcpoly = __spirv_ocl_fma(fcpoly,fR2,__ssincos_la_dp_fc0.f);

        fspoly = __spirv_ocl_fma(fspoly,fR,fR);

        sgn_x ^= R_sgn;
        c_sgn = index ^ R_sgn;

        sin_res.f = (index == 0) ? fspoly : fcpoly;
        cos_res.f = (index == 0) ? fcpoly : fspoly;
        sin_res.w ^= sgn_x;
        cos_res.w ^= c_sgn;

        *pcos = cos_res.f;
        *psin = sin_res.f;

    }
    else
    {
        if (x.w == 0x7f800000)
        {
            sin_res.w = 0xffc00000;
            nRet = 1;
            *psin = *pcos = sin_res.f;
            return nRet;
        }

        xd = (double) x.f;

        expon = x.w >> 23;

        k = (expon - (23 - 8 + 0x7f)) >> 3;
        k += k;
        dRh = __spirv_ocl_fma(xd,(__ssincos_la_dp_reduction_tbl[k]).f,0.0);
        dRl = __spirv_ocl_fma(xd,(__ssincos_la_dp_reduction_tbl[k]).f,-dRh);

        S.f = (__ssincos_la_dp_Shifter).f + dRh;
        dN = S.f - (__ssincos_la_dp_Shifter).f;

        dR = dRh - dN;
        dR = dR + dRl;
        dR = __spirv_ocl_fma(xd,(__ssincos_la_dp_reduction_tbl[k+1]).f,dR);

        R_sgn = (S.w32[0] >> 1);
        index = S.w32[0];

        dR2 = dR * dR;
        R_sgn <<= 31;
        index <<= 31;
        c_sgn = index;

        sgn_x ^= R_sgn;
        c_sgn ^= R_sgn;

        c_poly.f = __spirv_ocl_fma(dR2,(__ssincos_la_dp_c_coeff[4]).f,(__ssincos_la_dp_c_coeff[3]).f);
        s_poly.f = __spirv_ocl_fma(dR2,(__ssincos_la_dp_s_coeff[3]).f,(__ssincos_la_dp_s_coeff[2]).f);
        c_poly.f = __spirv_ocl_fma(dR2,c_poly.f,(__ssincos_la_dp_c_coeff[2]).f);
        s_poly.f = __spirv_ocl_fma(dR2,s_poly.f,(__ssincos_la_dp_s_coeff[1]).f);
        c_poly.f = __spirv_ocl_fma(dR2,c_poly.f,(__ssincos_la_dp_c_coeff[1]).f);
        s_poly.f = __spirv_ocl_fma(dR2,s_poly.f,(__ssincos_la_dp_s_coeff[0]).f);
        c_poly.f = __spirv_ocl_fma(dR2,c_poly.f,(__ssincos_la_dp_c_coeff[0]).f);
        s_poly.f = __spirv_ocl_fma(dR,s_poly.f,dR);
        fres0.f = (float) s_poly.f;
        fres1.f = (float) c_poly.f;
        sin_res.w = (index == 0) ? fres0.w : fres1.w;
        cos_res.w = (index == 0) ? fres1.w : fres0.w;
        sin_res.w ^= sgn_x;
        cos_res.w ^= c_sgn;

        *pcos = cos_res.f;
        *psin = sin_res.f;
    }

    return nRet;
}

void __ocl_svml_sincosf_dp (float a,  __private float *b, __private float *c)
{

    float va1;
    float vr1;
    float vr2;

    va1 = a;

    __internal_ssincos_cout_dp (&va1, &vr1, &vr2);
    ((float *) b)[0] = vr1;
    ((float *) c)[0] = vr2;

    return;

}

/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF
typedef struct
{
    unsigned int SgnBit;
    unsigned int OneHalf;
    unsigned int sRsqrtMsk;
    unsigned int SmallNorm;
    unsigned int MOne;
    unsigned int HalfMask;
    unsigned int SQMask;
    unsigned int Two;
    unsigned int sqrt_coeff[2];
    unsigned int poly_coeff[5];
    unsigned int Pi2H;
    unsigned int Pi2L;
    unsigned int PiH;
    unsigned int PiL;
    unsigned int Zero;
    unsigned int SgnMask;
    unsigned int NanMask;
    unsigned int ep_coeff[3];
} __internal_sacos_la_data_t;
static __constant __internal_sacos_la_data_t __internal_sacos_la_data = {

    0x80000000u, 0x3f000000u, 0xfffff000u, 0x2f800000u, 0xbf800000u, 0xffffe000u, 0xfffff800u, 0x40000000u, {
                                                                                                             0xbdC00004u, 0x3e800001u}

    , {
       0x3d2EDC07u, 0x3CC32A6Bu, 0x3d3A9AB4u, 0x3d997C12u, 0x3e2AAAFFu}

    , 0x3fc90FDBu, 0xB33BBD2Eu, 0x40490FDBu, 0xB3BBBD2Eu, 0x00000000u, 0x80000000u, 0xffc00000u, {
                                                                                                  0x3dC4C6AEu, 0x3e2876B2u, 0x380561A3u}

};
static __constant int_float __sacos_la_c5 = { 0x3d2edc07u };
static __constant int_float __sacos_la_c4 = { 0x3cc32a6bu };
static __constant int_float __sacos_la_c3 = { 0x3d3a9ab4u };
static __constant int_float __sacos_la_c2 = { 0x3d997c12u };
static __constant int_float __sacos_la_c1 = { 0x3e2aaaffu };
static __constant int_float __sacos_la_c0 = { 0x3f800000u };
static __constant int_float __sacos_la_small_float = { 0x01800000u };

static __constant int_float __sacos_la_two = { 0x40000000u };

static __constant int_float __sacos_la_pi2h = { 0x3FC90FDBu };
static __constant int_float __sacos_la_pi2l = { 0xB33BBD2Eu };

static __constant int_float __sacos_la_pih = { 0x40490FDBu };
static __constant int_float __sacos_la_pil = { 0xB3BBBD2Eu };

__attribute__((always_inline))
inline int __internal_sacos_la_cout (float *pxin, float *pres)
{
    int nRet = 0;
    float xin = *pxin;
    int_float y, res;
    {
        int_float x, xa, RS, Shh2, High, R0;
        int sgn_x;
        float R, poly, Sh, Shh;

        x.f = xin;

        xa.w = x.w & 0x7fffffff;

        sgn_x = x.w ^ xa.w;

        y.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (-(xa.f), 0.5f, 0.5f);

        R = xin * xin;
        R = SPIRV_OCL_BUILTIN(fmin, _f32_f32, ) (R, y.f);

        High.f = sgn_x ? __sacos_la_pih.f : 0.0f;
        High.f = (xa.f <= 0.5f) ? __sacos_la_pi2h.f : High.f;

        RS.f = 1.0f / SPIRV_OCL_BUILTIN(sqrt, _f32, ) ((y.f + __sacos_la_small_float.f));

        RS.w |= sgn_x;

        Sh = (y.f * RS.f);

        Shh2.f = -2.0f * Sh;

        poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (__sacos_la_c5.f, R, __sacos_la_c4.f);
        poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly, R, __sacos_la_c3.f);
        poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly, R, __sacos_la_c2.f);
        poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly, R, __sacos_la_c1.f);
        poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly, R, __sacos_la_c0.f);

        R0.f = (xa.f <= 0.5f) ? x.f : Shh2.f;

        res.f = (SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (-(poly), R0.f, High.f));
    }

    *pres = res.f;
    nRet = (y.f >= 0.0f) ? 0 : 1;

    return nRet;
}

float __ocl_svml_acosf (float a)
{

    float va1;
    float vr1;
    unsigned int vm;

    float r;

    va1 = a;;

    __internal_sacos_la_cout (&va1, &vr1);
    r = vr1;;

    return r;

}

/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF
typedef struct
{
    unsigned int sZERO;
    unsigned int sONE;
    unsigned int sTWO;
    unsigned int sSIGN_MASK;
    unsigned int sABS_MASK;
    unsigned int sPIO2;
    unsigned int sPI;

    unsigned int sPC8;
    unsigned int sPC7;
    unsigned int sPC6;
    unsigned int sPC5;
    unsigned int sPC4;

    unsigned int sPC3;
    unsigned int sPC2;
    unsigned int sPC1;
    unsigned int sPC0;
    unsigned int iCHK_WORK_SUB;
    unsigned int iCHK_WORK_CMP;

} __internal_satan2_la_data_t;

static __constant __internal_satan2_la_data_t __internal_satan2_la_data = {
    0x00000000u,
    0x3f800000u,
    0x40000000u,
    0x80000000u,
    0x7FFFFFFFu,
    0x3FC90FDBu,
    0x40490FDBu,

    0x3B322CC0u,
    0xBC7F2631u,
    0x3D2BC384u,
    0xBD987629u,
    0x3DD96474u,
    0xBE1161F8u,
    0x3E4CB79Fu,
    0xBEAAAA49u,
    0x3F800000u,

    0x81000000u,
    0xFC000000u,

};
static __constant int_float __satan2_la_c9 = { 0x3c909a7bu };
static __constant int_float __satan2_la_c8 = { 0xbd093579u };
static __constant int_float __satan2_la_c7 = { 0xbbacabd2u };
static __constant int_float __satan2_la_c6 = { 0x3d8c9f7bu };
static __constant int_float __satan2_la_c5 = { 0xbd85ac4eu };
static __constant int_float __satan2_la_c4 = { 0xbd39a551u };
static __constant int_float __satan2_la_c3 = { 0x3e348efbu };
static __constant int_float __satan2_la_c2 = { 0xbe05f628u };
static __constant int_float __satan2_la_c1 = { 0xbe8259b8u };
static __constant int_float __satan2_la_c0 = { 0xbd94e63du };

static __constant int_float __satan2_la_two32 = { 0x4f800000u };

static __constant int_float __satan2_la_zero = { 0x00000000u };

__attribute__((always_inline))
inline int __internal_satan2_la_cout (float *pyin, float *pxin, float *pres)
{
    int nRet = 0;
    float xin = *pxin, yin = *pyin;
    {
        int_float y, x, ya, xa, hcorr, fx, fy, hcorr2, sres, Q00;
        unsigned sgn_x, sgn_y, sgn_r, sgn_c;
        int sgnx_mask, smask;
        float frcp_x, R, poly;

        y.f = yin;
        x.f = xin;

        xa.w = x.w & 0x7fffffff;
        ya.w = y.w & 0x7fffffff;

        sgn_x = x.w ^ xa.w;
        sgn_y = y.w ^ ya.w;

        sgnx_mask = ((int) sgn_x) >> 31;
        hcorr.w = sgnx_mask & 0x40490FDB;

        fy.w = (((xa.w) < (ya.w)) ? (xa.w) : (ya.w));
        fx.w = (((xa.w) >= (ya.w)) ? (xa.w) : (ya.w));

        smask = ((int) (xa.w - ya.w)) >> 31;
        hcorr2.w = smask & 0x3fc90FDB;
        hcorr.f = hcorr2.f - hcorr.f;
        sgn_c = (smask & 0x80000000);
        hcorr.w ^= sgn_c;

        sgn_r = sgn_c ^ (sgn_x ^ sgn_y);

        if (((unsigned) (fx.w - 0x00800000) > 0x7e000000) || (fy.f == __satan2_la_zero.f))
            goto SPECIAL_ATAN2F;

      ATAN2F_MAIN:

        frcp_x = 1.0f / (fx.f);

        Q00.f = (fy.f * frcp_x);

        R = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (fy.f, frcp_x, -0.5f);

        poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (__satan2_la_c9.f, R, __satan2_la_c8.f);
        poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly, R, __satan2_la_c7.f);
        poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly, R, __satan2_la_c6.f);
        poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly, R, __satan2_la_c5.f);
        poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly, R, __satan2_la_c4.f);
        poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly, R, __satan2_la_c3.f);
        poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly, R, __satan2_la_c2.f);
        poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly, R, __satan2_la_c1.f);
        poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly, R, __satan2_la_c0.f);

        sres.f = (SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly, Q00.f, Q00.f));
        sres.f += hcorr.f;
        sres.w = sres.w ^ sgn_r;
        *pres = sres.f;
        return nRet;

      SPECIAL_ATAN2F:

        if (fx.w > 0x7f800000u)
        {
            if (xa.w > 0x7f800000u)
                sres.w = x.w | 0x00400000u;
            else
                sres.w = y.w | 0x00400000u;
            *pres = sres.f;
            return nRet;
        }

        if (fy.f == __satan2_la_zero.f)
        {
            sres.w = sgn_y ^ (hcorr.w & 0x7fffffff);
            *pres = sres.f;
            return nRet;
        }

        if (fx.w == 0x7f800000)
        {
            if (fy.w < 0x7f800000u)
            {
                if (ya.w == 0x7f800000u)
                    sres.w = sgn_y ^ 0x3fc90FDB;
                else
                    sres.w = sgn_r ^ sgn_c ^ hcorr.w;
                *pres = sres.f;
                return nRet;
            }

            if (x.w == 0xff800000u)
                sres.w = 0x4016CBE4;
            else
                sres.w = 0x3f490FDB;
            sres.w ^= sgn_y;
            *pres = sres.f;
            return nRet;
        }

        if (fx.w < 0x00800000u)
        {

            fx.f *= __satan2_la_two32.f;
            fy.f *= __satan2_la_two32.f;
        }
        else if (fx.w > 0x7e800000u)
        {

            fx.f *= 0.25f;
            fy.f *= 0.25f;
        }

        goto ATAN2F_MAIN;

    }

    return nRet;

}

float __ocl_svml_atan2f (float a, float b)
{

    float va1;
    float va2;
    float vr1;
    unsigned int vm;

    float r;

    va1 = a;
    va2 = b;

    __internal_satan2_la_cout (&va1, &va2, &vr1);
    r = vr1;;

    return r;

}

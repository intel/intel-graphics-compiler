/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF
typedef struct
{
    unsigned int AbsMask;
    unsigned int Shifter;
    unsigned int MaxThreshold;
    unsigned int MOne;
    unsigned int One;
    unsigned int LargeX;
    unsigned int Zero;
    unsigned int Tbl_H[32];
    unsigned int Tbl_L[32];
    unsigned int Pi2;
    unsigned int Pi2_low;
    unsigned int coeff[3];
} __internal_satan_la_data_avx512_t;
static __constant __internal_satan_la_data_avx512_t __internal_satan_la_data_avx512 = {

    0x7fffffffu, 0x4a000000u, 0x40F80000u, 0xbf800000u, 0x3f800000u, 0x4f800000u, 0x00000000u, {
                                                                                                0x00000000u, 0x3e7adbb0u,
                                                                                                0x3eed6338u, 0x3f24bc7du,
                                                                                                0x3f490fdbu, 0x3f6563e3u,
                                                                                                0x3f7b985fu, 0x3f869c79u,
                                                                                                0x3f8db70du, 0x3f93877bu,
                                                                                                0x3f985b6cu, 0x3f9c6b53u,
                                                                                                0x3f9fe0bbu, 0x3fa2daa4u,
                                                                                                0x3fa57088u, 0x3fa7b46fu,
                                                                                                0x3fa9b465u, 0x3fab7b7au,
                                                                                                0x3fad1283u, 0x3fae809eu,
                                                                                                0x3fafcb99u, 0x3fb0f836u,
                                                                                                0x3fb20a6au, 0x3fb30581u,
                                                                                                0x3fb3ec43u, 0x3fb4c10au,
                                                                                                0x3fb585d7u, 0x3fb63c64u,
                                                                                                0x3fb6e62cu, 0x3fb78478u,
                                                                                                0x3fb81868u, 0x3fb8a2f5u,
                                                                                                }
    , {
       0x00000000u, 0xb15a6fe4u,
       0x31ac376au, 0x31c9a7b8u,
       0xb2bbbd2eu, 0xb287b906u,
       0xb2d7e096u, 0x3345ba0au,
       0xb351441cu, 0xb325ac5eu,
       0xb2d2b64bu, 0x334e1335u,
       0x3337a856u, 0x325cd468u,
       0xb2669d97u, 0x33267261u,
       0xb32e1630u, 0xb345c196u,
       0xb35eeb1au, 0x32835b58u,
       0xb32dab7bu, 0x32d52571u,
       0x3281298fu, 0x334736a0u,
       0x326f266fu, 0xb2ac55f6u,
       0x33030c07u, 0xb190a736u,
       0xb2895340u, 0x32e86bfeu,
       0xb2d7f9cdu, 0x3342088au,
       }

    , 0x3fc90FDBu, 0xB33BBD2Eu, {

                                 0xbe0fa8deu, 0x3e4cc8e2u, 0xbeaaaaaau}
};

typedef struct
{
    unsigned int _sSIGN_MASK;
    unsigned int _sABS_MASK;
    unsigned int _sONE;
    unsigned int _sTWO;
    unsigned int _sPIO2;
    unsigned int _sRangeVal;

    unsigned int _sPC8;
    unsigned int _sPC7;
    unsigned int _sPC6;
    unsigned int _sPC5;
    unsigned int _sPC4;

    unsigned int _sPC3;
    unsigned int _sPC2;
    unsigned int _sPC1;
    unsigned int _sPC0;

} __internal_satan_la_data_t;
static __constant __internal_satan_la_data_t __internal_satan_la_data = {
    0x80000000u,
    0x7FFFFFFFu,
    0x3f800000u,
    0x40000000u,
    0x3FC90FDBu,
    0x7f800000u,

    0x3B322CC0u,
    0xBC7F2631u,
    0x3D2BC384u,
    0xBD987629u,
    0x3DD96474u,
    0xBE1161F8u,
    0x3E4CB79Fu,
    0xBEAAAA49u,
    0x3f800000u,
};
static __constant int_float __satan_la_c7 = { 0xbb9ca55fu };
static __constant int_float __satan_la_c6 = { 0x3cc92c07u };
static __constant int_float __satan_la_c5 = { 0xbd755ea6u };
static __constant int_float __satan_la_c4 = { 0x3dcba0b1u };
static __constant int_float __satan_la_c3 = { 0xbe0fa948u };
static __constant int_float __satan_la_c2 = { 0x3e4c81c3u };
static __constant int_float __satan_la_c1 = { 0xbeaaa90bu };
static __constant int_float __satan_la_c0 = { 0x3f7fffffu };

__attribute__((always_inline))
inline int __internal_satan_la_cout (float *pxin, float *pres)
{
    int nRet = 0;
    float xin = *pxin;
    {
        int_float x, xa, hcorr, ya, R0, sres;
        int sgn_x, smask, diff;
        float poly, R;

        x.f = xin;
        xa.w = x.w & 0x7fffffffu;
        sgn_x = x.w ^ xa.w;

        ya.f = 1.0f / (xa.f);
        ya.w ^= 0x80000000u;

        diff = xa.w - ya.w;
        smask = ((int) diff) >> 31;

        hcorr.w = smask & 0x3fc90FDB;

        R0.w = xa.w - (diff & smask);
        R = R0.f * R0.f;

        poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (__satan_la_c7.f, R, __satan_la_c6.f);
        poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly, R, __satan_la_c5.f);
        poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly, R, __satan_la_c4.f);
        poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly, R, __satan_la_c3.f);
        poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly, R, __satan_la_c2.f);
        poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly, R, __satan_la_c1.f);
        poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly, R, __satan_la_c0.f);

        sres.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly, R0.f, hcorr.f);

        sres.w = sres.w ^ sgn_x;
        *pres = sres.f;

    }
    return nRet;

}

float __ocl_svml_atanf (float a)
{

    float va1;
    float vr1;
    unsigned int vm;

    float r;

    va1 = a;;

    __internal_satan_la_cout (&va1, &vr1);
    r = vr1;;

    return r;

}

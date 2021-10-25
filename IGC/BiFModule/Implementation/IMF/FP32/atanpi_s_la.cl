/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF
typedef struct
{
    unsigned int S_ONE;
    unsigned int S_NONE;
    unsigned int S_ABS_MASK;
    unsigned int S_MIN_NUM;
    unsigned int S_MAX_NUM;
    unsigned int S_ZERO;
    unsigned int S_TWO;
    unsigned int S_SIGN_MASK;
    unsigned int S_PIO2;
    unsigned int S_PI;
    unsigned int AT_S_A08;
    unsigned int AT_S_A07;
    unsigned int AT_S_A06;
    unsigned int AT_S_A05;
    unsigned int AT_S_A04;
    unsigned int AT_S_A03;
    unsigned int AT_S_A02;
    unsigned int AT_S_A01;
    unsigned int AT_S_A00;
    unsigned int I_CHK_WORK_SUB;
    unsigned int I_CHK_WORK_CMP;
} __internal_satanpi_la_data_t;
static __constant __internal_satanpi_la_data_t __internal_satanpi_la_data = {
    0x3f800000u,
    0xbf800000u,
    0x7FFFFFFFu,
    0x01000000u,
    0x7d000000u,
    0x00000000u,
    0x40000000u,
    0x80000000u,
    0x3F000000u,
    0x3F800000u,
    0x3A62DBF5u,
    0xBBA26EDAu,
    0x3C5AB246u,
    0xBCC21EBEu,
    0x3D0A6574u,
    0xBD391B64u,
    0x3D8253BAu,
    0xBDD94C33u,
    0x3EA2F983u,
    0x81000000u,
    0xFC000000u,
};
static __constant int_float __satanpi_la_c7 = { 0xbb9ca55fu };
static __constant int_float __satanpi_la_c6 = { 0x3cc92c07u };
static __constant int_float __satanpi_la_c5 = { 0xbd755ea6u };
static __constant int_float __satanpi_la_c4 = { 0x3dcba0b1u };
static __constant int_float __satanpi_la_c3 = { 0xbe0fa948u };
static __constant int_float __satanpi_la_c2 = { 0x3e4c81c3u };
static __constant int_float __satanpi_la_c1 = { 0xbeaaa90bu };
static __constant int_float __satanpi_la_c0 = { 0x3f7fffffu };

__attribute__((always_inline))
inline int __internal_satanpi_la_cout (float *pxin, float *pres)
{
    int nRet = 0;
    float xin = *pxin;
    int_float invpi = { 0x3ea2f983 };
    {
        int_float x, xa, hcorr, ya, R0, sres;
        int sgn_x, smask, diff;
        float poly, R;
        int_float invpi = { 0x3ea2f983 };

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

        poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (__satanpi_la_c7.f, R, __satanpi_la_c6.f);
        poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly, R, __satanpi_la_c5.f);
        poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly, R, __satanpi_la_c4.f);
        poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly, R, __satanpi_la_c3.f);
        poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly, R, __satanpi_la_c2.f);
        poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly, R, __satanpi_la_c1.f);
        poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly, R, __satanpi_la_c0.f);

        sres.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly, R0.f, hcorr.f);

        sres.w = sres.w ^ sgn_x;
        *pres = invpi.f * sres.f;

    }
    return nRet;
}

float __ocl_svml_atanpif (float a)
{

    float va1;
    float vr1;
    unsigned int vm;

    float r;

    va1 = a;;

    __internal_satanpi_la_cout (&va1, &vr1);
    r = vr1;;

    return r;

}

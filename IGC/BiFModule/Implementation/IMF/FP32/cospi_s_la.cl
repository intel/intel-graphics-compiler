/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF
typedef struct
{
    unsigned int _sAbsMask;
    unsigned int _sHalf;
    unsigned int _sSignBit;
    unsigned int _sC1_h;
    unsigned int _sC2;
    unsigned int _sC3;
    unsigned int _sC4;
    unsigned int _sC5;
} __internal_scospi_la_data_avx512_t;
static __constant __internal_scospi_la_data_avx512_t __internal_scospi_la_data_avx512 = {
    0x7FFFFFFFu,
    0x3f000000u,
    0x80000000u,
    0xc0490fdbu,

    0x40a55decu,
    0xc023358fu,
    0x3f193f25u,
    0xbda026a1u,

};

typedef struct
{
    unsigned int _sAbsMask;
    unsigned int _sSignBit;
    unsigned int _sOneHalf;
    unsigned int _sReductionRangeVal;
    unsigned int _sRangeVal;
    unsigned int _sPiToRad;

    unsigned int _sA3;
    unsigned int _sA5;
    unsigned int _sA7;
    unsigned int _sA9;
    unsigned int _sA5_FMA;
    unsigned int _sA7_FMA;
    unsigned int _sA9_FMA;

    unsigned int _sRShifter;

} __internal_scospi_la_data_t;
static __constant __internal_scospi_la_data_t __internal_scospi_la_data = {
    0x7fffffffu,
    0x80000000u,
    0x3F000000u,
    0x4A800000u,
    0x7f800000u,
    0x40490FDBu,

    0xBE2AAAA6u,
    0x3c088769u,
    0xb94fb7ebu,
    0x362ede96u,
    0x3c088767u,
    0xb94fb6c5u,
    0x362ec33bu,

    0x4B400000u,
};
static __constant int_float __scospi_la_c4 = { 0x3d9f0be5u };
static __constant int_float __scospi_la_c3 = { 0xbf1929adu };
static __constant int_float __scospi_la_c2 = { 0x40233479u };
static __constant int_float __scospi_la_c1 = { 0xc0a55de2u };
static __constant int_float __scospi_la_c0 = { 0x40490fdau };
static __constant int_float __scospi_la_max_norm = { 0x7f7fffffu };

__attribute__((always_inline))
inline int __internal_scospi_la_cout (float *a, float *pres)
{
    int nRet = 0;
    float x = *a;
    float fN, fNi, R, R2, poly;
    int iN, sgn;
    int_float res;

    fN = SPIRV_OCL_BUILTIN(rint, _f32, ) (x);

    fNi = -SPIRV_OCL_BUILTIN(fabs, _f32, ) (fN);
    iN = (int) fNi;

    sgn = iN << 31;

    R = x - fN;
    R = 0.5f - SPIRV_OCL_BUILTIN(fabs, _f32, ) (R);
    R2 = R * R;

    poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (R2, __scospi_la_c4.f, __scospi_la_c3.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly, R2, __scospi_la_c2.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly, R2, __scospi_la_c1.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly, R2, __scospi_la_c0.f);

    res.f = R * poly;

    res.w ^= sgn;
    *pres = res.f;
    nRet = (SPIRV_OCL_BUILTIN(fabs, _f32, ) (x) > __scospi_la_max_norm.f) ? 1 : 0;
    return nRet;

}

float __ocl_svml_cospif (float a)
{

    float va1;
    float vr1;
    unsigned int vm;

    float r;

    va1 = a;;

    __internal_scospi_la_cout (&va1, &vr1);
    r = vr1;;

    return r;

}

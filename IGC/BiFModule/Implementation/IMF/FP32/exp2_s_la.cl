/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF
typedef struct
{
    unsigned int Frac_PowerS0[16];
    unsigned int poly_coeff1;
    unsigned int poly_coeff2;

    unsigned int poly_coeff3;

    unsigned int add_const;
    unsigned int AbsMask;
    unsigned int Threshold;
} __internal_sexp2_la_data_avx512_t;
static __constant __internal_sexp2_la_data_avx512_t __internal_sexp2_la_data_avx512 = {
    {
     0x3F800000u,
     0x3F85AAC3u,
     0x3F8B95C2u,
     0x3F91C3D3u,
     0x3F9837F0u,
     0x3F9EF532u,
     0x3FA5FED7u,
     0x3FAD583Fu,
     0x3FB504F3u,
     0x3FBD08A4u,
     0x3FC5672Au,
     0x3FCE248Cu,
     0x3FD744FDu,
     0x3FE0CCDFu,
     0x3FEAC0C7u,
     0x3FF5257Du,
     },

    0x3F317222u,
    0x3E75F16Bu,
    0x3D6854CAu,

    0x49400000u,
    0x7fffffffu,
    0x42fc0000u,
};

typedef struct
{
    unsigned int _sShifter;
    unsigned int _sPC0;
    unsigned int _sPC1;
    unsigned int _sPC2;
    unsigned int _sPC3;
    unsigned int _sPC4;
    unsigned int _sPC5;
    unsigned int _sPC6;

    unsigned int _iAbsMask;
    unsigned int _iDomainRange;
} __internal_sexp2_la_data_t;
static __constant __internal_sexp2_la_data_t __internal_sexp2_la_data = {
    0x4b400000u,

    0x3F800000u,
    0x3f317218u,
    0x3e75fdefu,
    0x3d6357cfu,
    0x3c1d962cu,
    0x3aaf7a51u,
    0x39213c8cu,
    0x7fffffffu,
    0x42fc0000u,
};

static __constant union
{
    unsigned int w;
    float f;
} __sexp2_la_c6 = { 0x39224c80u };

static __constant union
{
    unsigned int w;
    float f;
} __sexp2_la_c5 = { 0x3aafa463u };

static __constant union
{
    unsigned int w;
    float f;
} __sexp2_la_c4 = { 0x3c1d94cbu };

static __constant union
{
    unsigned int w;
    float f;
} __sexp2_la_c3 = { 0x3d635766u };

static __constant union
{
    unsigned int w;
    float f;
} __sexp2_la_c2 = { 0x3e75fdf1u };

static __constant union
{
    unsigned int w;
    float f;
} __sexp2_la_c1 = { 0x3e45c862u };

__attribute__((always_inline))
inline int __internal_sexp2_la_cout (float *a, float *r)
{
    int nRet = 0;
    float x = *a;
    float fN, R, poly, High, Rh, Rl;
    int sN, sN2;
    unsigned int N;
    union
    {
        unsigned int w;
        float f;
    } T, T2, xi, res;

    fN = SPIRV_OCL_BUILTIN(rint, _f32, ) (x);
    R = x - fN;

    sN = (int) fN;

    N = sN;

    poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (__sexp2_la_c6.f, R, __sexp2_la_c5.f);

    High = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (R, 0.5f, 1.0f);
    poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly, R, __sexp2_la_c4.f);

    Rh = High - 1.0f;
    poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly, R, __sexp2_la_c3.f);

    Rl = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (R, 0.5f, -Rh);
    poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly, R, __sexp2_la_c2.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly, R, __sexp2_la_c1.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly, R, Rl);
    res.f = High + poly;

    if (((unsigned int) (N + 0x7f - 2)) > 124 + 0x7f)
        goto EXP2F_SPECIAL;

    res.w += (N << 23);
    *r = res.f;
    return nRet;

  EXP2F_SPECIAL:
    xi.f = x;
    if ((xi.w & 0x7fffffffu) >= 0x7f800000u)
    {
        if (xi.w == 0xff800000)
        {
            *r = 0.0f;
            return nRet;
        }
        else
        {
            *r = x + x;
            return nRet;
        }
    }

    x = SPIRV_OCL_BUILTIN(fmin, _f32_f32, ) (x, 192.0f);
    x = SPIRV_OCL_BUILTIN(fmax, _f32_f32, ) (x, -192.0f);
    fN = SPIRV_OCL_BUILTIN(rint, _f32, ) (x);
    sN = (int) fN;

    sN2 = sN >> 1;
    sN -= sN2;

    T.w = (sN + 0x7f) << 23;
    T2.w = (sN2 + 0x7f) << 23;

    res.f *= T.f;
    res.f *= T2.f;

    nRet = (res.w < 0x00800000u) ? 4 : nRet;
    nRet = (res.w == 0x7f800000) ? 3 : nRet;

    *r = res.f;
    return nRet;
}

float __ocl_svml_exp2f (float a)
{

    float va1;
    float vr1;
    unsigned int vm;

    float r;

    va1 = a;;

    __internal_sexp2_la_cout (&va1, &vr1);
    r = vr1;;

    return r;

}

/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF
typedef struct
{
    unsigned int Exp_tbl_L[32];
    unsigned int Exp_tbl_H[32];

    unsigned int L2E;
    unsigned int Shifter;
    unsigned int L2H;
    unsigned int L2L;
    unsigned int EMask;

    unsigned int AbsMask;
    unsigned int Threshold;
    unsigned int SmallX;
    unsigned int IndexMask;

    unsigned int IndexMask2;
    unsigned int knc_Shifter;
    unsigned int knc_L2EH;
    unsigned int knc_L2EL;
    unsigned int knc_EMask;
    unsigned int poly_coeff2;
    unsigned int poly_coeff1;

} __internal_sexp10_la_data_avx512_t;
static __constant __internal_sexp10_la_data_avx512_t __internal_sexp10_la_data_avx512 = {
    {
     0x3f800001u, 0x3f801631u, 0x3f802c65u, 0x3f80429du,
     0x3f8058d9u, 0x3f806f18u, 0x3f80855cu, 0x3f809ba3u,
     0x3f80b1eeu, 0x3f80c83du, 0x3f80de90u, 0x3f80f4e7u,
     0x3f810b42u, 0x3f8121a0u, 0x3f813803u, 0x3f814e69u,
     0x3f8164d3u, 0x3f817b41u, 0x3f8191b3u, 0x3f81a829u,
     0x3f81bea2u, 0x3f81d520u, 0x3f81eba2u, 0x3f820227u,
     0x3f8218b0u, 0x3f822f3du, 0x3f8245cfu, 0x3f825c64u,
     0x3f8272fdu, 0x3f828999u, 0x3f82a03au, 0x3f82b6dfu,
     }
    , {
       0x3f800000u, 0x3f82cd87u, 0x3f85aac3u, 0x3f88980fu,
       0x3f8b95c2u, 0x3f8ea43au, 0x3f91c3d3u, 0x3f94f4f0u,
       0x3f9837f0u, 0x3f9b8d3au, 0x3f9ef532u, 0x3fa27043u,
       0x3fa5fed7u, 0x3fa9a15bu, 0x3fad583fu, 0x3fb123f6u,
       0x3fb504f3u, 0x3fb8fbafu, 0x3fbd08a4u, 0x3fc12c4du,
       0x3fc5672au, 0x3fc9b9beu, 0x3fce248cu, 0x3fd2a81eu,
       0x3fd744fdu, 0x3fdbfbb8u, 0x3fe0ccdfu, 0x3fe5b907u,
       0x3feac0c7u, 0x3fefe4bau, 0x3ff5257du, 0x3ffa83b3u,
       }

    , 0x40549A78u, 0x46400000u, 0x3e9a209bu, 0xb2760860u, 0x3f000000u, 0x7fffffffu, 0x4217B818u, 0x2f800000u, 0x0000007cu, 0x00000f80u, 0x4b400000u,
        0x40549A78u, 0x33979A37u, 0xbfffffffu, 0x4029B7DAu, 0x40135D8Du
};

typedef struct
{
    unsigned int _sT[(1 << 5)];
    unsigned int _sLg2_10;
    unsigned int _sShifter;
    unsigned int _sInvLg2_10hi;
    unsigned int _sInvLg2_10lo;
    unsigned int _sPC0;
    unsigned int _sPC1;
    unsigned int _sPC2;
    unsigned int _iIndexMask;
    unsigned int _iAbsMask;
    unsigned int _iDomainRange;
} __internal_sexp10_la_data_t;
static __constant __internal_sexp10_la_data_t __internal_sexp10_la_data = {
    {
     0x3f800000u,
     0x3f82cd87u,
     0x3f85aac3u,
     0x3f88980fu,
     0x3f8b95c2u,
     0x3f8ea43au,
     0x3f91c3d3u,
     0x3f94f4f0u,
     0x3f9837f0u,
     0x3f9b8d3au,
     0x3f9ef532u,
     0x3fa27043u,
     0x3fa5fed7u,
     0x3fa9a15bu,
     0x3fad583fu,
     0x3fb123f6u,
     0x3fb504f3u,
     0x3fb8fbafu,
     0x3fbd08a4u,
     0x3fc12c4du,
     0x3fc5672au,
     0x3fc9b9beu,
     0x3fce248cu,
     0x3fd2a81eu,
     0x3fd744fdu,
     0x3fdbfbb8u,
     0x3fe0ccdfu,
     0x3fe5b907u,
     0x3feac0c7u,
     0x3fefe4bau,
     0x3ff5257du,
     0x3ffa83b3u,
     },
    0x42d49a78u,
    0x4b400000u,
    0x3c1a2000u,
    0x341a84fcu,

    0x2fecc868u,
    0x40135e1bu,
    0x4029a8d2u,
    0x0000001fu,
    0x7fffffffu,
    0x4217b818u,
};

static __constant union
{
    unsigned int w;
    float f;
} __sexp10_la_Shifter = { 0x4ac000feu };

static __constant union
{
    unsigned int w;
    float f;
} __sexp10_la_L2_10 = { 0x40549A78u };

static __constant union
{
    unsigned int w;
    float f;
} __sexp10_la_L2H = { 0x3e9A209Bu };

static __constant union
{
    unsigned int w;
    float f;
} __sexp10_la_L2L = { 0xb2760860u };

static __constant union
{
    unsigned int w;
    float f;
} __sexp10_la_c5 = { 0x3f0a4794u };

static __constant union
{
    unsigned int w;
    float f;
} __sexp10_la_c4 = { 0x3f962559u };

static __constant union
{
    unsigned int w;
    float f;
} __sexp10_la_c3 = { 0x40023822u };

static __constant union
{
    unsigned int w;
    float f;
} __sexp10_la_c2 = { 0x4029a917u };

static __constant union
{
    unsigned int w;
    float f;
} __sexp10_la_c1 = { 0x40135d8eu };

__attribute__((always_inline))
inline int __internal_sexp10_la_cout (float *a, float *r)
{
    int nRet = 0;
    float x = *a;
    union
    {
        unsigned int w;
        float f;
    } S, Th, Tlr, Th2, xin, xa, res;
    float N, R, poly;
    int index_mask;

    S.f = (SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (x, __sexp10_la_L2_10.f, __sexp10_la_Shifter.f));
    N = (S.f - __sexp10_la_Shifter.f);

    R = (SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (-N, __sexp10_la_L2H.f, x));
    R = (SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (-N, __sexp10_la_L2L.f, R));

    Th.w = S.w << 22;

    index_mask = 0 - (S.w & 1);

    Th.w ^= (index_mask & 0x7504F3u);

    Tlr.w = index_mask & 0x329302AEu;

    poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (R, __sexp10_la_c5.f, __sexp10_la_c4.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (R, poly, __sexp10_la_c3.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (R, poly, __sexp10_la_c2.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (R, poly, __sexp10_la_c1.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (R, poly, Tlr.f);

    xin.f = x;
    xa.w = xin.w & 0x7fffffffu;

    if (xa.w > 0x4217B818u)
        goto EXPF_SPECIAL;

    res.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly, Th.f, Th.f);

    *r = res.f;
    return nRet;

  EXPF_SPECIAL:
    if (xa.w > 0x42349E35u)
    {
        if (xa.w > 0x7f800000u)
        {
            *r = x + x;
            return nRet;
        }
        if (x < 0)
        {
            *r = 0.0f;
            nRet = 4;
            return nRet;
        }

        nRet = 3;
        res.w = 0x7f800000;
        *r = res.f;
        return nRet;
    }

    S.w += 0xfe;
    Th2.w = (S.w >> 2) & 0xff;
    S.w -= (Th2.w << 1);

    Th2.w <<= 23;

    Th.w = S.w << 22;

    Th.w ^= (index_mask & 0x7504F3u);

    res.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly, Th.f, Th.f);
    res.f *= Th2.f;

    *r = res.f;
    return nRet;
}

float __ocl_svml_exp10f (float a)
{

    float va1;
    float vr1;
    unsigned int vm;

    float r;

    va1 = a;;

    __internal_sexp10_la_cout (&va1, &vr1);
    r = vr1;;

    return r;

}

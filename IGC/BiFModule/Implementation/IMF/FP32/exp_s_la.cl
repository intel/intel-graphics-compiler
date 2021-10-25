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
    unsigned int Zero;
    unsigned int knc_Shifter;
    unsigned int knc_L2EH;
    unsigned int knc_L2EL;
    unsigned int knc_EMask;

} __internal_sexp_la_data_avx512_t;
static __constant __internal_sexp_la_data_avx512_t __internal_sexp_la_data_avx512 = {
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

    , 0x3fB8AA3Bu, 0x46400000u, 0x3f317218u, 0xb102e308u, 0x3f000000u, 0x7fffffffu, 0x42AEAC4Fu, 0x2f800000u, 0x0000007cu, 0x00000f80u, 0x00000000u,
        0x4b400000u, 0x3fb8aa3bu, 0x32a57060u, 0xbfffffffu
};

typedef struct
{
    unsigned int _sInvLn2;
    unsigned int _sShifter;
    unsigned int _sLn2hi;
    unsigned int _sLn2lo;
    unsigned int _iBias;

    unsigned int _sPC0;
    unsigned int _sPC1;
    unsigned int _sPC2;
    unsigned int _sPC3;
    unsigned int _sPC4;
    unsigned int _sPC5;
    unsigned int _iAbsMask;
    unsigned int _iDomainRange;
    unsigned int _sOvfThreshold;
    unsigned int _sUdfThreshold;
} __internal_sexp_la_data_t;
static __constant __internal_sexp_la_data_t __internal_sexp_la_data = {
    0x3FB8AA3Bu,
    0x4b400000u,
    0x3F317200u,
    0x35BFBE8Eu,
    0x0000007fu,

    0x3F800000u,
    0x3F7FFFFEu,
    0x3EFFFF34u,
    0x3E2AACACu,
    0x3D2B8392u,
    0x3C07D9FEu,
    0x7fffffffu,
    0x42aeac4fu,

    0x42b17217u,
    0xc2cff1b4u,
};

static __constant union
{
    unsigned int w;
    float f;
} __sexp_la_Shifter = { 0x4ac000feu };

static __constant union
{
    unsigned int w;
    float f;
} __sexp_la_L2E = { 0x3FB8AA3Bu };

static __constant union
{
    unsigned int w;
    float f;
} __sexp_la_L2H = { 0x3f317218u };

static __constant union
{
    unsigned int w;
    float f;
} __sexp_la_L2L = { 0xb102E308u };

static __constant union
{
    unsigned int w;
    float f;
} __sexp_la_c5 = { 0x3c08ba8bu };

static __constant union
{
    unsigned int w;
    float f;
} __sexp_la_c4 = { 0x3d2aec4eu };

static __constant union
{
    unsigned int w;
    float f;
} __sexp_la_c3 = { 0x3e2aaa9cu };

static __constant union
{
    unsigned int w;
    float f;
} __sexp_la_c2 = { 0x3effffe8u };

static __constant union
{
    unsigned int w;
    float f;
} __sexp_la_c1 = { 0x3f800000u };

__attribute__((always_inline))
inline int __internal_sexp_la_cout (float *a, float *r)
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

    S.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (x, __sexp_la_L2E.f, __sexp_la_Shifter.f);
    N = S.f - __sexp_la_Shifter.f;

    R = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (-N, __sexp_la_L2H.f, x);
    R = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (-N, __sexp_la_L2L.f, R);

    Th.w = S.w << 22;

    index_mask = 0 - (S.w & 1);

    Th.w ^= (index_mask & 0x7504F3u);

    Tlr.w = index_mask & 0x329302AEu;

    poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (R, __sexp_la_c5.f, __sexp_la_c4.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (R, poly, __sexp_la_c3.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (R, poly, __sexp_la_c2.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (R, poly, __sexp_la_c1.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (R, poly, Tlr.f);

    xin.f = x;
    xa.w = xin.w & 0x7fffffffu;

    if (xa.w > 0x42AEAC4Fu)
        goto EXPF_SPECIAL;

    res.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly, Th.f, Th.f);

    *r = res.f;
    return nRet;

  EXPF_SPECIAL:
    if (xa.w > 0x432EAC4Fu)
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

        res.w = 0x7f800000;
        *r = res.f;
        nRet = 3;
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

float __ocl_svml_expf (float a)
{

    float va1;
    float vr1;
    unsigned int vm;

    float r;

    va1 = a;;

    __internal_sexp_la_cout (&va1, &vr1);
    r = vr1;;

    return r;

}

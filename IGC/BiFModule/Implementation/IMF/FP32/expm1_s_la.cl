/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF
typedef struct
{
    unsigned int Exp_tbl_H[32];
    unsigned int Exp_tbl_L[32];
    unsigned int L2E;
    unsigned int Shifter;
    unsigned int Threshold;
    unsigned int SgnMask;
    unsigned int L2H;
    unsigned int L2L;
    unsigned int EMask;
    unsigned int poly_coeff3;
    unsigned int poly_coeff2;
    unsigned int One;
} __internal_sexpm1_la_data_avx512_t;
static __constant __internal_sexpm1_la_data_avx512_t __internal_sexpm1_la_data_avx512 = {
    {
     0x3f800000u, 0x3f82cd87u, 0x3f85aac3u, 0x3f88980fu,
     0x3f8b95c2u, 0x3f8ea43au, 0x3f91c3d3u, 0x3f94f4f0u,
     0x3f9837f0u, 0x3f9b8d3au, 0x3f9ef532u, 0x3fa27043u,
     0x3fa5fed7u, 0x3fa9a15bu, 0x3fad583fu, 0x3fb123f6u,
     0x3fb504f3u, 0x3fb8fbafu, 0x3fbd08a4u, 0x3fc12c4du,
     0x3fc5672au, 0x3fc9b9beu, 0x3fce248cu, 0x3fd2a81eu,
     0x3fd744fdu, 0x3fdbfbb8u, 0x3fe0ccdfu, 0x3fe5b907u,
     0x3feac0c7u, 0x3fefe4bau, 0x3ff5257du, 0x3ffa83b3u,
     }
    , {
       0x00000000u, 0xb34a3a0au, 0x3346cb6au, 0xb36ed17eu,
       0xb24e0611u, 0xb3517dd9u, 0x334b2482u, 0xb31586deu,
       0x33092801u, 0xb2e6f467u, 0x331b85f2u, 0x3099b6f1u,
       0xb3051aa8u, 0xb2e2a0dau, 0xb2006c56u, 0xb3365942u,
       0x329302aeu, 0x32c595dcu, 0xb302e5a2u, 0xb28e10a1u,
       0x31b3d0e5u, 0xb31a472bu, 0x31d1daf2u, 0xb305bf64u,
       0xb27ce182u, 0xb2f26443u, 0xb1b4b0dau, 0xb1da8a8fu,
       0xb1d290beu, 0xb2d5b899u, 0x31b0a147u, 0xb2156afcu,
       }

    , 0x3fB8AA3Bu, 0x48c00000u, 0x42AD496Bu, 0x80000000u, 0x3f317218u, 0xb102e308u, 0xbfffffffu, 0x3e2AABF3u, 0x3f0000F6u, 0x3f800000u
};

typedef struct
{

    unsigned int Expm1_HA_table[(1 << 7)];

    unsigned int poly_coeff[4];
    unsigned int Log2e;
    unsigned int L2H;
    unsigned int L2L;
    unsigned int ExpAddConst;
    unsigned int IndexMask;
    unsigned int ExpMask;
    unsigned int HalfMask;
    unsigned int MOne;
    unsigned int AbsMask;
    unsigned int Threshold;
    unsigned int Log2e_5;
    unsigned int L2;
    unsigned int ExpAddConst2;
    unsigned int IndexMask2;
    unsigned int ExpMask2;
} __internal_sexpm1_la_data_t;
static __constant __internal_sexpm1_la_data_t __internal_sexpm1_la_data = {

    {
     0x00000000u, 0x00000000u, 0x00016000u, 0x391a3e78u, 0x0002d000u, 0xb89e59d5u, 0x00044000u, 0xb93ae78au, 0x0005b000u, 0xb9279306u, 0x00072000u,
     0xb79e6961u, 0x0008a000u, 0xb97e2feeu, 0x000a1000u, 0x391aaea9u, 0x000b9000u, 0x39383c7du, 0x000d2000u, 0xb9241490u, 0x000ea000u, 0x39073169u,
     0x00103000u, 0x386e218au, 0x0011c000u, 0x38f4dcebu, 0x00136000u, 0xb93a9a1eu, 0x0014f000u, 0x391df520u, 0x00169000u, 0x3905a6e4u, 0x00183000u,
     0x397e0a32u, 0x0019e000u, 0x370b2641u, 0x001b9000u, 0xb8b1918bu, 0x001d4000u, 0xb8132c6au, 0x001ef000u, 0x39264c12u, 0x0020b000u, 0x37221f73u,
     0x00227000u, 0x37060619u, 0x00243000u, 0x3922b5c1u, 0x00260000u, 0xb814ab27u, 0x0027d000u, 0xb89b12c6u, 0x0029a000u, 0x382d5a75u, 0x002b8000u,
     0xb938c94bu, 0x002d6000u, 0xb97822b8u, 0x002f4000u, 0xb910ea53u, 0x00312000u, 0x38fd6075u, 0x00331000u, 0x38620955u, 0x00350000u, 0x391e667fu,
     0x00370000u, 0xb89b8736u, 0x00390000u, 0xb90a1714u, 0x003b0000u, 0xb7a54dedu, 0x003d1000u, 0xb96b8c15u, 0x003f1000u, 0x397336cfu, 0x00413000u,
     0xb8eccd66u, 0x00434000u, 0x39599b45u, 0x00456000u, 0x3965422bu, 0x00479000u, 0xb8a2cdd5u, 0x0049c000u, 0xb9484f32u, 0x004bf000u, 0xb8fac043u,
     0x004e2000u, 0x391182a4u, 0x00506000u, 0x38ccf6bcu, 0x0052b000u, 0xb97c4dc2u, 0x0054f000u, 0x38d6aaf4u, 0x00574000u, 0x391f995bu, 0x0059a000u,
     0xb8ba8f62u, 0x005c0000u, 0xb9090d05u, 0x005e6000u, 0x37f4825eu, 0x0060d000u, 0xb8c844f5u, 0x00634000u, 0xb76d1a83u, 0x0065c000u, 0xb95f2310u,
     0x00684000u, 0xb952b5f8u, 0x006ac000u, 0x37c6e7ddu, 0x006d5000u, 0xb7cfe126u, 0x006fe000u, 0x3917337cu, 0x00728000u, 0x383b9e2du, 0x00752000u,
     0x392fa2a5u, 0x0077d000u, 0x37df730bu, 0x007a8000u, 0x38ecb6ddu, 0x007d4000u, 0xb879f986u}
    , {
       0x3e2AAABFu, 0x3f00000Fu}

    , 0x42B8AA3Bu, 0x3c318000u, 0xb65e8083u, 0x49f0fe00u, 0x000001f8u, 0x0001fe00u, 0xfffff000u, 0xbf800000u, 0x7fffffffu, 0x42AD496Bu, 0x4238AA3Bu,
        0x3cb17218u, 0x4a403f80u, 0x0000007cu, 0x00007f80u
};
static __constant int_float __sexpm1_la_c6 = { 0x39533223u };
static __constant int_float __sexpm1_la_c5 = { 0x3ab8cbe5u };
static __constant int_float __sexpm1_la_c4 = { 0x3c088428u };
static __constant int_float __sexpm1_la_c3 = { 0x3d2aa55eu };
static __constant int_float __sexpm1_la_c2 = { 0x3e2aaab3u };
static __constant int_float __sexpm1_la_c1 = { 0x3f000005u };
static __constant int_float __sexpm1_la_c0 = { 0xb11f699bu };
static __constant int_float __sexpm1_la_fL2E = { 0x3FB8AA3Bu };
static __constant int_float __sexpm1_la_fShifter = { 0x4b40007fu };

static __constant int_float __sexpm1_la_NL2H = { 0xbf317218u };
static __constant int_float __sexpm1_la_NL2L = { 0x3102E308u };

__attribute__((always_inline))
inline int __internal_sexpm1_la_cout (float *a, float *pres)
{
    int nRet = 0;
    float xin = *a;
    int_float xf, fN, xL2E, fS, xa;
    int_float T, sc, res;
    float R, poly, Th;

    xf.f = xin;
    xL2E.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (xf.f, __sexpm1_la_fL2E.f, 0.0f);
    fN.f = SPIRV_OCL_BUILTIN(trunc, _f32, ) (xL2E.f);
    fS.f = __sexpm1_la_fShifter.f + fN.f;

    R = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (fN.f, __sexpm1_la_NL2H.f, xf.f);
    R = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (fN.f, __sexpm1_la_NL2L.f, R);

    T.w = fS.w << 23;

    poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (__sexpm1_la_c6.f, R, __sexpm1_la_c5.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly, R, __sexpm1_la_c4.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly, R, __sexpm1_la_c3.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly, R, __sexpm1_la_c2.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly, R, __sexpm1_la_c1.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly, R, __sexpm1_la_c0.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly, R, R);

    Th = T.f - 1.0f;

    res.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (T.f, poly, Th);

    xa.f = SPIRV_OCL_BUILTIN(fabs, _f32, ) (xf.f);
    res.w |= (xf.w ^ xa.w);

    if (SPIRV_OCL_BUILTIN(fabs, _f32, ) (xf.f) <= 87.0f)
    {
        *pres = res.f;
        return nRet;
    }

    if (xf.f < 0.0f)
    {
        *pres = -1.0f;
        return nRet;
    }

    if (!(xf.f < 128.0f))
    {

        xa.w = xf.w & 0x7fffffff;
        if (xa.w > 0x7f800000)
        {
            *pres = xf.f + res.f;
            return nRet;
        }

        res.w = 0x7f800000 - 1;
        res.f = res.f * res.f;

        nRet = 3;
        {
            *pres = res.f;
            return nRet;
        }
    }

    T.w = (fS.w - 64) << 23;

    res.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (T.f, poly, T.f);
    sc.w = 0x5f800000u;
    res.f *= sc.f;

    if (res.w == 0x7f800000)
        nRet = 3;

    *pres = res.f;
    return nRet;

}

float __ocl_svml_expm1f (float a)
{

    float va1;
    float vr1;
    unsigned int vm;

    float r;

    va1 = a;;

    __internal_sexpm1_la_cout (&va1, &vr1);
    r = vr1;;

    return r;

}

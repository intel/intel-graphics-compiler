/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF
typedef struct
{
    unsigned int Log_tbl_H[32];
    unsigned int Log_tbl_L[32];
    unsigned int One;
    unsigned int AbsMask;
    unsigned int SmallThreshold;
    unsigned int Threshold;
    unsigned int LargeThreshold;
    unsigned int ca1;
    unsigned int c2s;
    unsigned int c1s;
    unsigned int AddB5;
    unsigned int RcpBitMask;
    unsigned int OneEighth;
    unsigned int Four;
    unsigned int poly_coeff3;
    unsigned int poly_coeff2;
    unsigned int poly_coeff1;
    unsigned int L2H;
    unsigned int L2L;
} __internal_sasinh_la_data_avx512_t;
static __constant __internal_sasinh_la_data_avx512_t __internal_sasinh_la_data_avx512 = {
    {
     0x00000000u, 0xbcfc0000u, 0xbd788000u, 0xbdb78000u, 0xbdf14000u, 0xbe14a000u, 0xbe300000u, 0xbe4aa000u, 0xbe648000u, 0xbe7dc000u, 0xbe8b4000u,
     0xbe974000u, 0xbea31000u, 0xbeae9000u, 0xbeb9d000u, 0xbec4d000u, 0xbecfa000u, 0xbeda2000u, 0xbee48000u, 0xbeeea000u, 0xbef89000u, 0xbf012800u,
     0xbf05f000u, 0xbf0aa800u, 0xbf0f4000u, 0xbf13c800u, 0xbf184000u, 0xbf1ca000u, 0xbf20f000u, 0xbf252800u, 0xbf295000u, 0xbf2d6800u}
    , {
       0x80000000u, 0xb726c39eu, 0x3839e7feu, 0xb7528ae5u, 0x377891d5u, 0xb8297c10u, 0x37cf8f58u, 0x3852b186u, 0x35838656u, 0xb80c36afu, 0x38235454u,
       0xb862bae1u, 0x37e87bc7u, 0x37848150u, 0x37202511u, 0xb74e1b05u, 0x385c1340u, 0xb8777bcdu, 0x36038656u, 0xb7d40984u, 0xb80f5fafu, 0xb8254b4cu,
       0xb865c84au, 0x37f0b42du, 0xb83ebce1u, 0xb83c2513u, 0x37a332c4u, 0x3779654fu, 0x38602f73u, 0x367449f8u, 0xb7b4996fu, 0xb800986bu}

    , 0x3f800000u, 0x7fffffffu, 0x3c800000u, 0x5f000000u, 0x7f7fffffu, 0xbe2AA5DEu, 0x3ec00000u, 0x3f000000u, 0x00020000u, 0xfffc0000u, 0x3e000000u,
        0x40800000u, 0xbe800810u, 0x3eaab11eu, 0xbf000000u, 0x3f317000u, 0x3805fdf4u
};

typedef struct
{
    unsigned int Log_HA_table[(1 << 8) + 2];
    unsigned int SgnMask;
    unsigned int XThreshold;
    unsigned int XhMask;
    unsigned int ExpMask0;
    unsigned int ExpMask2;
    unsigned int ha_poly_coeff[2];
    unsigned int ExpMask;
    unsigned int Two10;
    unsigned int MinLog1p;
    unsigned int MaxLog1p;
    unsigned int HalfMask;
    unsigned int L2H;
    unsigned int L2L;
    unsigned int sOne;
    unsigned int sPoly[8];
    unsigned int iHiDelta;
    unsigned int iLoRange;
    unsigned int iBrkValue;
    unsigned int iOffExpoMask;
    unsigned int sBigThreshold;
    unsigned int sC2;
    unsigned int sC3;
    unsigned int sHalf;
    unsigned int sLargestFinite;
    unsigned int sLittleThreshold;
    unsigned int sSign;
    unsigned int sThirtyOne;
    unsigned int sTopMask11;
    unsigned int sTopMask12;
    unsigned int sTopMask8;
    unsigned int XScale;

    unsigned int sLn2;

    unsigned int sInfs[2];
    unsigned int sOnes[2];
    unsigned int sZeros[2];
} __internal_sasinh_la_data_t;
static __constant __internal_sasinh_la_data_t __internal_sasinh_la_data = {

    {
     0xc2aeac38u, 0xb93cbf08u, 0xc2aeb034u, 0xb93ce972u, 0xc2aeb424u, 0xb95e1069u, 0xc2aeb814u, 0xb9412b26u, 0xc2aebbfcu, 0xb9272b41u, 0xc2aebfd4u,
     0xb950fcd7u, 0xc2aec3acu, 0xb93f86b8u, 0xc2aec77cu, 0xb933aa90u, 0xc2aecb44u, 0xb92e4507u, 0xc2aecf04u, 0xb9302df1u, 0xc2aed2bcu, 0xb93a3869u,
     0xc2aed66cu, 0xb94d32f7u, 0xc2aeda1cu, 0xb929e7b5u, 0xc2aeddbcu, 0xb9511c6au, 0xc2aee15cu, 0xb94392acu, 0xc2aee4f4u, 0xb94207fdu, 0xc2aee884u,
     0xb94d35eau, 0xc2aeec14u, 0xb925d225u, 0xc2aeef94u, 0xb94c8ea1u, 0xc2aef314u, 0xb94219adu, 0xc2aef68cu, 0xb9471e0bu, 0xc2aef9fcu, 0xb95c430bu,
     0xc2aefd6cu, 0xb9422ca0u, 0xc2af00d4u, 0xb9397b7bu, 0xc2af0434u, 0xb942cd1cu, 0xc2af0794u, 0xb91ebbeau, 0xc2af0ae4u, 0xb94ddf49u, 0xc2af0e34u,
     0xb950cbabu, 0xc2af1184u, 0xb92812a5u, 0xc2af14c4u, 0xb9544303u, 0xc2af1804u, 0xb955e8d7u, 0xc2af1b44u, 0xb92d8d8du, 0xc2af1e74u, 0xb95bb7fau,
     0xc2af21acu, 0xb920ec71u, 0xc2af24d4u, 0xb93dacccu, 0xc2af27fcu, 0xb9327882u, 0xc2af2b1cu, 0xb93fccb3u, 0xc2af2e3cu, 0xb9262434u, 0xc2af3154u,
     0xb925f7a4u, 0xc2af3464u, 0xb93fbd72u, 0xc2af3774u, 0xb933e9f2u, 0xc2af3a7cu, 0xb942ef61u, 0xc2af3d84u, 0xb92d3dfbu, 0xc2af4084u, 0xb93343ffu,
     0xc2af437cu, 0xb9556dbfu, 0xc2af4674u, 0xb95425adu, 0xc2af496cu, 0xb92fd461u, 0xc2af4c5cu, 0xb928e0a9u, 0xc2af4f44u, 0xb93faf8eu, 0xc2af522cu,
     0xb934a465u, 0xc2af550cu, 0xb94820d2u, 0xc2af57ecu, 0xb93a84d8u, 0xc2af5ac4u, 0xb94c2eddu, 0xc2af5d9cu, 0xb93d7bb5u, 0xc2af606cu, 0xb94ec6aeu,
     0xc2af633cu, 0xb9406992u, 0xc2af6604u, 0xb952bcb6u, 0xc2af68ccu, 0xb94616feu, 0xc2af6b8cu, 0xb95acde8u, 0xc2af6e4cu, 0xb951358fu, 0xc2af710cu,
     0xb929a0b7u, 0xc2af73c4u, 0xb92460d4u, 0xc2af7674u, 0xb941c60fu, 0xc2af7924u, 0xb9421f4du, 0xc2af7bd4u, 0xb925ba37u, 0xc2af7e7cu, 0xb92ce340u,
     0xc2af811cu, 0xb957e5adu, 0xc2af83c4u, 0xb9270b99u, 0xc2af865cu, 0xb95a9dfau, 0xc2af88fcu, 0xb932e4acu, 0xc2af8b94u, 0xb9302671u, 0xc2af8e24u,
     0xb952a8fau, 0xc2af90b4u, 0xb95ab0eeu, 0xc2af9344u, 0xb94881e8u, 0xc2af95ccu, 0xb95c5e87u, 0xc2af9854u, 0xb9568869u, 0xc2af9adcu, 0xb9374037u,
     0xc2af9d5cu, 0xb93ec5a6u, 0xc2af9fdcu, 0xb92d577du, 0xc2afa254u, 0xb9433399u, 0xc2afa4ccu, 0xb94096f3u, 0xc2afa744u, 0xb925bda3u, 0xc2afa9b4u,
     0xb932e2e5u, 0xc2afac24u, 0xb928411du, 0xc2afae8cu, 0xb94611dau, 0xc2afb0f4u, 0xb94c8ddbu, 0xc2afb35cu, 0xb93bed15u, 0xc2afb5bcu, 0xb95466b2u,
     0xc2afb81cu, 0xb9563119u, 0xc2afba7cu, 0xb94181f0u, 0xc2afbcd4u, 0xb9568e1eu, 0xc2afbf2cu, 0xb95589d1u, 0xc2afc184u, 0xb93ea881u, 0xc2afc3d4u,
     0xb9521cf3u, 0xc2afc624u, 0xb950193bu, 0xc2afc874u, 0xb938cec0u, 0xc2afcabcu, 0xb94c6e3fu, 0xc2afcd04u, 0xb94b27d0u, 0xc2afcf4cu, 0xb9352ae6u,
     0xc2afd18cu, 0xb94aa653u, 0xc2afd3ccu, 0xb94bc84cu, 0xc2afd60cu, 0xb938be68u, 0xc2afd844u, 0xb951b5a9u, 0xc2afda7cu, 0xb956da79u, 0xc2afdcb4u,
     0xb94858aeu, 0xc2afdeecu, 0xb9265b90u, 0xc2afe11cu, 0xb9310dd5u, 0xc2afe34cu, 0xb92899abu, 0xc2afe574u, 0xb94d28b2u, 0xc2afe7a4u, 0xb91ee407u,
     0xc2afe9c4u, 0xb95df440u, 0xc2afebecu, 0xb94a8170u, 0xc2afee14u, 0xb924b32au, 0xc2aff034u, 0xb92cb084u, 0xc2aff254u, 0xb922a015u, 0xc2aff46cu,
     0xb946a7fcu, 0xc2aff684u, 0xb958eddfu, 0xc2aff89cu, 0xb95996edu, 0xc2affab4u, 0xb948c7e3u, 0xc2affcccu, 0xb926a508u, 0xc2affedcu, 0xb9335235u,
     0xc2b000ecu, 0xb92ef2d4u, 0xc2b002f4u, 0xb959a9e1u, 0xc2b00504u, 0xb93399eeu, 0xc2b0070cu, 0xb93ce522u, 0xc2b00914u, 0xb935ad3du, 0xc2b00b14u,
     0xb95e1399u, 0xc2b00d1cu, 0xb936392bu, 0xc2b00f1cu, 0xb93e3e84u}

    , 0x7fffffffu, 0x39800000u, 0xffffff00u, 0x7f800000u, 0x7b000000u, {

                                                                        0x3eAAAB39u, 0xbf000036u}

    , 0x007fffffu, 0x3b800000u, 0xbf7fffffu, 0x7a800000u, 0xffffff00u, 0x3f317200u, 0x35bfbe00u, 0x3f800000u, {
                                                                                                               0xbf000000u, 0x3eaaaa94u, 0xbe80058eu,
                                                                                                               0x3e4ce190u, 0xbe28ad37u, 0x3e0fcb12u,
                                                                                                               0xbe1ad9e3u, 0x3e0d84edu}

    , 0x01000000u, 0x01800000u, 0x3f2aaaabu, 0x007fffffu, 0x4E800000u, 0x3EC00000u, 0x3EA00000u, 0x3F000000u, 0x7F7FFFFFu, 0x3D800000u, 0x80000000u,
        0x41F80000u, 0xFFFFE000u, 0xFFFFF000u, 0xFFFF0000u, 0x30800000u, 0x3f317218u, {0x7f800000u, 0xff800000u}

    , {0x3f800000u, 0xbf800000u}

    , {0x00000000u, 0x80000000u}

};
static __constant int_float __sasinh_la_large_x = { 0x49800000u };

static __constant int_float __sasinh_la_small_x = { 0x39800000u };

static __constant int_float __sasinh_la_largest_norm = { 0x7f7fffffu };
static __constant int_float __sasinh_la_ln2 = { 0x3f317218u };

static __constant int_float __sasinh_la_c8 = { 0x3d63bde3u };
static __constant int_float __sasinh_la_c7 = { 0xbdfa61f2u };
static __constant int_float __sasinh_la_c6 = { 0x3e19c853u };
static __constant int_float __sasinh_la_c5 = { 0xbe2c204au };
static __constant int_float __sasinh_la_c4 = { 0x3e4c843bu };
static __constant int_float __sasinh_la_c3 = { 0xbe7fef6cu };
static __constant int_float __sasinh_la_c2 = { 0x3eaaab24u };
static __constant int_float __sasinh_la_c1 = { 0xbf00000au };

__attribute__((always_inline))
inline int __internal_sasinh_la_cout (float *a, float *r)
{
    int nRet = 0;
    float x = *a;
    float x2h, z2h, x2l, z2l, A, B, Bh, Sh, S0h, Sl, RS, E, Yhh;
    float Bl, poly, R;
    int_float Yh, Yl, res, xin, sgn, xa, two_expon;
    int expon, e23, iexpon_corr;

    x2h = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (x, x, 0.0f);
    z2h = x2h + 1.0f;

    A = SPIRV_OCL_BUILTIN(fmax, _f32_f32, ) (x2h, 1.0f);
    B = SPIRV_OCL_BUILTIN(fmin, _f32_f32, ) (x2h, 1.0f);

    x2l = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (x, x, -(x2h));
    Bh = z2h - A;
    Bl = B - Bh;
    z2l = x2l + Bl;

    RS = 1.0f / SPIRV_OCL_BUILTIN(sqrt, _f32, ) (z2h);

    S0h = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (z2h, RS, 0.0f);

    RS *= 0.5f;

    E = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (-(S0h), S0h, z2h);
    E = E + z2l;

    Sl = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (E, RS, 0.0f);

    Sh = S0h + Sl;
    Yhh = Sh - S0h;
    Sl = Sl - Yhh;

    xa.f = SPIRV_OCL_BUILTIN(fabs, _f32, ) (x);

    Yh.f = xa.f + Sh;
    Yhh = Yh.f - Sh;
    Yl.f = xa.f - Yhh;
    Yl.f = Yl.f + Sl;

    Yh.f = (xa.f < __sasinh_la_large_x.f) ? Yh.f : xa.f * 0.5f;
    Yl.f = (xa.f < __sasinh_la_large_x.f) ? Yl.f : 0;

    iexpon_corr = (xa.f < __sasinh_la_large_x.f) ? 0 : 2;

    expon = ((Yh.w + 0x00400000) >> 23) - 0x7f;

    e23 = expon << 23;

    two_expon.w = 0x3f800000 - e23;

    Yl.f *= two_expon.f;

    Yh.w -= e23;

    R = Yh.f - 1.0f;
    R = Yl.f + R;

    expon += iexpon_corr;

    poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (__sasinh_la_c8.f, R, __sasinh_la_c7.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly, R, __sasinh_la_c6.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly, R, __sasinh_la_c5.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly, R, __sasinh_la_c4.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly, R, __sasinh_la_c3.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly, R, __sasinh_la_c2.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly, R, __sasinh_la_c1.f);

    xin.f = x;
    sgn.w = xin.w ^ xa.w;

    poly *= R;
    poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly, R, R);

    res.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (((float) expon), __sasinh_la_ln2.f, poly);

    res.w ^= sgn.w;

    res.f = ((xa.f < __sasinh_la_small_x.f) | (xa.w > __sasinh_la_largest_norm.w)) ? (x + sgn.f) : res.f;
    *r = res.f;
    return nRet;

}

float __ocl_svml_asinhf (float a)
{

    float va1;
    float vr1;
    unsigned int vm;

    float r;

    va1 = a;;

    __internal_sasinh_la_cout (&va1, &vr1);
    r = vr1;;

    return r;

}

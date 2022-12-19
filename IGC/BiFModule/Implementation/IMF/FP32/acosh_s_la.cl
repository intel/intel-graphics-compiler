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
} __internal_sacosh_la_data_avx512_t;
static __constant __internal_sacosh_la_data_avx512_t __internal_sacosh_la_data_avx512 = {
    {
     0x00000000u, 0xbcfc0000u, 0xbd788000u, 0xbdb78000u, 0xbdf14000u, 0xbe14a000u, 0xbe300000u, 0xbe4aa000u, 0xbe648000u, 0xbe7dc000u, 0xbe8b4000u,
     0xbe974000u, 0xbea31000u, 0xbeae9000u, 0xbeb9d000u, 0xbec4d000u, 0xbecfa000u, 0xbeda2000u, 0xbee48000u, 0xbeeea000u, 0xbef89000u, 0xbf012800u,
     0xbf05f000u, 0xbf0aa800u, 0xbf0f4000u, 0xbf13c800u, 0xbf184000u, 0xbf1ca000u, 0xbf20f000u, 0xbf252800u, 0xbf295000u, 0xbf2d6800u}
    , {
       0x80000000u, 0xb726c39eu, 0x3839e7feu, 0xb7528ae5u, 0x377891d5u, 0xb8297c10u, 0x37cf8f58u, 0x3852b186u, 0x35838656u, 0xb80c36afu, 0x38235454u,
       0xb862bae1u, 0x37e87bc7u, 0x37848150u, 0x37202511u, 0xb74e1b05u, 0x385c1340u, 0xb8777bcdu, 0x36038656u, 0xb7d40984u, 0xb80f5fafu, 0xb8254b4cu,
       0xb865c84au, 0x37f0b42du, 0xb83ebce1u, 0xb83c2513u, 0x37a332c4u, 0x3779654fu, 0x38602f73u, 0x367449f8u, 0xb7b4996fu, 0xb800986bu}

    , 0x3f800000u, 0x7fffffffu, 0x39800000u, 0x5f000000u, 0x7f7fffffu, 0xbe2AA5DEu, 0x3ec00000u, 0x3f000000u, 0x00020000u, 0xfffc0000u, 0x3e000000u,
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
} __internal_sacosh_la_data_t;
static __constant __internal_sacosh_la_data_t __internal_sacosh_la_data = {

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

static _iml_v2_sp_union_t _iml_sacosh_cout_tab[3] = {

    0x3F800000,
    0x00000000,
    0x7F800000
};

#pragma float_control(push)
#pragma float_control(precise, on)
__attribute__((always_inline))
inline int __internal_sacosh_la_cout (float *a, float *r)
{
    int nRet = 0;
    float purex = *a;

    if ((((((_iml_v2_sp_union_t *) & purex)->hex[0] >> 23) & 0xFF) == 0xFF) && ((((_iml_v2_sp_union_t *) & purex)->hex[0] & 0x007FFFFF) != 0x0))
    {
        (*r) = purex * purex;
        return nRet;
    }

    if (((_iml_v2_sp_union_t *) & (purex))->hex[0] == ((_iml_v2_sp_union_t *) & (((float *) _iml_sacosh_cout_tab)[0]))->hex[0])
    {
        (*r) = (float) ((float *) _iml_sacosh_cout_tab)[1];
        return nRet;
    }

    if (((_iml_v2_sp_union_t *) & (purex))->hex[0] == ((_iml_v2_sp_union_t *) & (((float *) _iml_sacosh_cout_tab)[2]))->hex[0])
    {
        (*r) = (float) ((float *) _iml_sacosh_cout_tab)[2];
        return nRet;
    }

    {
        (*r) = (float) (((float *) _iml_sacosh_cout_tab)[2] * ((float *) _iml_sacosh_cout_tab)[1]);
        nRet = 1;
        return nRet;
    }
}

#pragma float_control(pop)
float __ocl_svml_acoshf (float a)
{

    float va1;
    float vr1;
    unsigned int vm;

    float r;

    va1 = a;;

    {

        float SgnMask;
        float FpExponPlus;
        float sU;
        float sUHi;
        float sULo;
        float sV;
        float sVHi;
        float sVLo;
        float sVTmp;
        float sTmp1;
        float sTmp2;
        float sTmp3;
        float sTmp4;
        float sTmp5;
        float sTmp6;
        float sTmp7;
        float sTmp8;
        float sY;
        float sW;
        float sZ;
        float sR;
        float sS;
        float sT;
        float sE;
        float sTopMask8;
        float sTopMask12;
        float sC1;
        float sC2;
        float sC3;
        float sPol1;
        float sPol2;
        float sCorr;
        float sTmpf1;
        float sTmpf2;
        float sTmpf3;
        float sTmpf4;
        float sTmpf5;
        float sTmpf6;
        float sH;
        float sL;
        float XScale;
        float sThirtyOne;
        float sInfinityMask;
        float sTooSmallMask;
        float sSpecialMask;
        unsigned int iSpecialMask;
        float sBigThreshold;
        float sModerateMask;
        float sLargestFinite;
        unsigned int iBrkValue;
        unsigned int iOffExpoMask;
        float One;
        unsigned int iOne;
        float sExp;
        float X;
        float Xl;
        float A;
        float B;
        float Rl;
        float Rh;
        float Rlh;
        float sR2;
        float Kh;

        float sLn2;

        float sPoly[8];
        unsigned int iX;
        float sN;
        unsigned int iN;
        unsigned int iR;
        float sP;
        unsigned int iExp;

        One = as_float (__internal_sacosh_la_data.sOne);
        sLargestFinite = as_float (__internal_sacosh_la_data.sLargestFinite);
        sInfinityMask = as_float (((unsigned int) (-(signed int) (!(va1 <= sLargestFinite)))));
        sTooSmallMask = as_float (((unsigned int) (-(signed int) (!(va1 > One)))));
        sSpecialMask = as_float ((as_uint (sInfinityMask) | as_uint (sTooSmallMask)));
        iSpecialMask = as_uint (sSpecialMask);
        vm = 0;
        vm = iSpecialMask;
        sBigThreshold = as_float (__internal_sacosh_la_data.sBigThreshold);
        sModerateMask = as_float (((unsigned int) (-(signed int) (va1 < sBigThreshold))));

        sU = (va1 - One);
        sV = (va1 + One);
        sTmp5 = (sU * sV);
        sTopMask8 = as_float (__internal_sacosh_la_data.sTopMask8);
        sY = as_float ((as_uint (sTmp5) & as_uint (sTopMask8)));

        sW = (sTmp5 - sY);

        sZ = (1.0f / SPIRV_OCL_BUILTIN(sqrt, _f32, ) (sY));
        sR = as_float ((as_uint (sZ) & as_uint (sTopMask8)));

        sS = (sY * sR);
        sT = (sW * sR);

        sE = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (-(sS), sR, One);
        sE = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (-(sT), sR, sE);
        sC3 = as_float (__internal_sacosh_la_data.sC3);
        sC2 = as_float (__internal_sacosh_la_data.sC2);
        sC2 = as_float (__internal_sacosh_la_data.sC2);
        sPol2 = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sC3, sE, sC2);

        sC1 = as_float (__internal_sacosh_la_data.sHalf);
        sPol1 = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sPol2, sE, sC1);

        sCorr = (sPol1 * sE);
        sTmpf1 = (sS + sT);
        sTmpf2 = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sTmpf1, sCorr, sTmpf1);
        sH = (sU + sTmpf2);
        A = ((One > sH) ? One : sH);
        B = ((One < sH) ? One : sH);

        X = (A + B);
        Xl = (A - X);
        Xl = (Xl + B);

        XScale = as_float (__internal_sacosh_la_data.XScale);
        XScale = (va1 * XScale);
        X = as_float ((((~as_uint (sModerateMask)) & as_uint (XScale)) | (as_uint (sModerateMask) & as_uint (X))));
        Xl = as_float ((as_uint (Xl) & as_uint (sModerateMask)));

        iX = as_uint (X);

        iBrkValue = (__internal_sacosh_la_data.iBrkValue);
        iOffExpoMask = (__internal_sacosh_la_data.iOffExpoMask);
        iX = (iX - iBrkValue);
        iR = (iX & iOffExpoMask);
        iN = ((signed int) iX >> (23));
        iR = (iR + iBrkValue);
        sN = ((float) ((int) (iN)));
        sR = as_float (iR);

        iExp = ((unsigned int) (iN) << (23));
        iOne = as_uint (One);
        iExp = (iOne - iExp);
        sExp = as_float (iExp);
        Rl = (Xl * sExp);

        Rh = (sR - One);
        sR = (Rh + Rl);
        sPoly[7] = as_float (__internal_sacosh_la_data.sPoly[7]);
        sPoly[6] = as_float (__internal_sacosh_la_data.sPoly[6]);
        sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sPoly[7], sR, sPoly[6]);
        sPoly[5] = as_float (__internal_sacosh_la_data.sPoly[5]);
        sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sP, sR, sPoly[5]);
        sPoly[4] = as_float (__internal_sacosh_la_data.sPoly[4]);
        sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sP, sR, sPoly[4]);
        sPoly[3] = as_float (__internal_sacosh_la_data.sPoly[3]);
        sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sP, sR, sPoly[3]);
        sPoly[2] = as_float (__internal_sacosh_la_data.sPoly[2]);
        sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sP, sR, sPoly[2]);
        sPoly[1] = as_float (__internal_sacosh_la_data.sPoly[1]);
        sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sP, sR, sPoly[1]);
        sPoly[0] = as_float (__internal_sacosh_la_data.sPoly[0]);
        sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sP, sR, sPoly[0]);
        sP = (sP * sR);
        sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sP, sR, sR);

        sThirtyOne = as_float (__internal_sacosh_la_data.sThirtyOne);
        FpExponPlus = (sN + sThirtyOne);
        sN = as_float ((((~as_uint (sModerateMask)) & as_uint (FpExponPlus)) | (as_uint (sModerateMask) & as_uint (sN))));
        sLn2 = as_float (__internal_sacosh_la_data.sLn2);
        vr1 = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sN, sLn2, sP);
    }

    if ((vm) != 0)
    {
        float _vapi_arg1[1];
        float _vapi_res1[1];
        ((float *) _vapi_arg1)[0] = va1;
        ((float *) _vapi_res1)[0] = vr1;
        __internal_sacosh_la_cout (_vapi_arg1, _vapi_res1);
        vr1 = ((float *) _vapi_res1)[0];
    };
    r = vr1;;

    return r;

}

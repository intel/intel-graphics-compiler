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
    unsigned int AddB5;
    unsigned int RcpBitMask;
    unsigned int poly_coeff3;
    unsigned int poly_coeff2;
    unsigned int poly_coeff1;
    unsigned int poly_coeff0;
    unsigned int Half;
    unsigned int L2H;
    unsigned int L2L;
} __internal_satanh_la_data_avx512_t;
static __constant __internal_satanh_la_data_avx512_t __internal_satanh_la_data_avx512 = {
    {
     0x00000000u, 0x3cfc0000u, 0x3d780000u, 0x3db78000u, 0x3df10000u, 0x3e14c000u, 0x3e300000u, 0x3e4a8000u, 0x3e648000u, 0x3e7dc000u, 0x3e8b4000u,
     0x3e974000u, 0x3ea30000u, 0x3eae8000u, 0x3eb9c000u, 0x3ec4e000u, 0x3ecfa000u, 0x3eda2000u, 0x3ee48000u, 0x3eeea000u, 0x3ef8a000u, 0x3f013000u,
     0x3f05f000u, 0x3f0aa000u, 0x3f0f4000u, 0x3f13d000u, 0x3f184000u, 0x3f1ca000u, 0x3f20f000u, 0x3f252000u, 0x3f295000u, 0x3f2d7000u}
    , {
       0x00000000u, 0x3726c39eu, 0x38a30c01u, 0x37528ae5u, 0x38e0edc5u, 0xb8ab41f8u, 0xb7cf8f58u, 0x3896a73du, 0xb5838656u, 0x380c36afu, 0xb8235454u,
       0x3862bae1u, 0x38c5e10eu, 0x38dedfacu, 0x38ebfb5eu, 0xb8e63c9fu, 0xb85c1340u, 0x38777bcdu, 0xb6038656u, 0x37d40984u, 0xb8b85028u, 0xb8ad5a5au,
       0x3865c84au, 0x38c3d2f5u, 0x383ebce1u, 0xb8a1ed76u, 0xb7a332c4u, 0xb779654fu, 0xb8602f73u, 0x38f85db0u, 0x37b4996fu, 0xb8bfb3cau}

    , 0x3f800000u, 0x7fffffffu, 0x00020000u, 0xfffc0000u, 0xbe800810u, 0x3eaab11eu, 0xbf000000u, 0x3f800000u, 0x3f000000u, 0x3f317000u, 0x3805fdf4u
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
    unsigned int TinyRange;

    unsigned int sLn2;

    unsigned int sInfs[2];
    unsigned int sOnes[2];
    unsigned int sZeros[2];
} __internal_satanh_la_data_t;
static __constant __internal_satanh_la_data_t __internal_satanh_la_data = {

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
        0x41F80000u, 0xFFFFE000u, 0xFFFFF000u, 0xFFFF0000u, 0x30800000u, 0x0C000000u, 0x3f317218u, {0x7f800000u, 0xff800000u}

    , {0x3f800000u, 0xbf800000u}

    , {0x00000000u, 0x80000000u}

};

static __constant _iml_v2_sp_union_t _imlsAtanhHATab[3] = {

    0x3F800000,
    0x00000000,
    0x7F800000
};

#pragma float_control(push)
#pragma float_control(precise, on)
__attribute__((always_inline))
inline int __internal_satanh_la_cout (float *a, float *r)
{
    int nRet = 0;

    float absx;
    float fRes;
    float sp_a = (*a);

    absx = sp_a;
    (((_iml_v2_sp_union_t *) & absx)->hex[0] = (((_iml_v2_sp_union_t *) & absx)->hex[0] & 0x7FFFFFFF) | ((_iml_uint32_t) (0) << 31));

    if ((((((_iml_v2_sp_union_t *) & *a)->hex[0] >> 23) & 0xFF) != 0xFF))
    {
        if (((((_iml_v2_sp_union_t *) & (absx))->hex[0] ==
              ((__constant _iml_v2_sp_union_t *) & (((__constant float *) _imlsAtanhHATab)[0]))->hex[0]) ? 1 : 0))
        {
            fRes = (float) ((__constant float *) _imlsAtanhHATab)[2];
            if (sp_a < 0.0f)
                fRes = (0.0f - fRes);

            (*r) = fRes;
            nRet = 2;
            return nRet;
        }

        {
            (*r) = (float) (((__constant float *) _imlsAtanhHATab)[2] * ((__constant float *) _imlsAtanhHATab)[1]);
            nRet = 1;
            return nRet;
        }
    }
    else
    {
        if (((((_iml_v2_sp_union_t *) & (absx))->hex[0] ==
              ((__constant _iml_v2_sp_union_t *) & (((__constant float *) _imlsAtanhHATab)[2]))->hex[0]) ? 1 : 0))
        {
            (*r) = (float) (sp_a * ((__constant float *) _imlsAtanhHATab)[1]);
            nRet = 1;
            return nRet;
        }

        {
            (*r) = (float) ((*a) * (*a));
            return nRet;
        }
    }
}

#pragma float_control (pop)
float __ocl_svml_atanhf (float a)
{

    float va1;
    float vr1;
    unsigned int vm;

    float r;

    va1 = a;;

    {

        float SgnMask;
        unsigned int iSpecialMask;
        float sSpecialMask;
        float sTinyMask;
        float sD;
        float sE;
        float sH;
        float sHalf;
        float sInput;
        float sL;
        float sQHi;
        float sQLo;
        float sR;
        float sResult;
        float sSign;
        float sTmp1;
        float sTmp2;
        float sTmp3;
        float sTmp4;
        float sTopMask12;
        float sU;
        float sUHi;
        float sULo;
        float sUTmp;
        float sV;
        float sVHi;
        float sVLo;
        float sZ;
        float sTinyRes;
        float sTinyRange;
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

        One = as_float (__internal_satanh_la_data.sOne);
        SgnMask = as_float (__internal_satanh_la_data.SgnMask);
        sInput = as_float ((as_uint (va1) & as_uint (SgnMask)));

        sSpecialMask = as_float (((unsigned int) (-(signed int) (!(sInput < One)))));
        iSpecialMask = as_uint (sSpecialMask);
        vm = 0;
        vm = iSpecialMask;

        sTinyRange = as_float (__internal_satanh_la_data.TinyRange);
        sTinyMask = as_float (((unsigned int) (-(signed int) (sInput < sTinyRange))));
        sTinyRes = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (va1, va1, va1);

        sSign = as_float (__internal_satanh_la_data.sSign);
        sSign = as_float ((as_uint (va1) & as_uint (sSign)));

        sTinyRes = as_float ((as_uint (sTinyRes) | as_uint (sSign)));

        sV = (sInput + sInput);

        sU = (One - sInput);
        sUTmp = (One - sU);
        sUTmp = (sUTmp - sInput);

        sTopMask12 = as_float (__internal_satanh_la_data.sTopMask12);
        sZ = (1.0f / (sU));
        sR = as_float ((as_uint (sZ) & as_uint (sTopMask12)));

        sE = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (-(sR), sU, One);
        sE = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (-(sR), sUTmp, sE);

        sVHi = as_float ((as_uint (sV) & as_uint (sTopMask12)));
        sVLo = (sV - sVHi);

        sQHi = (sR * sVHi);
        sQLo = (sR * sVLo);

        sD = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sE, sE, sE);

        sTmp1 = (sD * sQHi);
        sTmp2 = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sD, sQLo, sQLo);
        sTmp3 = (sTmp1 + sTmp2);

        sH = (sQHi + sTmp3);
        sTmp4 = (sQHi - sH);
        sL = (sTmp4 + sTmp3);
        A = ((One > sH) ? One : sH);
        B = ((One < sH) ? One : sH);

        X = (A + B);
        Xl = (A - X);
        Xl = (Xl + B);
        Xl = (Xl + sL);

        iX = as_uint (X);

        iBrkValue = (__internal_satanh_la_data.iBrkValue);
        iOffExpoMask = (__internal_satanh_la_data.iOffExpoMask);
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
        sPoly[7] = as_float (__internal_satanh_la_data.sPoly[7]);
        sPoly[6] = as_float (__internal_satanh_la_data.sPoly[6]);
        sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sPoly[7], sR, sPoly[6]);
        sPoly[5] = as_float (__internal_satanh_la_data.sPoly[5]);
        sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sP, sR, sPoly[5]);
        sPoly[4] = as_float (__internal_satanh_la_data.sPoly[4]);
        sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sP, sR, sPoly[4]);
        sPoly[3] = as_float (__internal_satanh_la_data.sPoly[3]);
        sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sP, sR, sPoly[3]);
        sPoly[2] = as_float (__internal_satanh_la_data.sPoly[2]);
        sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sP, sR, sPoly[2]);
        sPoly[1] = as_float (__internal_satanh_la_data.sPoly[1]);
        sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sP, sR, sPoly[1]);
        sPoly[0] = as_float (__internal_satanh_la_data.sPoly[0]);
        sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sP, sR, sPoly[0]);
        sP = (sP * sR);
        sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sP, sR, sR);
        sLn2 = as_float (__internal_satanh_la_data.sLn2);
        sResult = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sN, sLn2, sP);

        sHalf = as_float (__internal_satanh_la_data.sHalf);
        sHalf = as_float ((as_uint (sHalf) ^ as_uint (sSign)));
        vr1 = (sHalf * sResult);
        vr1 = as_float ((((~as_uint (sTinyMask)) & as_uint (vr1)) | (as_uint (sTinyMask) & as_uint (sTinyRes))));
    }

    if ((vm) != 0)
    {
        float _vapi_arg1[1];
        float _vapi_res1[1];
        ((float *) _vapi_arg1)[0] = va1;
        ((float *) _vapi_res1)[0] = vr1;
        __internal_satanh_la_cout (_vapi_arg1, _vapi_res1);
        vr1 = ((float *) _vapi_res1)[0];
    };
    r = vr1;;

    return r;

}

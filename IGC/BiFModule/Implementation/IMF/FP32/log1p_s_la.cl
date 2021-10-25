/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF
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

    unsigned int sLn2;

    unsigned int sInfs[2];
    unsigned int sOnes[2];
    unsigned int sZeros[2];
} __internal_slog1p_la_data_t;
static __constant __internal_slog1p_la_data_t __internal_slog1p_la_data = {

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

    , 0x01000000u, 0x01800000u, 0x3f2aaaabu, 0x007fffffu, 0x3f317218u, {0x7f800000u, 0xff800000u}

    , {0x3f800000u, 0xbf800000u}

    , {0x00000000u, 0x80000000u}

};

static __constant _iml_v2_sp_union_t __slog1p_la_CoutTab[210] = {
    0x3F800000,
    0x00000000,
    0x00000000,
    0x3F7C0000,
    0x3C810000,
    0x35ACB127,
    0x3F780000,
    0x3D020000,
    0x372EC4F4,
    0x3F740000,
    0x3D44C000,
    0xB7D57AD8,
    0x3F700000,
    0x3D843000,
    0xB6CE94C4,
    0x3F6E0000,
    0x3D955000,
    0x349488E3,
    0x3F6A0000,
    0x3DB80000,
    0x37530AEB,
    0x3F660000,
    0x3DDB5000,
    0x37488DAD,
    0x3F640000,
    0x3DED4000,
    0xB7589C7C,
    0x3F600000,
    0x3E08BC00,
    0x35E8227E,
    0x3F5E0000,
    0x3E11EC00,
    0xB5ED5B64,
    0x3F5A0000,
    0x3E248800,
    0x36F60CCF,
    0x3F580000,
    0x3E2DFC00,
    0xB6FE52AF,
    0x3F540000,
    0x3E412000,
    0xB6FA6AB9,
    0x3F520000,
    0x3E4AD400,
    0xB6948C24,
    0x3F500000,
    0x3E54A000,
    0xB6161BA9,
    0x3F4C0000,
    0x3E688000,
    0x36DFC995,
    0x3F4A0000,
    0x3E729800,
    0x35EFFE71,
    0x3F480000,
    0x3E7CC800,
    0x3663659E,
    0x3F460000,
    0x3E838A00,
    0xB5F3F655,
    0x3F440000,
    0x3E88BC00,
    0x3668227E,
    0x3F400000,
    0x3E934B00,
    0x35044D37,
    0x3F3E0000,
    0x3E98A800,
    0xB661E2CA,
    0x3F3C0000,
    0x3E9E1300,
    0xB6588CCD,
    0x3F3A0000,
    0x3EA38C00,
    0x365C271C,
    0x3F380000,
    0x3EA91500,
    0x3660738A,
    0x3F360000,
    0x3EAEAE00,
    0xB50829A8,
    0x3F340000,
    0x3EB45600,
    0x3603E9C7,
    0x3F320000,
    0x3EBA0F00,
    0xB5F12731,
    0x3F300000,
    0x3EBFD800,
    0xB5B884FD,
    0x3F2E0000,
    0x3EC5B200,
    0xB5CAEE9A,
    0x3F2C0000,
    0x3ECB9D00,
    0x3550C4D6,
    0x3F2A0000,
    0x3ED19A00,
    0x3580449F,
    0x3F280000,
    0x3ED7A900,
    0x3615248D,
    0x3F280000,
    0x3ED7A900,
    0x3615248D,
    0x3F260000,
    0x3EDDCB00,
    0x348DC071,
    0x3F240000,
    0x3EE40000,
    0xB5C71755,
    0x3F220000,
    0x3EEA4800,
    0x3511B7BF,
    0x3F200000,
    0x3EF0A400,
    0x3621A272,
    0x3F200000,
    0x3EF0A400,
    0x3621A272,
    0x3F1E0000,
    0x3EF71500,
    0x34AB5A0A,
    0x3F1C0000,
    0x3EFD9B00,
    0xB5EA10B7,
    0x3F1A0000,
    0x3F021B00,
    0x34BE7604,
    0x3F1A0000,
    0x3F021B00,
    0x34BE7604,
    0x3F180000,
    0x3F0573C0,
    0xB50E97D6,
    0x3F160000,
    0x3F08D7C0,
    0x338F1D6B,
    0x3F140000,
    0x3F0C4780,
    0xB55F86E2,
    0x3F140000,
    0x3F0C4780,
    0xB55F86E2,
    0x3F120000,
    0x3F0FC300,
    0x35D7F186,
    0x3F100000,
    0x3F134B00,
    0x35844D37,
    0x3F100000,
    0x3F134B00,
    0x35844D37,
    0x3F0E0000,
    0x3F16DFC0,
    0xB5AA13C8,
    0x3F0E0000,
    0x3F16DFC0,
    0xB5AA13C8,
    0x3F0C0000,
    0x3F1A8140,
    0x34AD9D8D,
    0x3F0A0000,
    0x3F1E3040,
    0x32C36BFB,
    0x3F0A0000,
    0x3F1E3040,
    0x32C36BFB,
    0x3F080000,
    0x3F21ED00,
    0xB2D06DC4,
    0x3F080000,
    0x3F21ED00,
    0xB2D06DC4,
    0x3F060000,
    0x3F25B800,
    0xB5A41A3D,
    0x3F060000,
    0x3F25B800,
    0xB5A41A3D,
    0x3F040000,
    0x3F299180,
    0xB56CBCC4,
    0x3F040000,
    0x3F299180,
    0xB56CBCC4,
    0x3F020000,
    0x3F2D7A00,
    0x34386C94,
    0x3F020000,
    0x3F2D7A00,
    0x34386C94,
    0x3F000000,
    0x3F317200,
    0x35BFBE8E,

    0x3F317200,
    0x35BFBE8E,

    0x48000040,

    0x46000000,

    0x3C200000,

    0x53800000,

    0x00000000,
    0x3F800000,

    0xBF000000,
    0x3EAAAAAB,
    0xBE800000,
    0x3E4CCCCD,
    0xBE2AAAAB,
    0x3E124E01,
    0xBE0005A0,
};

__attribute__((always_inline))
inline int __internal_slog1p_la_cout (float *a, float *r)
{
    float fap1, *ap1 = (&fap1);
    float x, y, u;
    float fP;
    float fAbsU;
    float fN, fNLn2Hi, fNLn2Lo;
    float fRcprY, fLnRcprYHi, fLnRcprYLo, fWHi, fWLo;
    float fYHi, fYLo, fUHi, fULo, fResHi, fResLo;
    float fTmp;
    int iN, j;
    int i;
    int nRet = 0;
    int isDenorm = 0;

    *ap1 = (*a) + 1.0f;

    if ((((((_iml_v2_sp_union_t *) & ap1[0])->hex[0] >> 23) & 0xFF) != 0xFF))
    {

        x = ap1[0];
        iN = 0;

        if (((((_iml_v2_sp_union_t *) & x)->hex[0] >> 23) & 0xFF) == 0)
        {

            isDenorm = 1;

            x *= ((__constant float *) __slog1p_la_CoutTab)[200];
            iN -= 40;
        }

        if (x > ((__constant float *) __slog1p_la_CoutTab)[201])
        {

            u = x - 1.0f;
            fAbsU = u;
            (((_iml_v2_sp_union_t *) & fAbsU)->hex[0] = (((_iml_v2_sp_union_t *) & fAbsU)->hex[0] & 0x7FFFFFFF) | ((_iml_uint32_t) (0) << 31));

            if (fAbsU > ((__constant float *) __slog1p_la_CoutTab)[199])
            {

                iN += ((((_iml_v2_sp_union_t *) & x)->hex[0] >> 23) & 0xFF) - 0x7F;
                fN = (float) iN;

                if (isDenorm == 1)
                {
                    fNLn2Hi = (fN * (((__constant float *) __slog1p_la_CoutTab)[195] + ((__constant float *) __slog1p_la_CoutTab)[196]));
                    fNLn2Lo = 0.0f;
                }
                else
                {
                    fNLn2Hi = (fN * ((__constant float *) __slog1p_la_CoutTab)[195]);
                    fNLn2Lo = (fN * ((__constant float *) __slog1p_la_CoutTab)[196]);
                }

                y = x;
                (((_iml_v2_sp_union_t *) & y)->hex[0] = (((_iml_v2_sp_union_t *) & y)->hex[0] & 0x807FFFFF) | (((_iml_uint32_t) (0x7F) & 0xFF) << 23));

                fTmp = (y + ((__constant float *) __slog1p_la_CoutTab)[197]);
                j = (((_iml_v2_sp_union_t *) & fTmp)->hex[0] & 0x007FFFFF) & ((1 << (6 + 1)) - 1);

                fRcprY = ((__constant float *) __slog1p_la_CoutTab)[3 * (j)];
                fLnRcprYHi = ((__constant float *) __slog1p_la_CoutTab)[3 * (j) + 1];
                fLnRcprYLo = ((__constant float *) __slog1p_la_CoutTab)[3 * (j) + 2];

                fWHi = (fNLn2Hi + fLnRcprYHi);
                fTmp = (fWHi - fNLn2Hi);
                fTmp = (fLnRcprYHi - fTmp);
                fWLo = (fNLn2Lo + fLnRcprYLo);
                fWLo = (fTmp + fWLo);;

                fTmp = (y + ((__constant float *) __slog1p_la_CoutTab)[198]);
                fYHi = (fTmp - ((__constant float *) __slog1p_la_CoutTab)[198]);
                fYLo = (y - fYHi);

                fUHi = (fRcprY * fYHi - 1.0f);
                fULo = (fRcprY * fYLo);

                u = (fUHi + fULo);

                fP = ((((((((__constant float *) __slog1p_la_CoutTab)[209] * u + ((__constant float *) __slog1p_la_CoutTab)[208]) * u +
                          ((__constant float *) __slog1p_la_CoutTab)[207]) * u + ((__constant float *) __slog1p_la_CoutTab)[206]) * u +
                        ((__constant float *) __slog1p_la_CoutTab)[205]) * u + ((__constant float *) __slog1p_la_CoutTab)[204]) * u +
                      ((__constant float *) __slog1p_la_CoutTab)[203]);
                fP = (fP * u * u);

                fResHi = (fWHi + fUHi);

                fResLo = (fWLo + fULo);
                fTmp = (fResLo - fWLo);
                fTmp = (fULo - fTmp);
                fP = (fTmp + fP);

                r[0] = (fResHi + fResLo);
                fTmp = (r[0] - fResHi);
                fTmp = (fResLo - fTmp);
                fTmp = (fTmp + fP);
                r[0] = (r[0] + fTmp);
            }
            else
            {

                fP = ((((((((__constant float *) __slog1p_la_CoutTab)[209] * u + ((__constant float *) __slog1p_la_CoutTab)[208]) * u +
                          ((__constant float *) __slog1p_la_CoutTab)[207]) * u + ((__constant float *) __slog1p_la_CoutTab)[206]) * u +
                        ((__constant float *) __slog1p_la_CoutTab)[205]) * u + ((__constant float *) __slog1p_la_CoutTab)[204]) * u +
                      ((__constant float *) __slog1p_la_CoutTab)[203]);

                fP = (fP * u * u);
                fP = (fP + u);

                r[0] = fP;
            }
        }
        else
        {

            if (x == ((__constant float *) __slog1p_la_CoutTab)[201])
            {

                r[0] = -((__constant float *) __slog1p_la_CoutTab)[202] / ((__constant float *) __slog1p_la_CoutTab)[201];
                nRet = 2;
            }
            else
            {

                r[0] = ((__constant float *) __slog1p_la_CoutTab)[201] / ((__constant float *) __slog1p_la_CoutTab)[201];
                nRet = 1;
            }
        }
    }
    else
    {

        if (((((_iml_v2_sp_union_t *) & ap1[0])->hex[0] >> 31) == 1) && ((((_iml_v2_sp_union_t *) & ap1[0])->hex[0] & 0x007FFFFF) == 0))
        {

            r[0] = ((__constant float *) __slog1p_la_CoutTab)[201] / ((__constant float *) __slog1p_la_CoutTab)[201];
            nRet = 1;
        }
        else
        {

            r[0] = (ap1[0] * ap1[0]);
        }
    }

    return nRet;
}

float __ocl_svml_log1pf (float a)
{

    float va1;
    float vr1;
    unsigned int vm;

    float r;

    va1 = a;;

    {

        float SignMask;
        float SgnMask;
        unsigned int iHiDelta;
        unsigned int iLoRange;
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
        float NaNMask;
        unsigned int iNaNMask;

        float sLn2;

        float sPoly[8];

        unsigned int iRangeMask;

        unsigned int iX;
        unsigned int iXTest;
        float sN;
        unsigned int iN;
        float sR;
        unsigned int iR;
        float sP;
        unsigned int iExp;

        One = as_float (__internal_slog1p_la_data.sOne);
        SgnMask = as_float (__internal_slog1p_la_data.SgnMask);

        SignMask = as_float ((~(as_uint (SgnMask)) & as_uint (va1)));

        NaNMask = as_float (((unsigned int) (-(signed int) (!(va1 == va1)))));
        iNaNMask = as_uint (NaNMask);

        A = ((One > va1) ? One : va1);
        B = ((One < va1) ? One : va1);

        X = (A + B);
        Xl = (A - X);
        Xl = (Xl + B);

        iHiDelta = (__internal_slog1p_la_data.iHiDelta);
        iLoRange = (__internal_slog1p_la_data.iLoRange);
        iX = as_uint (X);
        iXTest = (iX + iHiDelta);
        iRangeMask = ((unsigned int) (-(signed int) ((signed int) iXTest < (signed int) iLoRange)));

        iBrkValue = (__internal_slog1p_la_data.iBrkValue);
        iOffExpoMask = (__internal_slog1p_la_data.iOffExpoMask);
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

        iRangeMask = (iRangeMask | iNaNMask);

        vm = 0;
        vm = iRangeMask;

        Rh = (sR - One);
        sR = (Rh + Rl);
        sPoly[7] = as_float (__internal_slog1p_la_data.sPoly[7]);
        sPoly[6] = as_float (__internal_slog1p_la_data.sPoly[6]);
        sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sPoly[7], sR, sPoly[6]);
        sPoly[5] = as_float (__internal_slog1p_la_data.sPoly[5]);
        sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sP, sR, sPoly[5]);
        sPoly[4] = as_float (__internal_slog1p_la_data.sPoly[4]);
        sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sP, sR, sPoly[4]);
        sPoly[3] = as_float (__internal_slog1p_la_data.sPoly[3]);
        sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sP, sR, sPoly[3]);
        sPoly[2] = as_float (__internal_slog1p_la_data.sPoly[2]);
        sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sP, sR, sPoly[2]);
        sPoly[1] = as_float (__internal_slog1p_la_data.sPoly[1]);
        sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sP, sR, sPoly[1]);
        sPoly[0] = as_float (__internal_slog1p_la_data.sPoly[0]);
        sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sP, sR, sPoly[0]);
        sP = (sP * sR);
        sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sP, sR, sR);
        sLn2 = as_float (__internal_slog1p_la_data.sLn2);
        vr1 = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sN, sLn2, sP);

        vr1 = as_float ((as_uint (vr1) | as_uint (SignMask)));
    }

    if ((vm) != 0)
    {
        float _vapi_arg1[1];
        float _vapi_res1[1];
        ((float *) _vapi_arg1)[0] = va1;
        ((float *) _vapi_res1)[0] = vr1;
        __internal_slog1p_la_cout (_vapi_arg1, _vapi_res1);
        vr1 = ((float *) _vapi_res1)[0];
    };
    r = vr1;;

    return r;

}

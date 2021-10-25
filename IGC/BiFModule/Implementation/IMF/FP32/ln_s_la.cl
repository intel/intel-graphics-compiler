/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF
typedef struct
{
    unsigned int One;
    unsigned int coeff4[16];
    unsigned int coeff3[16];
    unsigned int coeff2[16];
    unsigned int coeff1[16];
    unsigned int L2;
    unsigned int MinNorm;
    unsigned int MaxNorm;
} __internal_sln_la_data_avx512_t;
static __constant __internal_sln_la_data_avx512_t __internal_sln_la_data_avx512 = {

    0x3f800000u, {
                  0xbe6831efu, 0xbe40408eu, 0xbe20ce25u, 0xbe07bb45u,
                  0xbde70571u, 0xbdc611f1u, 0xbdaaf886u, 0xbd947e78u,
                  0xbf15ee8du, 0xbf040bd7u, 0xbee9b172u, 0xbecfb3f7u,
                  0xbeb95babu, 0xbea60c13u, 0xbe9546fbu, 0xbe86a5e0u,
                  }

    , {
       0x3eaa36bbu, 0x3ea68b39u, 0x3ea0b403u, 0x3e99b179u,
       0x3e922487u, 0x3e8a71e0u, 0x3e82d7c6u, 0x3e76f776u,
       0x3e4684feu, 0x3e758f3du, 0x3e8bea95u, 0x3e982131u,
       0x3ea088c4u, 0x3ea5fc8du, 0x3ea9260bu, 0x3eaa88fau,
       }

    , {
       0xbefffe93u, 0xbeffc28bu, 0xbeff0773u, 0xbefdb76bu,
       0xbefbd4fbu, 0xbef96e20u, 0xbef69561u, 0xbef35e29u,
       0xbf060f28u, 0xbf037ac6u, 0xbf01de22u, 0xbf00e8dau,
       0xbf0061a0u, 0xbf001fa4u, 0xbf0005f1u, 0xbf00001bu,
       }

    , {
       0x3f800000u, 0x3f7fff55u, 0x3f7ffb64u, 0x3f7ff0dbu,
       0x3f7fdcbcu, 0x3f7fbcb5u, 0x3f7f8f2cu, 0x3f7f532cu,
       0x3f7fa089u, 0x3f7fd0dcu, 0x3f7feac4u, 0x3f7ff79du,
       0x3f7ffd4cu, 0x3f7fff62u, 0x3f7fffefu, 0x3f800000u,
       }
    , 0x3f317218u, 0x00800000u, 0x7f7fffffu
};

typedef struct
{
    unsigned int sPoly[8];
    unsigned int iHiDelta;
    unsigned int iLoRange;
    unsigned int iBrkValue;
    unsigned int iOffExpoMask;
    unsigned int sOne;

    unsigned int sLn2;

    unsigned int sInfs[2];
    unsigned int sOnes[2];
    unsigned int sZeros[2];
} __internal_sln_la_data_t;
static __constant __internal_sln_la_data_t __internal_sln_la_data = {

    {
     0xbf000000u,
     0x3eaaaa94u,
     0xbe80058eu,
     0x3e4ce190u,
     0xbe28ad37u,
     0x3e0fcb12u,
     0xbe1ad9e3u,
     0x3e0d84edu,
     },

    0x00800000u,

    0x01000000u,

    0x3f2aaaabu,

    0x007fffffu,

    0x3f800000u,

    0x3f317218u,

    {0x7f800000u, 0xff800000u},

    {0x3f800000u, 0xbf800000u},

    {0x00000000u, 0x80000000u}

};

static __constant _iml_v2_sp_union_t __sln_la_CoutTab[210] = {
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

    0x4D000000,

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
inline int __internal_sln_la_cout (float *a, float *r)
{
    float fap1, *ap1 = &fap1;
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

    if ((((((_iml_v2_sp_union_t *) & a[0])->hex[0] >> 23) & 0xFF) != 0xFF))
    {

        x = a[0];
        iN = 0;

        if (((((_iml_v2_sp_union_t *) & x)->hex[0] >> 23) & 0xFF) == 0)
        {

            x = (x * ((__constant float *) __sln_la_CoutTab)[200]);
            iN -= (27);
        }

        if (x > ((__constant float *) __sln_la_CoutTab)[201])
        {

            u = x - 1.0f;
            fAbsU = u;
            (((_iml_v2_sp_union_t *) & fAbsU)->hex[0] = (((_iml_v2_sp_union_t *) & fAbsU)->hex[0] & 0x7FFFFFFF) | ((_iml_uint32_t) (0) << 31));

            if (fAbsU > ((__constant float *) __sln_la_CoutTab)[199])
            {

                iN += ((((_iml_v2_sp_union_t *) & x)->hex[0] >> 23) & 0xFF) - 0x7F;
                fN = (float) iN;

                if (isDenorm == 1)
                {
                    fNLn2Hi = (fN * (((__constant float *) __sln_la_CoutTab)[195] + ((__constant float *) __sln_la_CoutTab)[196]));
                    fNLn2Lo = 0.0f;
                }
                else
                {
                    fNLn2Hi = (fN * ((__constant float *) __sln_la_CoutTab)[195]);
                    fNLn2Lo = (fN * ((__constant float *) __sln_la_CoutTab)[196]);
                }

                y = x;
                (((_iml_v2_sp_union_t *) & y)->hex[0] = (((_iml_v2_sp_union_t *) & y)->hex[0] & 0x807FFFFF) | (((_iml_uint32_t) (0x7F) & 0xFF) << 23));

                fTmp = (y + ((__constant float *) __sln_la_CoutTab)[197]);
                j = (((_iml_v2_sp_union_t *) & fTmp)->hex[0] & 0x007FFFFF) & ((1 << (6 + 1)) - 1);

                fRcprY = ((__constant float *) __sln_la_CoutTab)[3 * (j)];

                fLnRcprYHi = ((__constant float *) __sln_la_CoutTab)[3 * (j) + 1];
                fLnRcprYLo = ((__constant float *) __sln_la_CoutTab)[3 * (j) + 2];

                fWHi = (fNLn2Hi + fLnRcprYHi);
                fWLo = (fNLn2Lo + fLnRcprYLo);

                fTmp = (y + ((__constant float *) __sln_la_CoutTab)[198]);
                fYHi = (fTmp - ((__constant float *) __sln_la_CoutTab)[198]);
                fYLo = (y - fYHi);

                fUHi = (fRcprY * fYHi - 1.0f);
                fULo = (fRcprY * fYLo);

                u = (fUHi + fULo);

                fP = ((((((((__constant float *) __sln_la_CoutTab)[209] * u + ((__constant float *) __sln_la_CoutTab)[208]) * u +
                          ((__constant float *) __sln_la_CoutTab)[207]) * u + ((__constant float *) __sln_la_CoutTab)[206]) * u +
                        ((__constant float *) __sln_la_CoutTab)[205]) * u + ((__constant float *) __sln_la_CoutTab)[204]) * u +
                      ((__constant float *) __sln_la_CoutTab)[203]);
                fP = (fP * u * u);

                fResHi = (fWHi + fUHi);
                fResLo = ((fWLo + fULo) + fP);

                r[0] = (fResHi + fResLo);
            }
            else
            {

                fP = ((((((((__constant float *) __sln_la_CoutTab)[209] * u + ((__constant float *) __sln_la_CoutTab)[208]) * u +
                          ((__constant float *) __sln_la_CoutTab)[207]) * u + ((__constant float *) __sln_la_CoutTab)[206]) * u +
                        ((__constant float *) __sln_la_CoutTab)[205]) * u + ((__constant float *) __sln_la_CoutTab)[204]) * u +
                      ((__constant float *) __sln_la_CoutTab)[203]);

                fP = (fP * u * u);
                fP = (fP + u);

                r[0] = fP;
            }
        }
        else
        {

            if (x == ((__constant float *) __sln_la_CoutTab)[201])
            {

                r[0] = -((__constant float *) __sln_la_CoutTab)[202] / ((__constant float *) __sln_la_CoutTab)[201];
                nRet = 2;
            }
            else
            {

                r[0] = ((__constant float *) __sln_la_CoutTab)[201] / ((__constant float *) __sln_la_CoutTab)[201];
                nRet = 1;
            }
        }
    }
    else
    {

        if (((((_iml_v2_sp_union_t *) & a[0])->hex[0] >> 31) == 1) && ((((_iml_v2_sp_union_t *) & a[0])->hex[0] & 0x007FFFFF) == 0))
        {

            r[0] = ((__constant float *) __sln_la_CoutTab)[201] / ((__constant float *) __sln_la_CoutTab)[201];
            nRet = 1;
        }
        else
        {

            r[0] = (a[0] * a[0]);
        }
    }

    return nRet;
}

float __ocl_svml_logf (float a)
{

    float va1;
    float vr1;
    unsigned int vm;

    float r;

    va1 = a;;

    {

        unsigned int iHiDelta;
        unsigned int iLoRange;
        unsigned int iBrkValue;
        unsigned int iOffExpoMask;
        float sOne;

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

        iHiDelta = (__internal_sln_la_data.iHiDelta);
        iLoRange = (__internal_sln_la_data.iLoRange);
        iX = as_uint (va1);

        iXTest = (iX + iHiDelta);
        iRangeMask = ((unsigned int) (-(signed int) ((signed int) iXTest < (signed int) iLoRange)));

        iBrkValue = (__internal_sln_la_data.iBrkValue);
        iOffExpoMask = (__internal_sln_la_data.iOffExpoMask);

        iX = (iX - iBrkValue);
        iR = (iX & iOffExpoMask);

        iN = ((signed int) iX >> (23));

        iR = (iR + iBrkValue);
        sN = ((float) ((int) (iN)));
        sR = as_float (iR);

        vm = 0;
        vm = iRangeMask;

        sOne = as_float (__internal_sln_la_data.sOne);

        sR = (sR - sOne);
        sPoly[7] = as_float (__internal_sln_la_data.sPoly[7]);
        sPoly[6] = as_float (__internal_sln_la_data.sPoly[6]);

        sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sPoly[7], sR, sPoly[6]);
        sPoly[5] = as_float (__internal_sln_la_data.sPoly[5]);
        sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sP, sR, sPoly[5]);
        sPoly[4] = as_float (__internal_sln_la_data.sPoly[4]);
        sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sP, sR, sPoly[4]);
        sPoly[3] = as_float (__internal_sln_la_data.sPoly[3]);
        sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sP, sR, sPoly[3]);
        sPoly[2] = as_float (__internal_sln_la_data.sPoly[2]);
        sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sP, sR, sPoly[2]);
        sPoly[1] = as_float (__internal_sln_la_data.sPoly[1]);
        sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sP, sR, sPoly[1]);
        sPoly[0] = as_float (__internal_sln_la_data.sPoly[0]);
        sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sP, sR, sPoly[0]);
        sP = (sP * sR);

        sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sP, sR, sR);
        sLn2 = as_float (__internal_sln_la_data.sLn2);

        vr1 = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sN, sLn2, sP);
    }

    if ((vm) != 0)
    {
        float _vapi_arg1[1];
        float _vapi_res1[1];
        ((float *) _vapi_arg1)[0] = va1;
        ((float *) _vapi_res1)[0] = vr1;
        __internal_sln_la_cout (_vapi_arg1, _vapi_res1);
        vr1 = ((float *) _vapi_res1)[0];
    };
    r = vr1;;

    return r;

}

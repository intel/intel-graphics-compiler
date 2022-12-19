/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF
typedef struct
{
    unsigned long D_PI;
    unsigned long D_PIO2;
    unsigned long AT_D_A21;
    unsigned long AT_D_A20;
    unsigned long AT_D_A19;
    unsigned long AT_D_A18;
    unsigned long AT_D_A17;
    unsigned long AT_D_A16;
    unsigned long AT_D_A15;
    unsigned long AT_D_A14;
    unsigned long AT_D_A13;
    unsigned long AT_D_A12;
    unsigned long AT_D_A11;
    unsigned long AT_D_A10;
    unsigned long AT_D_A09;
    unsigned long AT_D_A08;
    unsigned long AT_D_A07;
    unsigned long AT_D_A06;
    unsigned long AT_D_A05;
    unsigned long AT_D_A04;
    unsigned long AT_D_A03;
    unsigned long AT_D_A02;
    unsigned long AT_D_A01;
    unsigned long AT_D_A00;
    unsigned long D_SIGN_MASK;
    unsigned int I_CHK_WORK_SUB;
    unsigned int I_CHK_WORK_CMP;
    unsigned long D_ABSMASK;
    unsigned long D_ZERO;

    unsigned int I_D_EXP_MASK;
    unsigned int I_D_2_BIAS;
    unsigned int I_S_MANTISSA_MASK;
    unsigned int I_S_ONE;
    unsigned int I_D_BIAS;
    unsigned long D_HIGH_20_MASK;
    unsigned long D_ONE;
    unsigned long D_NONE;
} __internal_datanpi_la_data_t;
static __constant __internal_datanpi_la_data_t __internal_datanpi_la_data = {
    0x3ff0000000000000uL,
    0x3fe0000000000000uL,

    0xBEC435D571D7FB69uL,
    0x3EFEE969B7DB1149uL,
    0xBF26798A20DC9977uL,
    0x3F44C3C0C907686FuL,
    0xBF5B8DDB0F1CD2C5uL,
    0x3F6C3A72776AC84DuL,
    0xBF778AC56B6FECF2uL,
    0x3F80BAD5CF20A9EDuL,
    0xBF85278CBEF8B02EuL,
    0x3F88CA8EE19B9723uL,
    0xBF8BE48C140DF0C8uL,
    0x3F8EEA5306D0465FuL,
    0xBF9124120821BA9DuL,
    0x3F932BC4F824F7C6uL,
    0xBF95BAC83A460E3EuL,
    0x3F9912AF904B4DD1uL,
    0xBF9DA1BAA68634ECuL,
    0x3FA21BB94433ED0AuL,
    0xBFA7483758DF91CFuL,
    0x3FB04C26BE3AF8D5uL,
    0xBFBB2995E7B7B5EDuL,
    0x3FD45F306DC9C883uL,

    0x8000000000000000uL,
    0x80300000u,
    0xfdd00000u,
    0x7fffffffffffffffuL,
    0x0000000000000000uL,

    0xfff00000u,
    0x7fe00000u,
    0x007fffffu,
    0x3f800000u,
    0x07f00000u,
    0xffffffff00000000uL,
    0x3ff0000000000000uL,
    0xBff0000000000000uL,
};

static __constant _iml_v2_dp_union_t __datanpi_la_CoutTab[247] = {

    0xE8000000, 0x3FC3D6EE,
    0x8B0D1D86, 0x3DF8CC4D,
    0x50000000, 0x3FCB90D7,
    0x1022F622, 0x3E149305,
    0x38000000, 0x3FD36277,
    0x8658E951, 0xBE0F0286,
    0xC0000000, 0x3FDA64EE,
    0xE5B6427D, 0x3E2E611F,
    0xA8000000, 0x3FE1E00B,
    0x9F9B5C83, 0x3E3EF7F5,
    0xC8000000, 0x3FE700A7,
    0x618C34D2, 0xBE343DCE,
    0x58000000, 0x3FECAC7C,
    0x6F251EC5, 0xBE0EE418,
    0x30000000, 0x3FF0D38F,
    0x0B7A1B84, 0xBE4D22FB,
    0x78000000, 0x3FF30B6D,
    0x8589532C, 0x3E36A4DA,
    0x00000000, 0x3FF4AE11,
    0xDA7607D1, 0xBE4CD3B2,
    0x18000000, 0x3FF5F973,
    0x4931151A, 0xBE46D5BD,
    0x78000000, 0x3FF6DCC5,
    0xE5AC6F37, 0x3E4DAB2F,
    0x30000000, 0x3FF789BD,
    0x63E8AA08, 0xBE4F4FFD,
    0x08000000, 0x3FF7FDE8,
    0xFFBFDCA2, 0x3E1C30A7,
    0x28000000, 0x3FF8555A,
    0x3B43DC6C, 0xBE1E19F8,
    0x18000000, 0x3FF88FC2,
    0x6FF107D4, 0x3E259D3B,
    0x60000000, 0x3FF8BB9A,
    0x27F43144, 0x3E4B8C7A,
    0xC0000000, 0x3FF8D8D8,
    0x2931C287, 0xBE2359D2,
    0xD0000000, 0x3FF8EEC8,
    0x607FD6F0, 0xBE07FCCD,
    0x48000000, 0x3FF8FD69,
    0x8310236B, 0x3E4679B5,
    0xD0000000, 0x3FF90861,
    0x9CFF61AF, 0x3E205B36,
    0x38000000, 0x3FF90FB2,
    0x3F24AB6A, 0xBE160576,
    0x88000000, 0x3FF9152E,
    0xBC43465D, 0x3E419361,
    0xC0000000, 0x3FF918D6,
    0x1114E411, 0x3E47CE4F,
    0xF0000000, 0x3FF91B94,
    0x773951DF, 0xBE3CAD7B,
    0x08000000, 0x3FF91D69,
    0xFAA44CE6, 0x3E49FB97,
    0x20000000, 0x3FF91EC8,
    0x8B23FFA5, 0x3E31BE61,
    0x30000000, 0x3FF91FB2,
    0x87FC4A01, 0xBE128841,
    0xB8000000, 0x3FF92061,
    0x29392773, 0x3E455F88,
    0xC0000000, 0x3FF920D6,
    0x6C4176AE, 0x3E3FB87E,
    0x88000000, 0x3FF9212E,
    0x76D7A426, 0xBE210E80,
    0x08000000, 0x3FF92169,
    0xD9D720C0, 0x3E48FD55,
    0xF0000000, 0x3FF92194,
    0xC02EEAC5, 0xBE41119E,
    0x30000000, 0x3FF921B2,
    0xB22521F8, 0xBE138683,
    0x20000000, 0x3FF921C8,
    0x3D6A380D, 0x3E310FA9,
    0xC0000000, 0x3FF921D6,
    0x5ADD7895, 0x3E3FB08C,
    0xB8000000, 0x3FF921E1,
    0xA42B3618, 0x3E45549C,
    0x08000000, 0x3FF921E9,
    0xB8C09601, 0x3E48FCD6,
    0x88000000, 0x3FF921EE,
    0x3964E770, 0xBE2113F6,
    0x30000000, 0x3FF921F2,
    0xD33BBF6E, 0xBE138702,
    0xF0000000, 0x3FF921F4,
    0x6E436174, 0xBE4111CA,
    0xC0000000, 0x3FF921F6,
    0x61D4C384, 0x3E3FB088,
    0x20000000, 0x3FF921F8,
    0x51E51998, 0x3E310F9E,
    0x08000000, 0x3FF921F9,
    0x79300AAF, 0x3E48FCD6,
    0xB8000000, 0x3FF921F9,
    0xF572E42D, 0x3E45549B,
    0x30000000, 0x3FF921FA,
    0x12CC4AC0, 0xBE138703,
    0x88000000, 0x3FF921FA,
    0x90C11066, 0xBE2113F6,
    0xC0000000, 0x3FF921FA,
    0x5FD83F2A, 0x3E3FB088,
    0xF0000000, 0x3FF921FA,
    0x70FE42BC, 0xBE4111CA,
    0x08000000, 0x3FF921FB,
    0x79104269, 0x3E48FCD6,
    0x20000000, 0x3FF921FB,
    0x51366146, 0x3E310F9E,
    0x30000000, 0x3FF921FB,
    0x12EC1305, 0xBE138703,
    0x38000000, 0x3FF921FB,
    0xF567F8A8, 0x3E45549B,
    0x40000000, 0x3FF921FB,
    0x5FD740E8, 0x3E3FB088,
    0x48000000, 0x3FF921FB,
    0x90C68629, 0xBE2113F6,
    0x48000000, 0x3FF921FB,
    0x79103285, 0x3E48FCD6,
    0x50000000, 0x3FF921FB,
    0x70FE6E6A, 0xBE4111CA,
    0x50000000, 0x3FF921FB,
    0x12EC22EA, 0xBE138703,
    0x50000000, 0x3FF921FB,
    0x5136565A, 0x3E310F9E,
    0x50000000, 0x3FF921FB,
    0x5FD74068, 0x3E3FB088,
    0x50000000, 0x3FF921FB,
    0xF567F7FA, 0x3E45549B,
    0x50000000, 0x3FF921FB,
    0x7910327D, 0x3E48FCD6,
    0x50000000, 0x3FF921FB,
    0x5BCE5E60, 0x3E4BBB02,
    0x50000000, 0x3FF921FB,
    0x9DA27BA2, 0x3E4D8F1F,
    0x50000000, 0x3FF921FB,
    0x8F019193, 0x3E4EEE35,
    0x50000000, 0x3FF921FB,
    0x2FEBA034, 0x3E4FD844,
    0x58000000, 0x3FF921FB,
    0xD764D4D3, 0xBE4F7830,
    0x58000000, 0x3FF921FB,
    0x86EFCD83, 0xBE4F0329,
    0x58000000, 0x3FF921FB,
    0x0A980806, 0xBE4EAB64,
    0x58000000, 0x3FF921FB,
    0x625D845E, 0xBE4E70E0,
    0x58000000, 0x3FF921FB,
    0xA431A1A0, 0xBE4E44FD,
    0x58000000, 0x3FF921FB,
    0xD0145FCC, 0xBE4E27BB,
    0x58000000, 0x3FF921FB,
    0x70FE6E6D, 0xBE4E11CA,
    0x58000000, 0x3FF921FB,
    0x86EFCD83, 0xBE4E0329,
    0x58000000, 0x3FF921FB,
    0xD764D4D3, 0xBE4DF830,
    0x58000000, 0x3FF921FB,
    0x625D845E, 0xBE4DF0E0,
    0x58000000, 0x3FF921FB,
    0x0A980806, 0xBE4DEB64,
    0x58000000, 0x3FF921FB,
    0xD0145FCC, 0xBE4DE7BB,
    0x58000000, 0x3FF921FB,
    0xA431A1A0, 0xBE4DE4FD,
    0x58000000, 0x3FF921FB,
    0x86EFCD83, 0xBE4DE329,
    0x58000000, 0x3FF921FB,
    0x70FE6E6D, 0xBE4DE1CA,
    0x58000000, 0x3FF921FB,
    0x625D845E, 0xBE4DE0E0,
    0x58000000, 0x3FF921FB,
    0xD764D4D3, 0xBE4DE030,
    0x58000000, 0x3FF921FB,
    0xD0145FCC, 0xBE4DDFBB,
    0x58000000, 0x3FF921FB,
    0x0A980806, 0xBE4DDF64,
    0x58000000, 0x3FF921FB,
    0x86EFCD83, 0xBE4DDF29,
    0x58000000, 0x3FF921FB,
    0xA431A1A0, 0xBE4DDEFD,
    0x58000000, 0x3FF921FB,
    0x625D845E, 0xBE4DDEE0,
    0x58000000, 0x3FF921FB,
    0x70FE6E6D, 0xBE4DDECA,
    0x58000000, 0x3FF921FB,
    0xD0145FCC, 0xBE4DDEBB,
    0x58000000, 0x3FF921FB,
    0xD764D4D3, 0xBE4DDEB0,
    0x58000000, 0x3FF921FB,
    0x86EFCD83, 0xBE4DDEA9,
    0x58000000, 0x3FF921FB,
    0x0A980806, 0xBE4DDEA4,
    0x58000000, 0x3FF921FB,
    0x625D845E, 0xBE4DDEA0,
    0x58000000, 0x3FF921FB,
    0xA431A1A0, 0xBE4DDE9D,
    0x58000000, 0x3FF921FB,
    0xD0145FCC, 0xBE4DDE9B,
    0x58000000, 0x3FF921FB,
    0x70FE6E6D, 0xBE4DDE9A,
    0x58000000, 0x3FF921FB,
    0x86EFCD83, 0xBE4DDE99,
    0x58000000, 0x3FF921FB,
    0xD764D4D3, 0xBE4DDE98,
    0x58000000, 0x3FF921FB,
    0x625D845E, 0xBE4DDE98,
    0x58000000, 0x3FF921FB,
    0x0A980806, 0xBE4DDE98,
    0x58000000, 0x3FF921FB,
    0xD0145FCC, 0xBE4DDE97,
    0x58000000, 0x3FF921FB,
    0xA431A1A0, 0xBE4DDE97,
    0x58000000, 0x3FF921FB,
    0x86EFCD83, 0xBE4DDE97,
    0x58000000, 0x3FF921FB,
    0x70FE6E6D, 0xBE4DDE97,
    0x58000000, 0x3FF921FB,
    0x625D845E, 0xBE4DDE97,
    0x58000000, 0x3FF921FB,
    0x5764D4D3, 0xBE4DDE97,
    0x58000000, 0x3FF921FB,
    0x50145FCC, 0xBE4DDE97,
    0x58000000, 0x3FF921FB,
    0x4A980806, 0xBE4DDE97,
    0x58000000, 0x3FF921FB,
    0x46EFCD83, 0xBE4DDE97,
    0x58000000, 0x3FF921FB,
    0x4431A1A0, 0xBE4DDE97,
    0x58000000, 0x3FF921FB,
    0x425D845E, 0xBE4DDE97,
    0x58000000, 0x3FF921FB,
    0x40FE6E6D, 0xBE4DDE97,
    0x58000000, 0x3FF921FB,
    0x40145FCC, 0xBE4DDE97,

    0x55555555, 0xBFD55555,
    0x999991BB, 0x3FC99999,
    0x92382A27, 0xBFC24924,
    0xF9B157D3, 0x3FBC71C6,
    0x69CCA475, 0xBFB745BE,
    0x1048AFD1, 0x3FB3AB7C,
    0x34239994, 0xBFB029BD,

    0x00000000, 0x3CA00000,
    0x00000000, 0x3FC00000,
    0x00000000, 0x43500000,
    0x00000000, 0x3FF00000,
    0x54442D18, 0x3FF921FB,
    0x33145C07, 0x3C91A626,

    0x02000000, 0x41A00000,
    0x70000000, 0x3fd45f30,
    0xead603d9, 0xbe21b1bb,

    0x00000000, 0x45100000,
    0x00000000, 0x3AD00000,
    0x00000000, 0x00300000
};

__attribute__((always_inline))
inline int __internal_datanpi_la_cout (double *a, double *r)
{
    int nRet = 0;
    double dbX, dbB, dbU, dbTmp1, dbTmp2, dbUHi, dbULo, dbXHi, dbXLo, dbXB1Hi, dbXB1Lo,
        dbXBHi, dbXBLo, dbV1Hi, dbV2Lo, dbV1Lo, dbVHi, dbV3Lo, dbVLo, dbQHi,
        dbQLo, dbT1Hi, dbT1Lo, dbTHi, dbTLo, dbS, dbAtanPoly, dbAHi, dbALo, dbResHi, dbResMid, dbResLo, dbRes;
    double dbVTmp1, dbVTmp2, dbVTmp3;
    int i, iSign, iJ;

    if ((((((_iml_v2_dp_union_t *) & (*a))->dwords.hi_dword >> 20) & 0x7FF) != 0x7FF))
    {

        iSign = (((_iml_v2_dp_union_t *) & (*a))->dwords.hi_dword >> 31);

        dbX = (*a);
        (((_iml_v2_dp_union_t *) & dbX)->dwords.hi_dword = (((_iml_v2_dp_union_t *) & dbX)->dwords.hi_dword & 0x7FFFFFFF) | ((_iml_uint32_t) (0) << 31));

        if (dbX >= ((__constant double *) __datanpi_la_CoutTab)[236])
        {

            if (dbX < ((__constant double *) __datanpi_la_CoutTab)[237])
            {

                iJ = ((((_iml_v2_dp_union_t *) & (*a))->dwords.hi_dword >> 20) & 0x7FF) << 20;
                iJ = iJ | (((_iml_v2_dp_union_t *) & (*a))->dwords.hi_dword & 0x000FFFFF);
                iJ = iJ - 0x3FC00000;
                iJ = iJ >> 19;

                dbB = dbX;
                (((_iml_v2_dp_union_t *) & dbB)->dwords.lo_dword = (0));
                (((_iml_v2_dp_union_t *) & dbB)->dwords.hi_dword =
                 (((_iml_v2_dp_union_t *) & dbB)->dwords.
                  hi_dword & 0xFFF00000) | ((((((_iml_v2_dp_union_t *) & dbB)->dwords.hi_dword & 0x000FFFFF) & 0x80000) | 0x40000) & 0x000FFFFF));

                dbU = dbX - dbB;
                dbVTmp1 = ((dbU) * (((__constant double *) __datanpi_la_CoutTab)[241]));
                dbVTmp2 = (dbVTmp1 - (dbU));
                dbVTmp1 = (dbVTmp1 - dbVTmp2);
                dbVTmp2 = ((dbU) - dbVTmp1);
                dbUHi = dbVTmp1;
                dbULo = dbVTmp2;

                dbVTmp1 = ((dbX) * (((__constant double *) __datanpi_la_CoutTab)[241]));
                dbVTmp2 = (dbVTmp1 - (dbX));
                dbVTmp1 = (dbVTmp1 - dbVTmp2);
                dbVTmp2 = ((dbX) - dbVTmp1);
                dbXHi = dbVTmp1;
                dbXLo = dbVTmp2;
                dbXB1Hi = dbXHi * dbB;
                dbXB1Lo = dbXLo * dbB;

                dbVTmp1 = ((dbXB1Hi) + (dbXB1Lo));
                dbTmp1 = ((dbXB1Hi) - dbVTmp1);
                dbVTmp2 = (dbTmp1 + (dbXB1Lo));
                dbXBHi = dbVTmp1;
                dbXBLo = dbVTmp2;

                dbVTmp1 = ((((__constant double *) __datanpi_la_CoutTab)[238]) + (dbXBHi));
                dbVTmp2 = ((((__constant double *) __datanpi_la_CoutTab)[238]) - dbVTmp1);
                dbVTmp3 = (dbVTmp1 + dbVTmp2);
                dbVTmp2 = ((dbXBHi) + dbVTmp2);
                dbVTmp3 = ((((__constant double *) __datanpi_la_CoutTab)[238]) - dbVTmp3);
                dbVTmp3 = (dbVTmp2 + dbVTmp3);
                dbV1Hi = dbVTmp1;
                dbV2Lo = dbVTmp3;

                dbV1Lo = dbV2Lo + dbXBLo;

                dbVTmp1 = ((dbV1Hi) * (((__constant double *) __datanpi_la_CoutTab)[241]));
                dbVTmp2 = (dbVTmp1 - (dbV1Hi));
                dbVTmp1 = (dbVTmp1 - dbVTmp2);
                dbVTmp2 = ((dbV1Hi) - dbVTmp1);
                dbVHi = dbVTmp1;
                dbV3Lo = dbVTmp2;
                dbVLo = dbV3Lo + dbV1Lo;

                dbTmp1 = (((__constant double *) __datanpi_la_CoutTab)[238] / dbVHi);
                dbVTmp2 = (dbTmp1 * ((__constant double *) __datanpi_la_CoutTab)[241]);
                dbVTmp3 = (dbVTmp2 - dbTmp1);
                dbVTmp3 = (dbVTmp2 - dbVTmp3);
                dbTmp1 = (dbVHi * dbVTmp3);
                dbTmp1 = (((__constant double *) __datanpi_la_CoutTab)[238] - dbTmp1);
                dbVTmp2 = (dbVLo * dbVTmp3);
                dbVTmp2 = (dbTmp1 - dbVTmp2);
                dbTmp1 = (((__constant double *) __datanpi_la_CoutTab)[238] + dbVTmp2);
                dbQHi = dbVTmp3;
                dbTmp1 = (dbTmp1 * dbVTmp2);
                dbQLo = (dbTmp1 * dbVTmp3);

                dbTmp1 = ((dbQHi) * (dbUHi));
                dbTmp2 = ((dbQLo) * (dbULo));
                dbTmp2 = (dbTmp2 + (dbQHi) * (dbULo));
                dbVTmp1 = (dbTmp2 + (dbQLo) * (dbUHi));
                dbT1Hi = dbTmp1;
                dbT1Lo = dbVTmp1;

                dbVTmp1 = ((dbT1Hi) + (dbT1Lo));
                dbTmp1 = ((dbT1Hi) - dbVTmp1);
                dbVTmp2 = (dbTmp1 + (dbT1Lo));
                dbTHi = dbVTmp1;
                dbTLo = dbVTmp2;

                dbS = dbTHi * dbTHi;
                dbAtanPoly =
                    ((((((((__constant double *) __datanpi_la_CoutTab)[234]) * dbS + ((__constant double *) __datanpi_la_CoutTab)[233]) * dbS +
                        ((__constant double *) __datanpi_la_CoutTab)[232]) * dbS + ((__constant double *) __datanpi_la_CoutTab)[231]) * dbS +
                      ((__constant double *) __datanpi_la_CoutTab)[230]) * dbS + ((__constant double *) __datanpi_la_CoutTab)[229]) * dbS +
                    ((__constant double *) __datanpi_la_CoutTab)[228];

                dbVTmp1 = ((((__constant double *) __datanpi_la_CoutTab)[0 + 2 * (iJ) + 0]) + (dbTHi));
                dbTmp1 = ((((__constant double *) __datanpi_la_CoutTab)[0 + 2 * (iJ) + 0]) - dbVTmp1);
                dbVTmp2 = (dbTmp1 + (dbTHi));
                dbAHi = dbVTmp1;
                dbALo = dbVTmp2;

                dbTmp1 = dbAtanPoly * dbS;
                dbTmp1 = dbTmp1 * dbTHi;
                dbTmp1 = dbTmp1 + dbTLo;
                dbTmp1 = dbTmp1 + ((__constant double *) __datanpi_la_CoutTab)[0 + 2 * (iJ) + 1];
                dbTmp1 = dbTmp1 + dbALo;

                dbVTmp1 = ((dbAHi) * (((__constant double *) __datanpi_la_CoutTab)[241]));
                dbVTmp2 = (dbVTmp1 - (dbAHi));
                dbVTmp1 = (dbVTmp1 - dbVTmp2);
                dbVTmp2 = ((dbAHi) - dbVTmp1);
                dbResHi = dbVTmp1;
                dbResLo = dbVTmp2;;
                dbResLo += dbTmp1;

                dbTmp1 = ((dbResHi) * (((__constant double *) __datanpi_la_CoutTab)[242]));
                dbTmp2 = ((dbResLo) * (((__constant double *) __datanpi_la_CoutTab)[243]));
                dbTmp2 = (dbTmp2 + (dbResHi) * (((__constant double *) __datanpi_la_CoutTab)[243]));
                dbVTmp1 = (dbTmp2 + (dbResLo) * (((__constant double *) __datanpi_la_CoutTab)[242]));
                dbResHi = dbTmp1;
                dbResLo = dbVTmp1;;

                dbRes = dbResHi + dbResLo;

                (((_iml_v2_dp_union_t *) & dbRes)->dwords.hi_dword =
                 (((_iml_v2_dp_union_t *) & dbRes)->dwords.hi_dword & 0x7FFFFFFF) | ((_iml_uint32_t) (iSign) << 31));
                (*r) = dbRes;

            }
            else
            {

                dbRes = 0.5;

                (((_iml_v2_dp_union_t *) & dbRes)->dwords.hi_dword =
                 (((_iml_v2_dp_union_t *) & dbRes)->dwords.hi_dword & 0x7FFFFFFF) | ((_iml_uint32_t) (iSign) << 31));
                (*r) = dbRes;

            }
        }
        else
        {

            if (dbX >= ((__constant double *) __datanpi_la_CoutTab)[235])
            {

                dbS = dbX * dbX;
                dbAtanPoly =
                    ((((((((__constant double *) __datanpi_la_CoutTab)[234]) * dbS + ((__constant double *) __datanpi_la_CoutTab)[233]) * dbS +
                        ((__constant double *) __datanpi_la_CoutTab)[232]) * dbS + ((__constant double *) __datanpi_la_CoutTab)[231]) * dbS +
                      ((__constant double *) __datanpi_la_CoutTab)[230]) * dbS + ((__constant double *) __datanpi_la_CoutTab)[229]) * dbS +
                    ((__constant double *) __datanpi_la_CoutTab)[228];

                dbRes = dbAtanPoly * dbS;
                dbVTmp1 = ((dbX) * (((__constant double *) __datanpi_la_CoutTab)[241]));
                dbVTmp2 = (dbVTmp1 - (dbX));
                dbVTmp1 = (dbVTmp1 - dbVTmp2);
                dbVTmp2 = ((dbX) - dbVTmp1);
                dbResHi = dbVTmp1;
                dbResLo = dbVTmp2;;
                dbResLo += dbRes * dbX;

                dbTmp1 = ((dbResHi) * (((__constant double *) __datanpi_la_CoutTab)[242]));
                dbTmp2 = ((dbResLo) * (((__constant double *) __datanpi_la_CoutTab)[243]));
                dbTmp2 = (dbTmp2 + (dbResHi) * (((__constant double *) __datanpi_la_CoutTab)[243]));
                dbVTmp1 = (dbTmp2 + (dbResLo) * (((__constant double *) __datanpi_la_CoutTab)[242]));
                dbResHi = dbTmp1;
                dbResLo = dbVTmp1;;

                dbRes = dbResHi + dbResLo;

                (((_iml_v2_dp_union_t *) & dbRes)->dwords.hi_dword =
                 (((_iml_v2_dp_union_t *) & dbRes)->dwords.hi_dword & 0x7FFFFFFF) | ((_iml_uint32_t) (iSign) << 31));
                (*r) = dbRes;

            }
            else
            {

                dbVTmp1 = ((__constant double *) __datanpi_la_CoutTab)[238] + dbX;

                dbRes = dbVTmp1 * dbX;
                dbRes *= ((__constant double *) __datanpi_la_CoutTab)[244];
                dbVTmp1 = ((dbRes) * (((__constant double *) __datanpi_la_CoutTab)[241]));
                dbVTmp2 = (dbVTmp1 - (dbRes));
                dbVTmp1 = (dbVTmp1 - dbVTmp2);
                dbVTmp2 = ((dbRes) - dbVTmp1);
                dbResHi = dbVTmp1;
                dbResLo = dbVTmp2;;
                dbTmp1 = ((dbResHi) * (((__constant double *) __datanpi_la_CoutTab)[242]));
                dbTmp2 = ((dbResLo) * (((__constant double *) __datanpi_la_CoutTab)[243]));
                dbTmp2 = (dbTmp2 + (dbResHi) * (((__constant double *) __datanpi_la_CoutTab)[243]));
                dbVTmp1 = (dbTmp2 + (dbResLo) * (((__constant double *) __datanpi_la_CoutTab)[242]));
                dbResHi = dbTmp1;
                dbResLo = dbVTmp1;;

                dbRes = dbResHi + dbResLo;
                if (dbRes < ((__constant double *) __datanpi_la_CoutTab)[246])
                {

                    dbVTmp1 = ((dbResHi) * (((__constant double *) __datanpi_la_CoutTab)[241]));
                    dbVTmp2 = (dbVTmp1 - (dbResHi));
                    dbVTmp1 = (dbVTmp1 - dbVTmp2);
                    dbVTmp2 = ((dbResHi) - dbVTmp1);
                    dbResHi = dbVTmp1;
                    dbResMid = dbVTmp2;;
                    dbResLo += dbResMid;

                    dbVTmp1 = (dbResHi * ((__constant double *) __datanpi_la_CoutTab)[245]);
                    dbVTmp2 = (dbResLo * ((__constant double *) __datanpi_la_CoutTab)[245]);
                    dbRes = dbVTmp1 + (dbVTmp2);
                }
                else
                {
                    dbRes *= ((__constant double *) __datanpi_la_CoutTab)[245];
                }

                (((_iml_v2_dp_union_t *) & dbRes)->dwords.hi_dword =
                 (((_iml_v2_dp_union_t *) & dbRes)->dwords.hi_dword & 0x7FFFFFFF) | ((_iml_uint32_t) (iSign) << 31));
                (*r) = dbRes;
            }
        }
    }
    else
    {
        if ((((((_iml_v2_dp_union_t *) & (*a))->dwords.hi_dword & 0x000FFFFF) == 0) && ((((_iml_v2_dp_union_t *) & (*a))->dwords.lo_dword) == 0)))
        {

            dbRes = 0.5;

            iSign = (((_iml_v2_dp_union_t *) & (*a))->dwords.hi_dword >> 31);
            (((_iml_v2_dp_union_t *) & dbRes)->dwords.hi_dword =
             (((_iml_v2_dp_union_t *) & dbRes)->dwords.hi_dword & 0x7FFFFFFF) | ((_iml_uint32_t) (iSign) << 31));
            (*r) = dbRes;
        }
        else
        {

            (*r) = (*a) + (*a);
        }
    }

    return nRet;
}

double __ocl_svml_atanpi (double a)
{

    double va1;
    double vr1;
    unsigned int vm;

    double r;

    va1 = a;;

    {

        double dAY;
        double dSX;
        double dSY;
        unsigned long lAX;
        unsigned long lAY;
        unsigned int iAX;
        unsigned int iAY;
        unsigned int DOMCHK_iAX1;
        unsigned int DOMCHK_iAY1;
        unsigned int DOMCHK_iXBad;
        unsigned int DOMCHK_iYBad;
        unsigned int DOMCHK_iXYBad;
        double AT_dM;
        double AT_dA1;
        double AT_dB1;
        double AT_dNAX;
        double AT_dA2;
        double AT_dB2;
        double AT_dA;
        double AT_dB;
        double AT_dBHi;
        double AT_dBLo;
        double AT_dC;
        double AT_dXNeg;
        double AT_dD;
        double AT_dR;
        double AT_dR2;
        double AT_dR4;
        double AT_dP2;
        double AT_dP1;
        double AT_dP;
        unsigned long AT_lBHi;
        unsigned int AT_iBHi;
        unsigned int AT_iebhi;
        unsigned int AT_imebhi;
        unsigned int AT_iBHi1;
        unsigned int AT_iMBHi;
        float AT_sMBHi;
        float AT_sR0;
        unsigned int AT_iR0;
        unsigned long AT_lR0;
        double AT_dR0;
        double AT_dR0BHi;
        double AT_dR0BLo;
        double AT_dE;
        double AT_dEHi;
        double AT_dInv;
        double AT_dInvLo;

        double D_PI;
        double D_PIO2;
        double AT_D_A21;
        double AT_D_A20;
        double AT_D_A19;
        double AT_D_A18;
        double AT_D_A17;
        double AT_D_A16;
        double AT_D_A15;
        double AT_D_A14;
        double AT_D_A13;
        double AT_D_A12;
        double AT_D_A11;
        double AT_D_A10;
        double AT_D_A09;
        double AT_D_A08;
        double AT_D_A07;
        double AT_D_A06;
        double AT_D_A05;
        double AT_D_A04;
        double AT_D_A03;
        double AT_D_A02;
        double AT_D_A01;
        double AT_D_A00;
        double D_SIGN_MASK;
        unsigned int I_CHK_WORK_SUB;
        unsigned int I_CHK_WORK_CMP;
        double D_ABSMASK;
        double D_ZERO;
        unsigned int I_D_EXP_MASK;
        unsigned int I_D_2_BIAS;
        unsigned int I_S_MANTISSA_MASK;
        unsigned int I_S_ONE;
        unsigned int I_D_BIAS;
        double D_HIGH_20_MASK;
        double D_ONE;
        double D_NONE;

        D_ABSMASK = as_double (__internal_datanpi_la_data.D_ABSMASK);
        dAY = as_double ((as_ulong (va1) & as_ulong (D_ABSMASK)));

        D_SIGN_MASK = as_double (__internal_datanpi_la_data.D_SIGN_MASK);
        dSY = as_double ((as_ulong (va1) & as_ulong (D_SIGN_MASK)));

        lAY = as_ulong (dAY);
        iAY = ((unsigned int) ((unsigned long) lAY >> 32));
        I_CHK_WORK_SUB = (__internal_datanpi_la_data.I_CHK_WORK_SUB);
        DOMCHK_iAY1 = (iAY - I_CHK_WORK_SUB);
        I_CHK_WORK_CMP = (__internal_datanpi_la_data.I_CHK_WORK_CMP);
        DOMCHK_iYBad = ((unsigned int) (-(signed int) ((signed int) DOMCHK_iAY1 >= (signed int) I_CHK_WORK_CMP)));
        vm = 0;
        vm = DOMCHK_iYBad;

        D_ONE = as_double (__internal_datanpi_la_data.D_ONE);
        AT_dM = as_double ((unsigned long) ((dAY <= D_ONE) ? 0xffffffffffffffff : 0x0));

        AT_dA1 = as_double ((as_ulong (AT_dM) & as_ulong (dAY)));
        AT_dB1 = as_double ((as_ulong (AT_dM) & as_ulong (D_ONE)));

        D_NONE = as_double (__internal_datanpi_la_data.D_NONE);
        AT_dA2 = as_double ((~(as_ulong (AT_dM)) & as_ulong (D_NONE)));
        AT_dB2 = as_double ((~(as_ulong (AT_dM)) & as_ulong (dAY)));

        AT_dA = as_double ((as_ulong (AT_dA1) | as_ulong (AT_dA2)));
        AT_dB = as_double ((as_ulong (AT_dB1) | as_ulong (AT_dB2)));

        D_PIO2 = as_double (__internal_datanpi_la_data.D_PIO2);
        AT_dC = as_double ((~(as_ulong (AT_dM)) & as_ulong (D_PIO2)));
        D_HIGH_20_MASK = as_double (__internal_datanpi_la_data.D_HIGH_20_MASK);
        AT_dBHi = as_double ((as_ulong (AT_dB) & as_ulong (D_HIGH_20_MASK)));
        AT_dBLo = (AT_dB - AT_dBHi);
        AT_lBHi = as_ulong (AT_dB);
        AT_iBHi = ((unsigned int) ((unsigned long) AT_lBHi >> 32));
        I_D_EXP_MASK = (__internal_datanpi_la_data.I_D_EXP_MASK);
        AT_iebhi = (AT_iBHi & I_D_EXP_MASK);
        I_D_2_BIAS = (__internal_datanpi_la_data.I_D_2_BIAS);
        AT_imebhi = (I_D_2_BIAS - AT_iebhi);
        AT_iBHi1 = ((unsigned int) (AT_iBHi) << (3));
        I_S_MANTISSA_MASK = (__internal_datanpi_la_data.I_S_MANTISSA_MASK);
        AT_iMBHi = (AT_iBHi1 & I_S_MANTISSA_MASK);
        I_S_ONE = (__internal_datanpi_la_data.I_S_ONE);
        AT_iMBHi = (AT_iMBHi | I_S_ONE);
        AT_sMBHi = as_float (AT_iMBHi);
        AT_sR0 = (1.0f / (AT_sMBHi));
        AT_iR0 = as_uint (AT_sR0);
        AT_iR0 = ((unsigned int) (AT_iR0) >> (3));
        I_D_BIAS = (__internal_datanpi_la_data.I_D_BIAS);
        AT_iR0 = (AT_iR0 - I_D_BIAS);
        AT_iR0 = (AT_iR0 + AT_imebhi);
        AT_lR0 = (((unsigned long) (unsigned int) AT_iR0 << 32) | (unsigned long) (unsigned int) AT_iR0);
        AT_dR0 = as_double (AT_lR0);
        AT_dR0 = as_double ((as_ulong (AT_dR0) & as_ulong (D_HIGH_20_MASK)));

        AT_dR0BHi = (AT_dBHi * AT_dR0);
        AT_dR0BLo = (AT_dBLo * AT_dR0);
        AT_dEHi = (AT_dR0BHi - D_ONE);
        AT_dE = (AT_dEHi + AT_dR0BLo);

        AT_dInv = (AT_dE - D_ONE);
        AT_dInv = (AT_dInv * AT_dE);
        AT_dInv = (AT_dInv + D_ONE);
        AT_dInv = (AT_dInv * AT_dE);
        AT_dInv = (AT_dInv - D_ONE);
        AT_dInv = (AT_dInv * AT_dE);
        AT_dInvLo = (AT_dInv * AT_dR0);

        AT_dR0 = (AT_dR0 + AT_dInvLo);

        AT_dR = (AT_dA * AT_dR0);

        AT_dR2 = (AT_dR * AT_dR);
        AT_D_A21 = as_double (__internal_datanpi_la_data.AT_D_A21);
        AT_D_A20 = as_double (__internal_datanpi_la_data.AT_D_A20);
        AT_dP = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (AT_D_A21, AT_dR2, AT_D_A20);

        AT_D_A19 = as_double (__internal_datanpi_la_data.AT_D_A19);
        AT_dP = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (AT_dP, AT_dR2, AT_D_A19);

        AT_D_A18 = as_double (__internal_datanpi_la_data.AT_D_A18);
        AT_dP = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (AT_dP, AT_dR2, AT_D_A18);

        AT_D_A17 = as_double (__internal_datanpi_la_data.AT_D_A17);
        AT_dP = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (AT_dP, AT_dR2, AT_D_A17);

        AT_D_A16 = as_double (__internal_datanpi_la_data.AT_D_A16);
        AT_dP = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (AT_dP, AT_dR2, AT_D_A16);

        AT_D_A15 = as_double (__internal_datanpi_la_data.AT_D_A15);
        AT_dP = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (AT_dP, AT_dR2, AT_D_A15);

        AT_D_A14 = as_double (__internal_datanpi_la_data.AT_D_A14);
        AT_dP = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (AT_dP, AT_dR2, AT_D_A14);

        AT_D_A13 = as_double (__internal_datanpi_la_data.AT_D_A13);
        AT_dP = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (AT_dP, AT_dR2, AT_D_A13);

        AT_D_A12 = as_double (__internal_datanpi_la_data.AT_D_A12);
        AT_dP = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (AT_dP, AT_dR2, AT_D_A12);

        AT_D_A11 = as_double (__internal_datanpi_la_data.AT_D_A11);
        AT_dP = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (AT_dP, AT_dR2, AT_D_A11);

        AT_D_A10 = as_double (__internal_datanpi_la_data.AT_D_A10);
        AT_dP = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (AT_dP, AT_dR2, AT_D_A10);

        AT_D_A09 = as_double (__internal_datanpi_la_data.AT_D_A09);
        AT_dP = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (AT_dP, AT_dR2, AT_D_A09);

        AT_D_A08 = as_double (__internal_datanpi_la_data.AT_D_A08);
        AT_dP = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (AT_dP, AT_dR2, AT_D_A08);

        AT_D_A07 = as_double (__internal_datanpi_la_data.AT_D_A07);
        AT_dP = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (AT_dP, AT_dR2, AT_D_A07);

        AT_D_A06 = as_double (__internal_datanpi_la_data.AT_D_A06);
        AT_dP = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (AT_dP, AT_dR2, AT_D_A06);

        AT_D_A05 = as_double (__internal_datanpi_la_data.AT_D_A05);
        AT_dP = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (AT_dP, AT_dR2, AT_D_A05);

        AT_D_A04 = as_double (__internal_datanpi_la_data.AT_D_A04);
        AT_dP = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (AT_dP, AT_dR2, AT_D_A04);

        AT_D_A03 = as_double (__internal_datanpi_la_data.AT_D_A03);
        AT_dP = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (AT_dP, AT_dR2, AT_D_A03);

        AT_D_A02 = as_double (__internal_datanpi_la_data.AT_D_A02);
        AT_dP = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (AT_dP, AT_dR2, AT_D_A02);

        AT_D_A01 = as_double (__internal_datanpi_la_data.AT_D_A01);
        AT_dP = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (AT_dP, AT_dR2, AT_D_A01);

        AT_D_A00 = as_double (__internal_datanpi_la_data.AT_D_A00);
        AT_dP = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (AT_dP, AT_dR2, AT_D_A00);

        AT_dP = (AT_dP * AT_dR);

        AT_dP = (AT_dP + AT_dC);
        vr1 = as_double ((as_ulong (AT_dP) | as_ulong (dSY)));
    }

    if ((vm) != 0)
    {
        double _vapi_arg1[1];
        double _vapi_res1[1];
        ((double *) _vapi_arg1)[0] = va1;
        ((double *) _vapi_res1)[0] = vr1;
        __internal_datanpi_la_cout (_vapi_arg1, _vapi_res1);
        vr1 = ((double *) _vapi_res1)[0];
    };
    r = vr1;;

    return r;

}

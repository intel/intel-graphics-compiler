/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF
typedef struct
{
    unsigned long dPI;
    unsigned long dPIO2;

    unsigned long dA21;
    unsigned long dA20;
    unsigned long dA19;
    unsigned long dA18;
    unsigned long dA17;
    unsigned long dA16;
    unsigned long dA15;
    unsigned long dA14;
    unsigned long dA13;
    unsigned long dA12;
    unsigned long dA11;
    unsigned long dA10;

    unsigned long dA09;
    unsigned long dA08;
    unsigned long dA07;
    unsigned long dA06;
    unsigned long dA05;
    unsigned long dA04;
    unsigned long dA03;
    unsigned long dA02;
    unsigned long dA01;
    unsigned long dA00;
    unsigned long dSIGN_MASK;
    unsigned int iCHK_WORK_SUB;
    unsigned int iCHK_WORK_CMP;
    unsigned long dABS_MASK;
    unsigned long dZERO;
    unsigned int idEXP_MASK;
    unsigned int id2_BIAS;
    unsigned int isMANTISSA_MASK;
    unsigned int isONE;
    unsigned int idBIAS;
    unsigned long dHIGH_20_MASK;
    unsigned long dONE;

} __internal_datan2_la_data_t;

static __constant __internal_datan2_la_data_t __internal_datan2_la_data = {
    0x400921FB54442D18uL,
    0x3FF921FB54442D18uL,

    0xBEDFBF0A01116E8EuL,
    0x3F18472E14E48A9CuL,
    0xBEF4FDB537ABC7A3uL,
    0x3F2CED0A36665209uL,
    0xBF52E67C93954C23uL,
    0x3F6F5A1DAE82AFB3uL,
    0xBF82B2EC618E4BADuL,
    0x3F914F4C661116A5uL,
    0xBF9A5E83B081F69CuL,
    0x3FA169980CB6AD4FuL,
    0xBFA4EFA2E563C1BCuL,
    0x3FA7EC0FBC50683BuL,
    0xBFAAD261EAA09954uL,
    0x3FAE1749BD612DCFuL,
    0xBFB11084009435E0uL,
    0x3FB3B12A49295651uL,
    0xBFB745D009BADA94uL,
    0x3FBC71C707F7D5B5uL,
    0xBFC2492491EE55C7uL,
    0x3FC999999997EE34uL,
    0xBFD55555555553C5uL,
    0x3FF0000000000000uL,
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

};

#pragma float_control(precise,on)
static __constant _iml_v2_dp_union_t __datan2_la_CoutTab[251] = {

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

    0x00000000, 0x3FC00000,
    0x00000000, 0x3FF00000,
    0x00000000, 0x40000000,
    0x54442D18, 0x3FE921FB,
    0x33145C07, 0x3C81A626,
    0x54442D18, 0x3FF921FB,
    0x33145C07, 0x3C91A626,
    0x54442D18, 0x400921FB,
    0x33145C07, 0x3CA1A626,
    0x7F3321D2, 0x4002D97C,
    0x4C9E8A0A, 0x3C9A7939,
    0x00000000, 0xBFF00000,
    0x00000000, 0x00000000,
    0x00000000, 0x7FD00000,
    0x00000000, 0x00100000,

    0x02000000, 0x41A00000,
};

__attribute__((always_inline))
inline int __internal_datan2_la_cout (double *a, double *b, double *r)
{
    int nRet = 0;
    double dbY, dbX;
    double dbAY, dbAX, dbZPHi, dbZPLo, dbYOX, dbAY1, dbTwoPowN, dbAX1, dbB, dbXHi, dbXLo,
        dbBXHi, dbBXLo, dbTmp1, dbUHi, dbULo, dbYHi, dbYLo, dbBYHi, dbBYLo, dbVHi,
        dbVLo, dbTmp2, dbR0, dbE, dbQHi, dbQLo, dbTHi, dbTLo, dbT2, dbAtanPoly, dbAtanPolyHi, dbAtanPolyLo, dbRHi, dbRLo, dbRes;
    double dbVTmp1, dbVTmp2, dbVTmp3;
    int i, iSignY, iSignX, iSign, iJ, iEY, iEX, iEY1;

    i = 0;

    dbY = (((__constant double *) __datan2_la_CoutTab)[236]);
    dbY = (dbY * (a[0]));
    dbX = (((__constant double *) __datan2_la_CoutTab)[236]);
    dbX = (dbX * (b[0]));

    iSignY = (((_iml_v2_dp_union_t *) & dbY)->dwords.hi_dword >> 31);
    iSignX = (((_iml_v2_dp_union_t *) & dbX)->dwords.hi_dword >> 31);

    iEY = ((((_iml_v2_dp_union_t *) & dbY)->dwords.hi_dword >> 20) & 0x7FF);
    iEX = ((((_iml_v2_dp_union_t *) & dbX)->dwords.hi_dword >> 20) & 0x7FF);

    if ((((((_iml_v2_dp_union_t *) & dbY)->dwords.hi_dword >> 20) & 0x7FF) != 0x7FF)
        && (((((_iml_v2_dp_union_t *) & dbX)->dwords.hi_dword >> 20) & 0x7FF) != 0x7FF))
    {

        if (((iEY != 0)
             || !(((((_iml_v2_dp_union_t *) & dbY)->dwords.hi_dword & 0x000FFFFF) == 0) && ((((_iml_v2_dp_union_t *) & dbY)->dwords.lo_dword) == 0)))
            && ((iEX != 0)
                || !(((((_iml_v2_dp_union_t *) & dbX)->dwords.hi_dword & 0x000FFFFF) == 0) && ((((_iml_v2_dp_union_t *) & dbX)->dwords.lo_dword) == 0))))
        {

            dbAY = dbY;
            (((_iml_v2_dp_union_t *) & dbAY)->dwords.hi_dword =
             (((_iml_v2_dp_union_t *) & dbAY)->dwords.hi_dword & 0x7FFFFFFF) | ((_iml_uint32_t) (0) << 31));
            dbAX = dbX;
            (((_iml_v2_dp_union_t *) & dbAX)->dwords.hi_dword =
             (((_iml_v2_dp_union_t *) & dbAX)->dwords.hi_dword & 0x7FFFFFFF) | ((_iml_uint32_t) (0) << 31));

            if (iEY - iEX > -54)
            {

                if (iEY - iEX < 54)
                {

                    if (iSignX == 0)
                    {

                        dbZPHi = ((__constant double *) __datan2_la_CoutTab)[247];
                        dbZPLo = ((__constant double *) __datan2_la_CoutTab)[247];
                    }
                    else
                    {

                        dbZPHi = ((__constant double *) __datan2_la_CoutTab)[242];
                        dbZPLo = ((__constant double *) __datan2_la_CoutTab)[243];
                    }

                    dbYOX = dbAY / dbAX;

                    if (iEY > 0)
                    {

                        if (iEY < 0x7FF - 1)
                        {

                            dbAY1 = dbAY;
                            (((_iml_v2_dp_union_t *) & dbAY1)->dwords.hi_dword =
                             (((_iml_v2_dp_union_t *) & dbAY1)->dwords.hi_dword & 0x800FFFFF) | (((_iml_uint32_t) (0x3FF) & 0x7FF) << 20));
                            iEY1 = iEY - 0x3FF;
                        }
                        else
                        {

                            dbAY1 = dbAY * ((__constant double *) __datan2_la_CoutTab)[249];
                            iEY1 = (0x3FF - 1);
                        }
                    }
                    else
                    {

                        dbAY1 = dbAY * ((__constant double *) __datan2_la_CoutTab)[248];
                        iEY1 = -(0x3FF - 1);
                    }

                    dbTwoPowN = ((__constant double *) __datan2_la_CoutTab)[236];
                    (((_iml_v2_dp_union_t *) & dbTwoPowN)->dwords.hi_dword =
                     (((_iml_v2_dp_union_t *) & dbTwoPowN)->dwords.hi_dword & 0x800FFFFF) | (((_iml_uint32_t) (0x3FF - iEY1) & 0x7FF) << 20));
                    dbAX1 = dbAX * dbTwoPowN;

                    if (dbYOX >= ((__constant double *) __datan2_la_CoutTab)[235])
                    {

                        iJ = ((((_iml_v2_dp_union_t *) & dbYOX)->dwords.hi_dword >> 20) & 0x7FF);
                        iJ = iJ << 20;
                        iJ = iJ | (((_iml_v2_dp_union_t *) & dbYOX)->dwords.hi_dword & 0x000FFFFF);
                        iJ = iJ - 0x3FC00000;
                        iJ = iJ >> 19;
                        if (iJ > 113)
                            iJ = 113;

                        dbB = dbYOX;
                        (((_iml_v2_dp_union_t *) & dbB)->dwords.lo_dword = (0));
                        (((_iml_v2_dp_union_t *) & dbB)->dwords.hi_dword =
                         (((_iml_v2_dp_union_t *) & dbB)->dwords.
                          hi_dword & 0xFFF00000) | ((((((_iml_v2_dp_union_t *) & dbB)->dwords.hi_dword & 0x000FFFFF) & 0x80000) | 0x40000) &
                                                    0x000FFFFF));

                        dbVTmp1 = ((dbAX1) * (((__constant double *) __datan2_la_CoutTab)[250]));
                        dbVTmp2 = (dbVTmp1 - (dbAX1));
                        dbVTmp1 = (dbVTmp1 - dbVTmp2);
                        dbVTmp2 = ((dbAX1) - dbVTmp1);
                        dbXHi = dbVTmp1;
                        dbXLo = dbVTmp2;;

                        dbBXHi = dbXHi * dbB;
                        dbBXLo = dbXLo * dbB;

                        dbBXHi = dbBXHi * ((__constant double *) __datan2_la_CoutTab)[246];
                        dbBXLo = dbBXLo * ((__constant double *) __datan2_la_CoutTab)[246];
                        dbVTmp1 = ((dbBXHi) + (dbBXLo));
                        dbTmp1 = ((dbBXHi) - dbVTmp1);
                        dbVTmp2 = (dbTmp1 + (dbBXLo));
                        dbBXHi = dbVTmp1;
                        dbBXLo = dbVTmp2;;

                        dbVTmp1 = ((dbAY1) + (dbBXHi));
                        dbVTmp2 = ((dbAY1) - dbVTmp1);
                        dbVTmp3 = (dbVTmp1 + dbVTmp2);
                        dbVTmp2 = ((dbBXHi) + dbVTmp2);
                        dbVTmp3 = ((dbAY1) - dbVTmp3);
                        dbVTmp3 = (dbVTmp2 + dbVTmp3);
                        dbUHi = dbVTmp1;
                        dbULo = dbVTmp3;;

                        dbULo = dbULo + dbBXLo;

                        dbVTmp1 = ((dbUHi) * (((__constant double *) __datan2_la_CoutTab)[250]));
                        dbVTmp2 = (dbVTmp1 - (dbUHi));
                        dbVTmp1 = (dbVTmp1 - dbVTmp2);
                        dbVTmp2 = ((dbUHi) - dbVTmp1);
                        dbUHi = dbVTmp1;
                        dbTmp1 = dbVTmp2;;

                        dbULo = dbULo + dbTmp1;

                        dbVTmp1 = ((dbAY1) * (((__constant double *) __datan2_la_CoutTab)[250]));
                        dbVTmp2 = (dbVTmp1 - (dbAY1));
                        dbVTmp1 = (dbVTmp1 - dbVTmp2);
                        dbVTmp2 = ((dbAY1) - dbVTmp1);
                        dbYHi = dbVTmp1;
                        dbYLo = dbVTmp2;;

                        dbBYHi = dbYHi * dbB;
                        dbBYLo = dbYLo * dbB;

                        dbVTmp1 = ((dbBYHi) + (dbBYLo));
                        dbTmp1 = ((dbBYHi) - dbVTmp1);
                        dbVTmp2 = (dbTmp1 + (dbBYLo));
                        dbBYHi = dbVTmp1;
                        dbBYLo = dbVTmp2;;

                        dbVTmp1 = ((dbAX1) + (dbBYHi));
                        dbVTmp2 = ((dbAX1) - dbVTmp1);
                        dbVTmp3 = (dbVTmp1 + dbVTmp2);
                        dbVTmp2 = ((dbBYHi) + dbVTmp2);
                        dbVTmp3 = ((dbAX1) - dbVTmp3);
                        dbVTmp3 = (dbVTmp2 + dbVTmp3);
                        dbVHi = dbVTmp1;
                        dbVLo = dbVTmp3;;

                        dbVLo = dbVLo + dbBYLo;

                        dbVTmp1 = ((dbVHi) * (((__constant double *) __datan2_la_CoutTab)[250]));
                        dbVTmp2 = (dbVTmp1 - (dbVHi));
                        dbVTmp1 = (dbVTmp1 - dbVTmp2);
                        dbVTmp2 = ((dbVHi) - dbVTmp1);
                        dbVHi = dbVTmp1;
                        dbTmp1 = dbVTmp2;;

                        dbVLo = dbVLo + dbTmp1;

                        dbTmp1 = (((__constant double *) __datan2_la_CoutTab)[236] / dbVHi);
                        dbVTmp2 = (dbTmp1 * ((__constant double *) __datan2_la_CoutTab)[250]);
                        dbVTmp3 = (dbVTmp2 - dbTmp1);
                        dbVTmp3 = (dbVTmp2 - dbVTmp3);
                        dbTmp1 = (dbVHi * dbVTmp3);
                        dbTmp1 = (((__constant double *) __datan2_la_CoutTab)[236] - dbTmp1);
                        dbVTmp2 = (dbVLo * dbVTmp3);
                        dbVTmp2 = (dbTmp1 - dbVTmp2);
                        dbTmp1 = (((__constant double *) __datan2_la_CoutTab)[236] + dbVTmp2);
                        dbQHi = dbVTmp3;
                        dbTmp1 = (dbTmp1 * dbVTmp2);
                        dbQLo = (dbTmp1 * dbVTmp3);;

                        dbTmp1 = ((dbUHi) * (dbQHi));
                        dbTmp2 = ((dbULo) * (dbQLo));
                        dbTmp2 = (dbTmp2 + (dbUHi) * (dbQLo));
                        dbVTmp1 = (dbTmp2 + (dbULo) * (dbQHi));
                        dbTHi = dbTmp1;
                        dbTLo = dbVTmp1;;

                        dbVTmp1 = ((dbTHi) + (dbTLo));
                        dbTmp1 = ((dbTHi) - dbVTmp1);
                        dbVTmp2 = (dbTmp1 + (dbTLo));
                        dbTHi = dbVTmp1;
                        dbTLo = dbVTmp2;;

                        dbT2 = dbTHi * dbTHi;
                        dbAtanPoly =
                            ((((((((__constant double *) __datan2_la_CoutTab)[234]) * dbT2 +
                                 ((__constant double *) __datan2_la_CoutTab)[233]) * dbT2 + ((__constant double *) __datan2_la_CoutTab)[232]) * dbT2 +
                               ((__constant double *) __datan2_la_CoutTab)[231]) * dbT2 + ((__constant double *) __datan2_la_CoutTab)[230]) * dbT2 +
                             ((__constant double *) __datan2_la_CoutTab)[229]) * dbT2 + ((__constant double *) __datan2_la_CoutTab)[228];

                        dbAtanPoly = dbAtanPoly * dbT2;

                        dbVTmp1 = ((dbTHi) * (((__constant double *) __datan2_la_CoutTab)[250]));
                        dbVTmp2 = (dbVTmp1 - (dbTHi));
                        dbVTmp1 = (dbVTmp1 - dbVTmp2);
                        dbVTmp2 = ((dbTHi) - dbVTmp1);
                        dbTHi = dbVTmp1;
                        dbTmp1 = dbVTmp2;;

                        dbTLo = dbTLo + dbTmp1;
                        dbVTmp1 = ((dbAtanPoly) * (((__constant double *) __datan2_la_CoutTab)[250]));
                        dbVTmp2 = (dbVTmp1 - (dbAtanPoly));
                        dbVTmp1 = (dbVTmp1 - dbVTmp2);
                        dbVTmp2 = ((dbAtanPoly) - dbVTmp1);
                        dbAtanPolyHi = dbVTmp1;
                        dbAtanPolyLo = dbVTmp2;;

                        dbTmp1 = ((dbAtanPolyHi) * (dbTHi));
                        dbTmp2 = ((dbAtanPolyLo) * (dbTLo));
                        dbTmp2 = (dbTmp2 + (dbAtanPolyHi) * (dbTLo));
                        dbVTmp1 = (dbTmp2 + (dbAtanPolyLo) * (dbTHi));
                        dbRHi = dbTmp1;
                        dbRLo = dbVTmp1;;

                        dbVTmp1 = ((dbRHi) + (dbTHi));
                        dbVTmp2 = ((dbRHi) - dbVTmp1);
                        dbVTmp3 = (dbVTmp1 + dbVTmp2);
                        dbVTmp2 = ((dbTHi) + dbVTmp2);
                        dbVTmp3 = ((dbRHi) - dbVTmp3);
                        dbVTmp3 = (dbVTmp2 + dbVTmp3);
                        dbQHi = dbVTmp1;
                        dbQLo = dbVTmp3;;

                        dbQLo = dbQLo + dbTLo;
                        dbQLo = dbQLo + dbRLo;

                        dbVTmp1 = ((dbQHi) + (((__constant double *) __datan2_la_CoutTab)[0 + 2 * (iJ) + 0]));
                        dbVTmp2 = ((dbQHi) - dbVTmp1);
                        dbVTmp3 = (dbVTmp1 + dbVTmp2);
                        dbVTmp2 = ((((__constant double *) __datan2_la_CoutTab)[0 + 2 * (iJ) + 0]) + dbVTmp2);
                        dbVTmp3 = ((dbQHi) - dbVTmp3);
                        dbVTmp3 = (dbVTmp2 + dbVTmp3);
                        dbQHi = dbVTmp1;
                        dbTmp1 = dbVTmp3;;

                        dbQLo = dbQLo + dbTmp1;
                        dbQLo = dbQLo + ((__constant double *) __datan2_la_CoutTab)[0 + 2 * (iJ) + 1];

                        (((_iml_v2_dp_union_t *) & dbQHi)->dwords.hi_dword =
                         (((_iml_v2_dp_union_t *) & dbQHi)->dwords.hi_dword & 0x7FFFFFFF) | ((_iml_uint32_t) (iSignX) << 31));
                        iSign = iSignX ^ (((_iml_v2_dp_union_t *) & dbQLo)->dwords.hi_dword >> 31);
                        (((_iml_v2_dp_union_t *) & dbQLo)->dwords.hi_dword =
                         (((_iml_v2_dp_union_t *) & dbQLo)->dwords.hi_dword & 0x7FFFFFFF) | ((_iml_uint32_t) (iSign) << 31));

                        dbVTmp1 = ((dbQHi) + (dbZPHi));
                        dbVTmp2 = ((dbQHi) - dbVTmp1);
                        dbVTmp3 = (dbVTmp1 + dbVTmp2);
                        dbVTmp2 = ((dbZPHi) + dbVTmp2);
                        dbVTmp3 = ((dbQHi) - dbVTmp3);
                        dbVTmp3 = (dbVTmp2 + dbVTmp3);
                        dbQHi = dbVTmp1;
                        dbTmp1 = dbVTmp3;;

                        dbQLo = dbQLo + dbTmp1;
                        dbQLo = dbQLo + dbZPLo;
                        dbRes = dbQHi + dbQLo;

                        (((_iml_v2_dp_union_t *) & dbRes)->dwords.hi_dword =
                         (((_iml_v2_dp_union_t *) & dbRes)->dwords.hi_dword & 0x7FFFFFFF) | ((_iml_uint32_t) (iSignY) << 31));
                        (*r) = dbRes;
                    }
                    else
                    {

                        dbVTmp1 = ((dbAY1) * (((__constant double *) __datan2_la_CoutTab)[250]));
                        dbVTmp2 = (dbVTmp1 - (dbAY1));
                        dbVTmp1 = (dbVTmp1 - dbVTmp2);
                        dbVTmp2 = ((dbAY1) - dbVTmp1);
                        dbUHi = dbVTmp1;
                        dbULo = dbVTmp2;;

                        dbVTmp1 = ((dbAX1) * (((__constant double *) __datan2_la_CoutTab)[250]));
                        dbVTmp2 = (dbVTmp1 - (dbAX1));
                        dbVTmp1 = (dbVTmp1 - dbVTmp2);
                        dbVTmp2 = ((dbAX1) - dbVTmp1);
                        dbVHi = dbVTmp1;
                        dbVLo = dbVTmp2;;

                        dbTmp1 = (((__constant double *) __datan2_la_CoutTab)[236] / dbVHi);
                        dbVTmp2 = (dbTmp1 * ((__constant double *) __datan2_la_CoutTab)[250]);
                        dbVTmp3 = (dbVTmp2 - dbTmp1);
                        dbVTmp3 = (dbVTmp2 - dbVTmp3);
                        dbTmp1 = (dbVHi * dbVTmp3);
                        dbTmp1 = (((__constant double *) __datan2_la_CoutTab)[236] - dbTmp1);
                        dbVTmp2 = (dbVLo * dbVTmp3);
                        dbVTmp2 = (dbTmp1 - dbVTmp2);
                        dbTmp1 = (((__constant double *) __datan2_la_CoutTab)[236] + dbVTmp2);
                        dbQHi = dbVTmp3;
                        dbTmp1 = (dbTmp1 * dbVTmp2);
                        dbQLo = (dbTmp1 * dbVTmp3);;

                        dbTmp1 = ((dbUHi) * (dbQHi));
                        dbTmp2 = ((dbULo) * (dbQLo));
                        dbTmp2 = (dbTmp2 + (dbUHi) * (dbQLo));
                        dbVTmp1 = (dbTmp2 + (dbULo) * (dbQHi));
                        dbTHi = dbTmp1;
                        dbTLo = dbVTmp1;;

                        dbVTmp1 = ((dbTHi) + (dbTLo));
                        dbTmp1 = ((dbTHi) - dbVTmp1);
                        dbVTmp2 = (dbTmp1 + (dbTLo));
                        dbTHi = dbVTmp1;
                        dbTLo = dbVTmp2;;

                        dbT2 = dbYOX * dbYOX;
                        dbAtanPoly =
                            ((((((((__constant double *) __datan2_la_CoutTab)[234]) * dbT2 +
                                 ((__constant double *) __datan2_la_CoutTab)[233]) * dbT2 + ((__constant double *) __datan2_la_CoutTab)[232]) * dbT2 +
                               ((__constant double *) __datan2_la_CoutTab)[231]) * dbT2 + ((__constant double *) __datan2_la_CoutTab)[230]) * dbT2 +
                             ((__constant double *) __datan2_la_CoutTab)[229]) * dbT2 + ((__constant double *) __datan2_la_CoutTab)[228];

                        dbAtanPoly = dbAtanPoly * dbT2;

                        dbVTmp1 = ((dbTHi) * (((__constant double *) __datan2_la_CoutTab)[250]));
                        dbVTmp2 = (dbVTmp1 - (dbTHi));
                        dbVTmp1 = (dbVTmp1 - dbVTmp2);
                        dbVTmp2 = ((dbTHi) - dbVTmp1);
                        dbTHi = dbVTmp1;
                        dbTmp1 = dbVTmp2;;

                        dbTLo = dbTLo + dbTmp1;
                        dbVTmp1 = ((dbAtanPoly) * (((__constant double *) __datan2_la_CoutTab)[250]));
                        dbVTmp2 = (dbVTmp1 - (dbAtanPoly));
                        dbVTmp1 = (dbVTmp1 - dbVTmp2);
                        dbVTmp2 = ((dbAtanPoly) - dbVTmp1);
                        dbAtanPolyHi = dbVTmp1;
                        dbAtanPolyLo = dbVTmp2;;

                        dbTmp1 = ((dbAtanPolyHi) * (dbTHi));
                        dbTmp2 = ((dbAtanPolyLo) * (dbTLo));
                        dbTmp2 = (dbTmp2 + (dbAtanPolyHi) * (dbTLo));
                        dbVTmp1 = (dbTmp2 + (dbAtanPolyLo) * (dbTHi));
                        dbRHi = dbTmp1;
                        dbRLo = dbVTmp1;;

                        dbVTmp1 = ((dbRHi) + (dbTHi));
                        dbVTmp2 = ((dbRHi) - dbVTmp1);
                        dbVTmp3 = (dbVTmp1 + dbVTmp2);
                        dbVTmp2 = ((dbTHi) + dbVTmp2);
                        dbVTmp3 = ((dbRHi) - dbVTmp3);
                        dbVTmp3 = (dbVTmp2 + dbVTmp3);
                        dbQHi = dbVTmp1;
                        dbQLo = dbVTmp3;;

                        dbQLo = dbQLo + dbTLo;
                        dbQLo = dbQLo + dbRLo;

                        (((_iml_v2_dp_union_t *) & dbQHi)->dwords.hi_dword =
                         (((_iml_v2_dp_union_t *) & dbQHi)->dwords.hi_dword & 0x7FFFFFFF) | ((_iml_uint32_t) (iSignX) << 31));
                        iSign = iSignX ^ (((_iml_v2_dp_union_t *) & dbQLo)->dwords.hi_dword >> 31);
                        (((_iml_v2_dp_union_t *) & dbQLo)->dwords.hi_dword =
                         (((_iml_v2_dp_union_t *) & dbQLo)->dwords.hi_dword & 0x7FFFFFFF) | ((_iml_uint32_t) (iSign) << 31));

                        dbVTmp1 = ((dbQHi) + (dbZPHi));
                        dbVTmp2 = ((dbQHi) - dbVTmp1);
                        dbVTmp3 = (dbVTmp1 + dbVTmp2);
                        dbVTmp2 = ((dbZPHi) + dbVTmp2);
                        dbVTmp3 = ((dbQHi) - dbVTmp3);
                        dbVTmp3 = (dbVTmp2 + dbVTmp3);
                        dbQHi = dbVTmp1;
                        dbTmp1 = dbVTmp3;;

                        dbQLo = dbQLo + dbTmp1;
                        dbQLo = dbQLo + dbZPLo;
                        dbRes = dbQHi + dbQLo;

                        (((_iml_v2_dp_union_t *) & dbRes)->dwords.hi_dword =
                         (((_iml_v2_dp_union_t *) & dbRes)->dwords.hi_dword & 0x7FFFFFFF) | ((_iml_uint32_t) (iSignY) << 31));
                        (*r) = dbRes;
                    }
                }
                else if (iEY - iEX < 74)
                {

                    dbTmp1 = dbX / dbAY;
                    dbRes = ((__constant double *) __datan2_la_CoutTab)[241] - dbTmp1;
                    dbRes = dbRes + ((__constant double *) __datan2_la_CoutTab)[240];

                    (((_iml_v2_dp_union_t *) & dbRes)->dwords.hi_dword =
                     (((_iml_v2_dp_union_t *) & dbRes)->dwords.hi_dword & 0x7FFFFFFF) | ((_iml_uint32_t) (iSignY) << 31));
                    (*r) = dbRes;
                }
                else
                {

                    dbRes = ((__constant double *) __datan2_la_CoutTab)[240] + ((__constant double *) __datan2_la_CoutTab)[241];

                    (((_iml_v2_dp_union_t *) & dbRes)->dwords.hi_dword =
                     (((_iml_v2_dp_union_t *) & dbRes)->dwords.hi_dword & 0x7FFFFFFF) | ((_iml_uint32_t) (iSignY) << 31));
                    (*r) = dbRes;
                }
            }
            else
            {

                if (iSignX == 0)
                {

                    dbRes = dbAY / dbAX;

                    if (((((_iml_v2_dp_union_t *) & dbRes)->dwords.hi_dword >> 20) & 0x7FF) != 0)
                    {

                        dbVTmp1 = ((__constant double *) __datan2_la_CoutTab)[236] + dbRes;
                        dbRes = dbRes * dbVTmp1;

                        (((_iml_v2_dp_union_t *) & dbRes)->dwords.hi_dword =
                         (((_iml_v2_dp_union_t *) & dbRes)->dwords.hi_dword & 0x7FFFFFFF) | ((_iml_uint32_t) (iSignY) << 31));
                        (*r) = dbRes;
                    }
                    else
                    {

                        dbVTmp1 = dbRes * dbRes;
                        dbRes = dbRes + dbVTmp1;

                        (((_iml_v2_dp_union_t *) & dbRes)->dwords.hi_dword =
                         (((_iml_v2_dp_union_t *) & dbRes)->dwords.hi_dword & 0x7FFFFFFF) | ((_iml_uint32_t) (iSignY) << 31));
                        (*r) = dbRes;
                    }
                }
                else
                {

                    dbRes = ((__constant double *) __datan2_la_CoutTab)[242] + ((__constant double *) __datan2_la_CoutTab)[243];

                    (((_iml_v2_dp_union_t *) & dbRes)->dwords.hi_dword =
                     (((_iml_v2_dp_union_t *) & dbRes)->dwords.hi_dword & 0x7FFFFFFF) | ((_iml_uint32_t) (iSignY) << 31));
                    (*r) = dbRes;
                }
            }
        }
        else
        {

            if ((iEY != 0)
                || !(((((_iml_v2_dp_union_t *) & dbY)->dwords.hi_dword & 0x000FFFFF) == 0) && ((((_iml_v2_dp_union_t *) & dbY)->dwords.lo_dword) == 0)))
            {

                dbRes = ((__constant double *) __datan2_la_CoutTab)[240] + ((__constant double *) __datan2_la_CoutTab)[241];

                (((_iml_v2_dp_union_t *) & dbRes)->dwords.hi_dword =
                 (((_iml_v2_dp_union_t *) & dbRes)->dwords.hi_dword & 0x7FFFFFFF) | ((_iml_uint32_t) (iSignY) << 31));
                (*r) = dbRes;

            }
            else
            {

                if (iSignX == 0)
                {

                    dbRes = ((__constant double *) __datan2_la_CoutTab)[247];

                    (((_iml_v2_dp_union_t *) & dbRes)->dwords.hi_dword =
                     (((_iml_v2_dp_union_t *) & dbRes)->dwords.hi_dword & 0x7FFFFFFF) | ((_iml_uint32_t) (iSignY) << 31));
                    (*r) = dbRes;

                }
                else
                {

                    dbRes = ((__constant double *) __datan2_la_CoutTab)[242] + ((__constant double *) __datan2_la_CoutTab)[243];

                    (((_iml_v2_dp_union_t *) & dbRes)->dwords.hi_dword =
                     (((_iml_v2_dp_union_t *) & dbRes)->dwords.hi_dword & 0x7FFFFFFF) | ((_iml_uint32_t) (iSignY) << 31));
                    (*r) = dbRes;

                }
            }
        }
    }
    else
    {

        if (((iEY == 0x7FF)
             && !(((((_iml_v2_dp_union_t *) & dbY)->dwords.hi_dword & 0x000FFFFF) == 0) && ((((_iml_v2_dp_union_t *) & dbY)->dwords.lo_dword) == 0)))
            || ((iEX == 0x7FF)
                && !(((((_iml_v2_dp_union_t *) & dbX)->dwords.hi_dword & 0x000FFFFF) == 0) && ((((_iml_v2_dp_union_t *) & dbX)->dwords.lo_dword) == 0))))
        {

            (*r) = dbY + dbX;
        }
        else
        {

            if ((((((_iml_v2_dp_union_t *) & dbX)->dwords.hi_dword >> 20) & 0x7FF) != 0x7FF))
            {

                dbRes = ((__constant double *) __datan2_la_CoutTab)[240] + ((__constant double *) __datan2_la_CoutTab)[241];

                (((_iml_v2_dp_union_t *) & dbRes)->dwords.hi_dword =
                 (((_iml_v2_dp_union_t *) & dbRes)->dwords.hi_dword & 0x7FFFFFFF) | ((_iml_uint32_t) (iSignY) << 31));
                (*r) = dbRes;
            }
            else
            {

                if ((((((_iml_v2_dp_union_t *) & dbY)->dwords.hi_dword >> 20) & 0x7FF) != 0x7FF))
                {

                    if (iSignX == 0)
                    {

                        dbRes = ((__constant double *) __datan2_la_CoutTab)[247];

                        (((_iml_v2_dp_union_t *) & dbRes)->dwords.hi_dword =
                         (((_iml_v2_dp_union_t *) & dbRes)->dwords.hi_dword & 0x7FFFFFFF) | ((_iml_uint32_t) (iSignY) << 31));
                        (*r) = dbRes;

                    }
                    else
                    {

                        dbRes = ((__constant double *) __datan2_la_CoutTab)[242] + ((__constant double *) __datan2_la_CoutTab)[243];

                        (((_iml_v2_dp_union_t *) & dbRes)->dwords.hi_dword =
                         (((_iml_v2_dp_union_t *) & dbRes)->dwords.hi_dword & 0x7FFFFFFF) | ((_iml_uint32_t) (iSignY) << 31));
                        (*r) = dbRes;

                    }
                }
                else
                {

                    if (iSignX == 0)
                    {

                        dbRes = ((__constant double *) __datan2_la_CoutTab)[238] + ((__constant double *) __datan2_la_CoutTab)[239];

                        (((_iml_v2_dp_union_t *) & dbRes)->dwords.hi_dword =
                         (((_iml_v2_dp_union_t *) & dbRes)->dwords.hi_dword & 0x7FFFFFFF) | ((_iml_uint32_t) (iSignY) << 31));
                        (*r) = dbRes;
                    }
                    else
                    {

                        dbRes = ((__constant double *) __datan2_la_CoutTab)[244] + ((__constant double *) __datan2_la_CoutTab)[245];

                        (((_iml_v2_dp_union_t *) & dbRes)->dwords.hi_dword =
                         (((_iml_v2_dp_union_t *) & dbRes)->dwords.hi_dword & 0x7FFFFFFF) | ((_iml_uint32_t) (iSignY) << 31));
                        (*r) = dbRes;
                    }
                }
            }
        }
    }
    return nRet;
}

double __ocl_svml_atan2 (double a, double b)
{

    double va1;
    double va2;
    double vr1;
    unsigned int vm;

    double r;

    va1 = a;
    va2 = b;

    {

        unsigned int mSpecArgs;
        double dAX;
        double dAY;
        double dSX;
        double dSY;
        unsigned long lAX;
        unsigned long lAY;
        unsigned int iAX;
        unsigned int iAY;
        unsigned int iAX1;
        unsigned int iAY1;
        unsigned int iXBad;
        unsigned int iYBad;
        unsigned int iXYBad;

        double dM;
        double dXNeg;

        double dsMx;
        double dA;
        double dB;
        double dR;
        double dR2;
        double dR4;
        double dR8;
        double dP4;
        double dP3;
        double dP2;
        double dP1;
        double dP;
        double dR0;
        double dE;
        double dInv;

        unsigned int iCHK_WORK_SUB;
        unsigned int iCHK_WORK_CMP;
        double dPI;
        double dPIO2;
        double dA19;
        double dA18;
        double dA17;
        double dA16;
        double dA15;
        double dA14;
        double dA13;
        double dA12;
        double dA11;
        double dA10;
        double dA09;
        double dA08;
        double dA07;
        double dA06;
        double dA05;
        double dA04;
        double dA03;
        double dA02;
        double dA01;
        double dA00;
        double dSIGN_MASK;
        double dABS_MASK;
        double dZERO;
        double dONE;
        dABS_MASK = as_double (__internal_datan2_la_data.dABS_MASK);
        dAX = as_double ((as_ulong (va2) & as_ulong (dABS_MASK)));
        dAY = as_double ((as_ulong (va1) & as_ulong (dABS_MASK)));

        dSIGN_MASK = as_double (__internal_datan2_la_data.dSIGN_MASK);
        dSX = as_double ((as_ulong (va2) & as_ulong (dSIGN_MASK)));
        dSY = as_double ((as_ulong (va1) & as_ulong (dSIGN_MASK)));

        dPIO2 = as_double (__internal_datan2_la_data.dPIO2);
        dsMx = as_double ((as_ulong (dAX) | as_ulong (dSIGN_MASK)));

        dM = as_double ((unsigned long) (((!(dAY < dAX)) ? 0xffffffffffffffff : 0x0)));
        dPIO2 = as_double ((as_ulong (dM) & as_ulong (dPIO2)));
        dA = as_double ((((~as_ulong (dM)) & as_ulong (dAY)) | (as_ulong (dM) & as_ulong (dsMx))));
        dB = as_double ((((~as_ulong (dM)) & as_ulong (dAX)) | (as_ulong (dM) & as_ulong (dAY))));
        dR = (dA / dB);
        lAX = as_ulong (dAX);
        lAY = as_ulong (dAY);
        iAX = ((unsigned int) ((unsigned long) lAX >> 32));
        iAY = ((unsigned int) ((unsigned long) lAY >> 32));
        iCHK_WORK_SUB = (__internal_datan2_la_data.iCHK_WORK_SUB);
        iCHK_WORK_CMP = (__internal_datan2_la_data.iCHK_WORK_CMP);
        iAX1 = (iAX - iCHK_WORK_SUB);
        iAY1 = (iAY - iCHK_WORK_SUB);
        iXBad = ((unsigned int) (-(signed int) ((signed int) iAX1 >= (signed int) iCHK_WORK_CMP)));
        iYBad = ((unsigned int) (-(signed int) ((signed int) iAY1 >= (signed int) iCHK_WORK_CMP)));
        iXYBad = (iXBad | iYBad);

        vm = 0;
        mSpecArgs = 0;
        mSpecArgs = iXYBad;

        dPI = as_double (__internal_datan2_la_data.dPI);
        dZERO = as_double (__internal_datan2_la_data.dZERO);

        dXNeg = as_double ((unsigned long) ((va2 <= dZERO) ? 0xffffffffffffffff : 0x0));
        dPI = as_double ((as_ulong (dXNeg) & as_ulong (dPI)));

        dR2 = (dR * dR);
        dR4 = (dR2 * dR2);

        dR8 = (dR4 * dR4);
        dA19 = as_double (__internal_datan2_la_data.dA19);
        dA18 = as_double (__internal_datan2_la_data.dA18);
        dA17 = as_double (__internal_datan2_la_data.dA17);
        dA16 = as_double (__internal_datan2_la_data.dA16);
        dA15 = as_double (__internal_datan2_la_data.dA15);
        dA14 = as_double (__internal_datan2_la_data.dA14);
        dA13 = as_double (__internal_datan2_la_data.dA13);
        dA12 = as_double (__internal_datan2_la_data.dA12);
        dA11 = as_double (__internal_datan2_la_data.dA11);
        dA10 = as_double (__internal_datan2_la_data.dA10);

        dA09 = as_double (__internal_datan2_la_data.dA09);
        dA08 = as_double (__internal_datan2_la_data.dA08);
        dA07 = as_double (__internal_datan2_la_data.dA07);
        dA06 = as_double (__internal_datan2_la_data.dA06);
        dA05 = as_double (__internal_datan2_la_data.dA05);
        dA04 = as_double (__internal_datan2_la_data.dA04);
        dA03 = as_double (__internal_datan2_la_data.dA03);
        dA02 = as_double (__internal_datan2_la_data.dA02);
        dA01 = as_double (__internal_datan2_la_data.dA01);
        dA00 = as_double (__internal_datan2_la_data.dA00);

        dP1 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dA19, dR8, dA15);
        dP2 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dA18, dR8, dA14);
        dP3 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dA17, dR8, dA13);
        dP4 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dA16, dR8, dA12);

        dP1 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dP1, dR8, dA11);
        dP2 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dP2, dR8, dA10);
        dP3 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dP3, dR8, dA09);
        dP4 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dP4, dR8, dA08);

        dP1 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dP1, dR8, dA07);
        dP2 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dP2, dR8, dA06);
        dP3 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dP3, dR8, dA05);
        dP4 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dP4, dR8, dA04);

        dP1 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dP1, dR8, dA03);
        dP2 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dP2, dR8, dA02);
        dP3 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dP3, dR8, dA01);

        dP4 = (dP4 * dR8);

        dP1 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dP1, dR2, dP2);
        dP3 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dP3, dR2, dP4);
        dP = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dP1, dR4, dP3);

        dP = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dP, dR, dR);
        dP = (dP + dPIO2);
        dP = as_double ((as_ulong (dP) | as_ulong (dSX)));

        dP = (dP + dPI);

        vr1 = as_double ((as_ulong (dP) | as_ulong (dSY)));

        if ((mSpecArgs) != 0)
        {
            double dBZero;
            double dSpecRes;
            double dSpecArgsMask;
            double dXnotNAN;
            double dYnotNAN;
            double dXYnotNAN;
            unsigned long lXYnotNAN;
            unsigned int iXYnotNAN;
            unsigned long lZERO;
            unsigned int iZERO;
            unsigned long lXYBad;
            unsigned long lARG2;
            unsigned int iARG2;
            unsigned int iXNeg;
            unsigned long lXNeg;
            double dAXZERO;
            double dAYZERO;
            double dAXAYZERO;
            unsigned long lAXAYZERO;
            unsigned int iAXAYZERO;
            unsigned int iCallout;
            unsigned int iAXAYZEROnotNAN;
            unsigned long lAXAYZEROnotNAN;
            dXnotNAN = as_double ((unsigned long) (((va2 == va2) & (va2 == va2)) ? 0xffffffffffffffff : 0x0));
            dYnotNAN = as_double ((unsigned long) (((va1 == va1) & (va1 == va1)) ? 0xffffffffffffffff : 0x0));
            dXYnotNAN = as_double ((as_ulong (dXnotNAN) & as_ulong (dYnotNAN)));
            lXYnotNAN = as_ulong (dXYnotNAN);
            iXYnotNAN = ((unsigned int) ((unsigned long) lXYnotNAN >> 32));

            dAXZERO = as_double ((unsigned long) ((dAX == dZERO) ? 0xffffffffffffffff : 0x0));
            dAYZERO = as_double ((unsigned long) ((dAY == dZERO) ? 0xffffffffffffffff : 0x0));
            dAXAYZERO = as_double ((as_ulong (dAXZERO) | as_ulong (dAYZERO)));
            lAXAYZERO = as_ulong (dAXAYZERO);
            iAXAYZERO = ((unsigned int) ((unsigned long) lAXAYZERO >> 32));

            iAXAYZEROnotNAN = (iAXAYZERO & iXYnotNAN);

            iCallout = (~(iAXAYZEROnotNAN) & iXYBad);

            vm = 0;
            vm = iCallout;

            dBZero = as_double ((unsigned long) ((dB == dZERO) ? 0xffffffffffffffff : 0x0));

            dPIO2 = dPIO2;
            dPIO2 = as_double ((((~as_ulong (dBZero)) & as_ulong (dPIO2)) | (as_ulong (dBZero) & as_ulong (dZERO))));
            dP = as_double ((as_ulong (dPIO2) | as_ulong (dSX)));

            lZERO = as_ulong (dZERO);
            iZERO = ((unsigned int) ((unsigned long) lZERO >> 32));
            lARG2 = as_ulong (va2);
            iARG2 = ((unsigned int) ((unsigned long) lARG2 >> 32));

            iXNeg = ((unsigned int) (-(signed int) ((signed int) iARG2 < (signed int) iZERO)));
            lXNeg = (((unsigned long) (unsigned int) iXNeg << 32) | (unsigned long) (unsigned int) iXNeg);
            dXNeg = as_double (lXNeg);
            dPI = as_double ((as_ulong (dXNeg) & as_ulong (dPI)));
            dP = (dP + dPI);

            dSpecRes = as_double ((as_ulong (dP) | as_ulong (dSY)));

            lAXAYZEROnotNAN = (((unsigned long) (unsigned int) iAXAYZEROnotNAN << 32) | (unsigned long) (unsigned int) iAXAYZEROnotNAN);
            dSpecArgsMask = as_double (lAXAYZEROnotNAN);
            vr1 = as_double ((((~as_ulong (dSpecArgsMask)) & as_ulong (vr1)) | (as_ulong (dSpecArgsMask) & as_ulong (dSpecRes))));
        }
    }

    if ((vm) != 0)
    {
        double _vapi_arg1[1];
        double _vapi_arg2[1];
        double _vapi_res1[1];
        ((double *) _vapi_arg1)[0] = va1;
        ((double *) _vapi_arg2)[0] = va2;
        ((double *) _vapi_res1)[0] = vr1;
        __internal_datan2_la_cout (_vapi_arg1, _vapi_arg2, _vapi_res1);
        vr1 = ((double *) _vapi_res1)[0];
    };
    r = vr1;;

    return r;

}

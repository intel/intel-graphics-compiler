/*===================== begin_copyright_notice ==================================

Copyright (c) 2022 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

======================= end_copyright_notice ==================================*/

#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF

typedef struct
{
    unsigned long _dSIGN_MASK;
    unsigned long _dABS_MASK;
    unsigned long _dONE;
    unsigned long _dPIO2;
    unsigned long _dRangeVal;
    unsigned long _dPC9;
    unsigned long _dPC8;
    unsigned long _dPC7;
    unsigned long _dPC6;
    unsigned long _dPC5;
    unsigned long _dPC4;
    unsigned long _dPC3;
    unsigned long _dPC2;
    unsigned long _dPC1;
    unsigned long _dPC0;
} __internal_datan_ep_data_t;
static __constant __internal_datan_ep_data_t __internal_datan_ep_data = {
    0x8000000000000000uL,
    0x7FFFFFFFFFFFFFFFuL,
    0x3FF0000000000000uL,
    0x3FF921FB54442D18uL,
    0x7FF0000000000000uL,
    0xBF5C95B5AB286A02uL,
    0x3F85E36A8CED7474uL,
    0xBF9F791909CB44C4uL,
    0x3FAD6EE2DACE07AFuL,
    0xBFB56FCD7489159AuL,
    0x3FBC0236E7018BA7uL,
    0xBFC2414862594647uL,
    0x3FC999069C08617CuL,
    0xBFD55553391CE4C9uL,
    0x3FEFFFFFFEB5A845uL,
};

static __constant _iml_dp_union_t __datan_ep_CoutTab[242] = {
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
};

__attribute__((always_inline))
inline int __internal_datan_ep_nolut_cout (double *a, double *r)
{
    int nRet = 0;
    double dbX, dbB, dbU, dbTmp1, dbTmp2, dbUHi, dbULo, dbXHi, dbXLo, dbXB1Hi, dbXB1Lo,
        dbXBHi, dbXBLo, dbV1Hi, dbV2Lo, dbV1Lo, dbVHi, dbV3Lo, dbVLo, dbQHi,
        dbQLo, dbT1Hi, dbT1Lo, dbTHi, dbTLo, dbS, dbAtanPoly, dbAHi, dbALo, dbResHi, dbResLo, dbRes;
    double dbVTmp1, dbVTmp2, dbVTmp3;
    int i, iSign, iJ;
    if ((((((_iml_dp_union_t *) & (*a))->dwords.hi_dword >> 20) & 0x7FF) != 0x7FF))
    {
        iSign = (((_iml_dp_union_t *) & (*a))->dwords.hi_dword >> 31);
        dbX = (*a);
        (((_iml_dp_union_t *) & dbX)->dwords.hi_dword = (((_iml_dp_union_t *) & dbX)->dwords.hi_dword & 0x7FFFFFFF) | ((_iml_uint32_t) (0) << 31));
        if (dbX >= ((__constant double *) __datan_ep_CoutTab)[236])
        {
            if (dbX < ((__constant double *) __datan_ep_CoutTab)[237])
            {
                iJ = ((((_iml_dp_union_t *) & (*a))->dwords.hi_dword >> 20) & 0x7FF) << 20;
                iJ = iJ | (((_iml_dp_union_t *) & (*a))->dwords.hi_dword & 0x000FFFFF);
                iJ = iJ - 0x3FC00000;
                iJ = iJ >> 19;
                dbB = dbX;
                (((_iml_dp_union_t *) & dbB)->dwords.lo_dword = (0));
                (((_iml_dp_union_t *) & dbB)->dwords.hi_dword =
                 (((_iml_dp_union_t *) & dbB)->dwords.
                  hi_dword & 0xFFF00000) | ((((((_iml_dp_union_t *) & dbB)->dwords.hi_dword & 0x000FFFFF) & 0x80000) | 0x40000) & 0x000FFFFF));
                dbU = dbX - dbB;
                dbVTmp1 = ((dbU) * (((__constant double *) __datan_ep_CoutTab)[241]));
                dbVTmp2 = (dbVTmp1 - (dbU));
                dbVTmp1 = (dbVTmp1 - dbVTmp2);
                dbVTmp2 = ((dbU) - dbVTmp1);
                dbUHi = dbVTmp1;
                dbULo = dbVTmp2;
                dbVTmp1 = ((dbX) * (((__constant double *) __datan_ep_CoutTab)[241]));
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
                dbVTmp1 = ((((__constant double *) __datan_ep_CoutTab)[238]) + (dbXBHi));
                dbVTmp2 = ((((__constant double *) __datan_ep_CoutTab)[238]) - dbVTmp1);
                dbVTmp3 = (dbVTmp1 + dbVTmp2);
                dbVTmp2 = ((dbXBHi) + dbVTmp2);
                dbVTmp3 = ((((__constant double *) __datan_ep_CoutTab)[238]) - dbVTmp3);
                dbVTmp3 = (dbVTmp2 + dbVTmp3);
                dbV1Hi = dbVTmp1;
                dbV2Lo = dbVTmp3;
                dbV1Lo = dbV2Lo + dbXBLo;
                dbVTmp1 = ((dbV1Hi) * (((__constant double *) __datan_ep_CoutTab)[241]));
                dbVTmp2 = (dbVTmp1 - (dbV1Hi));
                dbVTmp1 = (dbVTmp1 - dbVTmp2);
                dbVTmp2 = ((dbV1Hi) - dbVTmp1);
                dbVHi = dbVTmp1;
                dbV3Lo = dbVTmp2;
                dbVLo = dbV3Lo + dbV1Lo;
                dbTmp1 = (((__constant double *) __datan_ep_CoutTab)[238] / dbVHi);
                dbVTmp2 = (dbTmp1 * ((__constant double *) __datan_ep_CoutTab)[241]);
                dbVTmp3 = (dbVTmp2 - dbTmp1);
                dbVTmp3 = (dbVTmp2 - dbVTmp3);
                dbTmp1 = (dbVHi * dbVTmp3);
                dbTmp1 = (((__constant double *) __datan_ep_CoutTab)[238] - dbTmp1);
                dbVTmp2 = (dbVLo * dbVTmp3);
                dbVTmp2 = (dbTmp1 - dbVTmp2);
                dbTmp1 = (((__constant double *) __datan_ep_CoutTab)[238] + dbVTmp2);
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
                    ((((((((__constant double *) __datan_ep_CoutTab)[234]) * dbS + ((__constant double *) __datan_ep_CoutTab)[233]) * dbS +
                        ((__constant double *) __datan_ep_CoutTab)[232]) * dbS + ((__constant double *) __datan_ep_CoutTab)[231]) * dbS +
                      ((__constant double *) __datan_ep_CoutTab)[230]) * dbS + ((__constant double *) __datan_ep_CoutTab)[229]) * dbS +
                    ((__constant double *) __datan_ep_CoutTab)[228];
                dbVTmp1 = ((((__constant double *) __datan_ep_CoutTab)[0 + 2 * (iJ) + 0]) + (dbTHi));
                dbTmp1 = ((((__constant double *) __datan_ep_CoutTab)[0 + 2 * (iJ) + 0]) - dbVTmp1);
                dbVTmp2 = (dbTmp1 + (dbTHi));
                dbAHi = dbVTmp1;
                dbALo = dbVTmp2;
                dbResHi = dbAHi;
                dbTmp1 = dbAtanPoly * dbS;
                dbTmp1 = dbTmp1 * dbTHi;
                dbTmp1 = dbTmp1 + dbTLo;
                dbTmp1 = dbTmp1 + ((__constant double *) __datan_ep_CoutTab)[0 + 2 * (iJ) + 1];
                dbResLo = dbTmp1 + dbALo;
                dbRes = dbResHi + dbResLo;
                (((_iml_dp_union_t *) & dbRes)->dwords.hi_dword =
                 (((_iml_dp_union_t *) & dbRes)->dwords.hi_dword & 0x7FFFFFFF) | ((_iml_uint32_t) (iSign) << 31));
                (*r) = dbRes;
            }
            else
            {
                dbRes = ((__constant double *) __datan_ep_CoutTab)[239] + ((__constant double *) __datan_ep_CoutTab)[240];
                (((_iml_dp_union_t *) & dbRes)->dwords.hi_dword =
                 (((_iml_dp_union_t *) & dbRes)->dwords.hi_dword & 0x7FFFFFFF) | ((_iml_uint32_t) (iSign) << 31));
                (*r) = dbRes;
            }
        }
        else
        {
            if (dbX >= ((__constant double *) __datan_ep_CoutTab)[235])
            {
                dbS = dbX * dbX;
                dbAtanPoly =
                    ((((((((__constant double *) __datan_ep_CoutTab)[234]) * dbS + ((__constant double *) __datan_ep_CoutTab)[233]) * dbS +
                        ((__constant double *) __datan_ep_CoutTab)[232]) * dbS + ((__constant double *) __datan_ep_CoutTab)[231]) * dbS +
                      ((__constant double *) __datan_ep_CoutTab)[230]) * dbS + ((__constant double *) __datan_ep_CoutTab)[229]) * dbS +
                    ((__constant double *) __datan_ep_CoutTab)[228];
                dbRes = dbAtanPoly * dbS;
                dbRes = dbRes * dbX;
                dbRes = dbRes + dbX;
                (((_iml_dp_union_t *) & dbRes)->dwords.hi_dword =
                 (((_iml_dp_union_t *) & dbRes)->dwords.hi_dword & 0x7FFFFFFF) | ((_iml_uint32_t) (iSign) << 31));
                (*r) = dbRes;
            }
            else
            {
                if (((((_iml_dp_union_t *) & dbX)->dwords.hi_dword >> 20) & 0x7FF) != 0)
                {
                    dbVTmp1 = ((__constant double *) __datan_ep_CoutTab)[238] + dbX;
                    dbRes = dbVTmp1 * dbX;
                    (((_iml_dp_union_t *) & dbRes)->dwords.hi_dword =
                     (((_iml_dp_union_t *) & dbRes)->dwords.hi_dword & 0x7FFFFFFF) | ((_iml_uint32_t) (iSign) << 31));
                    (*r) = dbRes;
                }
                else
                {
                    dbVTmp1 = dbX * dbX;
                    dbRes = dbX + dbVTmp1;
                    (((_iml_dp_union_t *) & dbRes)->dwords.hi_dword =
                     (((_iml_dp_union_t *) & dbRes)->dwords.hi_dword & 0x7FFFFFFF) | ((_iml_uint32_t) (iSign) << 31));
                    (*r) = dbRes;
                }
            }
        }
    }
    else
    {
        if ((((((_iml_dp_union_t *) & (*a))->dwords.hi_dword & 0x000FFFFF) == 0) && ((((_iml_dp_union_t *) & (*a))->dwords.lo_dword) == 0)))
        {
            dbRes = ((__constant double *) __datan_ep_CoutTab)[239] + ((__constant double *) __datan_ep_CoutTab)[240];
            iSign = (((_iml_dp_union_t *) & (*a))->dwords.hi_dword >> 31);
            (((_iml_dp_union_t *) & dbRes)->dwords.hi_dword =
             (((_iml_dp_union_t *) & dbRes)->dwords.hi_dword & 0x7FFFFFFF) | ((_iml_uint32_t) (iSign) << 31));
            (*r) = dbRes;
        }
        else
        {
            (*r) = (*a) + (*a);
        }
    }
    return nRet;
}

double __ocl_svml_atan_ep_noLUT (double a)
{
    double va1;
    double vr1;
    unsigned int vm;
    double r;
    va1 = a;;
    {
        double dAY;
        double dSY;
        double dM;
        double dA;
        double dB;
        double dA1;
        double dY;
        double dR;
        double dR2;
        double dR4;
        double dR8;
        double dPC[22];
        double dP;
        double dP1;
        double dP2;
        double dP3;
        double dP4;
        double dPH;
        double dPL;
        double dRcp;
        double dE;
        double dRangeMask;
        unsigned long lRangeMask;
        double dONE;
        double dPIO2;
        double dSIGN_MASK;
        double dABS_MASK;
        double dRangeVal;
        dABS_MASK = as_double (__internal_datan_ep_data._dABS_MASK);
        dAY = as_double ((as_ulong (va1) & as_ulong (dABS_MASK)));
        dSIGN_MASK = as_double (__internal_datan_ep_data._dSIGN_MASK);
        dSY = as_double ((as_ulong (va1) & as_ulong (dSIGN_MASK)));
        dONE = as_double (__internal_datan_ep_data._dONE);
        dM = as_double ((unsigned long) ((dAY <= dONE) ? 0xffffffffffffffff : 0x0));
        dPIO2 = as_double (__internal_datan_ep_data._dPIO2);
        dPIO2 = as_double ((~(as_ulong (dM)) & as_ulong (dPIO2)));
        dPIO2 = as_double ((as_ulong (dPIO2) ^ as_ulong (dSY)));
        dRangeVal = as_double (__internal_datan_ep_data._dRangeVal);
        dRangeMask = as_double ((unsigned long) ((dAY == dRangeVal) ? 0xffffffffffffffff : 0x0));
        lRangeMask = as_ulong (dRangeMask);
        vm = 0;
        vm = lRangeMask;
        dY = va1;
        dRcp = ((double) (1.0f / ((float) (dY))));
        dE = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (-(dY), dRcp, dONE);
        dE = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dE, dE, dE);
        dRcp = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dE, dRcp, dRcp);
        dRcp = as_double ((as_ulong (dSIGN_MASK) ^ as_ulong (dRcp)));
        dR = as_double ((((~as_ulong (dM)) & as_ulong (dRcp)) | (as_ulong (dM) & as_ulong (dY))));
        dPC[9] = as_double (__internal_datan_ep_data._dPC9);
        dPC[8] = as_double (__internal_datan_ep_data._dPC8);
        dPC[7] = as_double (__internal_datan_ep_data._dPC7);
        dPC[6] = as_double (__internal_datan_ep_data._dPC6);
        dPC[5] = as_double (__internal_datan_ep_data._dPC5);
        dPC[4] = as_double (__internal_datan_ep_data._dPC4);
        dPC[3] = as_double (__internal_datan_ep_data._dPC3);
        dPC[2] = as_double (__internal_datan_ep_data._dPC2);
        dPC[1] = as_double (__internal_datan_ep_data._dPC1);
        dPC[0] = as_double (__internal_datan_ep_data._dPC0);
        dR2 = (dR * dR);
        dR4 = (dR2 * dR2);
        dP2 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dPC[9], dR4, dPC[7]);
        dP1 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dPC[8], dR4, dPC[6]);
        dP2 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dP2, dR4, dPC[5]);
        dP1 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dP1, dR4, dPC[4]);
        dP2 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dP2, dR4, dPC[3]);
        dP1 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dP1, dR4, dPC[2]);
        dP2 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dP2, dR4, dPC[1]);
        dP1 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dP1, dR4, dPC[0]);
        dP = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dP2, dR2, dP1);
        vr1 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dP, dR, dPIO2);
    }
    if ((vm) != 0)
    {
        double _vapi_arg1[1];
        double _vapi_res1[1];
        ((double *) _vapi_arg1)[0] = va1;
        ((double *) _vapi_res1)[0] = vr1;
        __internal_datan_ep_nolut_cout (_vapi_arg1, _vapi_res1);
        vr1 = ((double *) _vapi_res1)[0];
    };
    r = vr1;;
    return r;
}

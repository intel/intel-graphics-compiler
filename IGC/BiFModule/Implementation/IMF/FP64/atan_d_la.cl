/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF
typedef struct
{
    unsigned long AbsMask;
    unsigned long Shifter;
    unsigned long MaxThreshold;
    unsigned long MOne;
    unsigned long One;
    unsigned long LargeX;
    unsigned long Zero;
    unsigned long Tbl_H[32];
    unsigned long Tbl_L[32];
    unsigned long dIndexMed;
    unsigned long Pi2;
    unsigned long Pi2_low;
    unsigned long coeff[6];
} __internal_datan_la_data_avx512_t;
static __constant __internal_datan_la_data_avx512_t __internal_datan_la_data_avx512 = {

    0x7fffffffffffffffuL, 0x4318000000000000uL, 0x401f800000000000uL, 0xbff0000000000000uL, 0x3ff0000000000000uL, 0x47f0000000000000uL,
        0x0000000000000000uL, {
                               0x0000000000000000uL, 0x3fcf5b75f92c80dduL,
                               0x3fddac670561bb4fuL, 0x3fe4978fa3269ee1uL,
                               0x3fe921fb54442d18uL, 0x3fecac7c57846f9euL,
                               0x3fef730bd281f69buL, 0x3ff0d38f2c5ba09fuL,
                               0x3ff1b6e192ebbe44uL, 0x3ff270ef55a53a25uL,
                               0x3ff30b6d796a4da8uL, 0x3ff38d6a6ce13353uL,
                               0x3ff3fc176b7a8560uL, 0x3ff45b54837351a0uL,
                               0x3ff4ae10fc6589a5uL, 0x3ff4f68dea672617uL,
                               0x3ff5368c951e9cfduL, 0x3ff56f6f33a3e6a7uL,
                               0x3ff5a25052114e60uL, 0x3ff5d013c41adabduL,
                               0x3ff5f97315254857uL, 0x3ff61f06c6a92b89uL,
                               0x3ff6414d44094c7cuL, 0x3ff660b02c736a06uL,
                               0x3ff67d8863bc99bduL, 0x3ff698213a9d5053uL,
                               0x3ff6b0bae830c070uL, 0x3ff6c78c7edeb195uL,
                               0x3ff6dcc57bb565fduL, 0x3ff6f08f07435fecuL,
                               0x3ff7030cf9403197uL, 0x3ff7145eac2088a4uL,
                               }
    , {
       0x0000000000000000uL, 0x3c68ab6e3cf7afbduL,
       0x3c7a2b7f222f65e2uL, 0x3c72419a87f2a458uL,
       0x3c81a62633145c07uL, 0x3c80dae13ad18a6buL,
       0x3c7007887af0cbbduL, 0xbc9bd0dc231bfd70uL,
       0x3c9b1b466a88828euL, 0xbc9a66b1af5f84fbuL,
       0x3c96254cb03bb199uL, 0xbc812c77e8a80f5cuL,
       0xbc4441a3bd3f1084uL, 0x3c79e4a72eedacc4uL,
       0xbc93b03e8a27f555uL, 0x3c9934f9f2b0020euL,
       0xbc996f47948a99f1uL, 0xbc7df6edd6f1ec3buL,
       0x3c78c2d0c89de218uL, 0x3c9f82bba194dd5duL,
       0xbc831151a43b51cauL, 0xbc8487d50bceb1a5uL,
       0xbc9c5f60a65c7397uL, 0xbc7acb6afb332a0fuL,
       0xbc99b7bd2e1e8c9cuL, 0xbc9b9839085189e3uL,
       0xbc97d1ab82ffb70buL, 0x3c99239ad620ffe2uL,
       0xbc929c86447928e7uL, 0xbc8957a7170df016uL,
       0xbc7cbe1896221608uL, 0xbc9fda5797b32a0buL,
       }

    , 0x4318000000000010uL, 0x3ff921fb54442d18uL, 0x3c91a62633145c07uL, {

                                                                         0x3fb2e9b9f5c4fe97uL, 0xbfb74257c46790ccuL, 0x3fbc71bfeff916a0uL,
                                                                         0xbfc249248eef04dauL, 0x3fc999999998741euL, 0xbfd555555555554duL}
};

typedef struct
{
    unsigned long _dSIGN_MASK;
    unsigned long _dABS_MASK;
    unsigned long _dONE;
    unsigned long _dPIO2;
    unsigned long _dRangeVal;

    unsigned long _dPC19;
    unsigned long _dPC18;
    unsigned long _dPC17;
    unsigned long _dPC16;
    unsigned long _dPC15;
    unsigned long _dPC14;
    unsigned long _dPC13;
    unsigned long _dPC12;
    unsigned long _dPC11;
    unsigned long _dPC10;

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

} __internal_datan_la_data_t;
static __constant __internal_datan_la_data_t __internal_datan_la_data = {
    0x8000000000000000uL,
    0x7FFFFFFFFFFFFFFFuL,
    0x3FF0000000000000uL,
    0x3FF921FB54442D18uL,
    0x7FF0000000000000uL,
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
};

static __constant _iml_v2_dp_union_t __datan_la_CoutTab[242] = {

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
inline int __internal_datan_la_cout (double *a, double *r)
{
    int nRet = 0;
    double dbX, dbB, dbU, dbTmp1, dbTmp2, dbUHi, dbULo, dbXHi, dbXLo, dbXB1Hi, dbXB1Lo,
        dbXBHi, dbXBLo, dbV1Hi, dbV2Lo, dbV1Lo, dbVHi, dbV3Lo, dbVLo, dbQHi,
        dbQLo, dbT1Hi, dbT1Lo, dbTHi, dbTLo, dbS, dbAtanPoly, dbAHi, dbALo, dbResHi, dbResLo, dbRes;
    double dbVTmp1, dbVTmp2, dbVTmp3;
    int i, iSign, iJ;

    if ((((((_iml_v2_dp_union_t *) & (*a))->dwords.hi_dword >> 20) & 0x7FF) != 0x7FF))
    {

        iSign = (((_iml_v2_dp_union_t *) & (*a))->dwords.hi_dword >> 31);

        dbX = (*a);
        (((_iml_v2_dp_union_t *) & dbX)->dwords.hi_dword = (((_iml_v2_dp_union_t *) & dbX)->dwords.hi_dword & 0x7FFFFFFF) | ((_iml_uint32_t) (0) << 31));

        if (dbX >= ((__constant double *) __datan_la_CoutTab)[236])
        {

            if (dbX < ((__constant double *) __datan_la_CoutTab)[237])
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
                dbVTmp1 = ((dbU) * (((__constant double *) __datan_la_CoutTab)[241]));
                dbVTmp2 = (dbVTmp1 - (dbU));
                dbVTmp1 = (dbVTmp1 - dbVTmp2);
                dbVTmp2 = ((dbU) - dbVTmp1);
                dbUHi = dbVTmp1;
                dbULo = dbVTmp2;

                dbVTmp1 = ((dbX) * (((__constant double *) __datan_la_CoutTab)[241]));
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

                dbVTmp1 = ((((__constant double *) __datan_la_CoutTab)[238]) + (dbXBHi));
                dbVTmp2 = ((((__constant double *) __datan_la_CoutTab)[238]) - dbVTmp1);
                dbVTmp3 = (dbVTmp1 + dbVTmp2);
                dbVTmp2 = ((dbXBHi) + dbVTmp2);
                dbVTmp3 = ((((__constant double *) __datan_la_CoutTab)[238]) - dbVTmp3);
                dbVTmp3 = (dbVTmp2 + dbVTmp3);
                dbV1Hi = dbVTmp1;
                dbV2Lo = dbVTmp3;

                dbV1Lo = dbV2Lo + dbXBLo;

                dbVTmp1 = ((dbV1Hi) * (((__constant double *) __datan_la_CoutTab)[241]));
                dbVTmp2 = (dbVTmp1 - (dbV1Hi));
                dbVTmp1 = (dbVTmp1 - dbVTmp2);
                dbVTmp2 = ((dbV1Hi) - dbVTmp1);
                dbVHi = dbVTmp1;
                dbV3Lo = dbVTmp2;
                dbVLo = dbV3Lo + dbV1Lo;

                dbTmp1 = (((__constant double *) __datan_la_CoutTab)[238] / dbVHi);
                dbVTmp2 = (dbTmp1 * ((__constant double *) __datan_la_CoutTab)[241]);
                dbVTmp3 = (dbVTmp2 - dbTmp1);
                dbVTmp3 = (dbVTmp2 - dbVTmp3);
                dbTmp1 = (dbVHi * dbVTmp3);
                dbTmp1 = (((__constant double *) __datan_la_CoutTab)[238] - dbTmp1);
                dbVTmp2 = (dbVLo * dbVTmp3);
                dbVTmp2 = (dbTmp1 - dbVTmp2);
                dbTmp1 = (((__constant double *) __datan_la_CoutTab)[238] + dbVTmp2);
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
                    ((((((((__constant double *) __datan_la_CoutTab)[234]) * dbS + ((__constant double *) __datan_la_CoutTab)[233]) * dbS +
                        ((__constant double *) __datan_la_CoutTab)[232]) * dbS + ((__constant double *) __datan_la_CoutTab)[231]) * dbS +
                      ((__constant double *) __datan_la_CoutTab)[230]) * dbS + ((__constant double *) __datan_la_CoutTab)[229]) * dbS +
                    ((__constant double *) __datan_la_CoutTab)[228];

                dbVTmp1 = ((((__constant double *) __datan_la_CoutTab)[0 + 2 * (iJ) + 0]) + (dbTHi));
                dbTmp1 = ((((__constant double *) __datan_la_CoutTab)[0 + 2 * (iJ) + 0]) - dbVTmp1);
                dbVTmp2 = (dbTmp1 + (dbTHi));
                dbAHi = dbVTmp1;
                dbALo = dbVTmp2;

                dbResHi = dbAHi;

                dbTmp1 = dbAtanPoly * dbS;
                dbTmp1 = dbTmp1 * dbTHi;
                dbTmp1 = dbTmp1 + dbTLo;
                dbTmp1 = dbTmp1 + ((__constant double *) __datan_la_CoutTab)[0 + 2 * (iJ) + 1];
                dbResLo = dbTmp1 + dbALo;

                dbRes = dbResHi + dbResLo;
                (((_iml_v2_dp_union_t *) & dbRes)->dwords.hi_dword =
                 (((_iml_v2_dp_union_t *) & dbRes)->dwords.hi_dword & 0x7FFFFFFF) | ((_iml_uint32_t) (iSign) << 31));
                (*r) = dbRes;

            }
            else
            {

                dbRes = ((__constant double *) __datan_la_CoutTab)[239] + ((__constant double *) __datan_la_CoutTab)[240];

                (((_iml_v2_dp_union_t *) & dbRes)->dwords.hi_dword =
                 (((_iml_v2_dp_union_t *) & dbRes)->dwords.hi_dword & 0x7FFFFFFF) | ((_iml_uint32_t) (iSign) << 31));
                (*r) = dbRes;

            }
        }
        else
        {

            if (dbX >= ((__constant double *) __datan_la_CoutTab)[235])
            {

                dbS = dbX * dbX;
                dbAtanPoly =
                    ((((((((__constant double *) __datan_la_CoutTab)[234]) * dbS + ((__constant double *) __datan_la_CoutTab)[233]) * dbS +
                        ((__constant double *) __datan_la_CoutTab)[232]) * dbS + ((__constant double *) __datan_la_CoutTab)[231]) * dbS +
                      ((__constant double *) __datan_la_CoutTab)[230]) * dbS + ((__constant double *) __datan_la_CoutTab)[229]) * dbS +
                    ((__constant double *) __datan_la_CoutTab)[228];

                dbRes = dbAtanPoly * dbS;
                dbRes = dbRes * dbX;
                dbRes = dbRes + dbX;

                (((_iml_v2_dp_union_t *) & dbRes)->dwords.hi_dword =
                 (((_iml_v2_dp_union_t *) & dbRes)->dwords.hi_dword & 0x7FFFFFFF) | ((_iml_uint32_t) (iSign) << 31));
                (*r) = dbRes;

            }
            else
            {

                if (((((_iml_v2_dp_union_t *) & dbX)->dwords.hi_dword >> 20) & 0x7FF) != 0)
                {

                    dbVTmp1 = ((__constant double *) __datan_la_CoutTab)[238] + dbX;

                    dbRes = dbVTmp1 * dbX;

                    (((_iml_v2_dp_union_t *) & dbRes)->dwords.hi_dword =
                     (((_iml_v2_dp_union_t *) & dbRes)->dwords.hi_dword & 0x7FFFFFFF) | ((_iml_uint32_t) (iSign) << 31));
                    (*r) = dbRes;

                }
                else
                {

                    dbVTmp1 = dbX * dbX;
                    dbRes = dbX + dbVTmp1;

                    (((_iml_v2_dp_union_t *) & dbRes)->dwords.hi_dword =
                     (((_iml_v2_dp_union_t *) & dbRes)->dwords.hi_dword & 0x7FFFFFFF) | ((_iml_uint32_t) (iSign) << 31));
                    (*r) = dbRes;
                }
            }
        }
    }
    else
    {
        if ((((((_iml_v2_dp_union_t *) & (*a))->dwords.hi_dword & 0x000FFFFF) == 0) && ((((_iml_v2_dp_union_t *) & (*a))->dwords.lo_dword) == 0)))
        {

            dbRes = ((__constant double *) __datan_la_CoutTab)[239] + ((__constant double *) __datan_la_CoutTab)[240];
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

double __ocl_svml_atan (double a)
{

    double va1;
    double vr1;
    unsigned int vm;

    double r;

    va1 = a;;

    {

        double X;
        double X0;
        double SgnX;
        double AbsMask;
        double Y;

        double LargeMask;
        double Tbl2Mask;

        double Shifter;
        double One;
        double Zero;
        double MaxThreshold;
        double dIndex;
        unsigned long lIndex;
        double dIndexMed;
        double DiffX;
        double Rcp;
        double Rcp1;
        double eps;
        double R;
        double Rl;
        double Tbl;
        double Tbl2;
        double R2;
        double R3;
        double R4;
        double Poly;
        double P56;
        double P34;
        double P12;
        double P36;
        double Res;
        double coeff[7];
        double LargeX;
        double Pi2;
        double MOne;

        vm = 0;

        AbsMask = as_double (__internal_datan_la_data_avx512.AbsMask);
        X = as_double ((as_ulong (va1) & as_ulong (AbsMask)));

        Shifter = as_double (__internal_datan_la_data_avx512.Shifter);

        dIndex = (X + Shifter);

        MaxThreshold = as_double (__internal_datan_la_data_avx512.MaxThreshold);

        LargeMask = as_double ((unsigned long) ((X >= MaxThreshold) ? 0xffffffffffffffff : 0x0));

        SgnX = as_double ((as_ulong (X) ^ as_ulong (va1)));

        X0 = (dIndex - Shifter);

        DiffX = (X - X0);

        One = as_double (__internal_datan_la_data_avx512.One);
        Y = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (X, X0, One);

        LargeX = as_double (__internal_datan_la_data_avx512.LargeX);
        X = ((LargeX < X) ? LargeX : X);

        MOne = as_double (__internal_datan_la_data_avx512.MOne);
        Zero = as_double (__internal_datan_la_data_avx512.Zero);

        DiffX = as_double ((((~as_ulong (LargeMask)) & as_ulong (DiffX)) | (as_ulong (LargeMask) & as_ulong (MOne))));
        Y = as_double ((((~as_ulong (LargeMask)) & as_ulong (Y)) | (as_ulong (LargeMask) & as_ulong (X))));
        R = (DiffX / Y);

        lIndex = as_ulong (dIndex);

        {

            unsigned long _t_emask;
            unsigned long _t_index;
            _t_emask = 0x0000000000000078uL;
            _t_index = ((unsigned long) (lIndex) << (3));
            lIndex = (_t_index & _t_emask);
            Tbl = as_double (((__constant unsigned long *) ((__constant double *) (&__internal_datan_la_data_avx512.Tbl_H[0])))[lIndex >> 3]);
            Tbl2 = as_double (((__constant unsigned long *) ((__constant double *) (&__internal_datan_la_data_avx512.Tbl_H[0]) + 16))[lIndex >> 3]);
        }

        dIndexMed = as_double (__internal_datan_la_data_avx512.dIndexMed);
        Tbl2Mask = as_double ((unsigned long) ((dIndex >= dIndexMed) ? 0xffffffffffffffff : 0x0));

        Tbl = as_double ((((~as_ulong (Tbl2Mask)) & as_ulong (Tbl)) | (as_ulong (Tbl2Mask) & as_ulong (Tbl2))));

        Pi2 = as_double (__internal_datan_la_data_avx512.Pi2);
        Tbl = as_double ((((~as_ulong (LargeMask)) & as_ulong (Tbl)) | (as_ulong (LargeMask) & as_ulong (Pi2))));

        R2 = (R * R);
        coeff[6] = as_double (__internal_datan_la_data_avx512.coeff[0]);
        coeff[5] = as_double (__internal_datan_la_data_avx512.coeff[1]);
        P56 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (R2, coeff[6], coeff[5]);
        coeff[4] = as_double (__internal_datan_la_data_avx512.coeff[2]);
        coeff[3] = as_double (__internal_datan_la_data_avx512.coeff[3]);
        P34 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (R2, coeff[4], coeff[3]);
        R4 = (R2 * R2);

        R3 = (R2 * R);

        coeff[2] = as_double (__internal_datan_la_data_avx512.coeff[4]);
        coeff[1] = as_double (__internal_datan_la_data_avx512.coeff[5]);
        P12 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (R2, coeff[2], coeff[1]);

        P36 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (P56, R4, P34);
        Poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (P36, R4, P12);

        Poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (Poly, R3, R);
        Res = (Poly + Tbl);
        vr1 = as_double ((as_ulong (Res) ^ as_ulong (SgnX)));
    }

    if ((vm) != 0)
    {
        double _vapi_arg1[1];
        double _vapi_res1[1];
        ((double *) _vapi_arg1)[0] = va1;
        ((double *) _vapi_res1)[0] = vr1;
        __internal_datan_la_cout (_vapi_arg1, _vapi_res1);
        vr1 = ((double *) _vapi_res1)[0];
    };
    r = vr1;;

    return r;

}

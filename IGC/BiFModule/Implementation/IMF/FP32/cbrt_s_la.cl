/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF
typedef struct
{
    unsigned int etbl_H[16];
    unsigned int etbl_L[16];
    unsigned int cbrt_tbl_H[32];
    unsigned int cbrt_tbl_L[32];
    unsigned int BiasL;
    unsigned int SZero;
    unsigned int OneThird;
    unsigned int Bias3;
    unsigned int Three;
    unsigned int One;
    unsigned int poly_coeff3;
    unsigned int poly_coeff2;
    unsigned int poly_coeff1;
} __internal_scbrt_la_data_avx512_t;
static __constant __internal_scbrt_la_data_avx512_t __internal_scbrt_la_data_avx512 = {
    {
     0x3f800000u,
     0x3fa14518u,
     0x3fcb2ff5u,
     0x00000000u,
     0x00000000u,
     0x00000000u,
     0x00000000u,
     0x00000000u,
     0x00000000u,
     0x00000000u,
     0x00000000u,
     0x00000000u,
     0x00000000u,
     0x00000000u,
     0x00000000u,
     0x00000000u,
     }
    , {
       0x00000000u,
       0xb2ce51afu,
       0x32a7adc8u,
       0x00000000u,
       0x00000000u,
       0x00000000u,
       0x00000000u,
       0x00000000u,
       0x00000000u,
       0x00000000u,
       0x00000000u,
       0x00000000u,
       0x00000000u,
       0x00000000u,
       0x00000000u,
       0x00000000u,
       }
    , {
       0x3fa14518u,
       0x3f9e0b2bu,
       0x3f9b0f9bu,
       0x3f984a9au,
       0x3f95b5afu,
       0x3f934b6cu,
       0x3f910737u,
       0x3f8ee526u,
       0x3f8ce1dau,
       0x3f8afa6au,
       0x3f892c4eu,
       0x3f87754eu,
       0x3f85d377u,
       0x3f844510u,
       0x3f82c892u,
       0x3f815c9fu,
       0x3f800000u,
       0x00000000u,
       0x00000000u,
       0x00000000u,
       0x00000000u,
       0x00000000u,
       0x00000000u,
       0x00000000u,
       0x00000000u,
       0x00000000u,
       0x00000000u,
       0x00000000u,
       0x00000000u,
       0x00000000u,
       0x00000000u,
       0x00000000u,
       }
    , {
       0xb2ce51afu,
       0x32a01a41u,
       0xb257ba40u,
       0x3329ad7du,
       0x335f6b55u,
       0xb2ed5dc4u,
       0xb2262ba0u,
       0xb35c85b1u,
       0xb2e2d14bu,
       0x3288c4e5u,
       0x3207ee10u,
       0x325f925fu,
       0x32bf355bu,
       0x330c3fe3u,
       0xb23159eeu,
       0x32d6123eu,
       0x00000000u,
       0x00000000u,
       0x00000000u,
       0x00000000u,
       0x00000000u,
       0x00000000u,
       0x00000000u,
       0x00000000u,
       0x00000000u,
       0x00000000u,
       0x00000000u,
       0x00000000u,
       0x00000000u,
       0x00000000u,
       0x00000000u,
       0x00000000u,
       }

    , 0x4b400000u, 0x80000000u, 0x3eaaaaabu, 0x4a800000u, 0x40400000u, 0x3f800000u, 0x3d7d057cu, 0xbde3a363u, 0x3eaaaaaau
};

typedef struct
{
    unsigned int _sRcp[32];

    unsigned int _sCbrtHL[96][2];

    unsigned int _sP4;
    unsigned int _sP3;
    unsigned int _sP2;
    unsigned int _sP1;

    unsigned int _sMantissaMask;
    unsigned int _sMantissaMask1;
    unsigned int _sExpMask;
    unsigned int _sExpMask1;
    unsigned int _iRcpIndexMask;
    unsigned int _iBExpMask;
    unsigned int _iSignMask;
    unsigned int _iBias;
    unsigned int _iOne;
    unsigned int _i555;

    unsigned int _iAbsMask;
    unsigned int _iSubConst;
    unsigned int _iCmpConst;

} __internal_scbrt_la_data_t;
static __constant __internal_scbrt_la_data_t __internal_scbrt_la_data = {
    {
     0xBF7C0FC1u,
     0xBF74898Du,
     0xBF6D7304u,
     0xBF66C2B4u,
     0xBF607038u,
     0xBF5A740Eu,
     0xBF54C77Bu,
     0xBF4F6475u,
     0xBF4A4588u,
     0xBF4565C8u,
     0xBF40C0C1u,
     0xBF3C5264u,
     0xBF381703u,
     0xBF340B41u,
     0xBF302C0Bu,
     0xBF2C7692u,
     0xBF28E83Fu,
     0xBF257EB5u,
     0xBF2237C3u,
     0xBF1F1166u,
     0xBF1C09C1u,
     0xBF191F1Au,
     0xBF164FDAu,
     0xBF139A86u,
     0xBF10FDBCu,
     0xBF0E7835u,
     0xBF0C08C1u,
     0xBF09AE41u,
     0xBF0767ABu,
     0xBF053408u,
     0xBF03126Fu,
     0xBF010204u,
     },

    {
     {0x3F80A9C9u,
      0x32075326u},
     {0x3F81F833u,
      0x33B8D172u},
     {0x3F834007u,
      0x32C53ACEu},
     {0x3F848194u,
      0x32E63BD6u},
     {0x3F85BD25u,
      0x338DEB25u},
     {0x3F86F300u,
      0x338127DCu},
     {0x3F882365u,
      0x3382CB80u},
     {0x3F894E90u,
      0x33954DD5u},
     {0x3F8A74B9u,
      0x33B66D7Au},
     {0x3F8B9615u,
      0x32EE5EA1u},
     {0x3F8CB2D4u,
      0x3290003Au},
     {0x3F8DCB24u,
      0x33438C56u},
     {0x3F8EDF31u,
      0x325A8311u},
     {0x3F8FEF22u,
      0x33E5D8FDu},
     {0x3F90FB1Fu,
      0x33FA0482u},
     {0x3F92034Cu,
      0x3346746Eu},
     {0x3F9307CAu,
      0x32A97733u},
     {0x3F9408B9u,
      0x3378E707u},
     {0x3F950638u,
      0x33CDA5B7u},
     {0x3F960064u,
      0x33F9131Fu},
     {0x3F96F759u,
      0x333B7FB8u},
     {0x3F97EB2Fu,
      0x33F8F7DEu},
     {0x3F98DC01u,
      0x337CCF6Au},
     {0x3F99C9E5u,
      0x335C5071u},
     {0x3F9AB4F2u,
      0x32A3C985u},
     {0x3F9B9D3Du,
      0x30AA99BEu},
     {0x3F9C82DAu,
      0x3326D76Du},
     {0x3F9D65DDu,
      0x33873AB6u},
     {0x3F9E4659u,
      0x32B1F468u},
     {0x3F9F245Fu,
      0x302B033Bu},
     {0x3FA00000u,
      0x00000000u},
     {0x3FA0D94Cu,
      0x33547801u},
     {0x3FA21B02u,
      0x33334D9Bu},
     {0x3FA3C059u,
      0x31106571u},
     {0x3FA55D61u,
      0x33F70338u},
     {0x3FA6F282u,
      0x33ED335Cu},
     {0x3FA8801Au,
      0x32029702u},
     {0x3FAA067Eu,
      0x33B7E88Eu},
     {0x3FAB8602u,
      0x32181C5Bu},
     {0x3FACFEEFu,
      0x33BD22FAu},
     {0x3FAE718Eu,
      0x3303539Eu},
     {0x3FAFDE1Fu,
      0x33904B35u},
     {0x3FB144E1u,
      0x3348F1B7u},
     {0x3FB2A60Du,
      0x33A18B6Eu},
     {0x3FB401DAu,
      0x33A51A72u},
     {0x3FB5587Bu,
      0x3390C133u},
     {0x3FB6AA20u,
      0x338C6046u},
     {0x3FB7F6F7u,
      0x30623529u},
     {0x3FB93F29u,
      0x33D27B23u},
     {0x3FBA82E1u,
      0x33888209u},
     {0x3FBBC244u,
      0x338266AAu},
     {0x3FBCFD77u,
      0x325AC94Du},
     {0x3FBE349Bu,
      0x33CFA303u},
     {0x3FBF67D3u,
      0x333BCA39u},
     {0x3FC0973Cu,
      0x33E573F6u},
     {0x3FC1C2F6u,
      0x30878E3Bu},
     {0x3FC2EB1Au,
      0x33E5A0DDu},
     {0x3FC40FC6u,
      0x335E083Eu},
     {0x3FC53112u,
      0x33002B8Bu},
     {0x3FC64F16u,
      0x33D12F2Fu},
     {0x3FC769EBu,
      0x33837081u},
     {0x3FC881A6u,
      0x33B195B7u},
     {0x3FC9965Du,
      0x33BF8679u},
     {0x3FCAA825u,
      0x32A2F7E7u},
     {0x3FCC3D79u,
      0x33A7C5D9u},
     {0x3FCE5054u,
      0x324DECC6u},
     {0x3FD058B8u,
      0x331CC13Du},
     {0x3FD25726u,
      0x3366419Fu},
     {0x3FD44C15u,
      0x32AACE06u},
     {0x3FD637F2u,
      0x338055ACu},
     {0x3FD81B24u,
      0x33BD0AD1u},
     {0x3FD9F60Bu,
      0x328CE2EBu},
     {0x3FDBC8FEu,
      0x33944623u},
     {0x3FDD9452u,
      0x328D277Du},
     {0x3FDF5853u,
      0x33975B72u},
     {0x3FE1154Bu,
      0x33DFAFB5u},
     {0x3FE2CB7Fu,
      0x3376AF98u},
     {0x3FE47B2Eu,
      0x33D632ADu},
     {0x3FE62496u,
      0x339C6747u},
     {0x3FE7C7F0u,
      0x31319368u},
     {0x3FE96571u,
      0x3243E1C2u},
     {0x3FEAFD4Cu,
      0x33E8A904u},
     {0x3FEC8FB3u,
      0x33CDB728u},
     {0x3FEE1CD3u,
      0x3395F918u},
     {0x3FEFA4D7u,
      0x33B460ACu},
     {0x3FF127E9u,
      0x3354C23Cu},
     {0x3FF2A62Fu,
      0x33EE2F64u},
     {0x3FF41FD0u,
      0x338DC0EEu},
     {0x3FF594EEu,
      0x33CEDD79u},
     {0x3FF705ACu,
      0x338C8E9Cu},
     {0x3FF8722Au,
      0x32117930u},
     {0x3FF9DA86u,
      0x32D153BAu},
     {0x3FFB3EDEu,
      0x334F922Eu},
     {0x3FFC9F4Eu,
      0x33D6BD25u},
     {0x3FFDFBF2u,
      0x3368CC9Du},
     {0x3FFF54E3u,
      0x31F23E6Au},
     },
    0xBD288F47u,
    0x3D7CD6EAu,
    0xBDE38E39u,
    0x3EAAAAABu,
    0x007fffffu,
    0x007e0000u,
    0xBF800000u,
    0xBF820000u,

    0x0000007cu,
    0x000000ffu,
    0x00000100u,
    0x00000055u,
    0x00000001u,
    0x00000555u,

    0x7fffffffu,
    0x80800000u,
    0xFEFFFFFFu,

};

typedef struct
{
    unsigned int _sRcp[32][4 / sizeof (unsigned int)];
    unsigned int _sCbrtHL[96][2][4 / sizeof (unsigned int)];

    unsigned int _sP4[16][4 / sizeof (unsigned int)];
    unsigned int _sP3[16][4 / sizeof (unsigned int)];
    unsigned int _sP2[16][4 / sizeof (unsigned int)];
    unsigned int _sP1[16][4 / sizeof (unsigned int)];

    unsigned int _sMantissaMask[16][4 / sizeof (unsigned int)];
    unsigned int _sMantissaMask1[16][4 / sizeof (unsigned int)];
    unsigned int _sExpMask[16][4 / sizeof (unsigned int)];
    unsigned int _sExpMask1[16][4 / sizeof (unsigned int)];
    unsigned int _iRcpIndexMask[16][4 / sizeof (unsigned int)];
    unsigned int _iBExpMask[16][4 / sizeof (unsigned int)];
    unsigned int _iSignMask[16][4 / sizeof (unsigned int)];
    unsigned int _iBias[16][4 / sizeof (unsigned int)];
    unsigned int _iOne[16][4 / sizeof (unsigned int)];

    unsigned int _iAbsMask[16][4 / sizeof (unsigned int)];
    unsigned int _iSubConst[16][4 / sizeof (unsigned int)];
    unsigned int _iCmpConst[16][4 / sizeof (unsigned int)];

} sCbrt_Table_Cout_Type;

static sCbrt_Table_Cout_Type vscbrt_ha_cout_data = {
    {
     {(unsigned int) (0xBF7C0FC1u)},
     {(unsigned int) (0xBF74898Du)},
     {(unsigned int) (0xBF6D7304u)},
     {(unsigned int) (0xBF66C2B4u)},
     {(unsigned int) (0xBF607038u)},
     {(unsigned int) (0xBF5A740Eu)},
     {(unsigned int) (0xBF54C77Bu)},
     {(unsigned int) (0xBF4F6475u)},
     {(unsigned int) (0xBF4A4588u)},
     {(unsigned int) (0xBF4565C8u)},
     {(unsigned int) (0xBF40C0C1u)},
     {(unsigned int) (0xBF3C5264u)},
     {(unsigned int) (0xBF381703u)},
     {(unsigned int) (0xBF340B41u)},
     {(unsigned int) (0xBF302C0Bu)},
     {(unsigned int) (0xBF2C7692u)},
     {(unsigned int) (0xBF28E83Fu)},
     {(unsigned int) (0xBF257EB5u)},
     {(unsigned int) (0xBF2237C3u)},
     {(unsigned int) (0xBF1F1166u)},
     {(unsigned int) (0xBF1C09C1u)},
     {(unsigned int) (0xBF191F1Au)},
     {(unsigned int) (0xBF164FDAu)},
     {(unsigned int) (0xBF139A86u)},
     {(unsigned int) (0xBF10FDBCu)},
     {(unsigned int) (0xBF0E7835u)},
     {(unsigned int) (0xBF0C08C1u)},
     {(unsigned int) (0xBF09AE41u)},
     {(unsigned int) (0xBF0767ABu)},
     {(unsigned int) (0xBF053408u)},
     {(unsigned int) (0xBF03126Fu)},
     {(unsigned int) (0xBF010204u)},
     },

    {
     {{(unsigned int) (0x3F80A9C9u)},
      {(unsigned int) (0x32075326u)}},
     {{(unsigned int) (0x3F81F833u)},
      {(unsigned int) (0x33B8D172u)}},
     {{(unsigned int) (0x3F834007u)},
      {(unsigned int) (0x32C53ACEu)}},
     {{(unsigned int) (0x3F848194u)},
      {(unsigned int) (0x32E63BD6u)}},
     {{(unsigned int) (0x3F85BD25u)},
      {(unsigned int) (0x338DEB25u)}},
     {{(unsigned int) (0x3F86F300u)},
      {(unsigned int) (0x338127DCu)}},
     {{(unsigned int) (0x3F882365u)},
      {(unsigned int) (0x3382CB80u)}},
     {{(unsigned int) (0x3F894E90u)},
      {(unsigned int) (0x33954DD5u)}},
     {{(unsigned int) (0x3F8A74B9u)},
      {(unsigned int) (0x33B66D7Au)}},
     {{(unsigned int) (0x3F8B9615u)},
      {(unsigned int) (0x32EE5EA1u)}},
     {{(unsigned int) (0x3F8CB2D4u)},
      {(unsigned int) (0x3290003Au)}},
     {{(unsigned int) (0x3F8DCB24u)},
      {(unsigned int) (0x33438C56u)}},
     {{(unsigned int) (0x3F8EDF31u)},
      {(unsigned int) (0x325A8311u)}},
     {{(unsigned int) (0x3F8FEF22u)},
      {(unsigned int) (0x33E5D8FDu)}},
     {{(unsigned int) (0x3F90FB1Fu)},
      {(unsigned int) (0x33FA0482u)}},
     {{(unsigned int) (0x3F92034Cu)},
      {(unsigned int) (0x3346746Eu)}},
     {{(unsigned int) (0x3F9307CAu)},
      {(unsigned int) (0x32A97733u)}},
     {{(unsigned int) (0x3F9408B9u)},
      {(unsigned int) (0x3378E707u)}},
     {{(unsigned int) (0x3F950638u)},
      {(unsigned int) (0x33CDA5B7u)}},
     {{(unsigned int) (0x3F960064u)},
      {(unsigned int) (0x33F9131Fu)}},
     {{(unsigned int) (0x3F96F759u)},
      {(unsigned int) (0x333B7FB8u)}},
     {{(unsigned int) (0x3F97EB2Fu)},
      {(unsigned int) (0x33F8F7DEu)}},
     {{(unsigned int) (0x3F98DC01u)},
      {(unsigned int) (0x337CCF6Au)}},
     {{(unsigned int) (0x3F99C9E5u)},
      {(unsigned int) (0x335C5071u)}},
     {{(unsigned int) (0x3F9AB4F2u)},
      {(unsigned int) (0x32A3C985u)}},
     {{(unsigned int) (0x3F9B9D3Du)},
      {(unsigned int) (0x30AA99BEu)}},
     {{(unsigned int) (0x3F9C82DAu)},
      {(unsigned int) (0x3326D76Du)}},
     {{(unsigned int) (0x3F9D65DDu)},
      {(unsigned int) (0x33873AB6u)}},
     {{(unsigned int) (0x3F9E4659u)},
      {(unsigned int) (0x32B1F468u)}},
     {{(unsigned int) (0x3F9F245Fu)},
      {(unsigned int) (0x302B033Bu)}},
     {{(unsigned int) (0x3FA00000u)},
      {(unsigned int) (0x00000000u)}},
     {{(unsigned int) (0x3FA0D94Cu)},
      {(unsigned int) (0x33547801u)}},
     {{(unsigned int) (0x3FA21B02u)},
      {(unsigned int) (0x33334D9Bu)}},
     {{(unsigned int) (0x3FA3C059u)},
      {(unsigned int) (0x31106571u)}},
     {{(unsigned int) (0x3FA55D61u)},
      {(unsigned int) (0x33F70338u)}},
     {{(unsigned int) (0x3FA6F282u)},
      {(unsigned int) (0x33ED335Cu)}},
     {{(unsigned int) (0x3FA8801Au)},
      {(unsigned int) (0x32029702u)}},
     {{(unsigned int) (0x3FAA067Eu)},
      {(unsigned int) (0x33B7E88Eu)}},
     {{(unsigned int) (0x3FAB8602u)},
      {(unsigned int) (0x32181C5Bu)}},
     {{(unsigned int) (0x3FACFEEFu)},
      {(unsigned int) (0x33BD22FAu)}},
     {{(unsigned int) (0x3FAE718Eu)},
      {(unsigned int) (0x3303539Eu)}},
     {{(unsigned int) (0x3FAFDE1Fu)},
      {(unsigned int) (0x33904B35u)}},
     {{(unsigned int) (0x3FB144E1u)},
      {(unsigned int) (0x3348F1B7u)}},
     {{(unsigned int) (0x3FB2A60Du)},
      {(unsigned int) (0x33A18B6Eu)}},
     {{(unsigned int) (0x3FB401DAu)},
      {(unsigned int) (0x33A51A72u)}},
     {{(unsigned int) (0x3FB5587Bu)},
      {(unsigned int) (0x3390C133u)}},
     {{(unsigned int) (0x3FB6AA20u)},
      {(unsigned int) (0x338C6046u)}},
     {{(unsigned int) (0x3FB7F6F7u)},
      {(unsigned int) (0x30623529u)}},
     {{(unsigned int) (0x3FB93F29u)},
      {(unsigned int) (0x33D27B23u)}},
     {{(unsigned int) (0x3FBA82E1u)},
      {(unsigned int) (0x33888209u)}},
     {{(unsigned int) (0x3FBBC244u)},
      {(unsigned int) (0x338266AAu)}},
     {{(unsigned int) (0x3FBCFD77u)},
      {(unsigned int) (0x325AC94Du)}},
     {{(unsigned int) (0x3FBE349Bu)},
      {(unsigned int) (0x33CFA303u)}},
     {{(unsigned int) (0x3FBF67D3u)},
      {(unsigned int) (0x333BCA39u)}},
     {{(unsigned int) (0x3FC0973Cu)},
      {(unsigned int) (0x33E573F6u)}},
     {{(unsigned int) (0x3FC1C2F6u)},
      {(unsigned int) (0x30878E3Bu)}},
     {{(unsigned int) (0x3FC2EB1Au)},
      {(unsigned int) (0x33E5A0DDu)}},
     {{(unsigned int) (0x3FC40FC6u)},
      {(unsigned int) (0x335E083Eu)}},
     {{(unsigned int) (0x3FC53112u)},
      {(unsigned int) (0x33002B8Bu)}},
     {{(unsigned int) (0x3FC64F16u)},
      {(unsigned int) (0x33D12F2Fu)}},
     {{(unsigned int) (0x3FC769EBu)},
      {(unsigned int) (0x33837081u)}},
     {{(unsigned int) (0x3FC881A6u)},
      {(unsigned int) (0x33B195B7u)}},
     {{(unsigned int) (0x3FC9965Du)},
      {(unsigned int) (0x33BF8679u)}},
     {{(unsigned int) (0x3FCAA825u)},
      {(unsigned int) (0x32A2F7E7u)}},
     {{(unsigned int) (0x3FCC3D79u)},
      {(unsigned int) (0x33A7C5D9u)}},
     {{(unsigned int) (0x3FCE5054u)},
      {(unsigned int) (0x324DECC6u)}},
     {{(unsigned int) (0x3FD058B8u)},
      {(unsigned int) (0x331CC13Du)}},
     {{(unsigned int) (0x3FD25726u)},
      {(unsigned int) (0x3366419Fu)}},
     {{(unsigned int) (0x3FD44C15u)},
      {(unsigned int) (0x32AACE06u)}},
     {{(unsigned int) (0x3FD637F2u)},
      {(unsigned int) (0x338055ACu)}},
     {{(unsigned int) (0x3FD81B24u)},
      {(unsigned int) (0x33BD0AD1u)}},
     {{(unsigned int) (0x3FD9F60Bu)},
      {(unsigned int) (0x328CE2EBu)}},
     {{(unsigned int) (0x3FDBC8FEu)},
      {(unsigned int) (0x33944623u)}},
     {{(unsigned int) (0x3FDD9452u)},
      {(unsigned int) (0x328D277Du)}},
     {{(unsigned int) (0x3FDF5853u)},
      {(unsigned int) (0x33975B72u)}},
     {{(unsigned int) (0x3FE1154Bu)},
      {(unsigned int) (0x33DFAFB5u)}},
     {{(unsigned int) (0x3FE2CB7Fu)},
      {(unsigned int) (0x3376AF98u)}},
     {{(unsigned int) (0x3FE47B2Eu)},
      {(unsigned int) (0x33D632ADu)}},
     {{(unsigned int) (0x3FE62496u)},
      {(unsigned int) (0x339C6747u)}},
     {{(unsigned int) (0x3FE7C7F0u)},
      {(unsigned int) (0x31319368u)}},
     {{(unsigned int) (0x3FE96571u)},
      {(unsigned int) (0x3243E1C2u)}},
     {{(unsigned int) (0x3FEAFD4Cu)},
      {(unsigned int) (0x33E8A904u)}},
     {{(unsigned int) (0x3FEC8FB3u)},
      {(unsigned int) (0x33CDB728u)}},
     {{(unsigned int) (0x3FEE1CD3u)},
      {(unsigned int) (0x3395F918u)}},
     {{(unsigned int) (0x3FEFA4D7u)},
      {(unsigned int) (0x33B460ACu)}},
     {{(unsigned int) (0x3FF127E9u)},
      {(unsigned int) (0x3354C23Cu)}},
     {{(unsigned int) (0x3FF2A62Fu)},
      {(unsigned int) (0x33EE2F64u)}},
     {{(unsigned int) (0x3FF41FD0u)},
      {(unsigned int) (0x338DC0EEu)}},
     {{(unsigned int) (0x3FF594EEu)},
      {(unsigned int) (0x33CEDD79u)}},
     {{(unsigned int) (0x3FF705ACu)},
      {(unsigned int) (0x338C8E9Cu)}},
     {{(unsigned int) (0x3FF8722Au)},
      {(unsigned int) (0x32117930u)}},
     {{(unsigned int) (0x3FF9DA86u)},
      {(unsigned int) (0x32D153BAu)}},
     {{(unsigned int) (0x3FFB3EDEu)},
      {(unsigned int) (0x334F922Eu)}},
     {{(unsigned int) (0x3FFC9F4Eu)},
      {(unsigned int) (0x33D6BD25u)}},
     {{(unsigned int) (0x3FFDFBF2u)},
      {(unsigned int) (0x3368CC9Du)}},
     {{(unsigned int) (0x3FFF54E3u)},
      {(unsigned int) (0x31F23E6Au)}},
     },

    {{(unsigned int) (0xBD288F47u)}, {(unsigned int) (0xBD288F47u)}, {(unsigned int) (0xBD288F47u)}, {(unsigned int) (0xBD288F47u)},
     {(unsigned int) (0xBD288F47u)}, {(unsigned int) (0xBD288F47u)}, {(unsigned int) (0xBD288F47u)}, {(unsigned int) (0xBD288F47u)},
     {(unsigned int) (0xBD288F47u)}, {(unsigned int) (0xBD288F47u)}, {(unsigned int) (0xBD288F47u)}, {(unsigned int) (0xBD288F47u)},
     {(unsigned int) (0xBD288F47u)}, {(unsigned int) (0xBD288F47u)}, {(unsigned int) (0xBD288F47u)}, {(unsigned int) (0xBD288F47u)}},
    {{(unsigned int) (0x3D7CD6EAu)}, {(unsigned int) (0x3D7CD6EAu)}, {(unsigned int) (0x3D7CD6EAu)}, {(unsigned int) (0x3D7CD6EAu)},
     {(unsigned int) (0x3D7CD6EAu)}, {(unsigned int) (0x3D7CD6EAu)}, {(unsigned int) (0x3D7CD6EAu)}, {(unsigned int) (0x3D7CD6EAu)},
     {(unsigned int) (0x3D7CD6EAu)}, {(unsigned int) (0x3D7CD6EAu)}, {(unsigned int) (0x3D7CD6EAu)}, {(unsigned int) (0x3D7CD6EAu)},
     {(unsigned int) (0x3D7CD6EAu)}, {(unsigned int) (0x3D7CD6EAu)}, {(unsigned int) (0x3D7CD6EAu)}, {(unsigned int) (0x3D7CD6EAu)}},
    {{(unsigned int) (0xBDE38E39u)}, {(unsigned int) (0xBDE38E39u)}, {(unsigned int) (0xBDE38E39u)}, {(unsigned int) (0xBDE38E39u)},
     {(unsigned int) (0xBDE38E39u)}, {(unsigned int) (0xBDE38E39u)}, {(unsigned int) (0xBDE38E39u)}, {(unsigned int) (0xBDE38E39u)},
     {(unsigned int) (0xBDE38E39u)}, {(unsigned int) (0xBDE38E39u)}, {(unsigned int) (0xBDE38E39u)}, {(unsigned int) (0xBDE38E39u)},
     {(unsigned int) (0xBDE38E39u)}, {(unsigned int) (0xBDE38E39u)}, {(unsigned int) (0xBDE38E39u)}, {(unsigned int) (0xBDE38E39u)}},
    {{(unsigned int) (0x3EAAAAABu)}, {(unsigned int) (0x3EAAAAABu)}, {(unsigned int) (0x3EAAAAABu)}, {(unsigned int) (0x3EAAAAABu)},
     {(unsigned int) (0x3EAAAAABu)}, {(unsigned int) (0x3EAAAAABu)}, {(unsigned int) (0x3EAAAAABu)}, {(unsigned int) (0x3EAAAAABu)},
     {(unsigned int) (0x3EAAAAABu)}, {(unsigned int) (0x3EAAAAABu)}, {(unsigned int) (0x3EAAAAABu)}, {(unsigned int) (0x3EAAAAABu)},
     {(unsigned int) (0x3EAAAAABu)}, {(unsigned int) (0x3EAAAAABu)}, {(unsigned int) (0x3EAAAAABu)}, {(unsigned int) (0x3EAAAAABu)}},

    {{(unsigned int) (0x007fffffu)}, {(unsigned int) (0x007fffffu)}, {(unsigned int) (0x007fffffu)}, {(unsigned int) (0x007fffffu)},
     {(unsigned int) (0x007fffffu)}, {(unsigned int) (0x007fffffu)}, {(unsigned int) (0x007fffffu)}, {(unsigned int) (0x007fffffu)},
     {(unsigned int) (0x007fffffu)}, {(unsigned int) (0x007fffffu)}, {(unsigned int) (0x007fffffu)}, {(unsigned int) (0x007fffffu)},
     {(unsigned int) (0x007fffffu)}, {(unsigned int) (0x007fffffu)}, {(unsigned int) (0x007fffffu)}, {(unsigned int) (0x007fffffu)}},
    {{(unsigned int) (0x007e0000u)}, {(unsigned int) (0x007e0000u)}, {(unsigned int) (0x007e0000u)}, {(unsigned int) (0x007e0000u)},
     {(unsigned int) (0x007e0000u)}, {(unsigned int) (0x007e0000u)}, {(unsigned int) (0x007e0000u)}, {(unsigned int) (0x007e0000u)},
     {(unsigned int) (0x007e0000u)}, {(unsigned int) (0x007e0000u)}, {(unsigned int) (0x007e0000u)}, {(unsigned int) (0x007e0000u)},
     {(unsigned int) (0x007e0000u)}, {(unsigned int) (0x007e0000u)}, {(unsigned int) (0x007e0000u)}, {(unsigned int) (0x007e0000u)}},
    {{(unsigned int) (0xBF800000u)}, {(unsigned int) (0xBF800000u)}, {(unsigned int) (0xBF800000u)}, {(unsigned int) (0xBF800000u)},
     {(unsigned int) (0xBF800000u)}, {(unsigned int) (0xBF800000u)}, {(unsigned int) (0xBF800000u)}, {(unsigned int) (0xBF800000u)},
     {(unsigned int) (0xBF800000u)}, {(unsigned int) (0xBF800000u)}, {(unsigned int) (0xBF800000u)}, {(unsigned int) (0xBF800000u)},
     {(unsigned int) (0xBF800000u)}, {(unsigned int) (0xBF800000u)}, {(unsigned int) (0xBF800000u)}, {(unsigned int) (0xBF800000u)}},
    {{(unsigned int) (0xBF820000u)}, {(unsigned int) (0xBF820000u)}, {(unsigned int) (0xBF820000u)}, {(unsigned int) (0xBF820000u)},
     {(unsigned int) (0xBF820000u)}, {(unsigned int) (0xBF820000u)}, {(unsigned int) (0xBF820000u)}, {(unsigned int) (0xBF820000u)},
     {(unsigned int) (0xBF820000u)}, {(unsigned int) (0xBF820000u)}, {(unsigned int) (0xBF820000u)}, {(unsigned int) (0xBF820000u)},
     {(unsigned int) (0xBF820000u)}, {(unsigned int) (0xBF820000u)}, {(unsigned int) (0xBF820000u)}, {(unsigned int) (0xBF820000u)}},

    {{(unsigned int) (0x0000007cu)}, {(unsigned int) (0x0000007cu)}, {(unsigned int) (0x0000007cu)}, {(unsigned int) (0x0000007cu)},
     {(unsigned int) (0x0000007cu)}, {(unsigned int) (0x0000007cu)}, {(unsigned int) (0x0000007cu)}, {(unsigned int) (0x0000007cu)},
     {(unsigned int) (0x0000007cu)}, {(unsigned int) (0x0000007cu)}, {(unsigned int) (0x0000007cu)}, {(unsigned int) (0x0000007cu)},
     {(unsigned int) (0x0000007cu)}, {(unsigned int) (0x0000007cu)}, {(unsigned int) (0x0000007cu)}, {(unsigned int) (0x0000007cu)}},
    {{(unsigned int) (0x000000ffu)}, {(unsigned int) (0x000000ffu)}, {(unsigned int) (0x000000ffu)}, {(unsigned int) (0x000000ffu)},
     {(unsigned int) (0x000000ffu)}, {(unsigned int) (0x000000ffu)}, {(unsigned int) (0x000000ffu)}, {(unsigned int) (0x000000ffu)},
     {(unsigned int) (0x000000ffu)}, {(unsigned int) (0x000000ffu)}, {(unsigned int) (0x000000ffu)}, {(unsigned int) (0x000000ffu)},
     {(unsigned int) (0x000000ffu)}, {(unsigned int) (0x000000ffu)}, {(unsigned int) (0x000000ffu)}, {(unsigned int) (0x000000ffu)}},
    {{(unsigned int) (0x00000100u)}, {(unsigned int) (0x00000100u)}, {(unsigned int) (0x00000100u)}, {(unsigned int) (0x00000100u)},
     {(unsigned int) (0x00000100u)}, {(unsigned int) (0x00000100u)}, {(unsigned int) (0x00000100u)}, {(unsigned int) (0x00000100u)},
     {(unsigned int) (0x00000100u)}, {(unsigned int) (0x00000100u)}, {(unsigned int) (0x00000100u)}, {(unsigned int) (0x00000100u)},
     {(unsigned int) (0x00000100u)}, {(unsigned int) (0x00000100u)}, {(unsigned int) (0x00000100u)}, {(unsigned int) (0x00000100u)}},
    {{(unsigned int) (0x00000055u)}, {(unsigned int) (0x00000055u)}, {(unsigned int) (0x00000055u)}, {(unsigned int) (0x00000055u)},
     {(unsigned int) (0x00000055u)}, {(unsigned int) (0x00000055u)}, {(unsigned int) (0x00000055u)}, {(unsigned int) (0x00000055u)},
     {(unsigned int) (0x00000055u)}, {(unsigned int) (0x00000055u)}, {(unsigned int) (0x00000055u)}, {(unsigned int) (0x00000055u)},
     {(unsigned int) (0x00000055u)}, {(unsigned int) (0x00000055u)}, {(unsigned int) (0x00000055u)}, {(unsigned int) (0x00000055u)}},
    {{(unsigned int) (0x00000001u)}, {(unsigned int) (0x00000001u)}, {(unsigned int) (0x00000001u)}, {(unsigned int) (0x00000001u)},
     {(unsigned int) (0x00000001u)}, {(unsigned int) (0x00000001u)}, {(unsigned int) (0x00000001u)}, {(unsigned int) (0x00000001u)},
     {(unsigned int) (0x00000001u)}, {(unsigned int) (0x00000001u)}, {(unsigned int) (0x00000001u)}, {(unsigned int) (0x00000001u)},
     {(unsigned int) (0x00000001u)}, {(unsigned int) (0x00000001u)}, {(unsigned int) (0x00000001u)}, {(unsigned int) (0x00000001u)}},

    {{(unsigned int) (0x7fffffffu)}, {(unsigned int) (0x7fffffffu)}, {(unsigned int) (0x7fffffffu)}, {(unsigned int) (0x7fffffffu)},
     {(unsigned int) (0x7fffffffu)}, {(unsigned int) (0x7fffffffu)}, {(unsigned int) (0x7fffffffu)}, {(unsigned int) (0x7fffffffu)},
     {(unsigned int) (0x7fffffffu)}, {(unsigned int) (0x7fffffffu)}, {(unsigned int) (0x7fffffffu)}, {(unsigned int) (0x7fffffffu)},
     {(unsigned int) (0x7fffffffu)}, {(unsigned int) (0x7fffffffu)}, {(unsigned int) (0x7fffffffu)}, {(unsigned int) (0x7fffffffu)}},
    {{(unsigned int) (0x80800000u)}, {(unsigned int) (0x80800000u)}, {(unsigned int) (0x80800000u)}, {(unsigned int) (0x80800000u)},
     {(unsigned int) (0x80800000u)}, {(unsigned int) (0x80800000u)}, {(unsigned int) (0x80800000u)}, {(unsigned int) (0x80800000u)},
     {(unsigned int) (0x80800000u)}, {(unsigned int) (0x80800000u)}, {(unsigned int) (0x80800000u)}, {(unsigned int) (0x80800000u)},
     {(unsigned int) (0x80800000u)}, {(unsigned int) (0x80800000u)}, {(unsigned int) (0x80800000u)}, {(unsigned int) (0x80800000u)}},
    {{(unsigned int) (0xFEFFFFFFu)}, {(unsigned int) (0xFEFFFFFFu)}, {(unsigned int) (0xFEFFFFFFu)}, {(unsigned int) (0xFEFFFFFFu)},
     {(unsigned int) (0xFEFFFFFFu)}, {(unsigned int) (0xFEFFFFFFu)}, {(unsigned int) (0xFEFFFFFFu)}, {(unsigned int) (0xFEFFFFFFu)},
     {(unsigned int) (0xFEFFFFFFu)}, {(unsigned int) (0xFEFFFFFFu)}, {(unsigned int) (0xFEFFFFFFu)}, {(unsigned int) (0xFEFFFFFFu)},
     {(unsigned int) (0xFEFFFFFFu)}, {(unsigned int) (0xFEFFFFFFu)}, {(unsigned int) (0xFEFFFFFFu)}, {(unsigned int) (0xFEFFFFFFu)}},

};

static __constant unsigned int ione[2] = { 0x3f800000, 0xbf800000 };

__attribute__((always_inline))
inline int __internal_scbrt_la_cout (float *a, float *r)
{
    int nRet = 0;
    float fA;
    __constant float *fone = (__constant float *) ione;

    fA = ((*a) * fone[0]);

    if ((((((_iml_v2_sp_union_t *) & (fA))->hex[0] >> 23) & 0xFF) != 0xFF))
    {

        if ((fA) != 0.0f)
        {
            float fR, fScale, fUnscale;
            float sRcp;
            float sY, sY1, sMantissaMask, sMantissaMask1, sExpMask;
            float sExpMask1, s2k, sP, sCbrtHiR, sCbrtHL[2];
            float sR, sP4, sP3, sP2, sP1;

            unsigned int iRangeMask, iRcpIndex, iX, iHiX, iRcpIndexMask, iSignedBiasedExp, iAbsBiasedExp;
            unsigned int iAbsBiasedExpm1, iBExpMask, i5Exp, i50Exp, i55Exp, i555Exp, iSign;
            unsigned int iSignMask, iOne, iK, iJ, iBias, iCbrtIndex, iIndexMask;
            unsigned int iAbsX, iAbsMask, iSubConst, iCmpConst;

            if (((((_iml_v2_sp_union_t *) & (fA))->hex[0] >> 23) & 0xFF) == 0)
            {
                *(unsigned int *) &fScale = 0x7E800000;
                *(unsigned int *) &fUnscale = 0x2A800000;
            }
            else
            {
                *(unsigned int *) &fScale = 0x3f800000;
                *(unsigned int *) &fUnscale = 0x3f800000;
            }

            fA = fA * fScale;

            iRcpIndexMask = (*(unsigned int *) &vscbrt_ha_cout_data._iRcpIndexMask);
            iBExpMask = (*(unsigned int *) &vscbrt_ha_cout_data._iBExpMask);
            iSignMask = (*(unsigned int *) &vscbrt_ha_cout_data._iSignMask);
            iOne = (*(unsigned int *) &vscbrt_ha_cout_data._iOne);
            iBias = (*(unsigned int *) &vscbrt_ha_cout_data._iBias);

            sMantissaMask = (*(float *) &vscbrt_ha_cout_data._sMantissaMask);
            sMantissaMask1 = (*(float *) &vscbrt_ha_cout_data._sMantissaMask1);

            sExpMask = (*(float *) &vscbrt_ha_cout_data._sExpMask);
            sExpMask1 = (*(float *) &vscbrt_ha_cout_data._sExpMask1);

            iAbsMask = (*(unsigned int *) &vscbrt_ha_cout_data._iAbsMask);
            iSubConst = (*(unsigned int *) &vscbrt_ha_cout_data._iSubConst);
            iCmpConst = (*(unsigned int *) &vscbrt_ha_cout_data._iCmpConst);

            (*((float *) &(iX))) = (fA);
            iHiX = ((unsigned int) iX >> 16);
            iRcpIndex = (iHiX & iRcpIndexMask);

            sRcp = *((float *) (((char *) (vscbrt_ha_cout_data._sRcp)) + iRcpIndex));

            iSignedBiasedExp = ((unsigned int) iHiX >> 7);

            iAbsBiasedExp = (iSignedBiasedExp & iBExpMask);

            iAbsX = (iX & iAbsMask);
            iAbsX = (iAbsX - iSubConst);
            iRangeMask = ((unsigned int) (-(signed int) ((signed int) iAbsX > (signed int) iCmpConst)));

            (*((unsigned int *) &(sY))) = ((*((unsigned int *) &fA)) & (*((unsigned int *) &(sMantissaMask))));
            (*((unsigned int *) &(sY1))) = ((*((unsigned int *) &fA)) & (*((unsigned int *) &(sMantissaMask1))));
            (*((unsigned int *) &(sY))) = ((*((unsigned int *) &(sY))) | (*((unsigned int *) &(sExpMask))));
            (*((unsigned int *) &(sY1))) = ((*((unsigned int *) &(sY1))) | (*((unsigned int *) &(sExpMask1))));

            sR = (sY - sY1);

            sR = (sR * sRcp);

            i5Exp = (iAbsBiasedExp + iAbsBiasedExp);
            i5Exp = (i5Exp + i5Exp);
            i5Exp = (i5Exp + iAbsBiasedExp);

            i50Exp = (i5Exp + i5Exp);
            i50Exp = (i50Exp + i50Exp);
            i50Exp = (i50Exp + i50Exp);
            i50Exp = (i50Exp + i50Exp);
            i55Exp = (i50Exp + i5Exp);

            i555Exp = (i50Exp + i50Exp);
            i555Exp = (i555Exp + i555Exp);
            i555Exp = (i555Exp + i555Exp);
            i555Exp = (i555Exp + i555Exp);
            i555Exp = (i555Exp + i55Exp);

            iSign = (iSignedBiasedExp & iSignMask);
            iAbsBiasedExpm1 = (iAbsBiasedExp - iOne);

            iK = ((unsigned int) i555Exp >> 12);

            iJ = (iAbsBiasedExpm1 - iK);
            iJ = (iJ - iK);
            iJ = (iJ - iK);

            iJ = ((unsigned int) iJ << 7);

            iK = (iK + iBias);

            iK = (iK | iSign);
            iK = ((unsigned int) iK << 23);
            (*((unsigned int *) &(s2k))) = (iK);

            iCbrtIndex = (iRcpIndex + iJ);

            iCbrtIndex = (~(iRangeMask) & iCbrtIndex);

            iCbrtIndex = ((unsigned int) iCbrtIndex << 1);
            sCbrtHL[0] = *((float *) (((char *) (vscbrt_ha_cout_data._sCbrtHL)) + iCbrtIndex));
            sCbrtHL[1] = *((float *) (((char *) (vscbrt_ha_cout_data._sCbrtHL)) + iCbrtIndex + 4));

            sCbrtHL[0] = (sCbrtHL[0] * s2k);

            sCbrtHL[1] = (sCbrtHL[1] * s2k);

            sP4 = (*(float *) &vscbrt_ha_cout_data._sP4);
            sP3 = (*(float *) &vscbrt_ha_cout_data._sP3);
            sP = ((sP4 * sR) + sP3);
            sP2 = (*(float *) &vscbrt_ha_cout_data._sP2);
            sP = ((sP * sR) + sP2);
            sP1 = (*(float *) &vscbrt_ha_cout_data._sP1);
            sP = ((sP * sR) + sP1);

            sCbrtHiR = (sCbrtHL[0] * sR);

            sP = (sP * sCbrtHiR);

            sP = (sP + sCbrtHL[1]);

            fR = (sP + sCbrtHL[0]);

            fR = fR * fUnscale;

            (*r) = fR;
        }
        else
        {

            (*r) = (fA);
        }
    }
    else
    {

        (*r) = (*a) + (*a);
    }

    return nRet;
}

float __ocl_svml_cbrtf (float a)
{

    float va1;
    float vr1;
    unsigned int vm;

    float r;

    va1 = a;;

    {

        unsigned int iRangeMask;
        unsigned int iRcpIndex;
        unsigned int iX;
        unsigned int iHiX;
        unsigned int iRcpIndexMask;
        float sRcp;
        unsigned int iSignedBiasedExp;
        unsigned int iAbsBiasedExp;
        unsigned int iAbsBiasedExpm1;
        unsigned int iBExpMask;
        unsigned int i5Exp;
        unsigned int i50Exp;
        unsigned int i55Exp;
        unsigned int i555Exp;
        unsigned int iSign;
        unsigned int iSignMask;
        unsigned int iOne;
        unsigned int i555;
        unsigned int iK;
        unsigned int iJ;
        unsigned int iBias;
        unsigned int iCbrtIndex;
        unsigned int iIndexMask;

        float sY;
        float sY1;
        float sMantissaMask;
        float sMantissaMask1;
        float sExpMask;
        float sExpMask1;
        float s2k;
        float sP;
        float sCbrtHiR;
        float sCbrtHL[2];

        float sR;
        float sP4;
        float sP3;
        float sP2;
        float sP1;

        unsigned int iAbsX;
        unsigned int iAbsMask;
        unsigned int iSubConst;
        unsigned int iCmpConst;

        iRcpIndexMask = (__internal_scbrt_la_data._iRcpIndexMask);
        iBExpMask = (__internal_scbrt_la_data._iBExpMask);
        iSignMask = (__internal_scbrt_la_data._iSignMask);
        iOne = (__internal_scbrt_la_data._iOne);
        iBias = (__internal_scbrt_la_data._iBias);

        sMantissaMask = as_float (__internal_scbrt_la_data._sMantissaMask);
        sMantissaMask1 = as_float (__internal_scbrt_la_data._sMantissaMask1);

        sExpMask = as_float (__internal_scbrt_la_data._sExpMask);
        sExpMask1 = as_float (__internal_scbrt_la_data._sExpMask1);

        iAbsMask = (__internal_scbrt_la_data._iAbsMask);
        iSubConst = (__internal_scbrt_la_data._iSubConst);
        iCmpConst = (__internal_scbrt_la_data._iCmpConst);

        iX = as_uint (va1);
        iHiX = ((unsigned int) (iX) >> (16));
        iRcpIndex = (iHiX & iRcpIndexMask);

        sRcp = as_float (((__constant unsigned int *) (__internal_scbrt_la_data._sRcp))[iRcpIndex >> 2]);

        iSignedBiasedExp = ((unsigned int) (iHiX) >> (7));

        iAbsBiasedExp = (iSignedBiasedExp & iBExpMask);

        iAbsX = (iX & iAbsMask);
        iAbsX = (iAbsX - iSubConst);
        iRangeMask = ((unsigned int) (-(signed int) ((signed int) iAbsX > (signed int) iCmpConst)));
        vm = 0;
        vm = iRangeMask;

        sY = as_float ((as_uint (va1) & as_uint (sMantissaMask)));
        sY1 = as_float ((as_uint (va1) & as_uint (sMantissaMask1)));
        sY = as_float ((as_uint (sY) | as_uint (sExpMask)));
        sY1 = as_float ((as_uint (sY1) | as_uint (sExpMask1)));

        sR = (sY - sY1);

        sR = (sR * sRcp);

        i555 = (__internal_scbrt_la_data._i555);
        i555Exp = (iAbsBiasedExp * i555);

        iSign = (iSignedBiasedExp & iSignMask);
        iAbsBiasedExpm1 = (iAbsBiasedExp - iOne);

        iK = ((unsigned int) (i555Exp) >> (12));

        iJ = (iAbsBiasedExpm1 - iK);
        iJ = (iJ - iK);
        iJ = (iJ - iK);

        iJ = ((unsigned int) (iJ) << (7));

        iK = (iK + iBias);

        iK = (iK | iSign);
        iK = ((unsigned int) (iK) << (23));
        s2k = as_float (iK);

        iCbrtIndex = (iRcpIndex + iJ);

        iCbrtIndex = (~(iRangeMask) & iCbrtIndex);

        iCbrtIndex = ((unsigned int) (iCbrtIndex) << (1));
        sCbrtHL[0] = as_float (((__constant unsigned int *) (__internal_scbrt_la_data._sCbrtHL))[iCbrtIndex >> 2]);
        sCbrtHL[1] = as_float (((__constant unsigned int *) (__internal_scbrt_la_data._sCbrtHL))[(iCbrtIndex >> 2) + 1]);

        sCbrtHL[0] = (sCbrtHL[0] * s2k);

        sCbrtHL[1] = (sCbrtHL[1] * s2k);

        sP3 = as_float (__internal_scbrt_la_data._sP3);
        sP2 = as_float (__internal_scbrt_la_data._sP2);
        sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sP3, sR, sP2);
        sP1 = as_float (__internal_scbrt_la_data._sP1);
        sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sP, sR, sP1);
        sCbrtHiR = (sCbrtHL[0] * sR);

        sP = (sP * sCbrtHiR);

        sP = (sP + sCbrtHL[1]);

        vr1 = (sP + sCbrtHL[0]);
    }

    if ((vm) != 0)
    {
        float _vapi_arg1[1];
        float _vapi_res1[1];
        ((float *) _vapi_arg1)[0] = va1;
        ((float *) _vapi_res1)[0] = vr1;
        __internal_scbrt_la_cout (_vapi_arg1, _vapi_res1);
        vr1 = ((float *) _vapi_res1)[0];
    };
    r = vr1;;

    return r;

}

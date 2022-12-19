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
    unsigned int Exp_tbl_H[16];
    unsigned int C15;
    unsigned int One;
    unsigned int poly_coeff5;
    unsigned int poly_coeff4;
    unsigned int poly_coeff3;
    unsigned int poly_coeff2;
    unsigned int poly_coeff1;
    unsigned int poly_c1h;
    unsigned int Shifter;
    unsigned int EMask;
    unsigned int poly_e_coeff3;
    unsigned int poly_e_coeff2;
    unsigned int poly_e_coeff1;

    unsigned int Zero;
    unsigned int LIndexMask;
    unsigned int EIndexMask;
    unsigned int dAbsMask;
    unsigned int dDomainRange;
    unsigned int ExpMask;
} __internal_spowr_la_data_avx512_t;
static __constant __internal_spowr_la_data_avx512_t __internal_spowr_la_data_avx512 = {
    {
     0x00000000u, 0xbd35d000u, 0xbdb32000u, 0xbe046400u, 0xbe2e0000u, 0xbe567c00u, 0xbe7de000u, 0xbe922000u, 0xbea4d400u, 0xbeb71200u, 0xbec8de00u,
     0xbeda4000u, 0xbeeb3a00u, 0xbefbd400u, 0xbf060800u, 0xbf0dfa00u, 0x3ed48000u, 0x3ec54400u, 0x3eb65800u, 0x3ea7b800u, 0x3e996000u, 0x3e8b4e00u,
     0x3e7b0000u, 0x3e5fe400u, 0x3e454400u, 0x3e2b2000u, 0x3e116c00u, 0x3df05800u, 0x3dbeb000u, 0x3d8dd800u, 0x3d3ba000u, 0x3cba2000u}
    , {
       0x00000000u, 0xb6d3758fu, 0x3510536fu, 0x369dcc96u, 0xb651cfdfu, 0x3687492cu, 0xb635c813u, 0xb5f561c1u, 0x35f6865du, 0x36f19318u, 0x35aedc1du,
       0x36a0463cu, 0xb69f0197u, 0xb5ad1961u, 0xb6201ac7u, 0x36ee16a3u, 0xb5d1cfdfu, 0x36c055feu, 0x3676865du, 0xb589a627u, 0xb48e4789u, 0x33a6c7e3u,
       0xb69d5be7u, 0xb642c000u, 0x364055feu, 0xb68f5801u, 0x36b70aadu, 0x35d74798u, 0x3492d9f7u, 0x364a9801u, 0xb6566c4du, 0xb48bcf06u}
    , {
       0x3f800000u, 0x3f85aac3u, 0x3f8b95c2u, 0x3f91c3d3u, 0x3f9837f0u, 0x3f9ef532u, 0x3fa5fed7u, 0x3fad583fu, 0x3fb504f3u, 0x3fbd08a4u, 0x3fc5672au,
       0x3fce248cu, 0x3fd744fdu, 0x3fe0ccdfu, 0x3feac0c7u, 0x3ff5257du}

    , 0x3fc00000u, 0x3f800000u, 0x3E93C705u, 0xBEB8B3EDu, 0x3EF6384Fu, 0xBF38AA3Bu, 0x32A570CCu, 0x3FB8AA3Bu, 0x494007f0u, 0xbfffffffu, 0x3d6854cbu,
        0x3e75f16cu, 0x3f317222u, 0x00000000u, 0x0000007cu, 0x0000003cu, 0x7fffffffu, 0x42fb0000u, 0x7f800000u
};

typedef struct
{

    unsigned int hsw_sMinNorm;
    unsigned int hsw_sMaxNorm;
    unsigned int hsw_RndBit;
    unsigned int hsw_s_RndMask;
    unsigned int hsw_sOne;

    unsigned int hsw_sTh[16];
    unsigned int hsw_sTl[16];

    unsigned int hsw_sc7;
    unsigned int hsw_sc6;
    unsigned int hsw_sc5;
    unsigned int hsw_sc4;
    unsigned int hsw_sc1h;
    unsigned int hsw_sc2;
    unsigned int hsw_sc1;
    unsigned int hsw_sc2h;
    unsigned int hsw_sc3;

    unsigned int hsw_sAbsMask;
    unsigned int hsw_sDomainRange;
    unsigned int hsw_sShifter;

    unsigned int hsw_sTe[16];

    unsigned int hsw_sce3;
    unsigned int hsw_sce2;
    unsigned int hsw_sEMask;
    unsigned int hsw_sce1;

} __internal_spowr_la_data_avx2_t;
static __constant __internal_spowr_la_data_avx2_t __internal_spowr_la_data_avx2 = {

    0x00800000u,
    0x7e400000u,
    0x00080000u,
    0xfff00000u,
    0x3f800000u,

    {
     0x42fe0000u, 0x42fda900u, 0x42fd5b2cu, 0x42fd14c4u, 0x42fcd480u, 0x42fc9960u, 0x42fc62a2u, 0x42fc2facu,
     0x42fe0000u, 0x42fda900u, 0x42fd5b2cu, 0x42fd14c4u, 0x42fcd480u, 0x42fc9960u, 0x42fc62a2u, 0x42fc2facu,
     },
    {
     0x00000000u, 0xb651cfdfu, 0x35f6865cu, 0x37307f34u, 0xb5d1cfdfu, 0xb48e4789u, 0x364055feu, 0x3492d9f8u,
     0x00000000u, 0xb651cfdfu, 0x35f6865cu, 0x37307f34u, 0xb5d1cfdfu, 0xb48e4789u, 0x364055feu, 0x3492d9f8u,
     },

    0x3e547edeu,
    0xbe777cc7u,
    0x3e93badcu,
    0xbeb8a9eau,
    0x3fb8aa3bu,
    0xb239c255u,
    0x32a56f15u,
    0xbf38aa3bu,
    0x3ef6384fu,

    0x7fffffffu,
    0x42fb0000u,
    0x49c003f8u,

    {
     0x3f800000u, 0x3f8b95c2u, 0x3f9837f0u, 0x3fa5fed7u, 0x3fb504f3u, 0x3fc5672au, 0x3fd744fdu, 0x3feac0c7u,
     0x3f800000u, 0x3f8b95c2u, 0x3f9837f0u, 0x3fa5fed7u, 0x3fb504f3u, 0x3fc5672au, 0x3fd744fdu, 0x3feac0c7u,
     },

    0x3d636078u,
    0x3e7607C9u,
    0x7f800000u,
    0x3f317218u
};

typedef struct
{
    unsigned int sHiMask;
    unsigned int sRSValue;
    unsigned int NMINNORM;
    unsigned int NMAXVAL;
    unsigned int INF;

    unsigned int LFR_TBL[65][3];
    unsigned int LFR_I_CHK_WORK_SUB;
    unsigned int LFR_I_CHK_WORK_CMP;
    unsigned int S_MANT_MASK;
    unsigned int S_ONE;
    unsigned int LFR_I_INDEX_MASK;
    unsigned int LFR_I_INDEX_ADD;
    unsigned int S_HI10BITS_MASK;
    unsigned int LFR_S_P4;
    unsigned int LFR_S_P3;
    unsigned int LFR_S_P2;
    unsigned int I_BIAS;
    unsigned int LFR_I_NZ_ADD;
    unsigned int LFR_I_NZ_CMP;
    unsigned int S_LOG2_HI;
    unsigned int S_LOG2_LO;

    unsigned int _sInvLn2;
    unsigned int _sShifter;
    unsigned int _sLn2hi;
    unsigned int _sLn2lo;
    unsigned int _sPC0;
    unsigned int _sPC1;
    unsigned int _sPC2;
    unsigned int _sPC3;
    unsigned int _sPC4;
    unsigned int _sPC5;
    unsigned int _iBias;
    unsigned int _iAbsMask;
    unsigned int _iDomainRange;
    unsigned int _s2N_2;
    unsigned int _sHuge;
    unsigned int _sTiny;
} __internal_spowr_la_data_t;
static __constant __internal_spowr_la_data_t __internal_spowr_la_data = {
    0xFFFFF000u,
    0x45800800u,
    0x80800000u,
    0xfeffffffu,
    0x7f800000u,

    {
     {0x00000000u, 0x00000000u, 0x3F800000u},
     {0x3C810000u, 0x35ACB127u, 0x3F7C0000u},
     {0x3D020000u, 0x372EC4F4u, 0x3F780000u},
     {0x3D33C000u, 0x38129E5Bu, 0x3F750000u},
     {0x3D774000u, 0x378C7002u, 0x3F710000u},
     {0x3D9DE000u, 0x37FAD3E9u, 0x3F6D0000u},
     {0x3DB80000u, 0x37530AEBu, 0x3F6A0000u},
     {0x3DD26000u, 0x381D902Cu, 0x3F670000u},
     {0x3DED2000u, 0x3849D8E1u, 0x3F640000u},
     {0x3E08B000u, 0x38474114u, 0x3F600000u},
     {0x3E168000u, 0x38308643u, 0x3F5D0000u},
     {0x3E248000u, 0x381EC19Au, 0x3F5A0000u},
     {0x3E2DF000u, 0x382035AAu, 0x3F580000u},
     {0x3E3C4000u, 0x3846C2A2u, 0x3F550000u},
     {0x3E4AD000u, 0x3735B9EEu, 0x3F520000u},
     {0x3E598000u, 0x386C2BAEu, 0x3F4F0000u},
     {0x3E637000u, 0x387DE378u, 0x3F4D0000u},
     {0x3E729000u, 0x38077FF4u, 0x3F4A0000u},
     {0x3E7CC000u, 0x380E365Au, 0x3F480000u},
     {0x3E862000u, 0x37359D86u, 0x3F450000u},
     {0x3E8B5800u, 0x37B9975Au, 0x3F430000u},
     {0x3E90A000u, 0x378ADA1Du, 0x3F410000u},
     {0x3E95F000u, 0x38707CDDu, 0x3F3F0000u},
     {0x3E9E1000u, 0x37A4EE66u, 0x3F3C0000u},
     {0x3EA38800u, 0x380DC272u, 0x3F3A0000u},
     {0x3EA91000u, 0x382E0739u, 0x3F380000u},
     {0x3EAEA800u, 0x383DDF59u, 0x3F360000u},
     {0x3EB45000u, 0x38483E9Cu, 0x3F340000u},
     {0x3EBA0800u, 0x385876C6u, 0x3F320000u},
     {0x3EBFD000u, 0x387A3BD8u, 0x3F300000u},
     {0x3EC5B000u, 0x3766A22Du, 0x3F2E0000u},
     {0x3ECB9800u, 0x38234313u, 0x3F2C0000u},
     {0x3ECE9800u, 0x372FA858u, 0x3F2B0000u},
     {0x3ED49800u, 0x386D3C8Bu, 0x3F290000u},
     {0x3EDAB000u, 0x387A0446u, 0x3F270000u},
     {0x3EE0E000u, 0x37C0D27Fu, 0x3F250000u},
     {0x3EE3F800u, 0x3879C745u, 0x3F240000u},
     {0x3EEA4800u, 0x3511B7BFu, 0x3F220000u},
     {0x3EED7000u, 0x37EA9099u, 0x3F210000u},
     {0x3EF3D800u, 0x378587BBu, 0x3F1F0000u},
     {0x3EF71000u, 0x382156B4u, 0x3F1E0000u},
     {0x3EFD9800u, 0x37B15EF5u, 0x3F1C0000u},
     {0x3F007000u, 0x3835CB64u, 0x3F1B0000u},
     {0x3F03C400u, 0x37FC14CFu, 0x3F190000u},
     {0x3F057000u, 0x386DC5A1u, 0x3F180000u},
     {0x3F08D400u, 0x3870478Fu, 0x3F160000u},
     {0x3F0A8C00u, 0x3807EDE5u, 0x3F150000u},
     {0x3F0C4400u, 0x385C81E4u, 0x3F140000u},
     {0x3F0FC000u, 0x3846BF8Cu, 0x3F120000u},
     {0x3F118400u, 0x37C362FBu, 0x3F110000u},
     {0x3F134800u, 0x3844226Au, 0x3F100000u},
     {0x3F16DC00u, 0x386AAF62u, 0x3F0E0000u},
     {0x3F18AC00u, 0x38348868u, 0x3F0D0000u},
     {0x3F1A8000u, 0x37A2B676u, 0x3F0C0000u},
     {0x3F1C5400u, 0x38442E33u, 0x3F0B0000u},
     {0x3F1E3000u, 0x3680C36Cu, 0x3F0A0000u},
     {0x3F200C00u, 0x3761092Eu, 0x3F090000u},
     {0x3F23D000u, 0x37293F45u, 0x3F070000u},
     {0x3F25B400u, 0x387ADF2Eu, 0x3F060000u},
     {0x3F27A000u, 0x383506B5u, 0x3F050000u},
     {0x3F299000u, 0x37B89A1Au, 0x3F040000u},
     {0x3F2B8000u, 0x38744D77u, 0x3F030000u},
     {0x3F2D7800u, 0x3800B86Du, 0x3F020000u},
     {0x3F2F7400u, 0x35AA8906u, 0x3F010000u},
     {0x3F317000u, 0x3805FDF4u, 0x3F000000u},
     },
    0x80800000u,
    0xFF000000u,
    0x007fffffu,
    0x3f800000u,
    0x007F0000u,
    0x00010000u,
    0x7fffff00u,
    0xBE800000u,
    0x3EAAAAABu,
    0xBF000000u,
    0x0000007fu,
    0x407e0000u,
    0x7ffc0000u,
    0x3F317000u,
    0x3805FDF4u,

    0x3FB8AA3Bu,
    0x4b400000u,
    0x3F317200u,
    0x35BFBE8Eu,
    0x3F800000u,
    0x3F7FFFFEu,
    0x3EFFFF34u,
    0x3E2AACACu,
    0x3D2B8392u,
    0x3C07D9FEu,
    0x0000007fu,
    0x7fffffffu,
    0x42819f00u,
    0x33800000u,
    0x7f7fffffu,
    0x00800000u
};

static __constant _iml_v2_sp_union_t __spowr_la_CoutTab[380] = {

    0x3F800000,
    0x3F640000,
    0x3F4C0000,
    0x3F3A0000,
    0x3F2A0000,
    0x3F1E0000,
    0x3F120000,
    0x3F080000,
    0x3F000000,

    0x00000000,
    0x00000000,
    0x3E2B1E00,
    0x36614FFD,
    0x3EA7B700,
    0x36DD9676,
    0x3EEBF300,
    0x3640ABC3,
    0xBED19B00,
    0xB6B053FB,
    0xBE9B8900,
    0xB599D49A,
    0xBE426000,
    0xB6AF40BC,
    0xBDB31C00,
    0xB6EDF592,
    0x00000000,
    0x00000000,

    0x3F800000,
    0x3F780000,
    0x3F700000,
    0x3F8A0000,
    0x3F880000,
    0x3F860000,
    0x3F840000,
    0x3F820000,
    0x3F800000,

    0x00000000,
    0x00000000,
    0x3D3B9800,
    0x3694C9D9,
    0x3DBEB000,
    0x3492D9F7,
    0xBDDE4000,
    0xB684815B,
    0xBDB31C00,
    0xB6EDF592,
    0xBD875800,
    0xB6627E8A,
    0xBD35D000,
    0xB6D3758F,
    0xBCB73000,
    0xB6CB42E1,
    0x00000000,
    0x00000000,

    0x3FB88000,
    0x3FB7C000,
    0x3FB70000,
    0x3FB64000,
    0x3FB5C000,
    0x3FBC8000,
    0x3FBC4000,
    0x3FBBC000,
    0x3FBB8000,
    0x3FBB0000,
    0x3FBAC000,
    0x3FBA4000,
    0x3FBA0000,
    0x3FB98000,
    0x3FB94000,
    0x3FB8C000,
    0x3FB88000,

    0x00000000,
    0x00000000,
    0x3BC08000,
    0x3601B0EA,
    0x3C40E000,
    0x36A82CE1,
    0x3C910000,
    0x35F27427,
    0x3CB17000,
    0x36BBF0CC,
    0xBCFD7000,
    0xB6DA84F4,
    0xBCEDC000,
    0xB6E53CD7,
    0xBCCE5000,
    0xB6FA51D3,
    0xBCBEA000,
    0xB4074B50,
    0xBC9F1000,
    0xB52D128E,
    0xBC8F4000,
    0xB5655E44,
    0xBC5F2000,
    0xB59903D9,
    0xBC3F6000,
    0xB5A1551A,
    0xBBFF8000,
    0xB5979427,
    0xBBBFC000,
    0xB5839E88,
    0xBB000000,
    0xB4E32477,
    0x00000000,
    0x00000000,

    0x3F800000,
    0x00000000,
    0x3F80B1EE,
    0xB3B02666,
    0x3F8164D2,
    0xB1C43FD0,
    0x3F8218B0,
    0xB3BC8C04,
    0x3F82CD86,
    0x3398AC2C,
    0x3F83835A,
    0xB3B11049,
    0x3F843A28,
    0x33C3ACDE,
    0x3F84F1F6,
    0x332C6F38,
    0x3F85AAC4,
    0xB39833B8,
    0x3F866492,
    0xB3A46DC0,
    0x3F871F62,
    0xB352C2E6,
    0x3F87DB36,
    0xB3800967,
    0x3F88980E,
    0x338092DB,
    0x3F8955EE,
    0x30D86398,
    0x3F8A14D6,
    0xB38AB691,
    0x3F8AD4C6,
    0x330A58E5,
    0x3F8B95C2,
    0xB260ABA1,
    0x3F8C57CA,
    0xB2EE6E43,
    0x3F8D1AE0,
    0xB3A481A4,
    0x3F8DDF04,
    0x32808B9A,
    0x3F8EA43A,
    0xB3697465,
    0x3F8F6A82,
    0xB3E81937,
    0x3F9031DC,
    0x330628CD,
    0x3F90FA4C,
    0x338BEEE5,
    0x3F91C3D4,
    0xB38C54EE,
    0x3F928E72,
    0x337B2A64,
    0x3F935A2C,
    0xB3D0EC19,
    0x3F942700,
    0xB3F054E4,
    0x3F94F4F0,
    0xB32E0212,
    0x3F95C3FE,
    0x3386D6CC,
    0x3F96942E,
    0xB3C8DFE8,
    0x3F97657E,
    0xB3B60E85,
    0x3F9837F0,
    0x33231B71,
    0x3F990B88,
    0xB26CC9F4,
    0x3F99E046,
    0xB359BE90,
    0x3F9AB62A,
    0x33FC9500,
    0x3F9B8D3A,
    0xB30C5563,
    0x3F9C6574,
    0xB397D13D,
    0x3F9D3EDA,
    0xB331A601,
    0x3F9E196E,
    0x3244EA39,
    0x3F9EF532,
    0x33412342,
    0x3F9FD228,
    0x32959003,
    0x3FA0B052,
    0xB3F0468F,
    0x3FA18FAE,
    0x33CA8545,
    0x3FA27044,
    0xB3FCF3B7,
    0x3FA35210,
    0xB39717FD,
    0x3FA43516,
    0xB323EC33,
    0x3FA51958,
    0xB37282C2,
    0x3FA5FED6,
    0x33A9B151,
    0x3FA6E594,
    0x33CFEEE8,
    0x3FA7CD94,
    0xB3162D36,
    0x3FA8B6D6,
    0xB3E984CE,
    0x3FA9A15A,
    0x33B4EA7C,
    0x3FAA8D26,
    0x3325D921,
    0x3FAB7A3A,
    0xB314AD82,
    0x3FAC6896,
    0x33A4BE40,
    0x3FAD583E,
    0x33EA42A1,
    0x3FAE4934,
    0x3325946B,
    0x3FAF3B78,
    0x33AD690A,
    0x3FB02F0E,
    0xB2D1247F,
    0x3FB123F6,
    0xB37C5AA8,
    0x3FB21A32,
    0xB33333CE,
    0x3FB311C4,
    0x32154889,
    0x3FB40AAE,
    0x33A2654C,
    0x3FB504F4,
    0xB3CC0622,
    0x3FB60094,
    0xB32F4254,
    0x3FB6FD92,
    0xB266B974,
    0x3FB7FBF0,
    0xB2D5CD70,
    0x3FB8FBB0,
    0xB3B89D04,
    0x3FB9FCD2,
    0x330A5817,
    0x3FBAFF5A,
    0x33B2133E,
    0x3FBC034A,
    0x337DE5D4,
    0x3FBD08A4,
    0xB3414FE8,
    0x3FBE0F68,
    0x31986099,
    0x3FBF179A,
    0xB3130B1A,
    0x3FC0213A,
    0x33A1F0D1,
    0x3FC12C4C,
    0x33CA6671,
    0x3FC238D2,
    0x32C478F6,
    0x3FC346CC,
    0x33DA2497,
    0x3FC4563E,
    0x33CC5335,
    0x3FC5672A,
    0x320AA837,
    0x3FC67990,
    0x33B5AA24,
    0x3FC78D74,
    0x33C8ABBA,
    0x3FC8A2D8,
    0x33391FFC,
    0x3FC9B9BE,
    0xB37323A2,
    0x3FCAD226,
    0x333C8521,
    0x3FCBEC14,
    0x33FEF272,
    0x3FCD078C,
    0xB3735F84,
    0x3FCE248C,
    0x3228FC24,
    0x3FCF4318,
    0x33CF1919,
    0x3FD06334,
    0xB2944353,
    0x3FD184E0,
    0xB39DAE96,
    0x3FD2A81E,
    0xB35C1DAA,
    0x3FD3CCF0,
    0x3399859B,
    0x3FD4F35A,
    0x33ABCFEE,
    0x3FD61B5E,
    0xB0303219,
    0x3FD744FC,
    0x33CAD69D,
    0x3FD8703A,
    0xB3B3924D,
    0x3FD99D16,
    0xB2F61D41,
    0x3FDACB94,
    0x335E5594,
    0x3FDBFBB8,
    0xB3504A1C,
    0x3FDD2D82,
    0xB375EF9B,
    0x3FDE60F4,
    0x33825E0F,
    0x3FDF9612,
    0x33DEB8F0,
    0x3FE0CCDE,
    0x33EC2A95,
    0x3FE2055A,
    0x33FFFE84,
    0x3FE33F8A,
    0xB38D4176,
    0x3FE47B6C,
    0x33A0373E,
    0x3FE5B906,
    0x33E77C83,
    0x3FE6F85A,
    0x33AAEE20,
    0x3FE8396A,
    0x33207898,
    0x3FE97C38,
    0x3300D89F,
    0x3FEAC0C6,
    0x33E7DD24,
    0x3FEC0718,
    0x33B64C1D,
    0x3FED4F30,
    0x3276CCA1,
    0x3FEE9910,
    0xB34FE4BA,
    0x3FEFE4BA,
    0xB348464A,
    0x3FF13230,
    0x33A7AD09,
    0x3FF28178,
    0xB3C3A600,
    0x3FF3D290,
    0xB2871670,
    0x3FF5257E,
    0xB3EADB79,
    0x3FF67A42,
    0xB3938CC0,
    0x3FF7D0E0,
    0xB38CF52F,
    0x3FF9295A,
    0xB3094457,
    0x3FFA83B2,
    0x33DB722A,
    0x3FFBDFEE,
    0xB3931A0F,
    0x3FFD3E0C,
    0x31CF486C,
    0x3FFE9E12,
    0xB3A38470,

    0x3A6A6369,
    0xBEB1C35D,
    0x3E246F69,
    0xBDAB1EA1,

    0x3F317218,
    0x3E75FDF0,
    0x3D635847,

    0x7F000000,
    0x00800000,
    0x00000000,
    0x3F800000,
    0xBF800000,

    0x47C00000,

    0x3FB88000,

    0x45800800,

    0x5F800000,
    0x1F800000,
    0x00000000,
    0x80000000,
};

static int __spowr_la_TestIntFunc (float a)
{
    int x = (*(int *) &a) & 0x7fffffff;
    int e;

    if ((x < 0x3f800000) || (x >= 0x7f800000))
    {
        return 0;
    }
    if (x >= 0x4B800000)
    {
        return 2;
    }

    e = ((x & 0x7f800000) - 0x3f800000) >> 23;
    x = x << e;
    if ((x << 9) != 0)
    {
        return 0;
    }
    if ((x << 8) == 0x80000000)
    {
        return 1;
    }

    return 2;
}

__attribute__((always_inline))
inline int __internal_spowr_la_cout (float *a, float *b, float *r)
{
    int nRet = 0;

    float flVTmp1, flVTmp2, flVPHH, flVPHL;
    float flAX, flSignRes, flX1, flRcp1, flL1Hi, flL1Lo, flX2, flRcp2, flL2Hi, flL2Lo,
        flX3, flRcp3C, flL3Hi, flL3Lo, flK, flT, flD, flR1, flCQ, flRcpC, flX1Hi, flX1Lo,
        flRcpCHi, flRcpCLo, flTmp1, flE, flT_CQHi, flCQLo, flR, flLogPart3, flLog2Poly,
        flHH, flHL, flHLL, flYHi, flYLo, flTmp2, flTmp3, flPH, flPL, flPLL, flZ,
        flExp2Poly, flExp2PolyT, flResLo, flResHi, flRes, flTwoPowN, flAY, flAi, flBi;
    float flT_lo_1, flT_lo_2, flT_lo_3;

    int i, iEXB, iEYB, iSignX, iSignY, iYHi, iYLo, iYIsFinite, iEY, iYIsInt, iXIsFinite,
        iDenoExpAdd, iXHi, k, i1, i2, i3, iELogAX, iN, j, iERes, iSign, iIsSigZeroX, iIsSigZeroY, iYMantissa, iEX;

    flAi = *a;
    flBi = *b;

    iEXB = ((((_iml_v2_sp_union_t *) & flAi)->hex[0] >> 23) & 0xFF);
    iEYB = ((((_iml_v2_sp_union_t *) & flBi)->hex[0] >> 23) & 0xFF);

    iEX = iEXB - 0x7F;
    iEY = iEYB - 0x7F;

    iSignX = (((_iml_v2_sp_union_t *) & flAi)->hex[0] >> 31);
    iSignY = (((_iml_v2_sp_union_t *) & flBi)->hex[0] >> 31);

    iIsSigZeroX = ((((_iml_v2_sp_union_t *) & flAi)->hex[0] & 0x007FFFFF) == 0);
    iIsSigZeroY = ((((_iml_v2_sp_union_t *) & flBi)->hex[0] & 0x007FFFFF) == 0);

    iYIsFinite = (((((_iml_v2_sp_union_t *) & flBi)->hex[0] >> 23) & 0xFF) != 0xFF);

    {
        int iXisZero = ((iEXB == 0) && (iIsSigZeroX));
        int iYisZero = ((iEYB == 0) && (iIsSigZeroY));
        int iXisNAN = (!((((((_iml_v2_sp_union_t *) & flAi)->hex[0] >> 23) & 0xFF) != 0xFF))) && (!(iIsSigZeroX));
        int iYisNAN = (!((((((_iml_v2_sp_union_t *) & flBi)->hex[0] >> 23) & 0xFF) != 0xFF))) && (!(iIsSigZeroY));
        int iXisINF = (!((((((_iml_v2_sp_union_t *) & flAi)->hex[0] >> 23) & 0xFF) != 0xFF))) && ((iIsSigZeroX));
        int iYisINF = (!((((((_iml_v2_sp_union_t *) & flBi)->hex[0] >> 23) & 0xFF) != 0xFF))) && ((iIsSigZeroY));

        if (iXisNAN)
        {
            flVTmp1 = ((__constant float *) __spowr_la_CoutTab)[370];
            flVTmp1 = (flVTmp1 / flVTmp1);
            *r = flVTmp1;
            return nRet;

        }

        if ((iXisINF) && (!iSignX) && (iYisZero))
        {
            flVTmp1 = ((__constant float *) __spowr_la_CoutTab)[370];
            flVTmp1 = flVTmp1 / flVTmp1;
            *r = flVTmp1;
            return nRet;

        }

        if (iXisZero)
        {

            if (iYisZero)
            {
                flVTmp1 = ((__constant float *) __spowr_la_CoutTab)[370];
                flVTmp1 = flVTmp1 / flVTmp1;
                *r = flVTmp1;
                return nRet;

            }

            if (iYisINF && (!iSignY))
            {
                flVTmp1 = ((__constant float *) __spowr_la_CoutTab)[370];
                *r = flVTmp1;
                return nRet;

            }

            if (iYisINF && iSignY)
            {
                flVTmp1 = ((__constant float *) __spowr_la_CoutTab)[370];
                flVTmp1 = 1.0f / flVTmp1;
                *r = flVTmp1;
                return nRet;

            }

            if (((((((_iml_v2_sp_union_t *) & flBi)->hex[0] >> 23) & 0xFF) != 0xFF)) && iSignY)
            {
                flVTmp1 = ((__constant float *) __spowr_la_CoutTab)[370];
                flVTmp1 = 1.0f / flVTmp1;
                *r = flVTmp1;
                return nRet;

            }

            if (((((((_iml_v2_sp_union_t *) & flBi)->hex[0] >> 23) & 0xFF) != 0xFF)) && (!iSignY))
            {
                *r = ((__constant float *) __spowr_la_CoutTab)[370];
                return nRet;

            }
        }

        if (flAi == ((__constant float *) __spowr_la_CoutTab)[371])
        {

            if (((((((_iml_v2_sp_union_t *) & flBi)->hex[0] >> 23) & 0xFF) != 0xFF)))
            {
                *r = ((__constant float *) __spowr_la_CoutTab)[371];
                return nRet;

            }

            if (iYisNAN || iYisINF)
            {
                flVTmp1 = ((__constant float *) __spowr_la_CoutTab)[370];
                flVTmp1 = flVTmp1 / flVTmp1;
                *r = flVTmp1;
                return nRet;

            }
        }

        if (iSignX)
        {
            flVTmp1 = ((__constant float *) __spowr_la_CoutTab)[370];
            flVTmp1 = flVTmp1 / flVTmp1;
            *r = flVTmp1;
            nRet = 1;
            return nRet;

        }
    }

    iYMantissa = (((_iml_v2_sp_union_t *) & flBi)->hex[0] & 0x007FFFFF);

    iYIsInt = __spowr_la_TestIntFunc (flBi);

    if (!((iSignX == 0) && (iEXB == 0x7F) && iIsSigZeroX) && !((iEYB == 0) && iIsSigZeroY))
    {
        ;

        iXIsFinite = (((((_iml_v2_sp_union_t *) & flAi)->hex[0] >> 23) & 0xFF) != 0xFF);

        if ((iXIsFinite || iIsSigZeroX) && (iYIsFinite || iIsSigZeroY))
        {
            ;

            if (flAi != ((__constant float *) __spowr_la_CoutTab)[370])
            {
                ;

                if (!((flAi == ((__constant float *) __spowr_la_CoutTab)[372]) && (iYIsInt || !iYIsFinite)))
                {
                    ;

                    if (iXIsFinite && iYIsFinite)
                    {
                        ;

                        if ((flAi > ((__constant float *) __spowr_la_CoutTab)[370]) || iYIsInt)
                        {
                            ;

                            flSignRes = ((__constant float *) __spowr_la_CoutTab)[371 + (iSignX & iYIsInt)];
                            iDenoExpAdd = 0;
                            flAX = flAi;
                            (((_iml_v2_sp_union_t *) & flAX)->hex[0] =
                             (((_iml_v2_sp_union_t *) & flAX)->hex[0] & 0x7FFFFFFF) | ((_iml_uint32_t) (0) << 31));

                            if (iEXB == 0)
                            {

                                flAX = flAX * ((__constant float *) __spowr_la_CoutTab)[376];
                                iDenoExpAdd = iDenoExpAdd - 64;
                            }

                            flX1 = flAX;
                            (((_iml_v2_sp_union_t *) & flX1)->hex[0] =
                             (((_iml_v2_sp_union_t *) & flX1)->hex[0] & 0x807FFFFF) | (((_iml_uint32_t) (0x7F) & 0xFF) << 23));

                            iXHi = ((((_iml_v2_sp_union_t *) & flAX)->hex[0] >> 23) & 0xFF);
                            iXHi = iXHi << 23;
                            iXHi = iXHi | (((_iml_v2_sp_union_t *) & flAX)->hex[0] & 0x007FFFFF);

                            k = iXHi - 0x3F380000;
                            k = k >> 23;
                            k = k + iDenoExpAdd;

                            i1 = (((_iml_v2_sp_union_t *) & flX1)->hex[0] & 0x007FFFFF);
                            i1 = i1 & 0x780000;
                            i1 = i1 + 0x80000;
                            i1 = i1 >> 20;

                            flRcp1 = ((__constant float *) __spowr_la_CoutTab)[0 + i1];

                            flL1Hi = ((__constant float *) __spowr_la_CoutTab)[9 + 2 * (i1) + 0];
                            flL1Lo = ((__constant float *) __spowr_la_CoutTab)[9 + 2 * (i1) + 1];

                            flX2 = flX1 * flRcp1;

                            i2 = (((_iml_v2_sp_union_t *) & flX2)->hex[0] & 0x007FFFFF);
                            i2 = i2 & 0x1E0000;
                            i2 = i2 + 0x20000;
                            i2 = i2 >> 18;

                            flRcp2 = ((__constant float *) __spowr_la_CoutTab)[27 + i2];

                            flL2Hi = ((__constant float *) __spowr_la_CoutTab)[36 + 2 * (i2) + 0];
                            flL2Lo = ((__constant float *) __spowr_la_CoutTab)[36 + 2 * (i2) + 1];

                            flX3 = (flX2 * flRcp2);

                            i3 = (((_iml_v2_sp_union_t *) & flX3)->hex[0] & 0x007FFFFF);
                            i3 = i3 & 0x7C000;
                            i3 = i3 + 0x4000;
                            i3 = i3 >> 15;

                            flRcp3C = ((__constant float *) __spowr_la_CoutTab)[54 + i3];

                            flL3Hi = ((__constant float *) __spowr_la_CoutTab)[71 + 2 * (i3) + 0];
                            flL3Lo = ((__constant float *) __spowr_la_CoutTab)[71 + 2 * (i3) + 1];

                            flK = (float) k;
                            flVTmp1 = ((flK) + (flL1Hi));
                            flTmp1 = ((flK) - flVTmp1);
                            flVTmp2 = (flTmp1 + (flL1Hi));
                            flT = flVTmp1;
                            flT_lo_1 = flVTmp2;

                            flVTmp1 = ((flT) + (flL2Hi));
                            flTmp1 = ((flT) - flVTmp1);
                            flVTmp2 = (flTmp1 + (flL2Hi));
                            flT = flVTmp1;
                            flT_lo_2 = flVTmp2;

                            flVTmp1 = ((flT) + (flL3Hi));
                            flTmp1 = ((flT) - flVTmp1);
                            flVTmp2 = (flTmp1 + (flL3Hi));
                            flT = flVTmp1;
                            flT_lo_3 = flVTmp2;

                            flD = (flT_lo_1 + flT_lo_2);
                            flD = (flD + flT_lo_3);
                            flD = (flD + flL1Lo);
                            flD = (flD + flL2Lo);
                            flD = (flD + flL3Lo);

                            flR1 = (flX3 * flRcp3C);
                            flCQ = (flR1 - ((__constant float *) __spowr_la_CoutTab)[374]);

                            flRcpC = (flRcp1 * flRcp2);
                            flRcpC = (flRcpC * flRcp3C);

                            flVTmp1 = ((flX1) * (((__constant float *) __spowr_la_CoutTab)[375]));
                            flVTmp2 = (flVTmp1 - (flX1));
                            flVTmp1 = (flVTmp1 - flVTmp2);
                            flVTmp2 = ((flX1) - flVTmp1);
                            flX1Hi = flVTmp1;
                            flX1Lo = flVTmp2;

                            flVTmp1 = ((flRcpC) * (((__constant float *) __spowr_la_CoutTab)[375]));
                            flVTmp2 = (flVTmp1 - (flRcpC));
                            flVTmp1 = (flVTmp1 - flVTmp2);
                            flVTmp2 = ((flRcpC) - flVTmp1);
                            flRcpCHi = flVTmp1;
                            flRcpCLo = flVTmp2;

                            flTmp1 = (flX1Hi * flRcpCHi);
                            flE = (flTmp1 - flR1);
                            flTmp1 = (flX1Lo * flRcpCHi);
                            flE = (flE + flTmp1);
                            flTmp1 = (flX1Hi * flRcpCLo);
                            flE = (flE + flTmp1);
                            flTmp1 = (flX1Lo * flRcpCLo);
                            flE = (flE + flTmp1);

                            flVTmp1 = ((flT) + (flCQ));
                            flTmp1 = ((flT) - flVTmp1);
                            flVTmp2 = (flTmp1 + (flCQ));
                            flT_CQHi = flVTmp1;
                            flCQLo = flVTmp2;

                            iELogAX = ((((_iml_v2_sp_union_t *) & flT_CQHi)->hex[0] >> 23) & 0xFF);

                            if (iELogAX + iEYB < 11 + 2 * 0x7F)
                            {
                                ;

                                if (iELogAX + iEYB > -62 + 2 * 0x7F)
                                {
                                    ;

                                    flR = (flCQ + flE);

                                    flLog2Poly =
                                        ((((((__constant float *) __spowr_la_CoutTab)[364]) * flR +
                                           ((__constant float *) __spowr_la_CoutTab)[363]) * flR +
                                          ((__constant float *) __spowr_la_CoutTab)[362]) * flR +
                                         ((__constant float *) __spowr_la_CoutTab)[361]) * flR;

                                    flLogPart3 = (flCQLo + flE);
                                    flLogPart3 = (flD + flLogPart3);

                                    flVTmp1 = ((flT_CQHi) + (flLog2Poly));
                                    flTmp1 = ((flT_CQHi) - flVTmp1);
                                    flVTmp2 = (flTmp1 + (flLog2Poly));
                                    flHH = flVTmp1;
                                    flHL = flVTmp2;

                                    flVTmp1 = ((flHH) + (flLogPart3));
                                    flTmp1 = ((flHH) - flVTmp1);
                                    flVTmp2 = (flTmp1 + (flLogPart3));
                                    flHH = flVTmp1;
                                    flHLL = flVTmp2;

                                    flHLL = (flHLL + flHL);

                                    flVTmp1 = ((flHH) * (((__constant float *) __spowr_la_CoutTab)[375]));
                                    flVTmp2 = (flVTmp1 - (flHH));
                                    flVTmp1 = (flVTmp1 - flVTmp2);
                                    flVTmp2 = ((flHH) - flVTmp1);
                                    flHH = flVTmp1;
                                    flHL = flVTmp2;

                                    flVTmp1 = ((flBi) * (((__constant float *) __spowr_la_CoutTab)[375]));
                                    flVTmp2 = (flVTmp1 - (flBi));
                                    flVTmp1 = (flVTmp1 - flVTmp2);
                                    flVTmp2 = ((flBi) - flVTmp1);
                                    flYHi = flVTmp1;
                                    flYLo = flVTmp2;

                                    flTmp1 = ((flYHi) * (flHH));
                                    flTmp2 = ((flYLo) * (flHL));
                                    flTmp2 = (flTmp2 + (flYHi) * (flHL));
                                    flTmp3 = (flTmp2 + (flYLo) * (flHH));
                                    flPH = flTmp1;
                                    flPL = flTmp3;

                                    flPLL = (flBi * flHLL);

                                    flVTmp1 = (flPH + ((__constant float *) __spowr_la_CoutTab)[373]);
                                    flVPHH = (flVTmp1 - ((__constant float *) __spowr_la_CoutTab)[373]);
                                    iN = (((_iml_v2_sp_union_t *) & flVTmp1)->hex[0] & 0x007FFFFF);
                                    j = iN & 0x7F;

                                    iN = iN << 10;
                                    iN = iN >> (7 + 10);
                                    flVPHL = (flPH - flVPHH);

                                    flZ = (flPLL + flPL);
                                    flZ = (flZ + flVPHL);

                                    flExp2Poly =
                                        (((((__constant float *) __spowr_la_CoutTab)[367]) * flZ +
                                          ((__constant float *) __spowr_la_CoutTab)[366]) * flZ +
                                         ((__constant float *) __spowr_la_CoutTab)[365]) * flZ;

                                    flExp2PolyT = (flExp2Poly * ((__constant float *) __spowr_la_CoutTab)[105 + 2 * (j) + 0]);
                                    flResLo = (flExp2PolyT + ((__constant float *) __spowr_la_CoutTab)[105 + 2 * (j) + 1]);
                                    flResHi = ((__constant float *) __spowr_la_CoutTab)[105 + 2 * (j) + 0];

                                    flRes = (flResHi + flResLo);
                                    iERes = ((((_iml_v2_sp_union_t *) & flRes)->hex[0] >> 23) & 0xFF);
                                    iERes = (iERes - 0x7F);
                                    iERes = (iERes + iN);

                                    if (iERes < 128)
                                    {
                                        ;
                                        if (iERes >= -126)
                                        {
                                            ;

                                            (((_iml_v2_sp_union_t *) & flRes)->hex[0] =
                                             (((_iml_v2_sp_union_t *) & flRes)->hex[0] & 0x807FFFFF) | (((_iml_uint32_t) (iERes + 0x7F) & 0xFF) << 23));

                                            flRes = (flRes * flSignRes);
                                            *r = flRes;
                                        }
                                        else
                                        {

                                            if (iERes >= -126 - 10)
                                            {
                                                ;

                                                flVTmp1 = ((flResHi) + (flResLo));
                                                flTmp1 = ((flResHi) - flVTmp1);
                                                flVTmp2 = (flTmp1 + (flResLo));
                                                flResHi = flVTmp1;
                                                flResLo = flVTmp2;

                                                flVTmp1 = ((flResHi) * (((__constant float *) __spowr_la_CoutTab)[375]));
                                                flVTmp2 = (flVTmp1 - (flResHi));
                                                flVTmp1 = (flVTmp1 - flVTmp2);
                                                flVTmp2 = ((flResHi) - flVTmp1);
                                                flResHi = flVTmp1;
                                                flTmp2 = flVTmp2;

                                                flResLo = (flResLo + flTmp2);

                                                flSignRes *= ((__constant float *) __spowr_la_CoutTab)[377];
                                                iN = (iN + 64);

                                                flTwoPowN = ((__constant float *) __spowr_la_CoutTab)[371];
                                                (((_iml_v2_sp_union_t *) & flTwoPowN)->hex[0] =
                                                 (((_iml_v2_sp_union_t *) & flTwoPowN)->
                                                  hex[0] & 0x807FFFFF) | (((_iml_uint32_t) (iN + 0x7F) & 0xFF) << 23));

                                                flResHi = (flResHi * flTwoPowN);
                                                flResHi = (flResHi * flSignRes);

                                                flResLo = (flResLo * flTwoPowN);
                                                flVTmp1 = (flResLo * flSignRes);

                                                flRes = (flResHi + flVTmp1);

                                                flVTmp1 = ((__constant float *) __spowr_la_CoutTab)[369];
                                                flVTmp1 = (flVTmp1 * flVTmp1);
                                                flRes = (flRes + flVTmp1);

                                                *r = flRes;
                                            }
                                            else
                                            {
                                                ;

                                                if (iERes >= -149 - 10)
                                                {

                                                    flSignRes *= ((__constant float *) __spowr_la_CoutTab)[377];
                                                    iN = iN + 64;

                                                    flTwoPowN = ((__constant float *) __spowr_la_CoutTab)[371];
                                                    (((_iml_v2_sp_union_t *) & flTwoPowN)->hex[0] =
                                                     (((_iml_v2_sp_union_t *) & flTwoPowN)->
                                                      hex[0] & 0x807FFFFF) | (((_iml_uint32_t) (iN + 0x7F) & 0xFF) << 23));

                                                    flRes = (flRes * flTwoPowN);
                                                    flRes = (flRes * flSignRes);

                                                    flVTmp1 = ((__constant float *) __spowr_la_CoutTab)[369];
                                                    flVTmp1 *= flVTmp1;
                                                    flRes = (flRes + flVTmp1);

                                                    *r = flRes;
                                                    nRet = 4;
                                                }

                                                else
                                                {
                                                    ;

                                                    flVTmp1 = ((__constant float *) __spowr_la_CoutTab)[369];
                                                    flVTmp1 *= flVTmp1;
                                                    flRes = (flVTmp1 * flSignRes);
                                                    *r = flRes;
                                                    nRet = 4;
                                                }
                                            }
                                        }
                                    }

                                    else
                                    {
                                        ;

                                        flVTmp1 = ((__constant float *) __spowr_la_CoutTab)[368];
                                        flVTmp1 = (flVTmp1 * flVTmp1);
                                        flRes = (flVTmp1 * flSignRes);
                                        *r = flRes;
                                        nRet = 3;
                                    }
                                }
                                else
                                {
                                    ;

                                    flVTmp1 = ((__constant float *) __spowr_la_CoutTab)[371];
                                    flVTmp1 = (flVTmp1 + ((__constant float *) __spowr_la_CoutTab)[369]);
                                    *r = (flVTmp1 * flSignRes);
                                }
                            }
                            else
                            {
                                ;

                                iSign = iSignY ^ (((_iml_v2_sp_union_t *) & flT_CQHi)->hex[0] >> 31);

                                flTmp1 = ((__constant float *) __spowr_la_CoutTab)[368 + (iSign)];

                                flTmp1 = (flTmp1 * flTmp1);

                                flTmp1 = (flTmp1 * flSignRes);
                                *r = flTmp1;
                                nRet = (iSign ? 4 : 3);
                            }
                        }
                        else
                        {
                            ;

                            flVTmp1 = ((__constant float *) __spowr_la_CoutTab)[370];
                            flVTmp1 = (flVTmp1 / flVTmp1);
                            *r = flVTmp1;
                            nRet = 1;
                        }
                    }
                    else
                    {
                        ;

                        if (iEXB < 0x7F)
                        {
                            ;

                            if (iSignY)
                            {
                                ;

                                *r = (flBi * flBi);
                            }
                            else
                            {
                                ;

                                *r = ((__constant float *) __spowr_la_CoutTab)[370];
                            }
                        }
                        else
                        {
                            ;

                            if (iSignY)
                            {
                                ;

                                flRes = ((__constant float *) __spowr_la_CoutTab)[378 + (iYIsInt & iSignX)];
                                *r = flRes;
                            }
                            else
                            {
                                ;

                                flTmp1 = (flAi * flAi);
                                flTmp1 = (flTmp1 * flBi);
                                *r = flTmp1 * ((__constant float *) __spowr_la_CoutTab)[371 + (iYIsInt & iSignX)];
                            }
                        }
                    }
                }
                else
                {
                    ;

                    *r = ((__constant float *) __spowr_la_CoutTab)[371 + (iYIsInt & 1)];
                }
            }
            else
            {
                ;

                flTmp1 = flAi * flAi;

                if (iSignY)
                {
                    ;

                    *r = ((__constant float *) __spowr_la_CoutTab)[371 + (iYIsInt & iSignX)] / flTmp1;
                    nRet = 1;

                }
                else
                {
                    ;

                    *r = ((__constant float *) __spowr_la_CoutTab)[371 + (iYIsInt & iSignX)] * flTmp1;
                }
            }
        }
        else
        {
            ;

            *r = *a + *b;
        }
    }

    else
    {
        ;

        flVTmp1 = flAi + flBi;
        iSign = (((_iml_v2_sp_union_t *) & flVTmp1)->hex[0] >> 31);
        flVTmp2 = ((__constant float *) __spowr_la_CoutTab)[371];
        (((_iml_v2_sp_union_t *) & flVTmp2)->hex[0] = (((_iml_v2_sp_union_t *) & flVTmp2)->hex[0] & 0x7FFFFFFF) | ((_iml_uint32_t) (iSign) << 31));

        *r = flVTmp2 * flVTmp2;
    }

    return nRet;
}

float __ocl_svml_powrf (float a, float b)
{

    float va1;
    float va2;
    float vr1;
    unsigned int vm;

    float r;

    va1 = a;
    va2 = b;

    {

        float sHiMask;
        float sRSValue;
        float sZ[2];
        float sZ0[2];
        float sL[2];
        float sY[2];
        unsigned int _NMINNORM;
        unsigned int _NMAXVAL;
        unsigned int _INF;
        unsigned int iSpecX;
        unsigned int iSpecY;
        unsigned int LFR_iY;
        unsigned int iRangeMask;

        unsigned int LFR_iX;
        unsigned int LFR_iXBadSub;
        unsigned int LFR_iXBad;
        float LFR_sXMant;
        float LFR_sM;
        unsigned int LFR_iInd;
        float LFR_sLnRcprYHi;
        float LFR_sLnRcprYLo;
        float LFR_sRcprY;
        float LFR_sYHi;
        float LFR_sYLo;
        float LFR_sRHi;
        float LFR_sTRHi;
        float LFR_sRLo;
        float LFR_sR;
        float LFR_sP;
        float LFR_sR2;
        unsigned int LFR_iN;
        float LFR_sN;
        float LFR_sNLog2Hi;
        float LFR_sNLog2Lo;
        float LFR_sWLo;
        float LFR_sResHi;
        float LFR_sResLo;
        unsigned int LFR_I_CHK_WORK_SUB;
        unsigned int LFR_I_CHK_WORK_CMP;
        float S_MANT_MASK;
        float S_ONE;
        unsigned int LFR_I_INDEX_MASK;
        unsigned int LFR_I_INDEX_ADD;
        float S_HI10BITS_MASK;
        float LFR_S_P4;
        float LFR_S_P3;
        float LFR_S_P2;
        unsigned int I_BIAS;
        float S_LOG2_HI;
        float S_LOG2_LO;
        float sR2;
        float sRHL[2];

        unsigned int iHiDelta;
        unsigned int iLoRange;
        unsigned int iBrkValue;
        unsigned int iOffExpoMask;
        float sOne;
        float sLn2Hi;
        float sLn2Lo;
        float sPoly[7];
        unsigned int iX;
        unsigned int iXTest;
        float sN;
        unsigned int iN;
        float sR;
        unsigned int iR;
        float sP;
        float sM;
        float s2N;
        unsigned int iAbsZ;
        unsigned int iRes;
        unsigned int iP;
        unsigned int iM;
        float sInvLn2;
        float sShifter;
        float sLn2hi;
        float sLn2lo;
        unsigned int iBias;
        unsigned int iAbsMask;
        unsigned int iDomainRange;
        float sPC[6];
        float stmp;

        LFR_iX = as_uint (va1);
        LFR_iY = as_uint (va2);

        _NMINNORM = (__internal_spowr_la_data.NMINNORM);
        _NMAXVAL = (__internal_spowr_la_data.NMAXVAL);
        _INF = (__internal_spowr_la_data.INF);
        iAbsMask = (__internal_spowr_la_data._iAbsMask);

        iSpecX = (LFR_iX - _NMINNORM);
        iSpecX = ((unsigned int) (-(signed int) ((signed int) iSpecX >= (signed int) _NMAXVAL)));
        iSpecY = (LFR_iY & iAbsMask);
        iSpecY = ((unsigned int) (-(signed int) ((signed int) iSpecY >= (signed int) _INF)));
        iRangeMask = (iSpecX | iSpecY);

        S_MANT_MASK = as_float (__internal_spowr_la_data.S_MANT_MASK);

        LFR_sXMant = as_float ((as_uint (va1) & as_uint (S_MANT_MASK)));
        S_ONE = as_float (__internal_spowr_la_data.S_ONE);
        LFR_sM = as_float ((as_uint (LFR_sXMant) | as_uint (S_ONE)));
        LFR_iN = ((unsigned int) (LFR_iX) >> (23));
        I_BIAS = (__internal_spowr_la_data.I_BIAS);
        LFR_iN = (LFR_iN - I_BIAS);
        LFR_sN = ((float) ((int) (LFR_iN)));

        LFR_I_INDEX_MASK = (__internal_spowr_la_data.LFR_I_INDEX_MASK);
        LFR_iInd = (LFR_iX & LFR_I_INDEX_MASK);

        LFR_I_INDEX_ADD = (__internal_spowr_la_data.LFR_I_INDEX_ADD);

        LFR_iInd = (LFR_iInd + LFR_I_INDEX_ADD);

        LFR_iInd = ((unsigned int) (LFR_iInd) >> (17));
        LFR_sLnRcprYHi = as_float (((__constant unsigned int *) (__internal_spowr_la_data.LFR_TBL))[(((0 + LFR_iInd) * (3 * 4)) >> (2)) + 0]);
        LFR_sLnRcprYLo = as_float (((__constant unsigned int *) (__internal_spowr_la_data.LFR_TBL))[(((0 + LFR_iInd) * (3 * 4)) >> (2)) + 1]);
        LFR_sRcprY = as_float (((__constant unsigned int *) (__internal_spowr_la_data.LFR_TBL))[(((0 + LFR_iInd) * (3 * 4)) >> (2)) + 2]);

        S_HI10BITS_MASK = as_float (__internal_spowr_la_data.S_HI10BITS_MASK);

        LFR_sYHi = as_float ((as_uint (LFR_sM) & as_uint (S_HI10BITS_MASK)));
        LFR_sYLo = (LFR_sM - LFR_sYHi);

        LFR_sRHi = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (LFR_sYHi, LFR_sRcprY, -(S_ONE));
        LFR_sRLo = (LFR_sYLo * LFR_sRcprY);
        LFR_sR = (LFR_sRHi + LFR_sRLo);

        LFR_S_P4 = as_float (__internal_spowr_la_data.LFR_S_P4);
        LFR_S_P3 = as_float (__internal_spowr_la_data.LFR_S_P3);
        LFR_sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (LFR_S_P4, LFR_sR, LFR_S_P3);

        LFR_S_P2 = as_float (__internal_spowr_la_data.LFR_S_P2);
        LFR_sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (LFR_sP, LFR_sR, LFR_S_P2);

        LFR_sR2 = (LFR_sR * LFR_sR);
        LFR_sP = (LFR_sP * LFR_sR2);

        S_LOG2_HI = as_float (__internal_spowr_la_data.S_LOG2_HI);

        LFR_sNLog2Hi = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (LFR_sN, S_LOG2_HI, LFR_sLnRcprYHi);

        S_LOG2_LO = as_float (__internal_spowr_la_data.S_LOG2_LO);
        LFR_sNLog2Lo = (LFR_sN * S_LOG2_LO);

        LFR_sResHi = (LFR_sNLog2Hi + LFR_sRHi);

        stmp = (LFR_sResHi - LFR_sNLog2Hi);
        LFR_sRHi = (LFR_sRHi - stmp);

        LFR_sWLo = (LFR_sNLog2Lo + LFR_sLnRcprYLo);

        LFR_sResLo = (LFR_sP + LFR_sWLo);

        LFR_sResLo = (LFR_sResLo + LFR_sRHi);

        sL[0] = (LFR_sResHi + LFR_sRLo);
        stmp = (sL[0] - LFR_sResHi);
        LFR_sRLo = (LFR_sRLo - stmp);

        sL[1] = (LFR_sResLo + LFR_sRLo);

        sRSValue = as_float (__internal_spowr_la_data.sRSValue);
        sHiMask = as_float (__internal_spowr_la_data.sHiMask);

        {
            float V1;
            float V2;;
            V1 = (sL[0] + sL[1]);
            V2 = (V1 * sRSValue);
            V1 = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (V1, 1.0f, V2);
            V2 = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (V1, 1.0f, -(V2));
            V1 = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sL[0], 1.0f, -(V2));
            V1 = (sL[1] + V1);;
            sL[0] = V2;
            sL[1] = V1;
        };

        {
            float V1;
            float V2;;
            V1 = (va2 * sRSValue);
            V2 = (V1 - va2);
            V1 = (V1 - V2);
            V2 = (va2 - V1);;
            sY[0] = V1;
            sY[1] = V2;
        };
        {
            float V1;
            float V2;;
            V1 = (sL[0] * sY[0]);
            V2 = (sL[1] * sY[1]);
            V2 = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sL[0], sY[1], V2);
            V2 = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sL[1], sY[0], V2);;
            sZ0[0] = V1;
            sZ0[1] = V2;
        };

        sZ[0] = (sZ0[0] + sZ0[1]);
        sZ0[0] = (sZ0[0] - sZ[0]);
        sZ[1] = (sZ0[1] + sZ0[0]);

        sInvLn2 = as_float (__internal_spowr_la_data._sInvLn2);
        sShifter = as_float (__internal_spowr_la_data._sShifter);

        sM = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sZ[0], sInvLn2, sShifter);
        sN = (sM - sShifter);

        iAbsZ = as_uint (sZ[0]);

        iAbsZ = (iAbsZ & iAbsMask);
        iDomainRange = (__internal_spowr_la_data._iDomainRange);
        iAbsZ = ((unsigned int) (-(signed int) ((signed int) iAbsZ > (signed int) iDomainRange)));
        iRangeMask = (iRangeMask | iAbsZ);
        vm = 0;
        vm = iRangeMask;

        iM = as_uint (sM);

        iM = ((unsigned int) (iM) << (23));

        sLn2hi = as_float (__internal_spowr_la_data._sLn2hi);
        sR = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (-(sN), sLn2hi, sZ[0]);
        sLn2lo = as_float (__internal_spowr_la_data._sLn2lo);
        sR = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (-(sN), sLn2lo, sR);
        sR = (sR + sZ[1]);
        sPC[4] = as_float (__internal_spowr_la_data._sPC4);
        sPC[5] = as_float (__internal_spowr_la_data._sPC5);

        sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sPC[5], sR, sPC[4]);
        sPC[3] = as_float (__internal_spowr_la_data._sPC3);
        sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sP, sR, sPC[3]);
        sPC[2] = as_float (__internal_spowr_la_data._sPC2);
        sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sP, sR, sPC[2]);
        sPC[1] = as_float (__internal_spowr_la_data._sPC1);
        sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sP, sR, sPC[1]);
        sPC[0] = as_float (__internal_spowr_la_data._sPC0);
        sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sP, sR, sPC[0]);
        iP = as_uint (sP);

        iRes = (iM + iP);
        vr1 = as_float (iRes);
    }

    if ((vm) != 0)
    {
        float _vapi_arg1[1];
        float _vapi_arg2[1];
        float _vapi_res1[1];
        ((float *) _vapi_arg1)[0] = va1;
        ((float *) _vapi_arg2)[0] = va2;
        ((float *) _vapi_res1)[0] = vr1;
        __internal_spowr_la_cout (_vapi_arg1, _vapi_arg2, _vapi_res1);
        vr1 = ((float *) _vapi_res1)[0];
    };
    r = vr1;;

    return r;

}

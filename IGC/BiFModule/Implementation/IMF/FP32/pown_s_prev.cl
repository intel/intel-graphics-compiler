/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// SVML code

#ifndef __SVML_DATA_STRUCTURES_CL__
#define __SVML_DATA_STRUCTURES_CL__

typedef int VINT32;
typedef signed int VSINT32;
typedef unsigned int VUINT32;

typedef long VINT64;
typedef signed long VSINT64;
typedef unsigned long VUINT64;

typedef int int1_gen;
typedef long long1_gen;
typedef float float1_gen;
typedef float float1_ref;
typedef int int1_ref;
typedef int __int32;

typedef unsigned int  _iml_uint32_t;
typedef int           _iml_int32_t;
typedef unsigned long _iml_uint64_t;

typedef union
{
    uint hex;
    float fp;
} _iml_sp_union_t;

__attribute__((always_inline))
uint _castf32_u32(float a)
{
  return as_uint(a);
}

__attribute__((always_inline))
float _castu32_f32(uint a)
{
  return as_float(a);
}

#if defined(cl_khr_fp64)

typedef double double1_ref;

typedef struct tag_iml_dpdwords_t
{
  _iml_uint32_t lo_dword;
  _iml_uint32_t hi_dword;
} _iml_dpdwords_t;

typedef union
{
  _iml_uint32_t hex[2];
  _iml_dpdwords_t dwords;
  double fp;
} _iml_dp_union_t;

typedef union
{
  ulong hex;
  double fp;
} _lng_dbl_union_t;

__attribute__((always_inline))
double _castu64_f64(ulong a)
{
  return as_double(a);
}

__attribute__((always_inline))
ulong _castf64_u64(double a)
{
  return as_ulong(a);
}

__attribute__((always_inline))
double _nearbyint(double a)
{
  return SPIRV_OCL_BUILTIN(round, _f64, )(a);
}
#endif

typedef struct
{

    VUINT32 sHiMask;
    VUINT32 sRSValue;
    VUINT32 NMINNORM;
    VUINT32 NMAXVAL;
    VUINT32 INF;

    VUINT32 LFR_TBL[65][3];
    VUINT32 LFR_I_CHK_WORK_SUB;
    VUINT32 LFR_I_CHK_WORK_CMP;
    VUINT32 S_MANT_MASK;
    VUINT32 S_ONE;
    VUINT32 LFR_I_INDEX_MASK;
    VUINT32 LFR_I_INDEX_ADD;
    VUINT32 S_HI10BITS_MASK;
    VUINT32 LFR_S_P4;
    VUINT32 LFR_S_P3;
    VUINT32 LFR_S_P2;
    VUINT32 I_BIAS;
    VUINT32 LFR_I_NZ_ADD;
    VUINT32 LFR_I_NZ_CMP;
    VUINT32 S_LOG2_HI;
    VUINT32 S_LOG2_LO;

    VUINT32 _sInvLn2;
    VUINT32 _sShifter;
    VUINT32 _sLn2hi;
    VUINT32 _sLn2lo;
    VUINT32 _sPC0;
    VUINT32 _sPC1;
    VUINT32 _sPC2;
    VUINT32 _sPC3;
    VUINT32 _sPC4;
    VUINT32 _sPC5;
    VUINT32 _iBias;
    VUINT32 _iAbsMask;
    VUINT32 _iDomainRange;
    VUINT32 _s2N_2;
    VUINT32 _sHuge;
    VUINT32 _sTiny;

}
sPow_Table_Type;

__constant sPow_Table_Type __ocl_svml_spow_data =
{

    (VUINT32) (0xFFFFF000u),
    (VUINT32) (0x45800800u),
    (VUINT32) (0x80800000u),
    (VUINT32) (0xfeffffffu),
    (VUINT32) (0x7f800000u),

    {
        {(VUINT32) (0x00000000u),
         (VUINT32) (0x00000000u),
         (VUINT32) (0x3F800000u)},
        {(VUINT32) (0x3C810000u),
         (VUINT32) (0x35ACB127u),
         (VUINT32) (0x3F7C0000u)},
        {(VUINT32) (0x3D020000u),
         (VUINT32) (0x372EC4F4u),
         (VUINT32) (0x3F780000u)},
        {(VUINT32) (0x3D33C000u),
         (VUINT32) (0x38129E5Bu),
         (VUINT32) (0x3F750000u)},
        {(VUINT32) (0x3D774000u),
         (VUINT32) (0x378C7002u),
         (VUINT32) (0x3F710000u)},
        {(VUINT32) (0x3D9DE000u),
         (VUINT32) (0x37FAD3E9u),
         (VUINT32) (0x3F6D0000u)},
        {(VUINT32) (0x3DB80000u),
         (VUINT32) (0x37530AEBu),
         (VUINT32) (0x3F6A0000u)},
        {(VUINT32) (0x3DD26000u),
         (VUINT32) (0x381D902Cu),
         (VUINT32) (0x3F670000u)},
        {(VUINT32) (0x3DED2000u),
         (VUINT32) (0x3849D8E1u),
         (VUINT32) (0x3F640000u)},
        {(VUINT32) (0x3E08B000u),
         (VUINT32) (0x38474114u),
         (VUINT32) (0x3F600000u)},
        {(VUINT32) (0x3E168000u),
         (VUINT32) (0x38308643u),
         (VUINT32) (0x3F5D0000u)},
        {(VUINT32) (0x3E248000u),
         (VUINT32) (0x381EC19Au),
         (VUINT32) (0x3F5A0000u)},
        {(VUINT32) (0x3E2DF000u),
         (VUINT32) (0x382035AAu),
         (VUINT32) (0x3F580000u)},
        {(VUINT32) (0x3E3C4000u),
         (VUINT32) (0x3846C2A2u),
         (VUINT32) (0x3F550000u)},
        {(VUINT32) (0x3E4AD000u),
         (VUINT32) (0x3735B9EEu),
         (VUINT32) (0x3F520000u)},
        {(VUINT32) (0x3E598000u),
         (VUINT32) (0x386C2BAEu),
         (VUINT32) (0x3F4F0000u)},
        {(VUINT32) (0x3E637000u),
         (VUINT32) (0x387DE378u),
         (VUINT32) (0x3F4D0000u)},
        {(VUINT32) (0x3E729000u),
         (VUINT32) (0x38077FF4u),
         (VUINT32) (0x3F4A0000u)},
        {(VUINT32) (0x3E7CC000u),
         (VUINT32) (0x380E365Au),
         (VUINT32) (0x3F480000u)},
        {(VUINT32) (0x3E862000u),
         (VUINT32) (0x37359D86u),
         (VUINT32) (0x3F450000u)},
        {(VUINT32) (0x3E8B5800u),
         (VUINT32) (0x37B9975Au),
         (VUINT32) (0x3F430000u)},
        {(VUINT32) (0x3E90A000u),
         (VUINT32) (0x378ADA1Du),
         (VUINT32) (0x3F410000u)},
        {(VUINT32) (0x3E95F000u),
         (VUINT32) (0x38707CDDu),
         (VUINT32) (0x3F3F0000u)},
        {(VUINT32) (0x3E9E1000u),
         (VUINT32) (0x37A4EE66u),
         (VUINT32) (0x3F3C0000u)},
        {(VUINT32) (0x3EA38800u),
         (VUINT32) (0x380DC272u),
         (VUINT32) (0x3F3A0000u)},
        {(VUINT32) (0x3EA91000u),
         (VUINT32) (0x382E0739u),
         (VUINT32) (0x3F380000u)},
        {(VUINT32) (0x3EAEA800u),
         (VUINT32) (0x383DDF59u),
         (VUINT32) (0x3F360000u)},
        {(VUINT32) (0x3EB45000u),
         (VUINT32) (0x38483E9Cu),
         (VUINT32) (0x3F340000u)},
        {(VUINT32) (0x3EBA0800u),
         (VUINT32) (0x385876C6u),
         (VUINT32) (0x3F320000u)},
        {(VUINT32) (0x3EBFD000u),
         (VUINT32) (0x387A3BD8u),
         (VUINT32) (0x3F300000u)},
        {(VUINT32) (0x3EC5B000u),
         (VUINT32) (0x3766A22Du),
         (VUINT32) (0x3F2E0000u)},
        {(VUINT32) (0x3ECB9800u),
         (VUINT32) (0x38234313u),
         (VUINT32) (0x3F2C0000u)},
        {(VUINT32) (0x3ECE9800u),
         (VUINT32) (0x372FA858u),
         (VUINT32) (0x3F2B0000u)},
        {(VUINT32) (0x3ED49800u),
         (VUINT32) (0x386D3C8Bu),
         (VUINT32) (0x3F290000u)},
        {(VUINT32) (0x3EDAB000u),
         (VUINT32) (0x387A0446u),
         (VUINT32) (0x3F270000u)},
        {(VUINT32) (0x3EE0E000u),
         (VUINT32) (0x37C0D27Fu),
         (VUINT32) (0x3F250000u)},
        {(VUINT32) (0x3EE3F800u),
         (VUINT32) (0x3879C745u),
         (VUINT32) (0x3F240000u)},
        {(VUINT32) (0x3EEA4800u),
         (VUINT32) (0x3511B7BFu),
         (VUINT32) (0x3F220000u)},
        {(VUINT32) (0x3EED7000u),
         (VUINT32) (0x37EA9099u),
         (VUINT32) (0x3F210000u)},
        {(VUINT32) (0x3EF3D800u),
         (VUINT32) (0x378587BBu),
         (VUINT32) (0x3F1F0000u)},
        {(VUINT32) (0x3EF71000u),
         (VUINT32) (0x382156B4u),
         (VUINT32) (0x3F1E0000u)},
        {(VUINT32) (0x3EFD9800u),
         (VUINT32) (0x37B15EF5u),
         (VUINT32) (0x3F1C0000u)},
        {(VUINT32) (0x3F007000u),
         (VUINT32) (0x3835CB64u),
         (VUINT32) (0x3F1B0000u)},
        {(VUINT32) (0x3F03C400u),
         (VUINT32) (0x37FC14CFu),
         (VUINT32) (0x3F190000u)},
        {(VUINT32) (0x3F057000u),
         (VUINT32) (0x386DC5A1u),
         (VUINT32) (0x3F180000u)},
        {(VUINT32) (0x3F08D400u),
         (VUINT32) (0x3870478Fu),
         (VUINT32) (0x3F160000u)},
        {(VUINT32) (0x3F0A8C00u),
         (VUINT32) (0x3807EDE5u),
         (VUINT32) (0x3F150000u)},
        {(VUINT32) (0x3F0C4400u),
         (VUINT32) (0x385C81E4u),
         (VUINT32) (0x3F140000u)},
        {(VUINT32) (0x3F0FC000u),
         (VUINT32) (0x3846BF8Cu),
         (VUINT32) (0x3F120000u)},
        {(VUINT32) (0x3F118400u),
         (VUINT32) (0x37C362FBu),
         (VUINT32) (0x3F110000u)},
        {(VUINT32) (0x3F134800u),
         (VUINT32) (0x3844226Au),
         (VUINT32) (0x3F100000u)},
        {(VUINT32) (0x3F16DC00u),
         (VUINT32) (0x386AAF62u),
         (VUINT32) (0x3F0E0000u)},
        {(VUINT32) (0x3F18AC00u),
         (VUINT32) (0x38348868u),
         (VUINT32) (0x3F0D0000u)},
        {(VUINT32) (0x3F1A8000u),
         (VUINT32) (0x37A2B676u),
         (VUINT32) (0x3F0C0000u)},
        {(VUINT32) (0x3F1C5400u),
         (VUINT32) (0x38442E33u),
         (VUINT32) (0x3F0B0000u)},
        {(VUINT32) (0x3F1E3000u),
         (VUINT32) (0x3680C36Cu),
         (VUINT32) (0x3F0A0000u)},
        {(VUINT32) (0x3F200C00u),
         (VUINT32) (0x3761092Eu),
         (VUINT32) (0x3F090000u)},
        {(VUINT32) (0x3F23D000u),
         (VUINT32) (0x37293F45u),
         (VUINT32) (0x3F070000u)},
        {(VUINT32) (0x3F25B400u),
         (VUINT32) (0x387ADF2Eu),
         (VUINT32) (0x3F060000u)},
        {(VUINT32) (0x3F27A000u),
         (VUINT32) (0x383506B5u),
         (VUINT32) (0x3F050000u)},
        {(VUINT32) (0x3F299000u),
         (VUINT32) (0x37B89A1Au),
         (VUINT32) (0x3F040000u)},
        {(VUINT32) (0x3F2B8000u),
         (VUINT32) (0x38744D77u),
         (VUINT32) (0x3F030000u)},
        {(VUINT32) (0x3F2D7800u),
         (VUINT32) (0x3800B86Du),
         (VUINT32) (0x3F020000u)},
        {(VUINT32) (0x3F2F7400u),
         (VUINT32) (0x35AA8906u),
         (VUINT32) (0x3F010000u)},
        {(VUINT32) (0x3F317000u),
         (VUINT32) (0x3805FDF4u),
         (VUINT32) (0x3F000000u)},
    },
    (VUINT32) (0x80800000u),
    (VUINT32) (0xFF000000u),
    (VUINT32) (0x007fffffu),
    (VUINT32) (0x3f800000u),
    (VUINT32) (0x007F0000u),
    (VUINT32) (0x00010000u),
    (VUINT32) (0x7fffc000u),
    (VUINT32) (0xBE800000u),
    (VUINT32) (0x3EAAAAABu),
    (VUINT32) (0xBF000000u),
    (VUINT32) (0x0000007fu),
    (VUINT32) (0x407e0000u),
    (VUINT32) (0x7ffc0000u),
    (VUINT32) (0x3F317000u),
    (VUINT32) (0x3805FDF4u),

    (VUINT32) (0x3FB8AA3Bu),
    (VUINT32) (0x4b400000u),
    (VUINT32) (0x3F317200u),
    (VUINT32) (0x35BFBE8Eu),
    (VUINT32) (0x3F800000u),
    (VUINT32) (0x3F7FFFFEu),
    (VUINT32) (0x3EFFFF34u),
    (VUINT32) (0x3E2AACACu),
    (VUINT32) (0x3D2B8392u),
    (VUINT32) (0x3C07D9FEu),
    (VUINT32) (0x0000007fu),
    (VUINT32) (0x7fffffffu),
    (VUINT32) (0x42819f00u),
    (VUINT32) (0x33800000u),
    (VUINT32) (0x7f7fffffu),
    (VUINT32) (0x00800000u)

};

__constant VUINT32 _vmlsPowHATab[380] =
{

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


typedef struct
{

  uint sHiMask;
  uint sRSValue;
  uint NMINNORM;
  uint NMAXVAL;
  uint INF;

  uint LFR_TBL[65][3][1];
  uint LFR_I_CHK_WORK_SUB;
  uint LFR_I_CHK_WORK_CMP;
  uint S_MANT_MASK;
  uint S_ONE;
  uint LFR_I_INDEX_MASK;
  uint LFR_I_INDEX_ADD;
  uint S_HI10BITS_MASK;
  uint LFR_S_P4;
  uint LFR_S_P3;
  uint LFR_S_P2;
  uint I_BIAS;
  uint LFR_I_NZ_ADD;
  uint LFR_I_NZ_CMP;
  uint S_LOG2_HI;
  uint S_LOG2_LO;

  uint _sInvLn2;
  uint _sShifter;
  uint _sLn2hi;
  uint _sLn2lo;
  uint _sPC0;
  uint _sPC1;
  uint _sPC2;
  uint _sPC3;
  uint _sPC4;
  uint _sPC5;
  uint _iBias;
  uint _iAbsMask;
  uint _iDomainRange;
  uint _s2N_2;
  uint _sHuge;
  uint _sTiny;

} sRootn_Table_Type;

__constant sRootn_Table_Type __ocl_svml_srootn_data = {

  (uint) (0xFFFFF000u),
  (uint) (0x45800800u),
  (uint) (0x80800000u),
  (uint) (0xfeffffffu),
  (uint) (0x7f800000u),

  {
   {{(uint) (0x00000000u)},
    {(uint) (0x00000000u)},
    {(uint) (0x3F800000u)}},
   {{(uint) (0x3C810000u)},
    {(uint) (0x35ACB127u)},
    {(uint) (0x3F7C0000u)}},
   {{(uint) (0x3D020000u)},
    {(uint) (0x372EC4F4u)},
    {(uint) (0x3F780000u)}},
   {{(uint) (0x3D33C000u)},
    {(uint) (0x38129E5Bu)},
    {(uint) (0x3F750000u)}},
   {{(uint) (0x3D774000u)},
    {(uint) (0x378C7002u)},
    {(uint) (0x3F710000u)}},
   {{(uint) (0x3D9DE000u)},
    {(uint) (0x37FAD3E9u)},
    {(uint) (0x3F6D0000u)}},
   {{(uint) (0x3DB80000u)},
    {(uint) (0x37530AEBu)},
    {(uint) (0x3F6A0000u)}},
   {{(uint) (0x3DD26000u)},
    {(uint) (0x381D902Cu)},
    {(uint) (0x3F670000u)}},
   {{(uint) (0x3DED2000u)},
    {(uint) (0x3849D8E1u)},
    {(uint) (0x3F640000u)}},
   {{(uint) (0x3E08B000u)},
    {(uint) (0x38474114u)},
    {(uint) (0x3F600000u)}},
   {{(uint) (0x3E168000u)},
    {(uint) (0x38308643u)},
    {(uint) (0x3F5D0000u)}},
   {{(uint) (0x3E248000u)},
    {(uint) (0x381EC19Au)},
    {(uint) (0x3F5A0000u)}},
   {{(uint) (0x3E2DF000u)},
    {(uint) (0x382035AAu)},
    {(uint) (0x3F580000u)}},
   {{(uint) (0x3E3C4000u)},
    {(uint) (0x3846C2A2u)},
    {(uint) (0x3F550000u)}},
   {{(uint) (0x3E4AD000u)},
    {(uint) (0x3735B9EEu)},
    {(uint) (0x3F520000u)}},
   {{(uint) (0x3E598000u)},
    {(uint) (0x386C2BAEu)},
    {(uint) (0x3F4F0000u)}},
   {{(uint) (0x3E637000u)},
    {(uint) (0x387DE378u)},
    {(uint) (0x3F4D0000u)}},
   {{(uint) (0x3E729000u)},
    {(uint) (0x38077FF4u)},
    {(uint) (0x3F4A0000u)}},
   {{(uint) (0x3E7CC000u)},
    {(uint) (0x380E365Au)},
    {(uint) (0x3F480000u)}},
   {{(uint) (0x3E862000u)},
    {(uint) (0x37359D86u)},
    {(uint) (0x3F450000u)}},
   {{(uint) (0x3E8B5800u)},
    {(uint) (0x37B9975Au)},
    {(uint) (0x3F430000u)}},
   {{(uint) (0x3E90A000u)},
    {(uint) (0x378ADA1Du)},
    {(uint) (0x3F410000u)}},
   {{(uint) (0x3E95F000u)},
    {(uint) (0x38707CDDu)},
    {(uint) (0x3F3F0000u)}},
   {{(uint) (0x3E9E1000u)},
    {(uint) (0x37A4EE66u)},
    {(uint) (0x3F3C0000u)}},
   {{(uint) (0x3EA38800u)},
    {(uint) (0x380DC272u)},
    {(uint) (0x3F3A0000u)}},
   {{(uint) (0x3EA91000u)},
    {(uint) (0x382E0739u)},
    {(uint) (0x3F380000u)}},
   {{(uint) (0x3EAEA800u)},
    {(uint) (0x383DDF59u)},
    {(uint) (0x3F360000u)}},
   {{(uint) (0x3EB45000u)},
    {(uint) (0x38483E9Cu)},
    {(uint) (0x3F340000u)}},
   {{(uint) (0x3EBA0800u)},
    {(uint) (0x385876C6u)},
    {(uint) (0x3F320000u)}},
   {{(uint) (0x3EBFD000u)},
    {(uint) (0x387A3BD8u)},
    {(uint) (0x3F300000u)}},
   {{(uint) (0x3EC5B000u)},
    {(uint) (0x3766A22Du)},
    {(uint) (0x3F2E0000u)}},
   {{(uint) (0x3ECB9800u)},
    {(uint) (0x38234313u)},
    {(uint) (0x3F2C0000u)}},
   {{(uint) (0x3ECE9800u)},
    {(uint) (0x372FA858u)},
    {(uint) (0x3F2B0000u)}},
   {{(uint) (0x3ED49800u)},
    {(uint) (0x386D3C8Bu)},
    {(uint) (0x3F290000u)}},
   {{(uint) (0x3EDAB000u)},
    {(uint) (0x387A0446u)},
    {(uint) (0x3F270000u)}},
   {{(uint) (0x3EE0E000u)},
    {(uint) (0x37C0D27Fu)},
    {(uint) (0x3F250000u)}},
   {{(uint) (0x3EE3F800u)},
    {(uint) (0x3879C745u)},
    {(uint) (0x3F240000u)}},
   {{(uint) (0x3EEA4800u)},
    {(uint) (0x3511B7BFu)},
    {(uint) (0x3F220000u)}},
   {{(uint) (0x3EED7000u)},
    {(uint) (0x37EA9099u)},
    {(uint) (0x3F210000u)}},
   {{(uint) (0x3EF3D800u)},
    {(uint) (0x378587BBu)},
    {(uint) (0x3F1F0000u)}},
   {{(uint) (0x3EF71000u)},
    {(uint) (0x382156B4u)},
    {(uint) (0x3F1E0000u)}},
   {{(uint) (0x3EFD9800u)},
    {(uint) (0x37B15EF5u)},
    {(uint) (0x3F1C0000u)}},
   {{(uint) (0x3F007000u)},
    {(uint) (0x3835CB64u)},
    {(uint) (0x3F1B0000u)}},
   {{(uint) (0x3F03C400u)},
    {(uint) (0x37FC14CFu)},
    {(uint) (0x3F190000u)}},
   {{(uint) (0x3F057000u)},
    {(uint) (0x386DC5A1u)},
    {(uint) (0x3F180000u)}},
   {{(uint) (0x3F08D400u)},
    {(uint) (0x3870478Fu)},
    {(uint) (0x3F160000u)}},
   {{(uint) (0x3F0A8C00u)},
    {(uint) (0x3807EDE5u)},
    {(uint) (0x3F150000u)}},
   {{(uint) (0x3F0C4400u)},
    {(uint) (0x385C81E4u)},
    {(uint) (0x3F140000u)}},
   {{(uint) (0x3F0FC000u)},
    {(uint) (0x3846BF8Cu)},
    {(uint) (0x3F120000u)}},
   {{(uint) (0x3F118400u)},
    {(uint) (0x37C362FBu)},
    {(uint) (0x3F110000u)}},
   {{(uint) (0x3F134800u)},
    {(uint) (0x3844226Au)},
    {(uint) (0x3F100000u)}},
   {{(uint) (0x3F16DC00u)},
    {(uint) (0x386AAF62u)},
    {(uint) (0x3F0E0000u)}},
   {{(uint) (0x3F18AC00u)},
    {(uint) (0x38348868u)},
    {(uint) (0x3F0D0000u)}},
   {{(uint) (0x3F1A8000u)},
    {(uint) (0x37A2B676u)},
    {(uint) (0x3F0C0000u)}},
   {{(uint) (0x3F1C5400u)},
    {(uint) (0x38442E33u)},
    {(uint) (0x3F0B0000u)}},
   {{(uint) (0x3F1E3000u)},
    {(uint) (0x3680C36Cu)},
    {(uint) (0x3F0A0000u)}},
   {{(uint) (0x3F200C00u)},
    {(uint) (0x3761092Eu)},
    {(uint) (0x3F090000u)}},
   {{(uint) (0x3F23D000u)},
    {(uint) (0x37293F45u)},
    {(uint) (0x3F070000u)}},
   {{(uint) (0x3F25B400u)},
    {(uint) (0x387ADF2Eu)},
    {(uint) (0x3F060000u)}},
   {{(uint) (0x3F27A000u)},
    {(uint) (0x383506B5u)},
    {(uint) (0x3F050000u)}},
   {{(uint) (0x3F299000u)},
    {(uint) (0x37B89A1Au)},
    {(uint) (0x3F040000u)}},
   {{(uint) (0x3F2B8000u)},
    {(uint) (0x38744D77u)},
    {(uint) (0x3F030000u)}},
   {{(uint) (0x3F2D7800u)},
    {(uint) (0x3800B86Du)},
    {(uint) (0x3F020000u)}},
   {{(uint) (0x3F2F7400u)},
    {(uint) (0x35AA8906u)},
    {(uint) (0x3F010000u)}},
   {{(uint) (0x3F317000u)},
    {(uint) (0x3805FDF4u)},
    {(uint) (0x3F000000u)}},
   },
  (uint) (0x80800000u),
  (uint) (0xFF000000u),
  (uint) (0x007fffffu),
  (uint) (0x3f800000u),
  (uint) (0x007F0000u),
  (uint) (0x00010000u),
  (uint) (0x7fffc000u),
  (uint) (0xBE800000u),
  (uint) (0x3EAAAAABu),
  (uint) (0xBF000000u),
  (uint) (0x0000007fu),
  (uint) (0x407e0000u),
  (uint) (0x7ffc0000u),
  (uint) (0x3F317000u),
  (uint) (0x3805FDF4u),

  (uint) (0x3FB8AA3Bu),
  (uint) (0x4b400000u),
  (uint) (0x3F317200u),
  (uint) (0x35BFBE8Eu),
  (uint) (0x3F800000u),
  (uint) (0x3F7FFFFEu),
  (uint) (0x3EFFFF34u),
  (uint) (0x3E2AACACu),
  (uint) (0x3D2B8392u),
  (uint) (0x3C07D9FEu),
  (uint) (0x0000007fu),
  (uint) (0x7fffffffu),
  (uint) (0x42ae9a00u),
  (uint) (0x33800000u),
  (uint) (0x7f7fffffu),
  (uint) (0x00800000u)

};



__constant _iml_sp_union_t _vmlsRootnHATab[380] = {

  { 0x3F800000 },
  { 0x3F640000 },
  { 0x3F4C0000 },
  { 0x3F3A0000 },
  { 0x3F2A0000 },
  { 0x3F1E0000 },
  { 0x3F120000 },
  { 0x3F080000 },
  { 0x3F000000 },

  { 0x00000000 },
  { 0x00000000 },
  { 0x3E2B1E00 },
  { 0x36614FFD },
  { 0x3EA7B700 },
  { 0x36DD9676 },
  { 0x3EEBF300 },
  { 0x3640ABC3 },
  { 0xBED19B00 },
  { 0xB6B053FB },
  { 0xBE9B8900 },
  { 0xB599D49A },
  { 0xBE426000 },
  { 0xB6AF40BC },
  { 0xBDB31C00 },
  { 0xB6EDF592 },
  { 0x00000000 },
  { 0x00000000 },

  { 0x3F800000 },
  { 0x3F780000 },
  { 0x3F700000 },
  { 0x3F8A0000 },
  { 0x3F880000 },
  { 0x3F860000 },
  { 0x3F840000 },
  { 0x3F820000 },
  { 0x3F800000 },

  { 0x00000000 },
  { 0x00000000 },
  { 0x3D3B9800 },
  { 0x3694C9D9 },
  { 0x3DBEB000 },
  { 0x3492D9F7 },
  { 0xBDDE4000 },
  { 0xB684815B },
  { 0xBDB31C00 },
  { 0xB6EDF592 },
  { 0xBD875800 },
  { 0xB6627E8A },
  { 0xBD35D000 },
  { 0xB6D3758F },
  { 0xBCB73000 },
  { 0xB6CB42E1 },
  { 0x00000000 },
  { 0x00000000 },

  { 0x3FB88000 },
  { 0x3FB7C000 },
  { 0x3FB70000 },
  { 0x3FB64000 },
  { 0x3FB5C000 },
  { 0x3FBC8000 },
  { 0x3FBC4000 },
  { 0x3FBBC000 },
  { 0x3FBB8000 },
  { 0x3FBB0000 },
  { 0x3FBAC000 },
  { 0x3FBA4000 },
  { 0x3FBA0000 },
  { 0x3FB98000 },
  { 0x3FB94000 },
  { 0x3FB8C000 },
  { 0x3FB88000 },

  { 0x00000000 },
  { 0x00000000 },
  { 0x3BC08000 },
  { 0x3601B0EA },
  { 0x3C40E000 },
  { 0x36A82CE1 },
  { 0x3C910000 },
  { 0x35F27427 },
  { 0x3CB17000 },
  { 0x36BBF0CC },
  { 0xBCFD7000 },
  { 0xB6DA84F4 },
  { 0xBCEDC000 },
  { 0xB6E53CD7 },
  { 0xBCCE5000 },
  { 0xB6FA51D3 },
  { 0xBCBEA000 },
  { 0xB4074B50 },
  { 0xBC9F1000 },
  { 0xB52D128E },
  { 0xBC8F4000 },
  { 0xB5655E44 },
  { 0xBC5F2000 },
  { 0xB59903D9 },
  { 0xBC3F6000 },
  { 0xB5A1551A },
  { 0xBBFF8000 },
  { 0xB5979427 },
  { 0xBBBFC000 },
  { 0xB5839E88 },
  { 0xBB000000 },
  { 0xB4E32477 },
  { 0x00000000 },
  { 0x00000000 },

  { 0x3F800000 },
  { 0x00000000 },
  { 0x3F80B1EE },
  { 0xB3B02666 },
  { 0x3F8164D2 },
  { 0xB1C43FD0 },
  { 0x3F8218B0 },
  { 0xB3BC8C04 },
  { 0x3F82CD86 },
  { 0x3398AC2C },
  { 0x3F83835A },
  { 0xB3B11049 },
  { 0x3F843A28 },
  { 0x33C3ACDE },
  { 0x3F84F1F6 },
  { 0x332C6F38 },
  { 0x3F85AAC4 },
  { 0xB39833B8 },
  { 0x3F866492 },
  { 0xB3A46DC0 },
  { 0x3F871F62 },
  { 0xB352C2E6 },
  { 0x3F87DB36 },
  { 0xB3800967 },
  { 0x3F88980E },
  { 0x338092DB },
  { 0x3F8955EE },
  { 0x30D86398 },
  { 0x3F8A14D6 },
  { 0xB38AB691 },
  { 0x3F8AD4C6 },
  { 0x330A58E5 },
  { 0x3F8B95C2 },
  { 0xB260ABA1 },
  { 0x3F8C57CA },
  { 0xB2EE6E43 },
  { 0x3F8D1AE0 },
  { 0xB3A481A4 },
  { 0x3F8DDF04 },
  { 0x32808B9A },
  { 0x3F8EA43A },
  { 0xB3697465 },
  { 0x3F8F6A82 },
  { 0xB3E81937 },
  { 0x3F9031DC },
  { 0x330628CD },
  { 0x3F90FA4C },
  { 0x338BEEE5 },
  { 0x3F91C3D4 },
  { 0xB38C54EE },
  { 0x3F928E72 },
  { 0x337B2A64 },
  { 0x3F935A2C },
  { 0xB3D0EC19 },
  { 0x3F942700 },
  { 0xB3F054E4 },
  { 0x3F94F4F0 },
  { 0xB32E0212 },
  { 0x3F95C3FE },
  { 0x3386D6CC },
  { 0x3F96942E },
  { 0xB3C8DFE8 },
  { 0x3F97657E },
  { 0xB3B60E85 },
  { 0x3F9837F0 },
  { 0x33231B71 },
  { 0x3F990B88 },
  { 0xB26CC9F4 },
  { 0x3F99E046 },
  { 0xB359BE90 },
  { 0x3F9AB62A },
  { 0x33FC9500 },
  { 0x3F9B8D3A },
  { 0xB30C5563 },
  { 0x3F9C6574 },
  { 0xB397D13D },
  { 0x3F9D3EDA },
  { 0xB331A601 },
  { 0x3F9E196E },
  { 0x3244EA39 },
  { 0x3F9EF532 },
  { 0x33412342 },
  { 0x3F9FD228 },
  { 0x32959003 },
  { 0x3FA0B052 },
  { 0xB3F0468F },
  { 0x3FA18FAE },
  { 0x33CA8545 },
  { 0x3FA27044 },
  { 0xB3FCF3B7 },
  { 0x3FA35210 },
  { 0xB39717FD },
  { 0x3FA43516 },
  { 0xB323EC33 },
  { 0x3FA51958 },
  { 0xB37282C2 },
  { 0x3FA5FED6 },
  { 0x33A9B151 },
  { 0x3FA6E594 },
  { 0x33CFEEE8 },
  { 0x3FA7CD94 },
  { 0xB3162D36 },
  { 0x3FA8B6D6 },
  { 0xB3E984CE },
  { 0x3FA9A15A },
  { 0x33B4EA7C },
  { 0x3FAA8D26 },
  { 0x3325D921 },
  { 0x3FAB7A3A },
  { 0xB314AD82 },
  { 0x3FAC6896 },
  { 0x33A4BE40 },
  { 0x3FAD583E },
  { 0x33EA42A1 },
  { 0x3FAE4934 },
  { 0x3325946B },
  { 0x3FAF3B78 },
  { 0x33AD690A },
  { 0x3FB02F0E },
  { 0xB2D1247F },
  { 0x3FB123F6 },
  { 0xB37C5AA8 },
  { 0x3FB21A32 },
  { 0xB33333CE },
  { 0x3FB311C4 },
  { 0x32154889 },
  { 0x3FB40AAE },
  { 0x33A2654C },
  { 0x3FB504F4 },
  { 0xB3CC0622 },
  { 0x3FB60094 },
  { 0xB32F4254 },
  { 0x3FB6FD92 },
  { 0xB266B974 },
  { 0x3FB7FBF0 },
  { 0xB2D5CD70 },
  { 0x3FB8FBB0 },
  { 0xB3B89D04 },
  { 0x3FB9FCD2 },
  { 0x330A5817 },
  { 0x3FBAFF5A },
  { 0x33B2133E },
  { 0x3FBC034A },
  { 0x337DE5D4 },
  { 0x3FBD08A4 },
  { 0xB3414FE8 },
  { 0x3FBE0F68 },
  { 0x31986099 },
  { 0x3FBF179A },
  { 0xB3130B1A },
  { 0x3FC0213A },
  { 0x33A1F0D1 },
  { 0x3FC12C4C },
  { 0x33CA6671 },
  { 0x3FC238D2 },
  { 0x32C478F6 },
  { 0x3FC346CC },
  { 0x33DA2497 },
  { 0x3FC4563E },
  { 0x33CC5335 },
  { 0x3FC5672A },
  { 0x320AA837 },
  { 0x3FC67990 },
  { 0x33B5AA24 },
  { 0x3FC78D74 },
  { 0x33C8ABBA },
  { 0x3FC8A2D8 },
  { 0x33391FFC },
  { 0x3FC9B9BE },
  { 0xB37323A2 },
  { 0x3FCAD226 },
  { 0x333C8521 },
  { 0x3FCBEC14 },
  { 0x33FEF272 },
  { 0x3FCD078C },
  { 0xB3735F84 },
  { 0x3FCE248C },
  { 0x3228FC24 },
  { 0x3FCF4318 },
  { 0x33CF1919 },
  { 0x3FD06334 },
  { 0xB2944353 },
  { 0x3FD184E0 },
  { 0xB39DAE96 },
  { 0x3FD2A81E },
  { 0xB35C1DAA },
  { 0x3FD3CCF0 },
  { 0x3399859B },
  { 0x3FD4F35A },
  { 0x33ABCFEE },
  { 0x3FD61B5E },
  { 0xB0303219 },
  { 0x3FD744FC },
  { 0x33CAD69D },
  { 0x3FD8703A },
  { 0xB3B3924D },
  { 0x3FD99D16 },
  { 0xB2F61D41 },
  { 0x3FDACB94 },
  { 0x335E5594 },
  { 0x3FDBFBB8 },
  { 0xB3504A1C },
  { 0x3FDD2D82 },
  { 0xB375EF9B },
  { 0x3FDE60F4 },
  { 0x33825E0F },
  { 0x3FDF9612 },
  { 0x33DEB8F0 },
  { 0x3FE0CCDE },
  { 0x33EC2A95 },
  { 0x3FE2055A },
  { 0x33FFFE84 },
  { 0x3FE33F8A },
  { 0xB38D4176 },
  { 0x3FE47B6C },
  { 0x33A0373E },
  { 0x3FE5B906 },
  { 0x33E77C83 },
  { 0x3FE6F85A },
  { 0x33AAEE20 },
  { 0x3FE8396A },
  { 0x33207898 },
  { 0x3FE97C38 },
  { 0x3300D89F },
  { 0x3FEAC0C6 },
  { 0x33E7DD24 },
  { 0x3FEC0718 },
  { 0x33B64C1D },
  { 0x3FED4F30 },
  { 0x3276CCA1 },
  { 0x3FEE9910 },
  { 0xB34FE4BA },
  { 0x3FEFE4BA },
  { 0xB348464A },
  { 0x3FF13230 },
  { 0x33A7AD09 },
  { 0x3FF28178 },
  { 0xB3C3A600 },
  { 0x3FF3D290 },
  { 0xB2871670 },
  { 0x3FF5257E },
  { 0xB3EADB79 },
  { 0x3FF67A42 },
  { 0xB3938CC0 },
  { 0x3FF7D0E0 },
  { 0xB38CF52F },
  { 0x3FF9295A },
  { 0xB3094457 },
  { 0x3FFA83B2 },
  { 0x33DB722A },
  { 0x3FFBDFEE },
  { 0xB3931A0F },
  { 0x3FFD3E0C },
  { 0x31CF486C },
  { 0x3FFE9E12 },
  { 0xB3A38470 },

  { 0x3A6A6369 },
  { 0xBEB1C35D },
  { 0x3E246F69 },
  { 0xBDAB1EA1 },

  { 0x3F317218 },
  { 0x3E75FDF0 },
  { 0x3D635847 },

  { 0x7F000000 },
  { 0x00800000 },
  { 0x00000000 },
  { 0x3F800000 },
  { 0xBF800000 },

  { 0x47C00000 },

  { 0x3FB88000 },

  { 0x45800800 },

  { 0x5F800000 },
  { 0x1F800000 },
  { 0x00000000 },
  { 0x80000000 },
};

#endif // __SVML_DATA_STRUCTURES_CL__

#ifndef __SVML_TEST_INT__
#define __SVML_TEST_INT__

__attribute__((always_inline))
int _TestInt (float a)
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

int _TestIntI (int a)

{

    int iRes = (a & 1) ? 1 : 2;

    return iRes;

}

#endif // __SVML_TEST_INT__

float __ocl_svml_spown_cout_rare (const float a, const int b)
{
    float r;

    float flVTmp1, flVTmp2, flVPHH, flVPHL;
    float flAX, flSignRes, flX1, flRcp1, flL1Hi, flL1Lo, flX2, flRcp2, flL2Hi,
        flL2Lo, flX3, flRcp3C, flL3Hi, flL3Lo, flK, flT, flD, flR1, flCQ, flRcpC,
        flX1Hi, flX1Lo, flRcpCHi, flRcpCLo, flTmp1, flE, flT_CQHi, flCQLo, flR,
        flLogPart3, flLog2Poly, flHH, flHL, flHLL, flYHi, flYLo, flTmp2, flTmp3,
        flPH, flPL, flPLL, flZ, flExp2Poly, flExp2PolyT, flResLo, flResHi, flRes,
        flTwoPowN, flAi, flBi;
    float flT_lo_1, flT_lo_2, flT_lo_3;

    int iEXB, iEYB, iSignX, iSignY, iYIsFinite, iEY, iYIsInt,
        iXIsFinite, iDenoExpAdd, iXHi, k, i1, i2, i3, iELogAX, iN, j, iERes,
        iSign, iIsSigZeroX, iIsSigZeroY, iYMantissa, iEX, iBi;

    iBi = b;
    flAi = a;
    flBi = (float) (iBi);

    iEXB = ((as_uint(flAi) >> 23) & 0xFF);
    iEYB = ((as_uint(flBi) >> 23) & 0xFF);

    iEX = iEXB - 0x7F;
    iEY = iEYB - 0x7F;

    iSignX = (as_uint(flAi) >> 31);
    iSignY = (as_uint(flBi) >> 31);

    iIsSigZeroX = ((as_uint(flAi) & 0x007FFFFF) == 0);
    iIsSigZeroY = ((as_uint(flBi) & 0x007FFFFF) == 0);

    iYIsFinite = 1;

    iYMantissa = (as_uint(flBi) & 0x007FFFFF);

    iYIsInt = _TestIntI (iBi);

    if (!((iSignX == 0) && (iEXB == 0x7F) && iIsSigZeroX) &&
        !((iEYB == 0) && iIsSigZeroY))
    {
        iXIsFinite =
        (((as_uint(flAi) >> 23) & 0xFF) != 0xFF);

        if ((iXIsFinite || iIsSigZeroX) && (iYIsFinite || iIsSigZeroY))
        {
            if (flAi != ((__constant float *) _vmlsPowHATab)[370])
            {
                if (!((flAi == ((__constant float *) _vmlsPowHATab)[372])
                && (iYIsInt || !iYIsFinite)))
                {
                    if (iXIsFinite && iYIsFinite)
                    {
                        if ((flAi > ((__constant float *) _vmlsPowHATab)[370])
                        || iYIsInt)
                        {
                            flSignRes =
                                ((__constant float *) _vmlsPowHATab)[371 +
                                                (iSignX & iYIsInt)];
                            iDenoExpAdd = 0;
                            flAX = flAi;
                            flAX = as_float(
                            (as_uint(flAX) & 0x7FFFFFFF)
                            | ((uint) (0) << 31));

                            if (iEXB == 0)
                            {
                                flAX =
                                flAX * ((__constant float *) _vmlsPowHATab)[376];
                                iDenoExpAdd = iDenoExpAdd - 64;
                            }

                            flX1 = flAX;
                            flX1 = as_float((as_uint(flX1) & 0x807FFFFF)
                                    | (((uint) (0x7F) & 0xFF) << 23));

                            iXHi = ((as_uint(flAX) >> 23) & 0xFF);
                            iXHi = iXHi << 23;
                            iXHi = iXHi | (as_uint(flAX) & 0x007FFFFF);

                            k = iXHi - 0x3F380000;
                            k = k >> 23;
                            k = k + iDenoExpAdd;

                            i1 = (as_uint(flX1) & 0x007FFFFF);
                            i1 = i1 & 0x780000;
                            i1 = i1 + 0x80000;
                            i1 = i1 >> 20;

                            flRcp1 = ((__constant float *) _vmlsPowHATab)[0 + i1];

                            flL1Hi =
                                ((__constant float *) _vmlsPowHATab)[9 + 2 * (i1) + 0];
                            flL1Lo =
                                ((__constant float *) _vmlsPowHATab)[9 + 2 * (i1) + 1];

                            flX2 = flX1 * flRcp1;

                            i2 = (as_uint(flX2) & 0x007FFFFF);
                            i2 = i2 & 0x1E0000;
                            i2 = i2 + 0x20000;
                            i2 = i2 >> 18;

                            flRcp2 = ((__constant float *) _vmlsPowHATab)[27 + i2];

                            flL2Hi =
                                ((__constant float *) _vmlsPowHATab)[36 + 2 * (i2) +
                                                0];
                            flL2Lo =
                                ((__constant float *) _vmlsPowHATab)[36 + 2 * (i2) +
                                                1];

                            flX3 = (flX2 * flRcp2);

                            i3 =
                                (as_uint(flX3) & 0x007FFFFF);
                            i3 = i3 & 0x7C000;
                            i3 = i3 + 0x4000;
                            i3 = i3 >> 15;

                            flRcp3C = ((__constant float *) _vmlsPowHATab)[54 + i3];

                            flL3Hi =
                                ((__constant float *) _vmlsPowHATab)[71 + 2 * (i3) +
                                                0];
                            flL3Lo =
                                ((__constant float *) _vmlsPowHATab)[71 + 2 * (i3) +
                                                1];

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
                            flCQ =
                                (flR1 - ((__constant float *) _vmlsPowHATab)[374]);

                            flRcpC = (flRcp1 * flRcp2);
                            flRcpC = (flRcpC * flRcp3C);

                            flVTmp1 =
                                ((flX1) * (((__constant float *) _vmlsPowHATab)[375]));
                            flVTmp2 = (flVTmp1 - (flX1));
                            flVTmp1 = (flVTmp1 - flVTmp2);
                            flVTmp2 = ((flX1) - flVTmp1);
                            flX1Hi = flVTmp1;
                            flX1Lo = flVTmp2;

                            flVTmp1 = ((flRcpC) *
                                (((__constant float *) _vmlsPowHATab)[375]));
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

                            iELogAX =
                                ((as_uint(flT_CQHi) >> 23) & 0xFF);

                            if (iELogAX + iEYB < 11 + 2 * 0x7F)
                            {
                                if (iELogAX + iEYB > -62 + 2 * 0x7F)
                                {
                                    flR = (flCQ + flE);

                                    flLog2Poly =
                                        ((((((__constant float *) _vmlsPowHATab)[364])
                                        * flR +
                                        ((__constant float *) _vmlsPowHATab)[363]) *
                                        flR +
                                        ((__constant float *) _vmlsPowHATab)[362]) *
                                        flR +
                                        ((__constant float *) _vmlsPowHATab)[361]) *
                                        flR;

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

                                    flVTmp1 =
                                        ((flHH) *
                                        (((__constant float *) _vmlsPowHATab)[375]));
                                    flVTmp2 = (flVTmp1 - (flHH));
                                    flVTmp1 = (flVTmp1 - flVTmp2);
                                    flVTmp2 = ((flHH) - flVTmp1);
                                    flHH = flVTmp1;
                                    flHL = flVTmp2;

                                    flVTmp1 =
                                        ((flBi) *
                                        (((__constant float *) _vmlsPowHATab)[375]));
                                    flVTmp2 = (flVTmp1 - (flBi));
                                    flVTmp1 = (flVTmp1 - flVTmp2);
                                    flVTmp2 = ((flBi) - flVTmp1);
                                    flYHi = flVTmp1;
                                    flYLo = flVTmp2;

                                    flYLo += (float) (iBi - (int) flBi);

                                    flTmp1 = ((flYHi) * (flHH));
                                    flTmp2 = ((flYLo) * (flHL));
                                    flTmp2 = (flTmp2 + (flYHi) * (flHL));
                                    flTmp3 = (flTmp2 + (flYLo) * (flHH));
                                    flPH = flTmp1;
                                    flPL = flTmp3;

                                    flPLL = (flBi * flHLL);

                                    flVTmp1 =
                                        (flPH +
                                        ((__constant float *) _vmlsPowHATab)[373]);
                                    flVPHH =
                                        (flVTmp1 -
                                        ((__constant float *) _vmlsPowHATab)[373]);
                                    iN =
                                        (as_uint(flVTmp1) & 0x007FFFFF);
                                    j = iN & 0x7F;

                                    iN = iN << 10;
                                    iN = iN >> (7 + 10);
                                    flVPHL = (flPH - flVPHH);

                                    flZ = (flPLL + flPL);
                                    flZ = (flZ + flVPHL);

                                    flExp2Poly =
                                        (((((__constant float *) _vmlsPowHATab)[367]) *
                                        flZ +
                                        ((__constant float *) _vmlsPowHATab)[366]) *
                                        flZ +
                                        ((__constant float *) _vmlsPowHATab)[365]) *
                                        flZ;

                                    flExp2PolyT =
                                        (flExp2Poly *
                                        ((__constant float *) _vmlsPowHATab)[105 +
                                                        2 * (j) +
                                                        0]);
                                    flResLo =
                                        (flExp2PolyT +
                                        ((__constant float *) _vmlsPowHATab)[105 +
                                                        2 * (j) +
                                                        1]);
                                    flResHi =
                                        ((__constant float *) _vmlsPowHATab)[105 +
                                                        2 * (j) +
                                                        0];

                                    flRes = (flResHi + flResLo);
                                    iERes =
                                        ((as_uint(flRes) >> 23) & 0xFF);
                                    iERes = (iERes - 0x7F);
                                    iERes = (iERes + iN);

                                    if (iERes < 128)
                                    {
                                        if (iERes >= -126)
                                        {
                                            flRes = as_float(
                                            (as_uint(flRes) & 0x807FFFFF) |
                                            (((uint) (iERes + 0x7F) &
                                                0xFF) << 23));

                                            flRes = (flRes * flSignRes);
                                            r = flRes;
                                        }
                                        else
                                        {
                                            if (iERes >= -126 - 10)
                                            {
                                                flVTmp1 =
                                                ((flResHi) + (flResLo));
                                                flTmp1 = ((flResHi) - flVTmp1);
                                                flVTmp2 = (flTmp1 + (flResLo));
                                                flResHi = flVTmp1;
                                                flResLo = flVTmp2;

                                                flVTmp1 =
                                                ((flResHi) *
                                                (((__constant float *)
                                                _vmlsPowHATab)[375]));
                                                flVTmp2 = (flVTmp1 - (flResHi));
                                                flVTmp1 = (flVTmp1 - flVTmp2);
                                                flVTmp2 = ((flResHi) - flVTmp1);
                                                flResHi = flVTmp1;
                                                flTmp2 = flVTmp2;

                                                flResLo = (flResLo + flTmp2);

                                                flSignRes *=
                                                ((__constant float *)
                                                _vmlsPowHATab)[377];
                                                iN = (iN + 64);

                                                flTwoPowN =
                                                ((__constant float *)
                                                _vmlsPowHATab)[371];
                                                flTwoPowN = as_float(
                                                (as_uint(flTwoPowN) & 0x807FFFFF) |
                                                (((uint) (iN + 0x7F) &
                                                0xFF) << 23));

                                                flResHi = (flResHi * flTwoPowN);
                                                flResHi = (flResHi * flSignRes);

                                                flResLo = (flResLo * flTwoPowN);
                                                flVTmp1 = (flResLo * flSignRes);

                                                flRes = (flResHi + flVTmp1);

                                                flVTmp1 =
                                                ((__constant float *)
                                                _vmlsPowHATab)[369];
                                                flVTmp1 = (flVTmp1 * flVTmp1);

                                                flRes = (flRes + flVTmp1);
                                                if (__FlushDenormals)
                                                {
                                                    r = 0.0f;
                                                }
                                                else
                                                {
                                                    r = flRes;
                                                }
                                            }
                                            else
                                            {
                                                if (iERes >= -149 - 10)
                                                {
                                                    flSignRes *=
                                                        ((__constant float *)
                                                        _vmlsPowHATab)[377];
                                                    iN = iN + 64;

                                                    flTwoPowN =
                                                        ((__constant float *)
                                                        _vmlsPowHATab)[371];
                                                        flTwoPowN = as_float(
                                                    (as_uint(flTwoPowN) & 0x807FFFFF) |
                                                    (((uint)
                                                        (iN + 0x7F) & 0xFF) << 23));

                                                    flRes = (flRes * flTwoPowN);
                                                    flRes = (flRes * flSignRes);

                                                    flVTmp1 =
                                                        ((__constant float *)
                                                        _vmlsPowHATab)[369];
                                                    flVTmp1 *= flVTmp1;
                                                    flRes = (flRes + flVTmp1);

                                                    r = flRes;
                                                }
                                                else
                                                {
                                                    flVTmp1 =
                                                        ((__constant float *)
                                                        _vmlsPowHATab)[369];
                                                    flVTmp1 *= flVTmp1;
                                                    flRes =
                                                        (flVTmp1 * flSignRes);
                                                    r = flRes;
                                                }
                                            }
                                        }
                                    }
                                    else
                                    {
                                        flVTmp1 =
                                        ((__constant float *) _vmlsPowHATab)[368];
                                        flVTmp1 = (flVTmp1 * flVTmp1);
                                        flRes = (flVTmp1 * flSignRes);
                                        r = flRes;
                                    }
                                }
                                else
                                {
                                    flVTmp1 =
                                        ((__constant float *) _vmlsPowHATab)[371];
                                    flVTmp1 =
                                        (flVTmp1 +
                                        ((__constant float *) _vmlsPowHATab)[369]);
                                    r = (flVTmp1 * flSignRes);
                                }
                            }
                            else
                            {
                                iSign =
                                iSignY ^ (as_uint(flT_CQHi) >> 31);

                                flTmp1 =
                                ((__constant float *) _vmlsPowHATab)[368 +
                                                (iSign)];

                                flTmp1 = (flTmp1 * flTmp1);

                                flTmp1 = (flTmp1 * flSignRes);
                                r = flTmp1;
                            }
                        }
                        else
                        {
                            flVTmp1 = ((__constant float *) _vmlsPowHATab)[370];
                            flVTmp1 = (flVTmp1 / flVTmp1);
                            r = flVTmp1;
                        }
                    }
                    else
                    {
                        if (iEXB < 0x7F)
                        {
                            if (iSignY)
                            {
                                r = (flBi * flBi);
                            }
                            else
                            {
                                r = ((__constant float *) _vmlsPowHATab)[370];
                            }
                        }
                        else
                        {
                            if (iSignY)
                            {
                                flRes =
                                ((__constant float *) _vmlsPowHATab)[378 +
                                                (iYIsInt &
                                                iSignX)];
                                r = flRes;
                            }
                            else
                            {
                                int iRes = iYIsInt & iSignX;
                                flTmp1 = (flAi * flAi);
                                flTmp1 = (flTmp1 * flBi);
                                flRes =
                                flTmp1 * ((__constant float *) _vmlsPowHATab)[371 +
                                                    (iRes)];
                                r = flRes;
                            }
                        }
                    }
                }
                else
                {
                    r = ((__constant float *) _vmlsPowHATab)[371 + (iYIsInt & 1)];
                }
            }
            else
            {
                flTmp1 = flAi * flAi;

                if (iSignY)
                {
                    r =
                        ((__constant float *) _vmlsPowHATab)[371 +
                                        (iYIsInt & iSignX)] /
                        flTmp1;
                }
                else
                {
                    r =
                        ((__constant float *) _vmlsPowHATab)[371 +
                                        (iYIsInt & iSignX)] *
                        flTmp1;
                }
            }
        }
        else
        {
            r = a + flBi;
        }
    }
    else
    {
        flVTmp1 = flAi + flBi;
        iSign = (as_uint(flVTmp1) >> 31);
        flVTmp2 = ((__constant float *) _vmlsPowHATab)[371];
        flVTmp2 = as_float(
        (as_uint(flVTmp2) & 0x7FFFFFFF) | ((uint) (iSign) << 31));

        r = flVTmp2 * flVTmp2;
    }

    return r;
}

float __ocl_svml_px_pownf1 (float a, int b)
{
    float va1;
    VUINT32 va2;
    float vr1;
    float r;
    VUINT32 vm;

    va1 = a;
    va2 = b;

    {

        float sHiMask;
        float sRSValue;
        float sZ[2];
        float sL[2];
        float sW[2];
        VUINT32 _NMINNORM;
        VUINT32 _NMAXVAL;
        VUINT32 _INF;
        VUINT32 iSpecX;
        VUINT32 iSpecY;
        VUINT32 LFR_iY;
        VUINT32 iRangeMask;

        VUINT32 LFR_iX;
        float LFR_sXMant;
        float LFR_sM;
        VUINT32 LFR_iInd;
        float LFR_sLnRcprYHi;
        float LFR_sLnRcprYLo;
        float LFR_sRcprY;
        float LFR_sYHi;
        float LFR_sYLo;
        float LFR_sYHiRcpY;
        float LFR_sRHi;
        float LFR_sTRHi;
        float LFR_sRLo;
        float LFR_sR;
        float LFR_sP;
        float LFR_sR2;
        VUINT32 LFR_iN;
        float LFR_sN;
        VUINT32 LFR_iXNearOne;
        float LFR_sXNearOne;
        float LFR_sNLog2Hi;
        float LFR_sNLog2Lo;
        float LFR_sWLo;
        float LFR_alfa;
        float LFR_sResHi;
        float LFR_beta;
        float LFR_sResLo;
        float S_MANT_MASK;
        float S_ONE;
        VUINT32 LFR_I_INDEX_MASK;
        VUINT32 LFR_I_INDEX_ADD;
        float S_HI10BITS_MASK;
        float LFR_S_P4;
        float LFR_S_P3;
        float LFR_S_P2;
        VUINT32 I_BIAS;
        VUINT32 LFR_I_NZ_ADD;
        VUINT32 LFR_I_NZ_CMP;
        float S_LOG2_HI;
        float S_LOG2_LO;

        float sN;
        float sR;
        float sP;
        float sM;
        VUINT32 iAbsZ;
        VUINT32 iRes;
        VUINT32 iP;
        VUINT32 iM;
        float sInvLn2;
        float sShifter;
        float sLn2hi;
        float sLn2lo;
        VUINT32 iAbsMask;
        VUINT32 iDomainRange;
        float sPC[6];

        float sX;
        float sY;
        VUINT32 iY;
        float sYLo;
        VUINT32 iYLo;
        VUINT32 iOddY;
        float sOddY;
        float sResultSign;
        float sAbsMask;

        sX = va1;
        sY = ((float) ((VINT32) (va2)));
        {
        int _cvt_iMantMask = 0x007fffff;
        int _cvt_iRightShifter = 0x4b000000;
        int _cvt_iBias = 0x00000096;
        int _cvt_iHighMantBit = 0x00800000;
        int _cvt_i2p23 = 0x4B000000;
        int _cvt_i2p32 = 0x4F800000;
        int _cvt_iNaN = 0x80000000;
        int _cvt_iX, _cvt_iMant, _cvt_iExp, _cvt_iRes1, _cvt_iRes2,
        _cvt_iRes;
        float _cvt_sAbsX, _cvt_sRes;
        float _cvt_sRightShifter = as_float(_cvt_iRightShifter);
        float _cvt_s2p23 = as_float(_cvt_i2p23);
        float _cvt_s2p32 = as_float(_cvt_i2p32);
        _cvt_sAbsX = (sY > 0) ? sY : (-sY);
        _cvt_sRes = _cvt_sAbsX + _cvt_sRightShifter;
        _cvt_iRes1 = as_uint(_cvt_sRes);
        _cvt_iRes1 = _cvt_iRes1 & _cvt_iMantMask;
        _cvt_iX = as_uint(_cvt_sAbsX);
        _cvt_iExp = _cvt_iX >> 23;
        _cvt_iExp = _cvt_iExp - _cvt_iBias;
        _cvt_iMant = _cvt_iX & _cvt_iMantMask;
        _cvt_iMant = _cvt_iMant | _cvt_iHighMantBit;
        _cvt_iRes2 = _cvt_iMant << _cvt_iExp;
        _cvt_iRes = (_cvt_sAbsX < _cvt_s2p23) ? _cvt_iRes1 : _cvt_iRes2;
        _cvt_iRes = (_cvt_sAbsX < _cvt_s2p32) ? _cvt_iRes : _cvt_iNaN;
        _cvt_iRes = (sY > 0) ? _cvt_iRes : (-_cvt_iRes);
        iY = _cvt_iRes;
        };
        iYLo = (va2 - iY);
        sYLo = ((float) ((VINT32) (iYLo)));

        iOddY = ((VUINT32) va2 << 31);
        sOddY = as_float(iOddY);
        sResultSign = as_float(as_uint(va1) & as_uint(sOddY));

        iAbsMask = as_uint(__ocl_svml_spow_data._iAbsMask);
        sAbsMask = as_float(iAbsMask);
        sX = as_float(as_uint(sX) & as_uint(sAbsMask));

        (LFR_iX) = as_uint(sX);
        (LFR_iY) = as_uint(sY);

        _NMINNORM = as_uint(__ocl_svml_spow_data.NMINNORM);
        _NMAXVAL = as_uint(__ocl_svml_spow_data.NMAXVAL);
        _INF = as_uint(__ocl_svml_spow_data.INF);

        iSpecX = (LFR_iX - _NMINNORM);
        iSpecX =
        ((VUINT32) (-(VSINT32) ((VSINT32) iSpecX >= (VSINT32) _NMAXVAL)));
        iSpecY = (LFR_iY & iAbsMask);
        iSpecY =
        ((VUINT32) (-(VSINT32) ((VSINT32) iSpecY >= (VSINT32) _INF)));
        iRangeMask = (iSpecX | iSpecY);

        LFR_I_NZ_ADD =
        as_uint(__ocl_svml_spow_data.LFR_I_NZ_ADD);
        LFR_iXNearOne = (LFR_iX + LFR_I_NZ_ADD);
        LFR_I_NZ_CMP =
        as_uint(__ocl_svml_spow_data.LFR_I_NZ_CMP);
        LFR_iXNearOne =
        ((VUINT32)
        (-(VSINT32) ((VSINT32) LFR_iXNearOne > (VSINT32) LFR_I_NZ_CMP)));
        LFR_sXNearOne = as_float(LFR_iXNearOne);
        S_MANT_MASK = as_float(__ocl_svml_spow_data.S_MANT_MASK);
        LFR_sXMant = as_float(as_uint(sX) & as_uint(S_MANT_MASK));
        S_ONE = as_float(__ocl_svml_spow_data.S_ONE);
        LFR_sM = as_float(as_uint(LFR_sXMant) | as_uint(S_ONE));
        LFR_iN = ((VUINT32) LFR_iX >> 23);
        I_BIAS = as_uint(__ocl_svml_spow_data.I_BIAS);
        LFR_iN = (LFR_iN - I_BIAS);
        LFR_sN = ((float) ((VINT32) (LFR_iN)));

        LFR_I_INDEX_MASK =
        as_uint(__ocl_svml_spow_data.LFR_I_INDEX_MASK);
        LFR_iInd = (LFR_iX & LFR_I_INDEX_MASK);

        LFR_I_INDEX_ADD =
        as_uint(__ocl_svml_spow_data.LFR_I_INDEX_ADD);
        LFR_iInd = (LFR_iInd + LFR_I_INDEX_ADD);

        LFR_iInd = ((VUINT32) LFR_iInd >> 17);
        {
            __constant char *_vlt_pPtr_[1];;
            {
                VUINT32 _vlt_sIndex_;
                VUINT32 _vlt_nIndex_;;
                {
                    VUINT32 _vsc_op1_tmp_;
                    _vsc_op1_tmp_ = ((VUINT32) LFR_iInd << 1);
                    _vsc_op1_tmp_ = (_vsc_op1_tmp_ + LFR_iInd);
                    _vlt_sIndex_ = ((VUINT32) _vsc_op1_tmp_ << 2);
                };
                _vlt_nIndex_ = _vlt_sIndex_;
                _vlt_pPtr_[0] =
                ((__constant char *) (__ocl_svml_spow_data.LFR_TBL)) + ((0) * (3 * 4)) +
                _vlt_nIndex_;
            }
            LFR_sLnRcprYHi = ((__constant float *) (_vlt_pPtr_[0]))[0];
            LFR_sLnRcprYLo = ((__constant float *) (_vlt_pPtr_[0]))[1];
            LFR_sRcprY = ((__constant float *) (_vlt_pPtr_[0]))[2];
        }

        S_HI10BITS_MASK =
        as_float(__ocl_svml_spow_data.S_HI10BITS_MASK);
        LFR_sYHi = as_float(as_uint(LFR_sM) & as_uint(S_HI10BITS_MASK));
        LFR_sYLo = (LFR_sM - LFR_sYHi);

        LFR_sYHiRcpY = (LFR_sYHi * LFR_sRcprY);
        LFR_sRHi = (LFR_sYHiRcpY - S_ONE);
        LFR_sTRHi = (LFR_sRHi + LFR_sLnRcprYHi);
        LFR_sRLo = (LFR_sYLo * LFR_sRcprY);
        LFR_sR = (LFR_sRHi + LFR_sRLo);

        LFR_S_P4 = as_float(__ocl_svml_spow_data.LFR_S_P4);
        LFR_S_P3 = as_float(__ocl_svml_spow_data.LFR_S_P3);
        LFR_sP = ((LFR_S_P4 * LFR_sR) + LFR_S_P3);

        LFR_S_P2 = as_float(__ocl_svml_spow_data.LFR_S_P2);
        LFR_sP = ((LFR_sP * LFR_sR) + LFR_S_P2);

        LFR_sR2 = (LFR_sR * LFR_sR);
        LFR_sP = (LFR_sP * LFR_sR2);

        S_LOG2_HI = as_float(__ocl_svml_spow_data.S_LOG2_HI);
        LFR_sNLog2Hi = (LFR_sN * S_LOG2_HI);

        S_LOG2_LO = as_float(__ocl_svml_spow_data.S_LOG2_LO);
        LFR_sNLog2Lo = (LFR_sN * S_LOG2_LO);

        LFR_sResHi = (LFR_sNLog2Hi + LFR_sTRHi);
        LFR_sWLo = (LFR_sNLog2Lo + LFR_sLnRcprYLo);
        LFR_sResLo = (LFR_sP + LFR_sWLo);
        LFR_alfa = as_float(as_uint(LFR_sXNearOne) & as_uint(LFR_sRLo));
        sL[0] = (LFR_sResHi + LFR_alfa);
        LFR_beta = as_float((~as_uint(LFR_sXNearOne)) & as_uint(LFR_sRLo));
        sL[1] = (LFR_sResLo + LFR_beta);

        sRSValue = as_float(__ocl_svml_spow_data.sRSValue);
        sHiMask = as_float(__ocl_svml_spow_data.sHiMask);
        {
            float V1;
            float V2;
            V1 = (sL[0] + sL[1]);
            V2 = (V1 * sRSValue);
            V1 = (V1 + V2);
            V2 = (V1 - V2);
            V1 = (sL[0] - V2);
            V1 = (sL[1] + V1);
            sL[0] = V2;
            sL[1] = V1;
        }

        {
            float V1;
            float V2;;
            V1 = (sY * sRSValue);
            V2 = (V1 - sY);
            V1 = (V1 - V2);
            V2 = (sY - V1);
            sW[0] = V1;
            sW[1] = V2;
        }
        sW[1] = (sW[1] + sYLo);

        {
            float V1;
            float V2;
            V1 = (sL[0] * sW[0]);
            V2 = (sL[1] * sW[1]);
            V2 = ((sL[0] * sW[1]) + V2);
            V2 = ((sL[1] * sW[0]) + V2);;
            sZ[0] = V1;
            sZ[1] = V2;
        }

        sInvLn2 = as_float(__ocl_svml_spow_data._sInvLn2);
        sShifter = as_float(__ocl_svml_spow_data._sShifter);
        sM = ((sZ[0] * sInvLn2) + sShifter);
        sN = (sM - sShifter);

        iAbsZ = as_uint(sZ[0]);
        iAbsZ = (iAbsZ & iAbsMask);
        iDomainRange =
        as_uint(__ocl_svml_spow_data._iDomainRange);
        iAbsZ =
        ((VUINT32)
        (-(VSINT32) ((VSINT32) iAbsZ > (VSINT32) iDomainRange)));
        iRangeMask = (iRangeMask | iAbsZ);
        vm = 0;
        vm |= (((VUINT32) iRangeMask >> 31) & 1);

        iM = as_uint(sM);
        iM = ((VUINT32) iM << 23);

        sLn2hi = as_float(__ocl_svml_spow_data._sLn2hi);
        float sR_0 = sN * sLn2hi;
        sR = (sZ[0] - sR_0);
        sLn2lo = as_float(__ocl_svml_spow_data._sLn2lo);
        sR = (sR - (sN * sLn2lo));
        sR = (sR + sZ[1]);

        sPC[4] = as_float(__ocl_svml_spow_data._sPC4);
        sPC[5] = as_float(__ocl_svml_spow_data._sPC5);
        sP = ((sPC[5] * sR) + sPC[4]);
        sPC[3] = as_float(__ocl_svml_spow_data._sPC3);
        sP = ((sP * sR) + sPC[3]);
        sPC[2] = as_float(__ocl_svml_spow_data._sPC2);
        sP = ((sP * sR) + sPC[2]);
        sPC[1] = as_float(__ocl_svml_spow_data._sPC1);
        sP = ((sP * sR) + sPC[1]);
        sPC[0] = as_float(__ocl_svml_spow_data._sPC0);
        sP = ((sP * sR) + sPC[0]);

        iP = as_uint(sP);
        iRes = (iM + iP);
        vr1 = as_float(iRes);

        vr1 = as_float(as_uint(vr1) | as_uint(sResultSign));

    }

    if ((vm & 0x00000001) != 0)
    {
        vr1 = __ocl_svml_spown_cout_rare (va1, va2);
    }
    r = vr1;

    return r;
}

/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF
typedef struct
{
    unsigned int TC_tbl[32];
    unsigned int Sgn_tbl[32];
    unsigned int _dAbsMask;
    unsigned int _dRShifter;
    unsigned int _lZero;
    unsigned int _dInf;
    unsigned int _dC1;
    unsigned int _dC2;
    unsigned int _dC3;
    unsigned int _dOne;
    unsigned int _dHShifter;
} __internal_stanpi_la_data_avx512_t;
static __constant __internal_stanpi_la_data_avx512_t __internal_stanpi_la_data_avx512 = {
    {

     0x00000000u, 0x3dc9b5dcu, 0x3e4bafafu, 0x3e9b5042u, 0x3ed413cdu, 0x3f08d5b9u, 0x3f2b0dc1u, 0x3f521801u,
     0xbf800000u, 0xbf521801u, 0xbf2b0dc1u, 0xbf08d5b9u, 0xbed413cdu, 0xbe9b5042u, 0xbe4bafafu, 0xbdc9b5dcu,
     0x80000000u, 0x3dc9b5dcu, 0x3e4bafafu, 0x3e9b5042u, 0x3ed413cdu, 0x3f08d5b9u, 0x3f2b0dc1u, 0x3f521801u,
     0xbf800000u, 0xbf521801u, 0xbf2b0dc1u, 0xbf08d5b9u, 0xbed413cdu, 0xbe9b5042u, 0xbe4bafafu, 0xbdc9b5dcu,
     },
    {

     0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u,
     0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u,
     0x80000000u, 0x80000000u, 0x80000000u, 0x80000000u,
     0x80000000u, 0x80000000u, 0x80000000u, 0x80000000u,
     0x80000000u, 0x80000000u, 0x80000000u, 0x80000000u,
     0x80000000u, 0x80000000u, 0x80000000u, 0x80000000u,
     0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u,
     0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u,
     },
    0x7FFFFFFFu,
    0x48c00000u,
    0x00000000u,
    0x7f800000u,
    0x40490fdbu,
    0x4125558bu,
    0x42242b8cu,
    0x3f800000u,
    0x4b000000u,
};

typedef struct
{
    unsigned int _sCoeffs[128][10];
    unsigned int _sAbsMask;
    unsigned int _sRangeReductionVal;
    unsigned int _sRangeVal;
    unsigned int _sRShifter;
    unsigned int _iIndexMask;
    unsigned int _sPIu;
    unsigned int _sPIoHi;
    unsigned int _sPIoLo1;
    unsigned int _sPIoLo2;
    unsigned int _sPIoTail;
    unsigned int _sS1;
    unsigned int _sS2;
    unsigned int _sC1;
    unsigned int _sC2;
    unsigned int _sTwo;
    unsigned int _sInvPI;
    unsigned int _sReductionRangeVal_new;
    unsigned int _sRShift;
    unsigned int _sPI1;
    unsigned int _sP0;
    unsigned int _sP1;
    unsigned int _sQ0;
    unsigned int _sQ1;

    unsigned int _sQ2;
} __internal_stanpi_la_data_t;
static __constant __internal_stanpi_la_data_t __internal_stanpi_la_data = {
    {

     {
      0x3FC90FDBu,
      0xB33BBD2Eu,
      0x00000000u,
      0x00000000u,
      0x00000000u,
      0x3F800000u,
      0x00000000u,
      0x00000000u,
      0x3EAAACDDu,
      0x00000000u},

     {
      0x3FC5EB9Bu,
      0x32DE638Cu,
      0x00000000u,
      0x3CC91A31u,
      0x2F8E8D1Au,
      0x3F800000u,
      0x3A1DFA00u,
      0x3CC9392Du,
      0x3EAB1889u,
      0x3C885D3Bu},

     {
      0x3FC2C75Cu,
      0xB2CBBE8Au,
      0x00000000u,
      0x3D49393Cu,
      0x30A39F5Bu,
      0x3F800000u,
      0x3B1E2B00u,
      0x3D49B5D4u,
      0x3EAC4F10u,
      0x3CFD9425u},

     {
      0x3FBFA31Cu,
      0x33450FB0u,
      0x00000000u,
      0x3D9711CEu,
      0x314FEB28u,
      0x3F800000u,
      0x3BB24C00u,
      0x3D97E43Au,
      0x3EAE6A89u,
      0x3D4D07E0u},

     {
      0x3FBC7EDDu,
      0xB1800ADDu,
      0x00000000u,
      0x3DC9B5DCu,
      0x3145AD86u,
      0x3F800000u,
      0x3C1EEF20u,
      0x3DCBAAEAu,
      0x3EB14E5Eu,
      0x3D858BB2u},

     {
      0x3FB95A9Eu,
      0xB3651267u,
      0x00000000u,
      0x3DFC98C2u,
      0xB0AE525Cu,
      0x3F800000u,
      0x3C793D20u,
      0x3E003845u,
      0x3EB5271Fu,
      0x3DAC669Eu},

     {
      0x3FB6365Eu,
      0x328BB91Cu,
      0x00000000u,
      0x3E17E564u,
      0xB1C5A2E4u,
      0x3F800000u,
      0x3CB440D0u,
      0x3E1B3D00u,
      0x3EB9F664u,
      0x3DD647C0u},

     {
      0x3FB3121Fu,
      0xB30F347Du,
      0x00000000u,
      0x3E31AE4Du,
      0xB1F32251u,
      0x3F800000u,
      0x3CF6A500u,
      0x3E3707DAu,
      0x3EBFA489u,
      0x3DFBD9C7u},

     {
      0x3FAFEDDFu,
      0x331BBA77u,
      0x00000000u,
      0x3E4BAFAFu,
      0x2F2A29E0u,
      0x3F800000u,
      0x3D221018u,
      0x3E53BED0u,
      0x3EC67E26u,
      0x3E1568E2u},

     {
      0x3FACC9A0u,
      0xB2655A50u,
      0x00000000u,
      0x3E65F267u,
      0x31B4B1DFu,
      0x3F800000u,
      0x3D4E8B90u,
      0x3E718ACAu,
      0x3ECE7164u,
      0x3E2DC161u},

     {
      0x3FA9A560u,
      0x33719861u,
      0x00000000u,
      0x3E803FD4u,
      0xB2279E66u,
      0x3F800000u,
      0x3D807FC8u,
      0x3E884BD4u,
      0x3ED7812Du,
      0x3E4636EBu},

     {
      0x3FA68121u,
      0x31E43AACu,
      0x00000000u,
      0x3E8DB082u,
      0xB132A234u,
      0x3F800000u,
      0x3D9CD7D0u,
      0x3E988A60u,
      0x3EE203E3u,
      0x3E63582Cu},

     {
      0x3FA35CE2u,
      0xB33889B6u,
      0x00000000u,
      0x3E9B5042u,
      0xB22A3AEEu,
      0x3F800000u,
      0x3DBC7490u,
      0x3EA99AF5u,
      0x3EEDE107u,
      0x3E80E9AAu},

     {
      0x3FA038A2u,
      0x32E4CA7Eu,
      0x00000000u,
      0x3EA92457u,
      0x30B80830u,
      0x3F800000u,
      0x3DDF8200u,
      0x3EBB99E9u,
      0x3EFB4AA8u,
      0x3E9182BEu},

     {
      0x3F9D1463u,
      0xB2C55799u,
      0x00000000u,
      0x3EB73250u,
      0xB2028823u,
      0x3F800000u,
      0x3E0318F8u,
      0x3ECEA678u,
      0x3F053C67u,
      0x3EA41E53u},

     {
      0x3F99F023u,
      0x33484328u,
      0x00000000u,
      0x3EC5800Du,
      0xB214C3C1u,
      0x3F800000u,
      0x3E185E54u,
      0x3EE2E342u,
      0x3F0DCA73u,
      0x3EB8CC21u},

     {
      0x3F96CBE4u,
      0xB14CDE2Eu,
      0x00000000u,
      0x3ED413CDu,
      0xB1C06152u,
      0x3F800000u,
      0x3E2FB0CCu,
      0x3EF876CBu,
      0x3F177807u,
      0x3ED08437u},

     {
      0x3F93A7A5u,
      0xB361DEEEu,
      0x00000000u,
      0x3EE2F439u,
      0xB1F4399Eu,
      0x3F800000u,
      0x3E49341Cu,
      0x3F07C61Au,
      0x3F22560Fu,
      0x3EEAA81Eu},

     {
      0x3F908365u,
      0x3292200Du,
      0x00000000u,
      0x3EF22870u,
      0x325271F4u,
      0x3F800000u,
      0x3E65107Au,
      0x3F1429F0u,
      0x3F2E8AFCu,
      0x3F040498u},

     {
      0x3F8D5F26u,
      0xB30C0105u,
      0x00000000u,
      0x3F00DC0Du,
      0xB214AF72u,
      0x3F800000u,
      0x3E81B994u,
      0x3F218233u,
      0x3F3C4531u,
      0x3F149688u},

     {
      0x3F8A3AE6u,
      0x331EEDF0u,
      0x00000000u,
      0x3F08D5B9u,
      0xB25EF98Eu,
      0x3F800000u,
      0x3E92478Du,
      0x3F2FEDC9u,
      0x3F4BCD58u,
      0x3F27AE9Eu},

     {
      0x3F8716A7u,
      0xB2588C6Du,
      0x00000000u,
      0x3F1105AFu,
      0x32F045B0u,
      0x3F800000u,
      0x3EA44EE2u,
      0x3F3F8FDBu,
      0x3F5D3FD0u,
      0x3F3D0A23u},

     {
      0x3F83F267u,
      0x3374CBD9u,
      0x00000000u,
      0x3F1970C4u,
      0x32904848u,
      0x3F800000u,
      0x3EB7EFF8u,
      0x3F50907Cu,
      0x3F710FEAu,
      0x3F561FEDu},

     {
      0x3F80CE28u,
      0x31FDD672u,
      0x00000000u,
      0x3F221C37u,
      0xB20C61DCu,
      0x3F800000u,
      0x3ECD4F71u,
      0x3F631DAAu,
      0x3F83B471u,
      0x3F7281EAu},

     {
      0x3F7B53D1u,
      0x32955386u,
      0x00000000u,
      0x3F2B0DC1u,
      0x32AB7EBAu,
      0x3F800000u,
      0x3EE496C2u,
      0x3F776C40u,
      0x3F9065C1u,
      0x3F89AFB6u},

     {
      0x3F750B52u,
      0x32EB316Fu,
      0x00000000u,
      0x3F344BA9u,
      0xB2B8B0EAu,
      0x3F800000u,
      0x3EFDF4F7u,
      0x3F86DCA8u,
      0x3F9ED53Bu,
      0x3F9CBEDEu},

     {
      0x3F6EC2D4u,
      0xB2BEF0A7u,
      0x00000000u,
      0x3F3DDCCFu,
      0x32D29606u,
      0x40000000u,
      0xBEE6606Fu,
      0x3F9325D6u,
      0x3FAF4E69u,
      0x3FB3080Cu},

     {
      0x3F687A55u,
      0xB252257Bu,
      0x00000000u,
      0x3F47C8CCu,
      0xB200F51Au,
      0x40000000u,
      0xBEC82C6Cu,
      0x3FA0BAE9u,
      0x3FC2252Fu,
      0x3FCD24C7u},

     {
      0x3F6231D6u,
      0xB119A6A2u,
      0x00000000u,
      0x3F521801u,
      0x32AE4178u,
      0x40000000u,
      0xBEA72938u,
      0x3FAFCC22u,
      0x3FD7BD4Au,
      0x3FEBB01Bu},

     {
      0x3F5BE957u,
      0x3205522Au,
      0x00000000u,
      0x3F5CD3BEu,
      0x31460308u,
      0x40000000u,
      0xBE8306C5u,
      0x3FC09232u,
      0x3FF09632u,
      0x4007DB00u},

     {
      0x3F55A0D8u,
      0x329886FFu,
      0x00000000u,
      0x3F68065Eu,
      0x32670D1Au,
      0x40000000u,
      0xBE36D1D6u,
      0x3FD35007u,
      0x4006A861u,
      0x401D4BDAu},

     {
      0x3F4F5859u,
      0x32EE64E8u,
      0x00000000u,
      0x3F73BB75u,
      0x32FC908Du,
      0x40000000u,
      0xBDBF94B0u,
      0x3FE8550Fu,
      0x40174F67u,
      0x4036C608u},

     {
      0x3F490FDBu,
      0xB2BBBD2Eu,
      0x3F800000u,
      0xBE8BE60Eu,
      0x320D8D84u,
      0x3F000000u,
      0xBDF817B1u,
      0xBD8345EBu,
      0x3D1DFDACu,
      0xBC52CF6Fu},

     {
      0x3F42C75Cu,
      0xB24BBE8Au,
      0x3F800000u,
      0xBE87283Fu,
      0xB268B966u,
      0x3F000000u,
      0xBDFE6529u,
      0xBD7B1953u,
      0x3D18E109u,
      0xBC4570B0u},

     {
      0x3F3C7EDDu,
      0xB1000ADDu,
      0x3F800000u,
      0xBE827420u,
      0x320B8B4Du,
      0x3E800000u,
      0x3DFB9428u,
      0xBD7002B4u,
      0x3D142A6Cu,
      0xBC3A47FFu},

     {
      0x3F36365Eu,
      0x320BB91Cu,
      0x3F800000u,
      0xBE7B9282u,
      0xB13383D2u,
      0x3E800000u,
      0x3DF5D211u,
      0xBD6542B3u,
      0x3D0FE5E5u,
      0xBC31FB14u},

     {
      0x3F2FEDDFu,
      0x329BBA77u,
      0x3F800000u,
      0xBE724E73u,
      0x3120C3E2u,
      0x3E800000u,
      0x3DF05283u,
      0xBD5AD45Eu,
      0x3D0BAFBFu,
      0xBC27B8BBu},

     {
      0x3F29A560u,
      0x32F19861u,
      0x3F800000u,
      0xBE691B44u,
      0x31F18936u,
      0x3E800000u,
      0x3DEB138Bu,
      0xBD50B2F7u,
      0x3D07BE3Au,
      0xBC1E46A7u},

     {
      0x3F235CE2u,
      0xB2B889B6u,
      0x3F800000u,
      0xBE5FF82Cu,
      0xB170723Au,
      0x3E800000u,
      0x3DE61354u,
      0xBD46DA06u,
      0x3D0401F8u,
      0xBC14E013u},

     {
      0x3F1D1463u,
      0xB2455799u,
      0x3F800000u,
      0xBE56E46Bu,
      0x31E3F001u,
      0x3E800000u,
      0x3DE15025u,
      0xBD3D4550u,
      0x3D00462Du,
      0xBC092C98u},

     {
      0x3F16CBE4u,
      0xB0CCDE2Eu,
      0x3F800000u,
      0xBE4DDF41u,
      0xB1AEA094u,
      0x3E800000u,
      0x3DDCC85Cu,
      0xBD33F0BEu,
      0x3CFA23B0u,
      0xBC01FCF7u},

     {
      0x3F108365u,
      0x3212200Du,
      0x3F800000u,
      0xBE44E7F8u,
      0xB1CAA3CBu,
      0x3E800000u,
      0x3DD87A74u,
      0xBD2AD885u,
      0x3CF3C785u,
      0xBBF1E348u},

     {
      0x3F0A3AE6u,
      0x329EEDF0u,
      0x3F800000u,
      0xBE3BFDDCu,
      0xB132521Au,
      0x3E800000u,
      0x3DD464FCu,
      0xBD21F8F1u,
      0x3CEE3076u,
      0xBBE6D263u},

     {
      0x3F03F267u,
      0x32F4CBD9u,
      0x3F800000u,
      0xBE33203Eu,
      0x31FEF5BEu,
      0x3E800000u,
      0x3DD0869Cu,
      0xBD194E8Cu,
      0x3CE8DCA9u,
      0xBBDADA55u},

     {
      0x3EFB53D1u,
      0x32155386u,
      0x3F800000u,
      0xBE2A4E71u,
      0xB19CFCECu,
      0x3E800000u,
      0x3DCCDE11u,
      0xBD10D605u,
      0x3CE382A7u,
      0xBBC8BD97u},

     {
      0x3EEEC2D4u,
      0xB23EF0A7u,
      0x3F800000u,
      0xBE2187D0u,
      0xB1B7C7F7u,
      0x3E800000u,
      0x3DC96A2Bu,
      0xBD088C22u,
      0x3CDE950Eu,
      0xBBB89AD1u},

     {
      0x3EE231D6u,
      0xB099A6A2u,
      0x3F800000u,
      0xBE18CBB7u,
      0xAFE28430u,
      0x3E800000u,
      0x3DC629CEu,
      0xBD006DCDu,
      0x3CDA5A2Cu,
      0xBBB0B3D2u},

     {
      0x3ED5A0D8u,
      0x321886FFu,
      0x3F800000u,
      0xBE101985u,
      0xB02FB2B8u,
      0x3E800000u,
      0x3DC31BF3u,
      0xBCF0F04Du,
      0x3CD60BC7u,
      0xBBA138BAu},

     {
      0x3EC90FDBu,
      0xB23BBD2Eu,
      0x3F800000u,
      0xBE07709Du,
      0xB18A2A83u,
      0x3E800000u,
      0x3DC03FA2u,
      0xBCE15096u,
      0x3CD26472u,
      0xBB9A1270u},

     {
      0x3EBC7EDDu,
      0xB0800ADDu,
      0x3F800000u,
      0xBDFDA0CBu,
      0x2F14FCA0u,
      0x3E800000u,
      0x3DBD93F7u,
      0xBCD1F71Bu,
      0x3CCEDD2Bu,
      0xBB905946u},

     {
      0x3EAFEDDFu,
      0x321BBA77u,
      0x3F800000u,
      0xBDEC708Cu,
      0xB14895C4u,
      0x3E800000u,
      0x3DBB181Eu,
      0xBCC2DEA6u,
      0x3CCB5027u,
      0xBB7F3969u},

     {
      0x3EA35CE2u,
      0xB23889B6u,
      0x3F800000u,
      0xBDDB4F55u,
      0x30F6437Eu,
      0x3E800000u,
      0x3DB8CB52u,
      0xBCB40210u,
      0x3CC82D45u,
      0xBB643075u},

     {
      0x3E96CBE4u,
      0xB04CDE2Eu,
      0x3F800000u,
      0xBDCA3BFFu,
      0x311C95EAu,
      0x3E800000u,
      0x3DB6ACDEu,
      0xBCA55C5Bu,
      0x3CC5BC04u,
      0xBB63A969u},

     {
      0x3E8A3AE6u,
      0x321EEDF0u,
      0x3F800000u,
      0xBDB93569u,
      0xAFB9ED00u,
      0x3E800000u,
      0x3DB4BC1Fu,
      0xBC96E905u,
      0x3CC2E6F5u,
      0xBB3E10A6u},

     {
      0x3E7B53D1u,
      0x31955386u,
      0x3F800000u,
      0xBDA83A77u,
      0x316D967Au,
      0x3E800000u,
      0x3DB2F87Cu,
      0xBC88A31Fu,
      0x3CC0E763u,
      0xBB3F1666u},

     {
      0x3E6231D6u,
      0xB019A6A2u,
      0x3F800000u,
      0xBD974A0Du,
      0xB14F365Bu,
      0x3E800000u,
      0x3DB1616Fu,
      0xBC750CD8u,
      0x3CBEB595u,
      0xBB22B883u},

     {
      0x3E490FDBu,
      0xB1BBBD2Eu,
      0x3F800000u,
      0xBD866317u,
      0xAFF02140u,
      0x3E800000u,
      0x3DAFF67Du,
      0xBC591CD0u,
      0x3CBCBEADu,
      0xBB04BBECu},

     {
      0x3E2FEDDFu,
      0x319BBA77u,
      0x3F800000u,
      0xBD6B08FFu,
      0xB0EED236u,
      0x3E800000u,
      0x3DAEB739u,
      0xBC3D6D51u,
      0x3CBB485Du,
      0xBAFFF5BAu},

     {
      0x3E16CBE4u,
      0xAFCCDE2Eu,
      0x3F800000u,
      0xBD495A6Cu,
      0xB0A427BDu,
      0x3E800000u,
      0x3DADA345u,
      0xBC21F648u,
      0x3CB9D1B4u,
      0xBACB5567u},

     {
      0x3DFB53D1u,
      0x31155386u,
      0x3F800000u,
      0xBD27B856u,
      0xB0F7EE91u,
      0x3E800000u,
      0x3DACBA4Eu,
      0xBC06AEE3u,
      0x3CB8E5DCu,
      0xBAEC00EEu},

     {
      0x3DC90FDBu,
      0xB13BBD2Eu,
      0x3F800000u,
      0xBD0620A3u,
      0xB0ECAB40u,
      0x3E800000u,
      0x3DABFC11u,
      0xBBD7200Fu,
      0x3CB79475u,
      0xBA2B0ADCu},

     {
      0x3D96CBE4u,
      0xAF4CDE2Eu,
      0x3F800000u,
      0xBCC92278u,
      0x302F2E68u,
      0x3E800000u,
      0x3DAB6854u,
      0xBBA1214Fu,
      0x3CB6C1E9u,
      0x3843C2F3u},

     {
      0x3D490FDBu,
      0xB0BBBD2Eu,
      0x3F800000u,
      0xBC861015u,
      0xAFD68E2Eu,
      0x3E800000u,
      0x3DAAFEEBu,
      0xBB569F3Fu,
      0x3CB6A84Eu,
      0xBAC64194u},

     {
      0x3CC90FDBu,
      0xB03BBD2Eu,
      0x3F800000u,
      0xBC060BF3u,
      0x2FE251AEu,
      0x3E800000u,
      0x3DAABFB9u,
      0xBAD67C60u,
      0x3CB64CA5u,
      0xBACDE881u},

     {
      0x00000000u,
      0x00000000u,
      0x3F800000u,
      0x00000000u,
      0x00000000u,
      0x3E800000u,
      0x3DAAAAABu,
      0x00000000u,
      0x3CB5E28Bu,
      0x00000000u},

     {
      0xBCC90FDBu,
      0x303BBD2Eu,
      0x3F800000u,
      0x3C060BF3u,
      0xAFE251AEu,
      0x3E800000u,
      0x3DAABFB9u,
      0x3AD67C60u,
      0x3CB64CA5u,
      0x3ACDE881u},

     {
      0xBD490FDBu,
      0x30BBBD2Eu,
      0x3F800000u,
      0x3C861015u,
      0x2FD68E2Eu,
      0x3E800000u,
      0x3DAAFEEBu,
      0x3B569F3Fu,
      0x3CB6A84Eu,
      0x3AC64194u},

     {
      0xBD96CBE4u,
      0x2F4CDE2Eu,
      0x3F800000u,
      0x3CC92278u,
      0xB02F2E68u,
      0x3E800000u,
      0x3DAB6854u,
      0x3BA1214Fu,
      0x3CB6C1E9u,
      0xB843C2F2u},

     {
      0xBDC90FDBu,
      0x313BBD2Eu,
      0x3F800000u,
      0x3D0620A3u,
      0x30ECAB40u,
      0x3E800000u,
      0x3DABFC11u,
      0x3BD7200Fu,
      0x3CB79475u,
      0x3A2B0ADCu},

     {
      0xBDFB53D1u,
      0xB1155386u,
      0x3F800000u,
      0x3D27B856u,
      0x30F7EE91u,
      0x3E800000u,
      0x3DACBA4Eu,
      0x3C06AEE3u,
      0x3CB8E5DCu,
      0x3AEC00EEu},

     {
      0xBE16CBE4u,
      0x2FCCDE2Eu,
      0x3F800000u,
      0x3D495A6Cu,
      0x30A427BDu,
      0x3E800000u,
      0x3DADA345u,
      0x3C21F648u,
      0x3CB9D1B4u,
      0x3ACB5567u},

     {
      0xBE2FEDDFu,
      0xB19BBA77u,
      0x3F800000u,
      0x3D6B08FFu,
      0x30EED236u,
      0x3E800000u,
      0x3DAEB739u,
      0x3C3D6D51u,
      0x3CBB485Du,
      0x3AFFF5BAu},

     {
      0xBE490FDBu,
      0x31BBBD2Eu,
      0x3F800000u,
      0x3D866317u,
      0x2FF02140u,
      0x3E800000u,
      0x3DAFF67Du,
      0x3C591CD0u,
      0x3CBCBEADu,
      0x3B04BBECu},

     {
      0xBE6231D6u,
      0x3019A6A2u,
      0x3F800000u,
      0x3D974A0Du,
      0x314F365Bu,
      0x3E800000u,
      0x3DB1616Fu,
      0x3C750CD8u,
      0x3CBEB595u,
      0x3B22B883u},

     {
      0xBE7B53D1u,
      0xB1955386u,
      0x3F800000u,
      0x3DA83A77u,
      0xB16D967Au,
      0x3E800000u,
      0x3DB2F87Cu,
      0x3C88A31Fu,
      0x3CC0E763u,
      0x3B3F1666u},

     {
      0xBE8A3AE6u,
      0xB21EEDF0u,
      0x3F800000u,
      0x3DB93569u,
      0x2FB9ED00u,
      0x3E800000u,
      0x3DB4BC1Fu,
      0x3C96E905u,
      0x3CC2E6F5u,
      0x3B3E10A6u},

     {
      0xBE96CBE4u,
      0x304CDE2Eu,
      0x3F800000u,
      0x3DCA3BFFu,
      0xB11C95EAu,
      0x3E800000u,
      0x3DB6ACDEu,
      0x3CA55C5Bu,
      0x3CC5BC04u,
      0x3B63A969u},

     {
      0xBEA35CE2u,
      0x323889B6u,
      0x3F800000u,
      0x3DDB4F55u,
      0xB0F6437Eu,
      0x3E800000u,
      0x3DB8CB52u,
      0x3CB40210u,
      0x3CC82D45u,
      0x3B643075u},

     {
      0xBEAFEDDFu,
      0xB21BBA77u,
      0x3F800000u,
      0x3DEC708Cu,
      0x314895C4u,
      0x3E800000u,
      0x3DBB181Eu,
      0x3CC2DEA6u,
      0x3CCB5027u,
      0x3B7F3969u},

     {
      0xBEBC7EDDu,
      0x30800ADDu,
      0x3F800000u,
      0x3DFDA0CBu,
      0xAF14FCA0u,
      0x3E800000u,
      0x3DBD93F7u,
      0x3CD1F71Bu,
      0x3CCEDD2Bu,
      0x3B905946u},

     {
      0xBEC90FDBu,
      0x323BBD2Eu,
      0x3F800000u,
      0x3E07709Du,
      0x318A2A83u,
      0x3E800000u,
      0x3DC03FA2u,
      0x3CE15096u,
      0x3CD26472u,
      0x3B9A1270u},

     {
      0xBED5A0D8u,
      0xB21886FFu,
      0x3F800000u,
      0x3E101985u,
      0x302FB2B8u,
      0x3E800000u,
      0x3DC31BF3u,
      0x3CF0F04Du,
      0x3CD60BC7u,
      0x3BA138BAu},

     {
      0xBEE231D6u,
      0x3099A6A2u,
      0x3F800000u,
      0x3E18CBB7u,
      0x2FE28430u,
      0x3E800000u,
      0x3DC629CEu,
      0x3D006DCDu,
      0x3CDA5A2Cu,
      0x3BB0B3D2u},

     {
      0xBEEEC2D4u,
      0x323EF0A7u,
      0x3F800000u,
      0x3E2187D0u,
      0x31B7C7F7u,
      0x3E800000u,
      0x3DC96A2Bu,
      0x3D088C22u,
      0x3CDE950Eu,
      0x3BB89AD1u},

     {
      0xBEFB53D1u,
      0xB2155386u,
      0x3F800000u,
      0x3E2A4E71u,
      0x319CFCECu,
      0x3E800000u,
      0x3DCCDE11u,
      0x3D10D605u,
      0x3CE382A7u,
      0x3BC8BD97u},

     {
      0xBF03F267u,
      0xB2F4CBD9u,
      0x3F800000u,
      0x3E33203Eu,
      0xB1FEF5BEu,
      0x3E800000u,
      0x3DD0869Cu,
      0x3D194E8Cu,
      0x3CE8DCA9u,
      0x3BDADA55u},

     {
      0xBF0A3AE6u,
      0xB29EEDF0u,
      0x3F800000u,
      0x3E3BFDDCu,
      0x3132521Au,
      0x3E800000u,
      0x3DD464FCu,
      0x3D21F8F1u,
      0x3CEE3076u,
      0x3BE6D263u},

     {
      0xBF108365u,
      0xB212200Du,
      0x3F800000u,
      0x3E44E7F8u,
      0x31CAA3CBu,
      0x3E800000u,
      0x3DD87A74u,
      0x3D2AD885u,
      0x3CF3C785u,
      0x3BF1E348u},

     {
      0xBF16CBE4u,
      0x30CCDE2Eu,
      0x3F800000u,
      0x3E4DDF41u,
      0x31AEA094u,
      0x3E800000u,
      0x3DDCC85Cu,
      0x3D33F0BEu,
      0x3CFA23B0u,
      0x3C01FCF7u},

     {
      0xBF1D1463u,
      0x32455799u,
      0x3F800000u,
      0x3E56E46Bu,
      0xB1E3F001u,
      0x3E800000u,
      0x3DE15025u,
      0x3D3D4550u,
      0x3D00462Du,
      0x3C092C98u},

     {
      0xBF235CE2u,
      0x32B889B6u,
      0x3F800000u,
      0x3E5FF82Cu,
      0x3170723Au,
      0x3E800000u,
      0x3DE61354u,
      0x3D46DA06u,
      0x3D0401F8u,
      0x3C14E013u},

     {
      0xBF29A560u,
      0xB2F19861u,
      0x3F800000u,
      0x3E691B44u,
      0xB1F18936u,
      0x3E800000u,
      0x3DEB138Bu,
      0x3D50B2F7u,
      0x3D07BE3Au,
      0x3C1E46A7u},

     {
      0xBF2FEDDFu,
      0xB29BBA77u,
      0x3F800000u,
      0x3E724E73u,
      0xB120C3E2u,
      0x3E800000u,
      0x3DF05283u,
      0x3D5AD45Eu,
      0x3D0BAFBFu,
      0x3C27B8BBu},

     {
      0xBF36365Eu,
      0xB20BB91Cu,
      0x3F800000u,
      0x3E7B9282u,
      0x313383D2u,
      0x3E800000u,
      0x3DF5D211u,
      0x3D6542B3u,
      0x3D0FE5E5u,
      0x3C31FB14u},

     {
      0xBF3C7EDDu,
      0x31000ADDu,
      0x3F800000u,
      0x3E827420u,
      0xB20B8B4Du,
      0x3E800000u,
      0x3DFB9428u,
      0x3D7002B4u,
      0x3D142A6Cu,
      0x3C3A47FFu},

     {
      0xBF42C75Cu,
      0x324BBE8Au,
      0x3F800000u,
      0x3E87283Fu,
      0x3268B966u,
      0x3F000000u,
      0xBDFE6529u,
      0x3D7B1953u,
      0x3D18E109u,
      0x3C4570B0u},

     {
      0xBF490FDBu,
      0x32BBBD2Eu,
      0x00000000u,
      0xBF800000u,
      0x2B410000u,
      0x40000000u,
      0xB3000000u,
      0xC0000000u,
      0x402AB7C8u,
      0xC05561DBu},

     {
      0xBF4F5859u,
      0xB2EE64E8u,
      0x00000000u,
      0xBF73BB75u,
      0xB2FC908Du,
      0x40000000u,
      0xBDBF94B0u,
      0xBFE8550Fu,
      0x40174F67u,
      0xC036C608u},

     {
      0xBF55A0D8u,
      0xB29886FFu,
      0x00000000u,
      0xBF68065Eu,
      0xB2670D1Au,
      0x40000000u,
      0xBE36D1D6u,
      0xBFD35007u,
      0x4006A861u,
      0xC01D4BDAu},

     {
      0xBF5BE957u,
      0xB205522Au,
      0x00000000u,
      0xBF5CD3BEu,
      0xB1460308u,
      0x40000000u,
      0xBE8306C5u,
      0xBFC09232u,
      0x3FF09632u,
      0xC007DB00u},

     {
      0xBF6231D6u,
      0x3119A6A2u,
      0x00000000u,
      0xBF521801u,
      0xB2AE4178u,
      0x40000000u,
      0xBEA72938u,
      0xBFAFCC22u,
      0x3FD7BD4Au,
      0xBFEBB01Bu},

     {
      0xBF687A55u,
      0x3252257Bu,
      0x00000000u,
      0xBF47C8CCu,
      0x3200F51Au,
      0x40000000u,
      0xBEC82C6Cu,
      0xBFA0BAE9u,
      0x3FC2252Fu,
      0xBFCD24C7u},

     {
      0xBF6EC2D4u,
      0x32BEF0A7u,
      0x00000000u,
      0xBF3DDCCFu,
      0xB2D29606u,
      0x40000000u,
      0xBEE6606Fu,
      0xBF9325D6u,
      0x3FAF4E69u,
      0xBFB3080Cu},

     {
      0xBF750B52u,
      0xB2EB316Fu,
      0x00000000u,
      0xBF344BA9u,
      0x32B8B0EAu,
      0x3F800000u,
      0x3EFDF4F7u,
      0xBF86DCA8u,
      0x3F9ED53Bu,
      0xBF9CBEDEu},

     {
      0xBF7B53D1u,
      0xB2955386u,
      0x00000000u,
      0xBF2B0DC1u,
      0xB2AB7EBAu,
      0x3F800000u,
      0x3EE496C2u,
      0xBF776C40u,
      0x3F9065C1u,
      0xBF89AFB6u},

     {
      0xBF80CE28u,
      0xB1FDD672u,
      0x00000000u,
      0xBF221C37u,
      0x320C61DCu,
      0x3F800000u,
      0x3ECD4F71u,
      0xBF631DAAu,
      0x3F83B471u,
      0xBF7281EAu},

     {
      0xBF83F267u,
      0xB374CBD9u,
      0x00000000u,
      0xBF1970C4u,
      0xB2904848u,
      0x3F800000u,
      0x3EB7EFF8u,
      0xBF50907Cu,
      0x3F710FEAu,
      0xBF561FEDu},

     {
      0xBF8716A7u,
      0x32588C6Du,
      0x00000000u,
      0xBF1105AFu,
      0xB2F045B0u,
      0x3F800000u,
      0x3EA44EE2u,
      0xBF3F8FDBu,
      0x3F5D3FD0u,
      0xBF3D0A23u},

     {
      0xBF8A3AE6u,
      0xB31EEDF0u,
      0x00000000u,
      0xBF08D5B9u,
      0x325EF98Eu,
      0x3F800000u,
      0x3E92478Du,
      0xBF2FEDC9u,
      0x3F4BCD58u,
      0xBF27AE9Eu},

     {
      0xBF8D5F26u,
      0x330C0105u,
      0x00000000u,
      0xBF00DC0Du,
      0x3214AF72u,
      0x3F800000u,
      0x3E81B994u,
      0xBF218233u,
      0x3F3C4531u,
      0xBF149688u},

     {
      0xBF908365u,
      0xB292200Du,
      0x00000000u,
      0xBEF22870u,
      0xB25271F4u,
      0x3F800000u,
      0x3E65107Au,
      0xBF1429F0u,
      0x3F2E8AFCu,
      0xBF040498u},

     {
      0xBF93A7A5u,
      0x3361DEEEu,
      0x00000000u,
      0xBEE2F439u,
      0x31F4399Eu,
      0x3F800000u,
      0x3E49341Cu,
      0xBF07C61Au,
      0x3F22560Fu,
      0xBEEAA81Eu},

     {
      0xBF96CBE4u,
      0x314CDE2Eu,
      0x00000000u,
      0xBED413CDu,
      0x31C06152u,
      0x3F800000u,
      0x3E2FB0CCu,
      0xBEF876CBu,
      0x3F177807u,
      0xBED08437u},

     {
      0xBF99F023u,
      0xB3484328u,
      0x00000000u,
      0xBEC5800Du,
      0x3214C3C1u,
      0x3F800000u,
      0x3E185E54u,
      0xBEE2E342u,
      0x3F0DCA73u,
      0xBEB8CC21u},

     {
      0xBF9D1463u,
      0x32C55799u,
      0x00000000u,
      0xBEB73250u,
      0x32028823u,
      0x3F800000u,
      0x3E0318F8u,
      0xBECEA678u,
      0x3F053C67u,
      0xBEA41E53u},

     {
      0xBFA038A2u,
      0xB2E4CA7Eu,
      0x00000000u,
      0xBEA92457u,
      0xB0B80830u,
      0x3F800000u,
      0x3DDF8200u,
      0xBEBB99E9u,
      0x3EFB4AA8u,
      0xBE9182BEu},

     {
      0xBFA35CE2u,
      0x333889B6u,
      0x00000000u,
      0xBE9B5042u,
      0x322A3AEEu,
      0x3F800000u,
      0x3DBC7490u,
      0xBEA99AF5u,
      0x3EEDE107u,
      0xBE80E9AAu},

     {
      0xBFA68121u,
      0xB1E43AACu,
      0x00000000u,
      0xBE8DB082u,
      0x3132A234u,
      0x3F800000u,
      0x3D9CD7D0u,
      0xBE988A60u,
      0x3EE203E3u,
      0xBE63582Cu},

     {
      0xBFA9A560u,
      0xB3719861u,
      0x00000000u,
      0xBE803FD4u,
      0x32279E66u,
      0x3F800000u,
      0x3D807FC8u,
      0xBE884BD4u,
      0x3ED7812Du,
      0xBE4636EBu},

     {
      0xBFACC9A0u,
      0x32655A50u,
      0x00000000u,
      0xBE65F267u,
      0xB1B4B1DFu,
      0x3F800000u,
      0x3D4E8B90u,
      0xBE718ACAu,
      0x3ECE7164u,
      0xBE2DC161u},

     {
      0xBFAFEDDFu,
      0xB31BBA77u,
      0x00000000u,
      0xBE4BAFAFu,
      0xAF2A29E0u,
      0x3F800000u,
      0x3D221018u,
      0xBE53BED0u,
      0x3EC67E26u,
      0xBE1568E2u},

     {
      0xBFB3121Fu,
      0x330F347Du,
      0x00000000u,
      0xBE31AE4Du,
      0x31F32251u,
      0x3F800000u,
      0x3CF6A500u,
      0xBE3707DAu,
      0x3EBFA489u,
      0xBDFBD9C7u},

     {
      0xBFB6365Eu,
      0xB28BB91Cu,
      0x00000000u,
      0xBE17E564u,
      0x31C5A2E4u,
      0x3F800000u,
      0x3CB440D0u,
      0xBE1B3D00u,
      0x3EB9F664u,
      0xBDD647C0u},

     {
      0xBFB95A9Eu,
      0x33651267u,
      0x00000000u,
      0xBDFC98C2u,
      0x30AE525Cu,
      0x3F800000u,
      0x3C793D20u,
      0xBE003845u,
      0x3EB5271Fu,
      0xBDAC669Eu},

     {
      0xBFBC7EDDu,
      0x31800ADDu,
      0x00000000u,
      0xBDC9B5DCu,
      0xB145AD86u,
      0x3F800000u,
      0x3C1EEF20u,
      0xBDCBAAEAu,
      0x3EB14E5Eu,
      0xBD858BB2u},

     {
      0xBFBFA31Cu,
      0xB3450FB0u,
      0x00000000u,
      0xBD9711CEu,
      0xB14FEB28u,
      0x3F800000u,
      0x3BB24C00u,
      0xBD97E43Au,
      0x3EAE6A89u,
      0xBD4D07E0u},

     {
      0xBFC2C75Cu,
      0x32CBBE8Au,
      0x00000000u,
      0xBD49393Cu,
      0xB0A39F5Bu,
      0x3F800000u,
      0x3B1E2B00u,
      0xBD49B5D4u,
      0x3EAC4F10u,
      0xBCFD9425u},

     {
      0xBFC5EB9Bu,
      0xB2DE638Cu,
      0x00000000u,
      0xBCC91A31u,
      0xAF8E8D1Au,
      0x3F800000u,
      0x3A1DFA00u,
      0xBCC9392Du,
      0x3EAB1889u,
      0xBC885D3Bu},
     },
    0x7FFFFFFFu,
    0x461C4000u,
    0x7f800000u,
    0x4B400000u,
    0x000000ffu,
    0x4222F983u,
    0xBCC90FDBu,
    0x303BC000u,
    0xA9346000u,
    0xA18D3132u,
    0xBE2AAAABu,
    0x3C08885Cu,
    0xBF000000u,
    0x3D2AAA7Cu,
    0x40000000u,
    0x3EA2F983u,
    0x49800000u,
    0x4AC00000u,
    0x40490fdbu,

    0x40490fd7u,
    0xc03e1bf0u,
    0x3F7FFFFCu,
    0xc0878864u,
    0x3f724e71u,
};

static __constant _iml_v2_sp_union_t __stanpi_la_Tab[5] = {
    0x00000000,
    0x7F800000,
    0xFF800000,
    0x40490FDB,
    0x38800000
};

#pragma float_control(push)
#pragma float_control(precise, on)
__attribute__((always_inline))
inline int __internal_stanpi_la_cout (float *a, float *r)
{
    int nRet = 0;
    int N, iAbsExp, i, NR;
    float absx, inf;

    absx = (*a);
    (((_iml_v2_sp_union_t *) & absx)->hex[0] = (((_iml_v2_sp_union_t *) & absx)->hex[0] & 0x7FFFFFFF) | ((_iml_uint32_t) (0) << 31));

    if ((((((_iml_v2_sp_union_t *) & *a)->hex[0] >> 23) & 0xFF) != 0xFF))
    {

        if (absx < ((__constant float *) __stanpi_la_Tab)[4])
        {

            (*r) = (*a) * ((__constant float *) __stanpi_la_Tab)[3];
        }
    }
    else
    {

        if (((_iml_v2_sp_union_t *) & (absx))->hex[0] == ((__constant _iml_v2_sp_union_t *) & (((__constant float *) __stanpi_la_Tab)[1 + (0)]))->hex[0])
        {

            (*r) =
                (((__constant float *) __stanpi_la_Tab)[1 +
                                                        ((((_iml_v2_sp_union_t *) & *a)->hex[0] >> 31))] * ((__constant float *) __stanpi_la_Tab)[0]);

            nRet = 1;
        }
        else
        {

            (*r) = (float) ((*a) + (*a));
        }
    }
    return nRet;
}

#pragma float_control (pop)
float __ocl_svml_tanpif (float a)
{

    float va1;
    float vr1;
    unsigned int vm;

    float r;

    va1 = a;;

    {

        float sAbsMask;
        float sAbsX;
        float sX;
        float sRangeReductionVal;

        float sRangeReductionMask;
        float sRangeMask;
        unsigned int iRangeReductionMask;
        unsigned int mRangeReductionMask;
        float sRangeVal;
        float sN;
        unsigned int iIndex;
        float sZ;
        float sE;

        float sY;
        float sPI1;
        float sPI2;
        float sRl;
        float sR;
        float sR2;
        float sTh;
        float sTl;
        float sPC[2];
        float sPoly;
        float sP;
        float sPlow;
        float sThP;
        float sThPlow;
        float sTmp;
        float sNumerator;
        float sNlow;
        float sOne;
        float sDenominator;
        float sDlow;
        float sRcp;
        float sAE;
        float sQuotient;

        float sZero;
        float sSignX;
        float sRShift;
        float sSignRes;
        float sInvMask;
        float sP1;
        float sP0;
        float sQ2;
        float sQ1;
        float sQ;
        float sQ0;
        float sNumP;
        float sNumQ;
        float sNum;
        float sDenP;
        float sDenQ;
        float sDen;
        float sRes;
        float sSignBit;
        float sInvBit;
        float sExp;
        unsigned int iRangeMask;
        float sSpecRes;
        float sSpecMask;
        float sModifier;

        vm = 0;
        sAbsMask = as_float (__internal_stanpi_la_data._sAbsMask);
        sAbsX = as_float ((as_uint (va1) & as_uint (sAbsMask)));
        sSignX = as_float ((as_uint (sAbsX) ^ as_uint (va1)));

        sRangeReductionVal = as_float (__internal_stanpi_la_data._sReductionRangeVal_new);
        sRangeReductionMask = as_float (((unsigned int) (-(signed int) (!(sAbsX <= sRangeReductionVal)))));
        iRangeReductionMask = as_uint (sRangeReductionMask);

        mRangeReductionMask = 0;
        mRangeReductionMask = iRangeReductionMask;
        if ((mRangeReductionMask) != 0)
        {

            sRangeVal = as_float (__internal_stanpi_la_data._sRangeVal);
            sExp = as_float ((as_uint (sRangeVal) & as_uint (sAbsX)));
            sRangeMask = as_float (((unsigned int) (-(signed int) (sExp == sRangeVal))));
            iRangeMask = as_uint (sRangeMask);
            vm = 0;
            vm = iRangeMask;

            {

                float sX;
                float sShifterThreshold;
                float sShifterMask;
                float sShifterPos;
                float sShifter;
                float sShiftedN;
                float sN;
                float sZero;
                sX = sAbsX;
                sShifterThreshold = as_float (0x4F000000u);
                sShifterMask = as_float (((unsigned int) (-(signed int) (sX < sShifterThreshold))));

                sShifterPos = as_float (0x4FC00000u);
                sZero = as_float (0x00000000u);
                sShifter = as_float ((((~as_uint (sShifterMask)) & as_uint (sZero)) | (as_uint (sShifterMask) & as_uint (sShifterPos))));

                sShiftedN = (sShifter + sAbsX);
                sN = (sShiftedN - sShifter);
                sZ = (sAbsX - sN);

            }

            sAbsX = as_float ((((~as_uint (sRangeReductionMask)) & as_uint (sAbsX)) | (as_uint (sRangeReductionMask) & as_uint (sZ))));
        }

        sRShift = as_float (__internal_stanpi_la_data._sRShift);
        sN = (sAbsX + sRShift);
        sY = (sN - sRShift);
        sR = (sAbsX - sY);

        sR2 = (sR * sR);

        sSignBit = as_float (((unsigned int) as_uint (sN) << (31)));

        sInvBit = as_float (((unsigned int) as_uint (sN) << (30)));
        sZero = as_float (0);
        sInvMask = as_float (((unsigned int) (-(signed int) (!(sInvBit == sZero)))));

        sP1 = as_float (__internal_stanpi_la_data._sP1);
        sP0 = as_float (__internal_stanpi_la_data._sP0);
        sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sP1, sR2, sP0);
        sP = (sP * sR);

        sQ2 = as_float (__internal_stanpi_la_data._sQ2);
        sQ1 = as_float (__internal_stanpi_la_data._sQ1);
        sQ = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sQ2, sR2, sQ1);
        sQ0 = as_float (__internal_stanpi_la_data._sQ0);
        sQ = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sQ, sR2, sQ0);

        sNum = as_float ((((~as_uint (sInvMask)) & as_uint (sP)) | (as_uint (sInvMask) & as_uint (sQ))));
        sDen = as_float ((((~as_uint (sInvMask)) & as_uint (sQ)) | (as_uint (sInvMask) & as_uint (sP))));

        sRes = (sNum / sDen);

        sSpecRes = as_float ((as_uint (sRes) | as_uint (sInvBit)));
        sSpecMask = as_float (((unsigned int) (-(signed int) (sR == sZero))));
        vr1 = as_float ((as_uint (sRes) ^ as_uint (sSignBit)));
        vr1 = as_float ((((~as_uint (sSpecMask)) & as_uint (vr1)) | (as_uint (sSpecMask) & as_uint (sSpecRes))));
        vr1 = as_float ((as_uint (vr1) ^ as_uint (sSignX)));
    }

    if ((vm) != 0)
    {
        float _vapi_arg1[1];
        float _vapi_res1[1];
        ((float *) _vapi_arg1)[0] = va1;
        ((float *) _vapi_res1)[0] = vr1;
        __internal_stanpi_la_cout (_vapi_arg1, _vapi_res1);
        vr1 = ((float *) _vapi_res1)[0];
    };
    r = vr1;;

    return r;

}

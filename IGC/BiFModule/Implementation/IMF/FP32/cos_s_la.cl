/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF
typedef struct
{
    unsigned int _sPtable[256][3];
} __scos_la_ReductionTab_t;

static __constant __scos_la_ReductionTab_t __internal_scos_la_reduction_data = {
    {

     {0x00000000u, 0x00000000u, 0x00000000u},
     {0x00000000u, 0x00000000u, 0x00000000u},
     {0x00000000u, 0x00000000u, 0x00000000u},
     {0x00000000u, 0x00000000u, 0x00000000u},
     {0x00000000u, 0x00000000u, 0x00000000u},
     {0x00000000u, 0x00000000u, 0x00000000u},
     {0x00000000u, 0x00000000u, 0x00000000u},
     {0x00000000u, 0x00000000u, 0x00000000u},
     {0x00000000u, 0x00000000u, 0x00000000u},
     {0x00000000u, 0x00000000u, 0x00000000u},
     {0x00000000u, 0x00000000u, 0x00000000u},
     {0x00000000u, 0x00000000u, 0x00000000u},
     {0x00000000u, 0x00000000u, 0x00000000u},
     {0x00000000u, 0x00000000u, 0x00000000u},
     {0x00000000u, 0x00000000u, 0x00000000u},
     {0x00000000u, 0x00000000u, 0x00000000u},
     {0x00000000u, 0x00000000u, 0x00000000u},
     {0x00000000u, 0x00000000u, 0x00000000u},
     {0x00000000u, 0x00000000u, 0x00000000u},
     {0x00000000u, 0x00000000u, 0x00000000u},
     {0x00000000u, 0x00000000u, 0x00000000u},
     {0x00000000u, 0x00000000u, 0x00000000u},
     {0x00000000u, 0x00000000u, 0x00000000u},
     {0x00000000u, 0x00000000u, 0x00000000u},
     {0x00000000u, 0x00000000u, 0x00000000u},
     {0x00000000u, 0x00000000u, 0x00000000u},
     {0x00000000u, 0x00000000u, 0x00000000u},
     {0x00000000u, 0x00000000u, 0x00000000u},
     {0x00000000u, 0x00000000u, 0x00000000u},
     {0x00000000u, 0x00000000u, 0x00000000u},
     {0x00000000u, 0x00000000u, 0x00000000u},
     {0x00000000u, 0x00000000u, 0x00000000u},
     {0x00000000u, 0x00000000u, 0x00000000u},
     {0x00000000u, 0x00000000u, 0x00000000u},
     {0x00000000u, 0x00000000u, 0x00000000u},
     {0x00000000u, 0x00000000u, 0x00000000u},
     {0x00000000u, 0x00000000u, 0x00000000u},
     {0x00000000u, 0x00000000u, 0x00000000u},
     {0x00000000u, 0x00000000u, 0x00000000u},
     {0x00000000u, 0x00000000u, 0x00000000u},
     {0x00000000u, 0x00000000u, 0x00000000u},
     {0x00000000u, 0x00000000u, 0x00000000u},
     {0x00000000u, 0x00000000u, 0x00000000u},
     {0x00000000u, 0x00000000u, 0x00000000u},
     {0x00000000u, 0x00000000u, 0x00000000u},
     {0x00000000u, 0x00000000u, 0x00000000u},
     {0x00000000u, 0x00000000u, 0x00000000u},
     {0x00000000u, 0x00000000u, 0x00000000u},
     {0x00000000u, 0x00000000u, 0x00000000u},
     {0x00000000u, 0x00000000u, 0x00000000u},
     {0x00000000u, 0x00000000u, 0x00000000u},
     {0x00000000u, 0x00000000u, 0x00000000u},
     {0x00000000u, 0x00000000u, 0x00000000u},
     {0x00000000u, 0x00000000u, 0x00000000u},
     {0x00000000u, 0x00000000u, 0x00000000u},
     {0x00000000u, 0x00000000u, 0x00000000u},
     {0x00000000u, 0x00000000u, 0x00000000u},
     {0x00000000u, 0x00000000u, 0x00000001u},
     {0x00000000u, 0x00000000u, 0x00000002u},
     {0x00000000u, 0x00000000u, 0x00000005u},
     {0x00000000u, 0x00000000u, 0x0000000Au},
     {0x00000000u, 0x00000000u, 0x00000014u},
     {0x00000000u, 0x00000000u, 0x00000028u},
     {0x00000000u, 0x00000000u, 0x00000051u},
     {0x00000000u, 0x00000000u, 0x000000A2u},
     {0x00000000u, 0x00000000u, 0x00000145u},
     {0x00000000u, 0x00000000u, 0x0000028Bu},
     {0x00000000u, 0x00000000u, 0x00000517u},
     {0x00000000u, 0x00000000u, 0x00000A2Fu},
     {0x00000000u, 0x00000000u, 0x0000145Fu},
     {0x00000000u, 0x00000000u, 0x000028BEu},
     {0x00000000u, 0x00000000u, 0x0000517Cu},
     {0x00000000u, 0x00000000u, 0x0000A2F9u},
     {0x00000000u, 0x00000000u, 0x000145F3u},
     {0x00000000u, 0x00000000u, 0x00028BE6u},
     {0x00000000u, 0x00000000u, 0x000517CCu},
     {0x00000000u, 0x00000000u, 0x000A2F98u},
     {0x00000000u, 0x00000000u, 0x00145F30u},
     {0x00000000u, 0x00000000u, 0x0028BE60u},
     {0x00000000u, 0x00000000u, 0x00517CC1u},
     {0x00000000u, 0x00000000u, 0x00A2F983u},
     {0x00000000u, 0x00000000u, 0x0145F306u},
     {0x00000000u, 0x00000000u, 0x028BE60Du},
     {0x00000000u, 0x00000000u, 0x0517CC1Bu},
     {0x00000000u, 0x00000000u, 0x0A2F9836u},
     {0x00000000u, 0x00000000u, 0x145F306Du},
     {0x00000000u, 0x00000000u, 0x28BE60DBu},
     {0x00000000u, 0x00000000u, 0x517CC1B7u},
     {0x00000000u, 0x00000000u, 0xA2F9836Eu},
     {0x00000000u, 0x00000001u, 0x45F306DCu},
     {0x00000000u, 0x00000002u, 0x8BE60DB9u},
     {0x00000000u, 0x00000005u, 0x17CC1B72u},
     {0x00000000u, 0x0000000Au, 0x2F9836E4u},
     {0x00000000u, 0x00000014u, 0x5F306DC9u},
     {0x00000000u, 0x00000028u, 0xBE60DB93u},
     {0x00000000u, 0x00000051u, 0x7CC1B727u},
     {0x00000000u, 0x000000A2u, 0xF9836E4Eu},
     {0x00000000u, 0x00000145u, 0xF306DC9Cu},
     {0x00000000u, 0x0000028Bu, 0xE60DB939u},
     {0x00000000u, 0x00000517u, 0xCC1B7272u},
     {0x00000000u, 0x00000A2Fu, 0x9836E4E4u},
     {0x00000000u, 0x0000145Fu, 0x306DC9C8u},
     {0x00000000u, 0x000028BEu, 0x60DB9391u},
     {0x00000000u, 0x0000517Cu, 0xC1B72722u},
     {0x00000000u, 0x0000A2F9u, 0x836E4E44u},
     {0x00000000u, 0x000145F3u, 0x06DC9C88u},
     {0x00000000u, 0x00028BE6u, 0x0DB93910u},
     {0x00000000u, 0x000517CCu, 0x1B727220u},
     {0x00000000u, 0x000A2F98u, 0x36E4E441u},
     {0x00000000u, 0x00145F30u, 0x6DC9C882u},
     {0x00000000u, 0x0028BE60u, 0xDB939105u},
     {0x00000000u, 0x00517CC1u, 0xB727220Au},
     {0x00000000u, 0x00A2F983u, 0x6E4E4415u},
     {0x00000000u, 0x0145F306u, 0xDC9C882Au},
     {0x00000000u, 0x028BE60Du, 0xB9391054u},
     {0x00000000u, 0x0517CC1Bu, 0x727220A9u},
     {0x00000000u, 0x0A2F9836u, 0xE4E44152u},
     {0x00000000u, 0x145F306Du, 0xC9C882A5u},
     {0x00000000u, 0x28BE60DBu, 0x9391054Au},
     {0x00000000u, 0x517CC1B7u, 0x27220A94u},
     {0x00000000u, 0xA2F9836Eu, 0x4E441529u},
     {0x00000001u, 0x45F306DCu, 0x9C882A53u},
     {0x00000002u, 0x8BE60DB9u, 0x391054A7u},
     {0x00000005u, 0x17CC1B72u, 0x7220A94Fu},
     {0x0000000Au, 0x2F9836E4u, 0xE441529Fu},
     {0x00000014u, 0x5F306DC9u, 0xC882A53Fu},
     {0x00000028u, 0xBE60DB93u, 0x91054A7Fu},
     {0x00000051u, 0x7CC1B727u, 0x220A94FEu},
     {0x000000A2u, 0xF9836E4Eu, 0x441529FCu},
     {0x00000145u, 0xF306DC9Cu, 0x882A53F8u},
     {0x0000028Bu, 0xE60DB939u, 0x1054A7F0u},
     {0x00000517u, 0xCC1B7272u, 0x20A94FE1u},
     {0x00000A2Fu, 0x9836E4E4u, 0x41529FC2u},
     {0x0000145Fu, 0x306DC9C8u, 0x82A53F84u},
     {0x000028BEu, 0x60DB9391u, 0x054A7F09u},
     {0x0000517Cu, 0xC1B72722u, 0x0A94FE13u},
     {0x0000A2F9u, 0x836E4E44u, 0x1529FC27u},
     {0x000145F3u, 0x06DC9C88u, 0x2A53F84Eu},
     {0x00028BE6u, 0x0DB93910u, 0x54A7F09Du},
     {0x000517CCu, 0x1B727220u, 0xA94FE13Au},
     {0x000A2F98u, 0x36E4E441u, 0x529FC275u},
     {0x00145F30u, 0x6DC9C882u, 0xA53F84EAu},
     {0x0028BE60u, 0xDB939105u, 0x4A7F09D5u},
     {0x00517CC1u, 0xB727220Au, 0x94FE13ABu},
     {0x00A2F983u, 0x6E4E4415u, 0x29FC2757u},
     {0x0145F306u, 0xDC9C882Au, 0x53F84EAFu},
     {0x028BE60Du, 0xB9391054u, 0xA7F09D5Fu},
     {0x0517CC1Bu, 0x727220A9u, 0x4FE13ABEu},
     {0x0A2F9836u, 0xE4E44152u, 0x9FC2757Du},
     {0x145F306Du, 0xC9C882A5u, 0x3F84EAFAu},
     {0x28BE60DBu, 0x9391054Au, 0x7F09D5F4u},
     {0x517CC1B7u, 0x27220A94u, 0xFE13ABE8u},
     {0xA2F9836Eu, 0x4E441529u, 0xFC2757D1u},
     {0x45F306DCu, 0x9C882A53u, 0xF84EAFA3u},
     {0x8BE60DB9u, 0x391054A7u, 0xF09D5F47u},
     {0x17CC1B72u, 0x7220A94Fu, 0xE13ABE8Fu},
     {0x2F9836E4u, 0xE441529Fu, 0xC2757D1Fu},
     {0x5F306DC9u, 0xC882A53Fu, 0x84EAFA3Eu},
     {0xBE60DB93u, 0x91054A7Fu, 0x09D5F47Du},
     {0x7CC1B727u, 0x220A94FEu, 0x13ABE8FAu},
     {0xF9836E4Eu, 0x441529FCu, 0x2757D1F5u},
     {0xF306DC9Cu, 0x882A53F8u, 0x4EAFA3EAu},
     {0xE60DB939u, 0x1054A7F0u, 0x9D5F47D4u},
     {0xCC1B7272u, 0x20A94FE1u, 0x3ABE8FA9u},
     {0x9836E4E4u, 0x41529FC2u, 0x757D1F53u},
     {0x306DC9C8u, 0x82A53F84u, 0xEAFA3EA6u},
     {0x60DB9391u, 0x054A7F09u, 0xD5F47D4Du},
     {0xC1B72722u, 0x0A94FE13u, 0xABE8FA9Au},
     {0x836E4E44u, 0x1529FC27u, 0x57D1F534u},
     {0x06DC9C88u, 0x2A53F84Eu, 0xAFA3EA69u},
     {0x0DB93910u, 0x54A7F09Du, 0x5F47D4D3u},
     {0x1B727220u, 0xA94FE13Au, 0xBE8FA9A6u},
     {0x36E4E441u, 0x529FC275u, 0x7D1F534Du},
     {0x6DC9C882u, 0xA53F84EAu, 0xFA3EA69Bu},
     {0xDB939105u, 0x4A7F09D5u, 0xF47D4D37u},
     {0xB727220Au, 0x94FE13ABu, 0xE8FA9A6Eu},
     {0x6E4E4415u, 0x29FC2757u, 0xD1F534DDu},
     {0xDC9C882Au, 0x53F84EAFu, 0xA3EA69BBu},
     {0xB9391054u, 0xA7F09D5Fu, 0x47D4D377u},
     {0x727220A9u, 0x4FE13ABEu, 0x8FA9A6EEu},
     {0xE4E44152u, 0x9FC2757Du, 0x1F534DDCu},
     {0xC9C882A5u, 0x3F84EAFAu, 0x3EA69BB8u},
     {0x9391054Au, 0x7F09D5F4u, 0x7D4D3770u},
     {0x27220A94u, 0xFE13ABE8u, 0xFA9A6EE0u},
     {0x4E441529u, 0xFC2757D1u, 0xF534DDC0u},
     {0x9C882A53u, 0xF84EAFA3u, 0xEA69BB81u},
     {0x391054A7u, 0xF09D5F47u, 0xD4D37703u},
     {0x7220A94Fu, 0xE13ABE8Fu, 0xA9A6EE06u},
     {0xE441529Fu, 0xC2757D1Fu, 0x534DDC0Du},
     {0xC882A53Fu, 0x84EAFA3Eu, 0xA69BB81Bu},
     {0x91054A7Fu, 0x09D5F47Du, 0x4D377036u},
     {0x220A94FEu, 0x13ABE8FAu, 0x9A6EE06Du},
     {0x441529FCu, 0x2757D1F5u, 0x34DDC0DBu},
     {0x882A53F8u, 0x4EAFA3EAu, 0x69BB81B6u},
     {0x1054A7F0u, 0x9D5F47D4u, 0xD377036Du},
     {0x20A94FE1u, 0x3ABE8FA9u, 0xA6EE06DBu},
     {0x41529FC2u, 0x757D1F53u, 0x4DDC0DB6u},
     {0x82A53F84u, 0xEAFA3EA6u, 0x9BB81B6Cu},
     {0x054A7F09u, 0xD5F47D4Du, 0x377036D8u},
     {0x0A94FE13u, 0xABE8FA9Au, 0x6EE06DB1u},
     {0x1529FC27u, 0x57D1F534u, 0xDDC0DB62u},
     {0x2A53F84Eu, 0xAFA3EA69u, 0xBB81B6C5u},
     {0x54A7F09Du, 0x5F47D4D3u, 0x77036D8Au},
     {0xA94FE13Au, 0xBE8FA9A6u, 0xEE06DB14u},
     {0x529FC275u, 0x7D1F534Du, 0xDC0DB629u},
     {0xA53F84EAu, 0xFA3EA69Bu, 0xB81B6C52u},
     {0x4A7F09D5u, 0xF47D4D37u, 0x7036D8A5u},
     {0x94FE13ABu, 0xE8FA9A6Eu, 0xE06DB14Au},
     {0x29FC2757u, 0xD1F534DDu, 0xC0DB6295u},
     {0x53F84EAFu, 0xA3EA69BBu, 0x81B6C52Bu},
     {0xA7F09D5Fu, 0x47D4D377u, 0x036D8A56u},
     {0x4FE13ABEu, 0x8FA9A6EEu, 0x06DB14ACu},
     {0x9FC2757Du, 0x1F534DDCu, 0x0DB62959u},
     {0x3F84EAFAu, 0x3EA69BB8u, 0x1B6C52B3u},
     {0x7F09D5F4u, 0x7D4D3770u, 0x36D8A566u},
     {0xFE13ABE8u, 0xFA9A6EE0u, 0x6DB14ACCu},
     {0xFC2757D1u, 0xF534DDC0u, 0xDB629599u},
     {0xF84EAFA3u, 0xEA69BB81u, 0xB6C52B32u},
     {0xF09D5F47u, 0xD4D37703u, 0x6D8A5664u},
     {0xE13ABE8Fu, 0xA9A6EE06u, 0xDB14ACC9u},
     {0xC2757D1Fu, 0x534DDC0Du, 0xB6295993u},
     {0x84EAFA3Eu, 0xA69BB81Bu, 0x6C52B327u},
     {0x09D5F47Du, 0x4D377036u, 0xD8A5664Fu},
     {0x13ABE8FAu, 0x9A6EE06Du, 0xB14ACC9Eu},
     {0x2757D1F5u, 0x34DDC0DBu, 0x6295993Cu},
     {0x4EAFA3EAu, 0x69BB81B6u, 0xC52B3278u},
     {0x9D5F47D4u, 0xD377036Du, 0x8A5664F1u},
     {0x3ABE8FA9u, 0xA6EE06DBu, 0x14ACC9E2u},
     {0x757D1F53u, 0x4DDC0DB6u, 0x295993C4u},
     {0xEAFA3EA6u, 0x9BB81B6Cu, 0x52B32788u},
     {0xD5F47D4Du, 0x377036D8u, 0xA5664F10u},
     {0xABE8FA9Au, 0x6EE06DB1u, 0x4ACC9E21u},
     {0x57D1F534u, 0xDDC0DB62u, 0x95993C43u},
     {0xAFA3EA69u, 0xBB81B6C5u, 0x2B327887u},
     {0x5F47D4D3u, 0x77036D8Au, 0x5664F10Eu},
     {0xBE8FA9A6u, 0xEE06DB14u, 0xACC9E21Cu},
     {0x7D1F534Du, 0xDC0DB629u, 0x5993C439u},
     {0xFA3EA69Bu, 0xB81B6C52u, 0xB3278872u},
     {0xF47D4D37u, 0x7036D8A5u, 0x664F10E4u},
     {0xE8FA9A6Eu, 0xE06DB14Au, 0xCC9E21C8u},
     {0xD1F534DDu, 0xC0DB6295u, 0x993C4390u},
     {0xA3EA69BBu, 0x81B6C52Bu, 0x32788720u},
     {0x47D4D377u, 0x036D8A56u, 0x64F10E41u},
     {0x8FA9A6EEu, 0x06DB14ACu, 0xC9E21C82u},
     {0x1F534DDCu, 0x0DB62959u, 0x93C43904u},
     {0x3EA69BB8u, 0x1B6C52B3u, 0x27887208u},
     {0x7D4D3770u, 0x36D8A566u, 0x4F10E410u},
     {0xFA9A6EE0u, 0x6DB14ACCu, 0x9E21C820u},
     {0xF534DDC0u, 0xDB629599u, 0x3C439041u},
     {0xEA69BB81u, 0xB6C52B32u, 0x78872083u},
     {0xD4D37703u, 0x6D8A5664u, 0xF10E4107u},
     {0xA9A6EE06u, 0xDB14ACC9u, 0xE21C820Fu},
     {0x534DDC0Du, 0xB6295993u, 0xC439041Fu},
     {0xA69BB81Bu, 0x6C52B327u, 0x8872083Fu},
     {0x4D377036u, 0xD8A5664Fu, 0x10E4107Fu},
     {0x9A6EE06Du, 0xB14ACC9Eu, 0x21C820FFu}
     }
};

typedef struct
{
    unsigned int _dT[256][4];
    unsigned int _sAbsMask;
    unsigned int _sRangeReductionVal;
    unsigned int _sRangeVal;
    unsigned int _sS1;
    unsigned int _sS2;
    unsigned int _sC1;
    unsigned int _sC2;
    unsigned int _sPI1;
    unsigned int _sPI2;
    unsigned int _sPI3;
    unsigned int _sPI4;
    unsigned int _sPI1_FMA;
    unsigned int _sPI2_FMA;
    unsigned int _sPI3_FMA;
    unsigned int _sA3;
    unsigned int _sA5;
    unsigned int _sA7;
    unsigned int _sA9;
    unsigned int _sA5_FMA;
    unsigned int _sA7_FMA;
    unsigned int _sA9_FMA;
    unsigned int _sInvPI;
    unsigned int _sRShifter;
    unsigned int _sHalfPI;
    unsigned int _sOneHalf;
    unsigned int _sOne;
} __internal_scos_la_data_t;
static __constant __internal_scos_la_data_t __internal_scos_la_data = {
    {

     {0x00000000u, 0x3F800000u, 0x00000000u, 0x00000000u},
     {0x3BDBD541u, 0x3F7FEC43u, 0x3084CD0Du, 0xBD000000u},
     {0x3C5C1342u, 0x3F7FB10Fu, 0x31DE5B5Fu, 0xBD800000u},
     {0xBC354825u, 0x3F7F4E6Du, 0x32D01884u, 0xBD800000u},
     {0x3CDD0B28u, 0x3F7EC46Du, 0x31F44949u, 0xBE000000u},
     {0x3B29B1A9u, 0x3F7E1324u, 0xB2F1E603u, 0xBE000000u},
     {0xBCB2041Cu, 0x3F7D3AACu, 0xB0F75AE9u, 0xBE000000u},
     {0xBD3C4289u, 0x3F7C3B28u, 0xB231D68Bu, 0xBE000000u},
     {0x3D60E8F8u, 0x3F7B14BEu, 0x32FF75CBu, 0xBE800000u},
     {0x3CFD1F65u, 0x3F79C79Du, 0x32C64E59u, 0xBE800000u},
     {0x3BE60685u, 0x3F7853F8u, 0xB20DB9E5u, 0xBE800000u},
     {0xBC88E931u, 0x3F76BA07u, 0x326D092Cu, 0xBE800000u},
     {0xBD25018Cu, 0x3F74FA0Bu, 0xB2939D22u, 0xBE800000u},
     {0xBD826B93u, 0x3F731447u, 0x32C48E11u, 0xBE800000u},
     {0xBDB1F34Fu, 0x3F710908u, 0x321ED0DDu, 0xBE800000u},
     {0x3E0F77ADu, 0x3F6ED89Eu, 0xB29333DCu, 0xBF000000u},
     {0x3DF043ABu, 0x3F6C835Eu, 0x32F328D4u, 0xBF000000u},
     {0x3DC210D8u, 0x3F6A09A7u, 0xB2EB236Cu, 0xBF000000u},
     {0x3D945DFFu, 0x3F676BD8u, 0xB2BC3389u, 0xBF000000u},
     {0x3D4E645Au, 0x3F64AA59u, 0x311A08FAu, 0xBF000000u},
     {0x3CEA5164u, 0x3F61C598u, 0xB2E7F425u, 0xBF000000u},
     {0x3BE8B648u, 0x3F5EBE05u, 0x32C6F953u, 0xBF000000u},
     {0xBC670F32u, 0x3F5B941Au, 0x32232DC8u, 0xBF000000u},
     {0xBD0F59AAu, 0x3F584853u, 0xB27D5FC0u, 0xBF000000u},
     {0xBD639D9Du, 0x3F54DB31u, 0x3290EA1Au, 0xBF000000u},
     {0xBD9B4153u, 0x3F514D3Du, 0x300C4F04u, 0xBF000000u},
     {0xBDC3FDFFu, 0x3F4D9F02u, 0x327E70E8u, 0xBF000000u},
     {0xBDEBFE8Au, 0x3F49D112u, 0x32992640u, 0xBF000000u},
     {0xBE099E65u, 0x3F45E403u, 0x32B15174u, 0xBF000000u},
     {0xBE1CD957u, 0x3F41D870u, 0x32BFF977u, 0xBF000000u},
     {0xBE2FAD27u, 0x3F3DAEF9u, 0x319AABECu, 0xBF000000u},
     {0xBE4216EBu, 0x3F396842u, 0xB2810007u, 0xBF000000u},
     {0x3E95F61Au, 0x3F3504F3u, 0x324FE77Au, 0xBF800000u},
     {0x3E8D2F7Du, 0x3F3085BBu, 0xB2AE2D32u, 0xBF800000u},
     {0x3E84A20Eu, 0x3F2BEB4Au, 0xB2B73136u, 0xBF800000u},
     {0x3E789E3Fu, 0x3F273656u, 0xB2038343u, 0xBF800000u},
     {0x3E686FF3u, 0x3F226799u, 0x322123BBu, 0xBF800000u},
     {0x3E58BBB7u, 0x3F1D7FD1u, 0x3292050Cu, 0xBF800000u},
     {0x3E4983F7u, 0x3F187FC0u, 0xB1C7A3F3u, 0xBF800000u},
     {0x3E3ACB0Cu, 0x3F13682Au, 0x32CDD12Eu, 0xBF800000u},
     {0x3E2C933Bu, 0x3F0E39DAu, 0xB24A32E7u, 0xBF800000u},
     {0x3E1EDEB5u, 0x3F08F59Bu, 0xB2BE4B4Eu, 0xBF800000u},
     {0x3E11AF97u, 0x3F039C3Du, 0xB25BA002u, 0xBF800000u},
     {0x3E0507EAu, 0x3EFC5D27u, 0xB180ECA9u, 0xBF800000u},
     {0x3DF1D344u, 0x3EF15AEAu, 0xB1FF2139u, 0xBF800000u},
     {0x3DDAAD38u, 0x3EE63375u, 0xB1D9C774u, 0xBF800000u},
     {0x3DC4A143u, 0x3EDAE880u, 0x321E15CCu, 0xBF800000u},
     {0x3DAFB2CCu, 0x3ECF7BCAu, 0x316A3B63u, 0xBF800000u},
     {0x3D9BE50Cu, 0x3EC3EF15u, 0x31D5D52Cu, 0xBF800000u},
     {0x3D893B12u, 0x3EB8442Au, 0xB2705BA6u, 0xBF800000u},
     {0x3D6F6F7Eu, 0x3EAC7CD4u, 0xB2254E02u, 0xBF800000u},
     {0x3D4EBB8Au, 0x3EA09AE5u, 0xB23E89A0u, 0xBF800000u},
     {0x3D305F55u, 0x3E94A031u, 0x326D59F0u, 0xBF800000u},
     {0x3D145F8Cu, 0x3E888E93u, 0x312C7D9Eu, 0xBF800000u},
     {0x3CF58104u, 0x3E78CFCCu, 0xB11BD41Du, 0xBF800000u},
     {0x3CC70C54u, 0x3E605C13u, 0x31A7E4F6u, 0xBF800000u},
     {0x3C9D6830u, 0x3E47C5C2u, 0xB0E5967Du, 0xBF800000u},
     {0x3C71360Bu, 0x3E2F10A2u, 0x311167F9u, 0xBF800000u},
     {0x3C315502u, 0x3E164083u, 0x31E8E614u, 0xBF800000u},
     {0x3BF66E3Cu, 0x3DFAB273u, 0xB11568CFu, 0xBF800000u},
     {0x3B9DC971u, 0x3DC8BD36u, 0xB07592F5u, 0xBF800000u},
     {0x3B319298u, 0x3D96A905u, 0xB1531E61u, 0xBF800000u},
     {0x3A9DE1C8u, 0x3D48FB30u, 0xB0EF227Fu, 0xBF800000u},
     {0x399DE7DFu, 0x3CC90AB0u, 0xB005C998u, 0xBF800000u},
     {0x00000000u, 0x00000000u, 0x00000000u, 0xBF800000u},
     {0x399DE7DFu, 0xBCC90AB0u, 0x3005C998u, 0xBF800000u},
     {0x3A9DE1C8u, 0xBD48FB30u, 0x30EF227Fu, 0xBF800000u},
     {0x3B319298u, 0xBD96A905u, 0x31531E61u, 0xBF800000u},
     {0x3B9DC971u, 0xBDC8BD36u, 0x307592F5u, 0xBF800000u},
     {0x3BF66E3Cu, 0xBDFAB273u, 0x311568CFu, 0xBF800000u},
     {0x3C315502u, 0xBE164083u, 0xB1E8E614u, 0xBF800000u},
     {0x3C71360Bu, 0xBE2F10A2u, 0xB11167F9u, 0xBF800000u},
     {0x3C9D6830u, 0xBE47C5C2u, 0x30E5967Du, 0xBF800000u},
     {0x3CC70C54u, 0xBE605C13u, 0xB1A7E4F6u, 0xBF800000u},
     {0x3CF58104u, 0xBE78CFCCu, 0x311BD41Du, 0xBF800000u},
     {0x3D145F8Cu, 0xBE888E93u, 0xB12C7D9Eu, 0xBF800000u},
     {0x3D305F55u, 0xBE94A031u, 0xB26D59F0u, 0xBF800000u},
     {0x3D4EBB8Au, 0xBEA09AE5u, 0x323E89A0u, 0xBF800000u},
     {0x3D6F6F7Eu, 0xBEAC7CD4u, 0x32254E02u, 0xBF800000u},
     {0x3D893B12u, 0xBEB8442Au, 0x32705BA6u, 0xBF800000u},
     {0x3D9BE50Cu, 0xBEC3EF15u, 0xB1D5D52Cu, 0xBF800000u},
     {0x3DAFB2CCu, 0xBECF7BCAu, 0xB16A3B63u, 0xBF800000u},
     {0x3DC4A143u, 0xBEDAE880u, 0xB21E15CCu, 0xBF800000u},
     {0x3DDAAD38u, 0xBEE63375u, 0x31D9C774u, 0xBF800000u},
     {0x3DF1D344u, 0xBEF15AEAu, 0x31FF2139u, 0xBF800000u},
     {0x3E0507EAu, 0xBEFC5D27u, 0x3180ECA9u, 0xBF800000u},
     {0x3E11AF97u, 0xBF039C3Du, 0x325BA002u, 0xBF800000u},
     {0x3E1EDEB5u, 0xBF08F59Bu, 0x32BE4B4Eu, 0xBF800000u},
     {0x3E2C933Bu, 0xBF0E39DAu, 0x324A32E7u, 0xBF800000u},
     {0x3E3ACB0Cu, 0xBF13682Au, 0xB2CDD12Eu, 0xBF800000u},
     {0x3E4983F7u, 0xBF187FC0u, 0x31C7A3F3u, 0xBF800000u},
     {0x3E58BBB7u, 0xBF1D7FD1u, 0xB292050Cu, 0xBF800000u},
     {0x3E686FF3u, 0xBF226799u, 0xB22123BBu, 0xBF800000u},
     {0x3E789E3Fu, 0xBF273656u, 0x32038343u, 0xBF800000u},
     {0x3E84A20Eu, 0xBF2BEB4Au, 0x32B73136u, 0xBF800000u},
     {0x3E8D2F7Du, 0xBF3085BBu, 0x32AE2D32u, 0xBF800000u},
     {0x3E95F61Au, 0xBF3504F3u, 0xB24FE77Au, 0xBF800000u},
     {0xBE4216EBu, 0xBF396842u, 0x32810007u, 0xBF000000u},
     {0xBE2FAD27u, 0xBF3DAEF9u, 0xB19AABECu, 0xBF000000u},
     {0xBE1CD957u, 0xBF41D870u, 0xB2BFF977u, 0xBF000000u},
     {0xBE099E65u, 0xBF45E403u, 0xB2B15174u, 0xBF000000u},
     {0xBDEBFE8Au, 0xBF49D112u, 0xB2992640u, 0xBF000000u},
     {0xBDC3FDFFu, 0xBF4D9F02u, 0xB27E70E8u, 0xBF000000u},
     {0xBD9B4153u, 0xBF514D3Du, 0xB00C4F04u, 0xBF000000u},
     {0xBD639D9Du, 0xBF54DB31u, 0xB290EA1Au, 0xBF000000u},
     {0xBD0F59AAu, 0xBF584853u, 0x327D5FC0u, 0xBF000000u},
     {0xBC670F32u, 0xBF5B941Au, 0xB2232DC8u, 0xBF000000u},
     {0x3BE8B648u, 0xBF5EBE05u, 0xB2C6F953u, 0xBF000000u},
     {0x3CEA5164u, 0xBF61C598u, 0x32E7F425u, 0xBF000000u},
     {0x3D4E645Au, 0xBF64AA59u, 0xB11A08FAu, 0xBF000000u},
     {0x3D945DFFu, 0xBF676BD8u, 0x32BC3389u, 0xBF000000u},
     {0x3DC210D8u, 0xBF6A09A7u, 0x32EB236Cu, 0xBF000000u},
     {0x3DF043ABu, 0xBF6C835Eu, 0xB2F328D4u, 0xBF000000u},
     {0x3E0F77ADu, 0xBF6ED89Eu, 0x329333DCu, 0xBF000000u},
     {0xBDB1F34Fu, 0xBF710908u, 0xB21ED0DDu, 0xBE800000u},
     {0xBD826B93u, 0xBF731447u, 0xB2C48E11u, 0xBE800000u},
     {0xBD25018Cu, 0xBF74FA0Bu, 0x32939D22u, 0xBE800000u},
     {0xBC88E931u, 0xBF76BA07u, 0xB26D092Cu, 0xBE800000u},
     {0x3BE60685u, 0xBF7853F8u, 0x320DB9E5u, 0xBE800000u},
     {0x3CFD1F65u, 0xBF79C79Du, 0xB2C64E59u, 0xBE800000u},
     {0x3D60E8F8u, 0xBF7B14BEu, 0xB2FF75CBu, 0xBE800000u},
     {0xBD3C4289u, 0xBF7C3B28u, 0x3231D68Bu, 0xBE000000u},
     {0xBCB2041Cu, 0xBF7D3AACu, 0x30F75AE9u, 0xBE000000u},
     {0x3B29B1A9u, 0xBF7E1324u, 0x32F1E603u, 0xBE000000u},
     {0x3CDD0B28u, 0xBF7EC46Du, 0xB1F44949u, 0xBE000000u},
     {0xBC354825u, 0xBF7F4E6Du, 0xB2D01884u, 0xBD800000u},
     {0x3C5C1342u, 0xBF7FB10Fu, 0xB1DE5B5Fu, 0xBD800000u},
     {0x3BDBD541u, 0xBF7FEC43u, 0xB084CD0Du, 0xBD000000u},
     {0x00000000u, 0xBF800000u, 0x00000000u, 0x00000000u},
     {0xBBDBD541u, 0xBF7FEC43u, 0xB084CD0Du, 0x3D000000u},
     {0xBC5C1342u, 0xBF7FB10Fu, 0xB1DE5B5Fu, 0x3D800000u},
     {0x3C354825u, 0xBF7F4E6Du, 0xB2D01884u, 0x3D800000u},
     {0xBCDD0B28u, 0xBF7EC46Du, 0xB1F44949u, 0x3E000000u},
     {0xBB29B1A9u, 0xBF7E1324u, 0x32F1E603u, 0x3E000000u},
     {0x3CB2041Cu, 0xBF7D3AACu, 0x30F75AE9u, 0x3E000000u},
     {0x3D3C4289u, 0xBF7C3B28u, 0x3231D68Bu, 0x3E000000u},
     {0xBD60E8F8u, 0xBF7B14BEu, 0xB2FF75CBu, 0x3E800000u},
     {0xBCFD1F65u, 0xBF79C79Du, 0xB2C64E59u, 0x3E800000u},
     {0xBBE60685u, 0xBF7853F8u, 0x320DB9E5u, 0x3E800000u},
     {0x3C88E931u, 0xBF76BA07u, 0xB26D092Cu, 0x3E800000u},
     {0x3D25018Cu, 0xBF74FA0Bu, 0x32939D22u, 0x3E800000u},
     {0x3D826B93u, 0xBF731447u, 0xB2C48E11u, 0x3E800000u},
     {0x3DB1F34Fu, 0xBF710908u, 0xB21ED0DDu, 0x3E800000u},
     {0xBE0F77ADu, 0xBF6ED89Eu, 0x329333DCu, 0x3F000000u},
     {0xBDF043ABu, 0xBF6C835Eu, 0xB2F328D4u, 0x3F000000u},
     {0xBDC210D8u, 0xBF6A09A7u, 0x32EB236Cu, 0x3F000000u},
     {0xBD945DFFu, 0xBF676BD8u, 0x32BC3389u, 0x3F000000u},
     {0xBD4E645Au, 0xBF64AA59u, 0xB11A08FAu, 0x3F000000u},
     {0xBCEA5164u, 0xBF61C598u, 0x32E7F425u, 0x3F000000u},
     {0xBBE8B648u, 0xBF5EBE05u, 0xB2C6F953u, 0x3F000000u},
     {0x3C670F32u, 0xBF5B941Au, 0xB2232DC8u, 0x3F000000u},
     {0x3D0F59AAu, 0xBF584853u, 0x327D5FC0u, 0x3F000000u},
     {0x3D639D9Du, 0xBF54DB31u, 0xB290EA1Au, 0x3F000000u},
     {0x3D9B4153u, 0xBF514D3Du, 0xB00C4F04u, 0x3F000000u},
     {0x3DC3FDFFu, 0xBF4D9F02u, 0xB27E70E8u, 0x3F000000u},
     {0x3DEBFE8Au, 0xBF49D112u, 0xB2992640u, 0x3F000000u},
     {0x3E099E65u, 0xBF45E403u, 0xB2B15174u, 0x3F000000u},
     {0x3E1CD957u, 0xBF41D870u, 0xB2BFF977u, 0x3F000000u},
     {0x3E2FAD27u, 0xBF3DAEF9u, 0xB19AABECu, 0x3F000000u},
     {0x3E4216EBu, 0xBF396842u, 0x32810007u, 0x3F000000u},
     {0xBE95F61Au, 0xBF3504F3u, 0xB24FE77Au, 0x3F800000u},
     {0xBE8D2F7Du, 0xBF3085BBu, 0x32AE2D32u, 0x3F800000u},
     {0xBE84A20Eu, 0xBF2BEB4Au, 0x32B73136u, 0x3F800000u},
     {0xBE789E3Fu, 0xBF273656u, 0x32038343u, 0x3F800000u},
     {0xBE686FF3u, 0xBF226799u, 0xB22123BBu, 0x3F800000u},
     {0xBE58BBB7u, 0xBF1D7FD1u, 0xB292050Cu, 0x3F800000u},
     {0xBE4983F7u, 0xBF187FC0u, 0x31C7A3F3u, 0x3F800000u},
     {0xBE3ACB0Cu, 0xBF13682Au, 0xB2CDD12Eu, 0x3F800000u},
     {0xBE2C933Bu, 0xBF0E39DAu, 0x324A32E7u, 0x3F800000u},
     {0xBE1EDEB5u, 0xBF08F59Bu, 0x32BE4B4Eu, 0x3F800000u},
     {0xBE11AF97u, 0xBF039C3Du, 0x325BA002u, 0x3F800000u},
     {0xBE0507EAu, 0xBEFC5D27u, 0x3180ECA9u, 0x3F800000u},
     {0xBDF1D344u, 0xBEF15AEAu, 0x31FF2139u, 0x3F800000u},
     {0xBDDAAD38u, 0xBEE63375u, 0x31D9C774u, 0x3F800000u},
     {0xBDC4A143u, 0xBEDAE880u, 0xB21E15CCu, 0x3F800000u},
     {0xBDAFB2CCu, 0xBECF7BCAu, 0xB16A3B63u, 0x3F800000u},
     {0xBD9BE50Cu, 0xBEC3EF15u, 0xB1D5D52Cu, 0x3F800000u},
     {0xBD893B12u, 0xBEB8442Au, 0x32705BA6u, 0x3F800000u},
     {0xBD6F6F7Eu, 0xBEAC7CD4u, 0x32254E02u, 0x3F800000u},
     {0xBD4EBB8Au, 0xBEA09AE5u, 0x323E89A0u, 0x3F800000u},
     {0xBD305F55u, 0xBE94A031u, 0xB26D59F0u, 0x3F800000u},
     {0xBD145F8Cu, 0xBE888E93u, 0xB12C7D9Eu, 0x3F800000u},
     {0xBCF58104u, 0xBE78CFCCu, 0x311BD41Du, 0x3F800000u},
     {0xBCC70C54u, 0xBE605C13u, 0xB1A7E4F6u, 0x3F800000u},
     {0xBC9D6830u, 0xBE47C5C2u, 0x30E5967Du, 0x3F800000u},
     {0xBC71360Bu, 0xBE2F10A2u, 0xB11167F9u, 0x3F800000u},
     {0xBC315502u, 0xBE164083u, 0xB1E8E614u, 0x3F800000u},
     {0xBBF66E3Cu, 0xBDFAB273u, 0x311568CFu, 0x3F800000u},
     {0xBB9DC971u, 0xBDC8BD36u, 0x307592F5u, 0x3F800000u},
     {0xBB319298u, 0xBD96A905u, 0x31531E61u, 0x3F800000u},
     {0xBA9DE1C8u, 0xBD48FB30u, 0x30EF227Fu, 0x3F800000u},
     {0xB99DE7DFu, 0xBCC90AB0u, 0x3005C998u, 0x3F800000u},
     {0x00000000u, 0x00000000u, 0x00000000u, 0x3F800000u},
     {0xB99DE7DFu, 0x3CC90AB0u, 0xB005C998u, 0x3F800000u},
     {0xBA9DE1C8u, 0x3D48FB30u, 0xB0EF227Fu, 0x3F800000u},
     {0xBB319298u, 0x3D96A905u, 0xB1531E61u, 0x3F800000u},
     {0xBB9DC971u, 0x3DC8BD36u, 0xB07592F5u, 0x3F800000u},
     {0xBBF66E3Cu, 0x3DFAB273u, 0xB11568CFu, 0x3F800000u},
     {0xBC315502u, 0x3E164083u, 0x31E8E614u, 0x3F800000u},
     {0xBC71360Bu, 0x3E2F10A2u, 0x311167F9u, 0x3F800000u},
     {0xBC9D6830u, 0x3E47C5C2u, 0xB0E5967Du, 0x3F800000u},
     {0xBCC70C54u, 0x3E605C13u, 0x31A7E4F6u, 0x3F800000u},
     {0xBCF58104u, 0x3E78CFCCu, 0xB11BD41Du, 0x3F800000u},
     {0xBD145F8Cu, 0x3E888E93u, 0x312C7D9Eu, 0x3F800000u},
     {0xBD305F55u, 0x3E94A031u, 0x326D59F0u, 0x3F800000u},
     {0xBD4EBB8Au, 0x3EA09AE5u, 0xB23E89A0u, 0x3F800000u},
     {0xBD6F6F7Eu, 0x3EAC7CD4u, 0xB2254E02u, 0x3F800000u},
     {0xBD893B12u, 0x3EB8442Au, 0xB2705BA6u, 0x3F800000u},
     {0xBD9BE50Cu, 0x3EC3EF15u, 0x31D5D52Cu, 0x3F800000u},
     {0xBDAFB2CCu, 0x3ECF7BCAu, 0x316A3B63u, 0x3F800000u},
     {0xBDC4A143u, 0x3EDAE880u, 0x321E15CCu, 0x3F800000u},
     {0xBDDAAD38u, 0x3EE63375u, 0xB1D9C774u, 0x3F800000u},
     {0xBDF1D344u, 0x3EF15AEAu, 0xB1FF2139u, 0x3F800000u},
     {0xBE0507EAu, 0x3EFC5D27u, 0xB180ECA9u, 0x3F800000u},
     {0xBE11AF97u, 0x3F039C3Du, 0xB25BA002u, 0x3F800000u},
     {0xBE1EDEB5u, 0x3F08F59Bu, 0xB2BE4B4Eu, 0x3F800000u},
     {0xBE2C933Bu, 0x3F0E39DAu, 0xB24A32E7u, 0x3F800000u},
     {0xBE3ACB0Cu, 0x3F13682Au, 0x32CDD12Eu, 0x3F800000u},
     {0xBE4983F7u, 0x3F187FC0u, 0xB1C7A3F3u, 0x3F800000u},
     {0xBE58BBB7u, 0x3F1D7FD1u, 0x3292050Cu, 0x3F800000u},
     {0xBE686FF3u, 0x3F226799u, 0x322123BBu, 0x3F800000u},
     {0xBE789E3Fu, 0x3F273656u, 0xB2038343u, 0x3F800000u},
     {0xBE84A20Eu, 0x3F2BEB4Au, 0xB2B73136u, 0x3F800000u},
     {0xBE8D2F7Du, 0x3F3085BBu, 0xB2AE2D32u, 0x3F800000u},
     {0xBE95F61Au, 0x3F3504F3u, 0x324FE77Au, 0x3F800000u},
     {0x3E4216EBu, 0x3F396842u, 0xB2810007u, 0x3F000000u},
     {0x3E2FAD27u, 0x3F3DAEF9u, 0x319AABECu, 0x3F000000u},
     {0x3E1CD957u, 0x3F41D870u, 0x32BFF977u, 0x3F000000u},
     {0x3E099E65u, 0x3F45E403u, 0x32B15174u, 0x3F000000u},
     {0x3DEBFE8Au, 0x3F49D112u, 0x32992640u, 0x3F000000u},
     {0x3DC3FDFFu, 0x3F4D9F02u, 0x327E70E8u, 0x3F000000u},
     {0x3D9B4153u, 0x3F514D3Du, 0x300C4F04u, 0x3F000000u},
     {0x3D639D9Du, 0x3F54DB31u, 0x3290EA1Au, 0x3F000000u},
     {0x3D0F59AAu, 0x3F584853u, 0xB27D5FC0u, 0x3F000000u},
     {0x3C670F32u, 0x3F5B941Au, 0x32232DC8u, 0x3F000000u},
     {0xBBE8B648u, 0x3F5EBE05u, 0x32C6F953u, 0x3F000000u},
     {0xBCEA5164u, 0x3F61C598u, 0xB2E7F425u, 0x3F000000u},
     {0xBD4E645Au, 0x3F64AA59u, 0x311A08FAu, 0x3F000000u},
     {0xBD945DFFu, 0x3F676BD8u, 0xB2BC3389u, 0x3F000000u},
     {0xBDC210D8u, 0x3F6A09A7u, 0xB2EB236Cu, 0x3F000000u},
     {0xBDF043ABu, 0x3F6C835Eu, 0x32F328D4u, 0x3F000000u},
     {0xBE0F77ADu, 0x3F6ED89Eu, 0xB29333DCu, 0x3F000000u},
     {0x3DB1F34Fu, 0x3F710908u, 0x321ED0DDu, 0x3E800000u},
     {0x3D826B93u, 0x3F731447u, 0x32C48E11u, 0x3E800000u},
     {0x3D25018Cu, 0x3F74FA0Bu, 0xB2939D22u, 0x3E800000u},
     {0x3C88E931u, 0x3F76BA07u, 0x326D092Cu, 0x3E800000u},
     {0xBBE60685u, 0x3F7853F8u, 0xB20DB9E5u, 0x3E800000u},
     {0xBCFD1F65u, 0x3F79C79Du, 0x32C64E59u, 0x3E800000u},
     {0xBD60E8F8u, 0x3F7B14BEu, 0x32FF75CBu, 0x3E800000u},
     {0x3D3C4289u, 0x3F7C3B28u, 0xB231D68Bu, 0x3E000000u},
     {0x3CB2041Cu, 0x3F7D3AACu, 0xB0F75AE9u, 0x3E000000u},
     {0xBB29B1A9u, 0x3F7E1324u, 0xB2F1E603u, 0x3E000000u},
     {0xBCDD0B28u, 0x3F7EC46Du, 0x31F44949u, 0x3E000000u},
     {0x3C354825u, 0x3F7F4E6Du, 0x32D01884u, 0x3D800000u},
     {0xBC5C1342u, 0x3F7FB10Fu, 0x31DE5B5Fu, 0x3D800000u},
     {0xBBDBD541u, 0x3F7FEC43u, 0x3084CD0Du, 0x3D000000u},
     },

    0x7FFFFFFFu,
    0x461C4000u,
    0x7f800000u,

    0xBE2AAAABu,
    0x3C08885Cu,
    0xBF000000u,
    0x3D2AAA7Cu,
    0x40490000u,
    0x3A7DA000u,
    0x34222000u,
    0x2CB4611Au,

    0x40490FDBu,
    0xB3BBBD2Eu,
    0xA7772CEDu,

    0xBE2AAAA6u,
    0x3C088766u,
    0xB94FB7FFu,
    0x362EDEF8u,

    0x3C088764u,
    0xB94FB6CFu,
    0x362EC335u,
    0x3EA2F983u,
    0x4B400000u,
    0x3FC90FDBu,
    0x3F000000u,
    0x3F800000u,
};

#pragma float_control(precise,on)
static __constant _iml_v2_sp_union_t _vmls_CosHATab[2] = {
    0x00000000,
    0x7F800000
};

#pragma float_control(push)
#pragma float_control(precise, on)
__attribute__((always_inline))
inline int __internal_scos_la_cout (float *a, float *r)
{
    float absx;
    int nRet = 0;

    absx = ((*a));
    (((_iml_v2_sp_union_t *) & absx)->hex[0] = (((_iml_v2_sp_union_t *) & absx)->hex[0] & 0x7FFFFFFF) | ((_iml_uint32_t) (0) << 31));

    if (!((((((_iml_v2_sp_union_t *) & (*a))->hex[0] >> 23) & 0xFF) != 0xFF)))
    {
        if (((_iml_v2_sp_union_t *) & (absx))->hex[0] == ((__constant _iml_v2_sp_union_t *) & (((__constant float *) _vmls_CosHATab)[1]))->hex[0])
        {

            (*r) = (float) ((*a) * ((__constant float *) _vmls_CosHATab)[0]);

            nRet = 1;
            return nRet;
        }
        else
        {

            (*r) = (float) ((*a) * (*a));
            return nRet;
        }
    }
    return nRet;
}

#pragma float_control (pop)
float __ocl_svml_cosf (float a)
{

    float va1;
    float vr1;
    unsigned int vm;

    float r;

    va1 = a;;

    {

        float sAX;
        float sRangeReductionMask;
        unsigned int iRangeReductionMask;
        unsigned int mRangeReductionMask;

        float sX;
        float sHalfPI;

        float sRangeReductionVal;
        float sAbsMask;

        vm = 0;

        sHalfPI = as_float (__internal_scos_la_data._sHalfPI);
        sAbsMask = as_float (__internal_scos_la_data._sAbsMask);
        sAX = as_float ((as_uint (va1) & as_uint (sAbsMask)));

        sX = (sAX + sHalfPI);

        sRangeReductionVal = as_float (__internal_scos_la_data._sRangeReductionVal);
        sRangeReductionMask = as_float (((unsigned int) (-(signed int) (!(sAX <= sRangeReductionVal)))));

        {
            float sN;
            float sSign;
            float sR;
            float sR2;
            float sP;

            float sPI1;
            float sPI2;
            float sPI3;
            float sPI4;
            float sA1;
            float sSRA1;
            float sA3;
            float sA5;
            float sA7;
            float sA9;
            float sInvPI;
            float sRShifter;
            float sOneHalf;

            float sOne;
            float sNearZero;
            float sR3;

            sNearZero = as_float (((unsigned int) (-(signed int) (sX == sHalfPI))));

            sInvPI = as_float (__internal_scos_la_data._sInvPI);
            sRShifter = as_float (__internal_scos_la_data._sRShifter);

            sN = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sX, sInvPI, sRShifter);

            sSign = as_float (((unsigned int) as_uint (sN) << (31)));

            sN = (sN - sRShifter);

            sOneHalf = as_float (__internal_scos_la_data._sOneHalf);

            sN = (sN - sOneHalf);

            sPI1 = as_float (__internal_scos_la_data._sPI1);

            sR = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (-(sN), sPI1, sAX);
            sPI2 = as_float (__internal_scos_la_data._sPI2);
            sR = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (-(sN), sPI2, sR);
            sPI3 = as_float (__internal_scos_la_data._sPI3);
            sR = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (-(sN), sPI3, sR);

            sPI4 = as_float (__internal_scos_la_data._sPI4);
            sR = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (-(sN), sPI4, sR);
            sR2 = (sR * sR);

            sR = as_float ((as_uint (sR) ^ as_uint (sSign)));

            sA9 = as_float (__internal_scos_la_data._sA9);
            sA7 = as_float (__internal_scos_la_data._sA7);
            sA5 = as_float (__internal_scos_la_data._sA5);

            sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sA9, sR2, sA7);
            sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sP, sR2, sA5);
            sA3 = as_float (__internal_scos_la_data._sA3);
            sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sP, sR2, sA3);

            sR3 = (sR2 * sR);
            vr1 = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sP, sR3, sR);
            sOne = as_float (__internal_scos_la_data._sOne);
            vr1 = as_float ((((~as_uint (sNearZero)) & as_uint (vr1)) | (as_uint (sNearZero) & as_uint (sOne))));

        }

        iRangeReductionMask = as_uint (sRangeReductionMask);

        mRangeReductionMask = 0;
        mRangeReductionMask = iRangeReductionMask;
        if ((mRangeReductionMask) != 0)
        {

            float sRangeMask;
            unsigned int iRangeMask;
            float sN;
            unsigned int iIndex;
            float sE;
            float sR;
            float sRp2;
            float sPS;
            float sPC;
            float sRSigma;
            float sMed;
            float sD;
            float sCorr;
            float sK0;
            float sK1;
            float sK2;
            float sK3;
            float sResLo;
            float sResLo0;
            float sResInt;
            float sResHi;
            float sResLarge;

            float sRangeVal;
            float sRShifter;
            float sSigma;
            float sCHL;
            float sSHi;
            float sSLo;
            float sS1;
            float sS2;
            float sC1;
            float sC2;

            sRangeVal = as_float (__internal_scos_la_data._sRangeVal);
            sAX = as_float ((as_uint (sRangeVal) & as_uint (sAX)));
            sRangeMask = as_float (((unsigned int) (-(signed int) (sAX == sRangeVal))));
            iRangeMask = as_uint (sRangeMask);
            vm = 0;
            vm = iRangeMask;

            {

                {

                    unsigned int iInput;
                    unsigned int iExponent;
                    unsigned int iSignificand;

                    unsigned int iIntegerBit;

                    float sP_hi;
                    float sP_med;
                    float sP_lo;

                    unsigned int iP_hi;
                    unsigned int iP_med;
                    unsigned int iP_lo;

                    unsigned int iLowMask;

                    unsigned int iP5;
                    unsigned int iP4;
                    unsigned int iP3;
                    unsigned int iP2;
                    unsigned int iP1;
                    unsigned int iP0;

                    unsigned int iM1;
                    unsigned int iM0;

                    unsigned int iM15;
                    unsigned int iM14;
                    unsigned int iM13;
                    unsigned int iM12;
                    unsigned int iM11;
                    unsigned int iM10;
                    unsigned int iM05;
                    unsigned int iM04;
                    unsigned int iM03;
                    unsigned int iM02;
                    unsigned int iM01;
                    unsigned int iM00;

                    unsigned int iN14;
                    unsigned int iN13;
                    unsigned int iN12;
                    unsigned int iN11;

                    unsigned int iP15;
                    unsigned int iP14;
                    unsigned int iP13;
                    unsigned int iP12;
                    unsigned int iP11;

                    unsigned int iQ14;
                    unsigned int iQ13;
                    unsigned int iQ12;
                    unsigned int iQ11;

                    unsigned int iReduceHi;
                    unsigned int iReduceMed;
                    unsigned int iReducedLo;

                    unsigned int iRoundBump;
                    unsigned int iShiftedN;
                    unsigned int iNMask;

                    float sReducedHi;
                    float sReducedMed;
                    float sReducedLo;

                    unsigned int iExponentPart;
                    unsigned int iShiftedSig;

                    float sShifter;
                    float sIntegerPart;

                    float sRHi;
                    float sRLo;
                    unsigned int iSignBit;

                    float s2pi_full;
                    float s2pi_lead;
                    float s2pi_trail;

                    float sLeadmask;
                    float sRHi_lead;
                    float sRHi_trail;

                    float sPir1;
                    float sPir2;
                    float sPir3;
                    float sPir4;
                    float sPir12;
                    float sPir34;
                    float sRedPreHi;
                    float sRedHi;
                    float sRedLo;

                    float sMinInput;
                    float sAbs;
                    float sMultiplex;
                    float sNotMultiplex;
                    float sMultiplexedInput;
                    float sMultiplexedOutput;

                    iInput = as_uint (va1);

                    iExponent = 0x7f800000u;;
                    iExponent = (iExponent & iInput);
                    iExponent = ((unsigned int) (iExponent) >> (23));

                    sP_hi =
                        as_float (((__constant unsigned int *) (__internal_scos_la_reduction_data._sPtable))[(((0 + iExponent) * (3 * 4)) >> (2)) +
                                                                                                             0]);
                    sP_med =
                        as_float (((__constant unsigned int *) (__internal_scos_la_reduction_data._sPtable))[(((0 + iExponent) * (3 * 4)) >> (2)) +
                                                                                                             1]);
                    sP_lo =
                        as_float (((__constant unsigned int *) (__internal_scos_la_reduction_data._sPtable))[(((0 + iExponent) * (3 * 4)) >> (2)) +
                                                                                                             2]);

                    iP_hi = as_uint (sP_hi);
                    iP_med = as_uint (sP_med);
                    iP_lo = as_uint (sP_lo);

                    iSignificand = 0x007fffffu;;
                    iIntegerBit = 0x00800000u;
                    iSignificand = (iSignificand & iInput);
                    iSignificand = (iSignificand + iIntegerBit);

                    iLowMask = 0x0000FFFFu;
                    iP5 = ((unsigned int) (iP_hi) >> (16));
                    iP4 = (iP_hi & iLowMask);
                    iP3 = ((unsigned int) (iP_med) >> (16));
                    iP2 = (iP_med & iLowMask);
                    iP1 = ((unsigned int) (iP_lo) >> (16));
                    iP0 = (iP_lo & iLowMask);
                    iM1 = ((unsigned int) (iSignificand) >> (16));
                    iM0 = (iSignificand & iLowMask);

                    iM15 = (iM1 * iP5);
                    iM14 = (iM1 * iP4);
                    iM13 = (iM1 * iP3);
                    iM12 = (iM1 * iP2);
                    iM11 = (iM1 * iP1);
                    iM10 = (iM1 * iP0);
                    iM05 = (iM0 * iP5);
                    iM04 = (iM0 * iP4);
                    iM03 = (iM0 * iP3);
                    iM02 = (iM0 * iP2);
                    iM01 = (iM0 * iP1);
                    iM00 = (iM0 * iP0);

                    iN11 = ((unsigned int) (iM01) >> (16));
                    iN12 = ((unsigned int) (iM02) >> (16));
                    iN13 = ((unsigned int) (iM03) >> (16));
                    iN14 = ((unsigned int) (iM04) >> (16));

                    iN11 = (iM11 + iN11);
                    iN12 = (iM12 + iN12);
                    iN13 = (iM13 + iN13);
                    iN14 = (iM14 + iN14);

                    iP11 = (iM02 & iLowMask);
                    iP12 = (iM03 & iLowMask);
                    iP13 = (iM04 & iLowMask);
                    iP14 = (iM05 & iLowMask);
                    iP15 = ((unsigned int) (iM05) >> (16));

                    iP11 = (iP11 + iN11);
                    iP12 = (iP12 + iN12);
                    iP13 = (iP13 + iN13);
                    iP14 = (iP14 + iN14);
                    iP15 = (iP15 + iM15);

                    iQ11 = ((unsigned int) (iM10) >> (16));
                    iQ11 = (iQ11 + iP11);

                    iQ12 = ((unsigned int) (iQ11) >> (16));
                    iQ12 = (iQ12 + iP12);

                    iQ13 = ((unsigned int) (iQ12) >> (16));
                    iQ13 = (iQ13 + iP13);

                    iQ14 = ((unsigned int) (iQ13) >> (16));
                    iQ14 = (iQ14 + iP14);

                    iQ11 = (iQ11 & iLowMask);
                    iQ13 = (iQ13 & iLowMask);

                    iReduceHi = ((unsigned int) (iQ14) << (16));
                    iReducedLo = ((unsigned int) (iQ12) << (16));

                    iReduceHi = (iReduceHi + iQ13);
                    iReducedLo = (iReducedLo + iQ11);

                    iSignBit = 0x80000000u;;
                    iSignBit = (iSignBit & iInput);

                    iExponentPart = 0x3F800000u;
                    iExponentPart = (iSignBit ^ iExponentPart);
                    iShiftedSig = ((unsigned int) (iReduceHi) >> (9));
                    iShiftedSig = (iShiftedSig | iExponentPart);
                    sReducedHi = as_float (iShiftedSig);
                    sShifter = as_float (0x47400000u);
                    sIntegerPart = (sShifter + sReducedHi);
                    sN = (sIntegerPart - sShifter);
                    sReducedHi = (sReducedHi - sN);

                    iIndex = as_uint (sIntegerPart);
                    iNMask = 0x000000FFu;
                    iIndex = (iIndex & iNMask);
                    iExponentPart = 0x28800000u;
                    iExponentPart = (iSignBit ^ iExponentPart);
                    iShiftedSig = 0x0003FFFFu;
                    iShiftedSig = (iShiftedSig & iReducedLo);
                    iShiftedSig = ((unsigned int) (iShiftedSig) << (5));
                    iShiftedSig = (iShiftedSig | iExponentPart);
                    sReducedLo = as_float (iShiftedSig);
                    sShifter = as_float (iExponentPart);
                    sReducedLo = (sReducedLo - sShifter);

                    iExponentPart = 0x34000000u;
                    iExponentPart = (iSignBit ^ iExponentPart);
                    iShiftedSig = 0x000001FFu;
                    iShiftedSig = (iShiftedSig & iReduceHi);
                    iShiftedSig = ((unsigned int) (iShiftedSig) << (14));
                    iReducedLo = ((unsigned int) (iReducedLo) >> (18));
                    iShiftedSig = (iShiftedSig | iReducedLo);
                    iShiftedSig = (iShiftedSig | iExponentPart);
                    sReducedMed = as_float (iShiftedSig);
                    sShifter = as_float (iExponentPart);
                    sReducedMed = (sReducedMed - sShifter);

                    sRHi = (sReducedHi + sReducedMed);
                    sReducedHi = (sReducedHi - sRHi);
                    sReducedMed = (sReducedMed + sReducedHi);
                    sRLo = (sReducedMed + sReducedLo);
                    s2pi_full = as_float (0x40C90FDBu);
                    s2pi_lead = as_float (0x40C91000u);
                    s2pi_trail = as_float (0xB795777Au);

                    sLeadmask = as_float (0xFFFFF000u);
                    sRHi_lead = as_float ((as_uint (sRHi) & as_uint (sLeadmask)));
                    sRHi_trail = (sRHi - sRHi_lead);

                    sRedPreHi = (s2pi_lead * sRHi_lead);
                    sPir1 = (s2pi_lead * sRHi_trail);
                    sPir2 = (s2pi_trail * sRHi_lead);
                    sPir3 = (s2pi_full * sRLo);
                    sPir4 = (s2pi_trail * sRHi_trail);

                    sPir12 = (sPir1 + sPir2);
                    sPir34 = (sPir3 + sPir4);
                    sRedLo = (sPir12 + sPir34);

                    sRedHi = (sRedPreHi + sRedLo);
                    sRedPreHi = (sRedPreHi - sRedHi);
                    sRedLo = (sRedPreHi + sRedLo);
                    sAbsMask = as_float (0x7FFFFFFFu);
                    sMinInput = as_float (0x35800000u);
                    sAbs = as_float ((as_uint (va1) & as_uint (sAbsMask)));
                    sMultiplex = as_float (((unsigned int) (-(signed int) (sAbs > sMinInput))));
                    sNotMultiplex = as_float (((unsigned int) (-(signed int) (sAbs <= sMinInput))));

                    sMultiplexedInput = as_float ((as_uint (sNotMultiplex) & as_uint (va1)));
                    sMultiplexedOutput = as_float ((as_uint (sMultiplex) & as_uint (sRedHi)));
                    sR = as_float ((as_uint (sMultiplexedInput) | as_uint (sMultiplexedOutput)));
                    sE = as_float ((as_uint (sMultiplex) & as_uint (sRedLo)));

                }
            }

            sRp2 = (sR * sR);

            sCHL = as_float (((__constant unsigned int *) (__internal_scos_la_data._dT))[(((0 + iIndex) * (4 * 4)) >> (2)) + 0]);
            sSHi = as_float (((__constant unsigned int *) (__internal_scos_la_data._dT))[(((0 + iIndex) * (4 * 4)) >> (2)) + 1]);
            sSLo = as_float (((__constant unsigned int *) (__internal_scos_la_data._dT))[(((0 + iIndex) * (4 * 4)) >> (2)) + 2]);
            sSigma = as_float (((__constant unsigned int *) (__internal_scos_la_data._dT))[(((0 + iIndex) * (4 * 4)) >> (2)) + 3]);

            sMed = (sCHL * sR);
            sRSigma = (sR * sSigma);
            sResInt = (sSHi + sRSigma);
            sResHi = (sMed + sResInt);

            sK0 = (sSHi - sResInt);
            sK2 = (sK0 + sRSigma);
            sK1 = (sResInt - sResHi);
            sK3 = (sK1 + sMed);
            sResLo0 = (sK3 + sK2);
            sS2 = as_float (__internal_scos_la_data._sS2);
            sS1 = as_float (__internal_scos_la_data._sS1);
            sPS = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sS2, sRp2, sS1);
            sPS = (sPS * sRp2);
            sPS = (sPS * sR);
            sD = (sCHL + sSigma);

            sC2 = as_float (__internal_scos_la_data._sC2);
            sC1 = as_float (__internal_scos_la_data._sC1);
            sPC = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sC2, sRp2, sC1);
            sPC = (sPC * sRp2);

            sD = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (-(sR), sSHi, sD);
            sCorr = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sE, sD, sSLo);
            sResLo = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sPC, sSHi, sCorr);
            sResLo0 = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sPS, sD, sResLo0);
            sResLo = (sResLo + sResLo0);

            sResLarge = (sResHi + sResLo);

            vr1 = as_float ((((~as_uint (sRangeReductionMask)) & as_uint (vr1)) | (as_uint (sRangeReductionMask) & as_uint (sResLarge))));
        }
    }

    if ((vm) != 0)
    {
        float _vapi_arg1[1];
        float _vapi_res1[1];
        ((float *) _vapi_arg1)[0] = va1;
        ((float *) _vapi_res1)[0] = vr1;
        __internal_scos_la_cout (_vapi_arg1, _vapi_res1);
        vr1 = ((float *) _vapi_res1)[0];
    };
    r = vr1;;

    return r;

}

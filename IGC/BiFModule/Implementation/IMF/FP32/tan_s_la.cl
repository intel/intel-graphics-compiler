/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF
typedef struct
{
    unsigned int _sPtable[256][3];
} __stan_la_ReductionTab_t;

static __constant __stan_la_ReductionTab_t __internal_stan_la_reduction_data = {
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

    unsigned int _sInvPI_uisa;
    unsigned int _sPI1_uisa;
    unsigned int _sPI2_uisa;
    unsigned int _sPI3_uisa;
    unsigned int _sPI2_ha_uisa;
    unsigned int _sPI3_ha_uisa;
    unsigned int Th_tbl_uisa[32];
    unsigned int Tl_tbl_uisa[32];
    unsigned int _sPC3_uisa;
    unsigned int _sPC5_uisa;
    unsigned int _sRangeReductionVal_uisa;

    unsigned int _sInvPi;
    unsigned int _sSignMask;
    unsigned int _sAbsMask;
    unsigned int _sRangeVal;
    unsigned int _sRShifter;
    unsigned int _sOne;

    unsigned int _sRangeReductionVal;
    unsigned int _sPI1;
    unsigned int _sPI2;
    unsigned int _sPI3;
    unsigned int _sPI4;
    unsigned int _sPI1_FMA;
    unsigned int _sPI2_FMA;
    unsigned int _sPI3_FMA;

    unsigned int _sP0;
    unsigned int _sP1;
    unsigned int _sQ0;
    unsigned int _sQ1;
    unsigned int _sQ2;
    unsigned int _sTwo;

    unsigned int _sCoeffs[128][10];

} __internal_stan_la_data_t;
static __constant __internal_stan_la_data_t __internal_stan_la_data = {

    0x4122f983u,
    0x3dc90fdau,
    0x31a22168u,
    0x25c234c5u,
    0x31a22000u,
    0x2a34611au,

    {
     0x80000000u, 0x3dc9b5dcu, 0x3e4bafafu, 0x3e9b5042u,
     0x3ed413cdu, 0x3f08d5b9u, 0x3f2b0dc1u, 0x3f521801u,
     0x3f800000u, 0x3f9bf7ecu, 0x3fbf90c7u, 0x3fef789eu,
     0x401a827au, 0x4052facfu, 0x40a0dff7u, 0x41227363u,
     0xff7fffffu, 0xc1227363u, 0xc0a0dff7u, 0xc052facfu,
     0xc01a827au, 0xbfef789eu, 0xbfbf90c7u, 0xbf9bf7ecu,
     0xbf800000u, 0xbf521801u, 0xbf2b0dc1u, 0xbf08d5b9u,
     0xbed413cdu, 0xbe9b5042u, 0xbe4bafafu, 0xbdc9b5dcu,
     },

    {
     0x80000000u, 0x3145b2dau, 0x2f2a62b0u, 0xb22a39c2u,
     0xb1c0621au, 0xb25ef963u, 0x32ab7f99u, 0x32ae4285u,
     0x00000000u, 0x33587608u, 0x32169d18u, 0xb30c3ec0u,
     0xb3cc0622u, 0x3390600eu, 0x331091dcu, 0xb454a046u,
     0xf3800000u, 0x3454a046u, 0xb31091dcu, 0xb390600eu,
     0x33cc0622u, 0x330c3ec0u, 0xb2169d18u, 0xb3587608u,
     0x00000000u, 0xb2ae4285u, 0xb2ab7f99u, 0x325ef963u,
     0x31c0621au, 0x322a39c2u, 0xaf2a62b0u, 0xb145b2dau,
     },
    0x3eaaaaa6u,
    0x3e08b888u,
    0x46010000u,

    0x3F22F983u,
    0x80000000u,
    0x7FFFFFFFu,
    0x7f800000u,
    0x4B400000u,
    0x3f800000u,
    0x46010000u,

    0x3FC90000u,
    0x39FDA000u,
    0x33A22000u,
    0x2C34611Au,

    0x3FC90FDBu,
    0xB33BBD2Eu,
    0xA6F72CEDu,
    0x3F7FFFFCu,
    0xBDC433B4u,
    0x3F7FFFFCu,
    0xBEDBB7ABu,
    0x3C1F336Bu,
    0x40000000u,
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

};

#pragma float_control(precise,on)
static __constant _iml_v2_sp_union_t _vmlsTanHATab[2] = {
    0x00000000,
    0x7F800000
};

#pragma float_control(push)
#pragma float_control(precise, on)
__attribute__((always_inline))
inline int __internal_stan_la_cout (float *a, float *r)
{
    int nRet = 0;
    float x, absx;

    absx = ((*a));
    (((_iml_v2_sp_union_t *) & absx)->hex[0] = (((_iml_v2_sp_union_t *) & absx)->hex[0] & 0x7FFFFFFF) | ((_iml_uint32_t) (0) << 31));

    if (!((((((_iml_v2_sp_union_t *) & (*a))->hex[0] >> 23) & 0xFF) != 0xFF)))
    {
        if (((_iml_v2_sp_union_t *) & (absx))->hex[0] == ((__constant _iml_v2_sp_union_t *) & (((__constant float *) _vmlsTanHATab)[1]))->hex[0])
        {

            (*r) = (float) ((*a) * ((__constant float *) _vmlsTanHATab)[0]);

            nRet = 1;
        }
        else
        {

            (*r) = (float) ((*a) * (*a));
        }
    }
    return nRet;
}

#pragma float_control (pop)
float __ocl_svml_tanf (float a)
{

    float va1;
    float vr1;
    unsigned int vm;

    float r;

    va1 = a;;

    {

        float sAbsX;
        float sAbsMask;
        float sRangeReductionVal;
        float sRangeReductionMask;
        unsigned int iRangeReductionMask;
        unsigned int mRangeReductionMask;
        float sZero;
        float sSignX;
        float sInvPi;
        float sY;
        float sRShift;
        float sN;
        float sR;
        float sR2;
        float sPI1;
        float sPI2;
        float sPI3;
        float sPI4;
        float sSignRes;
        float sInvMask;
        float sP1;
        float sP0;
        float sP;
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
        float sTwo;
        float sTmp1;
        float sResLarge;

        vm = 0;

        sZero = as_float (0);
        sAbsMask = as_float (__internal_stan_la_data._sAbsMask);
        sAbsX = as_float ((as_uint (va1) & as_uint (sAbsMask)));
        sSignX = as_float ((~(as_uint (sAbsMask)) & as_uint (va1)));

        sRangeReductionVal = as_float (__internal_stan_la_data._sRangeReductionVal);
        sRangeReductionMask = as_float (((unsigned int) (-(signed int) (!(sAbsX <= sRangeReductionVal)))));
        iRangeReductionMask = as_uint (sRangeReductionMask);

        mRangeReductionMask = 0;
        mRangeReductionMask = iRangeReductionMask;

        {

            sInvPi = as_float (__internal_stan_la_data._sInvPi);
            sRShift = as_float (__internal_stan_la_data._sRShifter);
            sN = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sAbsX, sInvPi, sRShift);
            sY = (sN - sRShift);

            sPI1 = as_float (__internal_stan_la_data._sPI1);
            sR = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (-(sY), sPI1, sAbsX);
            sPI2 = as_float (__internal_stan_la_data._sPI2);
            sR = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (-(sY), sPI2, sR);
            sPI3 = as_float (__internal_stan_la_data._sPI3);
            sR = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (-(sY), sPI3, sR);

            sPI4 = as_float (__internal_stan_la_data._sPI4);
            sR = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (-(sY), sPI4, sR);
            sR2 = (sR * sR);

            sSignRes = as_float (((unsigned int) as_uint (sN) << (31)));
            sSignRes = as_float ((as_uint (sSignRes) ^ as_uint (sSignX)));
            sInvMask = as_float (((unsigned int) as_uint (sN) << (30)));
            sInvMask = as_float (((unsigned int) (-(signed int) (!(sInvMask == sZero)))));

            sP1 = as_float (__internal_stan_la_data._sP1);
            sP0 = as_float (__internal_stan_la_data._sP0);
            sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sP1, sR2, sP0);
            sQ2 = as_float (__internal_stan_la_data._sQ2);
            sQ1 = as_float (__internal_stan_la_data._sQ1);
            sQ = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sQ2, sR2, sQ1);
            sP = (sP * sR);
            sQ0 = as_float (__internal_stan_la_data._sQ0);
            sQ = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sQ, sR2, sQ0);

            sNumP = as_float ((~(as_uint (sInvMask)) & as_uint (sP)));
            sNumQ = as_float ((as_uint (sInvMask) & as_uint (sQ)));
            sNum = as_float ((as_uint (sNumP) | as_uint (sNumQ)));
            sDenP = as_float ((as_uint (sInvMask) & as_uint (sP)));
            sDenQ = as_float ((~(as_uint (sInvMask)) & as_uint (sQ)));
            sDen = as_float ((as_uint (sDenP) | as_uint (sDenQ)));
            sRes = (sNum / sDen);
            vr1 = as_float ((as_uint (sRes) ^ as_uint (sSignRes)));

        }

        if ((mRangeReductionMask) != 0)
        {
            float sX;
            float sRangeMask;
            unsigned int iRangeMask;
            unsigned int mRangeMask;
            float sN;
            float sY;
            unsigned int iY;
            unsigned int iIndex;
            float sE1;
            float sE2;
            float sE3;
            float sE4;
            float sE;
            float sR1;
            float sR2;
            float sR3;
            float sR4;
            float sR;
            float sRMed;
            float sRp2;
            float sPS;
            float sPC;
            float sPolS;
            float sPolC;
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

            float sAbsMask;
            float sRangeVal;
            float sPIu;
            float sRShifter;
            unsigned int iIndexMask;
            float sPIoHi;
            float sPIoLo1;
            float sPIoLo2;
            float sPIoTail;
            float sSigma;
            float sCHL;
            float sSHi;
            float sSLo;
            float sS1;
            float sS2;
            float sC1;
            float sC2;

            unsigned int iIndexPeriodMask;
            float sB_hi;
            float sB_lo;
            float sR_full;
            float sR_hi;
            float sR_lo;
            float sHalfMask;
            float sOne;
            float sTau;
            float sRecip_hi;
            float sRecip_lo;
            float sEr;
            float dR_RE;
            float dRE;
            float sRecip_ok;
            float dD_E;
            float sD2;
            float sZ2;
            float sZ4;
            float sH1;
            float sH2;
            float sH3;
            float sH4;
            float sH5;
            float sH6;
            float sH7;
            float sH8;
            float sH9;
            float sC0_hi;
            float sC0_lo;
            float sC1_hi;
            float sC1_lo;
            float sC3;
            float sC4;
            float sC5;
            float sC6;
            float sC7;
            float sEC1;
            float sP1;
            float sP2;
            float sP3;
            float sP4;
            float sP5;
            float sP6;
            float sP7;
            float sP8;
            float sP9;
            float sP10;
            float sP11;
            float sP12;
            float sP13;
            float sP14;
            float sP15;
            float sP16;
            float sLoad[10];

            float sExpMask;
            float sEMax;
            float sSpecialMask;
            unsigned int iSpecialMask;

            sExpMask = as_float (0x7f800000u);;
            sX = as_float ((as_uint (va1) & as_uint (sExpMask)));
            sEMax = as_float (0x7f800000u);;
            sSpecialMask = as_float (((unsigned int) (-(signed int) (sX == sEMax))));
            iSpecialMask = as_uint (sSpecialMask);
            vm = 0;
            vm = iSpecialMask;

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

                    iInput = as_uint (sAbsX);

                    iExponent = 0x7f800000u;;
                    iExponent = (iExponent & iInput);
                    iExponent = ((unsigned int) (iExponent) >> (23));

                    sP_hi =
                        as_float (((__constant unsigned int *) (__internal_stan_la_reduction_data._sPtable))[(((0 + iExponent) * (3 * 4)) >> (2)) +
                                                                                                             0]);
                    sP_med =
                        as_float (((__constant unsigned int *) (__internal_stan_la_reduction_data._sPtable))[(((0 + iExponent) * (3 * 4)) >> (2)) +
                                                                                                             1]);
                    sP_lo =
                        as_float (((__constant unsigned int *) (__internal_stan_la_reduction_data._sPtable))[(((0 + iExponent) * (3 * 4)) >> (2)) +
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
                    sAbs = as_float ((as_uint (sAbsX) & as_uint (sAbsMask)));
                    sMultiplex = as_float (((unsigned int) (-(signed int) (sAbs > sMinInput))));
                    sNotMultiplex = as_float (((unsigned int) (-(signed int) (sAbs <= sMinInput))));

                    sMultiplexedInput = as_float ((as_uint (sNotMultiplex) & as_uint (sAbsX)));
                    sMultiplexedOutput = as_float ((as_uint (sMultiplex) & as_uint (sRedHi)));
                    sR = as_float ((as_uint (sMultiplexedInput) | as_uint (sMultiplexedOutput)));
                    sE = as_float ((as_uint (sMultiplex) & as_uint (sRedLo)));

                }
            }

            iIndexPeriodMask = 0x0000007Fu;
            iIndex = (iIndex & iIndexPeriodMask);

            sR = (sR + sE);
            sB_hi = as_float (((__constant unsigned int *) (__internal_stan_la_data._sCoeffs))[(((0 + iIndex) * (10 * 4)) >> (2)) + 0]);
            sB_lo = as_float (((__constant unsigned int *) (__internal_stan_la_data._sCoeffs))[(((0 + iIndex) * (10 * 4)) >> (2)) + 1]);
            sTau = as_float (((__constant unsigned int *) (__internal_stan_la_data._sCoeffs))[(((0 + iIndex) * (10 * 4)) >> (2)) + 2]);
            sC0_hi = as_float (((__constant unsigned int *) (__internal_stan_la_data._sCoeffs))[(((0 + iIndex) * (10 * 4)) >> (2)) + 3]);
            sC0_lo = as_float (((__constant unsigned int *) (__internal_stan_la_data._sCoeffs))[(((0 + iIndex) * (10 * 4)) >> (2)) + 4]);
            sC1_hi = as_float (((__constant unsigned int *) (__internal_stan_la_data._sCoeffs))[(((0 + iIndex) * (10 * 4)) >> (2)) + 5]);
            sC1_lo = as_float (((__constant unsigned int *) (__internal_stan_la_data._sCoeffs))[(((0 + iIndex) * (10 * 4)) >> (2)) + 6]);
            sC2 = as_float (((__constant unsigned int *) (__internal_stan_la_data._sCoeffs))[(((0 + iIndex) * (10 * 4)) >> (2)) + 7]);
            sC3 = as_float (((__constant unsigned int *) (__internal_stan_la_data._sCoeffs))[(((0 + iIndex) * (10 * 4)) >> (2)) + 8]);
            sC4 = as_float (((__constant unsigned int *) (__internal_stan_la_data._sCoeffs))[(((0 + iIndex) * (10 * 4)) >> (2)) + 9]);
            sR_full = (sB_hi - sR);
            sHalfMask = as_float (0xFFFFF000u);
            sR_hi = as_float ((as_uint (sR_full) & as_uint (sHalfMask)));

            sR_lo = (sB_hi - sR_full);
            sR_lo = (sR_lo - sR);

            sR_full = (sR_full - sR_hi);
            sR_full = (sR_full + sB_lo);

            sR_lo = (sR_lo + sR_full);

            sRecip_hi = (1.0f / (sR_hi));
            sRecip_hi = as_float ((as_uint (sRecip_hi) & as_uint (sHalfMask)));

            sOne = as_float (__internal_stan_la_data._sOne);
            sEr = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (-(sR_hi), sRecip_hi, sOne);

            dR_RE = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sRecip_hi, sEr, sRecip_hi);
            sE2 = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sEr, sEr, sOne);
            sRecip_ok = (dR_RE * sE2);

            sR_lo = (sR_lo * sRecip_ok);

            dD_E = (sR_lo - sEr);
            sRecip_lo = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sR_lo, sR_lo, -(dD_E));
            sRecip_lo = (sRecip_lo * sRecip_ok);

            sRecip_hi = (sRecip_hi * sTau);
            sRecip_lo = (sRecip_lo * sTau);

            sH1 = (sC1_hi * sR);
            sH2 = (sC0_hi + sH1);
            sH3 = (sC0_hi - sH2);
            sH4 = (sH2 + sRecip_hi);
            sH5 = (sH3 + sH1);
            sH6 = (sRecip_hi - sH4);
            sH7 = (sH5 + sRecip_lo);
            sH8 = (sH6 + sH2);
            sH9 = (sH7 + sH8);
            sP3 = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sC2, sR, sC1_lo);
            sP4 = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sC4, sR, sC3);

            sZ2 = (sR * sR);
            sC1 = (sC1_hi + sC1_lo);

            sP6 = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sZ2, sP4, sP3);

            sP9 = (sC0_lo + sH9);

            sP10 = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sP6, sR, sP9);

            sResLarge = (sH4 + sP10);
            sResLarge = as_float ((as_uint (sResLarge) ^ as_uint (sSignX)));

            vr1 = as_float ((((~as_uint (sRangeReductionMask)) & as_uint (vr1)) | (as_uint (sRangeReductionMask) & as_uint (sResLarge))));
        }
    }

    if ((vm) != 0)
    {
        float _vapi_arg1[1];
        float _vapi_res1[1];
        ((float *) _vapi_arg1)[0] = va1;
        ((float *) _vapi_res1)[0] = vr1;
        __internal_stan_la_cout (_vapi_arg1, _vapi_res1);
        vr1 = ((float *) _vapi_res1)[0];
    };
    r = vr1;;

    return r;

}

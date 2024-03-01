/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
/*
// ALGORITHM DESCRIPTION:
//  *
//  *  Approximation formula:
//  *  erfc(x) ~ erfc(x0) - 2/sqrt(pi)*exp(-x0^2)*D * [ 1 + (x0*D)*p1(x0*D) +
D^2 * p3(x0*D) ]
//  *    D = x - x0
//  *    erfc(x0) and 2/sqrt(pi)*exp(-x0^2)/(2*x0) are tabulated
//  *
//
*********************************************************************************************
*/
#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF
typedef struct {
  unsigned int _erfc_tbl[644 * 2];
  unsigned int _AbsMask;
  unsigned int _MaxThreshold;
  unsigned int _SgnMask;
  unsigned int _One;
  unsigned int _SRound;
  unsigned int _TwoM32;
  unsigned int _TwoM64;
  unsigned int _Exp_X0_Mask;
  unsigned int _ExpMask;
  unsigned int _TwoM6;
  unsigned int _poly1_0;
  unsigned int _poly1_1;
  unsigned int _poly3_0;
  unsigned int _poly3_1;
  unsigned int _poly1_2;
  unsigned int _poly3_2;
  unsigned int _poly1_3;
  //    VVALUE     ( S, _poly1_4             );
  unsigned int _UF_Threshold;
  unsigned int _SplitMask;
  unsigned int _TwoP64;
} __ocl_svml_internal_serfc_ha_data_t;
static __ocl_svml_internal_serfc_ha_data_t __ocl_svml_internal_serfc_ha_data = {
    // _erfc_tbl:
    //  first value is erfc_high(x0)*2^64
    //  second value combines erfc_low(x0)/exp_high(x0) as top 6 bits,
    //  and 2.0/sqrt(pi)*exp(-x0*x0)/(2^exponent(erfc(x0))*2^64
    //  in bottom 26 bits (using just 3 bits of the SP exponent)
    {
        0x5f800000u, 0x00106ebbu, 0x5f7b7ca2u, 0x1c9065b4u, 0x5f76f9d4u,
        0x64904aa3u, 0x5f727828u, 0x04901d93u, 0x5f6df82bu, 0x388fde94u,
        0x5f697a6eu, 0x288f8dbdu, 0x5f64ff7eu, 0x688f2b2eu, 0x5f6087eau,
        0x2c8eb70au, 0x5f5c143cu, 0x548e317du, 0x5f57a500u, 0x388d9ab9u,
        0x5f533abeu, 0x3c8cf2f5u, 0x5f4ed5fdu, 0x688c3a6fu, 0x5f4a7743u,
        0x448b716cu, 0x5f461f12u, 0x2c8a9834u, 0x5f41cdeau, 0x5089af18u,
        0x5f3d844au, 0x2c88b66cu, 0x5f3942acu, 0x3887ae8bu, 0x5f350989u,
        0x1c8697d3u, 0x5f30d955u, 0x648572a8u, 0x5f2cb284u, 0x18843f72u,
        0x5f289582u, 0x7c82fe9fu, 0x5f2482bdu, 0x2c81b0a0u, 0x5f207a9au,
        0x748055e8u, 0x5f1c7d7fu, 0x487ddddfu, 0x5f188bcbu, 0x287af867u,
        0x5f14a5dau, 0x3477fc62u, 0x5f10cc04u, 0x8074ead4u, 0x5f0cfe9fu,
        0x1071c4c4u, 0x5f093df8u, 0x6c6e8b3eu, 0x5f058a5du, 0x606b3f51u,
        0x5f01e415u, 0x3467e20fu, 0x5efc96c5u, 0x38e4748eu, 0x5ef5810au,
        0x10e0f7e5u, 0x5eee876cu, 0x48dd6d2du, 0x5ee7aa5au, 0x00d9d57eu,
        0x5ee0ea34u, 0x34d631f4u, 0x5eda4757u, 0x24d283a7u, 0x5ed3c214u,
        0x00cecbb1u, 0x5ecd5ab3u, 0x18cb0b28u, 0x5ec71175u, 0x28c74323u,
        0x5ec0e692u, 0x10c374b5u, 0x5ebada38u, 0x28bfa0eeu, 0x5eb4ec8eu,
        0x44bbc8dcu, 0x5eaf1db3u, 0x20b7ed89u, 0x5ea96dbcu, 0x10b40ff9u,
        0x5ea3dcb7u, 0x08b0312eu, 0x5e9e6aa9u, 0x5cac5223u, 0x5e991792u,
        0x60a873cfu, 0x5e93e368u, 0x40a49721u, 0x5e8ece1au, 0x04a0bd06u,
        0x5e89d78fu, 0x049ce661u, 0x5e84ffa8u, 0x3099140fu, 0x5e804640u,
        0x249546e7u, 0x5e775654u, 0x09117fb9u, 0x5e6e5c65u, 0x190dbf4cu,
        0x5e659e42u, 0x1d0a0662u, 0x5e5d1b6du, 0x190655b1u, 0x5e54d35cu,
        0x3502adebu, 0x5e4cc57cu, 0x38fe1f73u, 0x5e44f12fu, 0x0cf6f777u,
        0x5e3d55cbu, 0x38efe513u, 0x5e35f2a1u, 0x1ce8e968u, 0x5e2ec6f5u,
        0x40e20584u, 0x5e27d207u, 0x08db3a64u, 0x5e21130bu, 0x08d488f8u,
        0x5e1a8930u, 0x3ccdf21cu, 0x5e1433a0u, 0x20c7769bu, 0x5e0e117cu,
        0x0cc11733u, 0x5e0821e0u, 0x2cbad48du, 0x5e0263e4u, 0x58b4af46u,
        0x5df9ad38u, 0x212ea7eau, 0x5deef22au, 0x2128bef3u, 0x5de494b5u,
        0x3122f4ceu, 0x5dda92eau, 0x1d1d49d9u, 0x5dd0ead2u, 0x3117be62u,
        0x5dc79a74u, 0x391252aau, 0x5dbe9fd3u, 0x110d06e3u, 0x5db5f8edu,
        0x2107db31u, 0x5dada3c1u, 0x2502cfadu, 0x5da59e4cu, 0x04fbc8c2u,
        0x5d9de689u, 0x28f23298u, 0x5d967a77u, 0x04e8dcc1u, 0x5d8f5812u,
        0x24dfc70eu, 0x5d887d5bu, 0x44d6f136u, 0x5d81e855u, 0x40ce5adfu,
        0x5d772e0bu, 0x19460399u, 0x5d6b0eeau, 0x213deae4u, 0x5d5f6f64u,
        0x1936102bu, 0x5d544b9eu, 0x092e72cbu, 0x5d499fc7u, 0x2527120fu,
        0x5d3f681cu, 0x2d1fed36u, 0x5d35a0e5u, 0x3119036eu, 0x5d2c4678u,
        0x211253dcu, 0x5d235538u, 0x310bdd96u, 0x5d1ac999u, 0x11059fa9u,
        0x5d12a01cu, 0x18ff3230u, 0x5d0ad554u, 0x20f391b9u, 0x5d0365e4u,
        0x2ce85bd0u, 0x5cf89d02u, 0x055d8e4cu, 0x5ceb17deu, 0x255326f3u,
        0x5cde360eu, 0x21492385u, 0x5cd1f165u, 0x193f81b6u, 0x5cc643dbu,
        0x25363f32u, 0x5cbb278fu, 0x1d2d59a1u, 0x5cb096c4u, 0x1d24cea4u,
        0x5ca68be4u, 0x251c9bd9u, 0x5c9d017fu, 0x3114beddu, 0x5c93f24cu,
        0x0d0d354bu, 0x5c8b5926u, 0x1905fcbfu, 0x5c833110u, 0x3cfe25afu,
        0x5c76ea69u, 0x0d70ea68u, 0x5c6841c0u, 0x156442f0u, 0x5c5a5f10u,
        0x05582a98u, 0x5c4d398au, 0x294c9cbdu, 0x5c40c8aeu, 0x114194c7u,
        0x5c350440u, 0x0d370e2cu, 0x5c29e44eu, 0x252d0474u, 0x5c1f612fu,
        0x09237336u, 0x5c15737cu, 0x211a561bu, 0x5c0c1417u, 0x0511a8e1u,
        0x5c033c22u, 0x0509675au, 0x5bf5ca06u, 0x19818d6bu, 0x5be610c3u,
        0x09742e22u, 0x5bd74046u, 0x056600c0u, 0x5bc94cd7u, 0x21588b05u,
        0x5bbc2b3au, 0x014bc574u, 0x5bafd0a4u, 0x1d3fa8c4u, 0x5ba432c2u,
        0x21342ddbu, 0x5b9947afu, 0x15294dcfu, 0x5b8f05f2u, 0x251f01ecu,
        0x5b85647eu, 0x191543aeu, 0x5b78b556u, 0x118c0cc2u, 0x5b67c06au,
        0x0d83570au, 0x5b57da71u, 0x09763931u, 0x5b48f45eu, 0x1566af65u,
        0x5b3affd9u, 0x2558059cu, 0x5b2def39u, 0x0d4a3127u, 0x5b21b57au,
        0x113d27b5u, 0x5b16463cu, 0x2d30df57u, 0x5b0b95bcu, 0x29254e7bu,
        0x5b0198cbu, 0x351a6beeu, 0x5af08999u, 0x0d902ed6u, 0x5adf1f58u,
        0x09868eb5u, 0x5acedfb9u, 0x017b06c5u, 0x5abfb89fu, 0x1d6a0a1du,
        0x5ab198e5u, 0x155a1876u, 0x5aa4704du, 0x194b237au, 0x5a982f7bu,
        0x2d3d1d6fu, 0x5a8cc7ebu, 0x112ff93bu, 0x5a822be3u, 0x0523aa56u,
        0x5a709cddu, 0x0d9824ceu, 0x5a5e46a8u, 0x158d5d3cu, 0x5a4d3e18u,
        0x058348c6u, 0x5a3d6d6fu, 0x0d73ba24u, 0x5a2ec034u, 0x15622096u,
        0x5a212321u, 0x1151b22au, 0x5a148413u, 0x01425d18u, 0x5a08d1fbu,
        0x2d341080u, 0x59fbf9acu, 0x01a6bc5eu, 0x59e7eb28u, 0x119a5183u,
        0x59d55c2cu, 0x198ec18cu, 0x59c43238u, 0x0d83fedau, 0x59b45472u,
        0x0d73f919u, 0x59a5ab93u, 0x09615ce8u, 0x599821ceu, 0x0950121bu,
        0x598ba2bcu, 0x154002f8u, 0x59801b49u, 0x09311afeu, 0x596af33bu,
        0x15a346d7u, 0x59575a21u, 0x0596744cu, 0x59454c24u, 0x058a9237u,
        0x5934ac3au, 0x217f20e7u, 0x59255f50u, 0x1d6abfa9u, 0x59174c27u,
        0x1157e42fu, 0x590a5b3bu, 0x0d4673afu, 0x58fced4fu, 0x15b65507u,
        0x58e7141du, 0x09a770a1u, 0x58d304fbu, 0x1199b066u, 0x58c09cb4u,
        0x118cffa3u, 0x58afba92u, 0x0d814af8u, 0x58a04037u, 0x0d6d0088u,
        0x58921176u, 0x1d591d2du, 0x58851430u, 0x0146cc35u, 0x58726058u,
        0x05b5f011u, 0x585c9dfau, 0x01a66d2au, 0x5848b7beu, 0x059829c8u,
        0x58368738u, 0x058b0df2u, 0x5825e8dfu, 0x1d7e06abu, 0x5816bbdeu,
        0x1567ea53u, 0x5808e1dfu, 0x1153a034u, 0x57f87dc3u, 0x09c1045eu,
        0x57e1721eu, 0x11aff56du, 0x57cc712au, 0x01a05461u, 0x57b94efbu,
        0x05920475u, 0x57a7e31eu, 0x0584eafcu, 0x57980855u, 0x0171de7au,
        0x57899c5bu, 0x255bf4a3u, 0x5778ff5fu, 0x11c7ee0eu, 0x57612ab5u,
        0x05b5a381u, 0x574b8582u, 0x15a4f0bcu, 0x5737deeeu, 0x1d95b43bu,
        0x57260a26u, 0x1987cf0eu, 0x5715de0eu, 0x1976494du, 0x570734f9u,
        0x095f355eu, 0x56f3d8c4u, 0x01ca31bfu, 0x56dbc95du, 0x15b711b4u,
        0x56c601e9u, 0x0da5ac0eu, 0x56b24d80u, 0x1995dae6u, 0x56a07bcau,
        0x01877b62u, 0x56906098u, 0x0574daf4u, 0x5681d395u, 0x055d2782u,
        0x56695fe3u, 0x01c7a666u, 0x5651a834u, 0x0db42611u, 0x563c42e7u,
        0x09a2790fu, 0x5628f835u, 0x0d9275b9u, 0x56179554u, 0x1183f5e9u,
        0x5607ec0bu, 0x016dad68u, 0x55f3a498u, 0x05d5f04fu, 0x55da43bau,
        0x09c07a1du, 0x55c37002u, 0x0dad1570u, 0x55aeea57u, 0x159b9188u,
        0x559c7974u, 0x098bc1e7u, 0x558be962u, 0x117afbecu, 0x557a160bu,
        0x11e14167u, 0x555f6758u, 0x15ca10ceu, 0x55477954u, 0x15b52cb9u,
        0x553205fbu, 0x05a25d43u, 0x551ecdf2u, 0x15916f95u, 0x550d97f1u,
        0x15823575u, 0x54fc6061u, 0x09e909ccu, 0x54e0cfd4u, 0x05d06f88u,
        0x54c829c5u, 0x11ba56e4u, 0x54b221feu, 0x19a68119u, 0x549e73c2u,
        0x0994b538u, 0x548ce119u, 0x1184bfb1u, 0x547a6461u, 0x0dece39bu,
        0x545e6986u, 0x15d34296u, 0x54457726u, 0x09bc4fdcu, 0x542f3bdbu,
        0x09a7c64bu, 0x541b6e78u, 0x15956771u, 0x5409cd3du, 0x1584fae8u,
        0x53f43a38u, 0x09ec9b8fu, 0x53d85225u, 0x09d2644fu, 0x53bf8325u,
        0x05bafd47u, 0x53a977efu, 0x0da61be0u, 0x5395e415u, 0x0d937cf0u,
        0x53848320u, 0x0982e402u, 0x536a2f8au, 0x11e8356bu, 0x534ed659u,
        0x09cde050u, 0x53369893u, 0x09b670d5u, 0x53211e89u, 0x15a19816u,
        0x530e19e6u, 0x118f0f43u, 0x52fa896eu, 0x09fd2da8u, 0x52dcc11eu,
        0x05dfebabu, 0x52c26b7fu, 0x01c5f27cu, 0x52ab2548u, 0x15aee6b3u,
        0x52969604u, 0x0d9a767cu, 0x52846ee8u, 0x158858a4u, 0x5268d3a8u,
        0x05f09770u, 0x524c90c5u, 0x11d42a75u, 0x5233a635u, 0x09bb01ddu,
        0x521db15bu, 0x09a4c066u, 0x520a5aa0u, 0x059112d9u, 0x51f2a880u,
        0x0dff5e08u, 0x51d4b276u, 0x0de0a591u, 0x51ba58bcu, 0x15c5867eu,
        0x51a32ed5u, 0x0dad97f3u, 0x518ed4a1u, 0x11987c9eu, 0x5179ea08u,
        0x0205e186u, 0x515a8950u, 0x11eaf9d4u, 0x513f01d8u, 0x11ce1a8cu,
        0x5126ddccu, 0x09b4b0dcu, 0x5111b50cu, 0x159e558au, 0x50fe5749u,
        0x120aad05u, 0x50dde0d0u, 0x01f2cc48u, 0x50c1776du, 0x15d47203u,
        0x50a89cd2u, 0x15b9cbe3u, 0x5092e1aau, 0x09a26937u, 0x507fc7c2u,
        0x020de652u, 0x505e9a3au, 0x09f7d629u, 0x5041a2c2u, 0x11d85345u,
        0x50285baeu, 0x05bcba74u, 0x50124f78u, 0x15a49254u, 0x4ffe2dc3u,
        0x0a0f6fd2u, 0x4fdcae76u, 0x11f9e90cu, 0x4fbf81dcu, 0x0dd99a7du,
        0x4fa61c4du, 0x15bd616cu, 0x4f900357u, 0x09a4bcd6u, 0x4f799750u,
        0x020f3b3eu, 0x4f582e7fu, 0x01f8f1a9u, 0x4f3b27a3u, 0x09d83bc6u,
        0x4f21f2b6u, 0x05bbbab7u, 0x4f0c11c0u, 0x09a2e733u, 0x4ef22d4bu,
        0x0e0d4a80u, 0x4ed14265u, 0x0df4f900u, 0x4eb4bacau, 0x0dd443dfu,
        0x4e9c03d5u, 0x11b7d5b2u, 0x4e869d9au, 0x0d9f2272u, 0x4e68311fu,
        0x0a09af94u, 0x4e482705u, 0x0dee23cfu, 0x4e2c7394u, 0x11cdd76au,
        0x4e148365u, 0x15b1d636u, 0x4dffac6du, 0x0619912du, 0x4ddbf8f9u,
        0x0a048b78u, 0x4dbd2a87u, 0x09e4b060u, 0x4da298a0u, 0x0dc530bau,
        0x4d8bb0fcu, 0x05a9f27cu, 0x4d6fe94bu, 0x06126579u, 0x4d4deaf7u,
        0x11fc1860u, 0x4d30a807u, 0x0dd8f2d6u, 0x4d177affu, 0x0dba9c63u,
        0x4d01d48cu, 0x01a06fecu, 0x4cde7135u, 0x0209de04u, 0x4cbe77a1u,
        0x09ecd440u, 0x4ca302b0u, 0x09cb505au, 0x4c8b71e5u, 0x05ae74d6u,
        0x4c6e7533u, 0x0a159f2au, 0x4c4bca2du, 0x06004280u, 0x4c2e1413u,
        0x11dbc9bfu, 0x4c14a0a8u, 0x0dbc396du, 0x4bfdac7du, 0x02211d60u,
        0x4bd860d3u, 0x0e09d7a8u, 0x4bb87a3au, 0x05ebbf86u, 0x4b9d3454u,
        0x09c97f9au, 0x4b85e62fu, 0x15ac23f7u, 0x4b63fd06u, 0x0612fcf7u,
        0x4b4200dau, 0x09fae63cu, 0x4b25014du, 0x05d607b0u, 0x4b0c4622u,
        0x11b67d4eu, 0x4aee627du, 0x061b855cu, 0x4aca7603u, 0x06047920u,
        0x4aabde4au, 0x09e19261u, 0x4a91d40cu, 0x11bff482u, 0x4a77593cu,
        0x062344bfu, 0x4a51abc9u, 0x0e0acd1cu, 0x4a31a5bfu, 0x0debe2cau,
        0x4a167151u, 0x11c85727u, 0x49feaf84u, 0x062a1178u, 0x49d779e7u,
        0x02104cc0u, 0x49b63730u, 0x11f4c081u, 0x499a03dau, 0x11cf7703u,
        0x49821dcfu, 0x0dafc5c4u, 0x495bbf2eu, 0x0614d94au, 0x49397862u,
        0x09fbf99cu, 0x491c770bu, 0x11d52bb9u, 0x4903eeb4u, 0x05b4411fu,
        0x48de62c7u, 0x061858a7u, 0x48bb5652u, 0x0600b23eu, 0x489dbc88u,
        0x11d9543fu, 0x4884bfc1u, 0x0db76979u, 0x485f551eu, 0x0a1ab66bu,
        0x483bc5f1u, 0x060270afu, 0x481dccc1u, 0x0ddbd820u, 0x48048c15u,
        0x0db92c31u, 0x47de907eu, 0x061be496u, 0x47bac490u, 0x06032dcbu,
        0x479ca73du, 0x01dca86au, 0x478354cbu, 0x01b97ecau, 0x475c1947u,
        0x061bdc1eu, 0x473857fau, 0x0602e528u, 0x471a529eu, 0x09dbc03fu,
        0x470120efu, 0x05b85f58u, 0x46d7fdc8u, 0x061a9d36u, 0x46b48e3eu,
        0x06019878u, 0x4696dc65u, 0x05d9250bu, 0x467bfa7cu, 0x0635d48du,
        0x465255b2u, 0x06182f4cu, 0x462f7d26u, 0x0dfe9ef1u, 0x4612586bu,
        0x0dd4e64au, 0x45f3f750u, 0x0231ed7cu, 0x45cb4136u, 0x0e14a0bbu,
        0x45a94169u, 0x09f82f41u, 0x458ce028u, 0x09cf1ceeu, 0x456a654du,
        0x0a2cc100u, 0x4542e7d6u, 0x06100645u, 0x4521fd98u, 0x0df00749u,
        0x450691c0u, 0x11c7ea70u, 0x44df78d8u, 0x0e266ce0u, 0x44b976f1u,
        0x0e0a7a45u, 0x4499d8e4u, 0x11e65575u, 0x447f1de7u, 0x0a3f778au,
        0x44536be8u, 0x0a1f14b2u, 0x442f2031u, 0x0a041bb4u, 0x4410fdbdu,
        0x05db4f88u, 0x43eff7deu, 0x0a35f2bau, 0x43c67c0au, 0x0a16e08du,
        0x43a417d3u, 0x0dfa1a1eu, 0x43879862u, 0x09cf30a7u, 0x435ffc36u,
        0x0a2b8e9au, 0x4338e85au, 0x0a0dfba1u, 0x431892fbu, 0x05eae636u,
        0x42fbaaf5u, 0x0642374fu, 0x42cf75d5u, 0x02208024u, 0x42aaef8bu,
        0x0e0492c0u, 0x428cc60cu, 0x05dae75bu, 0x4267c170u, 0x0634a340u,
        0x423ead7bu, 0x0614fcf4u, 0x421cce13u, 0x0df5a5cfu, 0x4200e32cu,
        0x0dca68efu, 0x41d3c741u, 0x0226b383u, 0x41ade7c9u, 0x060939a3u,
        0x418ebc8du, 0x11e1cfd0u, 0x416a31edu, 0x0a39b3ceu, 0x414008fdu,
        0x0a18a48bu, 0x411d638eu, 0x09fad08du, 0x4100ee61u, 0x09cdf654u,
        0x40d322ceu, 0x06290c4du, 0x40accb15u, 0x0a0aaea4u, 0x408d5872u,
        0x09e36e45u, 0x4067216eu, 0x0a3a6544u, 0x403ce218u, 0x0218b095u,
        0x401a4893u, 0x11fa092du, 0x3ffbec06u, 0x0a4c9f41u, 0x3fcd9386u,
        0x02276003u, 0x3fa7acfcu, 0x0e08d758u, 0x3f88b255u, 0x0ddfa559u,
        0x3f5ec64bu, 0x0236ab0fu, 0x3f3570b9u, 0x02152037u, 0x3f13b439u,
        0x0df35dd3u, 0x3ef05d68u, 0x0a467c0au, 0x3ec37bc4u, 0x0221ccfcu,
        0x3e9ee7d7u, 0x0a03d518u, 0x3e811c13u, 0x11d6b981u, 0x3e51b336u,
        0x0e2ec87au, 0x3e2a371cu, 0x060e33a7u, 0x3e0a1932u, 0x05e74660u,
        0x3ddff98bu, 0x063bfab1u, 0x3db589f5u, 0x0e18b6d4u, 0x3d931279u,
        0x0df80230u, 0x3d6e2ef2u, 0x06494905u, 0x3d40c69du, 0x022348d9u,
        0x3d1bf30fu, 0x120464b5u, 0x3cfc31bfu, 0x06569685u, 0x3ccbd1eeu,
        0x062dd231u, 0x3ca4a51fu, 0x0e0cbaf0u, 0x3c84ef71u, 0x0de3c449u,
        0x3c568fcfu, 0x0a3839e2u, 0x3c2d122eu, 0x0a14efa4u, 0x3c0b893bu,
        0x11f0b1dfu, 0x3be0e3a5u, 0x0a4265d8u, 0x3bb5237au, 0x021cede7u,
        0x3b91d42cu, 0x05fd3dc9u, 0x3b6ab0cbu, 0x024c3b50u, 0x3b3cc25bu,
        0x0224a05eu, 0x3b17be61u, 0x0e04a304u, 0x3af3db16u, 0x06559f46u,
        0x3ac3d8aeu, 0x0a2bf19fu, 0x3a9d3680u, 0x060a545bu, 0x3a7c475du,
        0x025e76f3u, 0x3a4a5140u, 0x0a32cc8cu, 0x3a222c04u, 0x0a0fa24cu,
        0x3a01edfcu, 0x09e6a843u, 0x39d01832u, 0x0e391cbau, 0x39a68f97u,
        0x06147ce5u, 0x398540a5u, 0x01ee1a5bu, 0x39551b64u, 0x0a3eced8u,
        0x392a535bu, 0x0618d52bu, 0x39081118u, 0x0df4b613u, 0x38d94acfu,
        0x0243d10cu, 0x38ad6b36u, 0x061c9d69u, 0x388a563fu, 0x05fa666du,
        0x385c98dcu, 0x06481354u, 0x382fcd12u, 0x021fc975u, 0x380c08aau,
        0x01ff1904u, 0x37defaaeu, 0x0a4b87d8u, 0x37b17112u, 0x0e224ef3u,
        0x378d22bbu, 0x0a015f38u, 0x3760685eu, 0x0a4e2339u, 0x373251c3u,
        0x0224258fu, 0x370da0c5u, 0x0e02a54du, 0x36e0dd24u, 0x0a4fdccau,
        0x36b26c30u, 0x02254727u, 0x368d8120u, 0x0e035a7fu, 0x36605773u,
        0x0650aec3u, 0x3631bffcu, 0x0a25afefu, 0x360cc431u, 0x06037c6du,
        0x35ded902u, 0x06509664u, 0x35b04f61u, 0x06255e85u, 0x358b6c68u,
        0x0a030aa5u, 0x355c66c3u, 0x0a4f93fdu, 0x352e1f21u, 0x062453feu,
        0x35097e36u, 0x0a0206a5u, 0x34d908c7u, 0x0a4daaf4u, 0x34ab366fu,
        0x0a2293d6u, 0x3486fff0u, 0x060073d6u, 0x3454ca12u, 0x0a4ae1acu,
        0x34279ec8u, 0x0a2023e7u, 0x3403f9afu, 0x09fcaee7u, 0x33cfb85fu,
        0x06474167u, 0x33a363bcu, 0x0e1d0c41u, 0x33807524u, 0x09f770eau,
        0x3349e3d5u, 0x0e42d60cu, 0x331e92b5u, 0x06195702u, 0x32f8fac7u,
        0x0a713ec4u, 0x32c35eb5u, 0x063dadedu, 0x32993aaau, 0x0215101bu,
        0x32703d59u, 0x066a2c50u, 0x323c3cf5u, 0x0a37d975u, 0x32136bd7u,
        0x0a104516u, 0x31e6cc51u, 0x06624fc5u, 0x31b493e1u, 0x06316ad5u,
        0x318d376bu, 0x060b04c7u, 0x315cc355u, 0x0a59c147u, 0x312c79acu,
        0x0e2a759fu, 0x3106af2fu, 0x0e055f07u, 0x30d23edau, 0x0a509a6au,
        0x30a4050cu, 0x06230e6du, 0x307fca71u, 0x0a7ec8c1u, 0x30475b9cu,
        0x0646f5b1u, 0x301b4ccau, 0x021b4a74u, 0x2ff1d724u, 0x0a724b83u,
        0x2fbc361eu, 0x0a3cee14u, 0x2f926764u, 0x02133f23u, 0x2f63a7e2u,
        0x02656858u, 0x2f30ea36u, 0x02329e84u, 0x2f096aaeu, 0x020b01c6u,
        0x2ed55ed5u, 0x0a5840d5u, 0x2ea59296u, 0x0a282171u, 0x2e806b7cu,
        0x0602a722u, 0x2e471c6fu, 0x064af596u, 0x2e1a4873u, 0x0e1d9063u,
        0x2deefab4u, 0x0674865bu, 0x2db8fef0u, 0x023da5bbu, 0x2d8f232cu,
        0x0e130395u, 0x2d5d6496u, 0x0a63d176u, 0x2d2b220du, 0x0a306e78u,
        0x2d043806u, 0x020891a3u, 0x2ccc3532u, 0x06535285u, 0x2c9d9eaau,
        0x0e236ab9u, 0x2c7333ecu, 0x0a7c9e87u, 0x2c3b895bu, 0x0a43294fu,
        0x2c108aa2u, 0x0616b2d7u, 0x2bdeb304u, 0x06689e19u, 0x2bab79f6u,
        0x0a33722du, 0x2b83f8a5u, 0x060a5c5bu, 0x2b4b094eu, 0x065542a6u,
        0x2b1c1bdfu, 0x0a2445cau, 0x2aeff060u, 0x0a7cf3b9u, 0x2ab84dadu,
        0x0242a818u, 0x2a8d7fe9u, 0x0a15b90cu, 0x2a592b20u, 0x026635dbu,
        0x2a2691fdu, 0x0630e58du, 0x29ff65e7u, 0x0687dd07u, 0x29c3b3ebu,
        0x0a509839u, 0x2995e33eu, 0x0a200d5eu, 0x29657c22u, 0x0a757e16u,
        0x292f973cu, 0x0a3c2e00u, 0x290649d6u, 0x06102d3fu, 0x28cd4d82u,
        0x065cd18du, 0x289cdbfeu, 0x022904b4u, 0x286f93bcu, 0x0a814e73u,
        0x2836de67u, 0x0a45c0ebu, 0x280b8405u, 0x0a172479u, 0x27d4c713u,
        0x0666ec36u, 0x27a22d35u, 0x0630523du, 0x27771927u, 0x02869086u,
        0x273c2718u, 0x024d4b0fu, 0x270f32feu, 0x0a1c85bau, 0x26d9ddc3u,
        0x026e8f29u, 0x26a5a71au, 0x0e35b53du, 0x267bc87bu, 0x028a5661u,
        0x263f4166u, 0x0a5288d5u, 0x2611353cu, 0x0a20208au, 0x25dc631au,
        0x0273754bu, 0x25a729d6u, 0x0638fcbdu, 0x257d76aeu, 0x028c7d97u,
        0x254010bfu, 0x02554a3fu, 0x25117832u, 0x0a21d3cbu, 0x24dc3fc3u,
        0x06757178u, 0x24a6a764u, 0x063a0a73u, 0x247c141fu, 0x068cf23au,
        0x243e8d91u, 0x065575bfu, 0x240ff968u, 0x06218fb3u, 0x23d974f3u,
        0x06747139u, 0x23a4246eu, 0x0e38d48fu, 0x2377ad8au, 0x0a8bb00au,
        0x233ac5c9u, 0x065309beu, 0x230cc6a1u, 0x021f56bau, 0x22d41c58u,
        0x06707de1u, 0x229fb806u, 0x02356657u, 0x22706b46u, 0x0688c2bcu,
        0x2234dbf9u, 0x0a4e1cbau, 0x2207fd08u, 0x0a1b3d7du, 0x21cc668eu,
        0x0669bbfdu, 0x21998a40u, 0x062fdf73u, 0x21668edcu, 0x06844540u,
        0x212d054eu, 0x0246dbe6u, 0x2101c779u, 0x0e156976u, 0x20c29845u,
        0x0660691fu, 0x2091d1dfu, 0x0a28720du, 0x205a6f34u, 0x067cc054u,
        0x20238672u, 0x063d887fu, 0x1ff4b805u, 0x068e0eccu, 0x1fb70647u,
        0x0a54d840u, 0x1f88d12cu, 0x0e1f5fdau, 0x1f4c739cu, 0x0a6e8e9bu,
        0x1f18afacu, 0x063273f8u, 0x1ee3f1aeu, 0x06856d4au, 0x1eaa10bbu,
        0x06476cf4u, 0x1e7da48fu, 0x0294f659u, 0x1e3d0e19u, 0x0a5e6d9au,
        0x1e0cd879u, 0x0225fb65u, 0x1dd1c1e5u, 0x0677998cu, 0x1d9c1df9u,
        0x063895e1u, 0x1d68466du, 0x06898a88u, 0x1d2cb567u, 0x0a4cdf9bu,
        0x1d005b01u, 0x0218826bu, 0x1cbeb151u, 0x0662f29au, 0x1c8d955eu,
        0x0628c6d9u, 0x1c522406u, 0x0a7ae8e3u, 0x1c1bdf21u, 0x063a6a2du,
        0x1be71f71u, 0x068a6e25u, 0x1bab44c3u, 0x064d7e87u, 0x1b7db4e2u,
        0x0a987319u, 0x1b3bd23fu, 0x066215a4u, 0x1b0afa70u, 0x02278fa9u,
        0x1acd92eeu, 0x027840abu, 0x1a97f776u, 0x0a37cfb1u, 0x1a60915eu,
        0x068807fdu, 0x1a25d8a9u, 0x0a493e73u, 0x19f4d72bu, 0x0294c97au,
        0x19b4a473u, 0x0a5be665u, 0x19853669u, 0x06226be2u, 0x19446064u,
        0x0a6fd110u, 0x1910acc4u, 0x0630f593u, 0x18d51115u, 0x06828371u,
        0x189cd187u, 0x02406c14u, 0x1866ba06u, 0x068dc782u, 0x1829a6c0u,
        0x0650d3f6u, 0x17f95d83u, 0x0699b798u, 0x17b72d99u, 0x0a6230b9u,
        0x17867e40u, 0x0a2655c1u, 0x17456640u, 0x067484a6u, 0x1710cb42u,
        0x0633a328u, 0x16d44fd2u, 0x0683e85cu, 0x169b94c1u, 0x06419ffdu,
        0x1663e849u, 0x0a8e0a3du, 0x1626d8e3u, 0x0a504b66u, 0x15f42c6cu,
        0x0698a6f6u, 0x15b294efu, 0x065fa367u, 0x15828bdcu, 0x0e23bcb5u,
        0x153ec53fu, 0x066fa4d1u, 0x150b522fu, 0x0e2f48ccu, 0x14cb653au,
        0x0680259au, 0x0u,        0x0u,
    },
    0x7fffffffu, /* _AbsMask */
    0x4120C000u, /* _MaxThreshold=643.0/64.0 */
    0x80000000u, /* sign mask */
    0x3f800000u, /* 1.0, used when _VLANG_FMA_AVAILABLE is defined */
    0x48000000u, /* SRound */
    0x2f800000u, /* _TwoM32 */
    0x1f800000u, /* 2^(-64) */
    0x03ffffffu, /* Exp_x0_Mask */
    0x7f800000u, /* ExpMask */
    0x3c800000u, /* TwoM6 = 2^(-6) */
                 // polynomial coefficients
    0xbd362fd7u, // poly1[0]
    0x3e08a294u, // poly1[1]
    0xbecd2e81u, // poly3[0]
    0x3f004465u, // poly3[1]
    0xbeaaaaa7u, // poly1[2]
    0xbeaaaa8bu, // poly3[2]
    0x3f2aaaa8u, // poly1[3]
                 // VHEX_BROADCAST(S, bf800000),  // poly1[4]
    0x4120DDFBu, /* UF_Threshold */
    0xffff8000u, /* SplitMask */
    0x5f800000u, /* 2^64 */
};               /*dErf_Table*/
float __ocl_svml_erfcf_ha(float x) {
  float r;
  unsigned int vm;
  float va1;
  float vr1;
  va1 = x;
  {
    float _AbsMask;
    float _MaxThreshold;
    float _SRound;
    float _ExpMask;
    float _Exp_X0_Mask;
    float _TwoM6;
    float _poly1_0;
    float _poly1_1;
    float _poly3_0;
    float _poly3_1;
    float _poly1_2;
    float _poly3_2;
    float _poly1_3;
    float _poly3_3;
    float _TwoM32;
    float _TwoM64;
    float _One;
    float _UF_Threshold;
    float _SplitMask;
    float X;
    float X0;
    float T;
    float Diff;
    unsigned int Index;
    float P1;
    float P3;
    float D2;
    float D3;
    float T2;
    float THL[2];
    float Erfc_E0H;
    float Exp_X0H;
    float Erfc_L;
    float Exp_X0H_Low;
    float Exp_X0H_High;
    float HighRes;
    unsigned int lErfc_E0H;
    unsigned int lExp_X0H;
    float Ph;
    float Phh;
    float LowRes;
    float Sgn;
    float _SgnMask;
    float NegConst;
    float MOne;
    float RangeMask;
    unsigned int iRangeMask;
    float NanMask;
    _AbsMask = as_float(__ocl_svml_internal_serfc_ha_data._AbsMask);
    X = as_float((as_uint(va1) & as_uint(_AbsMask)));
    // erfc(10.125) underflows to 0
    // can compute all results in the main path
    _MaxThreshold = as_float(__ocl_svml_internal_serfc_ha_data._MaxThreshold);
    X = ((X < _MaxThreshold) ? X : _MaxThreshold);
    _SgnMask = as_float(__ocl_svml_internal_serfc_ha_data._SgnMask);
    _One = as_float(__ocl_svml_internal_serfc_ha_data._One);
    Sgn = as_float((as_uint(va1) & as_uint(_SgnMask)));
    MOne = as_float((as_uint(_One) | as_uint(Sgn)));
    // 2.0 if x<0, 0.0 otherwise
    NegConst = (_One - MOne);
    _SRound = as_float(__ocl_svml_internal_serfc_ha_data._SRound);
    _TwoM32 = as_float(__ocl_svml_internal_serfc_ha_data._TwoM32);
    {
      float dIndex;
      dIndex = (X + _SRound);
      X = ((X > _TwoM32) ? X : _TwoM32);
      X0 = (dIndex - _SRound);
      Diff = (X - X0);
      T = (X0 * Diff);
      Index = as_uint(dIndex);
      Index = ((unsigned int)(Index) << (3));
    };
    _TwoM64 = as_float(__ocl_svml_internal_serfc_ha_data._TwoM64);
    // 2^(-64) with sign of input
    _TwoM64 = as_float((as_uint(_TwoM64) | as_uint(Sgn)));
    // Start polynomial evaluation
    _poly1_0 = as_float(__ocl_svml_internal_serfc_ha_data._poly1_0);
    _poly1_1 = as_float(__ocl_svml_internal_serfc_ha_data._poly1_1);
    P1 = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(_poly1_0, T, _poly1_1);
    _poly3_0 = as_float(__ocl_svml_internal_serfc_ha_data._poly3_0);
    _poly3_1 = as_float(__ocl_svml_internal_serfc_ha_data._poly3_1);
    P3 = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(_poly3_0, T, _poly3_1);
    // vector gather: erfc_h(x0), (erfc_l(x0), 2/sqrt(pi)*exp(-x0^2))

    THL[0] =
        as_float(((unsigned int *)((char *)(&__ocl_svml_internal_serfc_ha_data
                                                 ._erfc_tbl[0]) -
                                   0x40000000))[Index >> 2]);
    THL[1] =
        as_float(((unsigned int *)((char *)(&__ocl_svml_internal_serfc_ha_data
                                                 ._erfc_tbl[0]) -
                                   0x40000000))[(Index >> 2) + 1]);

    _poly1_2 = as_float(__ocl_svml_internal_serfc_ha_data._poly1_2);
    P1 = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(P1, T, _poly1_2);
    _poly3_2 = as_float(__ocl_svml_internal_serfc_ha_data._poly3_2);
    P3 = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(P3, T, _poly3_2);
    // Diff^2
    D2 = (Diff * Diff);
    // Form Erfc_L = erfc_low(x0)/(2/sqrt(pi)*exp(-x0^2))
    _Exp_X0_Mask = as_float(__ocl_svml_internal_serfc_ha_data._Exp_X0_Mask);
    Exp_X0H = as_float((as_uint(THL[1]) & as_uint(_Exp_X0_Mask)));
    {
      _TwoM6 = as_float(__ocl_svml_internal_serfc_ha_data._TwoM6);
      Erfc_L = as_float(((unsigned int)as_uint(THL[1]) >> ((32 - 6))));
      Erfc_L = as_float((as_uint(Erfc_L) | as_uint(_TwoM6)));
      Erfc_L = (Erfc_L - _TwoM6);
    };
    _poly1_3 = as_float(__ocl_svml_internal_serfc_ha_data._poly1_3);
    P1 = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(P1, T, _poly1_3);
    // P3*D
    P3 = (P3 * Diff);
    // form EXP_X0H = 2/sqrt(pi)*exp(-x0^2);
    _ExpMask = as_float(__ocl_svml_internal_serfc_ha_data._ExpMask);
    Erfc_E0H =
        as_float((as_uint(THL[0]) & as_uint(_ExpMask))); // exponent of erfc_h
    lExp_X0H = as_uint(Exp_X0H);
    lErfc_E0H = as_uint(Erfc_E0H);
    lExp_X0H = (lExp_X0H + lErfc_E0H);
    Exp_X0H = as_float(lExp_X0H);
    // P3*D2
    P3 = (P3 * Diff);
    // T^2
    T2 = (T * T);
    // get high part of erfc_high(x0)-Diff*Exp_X0H
    HighRes = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(-(Diff), Exp_X0H, THL[0]);
    ;
    // P3*D3 - Erfc_L
    P3 = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(P3, Diff, -(Erfc_L));
    // Phh
    Phh = (THL[0] - HighRes);
    // P1 = P1*T2 - T
    P1 = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(P1, T2, -(T));
    // Ph = Diff*Exp_X0H - Phh
    Ph = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(Diff, Exp_X0H, -(Phh));
    ;
    // P1 = Diff*P1 + P3
    P1 = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(P1, Diff, P3);
    // low part of result
    LowRes = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(P1, Exp_X0H, Ph);
    ;
    HighRes = (HighRes - LowRes);
    vm = 0;
    vr1 = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(HighRes, _TwoM64, NegConst);
    NanMask =
        as_float(((unsigned int)(-(signed int)((va1 != va1) | (va1 != va1)))));
    NanMask = as_float((as_uint(NanMask) & as_uint(va1)));
    vr1 = (vr1 + NanMask);
  }
  r = vr1;
  return r;
}

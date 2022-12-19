/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF
typedef struct
{

    unsigned long _dTp_h[(1 << 4)];
    unsigned long _dTp_l[(1 << 4)];
    unsigned long _dTn_h[(1 << 4)];

    unsigned long _dbShifter_UISA;
    unsigned long _lIndexMask_UISA;
    unsigned long _dPC2_UISA;
    unsigned long _dPC3_UISA;
    unsigned long _dPC4_UISA;
    unsigned long _dPC5_UISA;
    unsigned long _dPC6_UISA;
    unsigned long _dPC7_UISA;

    unsigned long _dbT[(1 + (1 << 8))];

    unsigned long _dbInvLn2;
    unsigned long _dbLn2hi;
    unsigned long _dbLn2lo;
    unsigned long _dbShifter;
    unsigned int _iIndexMask;

    unsigned long _dPC2;
    unsigned long _dPC3;
    unsigned long _dPC4;
    unsigned int _iMaxIndex;

    unsigned long _lExpMask;
    unsigned long _dSign;
    unsigned int _iDomainRange;
} __internal_dcosh_la_data_t;
static __constant __internal_dcosh_la_data_t __internal_dcosh_la_data = {
    {
     0x3fe0000000000000uL, 0x3fe0b5586cf9890fuL, 0x3fe172b83c7d517buL, 0x3fe2387a6e756238uL,
     0x3fe306fe0a31b715uL, 0x3fe3dea64c123422uL, 0x3fe4bfdad5362a27uL, 0x3fe5ab07dd485429uL,
     0x3fe6a09e667f3bcduL, 0x3fe7a11473eb0187uL, 0x3fe8ace5422aa0dbuL, 0x3fe9c49182a3f090uL,
     0x3feae89f995ad3aduL, 0x3fec199bdd85529cuL, 0x3fed5818dcfba487uL, 0x3feea4afa2a490dauL},
    {
     0x0000000000000000uL, 0x3c88a62e4adc610buL, 0xbc719041b9d78a76uL, 0x3c89b07eb6c70573uL,
     0x3c76f46ad23182e4uL, 0x3c7ada0911f09ebcuL, 0x3c6d4397afec42e2uL, 0x3c86324c054647aduL,
     0xbc8bdd3413b26456uL, 0xbc741577ee04992fuL, 0x3c86e9f156864b27uL, 0x3c6c7c46b071f2beuL,
     0x3c87a1cd345dcc81uL, 0x3c711065895048dduL, 0x3c72ed02d75b3707uL, 0xbc8e9c23179c2893uL},
    {
     0x3fe0000000000000uL, 0x3fdea4afa2a490dauL, 0x3fdd5818dcfba487uL, 0x3fdc199bdd85529cuL,
     0x3fdae89f995ad3aduL, 0x3fd9c49182a3f090uL, 0x3fd8ace5422aa0dbuL, 0x3fd7a11473eb0187uL,
     0x3fd6a09e667f3bcduL, 0x3fd5ab07dd485429uL, 0x3fd4bfdad5362a27uL, 0x3fd3dea64c123422uL,
     0x3fd306fe0a31b715uL, 0x3fd2387a6e756238uL, 0x3fd172b83c7d517buL, 0x3fd0b5586cf9890fuL},
    0x42F8000000000000uL,
    0x000000000000000FuL,
    0x3fe0000000000004uL,
    0x3fc5555555555543uL,
    0x3fa5555555484f37uL,
    0x3f81111111286a0cuL,
    0x3f56c183da08f116uL,
    0x3f2a018d76da03dauL,

    {
     0x3fe0000000000000uL, 0x3fe00b1afa5abcbfuL, 0x3fe0163da9fb3335uL, 0x3fe02168143b0281uL,
     0x3fe02c9a3e778061uL, 0x3fe037d42e11bbccuL, 0x3fe04315e86e7f85uL, 0x3fe04e5f72f654b1uL,
     0x3fe059b0d3158574uL, 0x3fe0650a0e3c1f89uL, 0x3fe0706b29ddf6deuL, 0x3fe07bd42b72a836uL,
     0x3fe0874518759bc8uL, 0x3fe092bdf66607e0uL, 0x3fe09e3ecac6f383uL, 0x3fe0a9c79b1f3919uL,
     0x3fe0b5586cf9890fuL, 0x3fe0c0f145e46c85uL, 0x3fe0cc922b7247f7uL, 0x3fe0d83b23395decuL,
     0x3fe0e3ec32d3d1a2uL, 0x3fe0efa55fdfa9c5uL, 0x3fe0fb66affed31buL, 0x3fe1073028d7233euL,
     0x3fe11301d0125b51uL, 0x3fe11edbab5e2ab6uL, 0x3fe12abdc06c31ccuL, 0x3fe136a814f204abuL,
     0x3fe1429aaea92de0uL, 0x3fe14e95934f312euL, 0x3fe15a98c8a58e51uL, 0x3fe166a45471c3c2uL,
     0x3fe172b83c7d517buL, 0x3fe17ed48695bbc0uL, 0x3fe18af9388c8deauL, 0x3fe1972658375d2fuL,
     0x3fe1a35beb6fcb75uL, 0x3fe1af99f8138a1cuL, 0x3fe1bbe084045cd4uL, 0x3fe1c82f95281c6buL,
     0x3fe1d4873168b9aauL, 0x3fe1e0e75eb44027uL, 0x3fe1ed5022fcd91duL, 0x3fe1f9c18438ce4duL,
     0x3fe2063b88628cd6uL, 0x3fe212be3578a819uL, 0x3fe21f49917ddc96uL, 0x3fe22bdda27912d1uL,
     0x3fe2387a6e756238uL, 0x3fe2451ffb82140auL, 0x3fe251ce4fb2a63fuL, 0x3fe25e85711ece75uL,
     0x3fe26b4565e27cdduL, 0x3fe2780e341ddf29uL, 0x3fe284dfe1f56381uL, 0x3fe291ba7591bb70uL,
     0x3fe29e9df51fdee1uL, 0x3fe2ab8a66d10f13uL, 0x3fe2b87fd0dad990uL, 0x3fe2c57e39771b2fuL,
     0x3fe2d285a6e4030buL, 0x3fe2df961f641589uL, 0x3fe2ecafa93e2f56uL, 0x3fe2f9d24abd886buL,
     0x3fe306fe0a31b715uL, 0x3fe31432edeeb2fduL, 0x3fe32170fc4cd831uL, 0x3fe32eb83ba8ea32uL,
     0x3fe33c08b26416ffuL, 0x3fe3496266e3fa2duL, 0x3fe356c55f929ff1uL, 0x3fe36431a2de883buL,
     0x3fe371a7373aa9cbuL, 0x3fe37f26231e754auL, 0x3fe38cae6d05d866uL, 0x3fe39a401b7140efuL,
     0x3fe3a7db34e59ff7uL, 0x3fe3b57fbfec6cf4uL, 0x3fe3c32dc313a8e5uL, 0x3fe3d0e544ede173uL,
     0x3fe3dea64c123422uL, 0x3fe3ec70df1c5175uL, 0x3fe3fa4504ac801cuL, 0x3fe40822c367a024uL,
     0x3fe4160a21f72e2auL, 0x3fe423fb2709468auL, 0x3fe431f5d950a897uL, 0x3fe43ffa3f84b9d4uL,
     0x3fe44e086061892duL, 0x3fe45c2042a7d232uL, 0x3fe46a41ed1d0057uL, 0x3fe4786d668b3237uL,
     0x3fe486a2b5c13cd0uL, 0x3fe494e1e192aed2uL, 0x3fe4a32af0d7d3deuL, 0x3fe4b17dea6db7d7uL,
     0x3fe4bfdad5362a27uL, 0x3fe4ce41b817c114uL, 0x3fe4dcb299fddd0duL, 0x3fe4eb2d81d8abffuL,
     0x3fe4f9b2769d2ca7uL, 0x3fe508417f4531eeuL, 0x3fe516daa2cf6642uL, 0x3fe5257de83f4eefuL,
     0x3fe5342b569d4f82uL, 0x3fe542e2f4f6ad27uL, 0x3fe551a4ca5d920fuL, 0x3fe56070dde910d2uL,
     0x3fe56f4736b527dauL, 0x3fe57e27dbe2c4cfuL, 0x3fe58d12d497c7fduL, 0x3fe59c0827ff07ccuL,
     0x3fe5ab07dd485429uL, 0x3fe5ba11fba87a03uL, 0x3fe5c9268a5946b7uL, 0x3fe5d84590998b93uL,
     0x3fe5e76f15ad2148uL, 0x3fe5f6a320dceb71uL, 0x3fe605e1b976dc09uL, 0x3fe6152ae6cdf6f4uL,
     0x3fe6247eb03a5585uL, 0x3fe633dd1d1929fduL, 0x3fe6434634ccc320uL, 0x3fe652b9febc8fb7uL,
     0x3fe6623882552225uL, 0x3fe671c1c70833f6uL, 0x3fe68155d44ca973uL, 0x3fe690f4b19e9538uL,
     0x3fe6a09e667f3bcduL, 0x3fe6b052fa75173euL, 0x3fe6c012750bdabfuL, 0x3fe6cfdcddd47645uL,
     0x3fe6dfb23c651a2fuL, 0x3fe6ef9298593ae5uL, 0x3fe6ff7df9519484uL, 0x3fe70f7466f42e87uL,
     0x3fe71f75e8ec5f74uL, 0x3fe72f8286ead08auL, 0x3fe73f9a48a58174uL, 0x3fe74fbd35d7cbfduL,
     0x3fe75feb564267c9uL, 0x3fe77024b1ab6e09uL, 0x3fe780694fde5d3fuL, 0x3fe790b938ac1cf6uL,
     0x3fe7a11473eb0187uL, 0x3fe7b17b0976cfdbuL, 0x3fe7c1ed0130c132uL, 0x3fe7d26a62ff86f0uL,
     0x3fe7e2f336cf4e62uL, 0x3fe7f3878491c491uL, 0x3fe80427543e1a12uL, 0x3fe814d2add106d9uL,
     0x3fe82589994cce13uL, 0x3fe8364c1eb941f7uL, 0x3fe8471a4623c7aduL, 0x3fe857f4179f5b21uL,
     0x3fe868d99b4492eduL, 0x3fe879cad931a436uL, 0x3fe88ac7d98a6699uL, 0x3fe89bd0a478580fuL,
     0x3fe8ace5422aa0dbuL, 0x3fe8be05bad61778uL, 0x3fe8cf3216b5448cuL, 0x3fe8e06a5e0866d9uL,
     0x3fe8f1ae99157736uL, 0x3fe902fed0282c8auL, 0x3fe9145b0b91ffc6uL, 0x3fe925c353aa2fe2uL,
     0x3fe93737b0cdc5e5uL, 0x3fe948b82b5f98e5uL, 0x3fe95a44cbc8520fuL, 0x3fe96bdd9a7670b3uL,
     0x3fe97d829fde4e50uL, 0x3fe98f33e47a22a2uL, 0x3fe9a0f170ca07bauL, 0x3fe9b2bb4d53fe0duL,
     0x3fe9c49182a3f090uL, 0x3fe9d674194bb8d5uL, 0x3fe9e86319e32323uL, 0x3fe9fa5e8d07f29euL,
     0x3fea0c667b5de565uL, 0x3fea1e7aed8eb8bbuL, 0x3fea309bec4a2d33uL, 0x3fea42c980460ad8uL,
     0x3fea5503b23e255duL, 0x3fea674a8af46052uL, 0x3fea799e1330b358uL, 0x3fea8bfe53c12e59uL,
     0x3fea9e6b5579fdbfuL, 0x3feab0e521356ebauL, 0x3feac36bbfd3f37auL, 0x3fead5ff3a3c2774uL,
     0x3feae89f995ad3aduL, 0x3feafb4ce622f2ffuL, 0x3feb0e07298db666uL, 0x3feb20ce6c9a8952uL,
     0x3feb33a2b84f15fbuL, 0x3feb468415b749b1uL, 0x3feb59728de5593auL, 0x3feb6c6e29f1c52auL,
     0x3feb7f76f2fb5e47uL, 0x3feb928cf22749e4uL, 0x3feba5b030a1064auL, 0x3febb8e0b79a6f1fuL,
     0x3febcc1e904bc1d2uL, 0x3febdf69c3f3a207uL, 0x3febf2c25bd71e09uL, 0x3fec06286141b33duL,
     0x3fec199bdd85529cuL, 0x3fec2d1cd9fa652cuL, 0x3fec40ab5fffd07auL, 0x3fec544778fafb22uL,
     0x3fec67f12e57d14buL, 0x3fec7ba88988c933uL, 0x3fec8f6d9406e7b5uL, 0x3feca3405751c4dbuL,
     0x3fecb720dcef9069uL, 0x3feccb0f2e6d1675uL, 0x3fecdf0b555dc3fauL, 0x3fecf3155b5bab74uL,
     0x3fed072d4a07897cuL, 0x3fed1b532b08c968uL, 0x3fed2f87080d89f2uL, 0x3fed43c8eacaa1d6uL,
     0x3fed5818dcfba487uL, 0x3fed6c76e862e6d3uL, 0x3fed80e316c98398uL, 0x3fed955d71ff6075uL,
     0x3feda9e603db3285uL, 0x3fedbe7cd63a8315uL, 0x3fedd321f301b460uL, 0x3fede7d5641c0658uL,
     0x3fedfc97337b9b5fuL, 0x3fee11676b197d17uL, 0x3fee264614f5a129uL, 0x3fee3b333b16ee12uL,
     0x3fee502ee78b3ff6uL, 0x3fee653924676d76uL, 0x3fee7a51fbc74c83uL, 0x3fee8f7977cdb740uL,
     0x3feea4afa2a490dauL, 0x3feeb9f4867cca6euL, 0x3feecf482d8e67f1uL, 0x3feee4aaa2188510uL,
     0x3feefa1bee615a27uL, 0x3fef0f9c1cb6412auL, 0x3fef252b376bba97uL, 0x3fef3ac948dd7274uL,
     0x3fef50765b6e4540uL, 0x3fef6632798844f8uL, 0x3fef7bfdad9cbe14uL, 0x3fef91d802243c89uL,
     0x3fefa7c1819e90d8uL, 0x3fefbdba3692d514uL, 0x3fefd3c22b8f71f1uL, 0x3fefe9d96b2a23d9uL,
     0x3ff0000000000000uL},
    0x3ff71547652b82feuL,
    0x3FE62E42FEFC0000uL,
    0xBDAC610CA86C3899uL,
    0x42B8000000000000uL,
    0x000000FFu,
    0x3FDFFFFFFFFFFDBDuL,
    0x3FC5555570813E14uL,
    0x3FA55555CF16D299uL,
    (1 << 8),

    0x7ff0000000000000uL,
    0x8000000000000000uL,
    0x40861d99u
};

static __constant _iml_v2_dp_union_t __dcosh_la_CoutTab[144] = {

    0x00000000, 0x3FF00000,
    0x00000000, 0x00000000,
    0x3E778061, 0x3FF02C9A,
    0x535B085D, 0xBC719083,
    0xD3158574, 0x3FF059B0,
    0xA475B465, 0x3C8D73E2,
    0x18759BC8, 0x3FF08745,
    0x4BB284FF, 0x3C6186BE,
    0x6CF9890F, 0x3FF0B558,
    0x4ADC610B, 0x3C98A62E,
    0x32D3D1A2, 0x3FF0E3EC,
    0x27C57B53, 0x3C403A17,
    0xD0125B51, 0x3FF11301,
    0x39449B3A, 0xBC96C510,
    0xAEA92DE0, 0x3FF1429A,
    0x9AF1369E, 0xBC932FBF,
    0x3C7D517B, 0x3FF172B8,
    0xB9D78A76, 0xBC819041,
    0xEB6FCB75, 0x3FF1A35B,
    0x7B4968E4, 0x3C8E5B4C,
    0x3168B9AA, 0x3FF1D487,
    0x00A2643C, 0x3C9E016E,
    0x88628CD6, 0x3FF2063B,
    0x814A8495, 0x3C8DC775,
    0x6E756238, 0x3FF2387A,
    0xB6C70573, 0x3C99B07E,
    0x65E27CDD, 0x3FF26B45,
    0x9940E9D9, 0x3C82BD33,
    0xF51FDEE1, 0x3FF29E9D,
    0xAFAD1255, 0x3C8612E8,
    0xA6E4030B, 0x3FF2D285,
    0x54DB41D5, 0x3C900247,
    0x0A31B715, 0x3FF306FE,
    0xD23182E4, 0x3C86F46A,
    0xB26416FF, 0x3FF33C08,
    0x843659A6, 0x3C932721,
    0x373AA9CB, 0x3FF371A7,
    0xBF42EAE2, 0xBC963AEA,
    0x34E59FF7, 0x3FF3A7DB,
    0xD661F5E3, 0xBC75E436,
    0x4C123422, 0x3FF3DEA6,
    0x11F09EBC, 0x3C8ADA09,
    0x21F72E2A, 0x3FF4160A,
    0x1C309278, 0xBC5EF369,
    0x6061892D, 0x3FF44E08,
    0x04EF80D0, 0x3C489B7A,
    0xB5C13CD0, 0x3FF486A2,
    0xB69062F0, 0x3C73C1A3,
    0xD5362A27, 0x3FF4BFDA,
    0xAFEC42E2, 0x3C7D4397,
    0x769D2CA7, 0x3FF4F9B2,
    0xD25957E3, 0xBC94B309,
    0x569D4F82, 0x3FF5342B,
    0x1DB13CAD, 0xBC807ABE,
    0x36B527DA, 0x3FF56F47,
    0x011D93AD, 0x3C99BB2C,
    0xDD485429, 0x3FF5AB07,
    0x054647AD, 0x3C96324C,
    0x15AD2148, 0x3FF5E76F,
    0x3080E65E, 0x3C9BA6F9,
    0xB03A5585, 0x3FF6247E,
    0x7E40B497, 0xBC9383C1,
    0x82552225, 0x3FF66238,
    0x87591C34, 0xBC9BB609,
    0x667F3BCD, 0x3FF6A09E,
    0x13B26456, 0xBC9BDD34,
    0x3C651A2F, 0x3FF6DFB2,
    0x683C88AB, 0xBC6BBE3A,
    0xE8EC5F74, 0x3FF71F75,
    0x86887A99, 0xBC816E47,
    0x564267C9, 0x3FF75FEB,
    0x57316DD3, 0xBC902459,
    0x73EB0187, 0x3FF7A114,
    0xEE04992F, 0xBC841577,
    0x36CF4E62, 0x3FF7E2F3,
    0xBA15797E, 0x3C705D02,
    0x994CCE13, 0x3FF82589,
    0xD41532D8, 0xBC9D4C1D,
    0x9B4492ED, 0x3FF868D9,
    0x9BD4F6BA, 0xBC9FC6F8,
    0x422AA0DB, 0x3FF8ACE5,
    0x56864B27, 0x3C96E9F1,
    0x99157736, 0x3FF8F1AE,
    0xA2E3976C, 0x3C85CC13,
    0xB0CDC5E5, 0x3FF93737,
    0x81B57EBC, 0xBC675FC7,
    0x9FDE4E50, 0x3FF97D82,
    0x7C1B85D1, 0xBC9D185B,
    0x82A3F090, 0x3FF9C491,
    0xB071F2BE, 0x3C7C7C46,
    0x7B5DE565, 0x3FFA0C66,
    0x5D1CD533, 0xBC935949,
    0xB23E255D, 0x3FFA5503,
    0xDB8D41E1, 0xBC9D2F6E,
    0x5579FDBF, 0x3FFA9E6B,
    0x0EF7FD31, 0x3C90FAC9,
    0x995AD3AD, 0x3FFAE89F,
    0x345DCC81, 0x3C97A1CD,
    0xB84F15FB, 0x3FFB33A2,
    0x3084D708, 0xBC62805E,
    0xF2FB5E47, 0x3FFB7F76,
    0x7E54AC3B, 0xBC75584F,
    0x904BC1D2, 0x3FFBCC1E,
    0x7A2D9E84, 0x3C823DD0,
    0xDD85529C, 0x3FFC199B,
    0x895048DD, 0x3C811065,
    0x2E57D14B, 0x3FFC67F1,
    0xFF483CAD, 0x3C92884D,
    0xDCEF9069, 0x3FFCB720,
    0xD1E949DB, 0x3C7503CB,
    0x4A07897C, 0x3FFD072D,
    0x43797A9C, 0xBC9CBC37,
    0xDCFBA487, 0x3FFD5818,
    0xD75B3707, 0x3C82ED02,
    0x03DB3285, 0x3FFDA9E6,
    0x696DB532, 0x3C9C2300,
    0x337B9B5F, 0x3FFDFC97,
    0x4F184B5C, 0xBC91A5CD,
    0xE78B3FF6, 0x3FFE502E,
    0x80A9CC8F, 0x3C839E89,
    0xA2A490DA, 0x3FFEA4AF,
    0x179C2893, 0xBC9E9C23,
    0xEE615A27, 0x3FFEFA1B,
    0x86A4B6B0, 0x3C9DC7F4,
    0x5B6E4540, 0x3FFF5076,
    0x2DD8A18B, 0x3C99D3E1,
    0x819E90D8, 0x3FFFA7C1,
    0xF3A5931E, 0x3C874853,
    0x00000000, 0x40000000,
    0x00000000, 0x00000000,

    0x652B82FE, 0x40571547,

    0x00000000, 0x43380000,

    0x00000000, 0x3FE00000,
    0x555548F8, 0x3FC55555,
    0x55558FCC, 0x3FA55555,
    0x3AAF20D3, 0x3F811112,
    0x1C2A3FFD, 0x3F56C16A,

    0x8FB9F87E, 0x408633CE,

    0xFEFA0000, 0x3F862E42,
    0xBC9E3B3A, 0x3D1CF79A,

    0xFFFFFFFF, 0x7FEFFFFF,

    0x00000000, 0x00000000,
    0x00000000, 0x3FF00000,

    0xE7026820, 0x40357CD0
};

__attribute__((always_inline))
inline int __internal_dcosh_la_cout (double *a, double *r)
{

    double M, Nj;
    double R, RHi, RLo, scale;
    double p;
    double absAi;
    double rsq, podd, peven;
    double TpHi, TpLo, TmHi, TmLo;
    double TdH, TsH, TsL;
    double tmp;
    double vtmp1, vtmp2;
    _iml_uint32_t N, j;
    int nRet = 0;
    scale = ((__constant double *) __dcosh_la_CoutTab)[141];

    if ((((((_iml_v2_dp_union_t *) & (*a))->dwords.hi_dword >> 20) & 0x7FF) != 0x7FF))
    {

        absAi = (*a);
        (((_iml_v2_dp_union_t *) & absAi)->dwords.hi_dword =
         (((_iml_v2_dp_union_t *) & absAi)->dwords.hi_dword & 0x7FFFFFFF) | ((_iml_uint32_t) (0) << 31));

        if (((((_iml_v2_dp_union_t *) & absAi)->dwords.hi_dword >> 20) & 0x7FF) > (0x3FF - 54))
        {

            if (absAi < ((__constant double *) __dcosh_la_CoutTab)[137])
            {

                if (absAi >= ((__constant double *) __dcosh_la_CoutTab)[143])
                {

                    tmp = absAi * ((__constant double *) __dcosh_la_CoutTab)[130];
                    Nj = (tmp + ((__constant double *) __dcosh_la_CoutTab)[131]);
                    M = (Nj - ((__constant double *) __dcosh_la_CoutTab)[131]);
                    RHi = (M * ((__constant double *) __dcosh_la_CoutTab)[138]);
                    RLo = (M * ((__constant double *) __dcosh_la_CoutTab)[139]);
                    R = (absAi - RHi);
                    R = (R - RLo);

                    p = (((((((__constant double *) __dcosh_la_CoutTab)[136] * R + ((__constant double *) __dcosh_la_CoutTab)[135]) * R +
                            ((__constant double *) __dcosh_la_CoutTab)[134]) * R + ((__constant double *) __dcosh_la_CoutTab)[133]) * R +
                          ((__constant double *) __dcosh_la_CoutTab)[132]) * R);
                    p = (p * R + R);

                    N = (((_iml_v2_dp_union_t *) & Nj)->dwords.lo_dword);
                    j = N & ((1 << 6) - 1);
                    N = (N >> 6);
                    N = (N + 0x3FF);

                    p = ((__constant double *) __dcosh_la_CoutTab)[2 * (j)] * p;
                    p = (p + ((__constant double *) __dcosh_la_CoutTab)[2 * (j) + 1]);
                    p = (p + ((__constant double *) __dcosh_la_CoutTab)[2 * (j)]);

                    N = ((N - 1) & 0x7FF);

                    if (N <= (0x7FF - 1))
                    {

                        (((_iml_v2_dp_union_t *) & scale)->dwords.hi_dword =
                         (((_iml_v2_dp_union_t *) & scale)->dwords.hi_dword & 0x800FFFFF) | (((_iml_uint32_t) (N) & 0x7FF) << 20));

                        (*r) = scale * p;
                    }
                    else
                    {

                        (((_iml_v2_dp_union_t *) & scale)->dwords.hi_dword =
                         (((_iml_v2_dp_union_t *) & scale)->dwords.hi_dword & 0x800FFFFF) | (((_iml_uint32_t) (N - 1) & 0x7FF) << 20));

                        p = (p * scale);
                        (*r) = ((__constant double *) __dcosh_la_CoutTab)[128] * p;
                    }
                }
                else
                {

                    tmp = absAi * ((__constant double *) __dcosh_la_CoutTab)[130];
                    Nj = (tmp + ((__constant double *) __dcosh_la_CoutTab)[131]);
                    M = (Nj - ((__constant double *) __dcosh_la_CoutTab)[131]);
                    RHi = M * ((__constant double *) __dcosh_la_CoutTab)[138];
                    RLo = M * ((__constant double *) __dcosh_la_CoutTab)[139];
                    R = (absAi - RHi);
                    R = (R - RLo);

                    rsq = (R * R);
                    podd = (((((__constant double *) __dcosh_la_CoutTab)[135] * rsq + ((__constant double *) __dcosh_la_CoutTab)[133]) * rsq) * R);
                    peven =
                        ((((((__constant double *) __dcosh_la_CoutTab)[136] * rsq + ((__constant double *) __dcosh_la_CoutTab)[134]) * rsq) +
                          ((__constant double *) __dcosh_la_CoutTab)[132]) * rsq);

                    N = (((_iml_v2_dp_union_t *) & Nj)->dwords.lo_dword);
                    j = N & ((1 << 6) - 1);
                    N = (N >> 6);

                    N = (N + 0x3FF);
                    N = N & 0x7FF;

                    (((_iml_v2_dp_union_t *) & scale)->dwords.hi_dword =
                     (((_iml_v2_dp_union_t *) & scale)->dwords.hi_dword & 0x800FFFFF) | (((_iml_uint32_t) (N - 1) & 0x7FF) << 20));

                    TpHi = ((__constant double *) __dcosh_la_CoutTab)[2 * (j)] * scale;
                    TpLo = ((__constant double *) __dcosh_la_CoutTab)[2 * (j) + 1] * scale;

                    N = 2 * 0x3FF - N;
                    (((_iml_v2_dp_union_t *) & scale)->dwords.hi_dword =
                     (((_iml_v2_dp_union_t *) & scale)->dwords.hi_dword & 0x800FFFFF) | (((_iml_uint32_t) (N - 2) & 0x7FF) << 20));

                    TmHi = ((__constant double *) __dcosh_la_CoutTab)[2 * (64 - (j))] * scale;
                    TmLo = ((__constant double *) __dcosh_la_CoutTab)[2 * (64 - (j)) + 1] * scale;

                    vtmp1 = ((TpHi) + (TmHi));
                    tmp = ((TpHi) - vtmp1);
                    vtmp2 = (tmp + (TmHi));
                    TsH = vtmp1;
                    TsL = vtmp2;;

                    TdH = (TpHi - TmHi);

                    p = (R * (TpLo - TmLo));
                    p += TmLo;
                    p += (TpLo);
                    p += (TsL);
                    p += (podd * TdH);
                    p += (peven * TsH);
                    p += (R * TdH);

                    vtmp1 = p;
                    p = vtmp1 + TsH;

                    (*r) = p;
                }
            }
            else
            {

                (*r) = ((__constant double *) __dcosh_la_CoutTab)[140] * ((__constant double *) __dcosh_la_CoutTab)[140];

                nRet = 3;
            }
        }
        else
        {

            (*r) = ((__constant double *) __dcosh_la_CoutTab)[142] + absAi;
        }
    }
    else
    {

        (*r) = (*a) * (*a);
    }

    return nRet;

}

double __ocl_svml_cosh (double a)
{

    double va1;
    double vr1;
    unsigned int vm;

    double r;

    va1 = a;;

    {

        double dN;
        double dM;
        double dR;
        double dR2;
        double dSinh_r;
        double dOut;
        double dTp;
        double dTn;
        unsigned long lM;
        double dXSign;
        unsigned int iAbsX;
        double dAbsX;

        unsigned int iIndex;
        unsigned long lX;
        unsigned int iRangeMask;

        double dbInvLn2;
        double dbShifter;
        double dbLn2[2];
        double dPC[3];
        unsigned int iMaxIndex;
        unsigned long lExpMask;
        unsigned int iIndexMask;
        unsigned int iDomainRange;

        dbInvLn2 = as_double (__internal_dcosh_la_data._dbInvLn2);
        dbShifter = as_double (__internal_dcosh_la_data._dbShifter);
        iIndexMask = (__internal_dcosh_la_data._iIndexMask);
        dPC[0] = as_double (__internal_dcosh_la_data._dPC2);
        dPC[1] = as_double (__internal_dcosh_la_data._dPC3);
        dPC[2] = as_double (__internal_dcosh_la_data._dPC4);
        iMaxIndex = (__internal_dcosh_la_data._iMaxIndex);
        dXSign = as_double (__internal_dcosh_la_data._dSign);
        iDomainRange = (__internal_dcosh_la_data._iDomainRange);
        lExpMask = (__internal_dcosh_la_data._lExpMask);

        dAbsX = as_double ((~(as_ulong (dXSign)) & as_ulong (va1)));
        dXSign = as_double (((unsigned long) as_ulong (dXSign) >> (11)));

        dM = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dAbsX, dbInvLn2, dbShifter);

        lX = as_ulong (dAbsX);
        iAbsX = ((unsigned int) ((unsigned long) lX >> 32));
        iRangeMask = ((unsigned int) (-(signed int) ((signed int) iAbsX > (signed int) iDomainRange)));
        vm = 0;
        vm = iRangeMask;

        lM = as_ulong (dM);
        iIndex = (((unsigned int) lM & (unsigned int) -1));
        iIndex = (iIndex & iIndexMask);

        iDomainRange = (iMaxIndex - iIndex);
        iIndex = ((unsigned int) (iIndex) << (3));
        iDomainRange = ((unsigned int) (iDomainRange) << (3));
        dTp = as_double (((__constant unsigned long *) (__internal_dcosh_la_data._dbT))[iIndex >> 3]);
        dTn = as_double (((__constant unsigned long *) (__internal_dcosh_la_data._dbT))[iDomainRange >> 3]);

        dN = (dM - dbShifter);
        dbLn2[0] = as_double (__internal_dcosh_la_data._dbLn2hi);
        dR = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (-(dbLn2[0]), dN, dAbsX);
        dbLn2[1] = as_double (__internal_dcosh_la_data._dbLn2lo);
        dR = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (-(dbLn2[1]), dN, dR);
        dR2 = (dR * dR);

        lM = ((unsigned long) (lM) << ((52 - 8)));
        lM = (lM & lExpMask);

        lX = as_ulong (dTp);
        lX = (lX + lM);
        dTp = as_double (lX);

        lX = as_ulong (dTn);
        lX = (lX - lM);
        lM = as_ulong (dXSign);
        lX = (lX - lM);
        dM = as_double (lX);

        dTn = (dTp - dM);
        dTp = (dTp + dM);

        dSinh_r = (dPC[1] * dR2);
        dSinh_r = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dSinh_r, dR, dR);
        dSinh_r = (dSinh_r * dTn);

        dOut = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dPC[2], dR2, dPC[0]);
        dOut = (dOut * dR2);
        dOut = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dOut, dTp, dSinh_r);
        vr1 = (dOut + dTp);
    }

    if ((vm) != 0)
    {
        double _vapi_arg1[1];
        double _vapi_res1[1];
        ((double *) _vapi_arg1)[0] = va1;
        ((double *) _vapi_res1)[0] = vr1;
        __internal_dcosh_la_cout (_vapi_arg1, _vapi_res1);
        vr1 = ((double *) _vapi_res1)[0];
    };
    r = vr1;;

    return r;

}

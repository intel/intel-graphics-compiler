/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF
typedef struct
{

    unsigned long _dExp_tbl_PH[16];
    unsigned long _dExp_tbl_PL[16];
    unsigned long _dExp_tbl_NH[16];
    unsigned long _dExp_tbl_NL[16];
    unsigned long _dbShifter_UISA;
    unsigned long _dbShifterP1_UISA;
    unsigned int _iDomainRange_UISA;
    unsigned long _lIndexMask_UISA;
    unsigned long _dPC2_UISA;
    unsigned long _dPC3_UISA;
    unsigned long _dPC4_UISA;
    unsigned long _dPC5_UISA;
    unsigned long _dPC6_UISA;
    unsigned long _dPC7_UISA;
    unsigned long _dPC8_UISA;

    unsigned long _dbInvLn2;
    unsigned long _dbLn2hi;
    unsigned long _dbLn2lo;
    unsigned long _dSign;
    unsigned long _dHalf;
    unsigned long _dZero;

    unsigned long _dbT[(1 << 7)][2];
    unsigned long _dbShifter;
    unsigned int _iDomainRange;

    unsigned long _dPC1;
    unsigned long _dPC2;
    unsigned long _dPC3;
    unsigned long _dPC4;
    unsigned long _dPC5;

    unsigned long _lExpMask;
    unsigned long _lBias;
    unsigned long _lIndexMask;

} __internal_dsinh_la_data_t;
static __constant __internal_dsinh_la_data_t __internal_dsinh_la_data = {
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
    {
     0x0000000000000000uL, 0xbc7e9c23179c2893uL, 0x3c62ed02d75b3707uL, 0x3c611065895048dduL,
     0x3c77a1cd345dcc81uL, 0x3c5c7c46b071f2beuL, 0x3c76e9f156864b27uL, 0xbc641577ee04992fuL,
     0xbc7bdd3413b26456uL, 0x3c76324c054647aduL, 0x3c5d4397afec42e2uL, 0x3c6ada0911f09ebcuL,
     0x3c66f46ad23182e4uL, 0x3c79b07eb6c70573uL, 0xbc619041b9d78a76uL, 0x3c78a62e4adc610buL},
    0x42F8000000000000uL,
    0x42F8000000000001uL,
    0x4084ee33u,
    0x000000000000000FuL,
    0x3fe0000000000000uL,
    0x3fc5555555555ca6uL,
    0x3fa5555555554b4cuL,
    0x3f8111110edc6edauL,
    0x3f56c16c1902580buL,
    0x3f2a026d06015ec6uL,
    0x3efa01ac1b806259uL,

    0x3FF71547652B82FEuL,
    0x3FE62E42FEFA0000uL,
    0x3D7CF79ABC9E3B3AuL,
    0x8000000000000000uL,
    0x3FE0000000000000uL,
    0x0000000000000000uL,
    {
     {0x0000000000000000uL, 0x3FE0000000000000uL},
     {0x3F762E4A19BD1E74uL, 0x3FDFD3C22B8F71F1uL},
     {0x3F862E5F6A0DFD36uL, 0x3FDFA7C1819E90D8uL},
     {0x3F90A2E234040F5FuL, 0x3FDF7BFDAD9CBE14uL},
     {0x3F962EB4ABCC5A81uL, 0x3FDF50765B6E4540uL},
     {0x3F9BBAB1C5033244uL, 0x3FDF252B376BBA97uL},
     {0x3FA0A372144EEB45uL, 0x3FDEFA1BEE615A27uL},
     {0x3FA369AB3FFBF8B0uL, 0x3FDECF482D8E67F1uL},
     {0x3FA63009BA740A2AuL, 0x3FDEA4AFA2A490DAuL},
     {0x3FA8F692D8EA1B5AuL, 0x3FDE7A51FBC74C83uL},
     {0x3FABBD4BF0E31A6FuL, 0x3FDE502EE78B3FF6uL},
     {0x3FAE843A5840286AuL, 0x3FDE264614F5A129uL},
     {0x3FB0A5B1B2A46D0AuL, 0x3FDDFC97337B9B5FuL},
     {0x3FB20966375ABCDFuL, 0x3FDDD321F301B460uL},
     {0x3FB36D3D65DCA4E8uL, 0x3FDDA9E603DB3285uL},
     {0x3FB4D139EA06642AuL, 0x3FDD80E316C98398uL},
     {0x3FB6355E6FFBF9BAuL, 0x3FDD5818DCFBA487uL},
     {0x3FB799ADA42E4788uL, 0x3FDD2F87080D89F2uL},
     {0x3FB8FE2A336035BCuL, 0x3FDD072D4A07897CuL},
     {0x3FBA62D6CAABD6B6uL, 0x3FDCDF0B555DC3FAuL},
     {0x3FBBC7B617878BAFuL, 0x3FDCB720DCEF9069uL},
     {0x3FBD2CCAC7CB2A11uL, 0x3FDC8F6D9406E7B5uL},
     {0x3FBE921789B52185uL, 0x3FDC67F12E57D14BuL},
     {0x3FBFF79F0BEFA2C7uL, 0x3FDC40AB5FFFD07AuL},
     {0x3FC0AEB1FECAE3A9uL, 0x3FDC199BDD85529CuL},
     {0x3FC161B4871C5CECuL, 0x3FDBF2C25BD71E09uL},
     {0x3FC214D876F26FD0uL, 0x3FDBCC1E904BC1D2uL},
     {0x3FC2C81F2693816FuL, 0x3FDBA5B030A1064AuL},
     {0x3FC37B89EE88BEF7uL, 0x3FDB7F76F2FB5E47uL},
     {0x3FC42F1A27A0B3CDuL, 0x3FDB59728DE5593AuL},
     {0x3FC4E2D12AF1E037uL, 0x3FDB33A2B84F15FBuL},
     {0x3FC596B051DD508DuL, 0x3FDB0E07298DB666uL},
     {0x3FC64AB8F61134FAuL, 0x3FDAE89F995AD3ADuL},
     {0x3FC6FEEC718B79D1uL, 0x3FDAC36BBFD3F37AuL},
     {0x3FC7B34C1E9C607FuL, 0x3FDA9E6B5579FDBFuL},
     {0x3FC867D957E91912uL, 0x3FDA799E1330B358uL},
     {0x3FC91C95786E5C72uL, 0x3FDA5503B23E255DuL},
     {0x3FC9D181DB83072FuL, 0x3FDA309BEC4A2D33uL},
     {0x3FCA869FDCDAB512uL, 0x3FDA0C667B5DE565uL},
     {0x3FCB3BF0D8885D4CuL, 0x3FD9E86319E32323uL},
     {0x3FCBF1762B00EF69uL, 0x3FD9C49182A3F090uL},
     {0x3FCCA731311DF0FBuL, 0x3FD9A0F170CA07BAuL},
     {0x3FCD5D2348201C09uL, 0x3FD97D829FDE4E50uL},
     {0x3FCE134DCDB1FE3EuL, 0x3FD95A44CBC8520FuL},
     {0x3FCEC9B21FEA98EAuL, 0x3FD93737B0CDC5E5uL},
     {0x3FCF80519D5001D3uL, 0x3FD9145B0B91FFC6uL},
     {0x3FD01B96D26D026AuL, 0x3FD8F1AE99157736uL},
     {0x3FD07723CAFA6331uL, 0x3FD8CF3216B5448CuL},
     {0x3FD0D2D06841B373uL, 0x3FD8ACE5422AA0DBuL},
     {0x3FD12E9D5A715381uL, 0x3FD88AC7D98A6699uL},
     {0x3FD18A8B51F5C661uL, 0x3FD868D99B4492EDuL},
     {0x3FD1E69AFF7B04D7uL, 0x3FD8471A4623C7ADuL},
     {0x3FD242CD13EDD0F1uL, 0x3FD82589994CCE13uL},
     {0x3FD29F22407D0A0CuL, 0x3FD80427543E1A12uL},
     {0x3FD2FB9B369B0153uL, 0x3FD7E2F336CF4E62uL},
     {0x3FD35838A7FECEC8uL, 0x3FD7C1ED0130C132uL},
     {0x3FD3B4FB46A5A6CCuL, 0x3FD7A11473EB0187uL},
     {0x3FD411E3C4D4302FuL, 0x3FD780694FDE5D3FuL},
     {0x3FD46EF2D517DAC8uL, 0x3FD75FEB564267C9uL},
     {0x3FD4CC292A48369EuL, 0x3FD73F9A48A58174uL},
     {0x3FD5298777884B96uL, 0x3FD71F75E8EC5F74uL},
     {0x3FD5870E7047F1BCuL, 0x3FD6FF7DF9519484uL},
     {0x3FD5E4BEC8452A1AuL, 0x3FD6DFB23C651A2FuL},
     {0x3FD64299338D7827uL, 0x3FD6C012750BDABFuL},
     {0x3FD6A09E667F3BCDuL, 0x3FD6A09E667F3BCDuL},
     {0x3FD6FECF15CB0C0BuL, 0x3FD68155D44CA973uL},
     {0x3FD75D2BF6751239uL, 0x3FD6623882552225uL},
     {0x3FD7BBB5BDD665E8uL, 0x3FD6434634CCC320uL},
     {0x3FD81A6D219E6963uL, 0x3FD6247EB03A5585uL},
     {0x3FD87952D7D426DFuL, 0x3FD605E1B976DC09uL},
     {0x3FD8D86796D7AE49uL, 0x3FD5E76F15AD2148uL},
     {0x3FD937AC156373C8uL, 0x3FD5C9268A5946B7uL},
     {0x3FD997210A8DAEE4uL, 0x3FD5AB07DD485429uL},
     {0x3FD9F6C72DC9BA68uL, 0x3FD58D12D497C7FDuL},
     {0x3FDA569F36E974EAuL, 0x3FD56F4736B527DAuL},
     {0x3FDAB6A9DE1EA215uL, 0x3FD551A4CA5D920FuL},
     {0x3FDB16E7DBFC4CA3uL, 0x3FD5342B569D4F82uL},
     {0x3FDB7759E9782918uL, 0x3FD516DAA2CF6642uL},
     {0x3FDBD800BFEBF932uL, 0x3FD4F9B2769D2CA7uL},
     {0x3FDC38DD1916F025uL, 0x3FD4DCB299FDDD0DuL},
     {0x3FDC99EFAF1F1790uL, 0x3FD4BFDAD5362A27uL},
     {0x3FDCFB393C92B539uL, 0x3FD4A32AF0D7D3DEuL},
     {0x3FDD5CBA7C69B19CuL, 0x3FD486A2B5C13CD0uL},
     {0x3FDDBE742A06FF34uL, 0x3FD46A41ED1D0057uL},
     {0x3FDE2067013A029DuL, 0x3FD44E086061892DuL},
     {0x3FDE8293BE3FFB87uL, 0x3FD431F5D950A897uL},
     {0x3FDEE4FB1DC56E75uL, 0x3FD4160A21F72E2AuL},
     {0x3FDF479DDCE78F58uL, 0x3FD3FA4504AC801CuL},
     {0x3FDFAA7CB935ACFEuL, 0x3FD3DEA64C123422uL},
     {0x3FE006CC38594EB1uL, 0x3FD3C32DC313A8E5uL},
     {0x3FE03878E0EB1569uL, 0x3FD3A7DB34E59FF7uL},
     {0x3FE06A44B5C74101uL, 0x3FD38CAE6D05D866uL},
     {0x3FE09C3016A0D077uL, 0x3FD371A7373AA9CBuL},
     {0x3FE0CE3B63676360uL, 0x3FD356C55F929FF1uL},
     {0x3FE10066FC47F240uL, 0x3FD33C08B26416FFuL},
     {0x3FE132B341AD8761uL, 0x3FD32170FC4CD831uL},
     {0x3FE165209441F823uL, 0x3FD306FE0A31B715uL},
     {0x3FE197AF54EE9EBBuL, 0x3FD2ECAFA93E2F56uL},
     {0x3FE1CA5FE4DD1475uL, 0x3FD2D285A6E4030BuL},
     {0x3FE1FD32A577EC72uL, 0x3FD2B87FD0DAD990uL},
     {0x3FE23027F86B6ED6uL, 0x3FD29E9DF51FDEE1uL},
     {0x3FE263403FA65489uL, 0x3FD284DFE1F56381uL},
     {0x3FE2967BDD5A8364uL, 0x3FD26B4565E27CDDuL},
     {0x3FE2C9DB33FDCAE9uL, 0x3FD251CE4FB2A63FuL},
     {0x3FE2FD5EA64AA180uL, 0x3FD2387A6E756238uL},
     {0x3FE331069740E22FuL, 0x3FD21F49917DDC96uL},
     {0x3FE364D36A268AE0uL, 0x3FD2063B88628CD6uL},
     {0x3FE398C582887B27uL, 0x3FD1ED5022FCD91DuL},
     {0x3FE3CCDD443B3394uL, 0x3FD1D4873168B9AAuL},
     {0x3FE4011B135B9590uL, 0x3FD1BBE084045CD4uL},
     {0x3FE4357F544FA3C1uL, 0x3FD1A35BEB6FCB75uL},
     {0x3FE46A0A6BC742FDuL, 0x3FD18AF9388C8DEAuL},
     {0x3FE49EBCBEBCFBCAuL, 0x3FD172B83C7D517BuL},
     {0x3FE4D396B276BC6FuL, 0x3FD15A98C8A58E51uL},
     {0x3FE50898AC869B96uL, 0x3FD1429AAEA92DE0uL},
     {0x3FE53DC312CB9B7AuL, 0x3FD12ABDC06C31CCuL},
     {0x3FE573164B726DB6uL, 0x3FD11301D0125B51uL},
     {0x3FE5A892BCF6379BuL, 0x3FD0FB66AFFED31BuL},
     {0x3FE5DE38CE215725uL, 0x3FD0E3EC32D3D1A2uL},
     {0x3FE61408E60E2888uL, 0x3FD0CC922B7247F7uL},
     {0x3FE64A036C27CC52uL, 0x3FD0B5586CF9890FuL},
     {0x3FE68028C82AEE2FuL, 0x3FD09E3ECAC6F383uL},
     {0x3FE6B67962268C43uL, 0x3FD0874518759BC8uL},
     {0x3FE6ECF5A27CBF28uL, 0x3FD0706B29DDF6DEuL},
     {0x3FE7239DF1E38286uL, 0x3FD059B0D3158574uL},
     {0x3FE75A72B9657E51uL, 0x3FD04315E86E7F85uL},
     {0x3FE791746262D0A8uL, 0x3FD02C9A3E778061uL},
     {0x3FE7C8A35691D856uL, 0x3FD0163DA9FB3335uL}
     },
    0x42C8000000000000uL,
    0x40861d99u,

    0x3FF0000000000000uL,
    0x3FDFFFFFFFFFFDBDuL,
    0x3FC55555555554ADuL,
    0x3FA55555CF16D299uL,
    0x3F8111115712F425uL,
    0x7ff0000000000000uL,
    0x000000000000FFC0uL,
    ((1 << 7) - 1)
};

static __constant _iml_v2_dp_union_t __dsinh_la_CoutTab[149] = {

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

    0x00000000, 0x3FE00000,
    0x555548F8, 0x3FC55555,
    0x55558FCC, 0x3FA55555,
    0x3AAF20D3, 0x3F811112,
    0x1C2A3FFD, 0x3F56C16A,

    0x55555555, 0x3FC55555,
    0x11111111, 0x3F811111,
    0x1A01A01A, 0x3F2A01A0,
    0xA556C734, 0x3EC71DE3,

    0x652B82FE, 0x40571547,

    0x00000000, 0x43380000,

    0x02000000, 0x41A00000,

    0x8FB9F87E, 0x408633CE,

    0xFEFA0000, 0x3F862E42,
    0xBC9E3B3A, 0x3D1CF79A,

    0x00000001, 0x00100000,
    0xFFFFFFFF, 0x7FEFFFFF,

    0xDADBE120, 0x3F9BDB8C,

    0xE7026820, 0x40357CD0
};

__attribute__((always_inline))
inline int __internal_dsinh_la_cout (double *a, double *r)
{
    int nRet = 0;
    double M;
    double Nj;
    double R, RHi, RMid, RLo;
    double p, pHi, pLo;
    double ph, pl;
    double scale;
    double MLog;
    double absAi;
    double rsq, podd, peven;
    double TpHi, TpLo, TmHi, TmLo;
    double TdH, TdL;
    double TsHi, TsLo, TsH, TsL;
    double tmp1, dbIn;
    double v1, v2, v3;

    _iml_uint32_t N, j;
    int exp;
    scale = ((__constant double *) __dsinh_la_CoutTab)[1];
    dbIn = (*a);

    if ((((((_iml_v2_dp_union_t *) & dbIn)->dwords.hi_dword >> 20) & 0x7FF) != 0x7FF))
    {

        exp = ((((_iml_v2_dp_union_t *) & dbIn)->dwords.hi_dword >> 20) & 0x7FF);

        if (exp > 0)
        {
            absAi = dbIn;
            (((_iml_v2_dp_union_t *) & absAi)->dwords.hi_dword =
             (((_iml_v2_dp_union_t *) & absAi)->dwords.hi_dword & 0x7FFFFFFF) | ((_iml_uint32_t) (0) << 31));

            if (exp > (0x3FF - 54))
            {

                if (absAi < ((__constant double *) __dsinh_la_CoutTab)[142])
                {

                    if (absAi < ((__constant double *) __dsinh_la_CoutTab)[148])
                    {

                        if (absAi >= ((__constant double *) __dsinh_la_CoutTab)[147])
                        {

                            tmp1 = (absAi * ((__constant double *) __dsinh_la_CoutTab)[139]);
                            Nj = (tmp1 + ((__constant double *) __dsinh_la_CoutTab)[140]);
                            M = (Nj - ((__constant double *) __dsinh_la_CoutTab)[140]);

                            tmp1 = (absAi - M * ((__constant double *) __dsinh_la_CoutTab)[143]);
                            MLog = (-M * ((__constant double *) __dsinh_la_CoutTab)[144]);

                            v1 = ((tmp1) + (MLog));
                            v2 = ((tmp1) - v1);
                            v3 = (v1 + v2);
                            v2 = ((MLog) + v2);
                            v3 = ((tmp1) - v3);
                            v3 = (v2 + v3);
                            R = v1;
                            RLo = v3;;

                            v1 = ((R) * (((__constant double *) __dsinh_la_CoutTab)[141]));
                            v2 = (v1 - (R));
                            v1 = (v1 - v2);
                            v2 = ((R) - v1);
                            RHi = v1;
                            RMid = v2;;

                            rsq = R * R;
                            podd =
                                ((((__constant double *) __dsinh_la_CoutTab)[133] * rsq + ((__constant double *) __dsinh_la_CoutTab)[131]) * rsq) * R;
                            peven =
                                (((((__constant double *) __dsinh_la_CoutTab)[134] * rsq + ((__constant double *) __dsinh_la_CoutTab)[132]) * rsq) +
                                 ((__constant double *) __dsinh_la_CoutTab)[130]) * rsq;

                            N = (((_iml_v2_dp_union_t *) & Nj)->dwords.lo_dword);
                            j = N & ((1 << 6) - 1);
                            N = N >> 6;

                            N = N + 0x3FF;
                            N = N & 0x7FF;

                            (((_iml_v2_dp_union_t *) & scale)->dwords.hi_dword =
                             (((_iml_v2_dp_union_t *) & scale)->dwords.hi_dword & 0x800FFFFF) | (((_iml_uint32_t) (N - 1) & 0x7FF) << 20));

                            TpHi = ((__constant double *) __dsinh_la_CoutTab)[2 * (j)] * scale;
                            TpLo = ((__constant double *) __dsinh_la_CoutTab)[2 * (j) + 1] * scale;

                            N = 2 * 0x3FF - N;
                            (((_iml_v2_dp_union_t *) & scale)->dwords.hi_dword =
                             (((_iml_v2_dp_union_t *) & scale)->dwords.hi_dword & 0x800FFFFF) | (((_iml_uint32_t) (N - 2) & 0x7FF) << 20));

                            TmHi = ((__constant double *) __dsinh_la_CoutTab)[2 * (64 - (j))] * scale;
                            TmLo = ((__constant double *) __dsinh_la_CoutTab)[2 * (64 - (j)) + 1] * scale;

                            v1 = ((TpHi) + (-TmHi));
                            tmp1 = ((TpHi) - v1);
                            v2 = (tmp1 + (-TmHi));
                            TdH = v1;
                            TdL = v2;;

                            TdL -= TmLo;
                            TdL += TpLo;

                            v1 = ((TdH) + (TdL));
                            tmp1 = ((TdH) - v1);
                            v2 = (tmp1 + (TdL));
                            TdH = v1;
                            TdL = v2;;

                            v1 = ((TpHi) + (TmHi));
                            tmp1 = ((TpHi) - v1);
                            v2 = (tmp1 + (TmHi));
                            TsH = v1;
                            TsL = v2;;
                            tmp1 = (TpLo + TmLo);
                            TsL += tmp1;

                            v1 = ((TsH) + (TsL));
                            tmp1 = ((TsH) - v1);
                            v2 = (tmp1 + (TsL));
                            TsH = v1;
                            TsL = v2;;

                            v1 = ((TsH) * (((__constant double *) __dsinh_la_CoutTab)[141]));
                            v2 = (v1 - (TsH));
                            v1 = (v1 - v2);
                            v2 = ((TsH) - v1);
                            TsHi = v1;
                            TsLo = v2;;

                            pLo = (RLo * TsL);
                            pLo += (podd * TsL);
                            pLo += (peven * TdL);
                            pLo += (R * TsL);
                            pLo += (RLo * TsH);
                            pLo += TdL;

                            ph = (podd * TsH);
                            pl = (peven * TdH);

                            v1 = ((ph) + (pl));
                            tmp1 = ((ph) - v1);
                            v2 = (tmp1 + (pl));
                            pHi = v1;
                            pl = v2;;
                            pLo += pl;

                            pLo += (RMid * TsLo);
                            pLo += (RHi * TsLo);
                            pLo += (RMid * TsHi);

                            RHi = (RHi * TsHi);

                            v1 = ((RHi) + (pHi));
                            tmp1 = ((RHi) - v1);
                            v2 = (tmp1 + (pHi));
                            pHi = v1;
                            pl = v2;;
                            pLo += pl;

                            v1 = ((TdH) + (pHi));
                            tmp1 = ((TdH) - v1);
                            v2 = (tmp1 + (pHi));
                            pHi = v1;
                            pl = v2;;
                            pLo += pl;
                            p = (pHi + pLo);

                            (((_iml_v2_dp_union_t *) & p)->dwords.hi_dword =
                             (((_iml_v2_dp_union_t *) & p)->dwords.
                              hi_dword & 0x7FFFFFFF) | ((_iml_uint32_t) ((((_iml_v2_dp_union_t *) & dbIn)->dwords.hi_dword >> 31)) << 31));

                            (*r) = p;

                        }
                        else
                        {

                            rsq = (absAi * absAi);
                            p = (absAi *
                                 (rsq *
                                  (((__constant double *) __dsinh_la_CoutTab)[135] +
                                   rsq * (((__constant double *) __dsinh_la_CoutTab)[136] +
                                          rsq * (((__constant double *) __dsinh_la_CoutTab)[137] +
                                                 rsq * ((__constant double *) __dsinh_la_CoutTab)[138])))));

                            p += absAi;

                            (((_iml_v2_dp_union_t *) & p)->dwords.hi_dword =
                             (((_iml_v2_dp_union_t *) & p)->dwords.
                              hi_dword & 0x7FFFFFFF) | ((_iml_uint32_t) ((((_iml_v2_dp_union_t *) & dbIn)->dwords.hi_dword >> 31)) << 31));

                            (*r) = p;
                        }
                    }
                    else
                    {

                        tmp1 = absAi * ((__constant double *) __dsinh_la_CoutTab)[139];
                        Nj = (tmp1 + ((__constant double *) __dsinh_la_CoutTab)[140]);
                        M = (Nj - ((__constant double *) __dsinh_la_CoutTab)[140]);
                        R = (absAi - M * ((__constant double *) __dsinh_la_CoutTab)[143]);
                        R -= (M * ((__constant double *) __dsinh_la_CoutTab)[144]);

                        p = ((((((__constant double *) __dsinh_la_CoutTab)[134] * R + ((__constant double *) __dsinh_la_CoutTab)[133]) * R +
                               ((__constant double *) __dsinh_la_CoutTab)[132]) * R + ((__constant double *) __dsinh_la_CoutTab)[131]) * R +
                             ((__constant double *) __dsinh_la_CoutTab)[130]);
                        p = (p * R);
                        p = (p * R + R);

                        N = (((_iml_v2_dp_union_t *) & Nj)->dwords.lo_dword);
                        j = N & ((1 << 6) - 1);
                        N = N >> 6;
                        N += 0x3FF;

                        p *= ((__constant double *) __dsinh_la_CoutTab)[2 * (j)];
                        p += ((__constant double *) __dsinh_la_CoutTab)[2 * (j) + 1];
                        p += ((__constant double *) __dsinh_la_CoutTab)[2 * (j)];

                        N = (N - 1) & 0x7FF;

                        if (N <= (0x7FF - 1))
                        {

                            (((_iml_v2_dp_union_t *) & scale)->dwords.hi_dword =
                             (((_iml_v2_dp_union_t *) & scale)->dwords.hi_dword & 0x800FFFFF) | (((_iml_uint32_t) (N) & 0x7FF) << 20));

                            p = (p * scale);
                        }
                        else
                        {

                            (((_iml_v2_dp_union_t *) & scale)->dwords.hi_dword =
                             (((_iml_v2_dp_union_t *) & scale)->dwords.hi_dword & 0x800FFFFF) | (((_iml_uint32_t) (N - 1) & 0x7FF) << 20));

                            p = (p * scale);
                            p = (p * ((__constant double *) __dsinh_la_CoutTab)[128]);
                        }

                        (((_iml_v2_dp_union_t *) & p)->dwords.hi_dword =
                         (((_iml_v2_dp_union_t *) & p)->dwords.
                          hi_dword & 0x7FFFFFFF) | ((_iml_uint32_t) ((((_iml_v2_dp_union_t *) & dbIn)->dwords.hi_dword >> 31)) << 31));
                        (*r) = p;
                    }
                }
                else
                {

                    (*r) = ((__constant double *) __dsinh_la_CoutTab)[146] * dbIn;

                    nRet = 3;
                }
            }
            else
            {

                (*r) = (((__constant double *) __dsinh_la_CoutTab)[0] + ((__constant double *) __dsinh_la_CoutTab)[145]) * dbIn;
            }
        }
        else
        {

            v1 = dbIn * ((__constant double *) __dsinh_la_CoutTab)[145];
            (*r) = dbIn + v1;
        }
    }
    else
    {

        (*r) = dbIn + dbIn;
    }

    return nRet;

}

double __ocl_svml_sinh (double a)
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
        double dTdif;
        double dTn;
        double dG_1;
        double dG_2;
        double dG_3;
        unsigned long lM;
        double dXSign;
        unsigned int iAbsX;
        double dAbsX;

        unsigned long lIndex;
        unsigned long lX;
        unsigned int iRangeMask;

        double dbInvLn2;
        double dbShifter;
        double dbLn2[2];
        double dPC[5];
        unsigned long lIndexMask;
        unsigned int iDomainRange;

        dbInvLn2 = as_double (__internal_dsinh_la_data._dbInvLn2);
        dbShifter = as_double (__internal_dsinh_la_data._dbShifter);
        lIndexMask = (__internal_dsinh_la_data._lIndexMask);

        dPC[1] = as_double (__internal_dsinh_la_data._dPC2);
        dPC[2] = as_double (__internal_dsinh_la_data._dPC3);
        dPC[3] = as_double (__internal_dsinh_la_data._dPC4);
        dPC[4] = as_double (__internal_dsinh_la_data._dPC5);
        dXSign = as_double (__internal_dsinh_la_data._dSign);
        iDomainRange = (__internal_dsinh_la_data._iDomainRange);

        dXSign = as_double ((as_ulong (dXSign) & as_ulong (va1)));
        dAbsX = as_double ((as_ulong (dXSign) ^ as_ulong (va1)));

        dM = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dAbsX, dbInvLn2, dbShifter);

        lX = as_ulong (dAbsX);
        iAbsX = ((unsigned int) ((unsigned long) lX >> 32));
        iRangeMask = ((unsigned int) (-(signed int) ((signed int) iAbsX > (signed int) iDomainRange)));
        vm = 0;
        vm = iRangeMask;

        lM = as_ulong (dM);
        lIndex = (lM & lIndexMask);
        lM = (lM ^ lIndex);

        dN = (dM - dbShifter);
        dbLn2[0] = as_double (__internal_dsinh_la_data._dbLn2hi);
        dR = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (-(dbLn2[0]), dN, dAbsX);
        dbLn2[1] = as_double (__internal_dsinh_la_data._dbLn2lo);
        dTn = as_double (((__constant unsigned long *) (__internal_dsinh_la_data._dbT))[(((0 + lIndex) * (2 * 8)) >> (3)) + 1]);
        dTdif = as_double (((__constant unsigned long *) (__internal_dsinh_la_data._dbT))[(((0 + lIndex) * (2 * 8)) >> (3)) + 0]);
        dR = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (-(dbLn2[1]), dN, dR);
        dR2 = (dR * dR);

        lM = ((unsigned long) (lM) << ((52 - 7)));
        lX = as_ulong (dTdif);
        lX = (lX + lM);
        dG_1 = as_double (lX);

        lX = as_ulong (dTn);
        lX = (lX + lM);
        dG_2 = as_double (lX);

        lX = as_ulong (dTn);
        lX = (lX - lM);
        dM = as_double (lX);

        dG_3 = (dG_2 + dM);
        dG_2 = (dG_2 - dM);

        dSinh_r = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dPC[4], dR2, dPC[2]);
        dSinh_r = (dSinh_r * dR2);
        dSinh_r = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dSinh_r, dR, dR);

        dOut = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dPC[3], dR2, dPC[1]);
        dOut = (dOut * dR2);
        dG_2 = (dG_1 + dG_2);
        dOut = (dOut * dG_2);
        dG_1 = (dG_1 + dG_3);
        dOut = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dSinh_r, dG_1, dOut);
        dOut = (dOut + dG_2);

        vr1 = as_double ((as_ulong (dXSign) | as_ulong (dOut)));
    }

    if ((vm) != 0)
    {
        double _vapi_arg1[1];
        double _vapi_res1[1];
        ((double *) _vapi_arg1)[0] = va1;
        ((double *) _vapi_res1)[0] = vr1;
        __internal_dsinh_la_cout (_vapi_arg1, _vapi_res1);
        vr1 = ((double *) _vapi_res1)[0];
    };
    r = vr1;;

    return r;

}

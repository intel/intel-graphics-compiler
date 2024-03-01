/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
/*
// ALGORITHM DESCRIPTION:
//  *
//  *  Compute cosh(x) as (exp(x)+exp(-x))/2,
//  *  where exp is calculated as
//  *  exp(M*ln2 + ln2*(j/2^k) + r) = 2^M * 2^(j/2^k) * exp(r)
//  *
//  *  Special cases:
//  *
//  *  cosh(NaN) = quiet NaN, and raise invalid exception
//  *  cosh(INF) = that INF
//  *  cosh(0)   = 1
//  *  cosh(x) overflows for big x and returns MAXLOG+log(2)
//
*/
#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF
typedef struct {
  /*Shared, used ifdef _USE_SMALL_TABLE_ */
  unsigned long _dTp_h[(1 << 4)];
  unsigned long _dTp_l[(1 << 4)];
  unsigned long _dTn_h[(1 << 4)];
  unsigned long _dbShifter_UISA;
  unsigned long _lIndexMask_UISA; //(1<<K)1-
  unsigned long _dPC2_UISA;
  unsigned long _dPC3_UISA;
  unsigned long _dPC4_UISA;
  unsigned long _dPC5_UISA;
  unsigned long _dPC6_UISA;
  unsigned long _dPC7_UISA;
  /************************************************************/
  unsigned long _dbT[(1 << 7)][3]; // HA: <dTp_h,dTp_l,dTn_h>
  unsigned long _dbInvLn2;
  unsigned long _dbLn2hi;
  unsigned long _dbLn2lo;
  unsigned long _dbShifter;
  unsigned int _iIndexMask; //(1<<K)1-
  unsigned long _dPC1;      //=1
  unsigned long _dPC2;
  unsigned long _dPC3;
  unsigned long _dPC4;
  unsigned long _dPC5;
  unsigned long _lExpMask;
  unsigned long _dSign; // 0x8000000000000000
  unsigned int _iDomainRange;
} __ocl_svml_internal_dcosh_ha_data_t;
static __ocl_svml_internal_dcosh_ha_data_t __ocl_svml_internal_dcosh_ha_data = {
    {/*== _dTp_h ==*/
     0x3fe0000000000000uL, 0x3fe0b5586cf9890fuL, 0x3fe172b83c7d517buL,
     0x3fe2387a6e756238uL, 0x3fe306fe0a31b715uL, 0x3fe3dea64c123422uL,
     0x3fe4bfdad5362a27uL, 0x3fe5ab07dd485429uL, 0x3fe6a09e667f3bcduL,
     0x3fe7a11473eb0187uL, 0x3fe8ace5422aa0dbuL, 0x3fe9c49182a3f090uL,
     0x3feae89f995ad3aduL, 0x3fec199bdd85529cuL, 0x3fed5818dcfba487uL,
     0x3feea4afa2a490dauL}, /* dTp_h */
    {                       /*== dTp_l ==*/
     0x0000000000000000uL, 0x3c88a62e4adc610buL, 0xbc719041b9d78a76uL,
     0x3c89b07eb6c70573uL, 0x3c76f46ad23182e4uL, 0x3c7ada0911f09ebcuL,
     0x3c6d4397afec42e2uL, 0x3c86324c054647aduL, 0xbc8bdd3413b26456uL,
     0xbc741577ee04992fuL, 0x3c86e9f156864b27uL, 0x3c6c7c46b071f2beuL,
     0x3c87a1cd345dcc81uL, 0x3c711065895048dduL, 0x3c72ed02d75b3707uL,
     0xbc8e9c23179c2893uL}, /* dTp_l */
    {                       /*== dTn_h ==*/
     0x3fe0000000000000uL, 0x3fdea4afa2a490dauL, 0x3fdd5818dcfba487uL,
     0x3fdc199bdd85529cuL, 0x3fdae89f995ad3aduL, 0x3fd9c49182a3f090uL,
     0x3fd8ace5422aa0dbuL, 0x3fd7a11473eb0187uL, 0x3fd6a09e667f3bcduL,
     0x3fd5ab07dd485429uL, 0x3fd4bfdad5362a27uL, 0x3fd3dea64c123422uL,
     0x3fd306fe0a31b715uL, 0x3fd2387a6e756238uL, 0x3fd172b83c7d517buL,
     0x3fd0b5586cf9890fuL}, /* dTn_h */
    0x42F8000000000000uL,   /* _dbShifter_UISA  */
    0x000000000000000FuL,   /* _lIndexMask_UISA */
    0x3fe0000000000004uL,   /* _dPC2_UISA       */
    0x3fc5555555555543uL,   /* _dPC3_UISA       */
    0x3fa5555555484f37uL,   /* _dPC4_UISA       */
    0x3f81111111286a0cuL,   /* _dPC5_UISA       */
    0x3f56c183da08f116uL,   /* _dPC6_UISA       */
    0x3f2a018d76da03dauL,   /* _dPC7_UISA       */
    {                       /*== _dbT ==*/
     {0x3FE0000000000000uL, 0x0000000000000000uL, 0x3FE0000000000000uL},
     {0x3FE0163DA9FB3335uL, 0x3C8B61299AB8CDB7uL, 0x3FDFD3C22B8F71F1uL},
     {0x3FE02C9A3E778061uL, 0xBC619083535B085DuL, 0x3FDFA7C1819E90D8uL},
     {0x3FE04315E86E7F85uL, 0xBC80A31C1977C96EuL, 0x3FDF7BFDAD9CBE14uL},
     {0x3FE059B0D3158574uL, 0x3C7D73E2A475B465uL, 0x3FDF50765B6E4540uL},
     {0x3FE0706B29DDF6DEuL, 0xBC7C91DFE2B13C27uL, 0x3FDF252B376BBA97uL},
     {0x3FE0874518759BC8uL, 0x3C5186BE4BB284FFuL, 0x3FDEFA1BEE615A27uL},
     {0x3FE09E3ECAC6F383uL, 0x3C81487818316136uL, 0x3FDECF482D8E67F1uL},
     {0x3FE0B5586CF9890FuL, 0x3C88A62E4ADC610BuL, 0x3FDEA4AFA2A490DAuL},
     {0x3FE0CC922B7247F7uL, 0x3C801EDC16E24F71uL, 0x3FDE7A51FBC74C83uL},
     {0x3FE0E3EC32D3D1A2uL, 0x3C303A1727C57B53uL, 0x3FDE502EE78B3FF6uL},
     {0x3FE0FB66AFFED31BuL, 0xBC5B9BEDC44EBD7BuL, 0x3FDE264614F5A129uL},
     {0x3FE11301D0125B51uL, 0xBC86C51039449B3AuL, 0x3FDDFC97337B9B5FuL},
     {0x3FE12ABDC06C31CCuL, 0xBC41B514B36CA5C7uL, 0x3FDDD321F301B460uL},
     {0x3FE1429AAEA92DE0uL, 0xBC832FBF9AF1369EuL, 0x3FDDA9E603DB3285uL},
     {0x3FE15A98C8A58E51uL, 0x3C72406AB9EEAB0AuL, 0x3FDD80E316C98398uL},
     {0x3FE172B83C7D517BuL, 0xBC719041B9D78A76uL, 0x3FDD5818DCFBA487uL},
     {0x3FE18AF9388C8DEAuL, 0xBC811023D1970F6CuL, 0x3FDD2F87080D89F2uL},
     {0x3FE1A35BEB6FCB75uL, 0x3C7E5B4C7B4968E4uL, 0x3FDD072D4A07897CuL},
     {0x3FE1BBE084045CD4uL, 0xBC895386352EF607uL, 0x3FDCDF0B555DC3FAuL},
     {0x3FE1D4873168B9AAuL, 0x3C8E016E00A2643CuL, 0x3FDCB720DCEF9069uL},
     {0x3FE1ED5022FCD91DuL, 0xBC81DF98027BB78CuL, 0x3FDC8F6D9406E7B5uL},
     {0x3FE2063B88628CD6uL, 0x3C7DC775814A8495uL, 0x3FDC67F12E57D14BuL},
     {0x3FE21F49917DDC96uL, 0x3C72A97E9494A5EEuL, 0x3FDC40AB5FFFD07AuL},
     {0x3FE2387A6E756238uL, 0x3C89B07EB6C70573uL, 0x3FDC199BDD85529CuL},
     {0x3FE251CE4FB2A63FuL, 0x3C7AC155BEF4F4A4uL, 0x3FDBF2C25BD71E09uL},
     {0x3FE26B4565E27CDDuL, 0x3C72BD339940E9D9uL, 0x3FDBCC1E904BC1D2uL},
     {0x3FE284DFE1F56381uL, 0xBC8A4C3A8C3F0D7EuL, 0x3FDBA5B030A1064AuL},
     {0x3FE29E9DF51FDEE1uL, 0x3C7612E8AFAD1255uL, 0x3FDB7F76F2FB5E47uL},
     {0x3FE2B87FD0DAD990uL, 0xBC310ADCD6381AA4uL, 0x3FDB59728DE5593AuL},
     {0x3FE2D285A6E4030BuL, 0x3C80024754DB41D5uL, 0x3FDB33A2B84F15FBuL},
     {0x3FE2ECAFA93E2F56uL, 0x3C61CA0F45D52383uL, 0x3FDB0E07298DB666uL},
     {0x3FE306FE0A31B715uL, 0x3C76F46AD23182E4uL, 0x3FDAE89F995AD3ADuL},
     {0x3FE32170FC4CD831uL, 0x3C7A9CE78E18047CuL, 0x3FDAC36BBFD3F37AuL},
     {0x3FE33C08B26416FFuL, 0x3C832721843659A6uL, 0x3FDA9E6B5579FDBFuL},
     {0x3FE356C55F929FF1uL, 0xBC7B5CEE5C4E4628uL, 0x3FDA799E1330B358uL},
     {0x3FE371A7373AA9CBuL, 0xBC863AEABF42EAE2uL, 0x3FDA5503B23E255DuL},
     {0x3FE38CAE6D05D866uL, 0xBC8E958D3C9904BDuL, 0x3FDA309BEC4A2D33uL},
     {0x3FE3A7DB34E59FF7uL, 0xBC65E436D661F5E3uL, 0x3FDA0C667B5DE565uL},
     {0x3FE3C32DC313A8E5uL, 0xBC8EFFF8375D29C3uL, 0x3FD9E86319E32323uL},
     {0x3FE3DEA64C123422uL, 0x3C7ADA0911F09EBCuL, 0x3FD9C49182A3F090uL},
     {0x3FE3FA4504AC801CuL, 0xBC87D023F956F9F3uL, 0x3FD9A0F170CA07BAuL},
     {0x3FE4160A21F72E2AuL, 0xBC4EF3691C309278uL, 0x3FD97D829FDE4E50uL},
     {0x3FE431F5D950A897uL, 0xBC71C7DDE35F7999uL, 0x3FD95A44CBC8520FuL},
     {0x3FE44E086061892DuL, 0x3C389B7A04EF80D0uL, 0x3FD93737B0CDC5E5uL},
     {0x3FE46A41ED1D0057uL, 0x3C8C944BD1648A76uL, 0x3FD9145B0B91FFC6uL},
     {0x3FE486A2B5C13CD0uL, 0x3C63C1A3B69062F0uL, 0x3FD8F1AE99157736uL},
     {0x3FE4A32AF0D7D3DEuL, 0x3C89CB62F3D1BE56uL, 0x3FD8CF3216B5448CuL},
     {0x3FE4BFDAD5362A27uL, 0x3C6D4397AFEC42E2uL, 0x3FD8ACE5422AA0DBuL},
     {0x3FE4DCB299FDDD0DuL, 0x3C88ECDBBC6A7833uL, 0x3FD88AC7D98A6699uL},
     {0x3FE4F9B2769D2CA7uL, 0xBC84B309D25957E3uL, 0x3FD868D99B4492EDuL},
     {0x3FE516DAA2CF6642uL, 0xBC7F768569BD93EFuL, 0x3FD8471A4623C7ADuL},
     {0x3FE5342B569D4F82uL, 0xBC707ABE1DB13CADuL, 0x3FD82589994CCE13uL},
     {0x3FE551A4CA5D920FuL, 0xBC7D689CEFEDE59BuL, 0x3FD80427543E1A12uL},
     {0x3FE56F4736B527DAuL, 0x3C89BB2C011D93ADuL, 0x3FD7E2F336CF4E62uL},
     {0x3FE58D12D497C7FDuL, 0x3C7295E15B9A1DE8uL, 0x3FD7C1ED0130C132uL},
     {0x3FE5AB07DD485429uL, 0x3C86324C054647ADuL, 0x3FD7A11473EB0187uL},
     {0x3FE5C9268A5946B7uL, 0x3C2C4B1B816986A2uL, 0x3FD780694FDE5D3FuL},
     {0x3FE5E76F15AD2148uL, 0x3C8BA6F93080E65EuL, 0x3FD75FEB564267C9uL},
     {0x3FE605E1B976DC09uL, 0xBC83E2429B56DE47uL, 0x3FD73F9A48A58174uL},
     {0x3FE6247EB03A5585uL, 0xBC8383C17E40B497uL, 0x3FD71F75E8EC5F74uL},
     {0x3FE6434634CCC320uL, 0xBC7C483C759D8933uL, 0x3FD6FF7DF9519484uL},
     {0x3FE6623882552225uL, 0xBC8BB60987591C34uL, 0x3FD6DFB23C651A2FuL},
     {0x3FE68155D44CA973uL, 0x3C5038AE44F73E65uL, 0x3FD6C012750BDABFuL},
     {0x3FE6A09E667F3BCDuL, 0xBC8BDD3413B26456uL, 0x3FD6A09E667F3BCDuL},
     {0x3FE6C012750BDABFuL, 0xBC62895667FF0B0DuL, 0x3FD68155D44CA973uL},
     {0x3FE6DFB23C651A2FuL, 0xBC5BBE3A683C88ABuL, 0x3FD6623882552225uL},
     {0x3FE6FF7DF9519484uL, 0xBC783C0F25860EF6uL, 0x3FD6434634CCC320uL},
     {0x3FE71F75E8EC5F74uL, 0xBC716E4786887A99uL, 0x3FD6247EB03A5585uL},
     {0x3FE73F9A48A58174uL, 0xBC80A8D96C65D53CuL, 0x3FD605E1B976DC09uL},
     {0x3FE75FEB564267C9uL, 0xBC80245957316DD3uL, 0x3FD5E76F15AD2148uL},
     {0x3FE780694FDE5D3FuL, 0x3C8866B80A02162DuL, 0x3FD5C9268A5946B7uL},
     {0x3FE7A11473EB0187uL, 0xBC741577EE04992FuL, 0x3FD5AB07DD485429uL},
     {0x3FE7C1ED0130C132uL, 0x3C8F124CD1164DD6uL, 0x3FD58D12D497C7FDuL},
     {0x3FE7E2F336CF4E62uL, 0x3C605D02BA15797EuL, 0x3FD56F4736B527DAuL},
     {0x3FE80427543E1A12uL, 0xBC827C86626D972BuL, 0x3FD551A4CA5D920FuL},
     {0x3FE82589994CCE13uL, 0xBC8D4C1DD41532D8uL, 0x3FD5342B569D4F82uL},
     {0x3FE8471A4623C7ADuL, 0xBC78D684A341CDFBuL, 0x3FD516DAA2CF6642uL},
     {0x3FE868D99B4492EDuL, 0xBC8FC6F89BD4F6BAuL, 0x3FD4F9B2769D2CA7uL},
     {0x3FE88AC7D98A6699uL, 0x3C8994C2F37CB53AuL, 0x3FD4DCB299FDDD0DuL},
     {0x3FE8ACE5422AA0DBuL, 0x3C86E9F156864B27uL, 0x3FD4BFDAD5362A27uL},
     {0x3FE8CF3216B5448CuL, 0xBC60D55E32E9E3AAuL, 0x3FD4A32AF0D7D3DEuL},
     {0x3FE8F1AE99157736uL, 0x3C75CC13A2E3976CuL, 0x3FD486A2B5C13CD0uL},
     {0x3FE9145B0B91FFC6uL, 0xBC8DD6792E582524uL, 0x3FD46A41ED1D0057uL},
     {0x3FE93737B0CDC5E5uL, 0xBC575FC781B57EBCuL, 0x3FD44E086061892DuL},
     {0x3FE95A44CBC8520FuL, 0xBC664B7C96A5F039uL, 0x3FD431F5D950A897uL},
     {0x3FE97D829FDE4E50uL, 0xBC8D185B7C1B85D1uL, 0x3FD4160A21F72E2AuL},
     {0x3FE9A0F170CA07BAuL, 0xBC8173BD91CEE632uL, 0x3FD3FA4504AC801CuL},
     {0x3FE9C49182A3F090uL, 0x3C6C7C46B071F2BEuL, 0x3FD3DEA64C123422uL},
     {0x3FE9E86319E32323uL, 0x3C6824CA78E64C6EuL, 0x3FD3C32DC313A8E5uL},
     {0x3FEA0C667B5DE565uL, 0xBC8359495D1CD533uL, 0x3FD3A7DB34E59FF7uL},
     {0x3FEA309BEC4A2D33uL, 0x3C86305C7DDC36ABuL, 0x3FD38CAE6D05D866uL},
     {0x3FEA5503B23E255DuL, 0xBC8D2F6EDB8D41E1uL, 0x3FD371A7373AA9CBuL},
     {0x3FEA799E1330B358uL, 0x3C8BCB7ECAC563C7uL, 0x3FD356C55F929FF1uL},
     {0x3FEA9E6B5579FDBFuL, 0x3C80FAC90EF7FD31uL, 0x3FD33C08B26416FFuL},
     {0x3FEAC36BBFD3F37AuL, 0xBC7F9234CAE76CD0uL, 0x3FD32170FC4CD831uL},
     {0x3FEAE89F995AD3ADuL, 0x3C87A1CD345DCC81uL, 0x3FD306FE0A31B715uL},
     {0x3FEB0E07298DB666uL, 0xBC8BDEF54C80E425uL, 0x3FD2ECAFA93E2F56uL},
     {0x3FEB33A2B84F15FBuL, 0xBC52805E3084D708uL, 0x3FD2D285A6E4030BuL},
     {0x3FEB59728DE5593AuL, 0xBC8C71DFBBBA6DE3uL, 0x3FD2B87FD0DAD990uL},
     {0x3FEB7F76F2FB5E47uL, 0xBC65584F7E54AC3BuL, 0x3FD29E9DF51FDEE1uL},
     {0x3FEBA5B030A1064AuL, 0xBC8EFCD30E54292EuL, 0x3FD284DFE1F56381uL},
     {0x3FEBCC1E904BC1D2uL, 0x3C723DD07A2D9E84uL, 0x3FD26B4565E27CDDuL},
     {0x3FEBF2C25BD71E09uL, 0xBC8EFDCA3F6B9C73uL, 0x3FD251CE4FB2A63FuL},
     {0x3FEC199BDD85529CuL, 0x3C711065895048DDuL, 0x3FD2387A6E756238uL},
     {0x3FEC40AB5FFFD07AuL, 0x3C8B4537E083C60AuL, 0x3FD21F49917DDC96uL},
     {0x3FEC67F12E57D14BuL, 0x3C82884DFF483CADuL, 0x3FD2063B88628CD6uL},
     {0x3FEC8F6D9406E7B5uL, 0x3C61ACBC48805C44uL, 0x3FD1ED5022FCD91DuL},
     {0x3FECB720DCEF9069uL, 0x3C6503CBD1E949DBuL, 0x3FD1D4873168B9AAuL},
     {0x3FECDF0B555DC3FAuL, 0xBC7DD83B53829D72uL, 0x3FD1BBE084045CD4uL},
     {0x3FED072D4A07897CuL, 0xBC8CBC3743797A9CuL, 0x3FD1A35BEB6FCB75uL},
     {0x3FED2F87080D89F2uL, 0xBC8D487B719D8578uL, 0x3FD18AF9388C8DEAuL},
     {0x3FED5818DCFBA487uL, 0x3C72ED02D75B3707uL, 0x3FD172B83C7D517BuL},
     {0x3FED80E316C98398uL, 0xBC811EC18BEDDFE8uL, 0x3FD15A98C8A58E51uL},
     {0x3FEDA9E603DB3285uL, 0x3C8C2300696DB532uL, 0x3FD1429AAEA92DE0uL},
     {0x3FEDD321F301B460uL, 0x3C82DA5778F018C3uL, 0x3FD12ABDC06C31CCuL},
     {0x3FEDFC97337B9B5FuL, 0xBC81A5CD4F184B5CuL, 0x3FD11301D0125B51uL},
     {0x3FEE264614F5A129uL, 0xBC87B627817A1496uL, 0x3FD0FB66AFFED31BuL},
     {0x3FEE502EE78B3FF6uL, 0x3C739E8980A9CC8FuL, 0x3FD0E3EC32D3D1A2uL},
     {0x3FEE7A51FBC74C83uL, 0x3C82D522CA0C8DE2uL, 0x3FD0CC922B7247F7uL},
     {0x3FEEA4AFA2A490DAuL, 0xBC8E9C23179C2893uL, 0x3FD0B5586CF9890FuL},
     {0x3FEECF482D8E67F1uL, 0xBC8C93F3B411AD8CuL, 0x3FD09E3ECAC6F383uL},
     {0x3FEEFA1BEE615A27uL, 0x3C8DC7F486A4B6B0uL, 0x3FD0874518759BC8uL},
     {0x3FEF252B376BBA97uL, 0x3C83A1A5BF0D8E43uL, 0x3FD0706B29DDF6DEuL},
     {0x3FEF50765B6E4540uL, 0x3C89D3E12DD8A18BuL, 0x3FD059B0D3158574uL},
     {0x3FEF7BFDAD9CBE14uL, 0xBC8DBB12D006350AuL, 0x3FD04315E86E7F85uL},
     {0x3FEFA7C1819E90D8uL, 0x3C774853F3A5931EuL, 0x3FD02C9A3E778061uL},
     {0x3FEFD3C22B8F71F1uL, 0x3C52EB74966579E7uL, 0x3FD0163DA9FB3335uL}},
    0x3ff71547652b82feuL, /* _dbInvLn2 = 1/log(2) */
    0x3FE62E42FEFA0000uL, /* _dbLn2hi  = log(2) hi*/
    0x3D7CF79ABC9E3B3AuL, /* _dbLn2lo  = log(2) lo*/
    0x42C8000000000000uL, /* _dbShifter */
    0x0000007Fu,          /* _iIndexMask */
    0x3FF0000000000000uL, /* _dPC1 */
    0x3FDFFFFFFFFFFF58uL, /* _dPC2 */
    0x3FC55555555555E6uL, /* _dPC3 */
    0x3FA55555ACCA171DuL, /* _dPC4 */
    0x3F81111121A1CF67uL, /* _dPC5 */
    0x7ff0000000000000uL, /* _lExpMask */
    0x8000000000000000uL, /* _dSign*/
    0x40861d99u /* _iDomainRange 0x40861d9ac12a3e85 =(1021*2^K-0.5)*log(2)/2^K
                   -needed for quick exp*/
};              /*dCosh_Table*/
static __constant _iml_v2_dp_union_t __dcosh_ha_CoutTab[144] = {
    0x00000000, 0x3FF00000, /* T[0] = 1                       */
    0x00000000, 0x00000000, /* D[0] = 0                       */
    0x3E778061, 0x3FF02C9A, /* T[1] = 1.010889286051700475255 */
    0x535B085D, 0xBC719083, /* D[1] = -1.5234778603368578e-17 */
    0xD3158574, 0x3FF059B0, /* T[2] = 1.021897148654116627142 */
    0xA475B465, 0x3C8D73E2, /* D[2] = 5.10922502897344397e-17 */
    0x18759BC8, 0x3FF08745, /* T[3] = 1.033024879021228414899 */
    0x4BB284FF, 0x3C6186BE, /* D[3] = 7.60083887402708891e-18 */
    0x6CF9890F, 0x3FF0B558, /* T[4] = 1.044273782427413754803 */
    0x4ADC610B, 0x3C98A62E, /* D[4] = 8.55188970553796446e-17 */
    0x32D3D1A2, 0x3FF0E3EC, /* T[5] = 1.055645178360557157049 */
    0x27C57B53, 0x3C403A17, /* D[5] = 1.75932573877209185e-18 */
    0xD0125B51, 0x3FF11301, /* T[6] = 1.067140400676823697168 */
    0x39449B3A, 0xBC96C510, /* D[6] = -7.8998539668415819e-17 */
    0xAEA92DE0, 0x3FF1429A, /* T[7] = 1.078760797757119860307 */
    0x9AF1369E, 0xBC932FBF, /* D[7] = -6.6566604360565930e-17 */
    0x3C7D517B, 0x3FF172B8, /* T[8] = 1.090507732665257689675 */
    0xB9D78A76, 0xBC819041, /* D[8] = -3.0467820798124709e-17 */
    0xEB6FCB75, 0x3FF1A35B, /* T[9] = 1.102382583307840890896 */
    0x7B4968E4, 0x3C8E5B4C, /* D[9] = 5.26603687157069445e-17 */
    0x3168B9AA, 0x3FF1D487, /* T[10] = 1.114386742595892432206 */
    0x00A2643C, 0x3C9E016E, /* D[10] = 1.04102784568455711e-16 */
    0x88628CD6, 0x3FF2063B, /* T[11] = 1.126521618608241848136 */
    0x814A8495, 0x3C8DC775, /* D[11] = 5.16585675879545668e-17 */
    0x6E756238, 0x3FF2387A, /* T[12] = 1.138788634756691564576 */
    0xB6C70573, 0x3C99B07E, /* D[12] = 8.91281267602540758e-17 */
    0x65E27CDD, 0x3FF26B45, /* T[13] = 1.151189229952982673311 */
    0x9940E9D9, 0x3C82BD33, /* D[13] = 3.25071021886382730e-17 */
    0xF51FDEE1, 0x3FF29E9D, /* T[14] = 1.163724858777577475522 */
    0xAFAD1255, 0x3C8612E8, /* D[14] = 3.82920483692409357e-17 */
    0xA6E4030B, 0x3FF2D285, /* T[15] = 1.176396991650281220743 */
    0x54DB41D5, 0x3C900247, /* D[15] = 5.55420325421807881e-17 */
    0x0A31B715, 0x3FF306FE, /* T[16] = 1.189207115002721026897 */
    0xD23182E4, 0x3C86F46A, /* D[16] = 3.98201523146564623e-17 */
    0xB26416FF, 0x3FF33C08, /* T[17] = 1.202156731452703075647 */
    0x843659A6, 0x3C932721, /* D[17] = 6.64498149925230086e-17 */
    0x373AA9CB, 0x3FF371A7, /* T[18] = 1.215247359980468955243 */
    0xBF42EAE2, 0xBC963AEA, /* D[18] = -7.7126306926814877e-17 */
    0x34E59FF7, 0x3FF3A7DB, /* T[19] = 1.228480536106870024682 */
    0xD661F5E3, 0xBC75E436, /* D[19] = -1.8987816313025299e-17 */
    0x4C123422, 0x3FF3DEA6, /* T[20] = 1.241857812073484002013 */
    0x11F09EBC, 0x3C8ADA09, /* D[20] = 4.65802759183693656e-17 */
    0x21F72E2A, 0x3FF4160A, /* T[21] = 1.255380757024691096291 */
    0x1C309278, 0xBC5EF369, /* D[21] = -6.7113898212968785e-18 */
    0x6061892D, 0x3FF44E08, /* T[22] = 1.269050957191733219886 */
    0x04EF80D0, 0x3C489B7A, /* D[22] = 2.66793213134218605e-18 */
    0xB5C13CD0, 0x3FF486A2, /* T[23] = 1.282870016078778263591 */
    0xB69062F0, 0x3C73C1A3, /* D[23] = 1.71359491824356104e-17 */
    0xD5362A27, 0x3FF4BFDA, /* T[24] = 1.296839554651009640551 */
    0xAFEC42E2, 0x3C7D4397, /* D[24] = 2.53825027948883151e-17 */
    0x769D2CA7, 0x3FF4F9B2, /* T[25] = 1.310961211524764413738 */
    0xD25957E3, 0xBC94B309, /* D[25] = -7.1815361355194539e-17 */
    0x569D4F82, 0x3FF5342B, /* T[26] = 1.325236643159741323217 */
    0x1DB13CAD, 0xBC807ABE, /* D[26] = -2.8587312100388613e-17 */
    0x36B527DA, 0x3FF56F47, /* T[27] = 1.339667524053302916087 */
    0x011D93AD, 0x3C99BB2C, /* D[27] = 8.92728259483173191e-17 */
    0xDD485429, 0x3FF5AB07, /* T[28] = 1.354255546936892651289 */
    0x054647AD, 0x3C96324C, /* D[28] = 7.70094837980298924e-17 */
    0x15AD2148, 0x3FF5E76F, /* T[29] = 1.369002422974590515992 */
    0x3080E65E, 0x3C9BA6F9, /* D[29] = 9.59379791911884828e-17 */
    0xB03A5585, 0x3FF6247E, /* T[30] = 1.383909881963832022578 */
    0x7E40B497, 0xBC9383C1, /* D[30] = -6.7705116587947862e-17 */
    0x82552225, 0x3FF66238, /* T[31] = 1.398979672538311236352 */
    0x87591C34, 0xBC9BB609, /* D[31] = -9.6142132090513227e-17 */
    0x667F3BCD, 0x3FF6A09E, /* T[32] = 1.414213562373095145475 */
    0x13B26456, 0xBC9BDD34, /* D[32] = -9.6672933134529130e-17 */
    0x3C651A2F, 0x3FF6DFB2, /* T[33] = 1.429613338391970023267 */
    0x683C88AB, 0xBC6BBE3A, /* D[33] = -1.2031642489053655e-17 */
    0xE8EC5F74, 0x3FF71F75, /* T[34] = 1.445180806977046650275 */
    0x86887A99, 0xBC816E47, /* D[34] = -3.0237581349939875e-17 */
    0x564267C9, 0x3FF75FEB, /* T[35] = 1.460917794180647044655 */
    0x57316DD3, 0xBC902459, /* D[35] = -5.6003771860752163e-17 */
    0x73EB0187, 0x3FF7A114, /* T[36] = 1.476826145939499346227 */
    0xEE04992F, 0xBC841577, /* D[36] = -3.4839945568927958e-17 */
    0x36CF4E62, 0x3FF7E2F3, /* T[37] = 1.492907728291264835008 */
    0xBA15797E, 0x3C705D02, /* D[37] = 1.41929201542840360e-17 */
    0x994CCE13, 0x3FF82589, /* T[38] = 1.509164427593422841412 */
    0xD41532D8, 0xBC9D4C1D, /* D[38] = -1.0164553277542950e-16 */
    0x9B4492ED, 0x3FF868D9, /* T[39] = 1.525598150744538417101 */
    0x9BD4F6BA, 0xBC9FC6F8, /* D[39] = -1.1024941712342561e-16 */
    0x422AA0DB, 0x3FF8ACE5, /* T[40] = 1.542210825407940744114 */
    0x56864B27, 0x3C96E9F1, /* D[40] = 7.94983480969762076e-17 */
    0x99157736, 0x3FF8F1AE, /* T[41] = 1.559004400237836929222 */
    0xA2E3976C, 0x3C85CC13, /* D[41] = 3.78120705335752751e-17 */
    0xB0CDC5E5, 0x3FF93737, /* T[42] = 1.575980845107886496592 */
    0x81B57EBC, 0xBC675FC7, /* D[42] = -1.0136916471278303e-17 */
    0x9FDE4E50, 0x3FF97D82, /* T[43] = 1.593142151342266998881 */
    0x7C1B85D1, 0xBC9D185B, /* D[43] = -1.0094406542311963e-16 */
    0x82A3F090, 0x3FF9C491, /* T[44] = 1.610490331949254283472 */
    0xB071F2BE, 0x3C7C7C46, /* D[44] = 2.47071925697978889e-17 */
    0x7B5DE565, 0x3FFA0C66, /* T[45] = 1.628027421857347833978 */
    0x5D1CD533, 0xBC935949, /* D[45] = -6.7129550847070839e-17 */
    0xB23E255D, 0x3FFA5503, /* T[46] = 1.645755478153964945776 */
    0xDB8D41E1, 0xBC9D2F6E, /* D[46] = -1.0125679913674773e-16 */
    0x5579FDBF, 0x3FFA9E6B, /* T[47] = 1.663676580326736376136 */
    0x0EF7FD31, 0x3C90FAC9, /* D[47] = 5.89099269671309991e-17 */
    0x995AD3AD, 0x3FFAE89F, /* T[48] = 1.681792830507429004072 */
    0x345DCC81, 0x3C97A1CD, /* D[48] = 8.19901002058149703e-17 */
    0xB84F15FB, 0x3FFB33A2, /* T[49] = 1.700106353718523477525 */
    0x3084D708, 0xBC62805E, /* D[49] = -8.0237193703976998e-18 */
    0xF2FB5E47, 0x3FFB7F76, /* T[50] = 1.718619298122477934143 */
    0x7E54AC3B, 0xBC75584F, /* D[50] = -1.8513804182631109e-17 */
    0x904BC1D2, 0x3FFBCC1E, /* T[51] = 1.73733383527370621735  */
    0x7A2D9E84, 0x3C823DD0, /* D[51] = 3.16438929929295719e-17 */
    0xDD85529C, 0x3FFC199B, /* T[52] = 1.756252160373299453511 */
    0x895048DD, 0x3C811065, /* D[52] = 2.96014069544887343e-17 */
    0x2E57D14B, 0x3FFC67F1, /* T[53] = 1.775376492526521188253 */
    0xFF483CAD, 0x3C92884D, /* D[53] = 6.42973179655657173e-17 */
    0xDCEF9069, 0x3FFCB720, /* T[54] = 1.7947090750031071682   */
    0xD1E949DB, 0x3C7503CB, /* D[54] = 1.82274584279120882e-17 */
    0x4A07897C, 0x3FFD072D, /* T[55] = 1.814252175500398855945 */
    0x43797A9C, 0xBC9CBC37, /* D[55] = -9.9695315389203494e-17 */
    0xDCFBA487, 0x3FFD5818, /* T[56] = 1.834008086409342430656 */
    0xD75B3707, 0x3C82ED02, /* D[56] = 3.28310722424562714e-17 */
    0x03DB3285, 0x3FFDA9E6, /* T[57] = 1.853979125083385470774 */
    0x696DB532, 0x3C9C2300, /* D[57] = 9.76188749072759400e-17 */
    0x337B9B5F, 0x3FFDFC97, /* T[58] = 1.874167634110299962558 */
    0x4F184B5C, 0xBC91A5CD, /* D[58] = -6.1227634130041420e-17 */
    0xE78B3FF6, 0x3FFE502E, /* T[59] = 1.894575981586965607306 */
    0x80A9CC8F, 0x3C839E89, /* D[59] = 3.40340353521652984e-17 */
    0xA2A490DA, 0x3FFEA4AF, /* T[60] = 1.915206561397147400072 */
    0x179C2893, 0xBC9E9C23, /* D[60] = -1.0619946056195963e-16 */
    0xEE615A27, 0x3FFEFA1B, /* T[61] = 1.936061793492294347274 */
    0x86A4B6B0, 0x3C9DC7F4, /* D[61] = 1.03323859606763264e-16 */
    0x5B6E4540, 0x3FFF5076, /* T[62] = 1.957144124175400179411 */
    0x2DD8A18B, 0x3C99D3E1, /* D[62] = 8.96076779103666767e-17 */
    0x819E90D8, 0x3FFFA7C1, /* T[63] = 1.978456026387950927869 */
    0xF3A5931E, 0x3C874853, /* D[63] = 4.03887531092781669e-17 */
    0x00000000, 0x40000000, /* T[64] = 2                       */
    0x00000000, 0x00000000, /* D[64] = 0                       */
    /* TWO_TO_THE_K_DIV_LN2 = 2^6/ln(2.0) rounded to double */
    0x652B82FE, 0x40571547, /* 92.332482616893658 */
    /* Right Shifter */
    0x00000000, 0x43380000, /* RS = 2^52 + 2^51 */
    /* Coefficients for exp(R) - 1 approximation by polynomial p(R) */
    0x00000000, 0x3FE00000, /* A2 = .500000000000000 */
    0x555548F8, 0x3FC55555, /* A3 = .166666666666579 */
    0x55558FCC, 0x3FA55555, /* A4 = .041666666666771 */
    0x3AAF20D3, 0x3F811112, /* A5 = .008333341995140 */
    0x1C2A3FFD, 0x3F56C16A, /* A6 = .001388887045923 */
    /* Overflow Threshold */
    0x8FB9F87E, 0x408633CE, /* OVF = 710.475860073943977 */
    /* Two parts of ln(2.0)/65 */
    0xFEFA0000, 0x3F862E42, /* LOG_HI = .010830424696223 */
    0xBC9E3B3A, 0x3D1CF79A, /* LOG_LO = 2.572804622327669e-14 */
    /* HUGE_VALUE value to process overflow */
    0xFFFFFFFF, 0x7FEFFFFF,
    /* Double precision constants: 0.0, 1.0 */
    0x00000000, 0x00000000, 0x00000000, 0x3FF00000,
    /* EXP_OF_X_DIV_2_THRESHOLD = (52+10)/2*ln(2) rounded to double */
    0xE7026820, 0x40357CD0};
/*
//
//   Implementation of HA (High Accuracy) version of double precision vector
//   hyperbolic cosine function starts here.
//
*/
__attribute__((always_inline)) inline int
__ocl_svml_internal_dcosh_ha(double *a, double *r) {
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
  //_IML_DCOREFN_PROLOG2_IN_C(0, _IML_MODEX87_NEAR53_IN_C,
  //                                                _MODE_UNCHANGED, n, a, r );
  /* Set all bits of scale to 0.                                           */
  /* Only bits of exponent field will be updated then before each use of   */
  /* scale. Notice, that all other bits (i.e. sign and significand) should */
  /* be kept 0 across iterations. Otherwise, they should be explicitly set */
  /* to 0 before each use of scale                                         */
  scale = ((__constant double *)__dcosh_ha_CoutTab)[141];
  /* Filter out INFs and NaNs */
  if ((((((_iml_v2_dp_union_t *)&(*a))->dwords.hi_dword >> 20) & 0x7FF) !=
       0x7FF)) {
    /* Here if argument is finite double precision number */
    /* Save intermediate (*a) to absAi and clear sign bit of absAi */
    /* since cosh(x) == cosh(-x)                                   */
    absAi = (*a);
    (((_iml_v2_dp_union_t *)&absAi)->dwords.hi_dword =
         (((_iml_v2_dp_union_t *)&absAi)->dwords.hi_dword & 0x7FFFFFFF) |
         ((_iml_uint32_t)(0) << 31));
    /* Check if (*a) falls into "Near 0" range */
    if (((((_iml_v2_dp_union_t *)&absAi)->dwords.hi_dword >> 20) & 0x7FF) >
        (0x3FF - 54)) {
      /* Here if argument is not within "Near 0" interval */
      /* Check if cosh((*a)) overflows */
      if (absAi < ((__constant double *)__dcosh_ha_CoutTab)[137]) {
        /* Here if cosh((*a)) doesn't overflow */
        /* Check if |(*a)| is big enough                */
        /* to approximate cosh((*a)) by exp( |(*a)| )/2 */
        if (absAi >= ((__constant double *)__dcosh_ha_CoutTab)[143]) {
          /* Path 5 */
          /* Range Reduction part, 5a) */
          tmp = absAi * ((__constant double *)__dcosh_ha_CoutTab)[130];
          Nj = (tmp + ((__constant double *)__dcosh_ha_CoutTab)[131]);
          M = (Nj - ((__constant double *)__dcosh_ha_CoutTab)[131]);
          RHi = (M * ((__constant double *)__dcosh_ha_CoutTab)[138]);
          RLo = (M * ((__constant double *)__dcosh_ha_CoutTab)[139]);
          R = (absAi - RHi);
          R = (R - RLo);
          /* Approximation part: polynomial series, 5b) */
          p = (((((((__constant double *)__dcosh_ha_CoutTab)[136] * R +
                   ((__constant double *)__dcosh_ha_CoutTab)[135]) *
                      R +
                  ((__constant double *)__dcosh_ha_CoutTab)[134]) *
                     R +
                 ((__constant double *)__dcosh_ha_CoutTab)[133]) *
                    R +
                ((__constant double *)__dcosh_ha_CoutTab)[132]) *
               R);
          p = (p * R + R);
          /* Final reconstruction starts here, 5c) */
          /* Get N and j from Nj's significand */
          N = (((_iml_v2_dp_union_t *)&Nj)->dwords.lo_dword);
          j = N & ((1 << 6) - 1);
          N = (N >> 6);
          N = (N + 0x3FF);
          /* p = (T[j] * p +  D[j]) + T[j] */
          p = ((__constant double *)__dcosh_ha_CoutTab)[2 * (j)] * p;
          p = (p + ((__constant double *)__dcosh_ha_CoutTab)[2 * (j) + 1]);
          p = (p + ((__constant double *)__dcosh_ha_CoutTab)[2 * (j)]);
          N = ((N - 1) & 0x7FF);
          /* Check if path 5.1) or 5.2) should follow */
          if (N <= (0x7FF - 1)) {
            /* Path 5.1) */
            /* scale = 2^N */
            (((_iml_v2_dp_union_t *)&scale)->dwords.hi_dword =
                 (((_iml_v2_dp_union_t *)&scale)->dwords.hi_dword & 0x800FFFFF) |
                 (((_iml_uint32_t)(N)&0x7FF) << 20));
            /* scale * (T[j] + (D[j] + T[j] * p)) */
            (*r) = scale * p;
          } else {
            /* Path 5.2) "scale overflow" */
            /* scale = 2^(N - 1) */
            (((_iml_v2_dp_union_t *)&scale)->dwords.hi_dword =
                 (((_iml_v2_dp_union_t *)&scale)->dwords.hi_dword & 0x800FFFFF) |
                 (((_iml_uint32_t)(N - 1) & 0x7FF) << 20));
            /* 2.0*(scale * (T[j] + (D[j] + T[j] * p))) */
            p = (p * scale);
            (*r) = ((__constant double *)__dcosh_ha_CoutTab)[128] * p;
          }
        } else {
          /* Path 4 */
          /* Range Reduction part, 4a) */
          tmp = absAi * ((__constant double *)__dcosh_ha_CoutTab)[130];
          Nj = (tmp + ((__constant double *)__dcosh_ha_CoutTab)[131]);
          M = (Nj - ((__constant double *)__dcosh_ha_CoutTab)[131]);
          RHi = M * ((__constant double *)__dcosh_ha_CoutTab)[138];
          RLo = M * ((__constant double *)__dcosh_ha_CoutTab)[139];
          R = (absAi - RHi);
          R = (R - RLo);
          /* Approximation part: polynomial series, 4b) */
          rsq = (R * R);
          podd = (((((__constant double *)__dcosh_ha_CoutTab)[135] * rsq +
                    ((__constant double *)__dcosh_ha_CoutTab)[133]) *
                   rsq) *
                  R);
          peven = ((((((__constant double *)__dcosh_ha_CoutTab)[136] * rsq +
                      ((__constant double *)__dcosh_ha_CoutTab)[134]) *
                     rsq) +
                    ((__constant double *)__dcosh_ha_CoutTab)[132]) *
                   rsq);
          /* Final reconstuction starts here 4c) */
          /* Get N and j from Nj's significand */
          N = (((_iml_v2_dp_union_t *)&Nj)->dwords.lo_dword);
          j = N & ((1 << 6) - 1);
          N = (N >> 6);
          /* Obtain scale = 2^{N - 1 + bias} */
          N = (N + 0x3FF);
          N = N & 0x7FF;
          (((_iml_v2_dp_union_t *)&scale)->dwords.hi_dword =
               (((_iml_v2_dp_union_t *)&scale)->dwords.hi_dword & 0x800FFFFF) |
               (((_iml_uint32_t)(N - 1) & 0x7FF) << 20));
          TpHi = ((__constant double *)__dcosh_ha_CoutTab)[2 * (j)] * scale;
          TpLo = ((__constant double *)__dcosh_ha_CoutTab)[2 * (j) + 1] * scale;
          /* Obtain scale = 2^{-N - 2 + bias} */
          N = 2 * 0x3FF - N;
          (((_iml_v2_dp_union_t *)&scale)->dwords.hi_dword =
               (((_iml_v2_dp_union_t *)&scale)->dwords.hi_dword & 0x800FFFFF) |
               (((_iml_uint32_t)(N - 2) & 0x7FF) << 20));
          TmHi =
              ((__constant double *)__dcosh_ha_CoutTab)[2 * (64 - (j))] * scale;
          TmLo = ((__constant double *)__dcosh_ha_CoutTab)[2 * (64 - (j)) + 1] *
                 scale;
          /* TsH := round to double(TpHi + TmHi)  */
          /* TsL := (TpHi + TmHi) - TsH           */
          /* TsH + TsL exactly equals TpHi + TmHi */
          vtmp1 = ((TpHi) + (TmHi));
          tmp = ((TpHi)-vtmp1);
          vtmp2 = (tmp + (TmHi));
          TsH = vtmp1;
          TsL = vtmp2;
          ;
          TdH = (TpHi - TmHi);
          /* Final summation */
          /* (*r) = (((((( R*(D[j] - MD[j]) + MD[j] )    +   */
          /*      + D[j])+ TsL) + podd*TdH) + peven*TsH) +   */
          /*      + R*TdH) + TsH                             */
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
      } else {
        /* Here if cosh overflows, Path 3 */
        (*r) = ((__constant double *)__dcosh_ha_CoutTab)[140] *
               ((__constant double *)__dcosh_ha_CoutTab)[140];
        //_IML_FUNC_NAME_CALL_EM(dError,(_IML_SCODE_IN_C() =
        //  IML_STATUS_OVERFLOW,i, a, a, r, r, _IML_THISFUNC_NAME));
        nRet = 3;
      }
    } else {
      /* Here if argument is "near zero", Path 2 */
      (*r) = ((__constant double *)__dcosh_ha_CoutTab)[142] + absAi;
    }
  } else {
    /* Here if argument is infinity or NaN, Path 1 */
    (*r) = (*a) * (*a);
  }
  //    _IML_COREFN_EPILOG2_IN_C();
  return nRet;
}
double __ocl_svml_cosh_ha(double x) {
  double r;
  unsigned int vm;
  double va1;
  double vr1;
  va1 = x;
  {
    double dN;
    double dM;
    double dR;
    double dR2;
    double dTp[2];
    double dTn;
    double dGmjp;
    double dGmjn;
    double dOut;
    double dXSign;
    unsigned long lM;
    double dAbsX;
    unsigned int iAbsX;
    unsigned int iIndex;
    unsigned long lX;
    unsigned int iRangeMask;
    double dbInvLn2;
    double dbShifter;
    double dbLn2[2];
    double dPC[5];
    unsigned long lExpMask;
    unsigned int iIndexMask;
    unsigned int iDomainRange;
    dbInvLn2 = as_double(__ocl_svml_internal_dcosh_ha_data._dbInvLn2);
    dbShifter = as_double(__ocl_svml_internal_dcosh_ha_data._dbShifter);
    iIndexMask = (__ocl_svml_internal_dcosh_ha_data._iIndexMask);
    // VLOAD_CONST( D, dPC[0],         TAB._dPC1 );
    dPC[1] = as_double(__ocl_svml_internal_dcosh_ha_data._dPC2);
    dPC[2] = as_double(__ocl_svml_internal_dcosh_ha_data._dPC3);
    dPC[3] = as_double(__ocl_svml_internal_dcosh_ha_data._dPC4);
    dPC[4] = as_double(__ocl_svml_internal_dcosh_ha_data._dPC5);
    dXSign = as_double(__ocl_svml_internal_dcosh_ha_data._dSign);
    iDomainRange = (__ocl_svml_internal_dcosh_ha_data._iDomainRange);
    dbLn2[0] = as_double(__ocl_svml_internal_dcosh_ha_data._dbLn2hi);
    dbLn2[1] = as_double(__ocl_svml_internal_dcosh_ha_data._dbLn2lo);
    /* ............... Abs argument ............................ */
    dAbsX = as_double((~(as_ulong(dXSign)) & as_ulong(va1)));
    /* ............... Load argument ........................... */
    dM = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(
        dAbsX, dbInvLn2, dbShifter); // dM = x*2^K/log(2) + RShifter
    /* ...............Check for overflow\underflow ............. */
    lX = as_ulong(dAbsX);                              // lX = x
    iAbsX = ((unsigned int)((unsigned long)lX >> 32)); //
    iRangeMask = ((unsigned int)(-(signed int)((signed int)iAbsX >
                                               (signed int)iDomainRange)));
    vm = 0;
    vm = iRangeMask;
    /* .............. Index and lookup ......................... */
    lM = as_ulong(dM);
    iIndex = (((unsigned int)lM & (unsigned int)-1));
    iIndex = (iIndex & iIndexMask);
    dTp[0] = as_double(
        ((unsigned long *)(__ocl_svml_internal_dcosh_ha_data
                               ._dbT))[(((0 + iIndex) * (3 * 8)) >> (3)) + 0]);
    dTp[1] = as_double(
        ((unsigned long *)(__ocl_svml_internal_dcosh_ha_data
                               ._dbT))[(((0 + iIndex) * (3 * 8)) >> (3)) + 1]);
    dTn = as_double(
        ((unsigned long *)(__ocl_svml_internal_dcosh_ha_data
                               ._dbT))[(((0 + iIndex) * (3 * 8)) >> (3)) + 2]);
    /* ................... R ................................... */
    dN = (dM - dbShifter); // dN = dM - RShifter
    dR = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(
        -(dbLn2[0]), dN, dAbsX); // dR = dX - dN*Log2_hi/2^K
    dR = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(-(dbLn2[1]), dN,
                                                dR); // dR = dX - dN*Log2_hi/2^K
    // VMuL( D, dOut[0], /*=*/ dbLn2[0], dN );
    // VMuL( D, dOut[1], /*=*/ dbLn2[1], dN );
    // VSUB_HL_HHL(D, dR, dAbsX, dOut);
    /* ............... dTp,dTn * 2^N,2^(-N) .................... */
    lM = ((unsigned long)(lM) << ((52 - 7)));
    lExpMask = (__ocl_svml_internal_dcosh_ha_data._lExpMask);
    lM = (lM & lExpMask); // lM now is an EXP(2^N)
    lX = as_ulong(dTp[0]);
    lX = (lX + lM);
    dTp[0] = as_double(lX);
    lX = as_ulong(dTp[1]);
    lX = (lX + lM);
    dTp[1] = as_double(lX); // dTp[.] = dTp[.]*2^N
    lX = as_ulong(dTn);
    lX = (lX - lM);
    dTn = as_double(lX); // dTn *= 2^-N
    dGmjn = (dTp[0] - dTn);
    dGmjp = (dTp[0] + dTn);
    dR2 = (dR * dR);
    /* poly(r) = Gmjp(1 + a2*r^2 + a4*r^4) + Gmjn*(r+ a3*r^3 +a5*r^5)       = */
    /*   = Gmjp_h +Gmjp_l+ Gmjp*r^2*(a2 + a4*r^2) + Gmjn*(r+ r^3*(a3 +a5*r^2) */
    dM = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dPC[4], dR2,
                                                dPC[2]); // dM=(a3 +a5*r^2)
    dOut = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dPC[3], dR2,
                                                  dPC[1]); // dOut=(a2 + a4*r^2)
    dM = (dM * dR2);     // dM=r^2*(a3 +a5*r^2)
    dOut = (dOut * dR2); // dOut=r^2*(a2 + a4*r^2)
    // test VMuL( D, dOut, /*=*/ dOut, dGmjp );//error gets big
    dOut = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(
        dOut, dGmjp, dTp[1]); // dOut=Gmjp*r^2*(a2 + a4*r^2)+Gmjp_l(1)
    dM = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dM, dR,
                                                dR); // dM= r + r^3*(a3 +a5*r^2)
    dOut = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(
        dM, dGmjn, dOut); // dOut=Gmjp*r^2*(a2 + a4*r^2)+Gmjp_l(1) + Gmjn*(r+
                          // r^3*(a3 +a5*r^2)
    dOut = (dOut + dTn); // dOut=Gmjp*r^2*(a2 + a4*r^2)+Gmjp_l(1) + Gmjn*(r+
                         // r^3*(a3 +a5*r^2) + Gmjp_l(2)
    /* ................... Ret H ...................... */
    vr1 = (dOut + dTp[0]); // Gmjp_h +Gmjp_l+ Gmjp*r^2*(a2 + a4*r^2) + Gmjn*(r+
                           // r^3*(a3 +a5*r^2)
  }
  if (__builtin_expect((vm) != 0, 0)) {
    double __cout_a1;
    double __cout_r1;
    ((double *)&__cout_a1)[0] = va1;
    ((double *)&__cout_r1)[0] = vr1;
    __ocl_svml_internal_dcosh_ha(&__cout_a1, &__cout_r1);
    vr1 = ((double *)&__cout_r1)[0];
  }
  r = vr1;
  return r;
}

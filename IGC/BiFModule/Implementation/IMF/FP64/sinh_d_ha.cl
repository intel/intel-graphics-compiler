/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
/*
// ALGORITHM DESCRIPTION:
//  *
//  *  Compute sinh(x) as (exp(x)-exp(-x))/2,
//  *  where exp is calculated as
//  *  exp(M*ln2 + ln2*(j/2^k) + r) = 2^M * 2^(j/2^k) * exp(r)
//  *
//  *  Special cases:
//  *
//  *  sinh(NaN) = quiet NaN, and raise invalid exception
//  *  sinh(INF) = that INF
//  *  sinh(x)   = x for subnormals
//  *  sinh(x) overflows for big x and returns MAXLOG+log(2)
//
*/
#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF
typedef struct {
  /* Shared */
  unsigned long _dExp_tbl_PH[16];
  unsigned long _dExp_tbl_PL[16];
  unsigned long _dExp_tbl_NH[16];
  unsigned long _dExp_tbl_NL[16];
  unsigned long _dbShifter_UISA;
  unsigned long _dbShifterP1_UISA;
  unsigned int _iDomainRange_UISA;
  unsigned long _lIndexMask_UISA; //(1<<K)1-
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
  unsigned long _dSign; // 0x8000000000000000
  unsigned long _dHalf;
  unsigned long _dZero;
  /* Shared name but with different value */
  unsigned long _dbT[256][8]; // precalc poly coeff
  unsigned long _dbShifter;
  unsigned int _iDomainRange;
  /* Not shared */
  unsigned int _iIndexMask;
  unsigned long _dM1; // 0x4338000000000001
} __ocl_svml_internal_dsinh_ha_data_t;
//
static __ocl_svml_internal_dsinh_ha_data_t __ocl_svml_internal_dsinh_ha_data = {
    {/*== _dExp_tbl_PH ==*/
     0x3fe0000000000000uL, 0x3fe0b5586cf9890fuL, 0x3fe172b83c7d517buL,
     0x3fe2387a6e756238uL, 0x3fe306fe0a31b715uL, 0x3fe3dea64c123422uL,
     0x3fe4bfdad5362a27uL, 0x3fe5ab07dd485429uL, 0x3fe6a09e667f3bcduL,
     0x3fe7a11473eb0187uL, 0x3fe8ace5422aa0dbuL, 0x3fe9c49182a3f090uL,
     0x3feae89f995ad3aduL, 0x3fec199bdd85529cuL, 0x3fed5818dcfba487uL,
     0x3feea4afa2a490dauL}, /* dTp_h */
    {0x0000000000000000uL, 0x3c88a62e4adc610buL, 0xbc719041b9d78a76uL,
     0x3c89b07eb6c70573uL, 0x3c76f46ad23182e4uL, 0x3c7ada0911f09ebcuL,
     0x3c6d4397afec42e2uL, 0x3c86324c054647aduL, 0xbc8bdd3413b26456uL,
     0xbc741577ee04992fuL, 0x3c86e9f156864b27uL, 0x3c6c7c46b071f2beuL,
     0x3c87a1cd345dcc81uL, 0x3c711065895048dduL, 0x3c72ed02d75b3707uL,
     0xbc8e9c23179c2893uL}, /* dTp_l */
    {0x3fe0000000000000uL, 0x3fdea4afa2a490dauL, 0x3fdd5818dcfba487uL,
     0x3fdc199bdd85529cuL, 0x3fdae89f995ad3aduL, 0x3fd9c49182a3f090uL,
     0x3fd8ace5422aa0dbuL, 0x3fd7a11473eb0187uL, 0x3fd6a09e667f3bcduL,
     0x3fd5ab07dd485429uL, 0x3fd4bfdad5362a27uL, 0x3fd3dea64c123422uL,
     0x3fd306fe0a31b715uL, 0x3fd2387a6e756238uL, 0x3fd172b83c7d517buL,
     0x3fd0b5586cf9890fuL}, /* dTn_h */
    {0x0000000000000000uL, 0xbc7e9c23179c2893uL, 0x3c62ed02d75b3707uL,
     0x3c611065895048dduL, 0x3c77a1cd345dcc81uL, 0x3c5c7c46b071f2beuL,
     0x3c76e9f156864b27uL, 0xbc641577ee04992fuL, 0xbc7bdd3413b26456uL,
     0x3c76324c054647aduL, 0x3c5d4397afec42e2uL, 0x3c6ada0911f09ebcuL,
     0x3c66f46ad23182e4uL, 0x3c79b07eb6c70573uL, 0xbc619041b9d78a76uL,
     0x3c78a62e4adc610buL}, /* dTn_l */
    0x42F8000000000000uL,   /* _dbShifter_UISA    */
    0x42F8000000000001uL,   /* _dbShifterP1_UISA  */
    0x4084ee33u,            /* _iDomainRange_UISA */
    0x000000000000000FuL,   /* _lIndexMask_UISA   */
    0x3fe0000000000000uL,   /* _dPC2_UISA         */
    0x3fc5555555555ca6uL,   /* _dPC3_UISA         */
    0x3fa5555555554b4cuL,   /* _dPC4_UISA         */
    0x3f8111110edc6edauL,   /* _dPC5_UISA         */
    0x3f56c16c1902580buL,   /* _dPC6_UISA         */
    0x3f2a026d06015ec6uL,   /* _dPC7_UISA         */
    0x3efa01ac1b806259uL,   /* _dPC8_UISA         */
    0x3FF71547652B82FEuL,   /* _dbInvLn2 = 1/log(2) */
    0x3FE62E42FEFA0000uL,   /* _dbLn2hi  = log(2) hi*/
    0x3D7CF79ABC9E3B3AuL,   /* _dbLn2lo  = log(2) lo*/
    0x8000000000000000uL,   /* _dSign */
    0x3FE0000000000000uL,   /* _dHalf */
    0x0000000000000000uL,   /* _dZero */
    {
        //_dbT
        {0x0000000000000000uL, 0x3ff0000000000000uL, 0x3fc5555555555555uL,
         0x3f81111111111f3euL, 0x3f2a01a016b6dd67uL, 0x3ec71e3c853d177fuL,
         0x3e522e9439e95d3duL,
         0x3e84b525531b70dcuL}, // log2(rel_err)=
                                // -6.764683745905699485573307501187278555964319079196e1
        {0x3b792ed7a4eb7f9fuL, 0x3ff0000000000000uL, 0x3fc5555555555555uL,
         0x3f81111111111a4duL, 0x3f2a01a01850be39uL, 0x3ec71e0ae7094157uL,
         0x3e576ddf40ac3213uL,
         0x3e6e9ecfe4c85d2fuL}, // log2(rel_err)=
                                // -6.764683745905699485573307501187278555964319079196e1
        {0x3fc64ab8f61134fauL, 0x3ff03da6eb6f9076uL, 0x3fb64ab8f61135c3uL,
         0x3fc5a78939ea0b7euL, 0x3f7db8f69c6fe818uL, 0x3f8152d43101f555uL,
         0x3f2fb4b93d671dfauL,
         0x3f2a653b035954f8uL}, // log2(rel_err)=
                                // -5.76382330348443048251283471472561359405517578125e1
        {0x3fd0d2d06841b373uL, 0x3ff08b26bb25bd4buL, 0x3fc0d2d06841b042uL,
         0x3fc60ede4edcb4d0uL, 0x3f866e6b35eab1e3uL, 0x3f81a57eb6edc3bcuL,
         0x3f37ed7978b9b197uL,
         0x3f2adefff8e9d379uL}, // log2(rel_err)=
                                // -5.49992264001424899788617040030658245086669921875e1
        {0x3fd6a09e667f3bcduL, 0x3ff0f876ccdf6cdauL, 0x3fc6a09e667f2ecduL,
         0x3fc6a09e667ee8afuL, 0x3f8e2b7de096213auL, 0x3f821a18681e488duL,
         0x3f40172e25db93f5uL,
         0x3f2b8f02f3ca49c0uL}, // log2(rel_err)=
                                // -5.369916919811394251382807851769030094146728515625e1
        {0x3fdc99efaf1f1790uL, 0x3ff186695662daf8uL, 0x3fcc99efaf1f0c36uL,
         0x3fc75de1c883849auL, 0x3f93114a75b901aeuL, 0x3f82b1818073bfe8uL,
         0x3f4456ca1adb2b32uL,
         0x3f2c76ae1cf65df8uL}, // log2(rel_err)=
                                // -5.41653600496072016312609775923192501068115234375e1
        {0x3fe165209441f823uL, 0x3ff2360f4f39d79duL, 0x3fd165209441f1f6uL,
         0x3fc8481469a1fbe6uL, 0x3f973180c6ac52d5uL, 0x3f836cdd3eddea4fuL,
         0x3f48bd7610f5234cuL,
         0x3f2d916dc0a15908uL}, // log2(rel_err)=
                                // -5.429720050803388176063890568912029266357421875e1
        {0x3fe49ebcbebcfbcauL, 0x3ff308ba7d9d26a3uL, 0x3fd49ebcbebcf3bbuL,
         0x3fc960f8a77bd73buL, 0x3f9b7e50ffb0a700uL, 0x3f844d93d0b3c4fduL,
         0x3f4d53a823a4db49uL,
         0x3f2ee96c797fbf51uL}, // log2(rel_err)=
                                // -5.418339118917190688762275385670363903045654296875e1
        {0x3fe8000000000000uL, 0x3ff4000000000000uL, 0x3fd800000000019fuL,
         0x3fcaaaaaaaaaa78duL, 0x3f9ffffffebbf632uL, 0x3f855555563eeb8euL,
         0x3f51115958c0b333uL,
         0x3f3040fc45cdd907uL}, // log2(rel_err)=
                                // -5.818734773818733430061911349184811115264892578125e1
        {0x3feb8f6a41bbb9d4uL, 0x3ff51dbb581cc60cuL, 0x3fdb8f6a41bbb9a5uL,
         0x3fcc27a4757b93ceuL, 0x3fa25f9c2b5c8ac2uL, 0x3f86861d32668830uL,
         0x3f539977fd9fe7c1uL,
         0x3f312840d7c5bbb9uL}, // log2(rel_err)=
                                // -5.678820476820003904094846802763640880584716796875e1
        {0x3fef53d42e0cb93fuL, 0x3ff66411fd5d118buL, 0x3fdf53d42e0cbbe8uL,
         0x3fcddac2a726b076uL, 0x3fa4e28d7322e23buL, 0x3f87e2355690b628uL,
         0x3f56475e52f630a2uL,
         0x3f323203b39e4892uL}, // log2(rel_err)=
                                // -5.84364594351771842184462002478539943695068359375e1
        {0x3ff1aa3e2cf0d60cuL, 0x3ff7d5777d7b7e44uL, 0x3fe1aa3e2cf0cd10uL,
         0x3fcfc749fca3f547uL, 0x3fa78da83da34031uL, 0x3f896c3b5d0119c2uL,
         0x3f591fa7f8e3e184uL,
         0x3f335858c937eb39uL}, // log2(rel_err)=
                                // -5.385272053327020103097311221063137054443359375e1
        {0x3ff3cc8a99af5453uL, 0x3ff974b2334f2346uL, 0x3fe3cc8a99af5577uL,
         0x3fd0f876ccdf715cuL, 0x3faa660e2140d6f2uL, 0x3f8b27247956e5d5uL,
         0x3f5c290cedd745a3uL,
         0x3f34b0714b608ae5uL}, // log2(rel_err)=
                                // -5.797693960117280909116743714548647403717041015625e1
        {0x3ff614e9e783db96uL, 0x3ffb44e09cd1661fuL, 0x3fe614e9e783ecb2uL,
         0x3fd22deb13369498uL, 0x3fad7137da1ffd4fuL, 0x3f8d1644c1e20b15uL,
         0x3f5f68d2c3a19cd4uL,
         0x3f362fad29a3aba7uL}, // log2(rel_err)=
                                // -5.35577632990202090468301321379840373992919921875e1
        {0x3ff887bfd8149ccbuL, 0x3ffd497f5aa10a91uL, 0x3fe887bfd814952euL,
         0x3fd38654e715c7fauL, 0x3fb05a7fe5f241b4uL, 0x3f8f3d54c4da3d93uL,
         0x3f6171b57349f09buL,
         0x3f37c8b345e9bb6duL}, // log2(rel_err)=
                                // -5.44619092017332917521343915723264217376708984375e1
        {0x3ffb29c1d56bfa58uL, 0x3fff866fe48b4eb7uL, 0x3feb29c1d56bf52fuL,
         0x3fd5044a985cbb72uL, 0x3fb21bd68e798ebcuL, 0x3f90d03bb67e5330uL,
         0x3f63511407e5236duL,
         0x3f399c32049b0299uL}, // log2(rel_err)=
                                // -5.501502036500354364534359774552285671234130859375e1
        {0x3ffe000000000000uL, 0x4001000000000000uL, 0x3fee000000000207uL,
         0x3fd6aaaaaaaaa807uL, 0x3fb3ffffff356d03uL, 0x3f92222222e7d2d3uL,
         0x3f6555afb1488710uL,
         0x3f3ba1ad28788b56uL}, // log2(rel_err)=
                                // -5.820909274899700136529645533300936222076416015625e1
        {0x400087f775957457uL, 0x40025d7903652ea0uL, 0x3ff087f775956971uL,
         0x3fd87ca159dbc3dbuL, 0x3fb60a9f49b31ae4uL, 0x3f9396e79abf8340uL,
         0x3f6782b8980ef6d8uL,
         0x3f3dd0948dc8050cuL}, // log2(rel_err)=
                                // -5.35185107916941973371649510227143764495849609375e1
        {0x40022fb90d66e078uL, 0x4003de4306fc8db3uL, 0x3ff22fb90d66d9c1uL,
         0x3fda7daeb3fb29f7uL, 0x3fb83fa1684f595buL, 0x3f953158a0aa47c1uL,
         0x3f69dd97d80472eauL,
         0x3f40232d42009a85uL}, // log2(rel_err)=
                                // -5.42529645192197875758211011998355388641357421875e1
        {0x4003fa73ab24d520uL, 0x40058541ff477f2duL, 0x3ff3fa73ab24df1euL,
         0x3fdcb1ad545fc55auL, 0x3fbaa344e0e3a32buL, 0x3f96f48a8dfd1848uL,
         0x3f6c6ab107b2e1f4uL,
         0x3f418182d9004e13uL}, // log2(rel_err)=
                                // -5.42667400168163425178136094473302364349365234375e1
        {0x4005eb99734b41eeuL, 0x400755a359b335abuL, 0x3ff5eb99734b4924uL,
         0x3fdf1cd9cceef734uL, 0x3fbd3a21ec695572uL, 0x3f98e3e16df9767euL,
         0x3f6f2dadca0b27d1uL,
         0x3f42f76ed23fe76cuL}, // log2(rel_err)=
                                // -5.4993773439319312501538661308586597442626953125e1
        {0x400806e66b80ef8auL, 0x400952e418d4522cuL, 0x3ff806e66b80f5afuL,
         0x3fe0e1ed65e3017euL, 0x3fc004999bc7d297uL, 0x3f9b0315604ed16fuL,
         0x3f711657f13c992auL,
         0x3f4496fd2a1aede3uL}, // log2(rel_err)=
                                // -5.546737495836126896620044135488569736480712890625e1
        {0x400a5067a90945f5uL, 0x400b80d789ac6167uL, 0x3ffa5067a9093dfcuL,
         0x3fe255e50672ae51uL, 0x3fc18aefc6994871uL, 0x3f9d563b5c5824cbuL,
         0x3f72b6740dcc75e3uL,
         0x3f4655bdfb435943uL}, // log2(rel_err)=
                                // -5.44902135559456155533553101122379302978515625e1
        {0x400ccc831b17b9fbuL, 0x400de3ae9edf8f12uL, 0x3ffccc831b17c634uL,
         0x3fe3ed1f1495444cuL, 0x3fc333020ff71255uL, 0x3f9fe1cb37217007uL,
         0x3f747b368348cd71uL,
         0x3f484ee21dd54e33uL}, // log2(rel_err)=
                                // -5.45447522062957119715065346099436283111572265625e1
        {0x400f800000000000uL, 0x4010400000000000uL, 0x3fff800000000221uL,
         0x3fe5aaaaaaaaa825uL, 0x3fc4ffffff2b4989uL, 0x3fa1555556123a6fuL,
         0x3f7666c54767292buL,
         0x3f4a699a40320d71uL}, // log2(rel_err)=
                                // -5.821319721068672237151986337266862392425537109375e1
        {0x401138080ac35a32uL, 0x4011ad686e3748c4uL, 0x400138080ac354eduL,
         0x3fe791e092f44834uL, 0x3fc6f5600f1dcdf7uL, 0x3fa2db1a16f6566cuL,
         0x3f787d5b435305c9uL,
         0x3f4cb93e79163f40uL}, // log2(rel_err)=
                                // -5.447877842055104480323279858566820621490478515625e1
        {0x4012d12ccaff016euL, 0x40133ccf49646cbduL, 0x4002d12ccafefcdeuL,
         0x3fe9a669b73052d7uL, 0x3fc916e66485281fuL, 0x3fa485216f49a150uL,
         0x3f7ac34f746fe73buL,
         0x3f4f4049dfce81e4uL}, // log2(rel_err)=
                                // -5.474732092769006186472324770875275135040283203125e1
        {0x40148e810ab1d4e5uL, 0x4014f1349fba7f68uL, 0x40048e810ab1e3a1uL,
         0x3febec462a4e6b5buL, 0x3fcb68ac09b3ed4auL, 0x3fa6569e6c175182uL,
         0x3f7d3d7342a0ccb6uL,
         0x3f5109595ebede7buL}, // log2(rel_err)=
                                // -5.3681294354769903520718798972666263580322265625e1
        {0x4016735d29b23d55uL, 0x4016cddfa34c3a44uL, 0x4006735d29b23dbbuL,
         0x3fee67d4d9bb02aauL, 0x3fcdef26e15e5939uL, 0x3fa85310ac7d917duL,
         0x3f7fee837ec382bduL,
         0x3f5288baf720f209uL}, // log2(rel_err)=
                                // -5.726559945670236828618726576678454875946044921875e1
        {0x401883658c803487uL, 0x4018d664f7d50d30uL, 0x400883658c8037eeuL,
         0x3ff08eedfa8df7eduL, 0x3fd057990775d312uL, 0x3faa7e49981f7297uL,
         0x3f816ed39324d271uL,
         0x3f542ea655806364uL}, // log2(rel_err)=
                                // -5.686606144100885984471460687927901744842529296875e1
        {0x401ac2919d46703fuL, 0x401b0ead956f371buL, 0x400ac2919d4678d8uL,
         0x3ff209c90e4a4496uL, 0x3fd1d70bbc98267cuL, 0x3facdc74d399638auL,
         0x3f8307f6369be887uL,
         0x3f55ffcce377281buL}, // log2(rel_err)=
                                // -5.50367002305036976395058445632457733154296875e1
        {0x401d35336c82a9e4uL, 0x401d7afe4d749f2auL, 0x400d35336c82b221uL,
         0x3ff3a75433a31e8buL, 0x3fd378ccf16be56auL, 0x3faf72204c98cffeuL,
         0x3f84c595e0ef8890uL,
         0x3f57f691edf7ab5duL}, // log2(rel_err)=
                                // -5.527952335490692092889730702154338359832763671875e1
        {0x401fe00000000000uL, 0x4020100000000000uL, 0x400fe00000000227uL,
         0x3ff56aaaaaaaa82auL, 0x3fd53fffff28d875uL, 0x3fb1222222ddbfe0uL,
         0x3f86ab0aa8ce5154uL,
         0x3f5a1b9534ef1b73uL}, // log2(rel_err)=
                                // -5.82161493234420248654714669100940227508544921875e1
        {0x4021640c300ed3a9uL, 0x4021816448ebcf4euL, 0x4011640c300ec778uL,
         0x3ff757306139f365uL, 0x3fd7301042a346bfuL, 0x3fb2ac26d2fb59e3uL,
         0x3f88bbb17009b84cuL,
         0x3f5c6b144531a500uL}, // log2(rel_err)=
                                // -5.34378209106807418038442847318947315216064453125e1
        {0x4022f989ba6509abuL, 0x4023147259fe647euL, 0x4012f989ba65164duL,
         0x3ff9709877fe561euL, 0x3fd94cb79f3d1c05uL, 0x3fb45a137470c603uL,
         0x3f8afd62537e3cf2uL,
         0x3f5f0c53ee20a15buL}, // log2(rel_err)=
                                // -5.379904334717839020640894887037575244903564453125e1
        {0x4024b384629514d7uL, 0x4024cc3147d73f78uL, 0x4014b38462950bc6uL,
         0x3ffbbaec5fc962deuL, 0x3fdb9a05da684b93uL, 0x3fb62f239334a012uL,
         0x3f8d712c5b523f1auL,
         0x3f60e420b172a160uL}, // log2(rel_err)=
                                // -5.404223295294016082834787084721028804779052734375e1
        {0x4026954e174bfc2fuL, 0x4026abeeb5b27b6buL, 0x4016954e174bf28duL,
         0x3ffe3a939ceda933uL, 0x3fde1c6820c64e11uL, 0x3fb82edc934bfdffuL,
         0x3f900f333907d00buL,
         0x3f6269adbbc0b823uL}, // log2(rel_err)=
                                // -5.407527147581652826602294226177036762237548828125e1
        {0x4028a28554c005c6uL, 0x4028b7452f953bf0uL, 0x4018a28554c010d2uL,
         0x40007a2e1fb8f055uL, 0x3fe06c58e14be160uL, 0x3fba5d168938cb46uL,
         0x3f91851bbcb5e39fuL,
         0x3f6418b6073b91f6uL}, // log2(rel_err)=
                                // -5.445071640750001762398824212141335010528564453125e1
        {0x402adf1c1a55bad2uL, 0x402af223185fec89uL, 0x401adf1c1a55b6e7uL,
         0x4001f6c2103fe667uL, 0x3fe1ea12bc42a9e5uL, 0x3fbcbe0354ec567auL,
         0x3f931c0442bdd602uL,
         0x3f65e503ae12b240uL}, // log2(rel_err)=
                                // -5.5286529954752808180273859761655330657958984375e1
        {0x402d4f5f80dd65deuL, 0x402d60d23919e32fuL, 0x401d4f5f80dd7570uL,
         0x400395e17b66cfe4uL, 0x3fe38a3fa8b3c02auL, 0x3fbf5635751e204fuL,
         0x3f94d856f54f709duL,
         0x3f67e4a2e22986c5uL}, // log2(rel_err)=
                                // -5.41669640207029630118995555676519870758056640625e1
        {0x402ff80000000000uL, 0x4030040000000000uL, 0x401ff80000000229uL,
         0x40055aaaaaaaa82euL, 0x3fe54fffff282466uL, 0x3fc11555560f9f8buL,
         0x3f96bc1c054876a6uL,
         0x3f6a08144aa2b3e5uL}, // log2(rel_err)=
                                // -5.821490257232267850895368610508739948272705078125e1
        {0x40316f0d3961b206uL, 0x403176633f98f0efuL, 0x40216f0d3961bd17uL,
         0x4007488454cc28efuL, 0x3fe73ebc490429bauL, 0x3fc2a069cce9563cuL,
         0x3f98cc3e79796379uL,
         0x3f6c675fa2bc6156uL}, // log2(rel_err)=
                                // -5.387626141785602840172941796481609344482421875e1
        {0x403303a0f63e8bbbuL, 0x40330a5b1e24e270uL, 0x402303a0f63e83aduL,
         0x400963242830f329uL, 0x3fe95a2bf46b64aauL, 0x3fc44f5030655124uL,
         0x3f9b0aef953e12dfuL,
         0x3f6eedfddd3e7a4buL}, // log2(rel_err)=
                                // -5.408385429371609376403284841217100620269775390625e1
        {0x4034bcc5388de4d3uL, 0x4034c2f071de6f7buL, 0x4024bcc5388de677uL,
         0x400bae95ed28a809uL, 0x3feba65c4a3fe340uL, 0x3fc62544b98d795cuL,
         0x3f9d7ebf9979ac63uL,
         0x3f70e01bf684d929uL}, // log2(rel_err)=
                                // -5.825101552677870131446979939937591552734375e1
        {0x40369dca52b26be5uL, 0x4036a3727a4c0bb4uL, 0x40269dca52b2706auL,
         0x400e2f434dbac108uL, 0x3fee27b86c4a8414uL, 0x3fc825cf6f2dfdaeuL,
         0x3fa01582199d0a18uL,
         0x3f72667199a6af0duL}, // log2(rel_err)=
                                // -5.597740299150007814432683517225086688995361328125e1
        {0x4038aa4d46cffa16uL, 0x4038af7d3d8547a1uL, 0x4028aa4d46cffeb7uL,
         0x401074fe290373acuL, 0x3ff07188d8d6cadauL, 0x3fca54c9e2530a2cuL,
         0x3fa18a848814d664uL,
         0x3f740f15bffa064duL}, // log2(rel_err)=
                                // -5.611413498622398066117966664023697376251220703125e1
        {0x403ae63eb9998d76uL, 0x403aeb00791c19e3uL, 0x402ae63eb9999f67uL,
         0x4011f20050bdb431uL, 0x3ff1eed478ed0d29uL, 0x3fccb666c0349685uL,
         0x3fa3218384e5ccf9uL,
         0x3f75e63b28fa09a5uL}, // log2(rel_err)=
                                // -5.379650129222039822707301937043666839599609375e1
        {0x403d55ea85f414dduL, 0x403d5a4734033432uL, 0x402d55ea85f4159cuL,
         0x40139184cd57533buL, 0x3ff38e9c58b076beuL, 0x3fcf4f3af34e230fuL,
         0x3fa4dcb4be6e1db4uL,
         0x3f77d8a0552303ecuL}, // log2(rel_err)=
                                // -5.7346424483961612850180245004594326019287109375e1
        {0x403ffe0000000000uL, 0x4040010000000000uL, 0x402ffe0000000229uL,
         0x401556aaaaaaa82duL, 0x3ff553ffff280f2duL, 0x3fd1122222dcd67cuL,
         0x3fa6c06058466e2buL,
         0x3f7a0333cda27eecuL}, // log2(rel_err)=
                                // -5.821658794357315258594098850153386592864990234375e1
        {0x404171cd7bb6699euL, 0x404173a2fd443959uL, 0x403171cd7bb66182uL,
         0x401744d951afd2abuL, 0x3ff74267511d0d3auL, 0x3fd29d7ac60392b5uL,
         0x3fa8cf6a35556cdauL,
         0x3f7c551e12ade772uL}, // log2(rel_err)=
                                // -5.39672150700750279384010354988276958465576171875e1
        {0x40430626c534ec3fuL, 0x404307d54f2e81eduL, 0x40330626c534df05uL,
         0x40195fc7143d6889uL, 0x3ff95d8909b6f6cauL, 0x3fd44c9f6a7fcf6auL,
         0x3fab0e52e5b0b58duL,
         0x3f7ee369258d6f48uL}, // log2(rel_err)=
                                // -5.3448454175137015909058391116559505462646484375e1
        {0x4044bf156e0c18d2uL, 0x4044c0a03c603b7cuL, 0x4034bf156e0c1d24uL,
         0x401bab805080605buL, 0x3ffba971e63581cduL, 0x3fd622cd08b51a04uL,
         0x3fad82247564a8f5uL,
         0x3f80de5a81874143uL}, // log2(rel_err)=
                                // -5.588875673061125581853048061020672321319580078125e1
        {0x40469fe9618c07d3uL, 0x4046a1536b726fc7uL, 0x40369fe9618bff39uL,
         0x401e2c6f39ed98e0uL, 0x3ffe2a8c838140f3uL, 0x3fd8238c43f01b0buL,
         0x3fb016c351a85d2auL,
         0x3f82611cc74ac925uL}, // log2(rel_err)=
                                // -5.42171440515481180000278982333838939666748046875e1
        {0x4048ac3f4353f72auL, 0x4048ad8b41014a8duL, 0x4038ac3f4353fa30uL,
         0x402073b22b5620ffuL, 0x400072d4d6b9911euL, 0x3fda52b6b307906fuL,
         0x3fb18bdeb8dc4143uL,
         0x3f840d6e0f34f891uL}, // log2(rel_err)=
                                // -5.7238715154290190412211813963949680328369140625e1
        {0x404ae807616a8220uL, 0x404ae937d14b253cuL, 0x403ae807616a7836uL,
         0x4021f0cfe0dc8798uL, 0x4001f004ec6d6d1euL, 0x3fdcb47fecdd34a5uL,
         0x3fb3223e511cb309uL,
         0x3f85da7cc07ec0b5uL}, // log2(rel_err)=
                                // -5.42556302302703414852658170275390148162841796875e1
        {0x404d578d4739c09duL, 0x404d58a472bd8873uL, 0x403d578d4739b553uL,
         0x4023906da1d35eccuL, 0x40038fb385c50a46uL, 0x3fdf4d7c5ef2f8cbuL,
         0x3fb4dda2f1b17a72uL,
         0x3f87d3bd819ec665uL}, // log2(rel_err)=
                                // -5.4202127209194060242225532419979572296142578125e1
        {0x404fff8000000000uL, 0x4050004000000000uL, 0x403fff800000022auL,
         0x402555aaaaaaa830uL, 0x400554ffff27da49uL, 0x3fe11155560ee8ccuL,
         0x3fb6c171754721d4uL,
         0x3f8a01fc1d2b867buL}, // log2(rel_err)=
                                // -5.8212996286158301018076599575579166412353515625e1
        {0x4051727d8c4b9784uL, 0x405172f2ecaf0b73uL, 0x4041727d8c4b8a9duL,
         0x402743ee90e8ef11uL, 0x4007435213232e4fuL, 0x3fe29cbef9253d91uL,
         0x3fb8d035286d12e4uL,
         0x3f8c538f7fa94f8cuL}, // log2(rel_err)=
                                // -5.336750281283065078241634182631969451904296875e1
        {0x405306c838f28460uL, 0x40530733db70e9ccuL, 0x404306c838f275dbuL,
         0x40295eefcf409edauL, 0x40095e604f09db53uL, 0x3fe44bf33374e881uL,
         0x3fbb0f2bb9cd14a9uL,
         0x3f8ee2448edf1aeeuL}, // log2(rel_err)=
                                // -5.332558315439984397698935936205089092254638671875e1
        {0x4054bfa97b6ba5d2uL, 0x4054c00c2f00ae7duL, 0x4044bfa97b6ba27auL,
         0x402baabae955f1e7uL, 0x400baa374f5e2c2buL, 0x3fe6222f33bfba39uL,
         0x3fbd82ab19b43aa4uL,
         0x3f90da8685ebbb0buL}, // log2(rel_err)=
                                // -5.518571703816618168048080406151711940765380859375e1
        {0x4056a07125426eceuL, 0x4056a0cba7bc08cbuL, 0x4046a07125427395uL,
         0x402e2bba34fabcecuL, 0x400e2b4184f940d0uL, 0x3fe822fb5b59b561uL,
         0x3fc017661fc4abc7uL,
         0x3f92644ce547fbd6uL}, // log2(rel_err)=
                                // -5.585922016006731638526616734452545642852783203125e1
        {0x4058acbbc274f66fuL, 0x4058ad0ec1e04b48uL, 0x4048acbbc274f90fuL,
         0x4030735f2beacc56uL, 0x40107327d6321effuL, 0x3fea5231e732f921uL,
         0x3fc18c354b3eedabuL,
         0x3f940d0470e27182uL}, // log2(rel_err)=
                                // -5.7742914674942795727474731393158435821533203125e1
        {0x405ae8798b5ebf4auL, 0x405ae8c5a756e811uL, 0x404ae8798b5ebf12uL,
         0x4031f083c4e48c75uL, 0x4011f0510722ad6fuL, 0x3fecb4060f1deeb1uL,
         0x3fc322bf8443935buL,
         0x3f95dd92f813e770uL}, // log2(rel_err)=
                                // -5.67794700532021039407482021488249301910400390625e1
        {0x405d57f5f78b2b8cuL, 0x405d583bc26c1d81uL, 0x404d57f5f78b3e91uL,
         0x40339027d6f2f538uL, 0x40138ff94cb48bb5uL, 0x3fef4d0c6d9c8b11uL,
         0x3fc4de837ca44c98uL,
         0x3f97ddcfd8cbd0f1uL}, // log2(rel_err)=
                                // -5.384272148249302603062460548244416713714599609375e1
        {0x405fffe000000000uL, 0x4060001000000000uL, 0x404fffe00000022auL,
         0x4035556aaaaaa830uL, 0x4015553fff27d8f6uL, 0x3ff1112222dbb6b3uL,
         0x3fc6c1b5ba76d86duL,
         0x3f9a01ae172f443duL}, // log2(rel_err)=
                                // -5.821310340660480875385474064387381076812744140625e1
        {0x406172a99070e2fduL, 0x406172c6e889bff9uL, 0x405172a99070e58cuL,
         0x403743b3e0b78b46uL, 0x4017438cbf4f0739uL, 0x3ff29c8fedb8cbf9uL,
         0x3fc8d10ce56614c4uL,
         0x3f9c5ab5b9deb043uL}, // log2(rel_err)=
                                // -5.6714373016067924027083790861070156097412109375e1
        {0x406306f095e1ea68uL, 0x4063070b7e8183c3uL, 0x405306f095e1e3e4uL,
         0x40395eb9fe01c8e8uL, 0x40195e961e33f844uL, 0x3ff44bc80e76f5e4uL,
         0x3fcb0fb4649bdc03uL,
         0x3f9ee8c0bcf69da0uL}, // log2(rel_err)=
                                // -5.434474423172581936114511336199939250946044921875e1
        {0x4064bfce7ec38912uL, 0x4064bfe72ba8cb3duL, 0x4054bfce7ec383d0uL,
         0x403baa898f8b6f47uL, 0x401baa68a9a8272fuL, 0x3ff62207b8ef61bduL,
         0x3fcd82cccb08b007uL,
         0x3fa0da5213eeead3uL}, // log2(rel_err)=
                                // -5.4697548593015966389430104754865169525146484375e1
        {0x4066a0931630088duL, 0x4066a0a9b6ce6f0cuL, 0x4056a09316300857uL,
         0x403e2b8cf3bdf452uL, 0x401e2b6ec7826bbbuL, 0x3ff822d727c0165auL,
         0x3fd017658c05e72auL,
         0x3fa263f638035af5uL}, // log2(rel_err)=
                                // -5.674900034761757439127904945053160190582275390625e1
        {0x4068acdae23d3640uL, 0x4068acefa2180b76uL, 0x4058acdae23d411auL,
         0x4040734a6c102568uL, 0x4020733c94fb0044uL, 0x3ffa52109d03175duL,
         0x3fd18c7428ab279fuL,
         0x3fa4104c9cd98f6auL}, // log2(rel_err)=
                                // -5.44837989474460329120120150037109851837158203125e1
        {0x406ae89615dbce94uL, 0x406ae8a91cd9d8c5uL, 0x405ae89615dbe172uL,
         0x4041f070bde6ddb1uL, 0x4021f0640ba50228uL, 0x3ffcb3e76ec38fc2uL,
         0x3fd323325756fa31uL,
         0x3fa5e45e89e67ab8uL}, // log2(rel_err)=
                                // -5.3716565557074403614024049602448940277099609375e1
        {0x406d5810239f8648uL, 0x406d58219657c2c5uL, 0x405d5810239f988cuL,
         0x40439016643ab90euL, 0x4023900abf85e9bfuL, 0x3fff4cf082f41586uL,
         0x3fd4de925c3bfbbcuL,
         0x3fa7ddb176b8ab99uL}, // log2(rel_err)=
                                // -5.390788878177926335411029867827892303466796875e1
        {0x406ffff800000000uL, 0x4070000400000000uL, 0x405ffff80000022auL,
         0x4045555aaaaaa830uL, 0x4025554fff27d8a1uL, 0x40011115560eea2buL,
         0x3fd6c1c6cbc2d83euL,
         0x3faa019a95b132b0uL}, // log2(rel_err)=
                                // -5.821313022036678574977486277930438518524169921875e1
        {0x407172b4917a35dbuL, 0x407172bbe7806d1auL, 0x406172b4917a449cuL,
         0x404743a534ab75dauL, 0x4027439b682f19e3uL, 0x40029c841931bc98uL,
         0x3fd8d19556cd72dduL,
         0x3fac61c4ce351ddcuL}, // log2(rel_err)=
                                // -5.34202994061102032219423563219606876373291015625e1
        {0x407306faad1dc3eauL, 0x407307016745aa41uL, 0x406306faad1dbf67uL,
         0x40495eac89b1fa79uL, 0x40295ea391fe381fuL, 0x40044bbd4ac6912buL,
         0x3fdb0fd69bb18d64uL,
         0x3faee8e00a1ade3duL}, // log2(rel_err)=
                                // -5.47723296267623567246118909679353237152099609375e1
        {0x4074bfd7bf9981e2uL, 0x4074bfddead2d26duL, 0x4064bfd7bf997c26uL,
         0x404baa7d3918cea3uL, 0x402baa75003a765buL, 0x400621fdda39cd85uL,
         0x3fdd82d53f9eccabuL,
         0x3fb0da4539e05d3buL}, // log2(rel_err)=
                                // -5.459748973547372230541441240347921848297119140625e1
        {0x4076a09b926b6efduL, 0x4076a0a13a93089duL, 0x4066a09b926b6534uL,
         0x404e2b81a36e65b2uL, 0x402e2b7a1a4f52a7uL, 0x400822ce3214d02cuL,
         0x3fe0173c2c325e49uL,
         0x3fb2607de6ce178fuL}, // log2(rel_err)=
                                // -5.405816415909212224732982576824724674224853515625e1
        {0x4078ace2aa2f4635uL, 0x4078ace7da25fb83uL, 0x4068ace2aa2f3a21uL,
         0x405073453c191666uL, 0x40307341c7ed5e5auL, 0x400a52087f784270uL,
         0x3fe18c0825094014uL,
         0x3fb40936d7d5628euL}, // log2(rel_err)=
                                // -5.3902224921880502961357706226408481597900390625e1
        {0x407ae89d387b1267uL, 0x407ae8a1fa3a94f3uL, 0x406ae89d387b1961uL,
         0x4051f06bfc272e77uL, 0x4031f068cef092b2uL, 0x400cb3dfea068b6buL,
         0x3fe322fc85d1fff9uL,
         0x3fb5e0cb9df25babuL}, // log2(rel_err)=
                                // -5.544053166676587096617367933504283428192138671875e1
        {0x407d5816aea49cf7uL, 0x407d581b0b52ac16uL, 0x406d5816aea4af0buL,
         0x40539012078caa05uL, 0x4033900f1c3a355cuL, 0x400f4ce98848e050uL,
         0x3fe4de9616323981uL,
         0x3fb7ddaa0e1e0f00uL}, // log2(rel_err)=
                                // -5.392465055458696809864704846404492855072021484375e1
        {0x407ffffe00000000uL, 0x4080000100000000uL, 0x406ffffe0000022auL,
         0x40555556aaaaa830uL, 0x40355553ff27d88cuL, 0x4011111222dbb70buL,
         0x3fe6c1cb1015c607uL,
         0x3fba0195b5508b67uL}, // log2(rel_err)=
                                // -5.821313688960828613971898448653519153594970703125e1
        {0x408172b751bc8a93uL, 0x408172b9273e1863uL, 0x407172b751bc8bb8uL,
         0x405743a189a8026euL, 0x4037439f16bcb61cuL, 0x40129c8141d50ee9uL,
         0x3fe8d112771556f7uL,
         0x3fbc5a7e9062f907uL}, // log2(rel_err)=
                                // -5.809464338254102955261259921826422214508056640625e1
        {0x408306fd32ecba4auL, 0x408306fee176b3dfuL, 0x407306fd32ecc6f0uL,
         0x40595ea92c9ebfdauL, 0x40395ea6ea9b18c0uL, 0x40144bba6b608fc9uL,
         0x3feb108429a87c0fuL,
         0x3fbef673b9ce59d7uL}, // log2(rel_err)=
                                // -5.3801879655639453403637162409722805023193359375e1
        {0x4084bfda0fcf0016uL, 0x4084bfdb9a9d5439uL, 0x4074bfda0fcefa3buL,
         0x405baa7a237c2676uL, 0x403baa7815df39bbuL, 0x401621fb628de672uL,
         0x3fed82d75483548cuL,
         0x3fc0da41c0f49777uL}, // log2(rel_err)=
                                // -5.457352077398008560749076423235237598419189453125e1
        {0x4086a09db17a4899uL, 0x4086a09f1b842f02uL, 0x4076a09db17a3c6buL,
         0x405e2b7ecf5a371fuL, 0x403e2b7cef02a429uL, 0x401822cc055e8e40uL,
         0x3ff01731d22dbdb1uL,
         0x3fc25d5ef0b8e48euL}, // log2(rel_err)=
                                // -5.37806784358953251512502902187407016754150390625e1
        {0x4088ace49c2bca32uL, 0x4088ace5e8297786uL, 0x4078ace49c2bc0b7uL,
         0x40607343f01b67eduL, 0x4040734313948417uL, 0x401a52066bfaaaafuL,
         0x3ff18c1665356801uL,
         0x3fc40953f2387bc5uL}, // log2(rel_err)=
                                // -5.42040820685149782320877420715987682342529296875e1
        {0x408ae89f0122e35cuL, 0x408ae8a03192c3ffuL, 0x407ae89f0122df09uL,
         0x4061f06acbb720e7uL, 0x4041f06a00d8d0d3uL, 0x401cb3de1a81d694uL,
         0x3ff322c5d47c7bb1uL,
         0x3fc5dd445e80168fuL}, // log2(rel_err)=
                                // -5.5189452229558582985191605985164642333984375e1
        {0x408d58185165e2a3uL, 0x408d58196891666buL, 0x407d58185165ec57uL,
         0x40639010f060f805uL, 0x40439010347ca241uL, 0x401f4ce7e0d9f3bauL,
         0x3ff4de6dc7bbba79uL,
         0x3fc7da456cd460d7uL}, // log2(rel_err)=
                                // -5.498487277201150646988025982864201068878173828125e1
        {0x408fffff80000000uL, 0x4090000040000000uL, 0x407fffff8000022auL,
         0x40655555aaaaa830uL, 0x40455554ff27d886uL, 0x40211111560eea3duL,
         0x3ff6c1cc212ab7f9uL,
         0x3fca01947d3bca24uL}, // log2(rel_err)=
                                // -5.821313865939513476632782840169966220855712890625e1
        {0x409172b801cd1fc1uL, 0x409172b8772d8335uL, 0x408172b801cd1d7fuL,
         0x406743a09ee73e8buL, 0x404743a002601d2buL, 0x40229c80866cbb6cuL,
         0x3ff8d0f1bf2717fbuL,
         0x3fcc5a2d7795d3fbuL}, // log2(rel_err)=
                                // -5.54018012564609563241901923902332782745361328125e1
        {0x409306fdd46077e2uL, 0x409306fe4002f647uL, 0x408306fdd46088d2uL,
         0x40695ea85559bf3fuL, 0x40495ea7c0c268b1uL, 0x40244bb9beaa8d7cuL,
         0x3ffb10af89063a46uL,
         0x3fcef6d7514f058auL}, // log2(rel_err)=
                                // -5.33399751016432759342933422885835170745849609375e1
        {0x4094bfdaa3dc5fa3uL, 0x4094bfdb068ff4acuL, 0x4084bfdaa3dc59c0uL,
         0x406baa795e14fc69uL, 0x404baa78db48825duL, 0x402621fac4a39574uL,
         0x3ffd82d7d59c1b34uL,
         0x3fd0da40c5337a90uL}, // log2(rel_err)=
                                // -5.456759015694299108645282103680074214935302734375e1
        {0x4096a09e393dff00uL, 0x4096a09e93c0789buL, 0x4086a09e393df239uL,
         0x406e2b7e1a554475uL, 0x404e2b7da42f60c0uL, 0x402822cb749ed42fuL,
         0x4000172f3dbcb045uL,
         0x3fd25d579abcd267uL}, // log2(rel_err)=
                                // -5.37191188255788887317976332269608974456787109375e1
        {0x4098ace518aaeb31uL, 0x4098ace56baa5686uL, 0x4088ace518aaeab0uL,
         0x407073439d1c2a8buL, 0x405073436568ff6duL, 0x402a5205cfe07bc8uL,
         0x40018c4330247722uL,
         0x3fd40cbdd00f5c7auL}, // log2(rel_err)=
                                // -5.6563500626710123242446570657193660736083984375e1
        {0x409ae89f734cd799uL, 0x409ae89fbf68cfc2uL, 0x408ae89f734cd8c7uL,
         0x4071f06a7f9b264duL, 0x4051f06a4c3d7a77uL, 0x402cb3dda01806d2uL,
         0x400322e1672bc606uL,
         0x3fd5dd84af03d598uL}, // log2(rel_err)=
                                // -5.779583821363315365715607185848057270050048828125e1
        {0x409d5818ba16340euL, 0x409d5818ffe11500uL, 0x408d5818ba163ba9uL,
         0x40739010aa9617fduL, 0x405390107a8d6d10uL, 0x402f4ce771700c91uL,
         0x4004de63abdce42euL,
         0x3fd7da2bfafea4f0uL}, // log2(rel_err)=
                                // -5.5441532875848309913635603152215480804443359375e1
        {0x409fffffe0000000uL, 0x40a0000010000000uL, 0x408fffffe000022auL,
         0x407555556aaaa830uL, 0x405555553f27d885uL, 0x4031111122dbb70duL,
         0x4006c1cc656fd020uL,
         0x3fda01942f349bd3uL}, // log2(rel_err)=
                                // -5.821313903326244343361395294778048992156982421875e1
        {0x40a172b82dd1450cuL, 0x40a172b84b295de9uL, 0x409172b82dd15299uL,
         0x407743a0643762aeuL, 0x405743a038f34794uL, 0x40329c803f5db718uL,
         0x4008d18e915e7bf1uL,
         0x3fdc61a315f74228uL}, // log2(rel_err)=
                                // -5.35543599905295621965706232003867626190185546875e1
        {0x40a306fdfcbd6748uL, 0x40a306fe17a606e1uL, 0x409306fdfcbd794auL,
         0x40795ea81f887f14uL, 0x40595ea7f64c6c42uL, 0x40344bb9937ea118uL,
         0x400b10ba589cbcb6uL,
         0x3fdef6efab048bc4uL}, // log2(rel_err)=
                                // -5.324438019251882536764242104254662990570068359375e1
        {0x40a4bfdac8dfb786uL, 0x40a4bfdae18c9cc8uL, 0x4094bfdac8dfb9f6uL,
         0x407baa792cbb8e69uL, 0x405baa790a77c160uL, 0x403621fa85ea3abauL,
         0x400d832a804c6861uL,
         0x3fe0dda3ce89a9c5uL}, // log2(rel_err)=
                                // -5.73830393595931553818445536307990550994873046875e1
        {0x40a6a09e5b2eec99uL, 0x40a6a09e71cf8affuL, 0x4096a09e5b2ef8a9uL,
         0x407e2b7ded151d46uL, 0x405e2b7dcaf9fcffuL, 0x403822cb0ab7e164uL,
         0x401017aa59cdce5buL,
         0x3fe2677eb1f5d37buL}, // log2(rel_err)=
                                // -5.41612700686789452220182283781468868255615234375e1
        {0x40a8ace537cab371uL, 0x40a8ace54c8a8e47uL, 0x4098ace537caacdbuL,
         0x40807343885c207buL, 0x406073437af36076uL, 0x403a5205c5a54e6buL,
         0x40118c252a0c9aaeuL,
         0x3fe409758ed34865uL}, // log2(rel_err)=
                                // -5.463690506541291824760264717042446136474609375e1
        {0x40aae89f8fd754a8uL, 0x40aae89fa2de52b2uL, 0x409ae89f8fd75f8buL,
         0x4081f06a6c9455e8uL, 0x4061f06a5e012732uL, 0x403cb3dd6a3eeffauL,
         0x401323118efc8adfuL,
         0x3fe5e0f803f29217uL}, // log2(rel_err)=
                                // -5.462748672534371507936157286167144775390625e1
        {0x40ad5818d4424869uL, 0x40ad5818e5b500a6uL, 0x409d5818d442472auL,
         0x40839010992331beuL, 0x406390108d26eddcuL, 0x403f4ce76cd0e0deuL,
         0x4014de37ea01721fuL,
         0x3fe7d6c2f14310ebuL}, // log2(rel_err)=
                                // -5.62364008471714811321362503804266452789306640625e1
        {0x40affffff8000000uL, 0x40b0000004000000uL, 0x409ffffff800022auL,
         0x408555555aaaa830uL, 0x406555554f27d885uL, 0x40411111160eea44uL,
         0x4016c1cc76810400uL,
         0x3fea01941bb11dffuL}, // log2(rel_err)=
                                // -5.8213139093089324660468264482915401458740234375e1
        {0x40b172b838d24e5fuL, 0x40b172b840285496uL, 0x40a172b838d2578buL,
         0x408743a0558b5a1euL, 0x406743a048c30d8duL, 0x40429c803424592auL,
         0x4018d1633fa1a405uL,
         0x3fec613ba1ede2f9uL}, // log2(rel_err)=
                                // -5.41843410702682746205027797259390354156494140625e1
        {0x40b306fe06d4a322uL, 0x40b306fe0d8ecb08uL, 0x40a306fe06d4a4c0uL,
         0x40895ea81213d9f1uL, 0x40695ea8080484b6uL, 0x40444bb9a0e7513euL,
         0x401b101810700d96uL,
         0x3feeef6c4b62e91buL}, // log2(rel_err)=
                                // -5.823769704650387524225152446888387203216552734375e1
        {0x40b4bfdad2208d7fuL, 0x40b4bfdad84bc6d0uL, 0x40a4bfdad22089afuL,
         0x408baa792064bd6fuL, 0x406baa79186e8c7buL, 0x404621fa930b5620uL,
         0x401d82eca4af17d2uL,
         0x3ff0da592377efe8uL}, // log2(rel_err)=
                                // -5.504975787623128979930697823874652385711669921875e1
        {0x40b6a09e63ab2800uL, 0x40b6a09e69534f9auL, 0x40a6a09e63ab2148uL,
         0x408e2b7de1c416f3uL, 0x406e2b7ddb2d668cuL, 0x404822cb306568c9uL,
         0x4020174d5b83fd8auL,
         0x3ff2609f879d003fuL}, // log2(rel_err)=
                                // -5.45142491098156227735671564005315303802490234375e1
        {0x40b8ace53f92a581uL, 0x40b8ace544c29c37uL, 0x40a8ace53f929d65uL,
         0x40907343832c2a70uL, 0x4070734380561c68uL, 0x404a5205bd879809uL,
         0x40218c1da255e358uL,
         0x3ff409635637c8afuL}, // log2(rel_err)=
                                // -5.43933920224377658314551808871328830718994140625e1
        {0x40bae89f96f9f3ecuL, 0x40bae89f9bbbb36fuL, 0x40aae89f96f9f8e8uL,
         0x4091f06a67d26714uL, 0x4071f06a63877843uL, 0x404cb3dd799671efuL,
         0x402322f459eca50buL,
         0x3ff5ddb1b58b325fuL}, // log2(rel_err)=
                                // -5.614508664764922940548785845749080181121826171875e1
        {0x40bd5818dacd4d80uL, 0x40bd5818df29fb90uL, 0x40ad5818dacd41b6uL,
         0x4093901094c6566buL, 0x4073901092e2bfd8uL, 0x404f4ce77d54f3dduL,
         0x4024de03b875d03cuL,
         0x3ff7d345ef49a86euL}, // log2(rel_err)=
                                // -5.4149180295077229629896464757621288299560546875e1
        {0x40bffffffe000000uL, 0x40c0000001000000uL, 0x40affffffe00022auL,
         0x4095555556aaa830uL, 0x407555555327d884uL, 0x4051111112dbb709uL,
         0x4026c1cc7ac599a1uL,
         0x3ffa019416d53573uL}, // log2(rel_err)=
                                // -5.821313924429559705231440602801740169525146484375e1
        {0x40c172b83b9290b4uL, 0x40c172b83d681242uL, 0x40b172b83b929074uL,
         0x409743a051dffb81uL, 0x407743a04ee19b3cuL, 0x40529c804890f6fcuL,
         0x4028d105f56ae2e7uL,
         0x3ffc5a5c87c4a6eeuL}, // log2(rel_err)=
                                // -5.665854281028023109456626116298139095306396484375e1
        {0x40c306fe095a7218uL, 0x40c306fe0b08fc11uL, 0x40b306fe095a8046uL,
         0x40995ea80eb71ebduL, 0x40795ea8081cc3afuL, 0x40544bb9867affa7uL,
         0x402b109482b7f8ceuL,
         0x3ffef695f3ab02beuL}, // log2(rel_err)=
                                // -5.3619431165801728411679505370557308197021484375e1
        {0x40c4bfdad470c2fduL, 0x40c4bfdad5fb9151uL, 0x40b4bfdad470c5f1uL,
         0x409baa791d4f7ea4uL, 0x407baa7919c18b48uL, 0x405621fa7986944fuL,
         0x402d832fa7af5ff3uL,
         0x4000dda97b21a4d5uL}, // log2(rel_err)=
                                // -5.679787326040844419594577630050480365753173828125e1
        {0x40c6a09e65ca36d9uL, 0x40c6a09e673440bfuL, 0x40b6a09e65ca446duL,
         0x409e2b7ddef0b8efuL, 0x407e2b7dd8b966a6uL, 0x405822caff3a1becuL,
         0x403017b1e35018c0uL,
         0x400267908bbb7496uL}, // log2(rel_err)=
                                // -5.396746925741613409854835481382906436920166015625e1
        {0x40c8ace54184a205uL, 0x40c8ace542d09fb3uL, 0x40b8ace541849988uL,
         0x40a0734381e02cefuL, 0x4080734381aeb39auL, 0x405a5205bb7ed8d9uL,
         0x40318c1bc488b527uL,
         0x4004095f03201500uL}, // log2(rel_err)=
                                // -5.433841387172811465688937460072338581085205078125e1
        {0x40cae89f98c29bbduL, 0x40cae89f99f30b9euL, 0x40bae89f98c29f3fuL,
         0x40a1f06a66a1f7dauL, 0x4081f06a64e9186euL, 0x405cb3dd77dbe97euL,
         0x403322ed0a97fe04uL,
         0x4005dda03c065679uL}, // log2(rel_err)=
                                // -5.70503022624732096801380976103246212005615234375e1
        {0x40cd5818dc700ec5uL, 0x40cd5818dd873a49uL, 0x40bd5818dc701955uL,
         0x40a3901093af84deuL, 0x40839010911182acuL, 0x405f4ce74c73ea41uL,
         0x4034de726920663fuL,
         0x4007da4ea73355c0uL}, // log2(rel_err)=
                                // -5.483196763977029064562884741462767124176025390625e1
        {0x40cfffffff800000uL, 0x40d0000000400000uL, 0x40bfffffff80022auL,
         0x40a5555555aaa830uL, 0x408555555427d884uL, 0x40611111120eea3cuL,
         0x4036c1cc7bd6acdfuL,
         0x400a0194159d3c58uL}, // log2(rel_err)=
                                // -5.821313924780701398731252993457019329071044921875e1
        {0x40d172b83c42a149uL, 0x40d172b83cb801acuL, 0x40c172b83c42a702uL,
         0x40a743a050f5994duL, 0x408743a04e3e8aaeuL, 0x40629c8030df2bf9uL,
         0x4038d1411cc4cee4uL,
         0x400c60eabebcf374uL}, // log2(rel_err)=
                                // -5.49988144634315716530181816779077053070068359375e1
        {0x40d306fe09fbe5d6uL, 0x40d306fe0a678855uL, 0x40c306fe09fbe67fuL,
         0x40a95ea80ddf1dfauL, 0x40895ea80c789a8duL, 0x40644bb9b3eb77d1uL,
         0x403b100e9af81780uL,
         0x400ee953de5eb620uL}, // log2(rel_err)=
                                // -5.74428405339647412120029912330210208892822265625e1
        {0x40d4bfdad504d05duL, 0x40d4bfdad56783f3uL, 0x40c4bfdad504c45auL,
         0x40abaa791c895d03uL, 0x408baa791e6bb2f0uL, 0x406621faa72e0a9cuL,
         0x403d829b749fdd19uL,
         0x4010d6f7d2839bbduL}, // log2(rel_err)=
                                // -5.368699027217436281489426619373261928558349609375e1
        {0x40d6a09e6651fa90uL, 0x40d6a09e66ac7d0auL, 0x40c6a09e6651f439uL,
         0x40ae2b7dde3afddduL, 0x408e2b7dde9d40f6uL, 0x406822cb2d860da0uL,
         0x4040174f3de4870euL,
         0x401260a3fa6101c1uL}, // log2(rel_err)=
                                // -5.458202348491596467283670790493488311767578125e1
        {0x40d8ace542012126uL, 0x40d8ace542542092uL, 0x40c8ace542011890uL,
         0x40b07343818d2d8cuL, 0x409073438204fd17uL, 0x406a5205bafeb9b0uL,
         0x40418c1b46e473a5uL,
         0x4014095d9212f1e1uL}, // log2(rel_err)=
                                // -5.432498975939407870328068383969366550445556640625e1
        {0x40dae89f9934c5b1uL, 0x40dae89f9980e1a9uL, 0x40cae89f9934d129uL,
         0x40b1f06a66560a4buL, 0x4091f06a642c0eb0uL, 0x406cb3dd6030150fuL,
         0x4043231477d786b4uL,
         0x4015e0fedfd86406uL}, // log2(rel_err)=
                                // -5.453780313397447088163971784524619579315185546875e1
        {0x40dd5818dcd8bf17uL, 0x40dd5818dd1e89f8uL, 0x40cd5818dcd8b641uL,
         0x40b39010936990a9uL, 0x4093901093dd5928uL, 0x406f4ce76488cda5uL,
         0x4044de125a4d6ffbuL,
         0x4017d669cd4b85c6uL}, // log2(rel_err)=
                                // -5.449774486391009276076147216372191905975341796875e1
        {0x40dfffffffe00000uL, 0x40e0000000100000uL, 0x40cfffffffe0022auL,
         0x40b55555556aa830uL, 0x409555555467d884uL, 0x4071111111dbb709uL,
         0x4046c1cc7c1af1afuL,
         0x401a0194154f1a3duL}, // log2(rel_err)=
                                // -5.8213139248814485426919418387115001678466796875e1
        {0x40e172b83c6ea56euL, 0x40e172b83c8bfd86uL, 0x40d172b83c6eb4fauL,
         0x40b743a050bb4449uL, 0x409743a04beacb2euL, 0x40729c801945efe8uL,
         0x4048d1a26ce55445uL,
         0x401c67d41cac1786uL}, // log2(rel_err)=
                                // -5.33383323939278852776624262332916259765625e1
        {0x40e306fe0a2442c5uL, 0x40e306fe0a3f2b64uL, 0x40d306fe0a2450b6uL,
         0x40b95ea80daa08bauL, 0x40995ea80939b15cuL, 0x40744bb985a9dfbeuL,
         0x404b10922979fa3fuL,
         0x401ef690a777bc77uL}, // log2(rel_err)=
                                // -5.36464481682194360701032564975321292877197265625e1
        {0x40e4bfdad529d3b4uL, 0x40e4bfdad5428099uL, 0x40d4bfdad529e544uL,
         0x40bbaa791c59149duL, 0x409baa7916eb258buL, 0x407621fa60c913c8uL,
         0x404d83c05bde5f1fuL,
         0x4020e156582a0f25uL}, // log2(rel_err)=
                                // -5.341867987945438045471746590919792652130126953125e1
        {0x40e6a09e6673eb7duL, 0x40e6a09e668a8c1buL, 0x40d6a09e6673f929uL,
         0x40be2b7dde0e72a8uL, 0x409e2b7dd995750buL, 0x407822cafe82d7bauL,
         0x405017b259d7fb4euL,
         0x402267918e91d829uL}, // log2(rel_err)=
                                // -5.395617852425177574104964151047170162200927734375e1
        {0x40e8ace5422040eeuL, 0x40e8ace5423500c9uL, 0x40d8ace5422040a7uL,
         0x40c0734381789bf6uL, 0x40a073438104f9feuL, 0x407a5205a39f1636uL,
         0x40518c446ec0c302uL,
         0x40240cc0a2f85f34uL}, // log2(rel_err)=
                                // -5.67185055989564972378502716310322284698486328125e1
        {0x40eae89f9951502euL, 0x40eae89f9964572cuL, 0x40dae89f99515da3uL,
         0x40c1f06a66430269uL, 0x40a1f06a63fce40auL, 0x407cb3dd5fd7f24auL,
         0x4053231e4f0732a8uL,
         0x4025e11603bf353euL}, // log2(rel_err)=
                                // -5.426729514381066366013328661210834980010986328125e1
        {0x40ed5818dcf2eb2buL, 0x40ed5818dd045de3uL, 0x40dd5818dcf2ee24uL,
         0x40c3901093584aa4uL, 0x40a3901092658300uL, 0x407f4ce74cc91d16uL,
         0x4054de4cd4a199f1uL,
         0x4027d9f590d1e00buL}, // log2(rel_err)=
                                // -5.7945915141209098919716780073940753936767578125e1
        {0x40effffffff80000uL, 0x40f0000000040000uL, 0x40dffffffff8022auL,
         0x40c55555555aa830uL, 0x40a555555477d884uL, 0x4081111111ceea3cuL,
         0x4056c1cc7c2c02e3uL,
         0x402a0194153bb593uL}, // log2(rel_err)=
                                // -5.82131392489362013975551235489547252655029296875e1
        {0x40f172b83c79a678uL, 0x40f172b83c80fc7fuL, 0x40e172b83c799f7buL,
         0x40c743a050ab8090uL, 0x40a743a051d69dc9uL, 0x40829c805ea9778buL,
         0x4058d0c33651eb78uL,
         0x402c53bb9096e951uL}, // log2(rel_err)=
                                // -5.41524502386358790317899547517299652099609375e1
        {0x40f306fe0a2e5a01uL, 0x40f306fe0a351429uL, 0x40e306fe0a2e62efuL,
         0x40c95ea80d9c34f6uL, 0x40a95ea80a950a32uL, 0x40844bb99c7ad2bcuL,
         0x405b106082b1469duL,
         0x402ef01845f25edauL}, // log2(rel_err)=
                                // -5.437371086923663909828974283300340175628662109375e1
        {0x40f4bfdad533148auL, 0x40f4bfdad5393fc3uL, 0x40e4bfdad533252buL,
         0x40cbaa791c4cbf03uL, 0x40abaa7917359e64uL, 0x408621fa60d97702uL,
         0x405d83b71fe605fauL,
         0x4030e14b9da6912buL}, // log2(rel_err)=
                                // -5.350468220724966528223376371897757053375244140625e1
        {0x40f6a09e667c67b9uL, 0x40f6a09e66820fe1uL, 0x40e6a09e667c6168uL,
         0x40ce2b7dde026c4buL, 0x40ae2b7dded4448fuL, 0x408822cb2d5852c4uL,
         0x4060174f5b8688c7uL,
         0x403260a4376a7a69uL}, // log2(rel_err)=
                                // -5.4586366743419972635820158757269382476806640625e1
        {0x40f8ace5422808e0uL, 0x40f8ace5422d38d7uL, 0x40e8ace542280aacuL,
         0x40d0734381736b11uL, 0x40b0734380c51ce7uL, 0x408a5205a35aeb1auL,
         0x40618c4eb2873c7buL,
         0x40340cd8b993f3afuL}, // log2(rel_err)=
                                // -5.824451631332794221407311852090060710906982421875e1
        {0x40fae89f995872ceuL, 0x40fae89f995d348euL, 0x40eae89f995867c5uL,
         0x40d1f06a663de7a3uL, 0x40b1f06a673162d7uL, 0x408cb3dd8f342197uL,
         0x406322a503a4d619uL,
         0x4035d9f3d48d6345uL}, // log2(rel_err)=
                                // -5.4124188902028635084207053296267986297607421875e1
        {0x40fd5818dcf97630uL, 0x40fd5818dcfdd2deuL, 0x40ed5818dcf97c1duL,
         0x40d390109353eca8uL, 0x40b3901092078190uL, 0x408f4ce74c696d87uL,
         0x4064de5b75471b82uL,
         0x4037da186ef49966uL}, // log2(rel_err)=
                                // -5.59541944599940705984408850781619548797607421875e1
        {0x40fffffffffe0000uL, 0x4100000000010000uL, 0x40effffffffe022auL,
         0x40d555555556a830uL, 0x40b55555547bd884uL, 0x4091111111cbb709uL,
         0x4066c1cc7c304730uL,
         0x403a01941536b88cuL}, // log2(rel_err)=
                                // -5.82131392490967840558369061909615993499755859375e1
        {0x410172b83c7c66bauL, 0x410172b83c7e3c3cuL, 0x40f172b83c7c6ac4uL,
         0x40d743a050a82faauL, 0x40b743a04efbb383uL, 0x40929c80471847a8uL,
         0x4068d130712025b7uL,
         0x403c5ac1497d10deuL}, // log2(rel_err)=
                                // -5.56660994925330641081018256954848766326904296875e1
        {0x410306fe0a30dfd0uL, 0x410306fe0a328e5auL, 0x40f306fe0a30e77euL,
         0x40d95ea80d98d904uL, 0x40b95ea80aeb990auL, 0x40944bb99c9b51c0uL,
         0x406b1054256005c9uL,
         0x403eeffb88c89958uL}, // log2(rel_err)=
                                // -5.4631843187215650914367870427668094635009765625e1
        {0x4104bfdad53564c0uL, 0x4104bfdad536ef8euL, 0x40f4bfdad535647cuL,
         0x40dbaa791c49547duL, 0x40bbaa791b9e1b8auL, 0x409621fa7913e70cuL,
         0x406d830fc873fcb4uL,
         0x4040dd83be006397uL}, // log2(rel_err)=
                                // -5.667769752966996321674741921015083789825439453125e1
        {0x4106a09e667e86c8uL, 0x4106a09e667ff0d2uL, 0x40f6a09e667e7b78uL,
         0x40de2b7dddff9ca7uL, 0x40be2b7de023e0a7uL, 0x409822cb2dea338cuL,
         0x407017369e0233f3uL,
         0x404260698bd1e978uL}, // log2(rel_err)=
                                // -5.38756098829720286857991595752537250518798828125e1
        {0x4108ace54229fadduL, 0x4108ace5422b46dbuL, 0x40f8ace54229ec85uL,
         0x40e073438171f44auL, 0x40c0734382dffd4euL, 0x409a5205bb7eb988uL,
         0x40718bfec35f8e69uL,
         0x40440919d0a0f169uL}, // log2(rel_err)=
                                // -5.3682354820836962971952743828296661376953125e1
        {0x410ae89f995a3b75uL, 0x410ae89f995b6be5uL, 0x40fae89f995a4b9euL,
         0x40e1f06a663d0f05uL, 0x40c1f06a63a8d330uL, 0x409cb3dd5f7f2ae9uL,
         0x4073232bb0ff0cd0uL,
         0x4045e1359c98f0feuL}, // log2(rel_err)=
                                // -5.396588808246531954182501067407429218292236328125e1
        {0x410d5818dcfb18f1uL, 0x410d5818dcfc301cuL, 0x40fd5818dcfb27efuL,
         0x40e3901093530366uL, 0x40c3901090daa736uL, 0x409f4ce73516261duL,
         0x4074de885a649c8auL,
         0x4047dd83d79383a3uL}, // log2(rel_err)=
                                // -5.423157400193442612135186209343373775482177734375e1
        {0x410fffffffff8000uL, 0x4110000000004000uL, 0x40ffffffffff822auL,
         0x40e555555555a830uL, 0x40c55555547cd884uL, 0x40a1111111caea3cuL,
         0x4076c1cc7c315843uL,
         0x404a019415359d23uL}, // log2(rel_err)=
                                // -5.821313924900704250831040553748607635498046875e1
        {0x411172b83c7d16cbuL, 0x411172b83c7d8c2cuL, 0x410172b83c7d0ceeuL,
         0x40e743a050a6ed5duL, 0x40c743a0529aa84buL, 0x40a29c805ef9fce9uL,
         0x4078d0a6bfa11cebuL,
         0x404c537863e27e92uL}, // log2(rel_err)=
                                // -5.37177768693198487426343490369617938995361328125e1
        {0x411306fe0a318144uL, 0x411306fe0a31ece7uL, 0x410306fe0a31804duL,
         0x40e95ea80d97a583uL, 0x40c95ea80d2c4fe5uL, 0x40a44bb9b3e2a6f5uL,
         0x407b0ffe83a1be66uL,
         0x404ee92da4340165uL}, // log2(rel_err)=
                                // -5.61276496405584595095206168480217456817626953125e1
        {0x4114bfdad535f8cduL, 0x4114bfdad5365b80uL, 0x4104bfdad53604f9uL,
         0x40ebaa791c48e7f3uL, 0x40cbaa7918625be5uL, 0x40a621fa615b03d8uL,
         0x407d838afb0b1106uL,
         0x4050e117326987cbuL}, // log2(rel_err)=
                                // -5.40041204513035353329541976563632488250732421875e1
        {0x4116a09e667f0e8buL, 0x4116a09e667f690duL, 0x4106a09e667f1af9uL,
         0x40ee2b7dddff9a5cuL, 0x40ce2b7dd9f7052duL, 0x40a822cafe9b4e9fuL,
         0x408017ac33ef9d45uL,
         0x405267830cc80b7auL}, // log2(rel_err)=
                                // -5.41119480715215530608475091867148876190185546875e1
        {0x4118ace5422a775cuL, 0x4118ace5422aca5cuL, 0x4108ace5422a6d50uL,
         0x40f0734381719f66uL, 0x40d0734382512bd4uL, 0x40aa5205bafc4876uL,
         0x40818c140ccafb96uL,
         0x4054094cb06b160auL}, // log2(rel_err)=
                                // -5.41327120797698881915493984706699848175048828125e1
        {0x411ae89f995aad9fuL, 0x411ae89f995af9bbuL, 0x410ae89f995abc41uL,
         0x40f1f06a663cc39buL, 0x40d1f06a63dbf17buL, 0x40acb3dd5fa92207uL,
         0x4083232423819d26uL,
         0x4055e124253bc3eauL}, // log2(rel_err)=
                                // -5.41287021005665991424393723718822002410888671875e1
        {0x411d5818dcfb81a2uL, 0x411d5818dcfbc76duL, 0x410d5818dcfb79e7uL,
         0x40f39010935263cduL, 0x40d3901093cfba15uL, 0x40af4ce76444029fuL,
         0x4084de17d27dfeacuL,
         0x4057d6769baa5d72uL}, // log2(rel_err)=
                                // -5.465347325926033050791374989785254001617431640625e1
        {0x411fffffffffe000uL, 0x4120000000001000uL, 0x410fffffffffe22auL,
         0x40f5555555556830uL, 0x40d55555547d1884uL, 0x40b1111111cab709uL,
         0x4086c1cc7c319c88uL,
         0x405a019415353270uL}, // log2(rel_err)=
                                // -5.82131392491144907808120478875935077667236328125e1
        {0x412172b83c7d42cfuL, 0x412172b83c7d6027uL, 0x411172b83c7d3dccuL,
         0x40f743a050a7123buL, 0x40d743a05157c94duL, 0x40b29c80482636bauL,
         0x4088d0d6c908add7uL,
         0x405c59ebe5994e4fuL}, // log2(rel_err)=
                                // -5.45533367069773049706782330758869647979736328125e1
        {0x412306fe0a31a9a1uL, 0x412306fe0a31c48auL, 0x411306fe0a31a681uL,
         0x40f95ea80d97719euL, 0x40d95ea80dbc65d2uL, 0x40b44bb9b4222647uL,
         0x408b0fe91f52742duL,
         0x405ee8fb08d43bd1uL}, // log2(rel_err)=
                                // -5.5166248968017924880768987350165843963623046875e1
        {0x4124bfdad5361dd1uL, 0x4124bfdad536367euL, 0x4114bfdad536141cuL,
         0x40fbaa791c480240uL, 0x40dbaa791e13e71buL, 0x40b621fa906f3f8fuL,
         0x408d82b24975b454uL,
         0x4060da14049daf15uL}, // log2(rel_err)=
                                // -5.395941437817810992783051915466785430908203125e1
        {0x4126a09e667f307cuL, 0x4126a09e667f471cuL, 0x4116a09e667f3a85uL,
         0x40fe2b7dddff6f3buL, 0x40de2b7dda96b1e1uL, 0x40b822cafee2351buL,
         0x409017a058561f4euL,
         0x40626766ec771f8euL}, // log2(rel_err)=
                                // -5.446960724057957037302912794984877109527587890625e1
        {0x4128ace5422a967buL, 0x4128ace5422aab3buL, 0x4118ace5422aa67fuL,
         0x410073438171e2f9uL, 0x40e073437eed39e5uL, 0x40ba52058b6a8c0auL,
         0x40918c951e43a81cuL,
         0x40641081309dfb12uL}, // log2(rel_err)=
                                // -5.384050145681436561062582768499851226806640625e1
        {0x412ae89f995aca2auL, 0x412ae89f995add31uL, 0x411ae89f995ac7c1uL,
         0x4101f06a663c8630uL, 0x40e1f06a6613a885uL, 0x40bcb3dd77ea7c31uL,
         0x409322cfbbe87527uL,
         0x4065dd5a8055480buL}, // log2(rel_err)=
                                // -5.572799743406034878034915891475975513458251953125e1
        {0x412d5818dcfb9bceuL, 0x412d5818dcfbad41uL, 0x411d5818dcfb96bauL,
         0x4103901093525131uL, 0x40e3901093776953uL, 0x40bf4ce763f286ccuL,
         0x4094de24f7ca2575uL,
         0x4067d696348c5112uL}, // log2(rel_err)=
                                // -5.511280469517190994110933388583362102508544921875e1
        {0x412ffffffffff800uL, 0x4130000000000400uL, 0x411ffffffffffa2auL,
         0x4105555555555830uL, 0x40e55555547d2884uL, 0x40c1111111caaa3cuL,
         0x4096c1cc7c31ad99uL,
         0x406a019415353b9cuL}, // log2(rel_err)=
                                // -5.821313924901146918955419096164405345916748046875e1
        {0x413172b83c7d4dd0uL, 0x413172b83c7d5526uL, 0x412172b83c7d4a04uL,
         0x410743a050a7027euL, 0x40e743a05106e1f8uL, 0x40c29c8048011b98uL,
         0x4098d0e2d3a3b735uL,
         0x406c5a08c5870b4auL}, // log2(rel_err)=
                                // -5.486817858636086242540841340087354183197021484375e1
        {0x413306fe0a31b3b8uL, 0x413306fe0a31ba72uL, 0x412306fe0a31b862uL,
         0x41095ea80d97c122uL, 0x40e95ea80bb59f88uL, 0x40c44bb99cf57ca2uL,
         0x409b1036444707b6uL,
         0x406eefb42b4bfb23uL}, // log2(rel_err)=
                                // -5.555165743134069344932868261821568012237548828125e1
        {0x4134bfdad5362711uL, 0x4134bfdad5362d3cuL, 0x4124bfdad5363935uL,
         0x410baa791c48a4f8uL, 0x40ebaa7916d50300uL, 0x40c621fa60a9213auL,
         0x409d83c619550c3auL,
         0x4070e15d681ddb92uL}, // log2(rel_err)=
                                // -5.336816222408585730363483889959752559661865234375e1
        {0x4136a09e667f38f8uL, 0x4136a09e667f3ea0uL, 0x4126a09e667f4abcuL,
         0x410e2b7dddff5d0fuL, 0x40ee2b7dd893d144uL, 0x40c822cafdfc5dfbuL,
         0x40a017c6a074a277uL,
         0x407267c1cd212b3cuL}, // log2(rel_err)=
                                // -5.353716965466170307763604796491563320159912109375e1
        {0x4138ace5422a9e43uL, 0x4138ace5422aa373uL, 0x4128ace5422aac76uL,
         0x411073438171de94uL, 0x40f073437f29c6fduL, 0x40ca52058ba2a9a2uL,
         0x40a18c8c1d6c7b4euL,
         0x4074106b7b53729duL}, // log2(rel_err)=
                                // -5.403551671730065919518892769701778888702392578125e1
        {0x413ae89f995ad14cuL, 0x413ae89f995ad60duL, 0x412ae89f995ae39duL,
         0x4111f06a663cdc1duL, 0x40f1f06a6361649duL, 0x40ccb3dd48f8c468uL,
         0x40a323365f0f854euL,
         0x4075e4500f8158c7uL}, // log2(rel_err)=
                                // -5.37637205770127337700614589266479015350341796875e1
        {0x413d5818dcfba259uL, 0x413d5818dcfba6b6uL, 0x412d5818dcfb9deeuL,
         0x4113901093524c86uL, 0x40f39010936178d3uL, 0x40cf4ce763e11676uL,
         0x40a4de283aec3932uL,
         0x4077d69d99bde1a2uL}, // log2(rel_err)=
                                // -5.52547225218837780857938923873007297515869140625e1
        {0x413ffffffffffe00uL, 0x4140000000000100uL, 0x4130000000000015uL,
         0x4115555555555430uL, 0x40f55555547d2c84uL, 0x40d1111111caa709uL,
         0x40a6c1cc7c31b1dduL,
         0x407a019415351a06uL}, // log2(rel_err)=
                                // -5.821313924911613213453165371902287006378173828125e1
        {0x414172b83c7d5090uL, 0x414172b83c7d5265uL, 0x413172b83c7d5566uL,
         0x411743a050a75b0cuL, 0x40f743a04ec7dc5cuL, 0x40d29c8030bb4b4buL,
         0x40a8d13854534ca1uL,
         0x407c60d5c6d86cc6uL}, // log2(rel_err)=
                                // -5.53113492408646578724074061028659343719482421875e1
        {0x414306fe0a31b63euL, 0x414306fe0a31b7eduL, 0x413306fe0a31b486uL,
         0x41195ea80d975f8buL, 0x40f95ea80d5ed184uL, 0x40d44bb9b3f90544uL,
         0x40ab0ff70b5ba336uL,
         0x407ee91bdb8a2e05uL}, // log2(rel_err)=
                                // -5.5717738888341187930564046837389469146728515625e1
        {0x4144bfdad5362962uL, 0x4144bfdad5362aeduL, 0x4134bfdad536212buL,
         0x411baa791c47f181uL, 0x40fbaa791db090e1uL, 0x40d621fa9042dd14uL,
         0x40ad82c1110869a0uL,
         0x4080da258e5fc696uL}, // log2(rel_err)=
                                // -5.416744708820414189176517538726329803466796875e1
        {0x4146a09e667f3b18uL, 0x4146a09e667f3c83uL, 0x4136a09e667f2d79uL,
         0x411e2b7dddfe4a6auL, 0x40fe2b7de0be8f98uL, 0x40d822cb44725269uL,
         0x40b0172b3039bf58uL,
         0x40825d4d8af48d9cuL}, // log2(rel_err)=
                                // -5.36365545703889523565521812997758388519287109375e1
        {0x4148ace5422aa035uL, 0x4148ace5422aa181uL, 0x4138ace5422aadf4uL,
         0x412073438171dd7cuL, 0x410073437f38de5euL, 0x40da52058bafc5a8uL,
         0x40b18c89df46ddc2uL,
         0x40841066368d8e9fuL}, // log2(rel_err)=
                                // -5.408868545945506411953829228878021240234375e1
        {0x414ae89f995ad315uL, 0x414ae89f995ad445uL, 0x413ae89f995ad9ecuL,
         0x4121f06a663cae13uL, 0x4101f06a64df9f6auL, 0x40dcb3dd6093197auL,
         0x40b322fd89d06293uL,
         0x4085e0c820955f6buL}, // log2(rel_err)=
                                // -5.54793456178554578173134359531104564666748046875e1
        {0x414d5818dcfba3fcuL, 0x414d5818dcfba514uL, 0x413d5818dcfb9767uL,
         0x4123901093521d1duL, 0x4103901094716295uL, 0x40df4ce77b190136uL,
         0x40b4ddffccb0b880uL,
         0x4087d33c996c0cefuL}, // log2(rel_err)=
                                // -5.406850729312321135466845589689910411834716796875e1
        {0x414fffffffffff80uL, 0x4150000000000040uL, 0x41400000000000d5uL,
         0x4125555555555330uL, 0x41055555547d2d84uL, 0x40e1111111caa63cuL,
         0x40b6c1cc7c31b2eeuL,
         0x408a01941535357duL}, // log2(rel_err)=
                                // -5.82131392490121442051531630568206310272216796875e1
        {0x415172b83c7d5140uL, 0x415172b83c7d51b5uL, 0x414172b83c7d583euL,
         0x412743a050a75833uL, 0x410743a04e384a8auL, 0x40e29c80307ca99fuL,
         0x40b8d14dac3e3439uL,
         0x408c6107fd080260uL}, // log2(rel_err)=
                                // -5.4643155973572930861337226815521717071533203125e1
        {0x415306fe0a31b6dfuL, 0x415306fe0a31b74auL, 0x414306fe0a31c437uL,
         0x41295ea80d981918uL, 0x41095ea809738679uL, 0x40e44bb985af9647uL,
         0x40bb108c3931706auL,
         0x408ef681d8301ba1uL}, // log2(rel_err)=
                                // -5.371584918662608032491334597580134868621826171875e1
        {0x4154bfdad53629f6uL, 0x4154bfdad5362a59uL, 0x4144bfdad536237cuL,
         0x412baa791c47ef2buL, 0x410baa791d3cd827uL, 0x40e621fa901115bauL,
         0x40bd82d244bd3c25uL,
         0x4090da39ad867f02uL}, // log2(rel_err)=
                                // -5.445532805530195474830179591663181781768798828125e1
        {0x4156a09e667f3b9fuL, 0x4156a09e667f3bf9uL, 0x4146a09e667f4779uL,
         0x412e2b7dddff5ec3uL, 0x410e2b7dda1dc8b5uL, 0x40e822cafeacd727uL,
         0x40c017a9566d121fuL,
         0x4092677c2a567fe2uL}, // log2(rel_err)=
                                // -5.41904391167735042245112708769738674163818359375e1
        {0x4158ace5422aa0b2uL, 0x4158ace5422aa105uL, 0x4148ace5422a9dacuL,
         0x413073438171b2acuL, 0x4110734381675833uL, 0x40ea5205a3e523aeuL,
         0x40c18c36d5d4fb8fuL,
         0x40940ca0706d17d6uL}, // log2(rel_err)=
                                // -5.545290147531748203846291289664804935455322265625e1
        {0x415ae89f995ad387uL, 0x415ae89f995ad3d3uL, 0x414ae89f995adfd4uL,
         0x4131f06a663cab5cuL, 0x4111f06a6429bc54uL, 0x40ecb3dd5feff473uL,
         0x40c32318959572f3uL,
         0x4095e108751f2f87uL}, // log2(rel_err)=
                                // -5.4418295701387563667594804428517818450927734375e1
        {0x415d5818dcfba464uL, 0x415d5818dcfba4aauL, 0x414d5818dcfbaec2uL,
         0x413390109352768duL, 0x41139010917507acuL, 0x40ef4ce74be3096fuL,
         0x40c4de71745fbb71uL,
         0x4097da4ca6a395e1uL}, // log2(rel_err)=
                                // -5.486568910767687867746644769795238971710205078125e1
        {0x415fffffffffffe0uL, 0x4160000000000010uL, 0x4150000000000105uL,
         0x41355555555552f0uL, 0x41155555547d2dc4uL, 0x40f1111111caa609uL,
         0x40c6c1cc7c31b333uL,
         0x409a01941535188buL}, // log2(rel_err)=
                                // -5.821313924911550685692418483085930347442626953125e1
        {0x416172b83c7d516cuL, 0x416172b83c7d5189uL, 0x415172b83c7d58f5uL,
         0x413743a050a75785uL, 0x411743a04e1406ebuL, 0x40f29c803069ef03uL,
         0x40c8d15312bb1104uL,
         0x409c61159b92a65cuL}, // log2(rel_err)=
                                // -5.451523654771410321018265676684677600860595703125e1
        {0x416306fe0a31b708uL, 0x416306fe0a31b723uL, 0x415306fe0a31af27uL,
         0x41395ea80d9763f2uL, 0x41195ea80ef92ed5uL, 0x40f44bb9b4b119a5uL,
         0x40cb0fba066be7cauL,
         0x409ee88ab4e2922duL}, // log2(rel_err)=
                                // -5.411244040795492793449739110656082630157470703125e1
        {0x4164bfdad5362a1buL, 0x4164bfdad5362a34uL, 0x4154bfdad5362411uL,
         0x413baa791c47ee9cuL, 0x411baa791d1fa299uL, 0x40f621fa90023a82uL,
         0x40cd82d69e0befb3uL,
         0x40a0da3f2046e56buL}, // log2(rel_err)=
                                // -5.453724706996390381164019345305860042572021484375e1
        {0x4166a09e667f3bc1uL, 0x4166a09e667f3bd7uL, 0x4156a09e667f45a5uL,
         0x413e2b7dddff6055uL, 0x411e2b7ddaa062c3uL, 0x40f822cafee6810buL,
         0x40d0179fa0f57c84uL,
         0x40a2676537874a74uL}, // log2(rel_err)=
                                // -5.449428103100763820521024172194302082061767578125e1
        {0x4168ace5422aa0d1uL, 0x4168ace5422aa0e6uL, 0x4158ace5422aa1eduL,
         0x414073438171b0bduL, 0x4120734380ddc059uL, 0x40fa5205a36d863auL,
         0x40d18c4b4a3c2f21uL,
         0x40a40cd07d0bdb70uL}, // log2(rel_err)=
                                // -5.780200397587038452229535323567688465118408203125e1
        {0x416ae89f995ad3a4uL, 0x416ae89f995ad3b7uL, 0x415ae89f995ad0a6uL,
         0x4141f06a663c8022uL, 0x4121f06a66270f56uL, 0x40fcb3dd77faec9cuL,
         0x40d322ccda7daa6cuL,
         0x40a5dd53cb8f5a20uL}, // log2(rel_err)=
                                // -5.554178919275767611907212994992733001708984375e1
        {0x416d5818dcfba47euL, 0x416d5818dcfba48fuL, 0x415d5818dcfbb498uL,
         0x414390109352a5dfuL, 0x4123901090b614a2uL, 0x40ff4ce734f4c6e0uL,
         0x40d4de8dd81a96deuL,
         0x40a7dd90da58e956uL}, // log2(rel_err)=
                                // -5.411347473857347978309917380101978778839111328125e1
        {0x416ffffffffffff8uL, 0x4170000000000004uL, 0x4160000000000111uL,
         0x41455555555552e0uL, 0x41255555547d2dd4uL, 0x4101111111caa5fcuL,
         0x40d6c1cc7c31b344uL,
         0x40aa019415353526uL}, // log2(rel_err)=
                                // -5.821313924901145497869947575964033603668212890625e1
        {0x417172b83c7d5177uL, 0x417172b83c7d517euL, 0x416172b83c7d5922uL,
         0x414743a050a75754uL, 0x412743a04e0b3d63uL, 0x41029c80306750f4uL,
         0x40d8d1545ff8b720uL,
         0x40ac61184aad20a7uL}, // log2(rel_err)=
                                // -5.448495138998294606835770537145435810089111328125e1
        {0x417306fe0a31b712uL, 0x417306fe0a31b719uL, 0x416306fe0a31b238uL,
         0x41495ea80d97613euL, 0x41295ea80e2f6df9uL, 0x41044bb9b4542ec3uL,
         0x40db0fd80845fdcfuL,
         0x40aee8d2d92af214uL}, // log2(rel_err)=
                                // -5.469001624449401788297109305858612060546875e1
        {0x4174bfdad5362a24uL, 0x4174bfdad5362a2auL, 0x4164bfdad5362c8auL,
         0x414baa791c484af4uL, 0x412baa791aeda13auL, 0x410621fa78c28cceuL,
         0x40dd832a2e47b988uL,
         0x40b0dda347c59545uL}, // log2(rel_err)=
                                // -5.74380266939059964670377667061984539031982421875e1
        {0x4176a09e667f3bcauL, 0x4176a09e667f3bd0uL, 0x4166a09e667f3487uL,
         0x414e2b7dddfea7b6uL, 0x412e2b7ddf16fffduL, 0x410822cb2d71534buL,
         0x40e0174aad4de915uL,
         0x40b260991dfb1365uL}, // log2(rel_err)=
                                // -5.44222923581181561303310445509850978851318359375e1
        {0x4178ace5422aa0d9uL, 0x4178ace5422aa0dfuL, 0x4168ace5422a9aaauL,
         0x4150734381718206uL, 0x4130734381d09c96uL, 0x410a5205ba89a8ccuL,
         0x40e18c272e825b06uL,
         0x40b4097a0a2c6ab8uL}, // log2(rel_err)=
                                // -5.47090074340949428233216167427599430084228515625e1
        {0x417ae89f995ad3abuL, 0x417ae89f995ad3b0uL, 0x416ae89f995ad52euL,
         0x4151f06a663c7e1buL, 0x4131f06a659115fduL, 0x410cb3dd7776deeduL,
         0x40e322e3269bd1e2uL,
         0x40b5dd88706cd333uL}, // log2(rel_err)=
                                // -5.801292529294172339859869680367410182952880859375e1
        {0x417d5818dcfba485uL, 0x417d5818dcfba48auL, 0x416d5818dcfba566uL,
         0x41539010935248bcuL, 0x4133901092b10bdbuL, 0x410f4ce763421fa9uL,
         0x40e4de4277213189uL,
         0x40b7d6dc1db20696uL}, // log2(rel_err)=
                                // -5.74276052878796434697505901567637920379638671875e1
        {0x417ffffffffffffeuL, 0x4180000000000001uL, 0x4170000000000114uL,
         0x41555555555552dcuL, 0x41355555547d2dd8uL, 0x4111111111caa5f9uL,
         0x40e6c1cc7c31b348uL,
         0x40ba01941535186cuL}, // log2(rel_err)=
                                // -5.821313924911586212829206488095223903656005859375e1
        {0x418172b83c7d517auL, 0x418172b83c7d517cuL, 0x417172b83c7d50d9uL,
         0x415743a050a6fac8uL, 0x413743a05033ee91uL, 0x41129c8047a41e4fuL,
         0x40e8d102311f28c1uL,
         0x40bc5a52dc07954auL}, // log2(rel_err)=
                                // -5.63278196598278526607828098349273204803466796875e1
        {0x418306fe0a31b714uL, 0x418306fe0a31b715uL, 0x417306fe0a31c3a4uL,
         0x41595ea80d981989uL, 0x41395ea809a77e01uL, 0x41144bb985c4cc75uL,
         0x40eb108480ad1883uL,
         0x40bef67023c45da5uL}, // log2(rel_err)=
                                // -5.38127303682427964304224587976932525634765625e1
        {0x4184bfdad5362a26uL, 0x4184bfdad5362a27uL, 0x4174bfdad53636fcuL,
         0x415baa791c48a58cuL, 0x413baa7918366ce6uL, 0x411621fa61486ddduL,
         0x40ed83918c3f00f3uL,
         0x40c0e11ec76c0bb0uL}, // log2(rel_err)=
                                // -5.391766026252177113065044977702200412750244140625e1
        {0x4186a09e667f3bccuL, 0x4186a09e667f3bceuL, 0x4176a09e667f3894uL,
         0x415e2b7dddfea420uL, 0x413e2b7dde09abeeuL, 0x411822cb2cf823efuL,
         0x40f0175eb3892e80uL,
         0x40c260c8cf6909cduL}, // log2(rel_err)=
                                // -5.530904321014969582392950542271137237548828125e1
        {0x4188ace5422aa0dbuL, 0x4188ace5422aa0dduL, 0x4178ace5422a98d9uL,
         0x41607343817182d4uL, 0x41407343820d5f8buL, 0x411a5205babfc342uL,
         0x40f18c1e25838116uL,
         0x40c409649d9a99a5uL}, // log2(rel_err)=
                                // -5.440833307471945090583176352083683013916015625e1
        {0x418ae89f995ad3aduL, 0x418ae89f995ad3afuL, 0x417ae89f995acdfcuL,
         0x4161f06a663c4f5buL, 0x4141f06a6680fd89uL, 0x411cb3dd8e92225cuL,
         0x40f322bf7a9f43f4uL,
         0x40c5da32c0047c24uL}, // log2(rel_err)=
                                // -5.489830304219341172711210674606263637542724609375e1
        {0x418d5818dcfba487uL, 0x418d5818dcfba489uL, 0x417d5818dcfb9945uL,
         0x4163901093521c2buL, 0x4143901094454757uL, 0x411f4ce77af0eab8uL,
         0x40f4de065bbdf696uL,
         0x40c7d34c491c12c1uL}, // log2(rel_err)=
                                // -5.420583107688477042529484606347978115081787109375e1
        {0x418fffffffffffffuL, 0x4190000000000000uL, 0x4180000000000969uL,
         0x4165555555557d6auL, 0x4145555552524a47uL, 0x4121111105afc3acuL,
         0x40f6c21efe5b3f8buL, 0x40ca05592c5cb5bauL}, // log2(rel_err)= -54
        {0x419172b83c7d517buL, 0x419172b83c7d517cuL, 0x418172b83c7d4673uL,
         0x416743a050a6a022uL, 0x414743a052e8ced9uL, 0x41229c805f1dddf7uL,
         0x40f8d09b2b805e0auL,
         0x40cc535c776939e8uL}, // log2(rel_err)=
                                // -5.3573044996492399150156415998935699462890625e1
        {0x419306fe0a31b715uL, 0x419306fe0a31b716uL, 0x418306fe0a31b757uL,
         0x41695ea80d975cb1uL, 0x41495ea80cdb198duL, 0x41244bb9b3bc6d0cuL,
         0x40fb100aa2b63a61uL,
         0x40cee94aef33e667uL}, // log2(rel_err)=
                                // -5.70815989961755718695712857879698276519775390625e1
        {0x4194bfdad5362a27uL, 0x4194bfdad5362a28uL, 0x4184bfdad53628f1uL,
         0x416baa791c47ea44uL, 0x414baa791bde07c6uL, 0x412621fa8f7276b2uL,
         0x40fd83066fed2bc2uL,
         0x40d0da77ec792f73uL}, // log2(rel_err)=
                                // -5.60360773186010163726678001694381237030029296875e1
        {0x4196a09e667f3bccuL, 0x4196a09e667f3bccuL, 0x4186a09e667f4a40uL,
         0x416e2b7dddff5c3buL, 0x414e2b7dd97077ffuL, 0x412822cafe5e8821uL,
         0x410017b6395152d3uL,
         0x40d2679ae8b3b0dfuL}, // log2(rel_err)=
                                // -5.386637719271632107620462193153798580169677734375e1
        {0x4198ace5422aa0dbuL, 0x4198ace5422aa0dbuL, 0x4188ace5422aa90duL,
         0x417073438171df85uL, 0x415073437ff1b89duL, 0x412a52058c53fd13uL,
         0x41018c6e635ca702uL,
         0x40d4102516f0deb3uL}, // log2(rel_err)=
                                // -5.497645115977872620760535937733948230743408203125e1
        {0x419ae89f995ad3aduL, 0x419ae89f995ad3aduL, 0x418ae89f995adcd8uL,
         0x4171f06a663caca6uL, 0x4151f06a649213dduL, 0x412cb3dd604d66b5uL,
         0x4103230911c8cef2uL,
         0x40d5e0e39395dd98uL}, // log2(rel_err)=
                                // -5.493037419462854131779749877750873565673828125e1
        {0x419d5818dcfba487uL, 0x419d5818dcfba487uL, 0x418d5818dcfba6e5uL,
         0x4173901093527a01uL, 0x41539010927f7e8cuL, 0x412f4ce74cd1a38euL,
         0x4104de49d4fda8abuL,
         0x40d7d9ee79c85d5fuL}, // log2(rel_err)=
                                // -5.827879941239655892104565282352268695831298828125e1
        {0x41a0000000000000uL, 0x41a0000000000000uL, 0x418ffffffffffdffuL,
         0x41755555555554b0uL, 0x4155555555080a6fuL, 0x413111111209e913uL,
         0x4106c1b7d5762302uL,
         0x40da0162a7172717uL}, // log2(rel_err)=
                                // -5.599929691172398094067830243147909641265869140625e1
        {0x41a172b83c7d517buL, 0x41a172b83c7d517buL, 0x419172b83c7d4c2euL,
         0x417743a050a6fef3uL, 0x415743a0516b0b91uL, 0x41329c80482caf6auL,
         0x4108d0d3f061fd0cuL,
         0x40dc59e5c7c18971uL}, // log2(rel_err)=
                                // -5.448691718034938702430736157111823558807373046875e1
        {0x41a306fe0a31b715uL, 0x41a306fe0a31b715uL, 0x419306fe0a31bc97uL,
         0x41795ea80d97bbe2uL, 0x41595ea80b7d7c0cuL, 0x41344bb99cddceebuL,
         0x410b103e9cdee7d1uL,
         0x40deefc78a047e9cuL}, // log2(rel_err)=
                                // -5.5229579136405419603761401958763599395751953125e1
        {0x41a4bfdad5362a27uL, 0x41a4bfdad5362a27uL, 0x4194bfdad5362dc2uL,
         0x417baa791c4849deuL, 0x415baa791a9d3a85uL, 0x413621fa789e9bd8uL,
         0x410d833622c01ab6uL,
         0x40e0ddb17ad2b932uL}, // log2(rel_err)=
                                // -5.629369149274442207797619630582630634307861328125e1
        {0x41a6a09e667f3bcduL, 0x41a6a09e667f3bceuL, 0x4196a09e667f2d5auL,
         0x417e2b7dddfe4a35uL, 0x415e2b7de0f5b947uL, 0x413822cb448adcf5uL,
         0x411017271670e259uL,
         0x40e25d43d1d879afuL}, // log2(rel_err)=
                                // -5.3559848029436665228786296211183071136474609375e1
        {0x41a8ace5422aa0dbuL, 0x41a8ace5422aa0dbuL, 0x4198ace5422aad1auL,
         0x418073438171ddb9uL, 0x416073437f6acee0uL, 0x413a52058bdb87dfuL,
         0x41118c8272d35f00uL,
         0x40e41054b753368fuL}, // log2(rel_err)=
                                // -5.428015540628467761052888818085193634033203125e1
        {0x41aae89f995ad3aduL, 0x41aae89f995ad3aduL, 0x419ae89f995ae08fuL,
         0x4181f06a663cab01uL, 0x4161f06a64165970uL, 0x413cb3dd5fde5b50uL,
         0x4113231b7793c489uL,
         0x40e5e10f593c50b6uL}, // log2(rel_err)=
                                // -5.434035946244036807684096856974065303802490234375e1
        {0x41ad5818dcfba487uL, 0x41ad5818dcfba487uL, 0x419d5818dcfbaa4duL,
         0x418390109352787euL, 0x41639010920e0c58uL, 0x413f4ce74c6c7a85uL,
         0x4114de5ab34df189uL,
         0x40e7da1680a57a9buL}, // log2(rel_err)=
                                // -5.60106657735939990061524440534412860870361328125e1
        {0x41b0000000000000uL, 0x41b0000000000000uL, 0x41a000000000008fuL,
         0x418555555555534buL, 0x4165555554a02079uL, 0x4141111111dc543buL,
         0x4116c1c74831901duL,
         0x40ea018713a7d443uL}, // log2(rel_err)=
                                // -5.7646865721475279542573844082653522491455078125e1
        {0x41b172b83c7d517buL, 0x41b172b83c7d517buL, 0x41a172b83c7d4d9cuL,
         0x418743a050a6fda8uL, 0x416743a0510be21euL, 0x41429c80480437c1uL,
         0x4118d0e215390bb9uL,
         0x40ec5a06b93fb31auL}, // log2(rel_err)=
                                // -5.4847196417329570294896257109940052032470703125e1
        {0x41b306fe0a31b715uL, 0x41b306fe0a31b715uL, 0x41a306fe0a31bde8uL,
         0x41895ea80d97babfuL, 0x41695ea80b25b580uL, 0x41444bb99cb410c8uL,
         0x411b104babeb926auL,
         0x40eeefe759cf430duL}, // log2(rel_err)=
                                // -5.4839049255823880457683117128908634185791015625e1
        {0x41b4bfdad5362a27uL, 0x41b4bfdad5362a27uL, 0x41a4bfdad5362ef6uL,
         0x418baa791c4848cauL, 0x416baa791a4d1efeuL, 0x414621fa787bb880uL,
         0x411d83420b54b328uL,
         0x40f0ddbf7a8dfd00uL}, // log2(rel_err)=
                                // -5.56646248808614672043404425494372844696044921875e1
        {0x41b6a09e667f3bcduL, 0x41b6a09e667f3bceuL, 0x41a6a09e667f2e75uL,
         0x418e2b7dddfe493cuL, 0x416e2b7de0ac0e3duL, 0x414822cb44695543uL,
         0x4120172c90ddb942uL,
         0x40f25d50ecdf9c45uL}, // log2(rel_err)=
                                // -5.36630594027329834716510958969593048095703125e1
        {0x41b8ace5422aa0dbuL, 0x41b8ace5422aa0dbuL, 0x41a8ace5422aae1duL,
         0x419073438171dd45uL, 0x417073437f492057uL, 0x414a52058bbe29a1uL,
         0x41218c8774a08484uL,
         0x40f410607e32c395uL}, // log2(rel_err)=
                                // -5.4148266893122837473129038698971271514892578125e1
        {0x41bae89f995ad3aduL, 0x41bae89f995ad3aduL, 0x41aae89f995ae17duL,
         0x4191f06a663caa98uL, 0x4171f06a63f75ef0uL, 0x414cb3dd5fc25eb1uL,
         0x4123232013169d6duL,
         0x40f5e11a55cb59ecuL}, // log2(rel_err)=
                                // -5.4224091685179899968716199509799480438232421875e1
        {0x41bd5818dcfba487uL, 0x41bd5818dcfba487uL, 0x41ad5818dcfbab27uL,
         0x419390109352781duL, 0x4173901091f1afcbuL, 0x414f4ce74c535ca8uL,
         0x4124de5eeae203c0uL,
         0x40f7da207b034c91uL}, // log2(rel_err)=
                                // -5.57183341712276245516477501951158046722412109375e1
        {0x41c0000000000000uL, 0x41c0000000000000uL, 0x41b00000000000f3uL,
         0x41955555555552f3uL, 0x4175555554861a17uL, 0x4151111111d07959uL,
         0x4126c1cb26f074b6uL,
         0x40fa0190575677f8uL}, // log2(rel_err)=
                                // -5.809090877030549648907253867946565151214599609375e1
        {0x41c172b83c7d517buL, 0x41c172b83c7d517buL, 0x41b172b83c7d4df8uL,
         0x419743a050a6fd5auL, 0x417743a050f3e82cuL, 0x41529c8047f85940uL,
         0x4128d0e5a6afe0d7uL,
         0x40fc5a0f907e154auL}, // log2(rel_err)=
                                // -5.495331209496129076796933077275753021240234375e1
        {0x41c306fe0a31b715uL, 0x41c306fe0a31b715uL, 0x41b306fe0a31be3cuL,
         0x41995ea80d97ba74uL, 0x41795ea80b0fdba8uL, 0x41544bb99caa7674uL,
         0x412b104eeb8e1918uL,
         0x40feefef03fdbb8cuL}, // log2(rel_err)=
                                // -5.475594874100016795637202449142932891845703125e1
        {0x41c4bfdad5362a27uL, 0x41c4bfdad5362a27uL, 0x41b4bfdad5362f43uL,
         0x419baa791c484885uL, 0x417baa791a39181duL, 0x415621fa7872ffb1uL,
         0x412d83450579a2c5uL,
         0x4100ddc2fa7ad1f5uL}, // log2(rel_err)=
                                // -5.554246784633978251122243818826973438262939453125e1
        {0x41c6a09e667f3bcduL, 0x41c6a09e667f3bceuL, 0x41b6a09e667f2ebbuL,
         0x419e2b7dddfe48f8uL, 0x417e2b7de099eadauL, 0x415822cb44631a1fuL,
         0x4130172de9482f85uL,
         0x41025d53d3b12d39uL}, // log2(rel_err)=
                                // -5.369005574468670971555184223689138889312744140625e1
        {0x41c8ace5422aa0dbuL, 0x41c8ace5422aa0dbuL, 0x41b8ace5422aae5euL,
         0x41a073438171dd29uL, 0x418073437f40a8cfuL, 0x415a52058bb6130buL,
         0x41318c88b724320cuL,
         0x4104106391214641uL}, // log2(rel_err)=
                                // -5.411709539774884802909582504071295261383056640625e1
        {0x41cae89f995ad3aduL, 0x41cae89f995ad3aduL, 0x41bae89f995ae1b8uL,
         0x41a1f06a663caa7cuL, 0x4181f06a63efb81auL, 0x415cb3dd5fbcb118uL,
         0x4133232135d6f856uL,
         0x4105e11cd9e26b73uL}, // log2(rel_err)=
                                // -5.41964314191478848670158185996115207672119140625e1
        {0x41cd5818dcfba487uL, 0x41cd5818dcfba487uL, 0x41bd5818dcfbab5euL,
         0x41a3901093527807uL, 0x4183901091ea80dduL, 0x415f4ce74c4b6acbuL,
         0x4134de5ffce79a1duL,
         0x4107da23435de8dauL}, // log2(rel_err)=
                                // -5.565364888456080194600872346200048923492431640625e1
        {0x41d0000000000000uL, 0x41d0000000000000uL, 0x41c000000000010cuL,
         0x41a55555555552dduL, 0x41855555547f987euL, 0x4161111111cd829cuL,
         0x4136c1cc1ea05232uL,
         0x410a0192a844ae5duL}, // log2(rel_err)=
                                // -5.818779106263225031625552219338715076446533203125e1
        {0x41d172b83c7d517buL, 0x41d172b83c7d517buL, 0x41c172b83c7d4e0fuL,
         0x41a743a050a6fd46uL, 0x418743a050ede9afuL, 0x41629c8047f58e01uL,
         0x4138d0e68b0dba74uL,
         0x410c5a11b79d08eeuL}, // log2(rel_err)=
                                // -5.498110634234236471229451126419007778167724609375e1
        {0x41d306fe0a31b715uL, 0x41d306fe0a31b715uL, 0x41c306fe0a31be51uL,
         0x41a95ea80d97ba61uL, 0x41895ea80b0a6531uL, 0x41644bb99ca8260auL,
         0x413b104fbb77036duL,
         0x410eeff0e73439eauL}, // log2(rel_err)=
                                // -5.4735900280050685751120909117162227630615234375e1
        {0x41d4bfdad5362a27uL, 0x41d4bfdad5362a27uL, 0x41c4bfdad5362f57uL,
         0x41abaa791c48487auL, 0x418baa791a33cf04uL, 0x416621fa786e7e44uL,
         0x413d8345d064b86duL,
         0x4110ddc441c2f6d7uL}, // log2(rel_err)=
                                // -5.551347844334429026957877795211970806121826171875e1
        {0x41d6a09e667f3bcduL, 0x41d6a09e667f3bceuL, 0x41c6a09e667f2ecduL,
         0x41ae2b7dddfe48ebuL, 0x418e2b7de095326cuL, 0x416822cb44600d57uL,
         0x4140172e438355d0uL,
         0x41125d54cfced665uL}, // log2(rel_err)=
                                // -5.369688422220325918488015304319560527801513671875e1
        {0x41d8ace5422aa0dbuL, 0x41d8ace5422aa0dbuL, 0x41c8ace5422aae6euL,
         0x41b073438171dd21uL, 0x419073437f3e96d2uL, 0x416a52058bb4cc5fuL,
         0x41418c8905b4efc6uL,
         0x4114106434a9f7bduL}, // log2(rel_err)=
                                // -5.410940659055108170605308259837329387664794921875e1
        {0x41dae89f995ad3aduL, 0x41dae89f995ad3aduL, 0x41cae89f995ae1c7uL,
         0x41b1f06a663caa76uL, 0x4191f06a63edc280uL, 0x416cb3dd5fba86c0uL,
         0x4143232180971863uL,
         0x4115e11d9c18eb3duL}, // log2(rel_err)=
                                // -5.418959839424048396949729067273437976837158203125e1
        {0x41dd5818dcfba487uL, 0x41dd5818dcfba487uL, 0x41cd5818dcfbab6buL,
         0x41b39010935277feuL, 0x4193901091e8d8d2uL, 0x416f4ce74c4c0426uL,
         0x4144de603b3809bfuL,
         0x4117da2383211daauL}, // log2(rel_err)=
                                // -5.563792063314523517192355939187109470367431640625e1
    },                    /* _dbT */
    0x4308000000000000uL, /* _dbShifter */
    0x40862de9u,          /* _iDomainRange 0x40862dea45ee3e06
                             =(1024*2^K-0.5)*log(2)/2^K*/
    255,                  /* =(1<<(K+5))-1 _iIndexMask */
    0x4308000000000001uL  /* _dM1 */
};                        /*dCosh_Table*/
static __constant _iml_v2_dp_union_t __dsinh_ha_CoutTab[149] = {
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
    /* Coefficients for exp(R) - 1 approximation by polynomial p(R) */
    0x00000000, 0x3FE00000, /* A2 = .500000000000000 */
    0x555548F8, 0x3FC55555, /* A3 = .166666666666579 */
    0x55558FCC, 0x3FA55555, /* A4 = .041666666666771 */
    0x3AAF20D3, 0x3F811112, /* A5 = .008333341995140 */
    0x1C2A3FFD, 0x3F56C16A, /* A6 = .001388887045923 */
    /* Coefficients for Taylor approximation of sinh(.) */
    0x55555555, 0x3FC55555, /* Q3 = .166666666666667 */
    0x11111111, 0x3F811111, /* Q5 = .008333333333333 */
    0x1A01A01A, 0x3F2A01A0, /* Q7 = .000198412698413 */
    0xA556C734, 0x3EC71DE3, /* Q9 = .000002755731922 */
    /* TWO_TO_THE_K_DIV_LN2 = 2^6/ln(2.0) rounded to double */
    0x652B82FE, 0x40571547, /* 92.332482616893658 */
    /* Right Shifter */
    0x00000000, 0x43380000, /* RS = 2^52 + 2^51 */
    /* RS_MuL = 2^27 + 1 stored in double */
    0x02000000, 0x41A00000,
    /* Overflow Threshold */
    0x8FB9F87E, 0x408633CE, /* OVF = 710.475860073943977 */
    /* Two parts of ln(2.0)/65 */
    0xFEFA0000, 0x3F862E42, /* LOG_HI = .010830424696223 */
    0xBC9E3B3A, 0x3D1CF79A, /* LOG_LO = 2.572804622327669e-14 */
    /* TINY and HUGE_VALUE values to process (under-) overflow */
    0x00000001, 0x00100000, 0xFFFFFFFF, 0x7FEFFFFF,
    /* TAYLOR_THRESHOLD = 2^{-5.2} rounded to double */
    0xDADBE120, 0x3F9BDB8C,
    /* EXP_OF_X_DIV_2_THRESHOLD = (52+10)/2*ln(2) rounded to double */
    0xE7026820, 0x40357CD0};
/*
//
//   Implementation of HA (High Accuracy) version of double precision vector
//   hyperbolic sine function starts here.
//
*/
__attribute__((always_inline)) inline int
__ocl_svml_internal_dsinh_ha(double *a, double *r) {
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
  //    _IML_DCOREFN_PROLOG2_IN_C(0, _IML_MODEX87_NEAR53_IN_C, _MODE_UNCHANGED,
  //    n, a, r );
  /* Set all bits of scale to 0.                                           */
  /* Only bits of exponent field will be updated then before each use of   */
  /* scale. Notice, that all other bits (i.e. sign and significand) should */
  /* be kept 0 across iterations. Otherwise, they should be explicitly set */
  /* to 0 before each use of scale                                         */
  scale = ((__constant double *)__dsinh_ha_CoutTab)[1];
  dbIn = (*a);
  /* Filter out INFs and NaNs */
  if ((((((_iml_v2_dp_union_t *)&dbIn)->dwords.hi_dword >> 20) & 0x7FF) !=
       0x7FF)) {
    /* Here if argument is finite double precision number */
    exp = ((((_iml_v2_dp_union_t *)&dbIn)->dwords.hi_dword >> 20) & 0x7FF);
    /* Check if argument is normalized */
    if (exp > 0) {
      absAi = dbIn;
      (((_iml_v2_dp_union_t *)&absAi)->dwords.hi_dword =
           (((_iml_v2_dp_union_t *)&absAi)->dwords.hi_dword & 0x7FFFFFFF) |
           ((_iml_uint32_t)(0) << 31));
      /* Check if dbIn falls into "Near 0" range */
      if (exp > (0x3FF - 54)) {
        /* Here if argument is not within "Near 0" interval */
        /* Check if sinh(dbIn) overflows */
        if (absAi < ((__constant double *)__dsinh_ha_CoutTab)[142]) {
          /* Here if sinh doesn't overflow */
          /* Check if |dbIn| is big enough to             */
          /* approximate sinh(|dbIn|) by exp( |dbIn| )/2  */
          if (absAi < ((__constant double *)__dsinh_ha_CoutTab)[148]) {
            /* Here if |dbIn| is not big enough to         */
            /* approximate sinh(|dbIn|) by exp( |dbIn| )/2 */
            /* Check if path 5) or 6) should follow */
            if (absAi >= ((__constant double *)__dsinh_ha_CoutTab)[147]) {
              /* Path 6 */
              /* Range Reduction part, path 6a) */
              tmp1 = (absAi * ((__constant double *)__dsinh_ha_CoutTab)[139]);
              Nj = (tmp1 + ((__constant double *)__dsinh_ha_CoutTab)[140]);
              M = (Nj - ((__constant double *)__dsinh_ha_CoutTab)[140]);
              tmp1 =
                  (absAi - M * ((__constant double *)__dsinh_ha_CoutTab)[143]);
              MLog = (-M * ((__constant double *)__dsinh_ha_CoutTab)[144]);
              /* R + RLo := tmp1 + MLog */
              v1 = ((tmp1) + (MLog));
              v2 = ((tmp1)-v1);
              v3 = (v1 + v2);
              v2 = ((MLog) + v2);
              v3 = ((tmp1)-v3);
              v3 = (v2 + v3);
              R = v1;
              RLo = v3;
              ;
              /* Splitting R into RHi and RMid */
              v1 = ((R) * (((__constant double *)__dsinh_ha_CoutTab)[141]));
              v2 = (v1 - (R));
              v1 = (v1 - v2);
              v2 = ((R)-v1);
              RHi = v1;
              RMid = v2;
              ;
              /* Approximation part: polynomial series, */
              /*                              path 6b)  */
              rsq = R * R;
              podd = ((((__constant double *)__dsinh_ha_CoutTab)[133] * rsq +
                       ((__constant double *)__dsinh_ha_CoutTab)[131]) *
                      rsq) *
                     R;
              peven = (((((__constant double *)__dsinh_ha_CoutTab)[134] * rsq +
                         ((__constant double *)__dsinh_ha_CoutTab)[132]) *
                        rsq) +
                       ((__constant double *)__dsinh_ha_CoutTab)[130]) *
                      rsq;
              /* Final reconstuction starts here, path 6c) */
              /* Get N and j from Nj's significand */
              N = (((_iml_v2_dp_union_t *)&Nj)->dwords.lo_dword);
              j = N & ((1 << 6) - 1);
              N = N >> 6;
              /* Obtain scale = 2^{N - 1 + bias} */
              N = N + 0x3FF;
              N = N & 0x7FF;
              (((_iml_v2_dp_union_t *)&scale)->dwords.hi_dword =
                   (((_iml_v2_dp_union_t *)&scale)->dwords.hi_dword & 0x800FFFFF) |
                   (((_iml_uint32_t)(N - 1) & 0x7FF) << 20));
              TpHi = ((__constant double *)__dsinh_ha_CoutTab)[2 * (j)] * scale;
              TpLo = ((__constant double *)__dsinh_ha_CoutTab)[2 * (j) + 1] *
                     scale;
              /* Obtain scale = 2^{-N - 2 + bias} */
              N = 2 * 0x3FF - N;
              (((_iml_v2_dp_union_t *)&scale)->dwords.hi_dword =
                   (((_iml_v2_dp_union_t *)&scale)->dwords.hi_dword & 0x800FFFFF) |
                   (((_iml_uint32_t)(N - 2) & 0x7FF) << 20));
              TmHi = ((__constant double *)__dsinh_ha_CoutTab)[2 * (64 - (j))] *
                     scale;
              TmLo = ((__constant double *)
                          __dsinh_ha_CoutTab)[2 * (64 - (j)) + 1] *
                     scale;
              /* Get intermediate values */
              /* TdH + TdL = TpHi + TpLo - TmHi - TmLo */
              v1 = ((TpHi) + (-TmHi));
              tmp1 = ((TpHi)-v1);
              v2 = (tmp1 + (-TmHi));
              TdH = v1;
              TdL = v2;
              ;
              TdL -= TmLo;
              TdL += TpLo;
              /* Re-split TdH + TdL so that the most */
              /* significant part of the sum resides */
              /* in TdH                              */
              v1 = ((TdH) + (TdL));
              tmp1 = ((TdH)-v1);
              v2 = (tmp1 + (TdL));
              TdH = v1;
              TdL = v2;
              ;
              /* TsH + TsL = TpHi + TpLo + TmHi + TmLo */
              v1 = ((TpHi) + (TmHi));
              tmp1 = ((TpHi)-v1);
              v2 = (tmp1 + (TmHi));
              TsH = v1;
              TsL = v2;
              ;
              tmp1 = (TpLo + TmLo);
              TsL += tmp1;
              /* Re-split TsH + TsL so that the most */
              /* significant part of the sum resides */
              /* in TsH                              */
              v1 = ((TsH) + (TsL));
              tmp1 = ((TsH)-v1);
              v2 = (tmp1 + (TsL));
              TsH = v1;
              TsL = v2;
              ;
              /* Splitting TsH into high and low parts */
              v1 = ((TsH) * (((__constant double *)__dsinh_ha_CoutTab)[141]));
              v2 = (v1 - (TsH));
              v1 = (v1 - v2);
              v2 = ((TsH)-v1);
              TsHi = v1;
              TsLo = v2;
              ;
              /* Gather the items in pLo and pHi */
              pLo = (RLo * TsL);
              pLo += (podd * TsL);
              pLo += (peven * TdL);
              pLo += (R * TsL);
              pLo += (RLo * TsH);
              pLo += TdL;
              ph = (podd * TsH);
              pl = (peven * TdH);
              /* pHi + pl = podd * TsH + peven * TdH */
              v1 = ((ph) + (pl));
              tmp1 = ((ph)-v1);
              v2 = (tmp1 + (pl));
              pHi = v1;
              pl = v2;
              ;
              pLo += pl;
              pLo += (RMid * TsLo);
              pLo += (RHi * TsLo);
              pLo += (RMid * TsHi);
              RHi = (RHi * TsHi);
              /* pHi + pl = RHi + pHi */
              v1 = ((RHi) + (pHi));
              tmp1 = ((RHi)-v1);
              v2 = (tmp1 + (pHi));
              pHi = v1;
              pl = v2;
              ;
              pLo += pl;
              /* pHi + pl = TdH + pHi */
              v1 = ((TdH) + (pHi));
              tmp1 = ((TdH)-v1);
              v2 = (tmp1 + (pHi));
              pHi = v1;
              pl = v2;
              ;
              pLo += pl;
              p = (pHi + pLo);
              (((_iml_v2_dp_union_t *)&p)->dwords.hi_dword =
                   (((_iml_v2_dp_union_t *)&p)->dwords.hi_dword & 0x7FFFFFFF) |
                   ((_iml_uint32_t)((
                        ((_iml_v2_dp_union_t *)&dbIn)->dwords.hi_dword >> 31))
                    << 31));
              (*r) = p;
            } else {
              /* Here if |dbIn| < TAYLOR_THRESHOLD, Path 5 */
              rsq = (absAi * absAi);
              p = (absAi *
                   (rsq *
                    (((__constant double *)__dsinh_ha_CoutTab)[135] +
                     rsq *
                         (((__constant double *)__dsinh_ha_CoutTab)[136] +
                          rsq *
                              (((__constant double *)__dsinh_ha_CoutTab)[137] +
                               rsq * ((__constant double *)
                                          __dsinh_ha_CoutTab)[138])))));
              p += absAi;
              (((_iml_v2_dp_union_t *)&p)->dwords.hi_dword =
                   (((_iml_v2_dp_union_t *)&p)->dwords.hi_dword & 0x7FFFFFFF) |
                   ((_iml_uint32_t)((
                        ((_iml_v2_dp_union_t *)&dbIn)->dwords.hi_dword >> 31))
                    << 31));
              (*r) = p;
            }
          } else {
            /* Path 7 */
            /* Range Reduction part, 7a) */
            tmp1 = absAi * ((__constant double *)__dsinh_ha_CoutTab)[139];
            Nj = (tmp1 + ((__constant double *)__dsinh_ha_CoutTab)[140]);
            M = (Nj - ((__constant double *)__dsinh_ha_CoutTab)[140]);
            R = (absAi - M * ((__constant double *)__dsinh_ha_CoutTab)[143]);
            R -= (M * ((__constant double *)__dsinh_ha_CoutTab)[144]);
            /* Approximation part: polynomial series, 7b) */
            p = ((((((__constant double *)__dsinh_ha_CoutTab)[134] * R +
                    ((__constant double *)__dsinh_ha_CoutTab)[133]) *
                       R +
                   ((__constant double *)__dsinh_ha_CoutTab)[132]) *
                      R +
                  ((__constant double *)__dsinh_ha_CoutTab)[131]) *
                     R +
                 ((__constant double *)__dsinh_ha_CoutTab)[130]);
            p = (p * R);
            p = (p * R + R);
            /* Final reconstruction starts here, 7c) */
            /* Get N and j from Nj's significand */
            N = (((_iml_v2_dp_union_t *)&Nj)->dwords.lo_dword);
            j = N & ((1 << 6) - 1);
            N = N >> 6;
            N += 0x3FF;
            /* p = (T[j] * p +  D[j]) + T[j] */
            p *= ((__constant double *)__dsinh_ha_CoutTab)[2 * (j)];
            p += ((__constant double *)__dsinh_ha_CoutTab)[2 * (j) + 1];
            p += ((__constant double *)__dsinh_ha_CoutTab)[2 * (j)];
            /* N = N - 1 */
            N = (N - 1) & 0x7FF;
            /* Check if path 7.1) or 7.2) should follow */
            if (N <= (0x7FF - 1)) {
              /* Path 7.1) */
              /* scale = 2^N */
              (((_iml_v2_dp_union_t *)&scale)->dwords.hi_dword =
                   (((_iml_v2_dp_union_t *)&scale)->dwords.hi_dword & 0x800FFFFF) |
                   (((_iml_uint32_t)(N)&0x7FF) << 20));
              /* scale * (T[j] + (D[j] + T[j] * p)) */
              p = (p * scale);
            } else {
              /* Path 7.2) "scale overflow" */
              /* scale = 2^(N - 1) */
              (((_iml_v2_dp_union_t *)&scale)->dwords.hi_dword =
                   (((_iml_v2_dp_union_t *)&scale)->dwords.hi_dword & 0x800FFFFF) |
                   (((_iml_uint32_t)(N - 1) & 0x7FF) << 20));
              /* 2.0*(scale * (T[j] + (D[j] + T[j] * p))) */
              p = (p * scale);
              p = (p * ((__constant double *)__dsinh_ha_CoutTab)[128]);
            }
            (((_iml_v2_dp_union_t *)&p)->dwords.hi_dword =
                 (((_iml_v2_dp_union_t *)&p)->dwords.hi_dword & 0x7FFFFFFF) |
                 ((_iml_uint32_t)((
                      ((_iml_v2_dp_union_t *)&dbIn)->dwords.hi_dword >> 31))
                  << 31));
            (*r) = p;
          }
        } else {
          /* Here if sinh overflows, Path 4 */
          (*r) = ((__constant double *)__dsinh_ha_CoutTab)[146] * dbIn;
          //_IML_FUNC_NAME_CALL_EM(dError,(_IML_SCODE_IN_C() =
          //IML_STATUS_OVERFLOW,i,a,a, r, r, _IML_THISFUNC_NAME));
          nRet = 3;
        }
      } else {
        /* Here if argument is within "Near 0" interval, Path 3 */
        (*r) = (((__constant double *)__dsinh_ha_CoutTab)[0] +
                ((__constant double *)__dsinh_ha_CoutTab)[145]) *
               dbIn;
      }
    } else {
      /* Here if dbIn is zero or denormalized, Path 2 */
      v1 = dbIn * ((__constant double *)__dsinh_ha_CoutTab)[145];
      (*r) = dbIn + v1;
    }
  } else {
    /* Here if argument is infinity or NaN, Path 1 */
    (*r) = dbIn + dbIn;
  }
  //_IML_COREFN_EPILOG2_IN_C();
  return nRet;
}
double __ocl_svml_sinh_ha(double x) {
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
    double dSinh_r;
    double dOut;
    double dTph;
    double dTpl;
    double dTnh;
    double dTnl;
    double dAbsTnl;
    double dTdif;
    double dTn;
    double dG1;
    double dG2;
    double dG1_low;
    double dG2_low;
    double dH3;
    unsigned long lM;
    double dXSign;
    unsigned int iAbsX;
    double dAbsX;
    unsigned long lIndex;
    unsigned long lX;
    unsigned int iRangeMask;
    double dMask;
    double dbInvLn2;
    double dbShifter;
    double dbShifterP1;
    double dbLn2[2];
    double dPC[8];
    unsigned long lIndexMask;
    double dSign;
    double dHalf;
    double dZero;
    unsigned int iDomainRange;
    dbInvLn2 = as_double(__ocl_svml_internal_dsinh_ha_data._dbInvLn2);
    dbShifter = as_double(__ocl_svml_internal_dsinh_ha_data._dbShifter_UISA);
    lIndexMask = (__ocl_svml_internal_dsinh_ha_data._lIndexMask_UISA);
    iDomainRange = (__ocl_svml_internal_dsinh_ha_data._iDomainRange_UISA);
    dPC[1] = as_double(__ocl_svml_internal_dsinh_ha_data._dPC2_UISA);
    dPC[2] = as_double(__ocl_svml_internal_dsinh_ha_data._dPC3_UISA);
    dPC[3] = as_double(__ocl_svml_internal_dsinh_ha_data._dPC4_UISA);
    dPC[4] = as_double(__ocl_svml_internal_dsinh_ha_data._dPC5_UISA);
    dPC[5] = as_double(__ocl_svml_internal_dsinh_ha_data._dPC6_UISA);
    dPC[6] = as_double(__ocl_svml_internal_dsinh_ha_data._dPC7_UISA);
    dPC[7] = as_double(__ocl_svml_internal_dsinh_ha_data._dPC8_UISA);
    dSign = as_double(__ocl_svml_internal_dsinh_ha_data._dSign);
    // Compute argument sign and absolute argument
    dXSign = as_double((as_ulong(dSign) & as_ulong(va1)));
    dAbsX = as_double((as_ulong(dXSign) ^ as_ulong(va1)));
    // dM = x*2^K/log(2) + RShifter
    dM = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dAbsX, dbInvLn2, dbShifter);
    // Check for overflow\underflow
    lX = as_ulong(dAbsX);
    iAbsX = ((unsigned int)((unsigned long)lX >> 32));
    iRangeMask = ((unsigned int)(-(signed int)((signed int)iAbsX >
                                               (signed int)iDomainRange)));
    vm = 0;
    vm = iRangeMask;
    dbShifterP1 =
        as_double(__ocl_svml_internal_dsinh_ha_data._dbShifterP1_UISA);
    dMask = as_double(
        (unsigned long)((dbShifterP1 == dM) ? 0xffffffffffffffff : 0x0));
    dM = as_double((((~as_ulong(dMask)) & as_ulong(dM)) |
                    (as_ulong(dMask) & as_ulong(dbShifter))));
    // Index and lookup
    lM = as_ulong(dM);
    // Masking index
    lIndex = (lM & lIndexMask);
    // Split j and N
    lM = (lM ^ lIndex);
    // Scale index
    lIndex = ((unsigned long)(lIndex) << (3));
    // Gather PHi and NHi
    dTph = as_double(
        ((unsigned long *)((double *)(&__ocl_svml_internal_dsinh_ha_data
                                           ._dExp_tbl_PH[0])))[lIndex >> 3]);
    dTnh = as_double(
        ((unsigned long *)((double *)(&__ocl_svml_internal_dsinh_ha_data
                                           ._dExp_tbl_NH[0])))[lIndex >> 3]);
    // Gather PLo and NLo
    dTpl = as_double(
        ((unsigned long *)((double *)(&__ocl_svml_internal_dsinh_ha_data
                                           ._dExp_tbl_PL[0])))[lIndex >> 3]);
    dTnl = as_double(
        ((unsigned long *)((double *)(&__ocl_svml_internal_dsinh_ha_data
                                           ._dExp_tbl_NL[0])))[lIndex >> 3]);
    // Reduced argument R:
    // dN = dM - RShifter
    dN = (dM - dbShifter);
    dbLn2[0] = as_double(__ocl_svml_internal_dsinh_ha_data._dbLn2hi);
    // dR = dX - dN*Log2_hi/2^K
    dR = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(-(dbLn2[0]), dN, dAbsX);
    dbLn2[1] = as_double(__ocl_svml_internal_dsinh_ha_data._dbLn2lo);
    // dR = (dX - dN*Log2_hi/2^K) - dN*Log2_lo/2^K
    dR = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(-(dbLn2[1]), dN, dR);
    // dR2 = dR^2
    dR2 = (dR * dR);
    // G1,G2,G3: dTdif,dTn * 2^N,2^(-N):
    // lM now is an EXP(2^N)
    lM = ((unsigned long)(lM) << ((52 - 4)));
    lX = as_ulong(dTph);
    lX = (lX + lM);
    // dTph = dTph*2^N
    dTph = as_double(lX);
    lX = as_ulong(dTnh);
    lX = (lX - lM);
    // dTnh = dTnh*2^-N
    dTnh = as_double(lX);
    dG1 = (dTph + dTnh);
    dG2 = (dTph - dTnh);
    lX = as_ulong(dTpl);
    // dTpl = dTpl*2^N
    lX = (lX + lM);
    dTpl = as_double(lX);
    // if |dTnl| < dM, then scaled dTnl = 0
    dAbsTnl = as_double((~(as_ulong(dSign)) & as_ulong(dTnl)));
    dM = as_double(lM);
    dMask =
        as_double((unsigned long)((dAbsTnl < dM) ? 0xffffffffffffffff : 0x0));
    lX = as_ulong(dTnl);
    // dTnl = dTnl*2^-N
    lX = (lX - lM);
    dTnl = as_double(lX);
    dTnl = as_double((~(as_ulong(dMask)) & as_ulong(dTnl)));
    dH3 = (dG2 - dTph);
    dH3 = (dTnh + dH3);
    dG2_low = (dTpl - dH3);
    dG2_low = (dG2_low - dTnl);
    // poly(r) = G1(1 + a2*r^2 + a4*r^4) + G2*(r+ a3*r^3 +a5*r^5)
    dM = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dPC[6], dR2, dPC[4]);
    dOut = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dPC[7], dR2, dPC[5]);
    dM = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dM, dR2, dPC[2]);
    dOut = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dOut, dR2, dPC[3]);
    dOut = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dOut, dR2, dPC[1]);
    dM = (dM * dR2);
    dOut = (dOut * dR2);
    dM = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dM, dR, dR);
    dOut = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dOut, dG2, dG2_low);
    dOut = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dM, dG1, dOut);
    dOut = (dOut + dG2);
    // Set result sign
    vr1 = as_double((as_ulong(dXSign) | as_ulong(dOut)));
  }
  if (__builtin_expect((vm) != 0, 0)) {
    double __cout_a1;
    double __cout_r1;
    ((double *)&__cout_a1)[0] = va1;
    ((double *)&__cout_r1)[0] = vr1;
    __ocl_svml_internal_dsinh_ha(&__cout_a1, &__cout_r1);
    vr1 = ((double *)&__cout_r1)[0];
  }
  r = vr1;
  return r;
}

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
  /*Shared*/
  unsigned int _sExp_tbl_PH[32];
  unsigned int _sExp_tbl_PL[32];
  unsigned int _sExp_tbl_NH[32];
  unsigned int _sShifter_UISA;
  unsigned int _iIndexMask_UISA;
  unsigned int _iDomainRange_UISA;
  unsigned int _sPC1_UISA;
  unsigned int _sPC2_UISA;
  unsigned int _sPC3_UISA;
  unsigned int _sInvLn2;
  unsigned int _sInvLn2lo;
  unsigned int _sLn2hi;
  unsigned int _sLn2lo;
  unsigned int _sSign;
  unsigned int _sOne;
  unsigned int _sNOne;
  unsigned int _iExpMask;
  /* Shared name but with different value */
  unsigned int _sShifter;
  unsigned int _iIndexMask;
  unsigned int _iDomainRange;
  unsigned int _dbT[256][4]; // precalc poly(4) coeff
} __ocl_svml_internal_scosh_ha_data_t;
static __ocl_svml_internal_scosh_ha_data_t __ocl_svml_internal_scosh_ha_data = {
    {/* _sExp_tbl_PH 2^(i/32-1), i=0..31 */
     0x3f000000u, 0x3f02cd87u, 0x3f05aac3u, 0x3f08980fu, 0x3f0b95c2u,
     0x3f0ea43au, 0x3f11c3d3u, 0x3f14f4f0u, 0x3f1837f0u, 0x3f1b8d3au,
     0x3f1ef532u, 0x3f227043u, 0x3f25fed7u, 0x3f29a15bu, 0x3f2d583fu,
     0x3f3123f6u, 0x3f3504f3u, 0x3f38fbafu, 0x3f3d08a4u, 0x3f412c4du,
     0x3f45672au, 0x3f49b9beu, 0x3f4e248cu, 0x3f52a81eu, 0x3f5744fdu,
     0x3f5bfbb8u, 0x3f60ccdfu, 0x3f65b907u, 0x3f6ac0c7u, 0x3f6fe4bau,
     0x3f75257du, 0x3f7a83b3u},
    {/*  for i in [|0,...,31|] do printsingle( 2^(i/32-1) -
        round(2^(i/32-1),SG,RN)); */
     0x00000000u, 0xb2cea7a9u, 0x32cf9891u, 0xb2feda4bu, 0xb1e0aba1u,
     0xb2e97465u, 0x32e75624u, 0xb2ae0212u, 0x32a31b71u, 0xb28c5563u,
     0x32c12342u, 0x3043125au, 0xb2ac9d5eu, 0xb2962b08u, 0xb1adeaf6u,
     0xb2fc5aa8u, 0x324fe77au, 0x328ec5f7u, 0xb2c14fe8u, 0xb256663eu,
     0x318aa837u, 0xb2f323a2u, 0x31a8fc24u, 0xb2dc1daau, 0xb254a58au,
     0xb2d04a1cu, 0xb19eab59u, 0xb1c41be6u, 0xb1c116deu, 0xb2c8464au,
     0x31a92436u, 0xb2123758u},
    {/* _sExp_tbl_NH 2^(-i/32-1), i=0..31 */
     0x3f000000u, 0x3efa83b3u, 0x3ef5257du, 0x3eefe4bau, 0x3eeac0c7u,
     0x3ee5b907u, 0x3ee0ccdfu, 0x3edbfbb8u, 0x3ed744fdu, 0x3ed2a81eu,
     0x3ece248cu, 0x3ec9b9beu, 0x3ec5672au, 0x3ec12c4du, 0x3ebd08a4u,
     0x3eb8fbafu, 0x3eb504f3u, 0x3eb123f6u, 0x3ead583fu, 0x3ea9a15bu,
     0x3ea5fed7u, 0x3ea27043u, 0x3e9ef532u, 0x3e9b8d3au, 0x3e9837f0u,
     0x3e94f4f0u, 0x3e91c3d3u, 0x3e8ea43au, 0x3e8b95c2u, 0x3e88980fu,
     0x3e85aac3u, 0x3e82cd87u},
    0x48c00000u, /* 1.5*2^18 _sShifter_UISA */
    0x0000001fu, /* _iIndexMask_UISA   */
    0x42AEAC4Eu, /* _iDomainRange_UISA */
    0x3F800000u, /* _sPC1_UISA=1       */
    0x3f00010fu, /* _sPC2_UISA         */
    0x3e2aaacdu, /* _sPC3_UISA         */
    0x3FB8AA3Bu,
    /* _sInvLn2  */ // k=0
    0x32A57060u,    /* _sInvLn2lo*/
    0x3F317000u,    /* _sLn2hi   */
    0x3805fdf4u,    /* _sLn2lo   */
    0x80000000u,    /* _sSign    */
    0x3f800000u,    /* _sOne     */
    0xbf800000u,    /* _sNOne    */
    0x7f800000u,    /* _iExpMask */
    0x49400000u,    /* _sShifter */
    255,            /* _iIndexMask = (1<<(K+4))-1 */
    0x42B16700u,    /* _iDomainRange =(128*2^K-0.5)*log(2)/2^K*/
    {               //_dbT
     {0x384cffecu, 0x33e35aa1u, 0x3efffcccu, 0x3aec9983u}, // log2(rel_err)= 33
     {0xb82404a3u, 0x3d31804eu, 0x3f002009u,
      0x3bec9e23u}, // log2(rel_err)= 29 every where
     {0xb824ab46u, 0x3db1aaf4u, 0x3f007c5cu, 0x3c6cbcdcu},
     {0xb82546cdu, 0x3e057590u, 0x3f01166cu, 0x3cb1efc2u},
     {0xb8265381u, 0x3e3255c8u, 0x3f01ee84u, 0x3cedafd2u},
     {0xb827e67bu, 0x3e5f8bb1u, 0x3f03050cu, 0x3d15161fu},
     {0xb82994afu, 0x3e869683u, 0x3f045a89u, 0x3d3388acu},
     {0xb82bd2d0u, 0x3e9da7dau, 0x3f05efa0u, 0x3d524691u},
     {0xb82dcdd0u, 0x3eb504f3u, 0x3f07c512u, 0x3d716d19u},
     {0xb830bc36u, 0x3eccb908u, 0x3f09dbc3u, 0x3d888912u},
     {0xb833a7ceu, 0x3ee4cf7eu, 0x3f0c34b2u, 0x3d9875d8u},
     {0xb8373007u, 0x3efd53e6u, 0x3f0ed101u, 0x3da8dac8u},
     {0xb83ac480u, 0x3f0b2905u, 0x3f11b1f0u, 0x3db96f6cu},
     {0xb83ed7f0u, 0x3f17eaf5u, 0x3f14d8e2u, 0x3dca9f1eu},
     {0xb8430b02u, 0x3f24f5e6u, 0x3f18475au, 0x3ddbf03du},
     {0xb847cf3du, 0x3f32501bu, 0x3f1bfeffu, 0x3deddf6eu},
     {0xb84d018au, 0x3f400000u, 0x3f20019au, 0x3e000080u},
     {0xb8527c5cu, 0x3f4e0c28u, 0x3f245118u, 0x3e094b9au},
     {0xb858a12eu, 0x3f5c7b52u, 0x3f28ef8cu, 0x3e12ff9fu},
     {0xb85ed915u, 0x3f6b546fu, 0x3f2ddf2du, 0x3e1cea7cu},
     {0xb8658c45u, 0x3f7a9ea1u, 0x3f33225bu, 0x3e27269eu},
     {0xb86c902bu, 0x3f8530a1u, 0x3f38bb9du, 0x3e318c3au},
     {0xb8748beau, 0x3f8d51f1u, 0x3f3eada5u, 0x3e3c8e51u},
     {0xb87c52ceu, 0x3f95b72au, 0x3f44fb4cu, 0x3e47c49cu},
     {0xb8825a5bu, 0x3f9e6455u, 0x3f4ba79bu, 0x3e53217du},
     {0xb886e1dau, 0x3fa75d9cu, 0x3f52b5c7u, 0x3e5f1502u},
     {0xb88bc76au, 0x3fb0a74fu, 0x3f5a2934u, 0x3e6b9d6fu},
     {0xb890a8c9u, 0x3fba45e6u, 0x3f620575u, 0x3e784d02u},
     {0xb8960bdeu, 0x3fc43dffu, 0x3f6a4e53u, 0x3e82ca9eu},
     {0xb89baa83u, 0x3fce9465u, 0x3f7307c8u, 0x3e89a476u},
     {0xb8a1781fu, 0x3fd94e0fu, 0x3f7c3605u, 0x3e90d20fu},
     {0xb8a7a0bau, 0x3fe47025u, 0x3f82eebau, 0x3e983da5u},
     {0xb8ae014du, 0x3ff00000u, 0x3f88015cu, 0x3ea0009eu},
     {0xb8b4d920u, 0x3ffc032fu, 0x3f8d5559u, 0x3ea80281u},
     {0xb8bbf3d2u, 0x40043fbcu, 0x3f92ed40u, 0x3eb03ba5u},
     {0xb8c3cbf2u, 0x400abd6du, 0x3f98cbc2u, 0x3eb8d42cu},
     {0xb8cbe598u, 0x40117dc8u, 0x3f9ef3b0u, 0x3ec21f47u},
     {0xb8d420fcu, 0x4018840eu, 0x3fa567ffu, 0x3ecb5b83u},
     {0xb8dc848au, 0x401fd39du, 0x3fac2bc9u, 0x3ed536d0u},
     {0xb8e57e19u, 0x40276ffau, 0x3fb3424fu, 0x3edf3100u},
     {0xb8ef1afeu, 0x402f5cccu, 0x3fbaaef9u, 0x3ee9b23eu},
     {0xb8f9368au, 0x40379de2u, 0x3fc27558u, 0x3ef4b801u},
     {0xb901cf51u, 0x40403733u, 0x3fca9928u, 0x3f0033aau},
     {0xb90716f9u, 0x40492ce3u, 0x3fd31e52u, 0x3f060acbu},
     {0xb90cedb6u, 0x4052833du, 0x3fdc08f0u, 0x3f0c6353u},
     {0xb9131f2au, 0x405c3ec0u, 0x3fe55d4au, 0x3f12e1a7u},
     {0xb9194368u, 0x40666419u, 0x3fef1fdau, 0x3f19928au},
     {0xb91fb51au, 0x4070f827u, 0x3ff95552u, 0x3f20a51cu},
     {0xb9268140u, 0x407c0000u, 0x4002014du, 0x3f2800a7u},
     {0xb92dd054u, 0x4083c078u, 0x4007966au, 0x3f2fce34u},
     {0xb9354880u, 0x4089c040u, 0x400d6caeu, 0x3f37c699u},
     {0xb93ce8a8u, 0x4090023au, 0x401386e7u, 0x3f3fec8fu},
     {0xb9455bebu, 0x40968966u, 0x4019e805u, 0x3f48d373u},
     {0xb94dc52eu, 0x409d58e9u, 0x40209317u, 0x3f51e34eu},
     {0xb95702b6u, 0x40a47408u, 0x40278b53u, 0x3f5b60f3u},
     {0xb95fe8edu, 0x40abde2eu, 0x402ed410u, 0x3f650c1bu},
     {0xb969f492u, 0x40b39ae9u, 0x403670d1u, 0x3f6f925fu},
     {0xb973f33fu, 0x40bbadf3u, 0x403e653cu, 0x3f7a48b4u},
     {0xb97ea296u, 0x40c41b2cu, 0x4046b525u, 0x3f82cce7u},
     {0xb984e287u, 0x40cce6a2u, 0x404f648au, 0x3f888ddcu},
     {0xb98a962bu, 0x40d6148du, 0x40587797u, 0x3f8eb585u},
     {0xb990dc53u, 0x40dfa957u, 0x4061f2aau, 0x3f9526f5u},
     {0xb997263au, 0x40e9a99bu, 0x406bda4fu, 0x3f9bd6a3u},
     {0xb99d9a31u, 0x40f41a28u, 0x40763349u, 0x3fa2aafeu},
     {0xb9a4813bu, 0x40ff0000u, 0x40808149u, 0x3faa00a9u},
     {0xb9abee1fu, 0x41053031u, 0x408626aeu, 0x3fb17b34u},
     {0xb9b35da8u, 0x410b2062u, 0x408c0c89u, 0x3fb9596bu},
     {0xb9bb8fd9u, 0x4111536du, 0x409235b1u, 0x3fc1c6a5u},
     {0xb9c3997eu, 0x4117cc4eu, 0x4098a51au, 0x3fca5888u},
     {0xb9cc2e3au, 0x411e8e20u, 0x409f5dddu, 0x3fd37146u},
     {0xb9d5623eu, 0x41259c23u, 0x40a66335u, 0x3fdcd780u},
     {0xb9de63a1u, 0x412cf9bbu, 0x40adb880u, 0x3fe682e0u},
     {0xb9e82af3u, 0x4134aa71u, 0x40b56146u, 0x3ff0ce74u},
     {0xb9f2a26bu, 0x413cb1f8u, 0x40bd6135u, 0x3ffb70f2u},
     {0xb9fd4391u, 0x4145142bu, 0x40c5bc24u, 0x4003553eu},
     {0xba041568u, 0x414dd512u, 0x40ce7617u, 0x400924a1u},
     {0xba0a1049u, 0x4156f8e1u, 0x40d79341u, 0x400f4a12u},
     {0xba104b9du, 0x416083fdu, 0x40e11802u, 0x4015ae4bu},
     {0xba168eeeu, 0x416a7afcu, 0x40eb08ecu, 0x401c53aeu},
     {0xba1d2377u, 0x4174e2a8u, 0x40f56ac7u, 0x40233675u},
     {0xba24013au, 0x417fc000u, 0x41002148u, 0x402a80a9u},
     {0xba2b7592u, 0x41858c1fu, 0x4105cabfu, 0x4031fa6fu},
     {0xba3302f4u, 0x418b786au, 0x410bb480u, 0x4039e615u},
     {0xba3af9a3u, 0x4191a7bau, 0x4111e163u, 0x4042292eu},
     {0xba4308e2u, 0x41981d08u, 0x4118545fu, 0x404ab9cdu},
     {0xba4b887bu, 0x419edb6eu, 0x411f108eu, 0x4053c0c8u},
     {0xba54ba1eu, 0x41a5e62au, 0x4126192du, 0x405d2127u},
     {0xba5e024eu, 0x41ad409eu, 0x412d719cu, 0x4066f48cu},
     {0xba68188du, 0x41b4ee53u, 0x41351d64u, 0x40711d7cu},
     {0xba722e35u, 0x41bcf2f9u, 0x413d2033u, 0x407bcefbu},
     {0xba7d0bd2u, 0x41c5526au, 0x41457de4u, 0x4083954cu},
     {0xba845225u, 0x41ce10adu, 0x414e3a7cu, 0x4089724bu},
     {0xba89ced0u, 0x41d731f6u, 0x41575a2bu, 0x408f6f34u},
     {0xba902770u, 0x41e0baa6u, 0x4160e158u, 0x4095e41bu},
     {0xba96991cu, 0x41eaaf54u, 0x416ad494u, 0x409c7cefu},
     {0xba9d25cau, 0x41f514c8u, 0x417538a7u, 0x40a35953u},
     {0xbaa4013bu, 0x41fff000u, 0x41800948u, 0x40aaa0aau},
     {0xbaab376fu, 0x4205a31au, 0x4185b3c3u, 0x40b24231u},
     {0xbab30c48u, 0x420b8e6cu, 0x418b9e7eu, 0x40ba0941u},
     {0xbaba9414u, 0x4211bccdu, 0x4191cc4fu, 0x40c255c9u},
     {0xbac344bfu, 0x42183136u, 0x41984031u, 0x40cafa15u},
     {0xbacbbf0fu, 0x421eeec1u, 0x419efd3bu, 0x40d3fca0u},
     {0xbad5101bu, 0x4225f8abu, 0x41a606acu, 0x40dd6f83u},
     {0xbadde9f9u, 0x422d5257u, 0x41ad5fe3u, 0x40e6fcfdu},
     {0xbae7d3f3u, 0x4234ff4bu, 0x41b50c6bu, 0x40f15931u},
     {0xbaf2512au, 0x423d0339u, 0x41bd0ff3u, 0x40fbfa7au},
     {0xbafcfde1u, 0x424561fau, 0x41c56e54u, 0x41039b52u},
     {0xbb041152u, 0x424e1f94u, 0x41ce2b94u, 0x41097bb5u},
     {0xbb09de72u, 0x4257403bu, 0x41d74be6u, 0x410f827bu},
     {0xbb0ffe63u, 0x4260c851u, 0x41e0d3adu, 0x4115d397u},
     {0xbb165ba6u, 0x426abc6au, 0x41eac77du, 0x411c873eu},
     {0xbb1ce65cu, 0x42752150u, 0x41f52c1eu, 0x41236209u},
     {0xbb24013bu, 0x427ffc00u, 0x42000348u, 0x412aa8aau},
     {0xbb2b27e5u, 0x4285a8d9u, 0x4205ae04u, 0x41324027u},
     {0xbb32ce9cu, 0x428b93ecu, 0x420b98fdu, 0x413a39feu},
     {0xbb3a7ab0u, 0x4291c212u, 0x4211c70au, 0x41424cf5u},
     {0xbb4313b3u, 0x42983642u, 0x42183b25u, 0x414ae231u},
     {0xbb4bacb3u, 0x429ef396u, 0x421ef866u, 0x4153f79au},
     {0xbb54c596u, 0x42a5fd4cu, 0x4226020bu, 0x415d4729u},
     {0xbb5e03e5u, 0x42ad56c5u, 0x422d5b75u, 0x41671314u},
     {0xbb67e2ceu, 0x42b50389u, 0x4235082du, 0x4171681fu},
     {0xbb7259e8u, 0x42bd0749u, 0x423d0be3u, 0x417c0559u},
     {0xbb7cfa65u, 0x42c565deu, 0x42456a70u, 0x41839cd4u},
     {0xbb84011cu, 0x42ce234eu, 0x424e27dau, 0x41897413u},
     {0xbb89f25cu, 0x42d743ccu, 0x42574855u, 0x418f914au},
     {0xbb902422u, 0x42e0cbbbu, 0x4260d043u, 0x4195ed6eu},
     {0xbb963c47u, 0x42eabfb0u, 0x426ac437u, 0x419c75d7u},
     {0xbb9ce682u, 0x42f52472u, 0x427528fcu, 0x41a36437u},
     {0xbba4013bu, 0x42ffff00u, 0x428001c8u, 0x41aaaaabu},
     {0xbbab0402u, 0x4305aa49u, 0x4285ac94u, 0x41b22baau},
     {0xbbb2df30u, 0x430b954du, 0x428b979du, 0x41b9f645u},
     {0xbbba9458u, 0x4311c363u, 0x4291c5b9u, 0x41c25ebcu},
     {0xbbc30770u, 0x43183785u, 0x429839e2u, 0x41cadc38u},
     {0xbbcbc81du, 0x431ef4cbu, 0x429ef731u, 0x41d40a54u},
     {0xbbd4d2f7u, 0x4325fe74u, 0x42a600e3u, 0x41dd510du},
     {0xbbde4a63u, 0x432d57e0u, 0x42ad5a5au, 0x41e74090u},
     {0xbbe7a681u, 0x43350499u, 0x42b5071du, 0x41f143e4u},
     {0xbbf25c17u, 0x433d084du, 0x42bd0adfu, 0x41fc0811u},
     {0xbbfcf986u, 0x434566d7u, 0x42c56977u, 0x42039d34u},
     {0xbc03dd0eu, 0x434e243du, 0x42ce26ebu, 0x42095e2fu},
     {0xbc09c754u, 0x435744b1u, 0x42d74770u, 0x420f7705u},
     {0xbc100d90u, 0x4360cc96u, 0x42e0cf68u, 0x4215dfe9u},
     {0xbc165471u, 0x436ac081u, 0x42eac366u, 0x421c8578u},
     {0xbc1d068cu, 0x4375253au, 0x42f52834u, 0x422378beu},
     {0xbc24013bu, 0x437fffc0u, 0x43000168u, 0x422aab2bu},
     {0xbc2afb09u, 0x4385aaa5u, 0x4305ac38u, 0x4232268au},
     {0xbc32e356u, 0x438b95a5u, 0x430b9745u, 0x4239f951u},
     {0xbc3abac4u, 0x4391c3b7u, 0x4311c565u, 0x42427729u},
     {0xbc436464u, 0x439837d5u, 0x43183992u, 0x424b16abu},
     {0xbc4b6ef3u, 0x439ef519u, 0x431ef6e3u, 0x4253d311u},
     {0xbc54d64fu, 0x43a5febeu, 0x43260099u, 0x425d5386u},
     {0xbc5e3c01u, 0x43ad5827u, 0x432d5a13u, 0x426737f4u},
     {0xbc67976eu, 0x43b504ddu, 0x433506d9u, 0x42713ad6u},
     {0xbc725ca3u, 0x43bd088eu, 0x433d0a9eu, 0x427c08bfu},
     {0xbc7d1950u, 0x43c56715u, 0x43456939u, 0x4283a74au},
     {0xbc84040cu, 0x43ce2478u, 0x434e26b0u, 0x428976afu},
     {0xbc89cc93u, 0x43d744eau, 0x43574737u, 0x428f7a72u},
     {0xbc8ff7ebu, 0x43e0cccdu, 0x4360cf31u, 0x4295d28au},
     {0xbc966a7cu, 0x43eac0b5u, 0x436ac332u, 0x429c935eu},
     {0xbc9d0e8fu, 0x43f5256cu, 0x43752802u, 0x42a37ddfu},
     {0xbca4013bu, 0x43fffff0u, 0x43800150u, 0x42aaab4bu},
     {0xbcaaf8cau, 0x4405aabcu, 0x4385ac21u, 0x42b22542u},
     {0xbcb2e45fu, 0x440b95bbu, 0x438b972fu, 0x42b9fa14u},
     {0xbcbac45fu, 0x4411c3ccu, 0x4391c550u, 0x42c27d44u},
     {0xbcc2fb9bu, 0x441837eau, 0x4398397du, 0x42cad55bu},
     {0xbccb98acu, 0x441ef52cu, 0x439ef6d0u, 0x42d3ed37u},
     {0xbcd49722u, 0x4425fed1u, 0x43a60086u, 0x42dd2c2eu},
     {0xbcde1867u, 0x442d5839u, 0x43ad5a01u, 0x42e721d2u},
     {0xbce793aau, 0x443504eeu, 0x43b506c8u, 0x42f13892u},
     {0xbcf27cc7u, 0x443d089eu, 0x43bd0a8eu, 0x42fc1ce6u},
     {0xbcfce140u, 0x44456725u, 0x43c56929u, 0x430395d4u},
     {0xbd03fdcbu, 0x444e2487u, 0x43ce26a1u, 0x430972d1u},
     {0xbd09dde3u, 0x445744f8u, 0x43d74729u, 0x430f854au},
     {0xbd102284u, 0x4460ccdau, 0x43e0cf24u, 0x4315ed2bu},
     {0xbd162ffcu, 0x446ac0c3u, 0x43eac324u, 0x431c6ee1u},
     {0xbd1cf08eu, 0x44752579u, 0x43f527f5u, 0x43236b2cu},
     {0xbd24013bu, 0x447ffffcu, 0x4400014au, 0x432aab53u},
     {0xbd2ad839u, 0x4485aac2u, 0x4405ac1bu, 0x433210f5u},
     {0xbd32a4a0u, 0x448b95c0u, 0x440b9729u, 0x433a2238u},
     {0xbd3a66c1u, 0x4491c3d2u, 0x4411c54au, 0x434242d9u},
     {0xbd43016au, 0x449837efu, 0x44183978u, 0x434ad902u},
     {0xbd4b8318u, 0x449ef531u, 0x441ef6cbu, 0x4353dfc5u},
     {0xbd54e75bu, 0x44a5fed5u, 0x44260082u, 0x435d5e4au},
     {0xbd5e4f83u, 0x44ad583du, 0x442d59fdu, 0x43674440u},
     {0xbd67b2bau, 0x44b504f2u, 0x443506c4u, 0x43714bfcu},
     {0xbd7284d0u, 0x44bd08a2u, 0x443d0a8au, 0x437c21efu},
     {0xbd7cd33cu, 0x44c56729u, 0x44456925u, 0x43839177u},
     {0xbd83ec3au, 0x44ce248bu, 0x444e269du, 0x438967dcu},
     {0xbd89c236u, 0x44d744fcu, 0x44574725u, 0x438f7405u},
     {0xbd8ffd28u, 0x44e0ccdeu, 0x4460cf20u, 0x4395d5dbu},
     {0xbd96415eu, 0x44eac0c6u, 0x446ac321u, 0x439c79bdu},
     {0xbd9cf90fu, 0x44f5257cu, 0x447527f2u, 0x43a3707du},
     {0xbda3c139u, 0x44ffffffu, 0x44800148u, 0x43aaab53u},
     {0xbdab1018u, 0x4505aac3u, 0x4485ac1au, 0x43b233d8u},
     {0xbdb2d4b3u, 0x450b95c1u, 0x448b9728u, 0x43ba403du},
     {0xbdba8f5du, 0x4511c3d3u, 0x4491c549u, 0x43c25c35u},
     {0xbdc322dfu, 0x451837f0u, 0x44983977u, 0x43caede7u},
     {0xbdcb9db5u, 0x451ef532u, 0x449ef6cau, 0x43d3f064u},
     {0xbdd4fb69u, 0x4525fed6u, 0x44a60081u, 0x43dd6ad1u},
     {0xbddddd45u, 0x452d583fu, 0x44ad59fbu, 0x43e6fceeu},
     {0xbde7ba7eu, 0x453504f3u, 0x44b506c3u, 0x43f150d7u},
     {0xbdf286d2u, 0x453d08a3u, 0x44bd0a89u, 0x43fc2332u},
     {0xbdfccfbbu, 0x4545672au, 0x44c56924u, 0x4403905fu},
     {0xbe03e7d6u, 0x454e248cu, 0x44ce269cu, 0x4409651fu},
     {0xbe09bb4bu, 0x455744fdu, 0x44d74724u, 0x440f6fb4u},
     {0xbe0ff3d1u, 0x4560ccdfu, 0x44e0cf1fu, 0x4415d007u},
     {0xbe1635b5u, 0x456ac0c7u, 0x44eac320u, 0x441c7277u},
     {0xbe1ceb2eu, 0x4575257du, 0x44f527f1u, 0x442367d4u},
     {0xbe23f13bu, 0x45800000u, 0x45000148u, 0x442aa157u},
     {0xbe2b3e11u, 0x4585aac3u, 0x4505ac1au, 0x4432508cu},
     {0xbe3280b3u, 0x458b95c2u, 0x450b9727u, 0x443a0bcdu},
     {0xbe3ab985u, 0x4591c3d3u, 0x4511c549u, 0x44427687u},
     {0xbe434b3eu, 0x459837f0u, 0x45183977u, 0x444b071cu},
     {0xbe4bc45eu, 0x459ef532u, 0x451ef6cau, 0x44540887u},
     {0xbe54a068u, 0x45a5fed7u, 0x45260080u, 0x445d3201u},
     {0xbe5e00b8u, 0x45ad583fu, 0x452d59fbu, 0x44671311u},
     {0xbe67dc70u, 0x45b504f3u, 0x453506c3u, 0x44716609u},
     {0xbe72274fu, 0x45bd08a4u, 0x453d0a88u, 0x447be791u},
     {0xbe7ceedcu, 0x45c5672au, 0x45456924u, 0x44839a17u},
     {0xbe83f6beu, 0x45ce248cu, 0x454e269cu, 0x44896e6du},
     {0xbe89c990u, 0x45d744fdu, 0x45574724u, 0x448f789du},
     {0xbe90017cu, 0x45e0ccdfu, 0x4560cf1fu, 0x4495d88fu},
     {0xbe9642ccu, 0x45eac0c7u, 0x456ac320u, 0x449c7aa2u},
     {0xbe9cf7b7u, 0x45f5257du, 0x457527f1u, 0x44a36fa7u},
     {0xbea3fd3bu, 0x46000000u, 0x45800148u, 0x44aaa8d6u},
     {0xbeab498fu, 0x4605aac3u, 0x4585ac1au, 0x44b257b9u},
     {0xbeb28bb5u, 0x460b95c2u, 0x458b9727u, 0x44ba12acu},
     {0xbebac40fu, 0x4611c3d3u, 0x4591c549u, 0x44c27d1bu},
     {0xbec35556u, 0x461837f0u, 0x45983977u, 0x44cb0d69u},
     {0xbecbce08u, 0x461ef532u, 0x459ef6cau, 0x44d40e8fu},
     {0xbed4a9aau, 0x4625fed7u, 0x45a60080u, 0x44dd37c8u},
     {0xbede0995u, 0x462d583fu, 0x45ad59fbu, 0x44e71899u},
     {0xbee7e4edu, 0x463504f3u, 0x45b506c3u, 0x44f16b55u},
     {0xbef22f6fu, 0x463d08a4u, 0x45bd0a88u, 0x44fbeca4u},
     {0xbefcf6a4u, 0x4645672au, 0x45c56924u, 0x45039c85u},
     {0xbf03fa77u, 0x464e248cu, 0x45ce269cu, 0x450970c1u},
     {0xbf09cd22u, 0x465744fdu, 0x45d74724u, 0x450f7ad8u},
     {0xbf1004e7u, 0x4660ccdfu, 0x45e0cf1fu, 0x4515dab1u},
     {0xbf164612u, 0x466ac0c7u, 0x45eac320u, 0x451c7cadu},
     {0xbf1cfad9u, 0x4675257du, 0x45f527f1u, 0x4523719cu},
     {0xbf24003bu, 0x46800000u, 0x46000148u, 0x452aaab5u},
     {0xbf2b4c6fu, 0x4685aac3u, 0x4605ac1au, 0x45325985u},
     {0xbf328e75u, 0x468b95c2u, 0x460b9727u, 0x453a1464u},
     {0xbf3ac6b1u, 0x4691c3d3u, 0x4611c549u, 0x45427ec0u},
     {0xbf4357dcu, 0x469837f0u, 0x46183977u, 0x454b0efcu},
     {0xbf4bd072u, 0x469ef532u, 0x461ef6cau, 0x45541012u},
     {0xbf54abfau, 0x46a5fed7u, 0x46260080u, 0x455d393au},
     {0xbf5e0bccu, 0x46ad583fu, 0x462d59fbu, 0x456719fbu},
     {0xbf67e70cu, 0x46b504f3u, 0x463506c3u, 0x45716ca8u},
     {0xbf723177u, 0x46bd08a4u, 0x463d0a88u, 0x457bede8u},
     {0xbf7cf896u, 0x46c5672au, 0x46456924u, 0x45839d20u},
     {0xbf83fb66u, 0x46ce248cu, 0x464e269cu, 0x45897156u},
     {0xbf89ce06u, 0x46d744fdu, 0x46574724u, 0x458f7b66u},
     {0xbf9005c1u, 0x46e0ccdfu, 0x4660cf1fu, 0x4595db3au},
     {0xbf9646e3u, 0x46eac0c7u, 0x466ac320u, 0x459c7d30u},
     {0xbf9cfba1u, 0x46f5257du, 0x467527f1u, 0x45a37219u}}}; /*_VAPI_DATA_TYPE*/
static __constant union {
  unsigned int w;
  float f;
  int i;
} __scosh_ha_Shifter = {0x4ac000feu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __scosh_ha_L2E = {0x3FB8AA3Bu};
// log(2) high, low
static __constant union {
  unsigned int w;
  float f;
  int i;
} __scosh_ha_L2H = {0x3f317218u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __scosh_ha_L2L = {0xb102E308u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __scosh_ha_c5 = {0x3c08ba8bu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __scosh_ha_c4 = {0x3d2aec4eu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __scosh_ha_c3 = {0x3e2aaa9cu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __scosh_ha_c2 = {0x3effffe8u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __scosh_ha_c1 = {0x3f800000u};
__attribute__((always_inline)) inline int
__ocl_svml_internal_scosh_ha(float *a, float *r) {
  int nRet = 0;
  float x = __spirv_ocl_fabs(*a);
  union {
    unsigned int w;
    float f;
    int i;
  } S, Th, Tlr, Th2, xin, xa, res;
  float N, R, poly;
  int index_mask;
  S.f = __spirv_ocl_fma(x, __scosh_ha_L2E.f,
                                               __scosh_ha_Shifter.f);
  N = S.f - __scosh_ha_Shifter.f;
  R = __spirv_ocl_fma((-N), __scosh_ha_L2H.f, x);
  R = __spirv_ocl_fma((-N), __scosh_ha_L2L.f, R);
  // set exponent in place
  Th.w = S.w << 22;
  // index_mask is based on last bit of S.w
  index_mask = 0 - (S.w & 1);
  // set Th mantissa
  Th.w ^= (index_mask & 0x7504F3u);
  // set Tl/Th value
  Tlr.w = index_mask & 0x329302AEu;
  // polynomial
  poly = __spirv_ocl_fma(R, __scosh_ha_c5.f,
                                                __scosh_ha_c4.f);
  poly = __spirv_ocl_fma(R, poly, __scosh_ha_c3.f);
  poly = __spirv_ocl_fma(R, poly, __scosh_ha_c2.f);
  poly = __spirv_ocl_fma(R, poly, __scosh_ha_c1.f);
  poly = __spirv_ocl_fma(R, poly, Tlr.f);
  xin.f = x;
  xa.w = xin.w & 0x7fffffffu;
  // redirect special cases
  if (xa.w > 0x42AEAC4Fu)
    goto COSHF_SPECIAL;
  res.f = __spirv_ocl_fma(poly, Th.f, Th.f);
  *r = res.f;
  return nRet;
COSHF_SPECIAL:
  if (xa.w > 0x42b2d4fcu) {
    if (xa.w > 0x7f800000u) { // NaN?
      *r = x + x;
      return nRet;
    }
    // overflow
    res.w = 0x7f800000;
    *r = res.f;
    nRet = 3;
    return nRet;
  }
  S.w += 0xfe;
  Th2.w = (S.w >> 2) & 0xff;
  S.w -= (Th2.w << 1);
  Th2.w <<= 23; // second exponent scale
  Th.w = S.w << 22;
  // set Th mantissa
  Th.w ^= (index_mask & 0x7504F3u);
  res.f = 0.5f * __spirv_ocl_fma(poly, Th.f, Th.f);
  res.f *= Th2.f;
  *r = res.f;
  return nRet;
}
float __ocl_svml_coshf_ha(float x) {
  float r;
  unsigned int vm;
  float va1;
  float vr1;
  va1 = x;
  {
    /*
    //  Design:
    // sh(x) = sh(ln2*(x/ln2))=sh(ln2*(N+Jn/2^K) + r) =
    //         = 2^N*(a0(Jn)+a1(Jn)*r+a2(Jn)*r^2+a3(Jn)*r^4);
    // where:
    // N==0 OR >=16;Jn in[0,2^(4+K)-1]-composite index [xxxx(4 bits of
    N)xxx...x(K bits of j);
    // R<2^(-K-1), r= R*ln2;
    // a&(Jn) - loaded from the table
    // Special tricks to reach HA accuracy :
    // we keep in a0 the difference SP(D(a0)-2*a2), where D(a0) - DP coeff, so
    2*a2 is added in the end
    //
    */
    float sN;
    float sM;
    float sR;
    float sOut;
    unsigned int iM;
    unsigned int iMScale;
    float sXSign;
    float sPC[4];
    float sAbsX;
    unsigned int iRangeMask;
    unsigned int iIndex; // Jn
    unsigned int iX;
    float sInvLn2;
    float sShifter;
    float sLn2[2];
    unsigned int iIndexMask;
    unsigned int iDomainRange;
    sXSign = as_float(__ocl_svml_internal_scosh_ha_data._sSign);
    sShifter = as_float(__ocl_svml_internal_scosh_ha_data._sShifter);
    iDomainRange = (__ocl_svml_internal_scosh_ha_data._iDomainRange);
    iIndexMask = (__ocl_svml_internal_scosh_ha_data._iIndexMask);
    sLn2[0] = as_float(__ocl_svml_internal_scosh_ha_data._sLn2hi);
    sLn2[1] = as_float(__ocl_svml_internal_scosh_ha_data._sLn2lo);
    sInvLn2 = as_float(__ocl_svml_internal_scosh_ha_data._sInvLn2);
    /* ............... Abs argument ............................ */
    sAbsX = as_float((~(as_uint(sXSign)) & as_uint(va1)));
    /* ............... Scale & Index............................ */
    sM = __spirv_ocl_fma(
        sAbsX, sInvLn2, sShifter); // sM = x*2^K/log(2) + RShifter
    /* ...............Check for overflow\underflow ............. */
    iX = as_uint(sAbsX); // iX = bitimage(abs(x))
    iRangeMask = ((unsigned int)(-(signed int)((signed int)iX >
                                               (signed int)iDomainRange)));
    vm = 0;
    vm = iRangeMask;
    /* .............. Index and Scale ......................... */
    sN = as_float((as_uint(sM) ^ as_uint(sShifter)));
    iM = as_uint(sN);
    // trick2 : if sN>255, iIndex=iIndex|11110000b,so iIndex will be >=15
    iIndex = (iIndexMask - iM); // iIndex=255-iM
    iIndex = ((unsigned int)(iIndex) >>
              ((32 - 4)));                    // iIndex=(255-iM)>>28= 0 OR 1111b
    iIndex = ((unsigned int)(iIndex) << (4)); // iIndex= 0 OR 11110000b
    iIndex = (iIndex | iM);         // iIndex=iM OR Mscale|new adjusted index
    iIndex = (iIndex & iIndexMask); // iIndex=adjusted index
    iMScale = (iM - iIndex);        // iMScale = iM-adjustedindex
    iMScale = ((unsigned int)(iMScale)
               << ((23 - 4))); // iMscale is in SPFP EXP bits now
    /* ................... R ................................... */
    sN = (sM - sShifter); // sN = sM - RShifter
    sR = __spirv_ocl_fma(
        -(sLn2[0]), sN, sAbsX); // sR = sX - sN*sLog2_hi/2^K
    sR = __spirv_ocl_fma(
        -(sLn2[1]), sN, sR); // sR = (sX - sN*Log2_hi/2^K)-sN*Log2_lo/2^K
    /* poly(r) = a0+a1*r+...a3*r^4...................... */
    sPC[2] = as_float(
        ((unsigned int *)(__ocl_svml_internal_scosh_ha_data
                              ._dbT))[(((0 + iIndex) * (4 * 4)) >> (2)) + 2]);
    sPC[3] = as_float(
        ((unsigned int *)(__ocl_svml_internal_scosh_ha_data
                              ._dbT))[(((0 + iIndex) * (4 * 4)) >> (2)) + 3]);
    sN = (sPC[2] + sPC[2]); // sN=2*a2
    sOut = __spirv_ocl_fma(sPC[3], sR,
                                                  sPC[2]); // sOut = (a2 +a3*sR)
    sPC[1] = as_float(
        ((unsigned int *)(__ocl_svml_internal_scosh_ha_data
                              ._dbT))[(((0 + iIndex) * (4 * 4)) >> (2)) + 1]);
    sOut = __spirv_ocl_fma(
        sOut, sR, sPC[1]); // sOut = a1+sR*(a2 +a3*sR)
    sPC[0] = as_float(
        ((unsigned int *)(__ocl_svml_internal_scosh_ha_data
                              ._dbT))[(((0 + iIndex) * (4 * 4)) >> (2)) + 0]);
    sOut = __spirv_ocl_fma(
        sOut, sR, sPC[0]); // sOut = a0+sR*(a1+sR*(a2 +a3*sR))
    sOut = (sOut + sN);    // sOut = (2*a2?sAbsX)+a0+sR*(a1+sR*(a2 +a3*sR))
    /*...scale up..........................*/
    iM = as_uint(sOut);
    iM = (iM + iMScale);
    /* ................... Ret H ...................... */
    vr1 = as_float(iM);
  }
  if (__builtin_expect((vm) != 0, 0)) {
    float __cout_a1;
    float __cout_r1;
    ((float *)&__cout_a1)[0] = va1;
    ((float *)&__cout_r1)[0] = vr1;
    __ocl_svml_internal_scosh_ha(&__cout_a1, &__cout_r1);
    vr1 = ((float *)&__cout_r1)[0];
  }
  r = vr1;
  return r;
}

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
  /*Shared*/
  unsigned int _sExp_tbl_PH[32];
  unsigned int _sExp_tbl_PL[32];
  unsigned int _sExp_tbl_NH[32];
  unsigned int _sExp_tbl_NL[32];
  unsigned int _sShifter_UISA;
  unsigned int _sShifterP1_UISA;
  unsigned int _iIndexMask_UISA;
  unsigned int _iDomainRange_UISA;
  unsigned int _sPC1_UISA;
  unsigned int _sPC2_UISA;
  unsigned int _sPC3_UISA;
  unsigned int _sInvLn2;
  unsigned int _sLn2hi;
  unsigned int _sLn2lo;
  unsigned int _sSign;
  unsigned int _sOne;
  /* Shared name but with different value */
  unsigned int _sShifter;
  unsigned int _iIndexMask;
  unsigned int _iDomainRange;
  unsigned int _dbT[256][4];  // precalc poly(4) coeff
  unsigned int _iIndexTrshld; // 63/32*log(2) = 3faeac50;
} __ocl_svml_internal_ssinh_ha_data_t;
static __ocl_svml_internal_ssinh_ha_data_t __ocl_svml_internal_ssinh_ha_data = {
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
    {/* 2^(-i/32-1), i=0..31 */
     0x3f000000u, 0x3efa83b3u, 0x3ef5257du, 0x3eefe4bau, 0x3eeac0c7u,
     0x3ee5b907u, 0x3ee0ccdfu, 0x3edbfbb8u, 0x3ed744fdu, 0x3ed2a81eu,
     0x3ece248cu, 0x3ec9b9beu, 0x3ec5672au, 0x3ec12c4du, 0x3ebd08a4u,
     0x3eb8fbafu, 0x3eb504f3u, 0x3eb123f6u, 0x3ead583fu, 0x3ea9a15bu,
     0x3ea5fed7u, 0x3ea27043u, 0x3e9ef532u, 0x3e9b8d3au, 0x3e9837f0u,
     0x3e94f4f0u, 0x3e91c3d3u, 0x3e8ea43au, 0x3e8b95c2u, 0x3e88980fu,
     0x3e85aac3u, 0x3e82cd87u},
    {/* for i in [|0,...,31|] do printsingle( 2^(-i/32-1) -
        round(2^(-i/32-1),SG,RN)); */
     0x00000000u, 0xb1923758u, 0x31292436u, 0xb248464au, 0xb14116deu,
     0xb1441be6u, 0xb11eab59u, 0xb2504a1cu, 0xb1d4a58au, 0xb25c1daau,
     0x3128fc24u, 0xb27323a2u, 0x310aa837u, 0xb1d6663eu, 0xb2414fe8u,
     0x320ec5f7u, 0x31cfe77au, 0xb27c5aa8u, 0xb12deaf6u, 0xb2162b08u,
     0xb22c9d5eu, 0x2fc3125au, 0x32412342u, 0xb20c5563u, 0x32231b71u,
     0xb22e0212u, 0x32675624u, 0xb2697465u, 0xb160aba1u, 0xb27eda4bu,
     0x324f9891u, 0xb24ea7a9u},
    0x48c00000u, /* 1.5*2^18 _sShifter_UISA */
    0x48c00001u, /* 1.5*2^18 _sShifterP1_UISA */
    0x0000001fu, /* _iIndexMask_UISA */
    0x42aeac4fu, /* _iDomainRange_UISA */
    0x3F800000u, /* _sPC1_UISA */
    0x3f000044u, /* _sPC2_UISA */
    0x3e2aaab5u, /* _sPC3_UISA */
    0x3FB8AA3Bu,
    /* _sInvLn2  */ // k=0
    0x3F317000u,    /* _sLn2hi   */
    0x3805FDF4u,    /* _sLn2lo   */
    0x80000000u,    /* _sSign    */
    0x3f800000u,    /* _sOne     */
    0x49400000u,    /* _sShifter */
    255,            /* _iIndexMask = (1<<(K+4))-1 */
    0x42B16700u,    /* _iDomainRange =(128*2^K-0.5)*log(2)/2^K*/
    {
        //_dbT
        {0x00000000u, 0x2900542eu, 0xadc1203eu,
         0x3e2aab2eu}, // log2(rel_err)= 30
        {0x37635db6u, 0x3a760862u, 0x3cb181aau,
         0x3e2acd5au}, // log2(rel_err)= 28
        {0x38e36e16u, 0x3b762581u, 0x3d31ac9fu,
         0x3e2b4860u}, // log2(rel_err)= 28
        {0x39bffbf4u, 0x3c0a90c4u, 0x3d8576e1u,
         0x3e2c1654u}, // log2(rel_err)= 29
        {0x3a63afb5u, 0x3c769bb7u, 0x3db25795u,
         0x3e2d36b6u}, // log2(rel_err)= 29
        {0x3ade89afu, 0x3cc0ef12u, 0x3ddf8decu,
         0x3e2eaa14u}, // log2(rel_err)= 29
        {0x3b4078a3u, 0x3d0b26beu, 0x3e0697d2u,
         0x3e306fdeu}, // log2(rel_err)= 29
        {0x3b99014eu, 0x3d3dc90du, 0x3e1da98bu,
         0x3e32900cu}, // log2(rel_err)= 29
        {0x3be4b6ceu, 0x3d7876cfu, 0x3e3506d2u,
         0x3e34fff8u}, // log2(rel_err)= 29
        {0x3c2315abu, 0x3d9da619u, 0x3e4cbb3au,
         0x3e37cc7au}, // log2(rel_err)= 29
        {0x3c601bf0u, 0x3dc334adu, 0x3e64d1adu,
         0x3e3ae61du}, // log2(rel_err)= 29
        {0x3c95704du, 0x3decf92bu, 0x3e7d5670u,
         0x3e3e6630u}, // log2(rel_err)= 29
        {0x3cc26e55u, 0x3e0d83d5u, 0x3e8b2a32u,
         0x3e42346au}, // log2(rel_err)= 29
        {0x3cf7c837u, 0x3e26bb24u, 0x3e97ec4fu,
         0x3e466653u}, // log2(rel_err)= 29
        {0x3d1b2110u, 0x3e422ea1u, 0x3ea4f705u,
         0x3e4af6f2u}, // log2(rel_err)= 28
        {0x3d3f524eu, 0x3e5feb7bu, 0x3eb2525eu,
         0x3e4fff44u}, // log2(rel_err)= 28
        {0x3d68de80u, 0x3e800000u, 0x3ec00250u,
         0x3e55566fu}, // log2(rel_err)= 28
        {0x3d8c1770u, 0x3e913dcbu, 0x3ece0f86u,
         0x3e5b2e97u}, // log2(rel_err)= 27
        {0x3da6d7b9u, 0x3ea3b76cu, 0x3edc7c62u,
         0x3e6127c8u}, // log2(rel_err)= 28
        {0x3dc4e796u, 0x3eb775bfu, 0x3eeb5587u,
         0x3e67ad96u}, // log2(rel_err)= 28
        {0x3de6801cu, 0x3ecc8241u, 0x3efaa041u,
         0x3e6eba88u}, // log2(rel_err)= 28
        {0x3e05ee09u, 0x3ee2e710u, 0x3f053206u,
         0x3e76449bu}, // log2(rel_err)= 29
        {0x3e1a9c07u, 0x3efaaeefu, 0x3f0d5443u,
         0x3e7e4b0cu}, // log2(rel_err)= 27
        {0x3e31694au, 0x3f09f2a7u, 0x3f15b816u,
         0x3e834b0au}, // log2(rel_err)= 28
        {0x3e4a7617u, 0x3f174b24u, 0x3f1e6494u,
         0x3e87b25bu}, // log2(rel_err)= 27
        {0x3e65e3c8u, 0x3f256757u, 0x3f275fbdu,
         0x3e8c781fu}, // log2(rel_err)= 28
        {0x3e81ea6fu, 0x3f344e0au, 0x3f30a906u,
         0x3e916b9cu}, // log2(rel_err)= 29
        {0x3e923686u, 0x3f440664u, 0x3f3a48d6u,
         0x3e96bd92u}, // log2(rel_err)= 27
        {0x3ea3e8a7u, 0x3f5497f6u, 0x3f44404fu,
         0x3e9c2cafu}, // log2(rel_err)= 28
        {0x3eb713fbu, 0x3f660ab3u, 0x3f4e968fu,
         0x3ea1fe30u}, // log2(rel_err)= 29
        {0x3ecbcc61u, 0x3f7866ffu, 0x3f594ebfu,
         0x3ea812ccu}, // log2(rel_err)= 27
        {0x3ee22676u, 0x3f85dad6u, 0x3f6470abu,
         0x3eae7cc9u}, // log2(rel_err)= 27
        {0xb899c128u, 0x40080000u, 0x3f700267u,
         0x3eb5560bu}, // log2(rel_err)= 29
        {0xb8a181a6u, 0x400d53efu, 0x3f7c05b5u,
         0x3ebc89c4u}, // log2(rel_err)= 29
        {0xb8a9aaf2u, 0x4012ebc8u, 0x3f84410fu,
         0x3ec3ee4bu}, // log2(rel_err)= 29
        {0xb8b1c332u, 0x4018ca3au, 0x3f8abed0u,
         0x3ecbda84u}, // log2(rel_err)= 29
        {0xb8ba4bcau, 0x401ef218u, 0x3f917f3du,
         0x3ed3ffbbu}, // log2(rel_err)= 29
        {0xb8c38184u, 0x40256657u, 0x3f988595u,
         0x3edc75b9u}, // log2(rel_err)= 29
        {0xb8ccd4f7u, 0x402c2a10u, 0x3f9fd537u,
         0x3ee58c84u}, // log2(rel_err)= 29
        {0xb8d69af9u, 0x40334084u, 0x3fa771a7u,
         0x3eef03e2u}, // log2(rel_err)= 29
        {0xb8e0b483u, 0x403aad1bu, 0x3faf5e8du,
         0x3ef8d80cu}, // log2(rel_err)= 29
        {0xb8eb2dc0u, 0x40427366u, 0x3fb79fb8u,
         0x3f019254u}, // log2(rel_err)= 29
        {0xb8f653d6u, 0x404a9721u, 0x3fc03920u,
         0x3f0706feu}, // log2(rel_err)= 29
        {0xb900e071u, 0x40531c36u, 0x3fc92ee6u,
         0x3f0cb04fu}, // log2(rel_err)= 29
        {0xb906eef1u, 0x405c06bcu, 0x3fd28559u,
         0x3f12bbcfu}, // log2(rel_err)= 29
        {0xb90ceceau, 0x40655afeu, 0x3fdc40f4u,
         0x3f18d528u}, // log2(rel_err)= 29
        {0xb9138aecu, 0x406f1d75u, 0x3fe66667u,
         0x3f1f682fu}, // log2(rel_err)= 29
        {0xb91a82b0u, 0x407952d3u, 0x3ff0fa91u,
         0x3f263f6cu}, // log2(rel_err)= 29
        {0xb9218137u, 0x40820000u, 0x3ffc0286u,
         0x3f2d5603u}, // log2(rel_err)= 29
        {0xb928ca74u, 0x4087950eu, 0x4003c1cau,
         0x3f34e607u}, // log2(rel_err)= 29
        {0xb930d649u, 0x408d6b43u, 0x4009c1a2u,
         0x3f3cb344u}, // log2(rel_err)= 29
        {0xb938a67bu, 0x4093856du, 0x401003abu,
         0x3f44c221u}, // log2(rel_err)= 29
        {0xb940d577u, 0x4099e67au, 0x40168ae8u,
         0x3f4d4b8fu}, // log2(rel_err)= 29
        {0xb949dd53u, 0x40a0917bu, 0x401d5a7du,
         0x3f563dd7u}, // log2(rel_err)= 29
        {0xb952d6cfu, 0x40a789a5u, 0x402475aeu,
         0x3f5f6264u}, // log2(rel_err)= 29
        {0xb95c3025u, 0x40aed250u, 0x402bdfe6u,
         0x3f6928c8u}, // log2(rel_err)= 29
        {0xb9665af3u, 0x40b66efdu, 0x40339cb6u,
         0x3f7347d8u}, // log2(rel_err)= 29
        {0xb970710cu, 0x40be6354u, 0x403bafd4u,
         0x3f7de3deu}, // log2(rel_err)= 29
        {0xb97b4fe2u, 0x40c6b328u, 0x40441d23u,
         0x3f846dc2u}, // log2(rel_err)= 29
        {0xb98314e3u, 0x40cf6277u, 0x404ce8aeu,
         0x3f8a2d3eu}, // log2(rel_err)= 29
        {0xb989067au, 0x40d8756du, 0x405616b1u,
         0x3f9041a7u}, // log2(rel_err)= 29
        {0xb98f2fc1u, 0x40e1f067u, 0x405fab94u,
         0x3f968fdau}, // log2(rel_err)= 29
        {0xb995e81cu, 0x40ebd7f2u, 0x4069abf3u,
         0x3f9d4c0du}, // log2(rel_err)= 29
        {0xb99c5d97u, 0x40f630d3u, 0x40741c99u,
         0x3fa41193u}, // log2(rel_err)= 29
        {0xb9a34139u, 0x41008000u, 0x407f028du,
         0x3fab5600u}, // log2(rel_err)= 29
        {0xb9aaaca8u, 0x41062556u, 0x40853186u,
         0x3fb2e91eu}, // log2(rel_err)= 29
        {0xb9b2411cu, 0x410c0b22u, 0x408b21c6u,
         0x3fbad085u}, // log2(rel_err)= 29
        {0xb9b9ff4au, 0x4112343au, 0x409154e1u,
         0x3fc2e80bu}, // log2(rel_err)= 29
        {0xb9c297e2u, 0x4118a393u, 0x4097cdd3u,
         0x3fcb7690u}, // log2(rel_err)= 29
        {0xb9caf441u, 0x411f5c45u, 0x409e8fb6u,
         0x3fd45ff2u}, // log2(rel_err)= 29
        {0xb9d47746u, 0x4126618au, 0x40a59dccu,
         0x3fddebd7u}, // log2(rel_err)= 29
        {0xb9ddb572u, 0x412db6c3u, 0x40acfb76u,
         0x3fe7b202u}, // log2(rel_err)= 29
        {0xb9e7248au, 0x41355f76u, 0x40b4ac3fu,
         0x3ff1bbd2u}, // log2(rel_err)= 29
        {0xb9f1c1dfu, 0x413d5f50u, 0x40bcb3dbu,
         0x3ffc6bb7u}, // log2(rel_err)= 29
        {0xb9fc2ee2u, 0x4145ba2au, 0x40c51623u,
         0x4003bd74u}, // log2(rel_err)= 29
        {0xba03e202u, 0x414e7407u, 0x40cdd721u,
         0x40099679u}, // log2(rel_err)= 29
        {0xba098c5cu, 0x41579119u, 0x40d6fb07u,
         0x400fad1au}, // log2(rel_err)= 29
        {0xba0fc078u, 0x416115c1u, 0x40e0863cu,
         0x40160883u}, // log2(rel_err)= 29
        {0xba163f66u, 0x416b0692u, 0x40ea7d55u,
         0x401ca70bu}, // log2(rel_err)= 29
        {0xba1cd451u, 0x41756853u, 0x40f4e51bu,
         0x4023861du}, // log2(rel_err)= 29
        {0xba23c13au, 0x41802000u, 0x40ffc28fu,
         0x402ad5ffu}, // log2(rel_err)= 29
        {0xba2b2534u, 0x4185c968u, 0x41058d75u,
         0x403269e3u}, // log2(rel_err)= 29
        {0xba329bd0u, 0x418bb31au, 0x410b79cfu,
         0x403a43dbu}, // log2(rel_err)= 29
        {0xba3a9580u, 0x4191dfedu, 0x4111a92fu,
         0x40428582u}, // log2(rel_err)= 29
        {0xba42a87au, 0x419852d9u, 0x41181e8du,
         0x404b1548u}, // log2(rel_err)= 29
        {0xba4b9a00u, 0x419f0ef7u, 0x411edd05u,
         0x40541070u}, // log2(rel_err)= 29
        {0xba541f5du, 0x41a61784u, 0x4125e7d2u,
         0x405d5240u}, // log2(rel_err)= 29
        {0xba5e16c4u, 0x41ad6fe0u, 0x412d425au,
         0x40674056u}, // log2(rel_err)= 29
        {0xba67b6f3u, 0x41b51b94u, 0x4134f022u,
         0x40716ccdu}, // log2(rel_err)= 29
        {0xba71b610u, 0x41bd1e4fu, 0x413cf4dcu,
         0x407c0dabu}, // log2(rel_err)= 29
        {0xba7ce6a6u, 0x41c57beau, 0x41455464u,
         0x4083a55du}, // log2(rel_err)= 29
        {0xba83e548u, 0x41ce386bu, 0x414e12bdu,
         0x408970c6u}, // log2(rel_err)= 29
        {0xba89cdd5u, 0x41d75804u, 0x4157341du,
         0x408f87f7u}, // log2(rel_err)= 29
        {0xba9024a7u, 0x41e0df17u, 0x4160bce7u,
         0x4095faaau}, // log2(rel_err)= 29
        {0xba963537u, 0x41ead23au, 0x416ab1adu,
         0x409c7dcau}, // log2(rel_err)= 29
        {0xba9d1201u, 0x41f53632u, 0x4175173cu,
         0x40a38b35u}, // log2(rel_err)= 29
        {0xbaa4013bu, 0x42000800u, 0x417ff290u,
         0x40aab600u}, // log2(rel_err)= 29
        {0xbaaae354u, 0x4205b26du, 0x4185a470u,
         0x40b2221du}, // log2(rel_err)= 29
        {0xbab2927cu, 0x420b9d18u, 0x418b8fd1u,
         0x40ba20afu}, // log2(rel_err)= 29
        {0xbaba7b0bu, 0x4211cadau, 0x4191be42u,
         0x40c258e4u}, // log2(rel_err)= 29
        {0xbac36ca7u, 0x42183eaau, 0x419832bdu,
         0x40cb24f0u}, // log2(rel_err)= 29
        {0xbacb636cu, 0x421efba4u, 0x419ef058u,
         0x40d3d498u}, // log2(rel_err)= 29
        {0xbad4c969u, 0x42260502u, 0x41a5fa55u,
         0x40dd53d4u}, // log2(rel_err)= 29
        {0xbade2f19u, 0x422d5e27u, 0x41ad5413u,
         0x40e737e6u}, // log2(rel_err)= 29
        {0xbae7fb8fu, 0x42350a9bu, 0x41b5011bu,
         0x40f18102u}, // log2(rel_err)= 29
        {0xbaf29325u, 0x423d0e0eu, 0x41bd051eu,
         0x40fc321du}, // log2(rel_err)= 29
        {0xbafcf497u, 0x42456c5au, 0x41c563f4u,
         0x41039f56u}, // log2(rel_err)= 29
        {0xbb03e619u, 0x424e2984u, 0x41ce21a4u,
         0x4109675au}, // log2(rel_err)= 29
        {0xbb09be32u, 0x425749bfu, 0x41d74262u,
         0x410f74b0u}, // log2(rel_err)= 29
        {0xbb100db1u, 0x4260d16du, 0x41e0ca91u,
         0x4115e338u}, // log2(rel_err)= 29
        {0xbb1632acu, 0x426ac524u, 0x41eabec3u,
         0x411c737au}, // log2(rel_err)= 29
        {0xbb1d116cu, 0x427529aau, 0x41f523c4u,
         0x4123827du}, // log2(rel_err)= 29
        {0xbb24013bu, 0x42800200u, 0x41fffe90u,
         0x412aae00u}, // log2(rel_err)= 29
        {0xbb2af2ddu, 0x4285acaeu, 0x4205aa2fu,
         0x41322427u}, // log2(rel_err)= 29
        {0xbb32d02au, 0x428b9797u, 0x420b9552u,
         0x413a3fdbu}, // log2(rel_err)= 29
        {0xbb3a946fu, 0x4291c595u, 0x4211c387u,
         0x414261b7u}, // log2(rel_err)= 29
        {0xbb431dadu, 0x4298399fu, 0x421837c8u,
         0x414aece7u}, // log2(rel_err)= 29
        {0xbb4b75c9u, 0x429ef6cfu, 0x421ef52du,
         0x4153d99du}, // log2(rel_err)= 29
        {0xbb5513eeu, 0x42a60061u, 0x4225fef6u,
         0x415d7c2eu}, // log2(rel_err)= 29
        {0xbb5e152du, 0x42ad59b9u, 0x422d5881u,
         0x416721ceu}, // log2(rel_err)= 29
        {0xbb67ecb4u, 0x42b5065du, 0x42350559u,
         0x41717213u}, // log2(rel_err)= 29
        {0xbb728a68u, 0x42bd09feu, 0x423d092eu,
         0x417c273du}, // log2(rel_err)= 29
        {0xbb7cf813u, 0x42c56876u, 0x424567d8u,
         0x41839dd5u}, // log2(rel_err)= 29
        {0xbb83f64eu, 0x42ce25cau, 0x424e255eu,
         0x41896efcu}, // log2(rel_err)= 29
        {0xbb89ea4cu, 0x42d7462du, 0x425745f4u,
         0x418f8dd8u}, // log2(rel_err)= 29
        {0xbb9027f5u, 0x42e0ce02u, 0x4260cdfcu,
         0x4195f157u}, // log2(rel_err)= 29
        {0xbb96520au, 0x42eac1deu, 0x426ac209u,
         0x419c84e1u}, // log2(rel_err)= 29
        {0xbb9d1147u, 0x42f52688u, 0x427526e6u,
         0x41a3804fu}, // log2(rel_err)= 29
        {0xbba4013bu, 0x43000080u, 0x428000c8u,
         0x41aaac00u}, // log2(rel_err)= 29
        {0xbbab16c1u, 0x4305ab3eu, 0x4285ab9fu,
         0x41b238a5u}, // log2(rel_err)= 29
        {0xbbb2bf94u, 0x430b9637u, 0x428b96b2u,
         0x41ba33abu}, // log2(rel_err)= 29
        {0xbbba7ac7u, 0x4311c444u, 0x4291c4d8u,
         0x41c24ff1u}, // log2(rel_err)= 29
        {0xbbc329f0u, 0x4318385cu, 0x4298390bu,
         0x41caf2e1u}, // log2(rel_err)= 29
        {0xbbcbda64u, 0x431ef599u, 0x429ef663u,
         0x41d416d0u}, // log2(rel_err)= 29
        {0xbbd5068eu, 0x4325ff39u, 0x42a6001eu,
         0x41dd724au}, // log2(rel_err)= 29
        {0xbbde4eb5u, 0x432d589du, 0x42ad599du,
         0x41e7443fu}, // log2(rel_err)= 29
        {0xbbe7a8fbu, 0x4335054eu, 0x42b50668u,
         0x41f14661u}, // log2(rel_err)= 29
        {0xbbf28838u, 0x433d08fau, 0x42bd0a32u,
         0x41fc2485u}, // log2(rel_err)= 29
        {0xbbfcf8f2u, 0x4345677du, 0x42c568d1u,
         0x42039d74u}, // log2(rel_err)= 29
        {0xbc03da5au, 0x434e24dcu, 0x42ce264cu,
         0x42095ce9u}, // log2(rel_err)= 29
        {0xbc09d551u, 0x43574549u, 0x42d746d8u,
         0x420f8026u}, // log2(rel_err)= 29
        {0xbc0ffe84u, 0x4360cd28u, 0x42e0ced6u,
         0x4215d6e6u}, // log2(rel_err)= 29
        {0xbc1639e0u, 0x436ac10du, 0x42eac2dau,
         0x421c753fu}, // log2(rel_err)= 29
        {0xbc1cf13cu, 0x437525c0u, 0x42f527aeu,
         0x42236bc8u}, // log2(rel_err)= 29
        {0xbc24013bu, 0x43800020u, 0x43000128u,
         0x422aab80u}, // log2(rel_err)= 29
        {0xbc2b1fbau, 0x4385aae2u, 0x4305abfbu,
         0x42323dc4u}, // log2(rel_err)= 29
        {0xbc32bb6eu, 0x438b95dfu, 0x430b970au,
         0x423a309fu}, // log2(rel_err)= 29
        {0xbc3a545bu, 0x4391c3f0u, 0x4311c52cu,
         0x42423784u}, // log2(rel_err)= 29
        {0xbc434d02u, 0x4398380bu, 0x4318395cu,
         0x424b085au}, // log2(rel_err)= 29
        {0xbc4bb388u, 0x439ef54cu, 0x431ef6b0u,
         0x4253fe27u}, // log2(rel_err)= 29
        {0xbc550336u, 0x43a5feefu, 0x43260068u,
         0x425d6fd1u}, // log2(rel_err)= 29
        {0xbc5ddd11u, 0x43ad5857u, 0x432d59e3u,
         0x4266fceeu}, // log2(rel_err)= 29
        {0xbc67b80eu, 0x43b5050au, 0x433506acu,
         0x42714f70u}, // log2(rel_err)= 29
        {0xbc7287acu, 0x43bd08b9u, 0x433d0a73u,
         0x427c23d7u}, // log2(rel_err)= 29
        {0xbc7cd928u, 0x43c5673fu, 0x4345690fu,
         0x4283935fu}, // log2(rel_err)= 29
        {0xbc83f35fu, 0x43ce24a0u, 0x434e2688u,
         0x42896c60u}, // log2(rel_err)= 29
        {0xbc89d012u, 0x43d74510u, 0x43574711u,
         0x428f7cbau}, // log2(rel_err)= 29
        {0xbc901429u, 0x43e0ccf1u, 0x4360cf0du,
         0x4295e444u}, // log2(rel_err)= 29
        {0xbc9663d8u, 0x43eac0d8u, 0x436ac30fu,
         0x429c8f50u}, // log2(rel_err)= 29
        {0xbc9ce93au, 0x43f5258eu, 0x437527e0u,
         0x42a366a7u}, // log2(rel_err)= 29
        {0xbca4013bu, 0x44000008u, 0x43800140u,
         0x42aaab60u}, // log2(rel_err)= 29
        {0xbcab21f8u, 0x4405aacbu, 0x4385ac12u,
         0x42b23f0cu}, // log2(rel_err)= 29
        {0xbcb2ba65u, 0x440b95c9u, 0x438b9720u,
         0x42ba2fdcu}, // log2(rel_err)= 29
        {0xbcba4ac0u, 0x4411c3dbu, 0x4391c541u,
         0x42c23169u}, // log2(rel_err)= 29
        {0xbcc335c5u, 0x441837f7u, 0x43983970u,
         0x42caf9bdu}, // log2(rel_err)= 29
        {0xbccb89d0u, 0x441ef539u, 0x439ef6c3u,
         0x42d3e401u}, // log2(rel_err)= 29
        {0xbcd4c25du, 0x4425feddu, 0x43a6007au,
         0x42dd473cu}, // log2(rel_err)= 29
        {0xbcde00abu, 0x442d5845u, 0x43ad59f5u,
         0x42e71310u}, // log2(rel_err)= 29
        {0xbce7bbd3u, 0x443504f9u, 0x43b506bdu,
         0x42f151b4u}, // log2(rel_err)= 29
        {0xbcf26788u, 0x443d08a9u, 0x43bd0a83u,
         0x42fc0fb1u}, // log2(rel_err)= 29
        {0xbcfd1138u, 0x4445672fu, 0x43c5691fu,
         0x4303a4d4u}, // log2(rel_err)= 29
        {0xbd03f9a0u, 0x444e2491u, 0x43ce2697u,
         0x4309703eu}, // log2(rel_err)= 29
        {0xbd09bec2u, 0x44574502u, 0x43d7471fu,
         0x430f71e1u}, // log2(rel_err)= 29
        {0xbd0fe991u, 0x4460cce4u, 0x43e0cf1au,
         0x4315c9a3u}, // log2(rel_err)= 29
        {0xbd165e55u, 0x446ac0cbu, 0x43eac31cu,
         0x431c8bd6u}, // log2(rel_err)= 29
        {0xbd1d073au, 0x44752581u, 0x43f527edu,
         0x4323795au}, // log2(rel_err)= 29
        {0xbd24013bu, 0x44800002u, 0x44000146u,
         0x432aab58u}, // log2(rel_err)= 29
        {0xbd2b4289u, 0x4485aac5u, 0x4405ac18u,
         0x43325359u}, // log2(rel_err)= 29
        {0xbd327a20u, 0x448b95c4u, 0x440b9725u,
         0x433a07b4u}, // log2(rel_err)= 29
        {0xbd3aa85eu, 0x4491c3d5u, 0x4411c547u,
         0x43426bd4u}, // log2(rel_err)= 29
        {0xbd432ff6u, 0x449837f2u, 0x44183975u,
         0x434af616u}, // log2(rel_err)= 29
        {0xbd4b9f63u, 0x449ef534u, 0x441ef6c8u,
         0x4353f173u}, // log2(rel_err)= 29
        {0xbd54f22au, 0x44a5fed8u, 0x4426007fu,
         0x435d650du}, // log2(rel_err)= 29
        {0xbd5e4994u, 0x44ad5840u, 0x442d59fau,
         0x4367408fu}, // log2(rel_err)= 29
        {0xbd679cc3u, 0x44b504f5u, 0x443506c1u,
         0x43713e4au}, // log2(rel_err)= 29
        {0xbd725f7fu, 0x44bd08a5u, 0x443d0a87u,
         0x437c0aa7u}, // log2(rel_err)= 29
        {0xbd7d1f3du, 0x44c5672bu, 0x44456923u,
         0x4383a932u}, // log2(rel_err)= 29
        {0xbd840b31u, 0x44ce248du, 0x444e269bu,
         0x43897b33u}, // log2(rel_err)= 29
        {0xbd89da6fu, 0x44d744feu, 0x44574723u,
         0x438f8326u}, // log2(rel_err)= 29
        {0xbd900eecu, 0x44e0cce0u, 0x4460cf1eu,
         0x4395e0f4u}, // log2(rel_err)= 29
        {0xbd964cf4u, 0x44eac0c8u, 0x446ac31fu,
         0x439c80fau}, // log2(rel_err)= 29
        {0xbd9cfebau, 0x44f5257eu, 0x447527f0u,
         0x43a37409u}, // log2(rel_err)= 29
        {0xbda3c138u, 0x45000001u, 0x44800147u,
         0x43aa835fu}, // log2(rel_err)= 29
        {0xbdab0aaau, 0x4505aac4u, 0x4485ac19u,
         0x43b23076u}, // log2(rel_err)= 29
        {0xbdb2ca13u, 0x450b95c2u, 0x448b9727u,
         0x43ba399cu}, // log2(rel_err)= 29
        {0xbdba7fc2u, 0x4511c3d4u, 0x4491c548u,
         0x43c25278u}, // log2(rel_err)= 29
        {0xbdc30e81u, 0x451837f1u, 0x44983976u,
         0x43cae131u}, // log2(rel_err)= 29
        {0xbdcb84c6u, 0x451ef533u, 0x449ef6c9u,
         0x43d3e0d4u}, // log2(rel_err)= 29
        {0xbdd4de1bu, 0x4525fed7u, 0x44a60080u,
         0x43dd5886u}, // log2(rel_err)= 29
        {0xbdde3bcdu, 0x452d583fu, 0x44ad59fbu,
         0x43e737f4u}, // log2(rel_err)= 29
        {0xbde794ffu, 0x453504f4u, 0x44b506c2u,
         0x43f1396fu}, // log2(rel_err)= 29
        {0xbdf25d7du, 0x453d08a4u, 0x44bd0a88u,
         0x43fc0964u}, // log2(rel_err)= 29
        {0xbdfd22beu, 0x4545672au, 0x44c56924u,
         0x4403aa49u}, // log2(rel_err)= 29
        {0xbe040f95u, 0x454e248cu, 0x44ce269cu,
         0x44097df0u}, // log2(rel_err)= 29
        {0xbe09e15au, 0x455744fdu, 0x44d74724u,
         0x440f8777u}, // log2(rel_err)= 29
        {0xbe101843u, 0x4560ccdfu, 0x44e0cf1fu,
         0x4415e6c8u}, // log2(rel_err)= 29
        {0xbe16589cu, 0x456ac0c7u, 0x44eac320u,
         0x441c8841u}, // log2(rel_err)= 29
        {0xbe1d0c9au, 0x4575257du, 0x44f527f1u,
         0x44237cb2u}, // log2(rel_err)= 29
        {0xbe24113cu, 0x45800000u, 0x45000148u,
         0x442ab553u}, // log2(rel_err)= 29
        {0xbe2adcb1u, 0x4585aac4u, 0x4505ac19u,
         0x443213c2u}, // log2(rel_err)= 29
        {0xbe329e0du, 0x458b95c2u, 0x450b9727u,
         0x443a1e20u}, // log2(rel_err)= 29
        {0xbe3a559au, 0x4591c3d4u, 0x4511c548u,
         0x44423826u}, // log2(rel_err)= 29
        {0xbe436628u, 0x459837f0u, 0x45183977u,
         0x444b17e9u}, // log2(rel_err)= 29
        {0xbe4b5e1eu, 0x459ef533u, 0x451ef6c9u,
         0x4453c8b1u}, // log2(rel_err)= 29
        {0xbe54b916u, 0x45a5fed7u, 0x45260080u,
         0x445d4169u}, // log2(rel_err)= 29
        {0xbe5e185au, 0x45ad583fu, 0x452d59fbu,
         0x446721d2u}, // log2(rel_err)= 29
        {0xbe67f312u, 0x45b504f3u, 0x453506c3u,
         0x4471742au}, // log2(rel_err)= 29
        {0xbe723cfbu, 0x45bd08a4u, 0x453d0a88u,
         0x447bf519u}, // log2(rel_err)= 29
        {0xbe7d039cu, 0x45c5672au, 0x45456924u,
         0x4483a091u}, // log2(rel_err)= 29
        {0xbe8400adu, 0x45ce248cu, 0x454e269cu,
         0x448974a2u}, // log2(rel_err)= 29
        {0xbe89d314u, 0x45d744fdu, 0x45574724u,
         0x448f7e8eu}, // log2(rel_err)= 29
        {0xbe900a98u, 0x45e0ccdfu, 0x4560cf1fu,
         0x4495de3fu}, // log2(rel_err)= 29
        {0xbe964b86u, 0x45eac0c7u, 0x456ac320u,
         0x449c8015u}, // log2(rel_err)= 29
        {0xbe9d0012u, 0x45f5257du, 0x457527f1u,
         0x44a374dfu}, // log2(rel_err)= 29
        {0xbea4053bu, 0x46000000u, 0x45800148u,
         0x44aaadd5u}, // log2(rel_err)= 29
        {0xbeab5139u, 0x4605aac3u, 0x4585ac1au,
         0x44b25c82u}, // log2(rel_err)= 29
        {0xbeb2930bu, 0x460b95c2u, 0x458b9727u,
         0x44ba1741u}, // log2(rel_err)= 29
        {0xbeba4b10u, 0x4611c3d4u, 0x4591c548u,
         0x44c23191u}, // log2(rel_err)= 29
        {0xbec35c10u, 0x461837f0u, 0x45983977u,
         0x44cb119cu}, // log2(rel_err)= 29
        {0xbecbd479u, 0x461ef532u, 0x459ef6cau,
         0x44d41295u}, // log2(rel_err)= 29
        {0xbed4afd5u, 0x4625fed7u, 0x45a60080u,
         0x44dd3ba2u}, // log2(rel_err)= 29
        {0xbede0f7du, 0x462d583fu, 0x45ad59fbu,
         0x44e71c49u}, // log2(rel_err)= 29
        {0xbee7ea95u, 0x463504f3u, 0x45b506c3u,
         0x44f16edeu}, // log2(rel_err)= 29
        {0xbef234dau, 0x463d08a4u, 0x45bd0a88u,
         0x44fbf006u}, // log2(rel_err)= 29
        {0xbefcfbd4u, 0x4645672au, 0x45c56924u,
         0x45039e24u}, // log2(rel_err)= 29
        {0xbf03fcf3u, 0x464e248cu, 0x45ce269cu,
         0x4509724eu}, // log2(rel_err)= 29
        {0xbf09cf83u, 0x465744fdu, 0x45d74724u,
         0x450f7c54u}, // log2(rel_err)= 29
        {0xbf10072eu, 0x4660ccdfu, 0x45e0cf1fu,
         0x4515dc1du}, // log2(rel_err)= 29
        {0xbf164840u, 0x466ac0c7u, 0x45eac320u,
         0x451c7e0au}, // log2(rel_err)= 29
        {0xbf1cfcf0u, 0x4675257du, 0x45f527f1u,
         0x452372eau}, // log2(rel_err)= 29
        {0xbf24023bu, 0x46800000u, 0x46000148u,
         0x452aabf5u}, // log2(rel_err)= 29
        {0xbf2b4e59u, 0x4685aac3u, 0x4605ac1au,
         0x45325ab7u}, // log2(rel_err)= 29
        {0xbf32904bu, 0x468b95c2u, 0x460b9727u,
         0x453a1589u}, // log2(rel_err)= 29
        {0xbf3ac873u, 0x4691c3d3u, 0x4611c549u,
         0x45427fd9u}, // log2(rel_err)= 29
        {0xbf43598au, 0x469837f0u, 0x46183977u,
         0x454b1009u}, // log2(rel_err)= 29
        {0xbf4bd20fu, 0x469ef532u, 0x461ef6cau,
         0x45541113u}, // log2(rel_err)= 29
        {0xbf54ad85u, 0x46a5fed7u, 0x46260080u,
         0x455d3a30u}, // log2(rel_err)= 29
        {0xbf5e0d46u, 0x46ad583fu, 0x462d59fbu,
         0x45671ae7u}, // log2(rel_err)= 29
        {0xbf67e876u, 0x46b504f3u, 0x463506c3u,
         0x45716d8au}, // log2(rel_err)= 29
        {0xbf7232d2u, 0x46bd08a4u, 0x463d0a88u,
         0x457beec1u}, // log2(rel_err)= 29
        {0xbf7cf9e2u, 0x46c5672au, 0x46456924u,
         0x45839d88u}, // log2(rel_err)= 29
        {0xbf83fc05u, 0x46ce248cu, 0x464e269cu,
         0x458971b9u}, // log2(rel_err)= 29
        {0xbf89ce9fu, 0x46d744fdu, 0x46574724u,
         0x458f7bc5u}, // log2(rel_err)= 29
        {0xbf900653u, 0x46e0ccdfu, 0x4660cf1fu,
         0x4595db95u}, // log2(rel_err)= 29
        {0xbf96476fu, 0x46eac0c7u, 0x466ac320u,
         0x459c7d87u}, // log2(rel_err)= 29
        {0xbf9cfc27u, 0x46f5257du, 0x467527f1u,
         0x45a3726du} // log2(rel_err)= 29
    },
    32 /* _iIndexTrshld = 32 */
};     /*sSinh_Table*/
static __constant union {
  unsigned int w;
  float f;
  int i;
} __ssinh_ha_Shifter = {0x4ac000feu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __ssinh_ha_L2E = {0x3FB8AA3Bu};
// log(2) high, low
static __constant union {
  unsigned int w;
  float f;
  int i;
} __ssinh_ha_L2H = {0x3f317218u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __ssinh_ha_L2L = {0xb102E308u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __ssinh_ha_c5 = {0x3c08ba8bu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __ssinh_ha_c4 = {0x3d2aec4eu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __ssinh_ha_c3 = {0x3e2aaa9cu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __ssinh_ha_c2 = {0x3effffe8u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __ssinh_ha_c1 = {0x3f800000u};
__attribute__((always_inline)) inline int
__ocl_svml_internal_ssinh_ha(float *a, float *r) {
  int nRet = 0;
  float x = SPIRV_OCL_BUILTIN(fabs, _f32, )(*a);
  union {
    unsigned int w;
    float f;
    int i;
  } S, Th, Tlr, Th2, xin, xa, res;
  float N, R, poly;
  int index_mask;
  float ressign = ((*a) > 0.0f) ? 1.0f : -1.0f;
  S.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(x, __ssinh_ha_L2E.f,
                                               __ssinh_ha_Shifter.f);
  N = S.f - __ssinh_ha_Shifter.f;
  R = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )((-N), __ssinh_ha_L2H.f, x);
  R = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )((-N), __ssinh_ha_L2L.f, R);
  // set exponent in place
  Th.w = S.w << 22;
  // index_mask is based on last bit of S.w
  index_mask = 0 - (S.w & 1);
  // set Th mantissa
  Th.w ^= (index_mask & 0x7504F3u);
  // set Tl/Th value
  Tlr.w = index_mask & 0x329302AEu;
  // polynomial
  poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(R, __ssinh_ha_c5.f,
                                                __ssinh_ha_c4.f);
  poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(R, poly, __ssinh_ha_c3.f);
  poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(R, poly, __ssinh_ha_c2.f);
  poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(R, poly, __ssinh_ha_c1.f);
  poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(R, poly, Tlr.f);
  xin.f = x;
  xa.w = xin.w & 0x7fffffffu;
  // redirect special cases
  if (xa.w > 0x42AEAC4Fu)
    goto SINHF_SPECIAL;
  res.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(poly, Th.f, Th.f);
  *r = ressign * 0.5f * res.f;
  return nRet;
SINHF_SPECIAL:
  if (xa.w > 0x42b2d4fcu) {
    if (xa.w > 0x7f800000u) { // NaN?
      *r = ressign * (x + x);
      return nRet;
    }
    // overflow
    res.w = 0x7f800000;
    *r = ressign * res.f;
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
  res.f = 0.5f * SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(poly, Th.f, Th.f);
  res.f *= Th2.f;
  *r = ressign * res.f;
  return nRet;
}
float __ocl_svml_sinhf_ha(float x) {
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
    // 1) if Jn<32, we use coeff of the poly sh(ln2*Jn/2^K + r)-(ln2*Jn/2^K +
    r), so |x| is added in the end
    // 2) otherwise , we keep in a0 the difference SP(D(a0)-2*a2), where D(a0) -
    DP coeff, so 2*a2 is added in the end
    //
    */
    float sN;
    float sM;
    float sR;
    float sOut;
    unsigned int iM;
    unsigned int iMScale;
    float sXSign;
    float sMask;
    float sPC[4];
    float sAbsX;
    unsigned int iRangeMask;
    unsigned int iIndex;
    unsigned int iX;
    float sInvLn2;
    float sShifter;
    float sLn2[2];
    unsigned int iIndexMask;
    unsigned int iDomainRange;
    unsigned int iIndexTrshld;
    sXSign = as_float(__ocl_svml_internal_ssinh_ha_data._sSign);
    sShifter = as_float(__ocl_svml_internal_ssinh_ha_data._sShifter);
    iDomainRange = (__ocl_svml_internal_ssinh_ha_data._iDomainRange);
    iIndexMask = (__ocl_svml_internal_ssinh_ha_data._iIndexMask);
    sLn2[0] = as_float(__ocl_svml_internal_ssinh_ha_data._sLn2hi);
    sLn2[1] = as_float(__ocl_svml_internal_ssinh_ha_data._sLn2lo);
    sInvLn2 = as_float(__ocl_svml_internal_ssinh_ha_data._sInvLn2);
    iIndexTrshld = (__ocl_svml_internal_ssinh_ha_data._iIndexTrshld);
    // Compute argument sign and absolute argument
    sXSign = as_float((as_uint(sXSign) & as_uint(va1)));
    sAbsX = as_float((as_uint(sXSign) ^ as_uint(va1)));
    // Scale & Index:
    // sM = x*2^K/log(2) + RShifter
    sM = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(sAbsX, sInvLn2, sShifter);
    iX = as_uint(sAbsX);
    // Check for overflow\underflow
    iRangeMask = ((unsigned int)(-(signed int)((signed int)iX >
                                               (signed int)iDomainRange)));
    vm = 0;
    vm = iRangeMask;
    // Index and Scale:
    sN = as_float((as_uint(sM) ^ as_uint(sShifter)));
    iM = as_uint(sN);
    // if sN>255, iIndex=iIndex|11110000b,so iIndex will be >=15
    // iIndex=255-iM
    iIndex = (iIndexMask - iM);
    // iIndex=(255-iM)>>28= 0 OR 1111b
    iIndex = ((unsigned int)(iIndex) >> ((32 - 4)));
    // iIndex= 0 OR 11110000b
    iIndex = ((unsigned int)(iIndex) << (4));
    // iIndex=iM OR Mscale|new adjusted index
    iIndex = (iIndex | iM);
    // iIndex=adjusted index
    iIndex = (iIndex & iIndexMask);
    // iMScale = iM-adjustedindex
    iMScale = (iM - iIndex);
    // iMscale is in SPFP EXP bits now
    iMScale = ((unsigned int)(iMScale) << ((23 - 4)));
    // Reduced R:
    // sN = sM - RShifter
    sN = (sM - sShifter);
    // sR = sX - sN*sLog2_hi/2^K
    sR = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(-(sLn2[0]), sN, sAbsX);
    // sR = (sX - sN*Log2_hi/2^K)-sN*Log2_lo/2^K
    sR = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(-(sLn2[1]), sN, sR);
    // poly(r) = a0+a1*r+...a3*r^4
    sPC[2] = as_float(
        ((unsigned int *)(__ocl_svml_internal_ssinh_ha_data
                              ._dbT))[(((0 + iIndex) * (4 * 4)) >> (2)) + 2]);
    sPC[3] = as_float(
        ((unsigned int *)(__ocl_svml_internal_ssinh_ha_data
                              ._dbT))[(((0 + iIndex) * (4 * 4)) >> (2)) + 3]);
    // smask =-1 if |x|<(32/16+1/32)*log(2) (so Jn<32)
    iM = ((unsigned int)(-(signed int)((signed int)iIndex <
                                       (signed int)iIndexTrshld)));
    sMask = as_float(iM);
    // sN=2*a2
    sN = (sPC[2] + sPC[2]);
    // sOut = (a2 +a3*sR)
    sOut = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(sPC[3], sR, sPC[2]);
    sPC[1] = as_float(
        ((unsigned int *)(__ocl_svml_internal_ssinh_ha_data
                              ._dbT))[(((0 + iIndex) * (4 * 4)) >> (2)) + 1]);
    sAbsX = as_float((as_uint(sAbsX) & as_uint(sMask)));
    // sOut = a1+sR*(a2 +a3*sR)
    sOut = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(sOut, sR, sPC[1]);
    sPC[0] = as_float(
        ((unsigned int *)(__ocl_svml_internal_ssinh_ha_data
                              ._dbT))[(((0 + iIndex) * (4 * 4)) >> (2)) + 0]);
    sN = as_float((~(as_uint(sMask)) & as_uint(sN)));
    // sOut = a0+sR*(a1+sR*(a2 +a3*sR))
    sOut = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(sOut, sR, sPC[0]);
    // sN=sAbsX<RThr?sAbsX:2*a2
    sN = as_float((as_uint(sN) | as_uint(sAbsX)));
    // sOut = (2*a2?sAbsX)+a0+sR*(a1+sR*(a2 +a3*sR))
    sOut = (sOut + sN);
    // Scale up
    iM = as_uint(sOut);
    iM = (iM + iMScale);
    sOut = as_float(iM);
    // Set result sign
    vr1 = as_float((as_uint(sXSign) | as_uint(sOut)));
  }
  if (__builtin_expect((vm) != 0, 0)) {
    float __cout_a1;
    float __cout_r1;
    ((float *)&__cout_a1)[0] = va1;
    ((float *)&__cout_r1)[0] = vr1;
    __ocl_svml_internal_ssinh_ha(&__cout_a1, &__cout_r1);
    vr1 = ((float *)&__cout_r1)[0];
  }
  r = vr1;
  return r;
}

/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF
#pragma float_control(precise,on)
typedef struct
{
    unsigned long _erf_tbl[6 * 128 * 2];

    unsigned long _AbsMask;
    unsigned long _MaxThreshold;
    unsigned long _SRound;
    unsigned long _U2Threshold;
    unsigned long _SignMask;
    unsigned long _One;

    unsigned long _poly1_0;
    unsigned long _poly1_1;
    unsigned long _poly3_0;
    unsigned long _poly3_1;
    unsigned long _poly5_0;
    unsigned long _poly5_1;
    unsigned long _poly1_2;
    unsigned long _poly3_2;
    unsigned long _poly1_3;
    unsigned long _poly3_3;
    unsigned long _poly1_4;
    unsigned long _poly1_5;

    unsigned long _Mask32;

    unsigned long _ep_poly1_0;
    unsigned long _ep_poly1_1;
    unsigned long _ep_poly3_0;
    unsigned long _ep_poly3_1;

    unsigned long _gf_MaxThreshold_EP;
    unsigned long _gf_ep_poly_0;
    unsigned long _gf_ep_poly_1;
    unsigned long _gf_ep_poly_2;
    unsigned long _gf_ep_poly_3;
    unsigned long _gf_ep_poly_4;
    unsigned long _gf_ep_poly_5;
    unsigned long _gf_ep_poly_6;
    unsigned long _gf_ep_poly_7;
    unsigned long _gf_ep_poly_8;
    unsigned long _gf_ep_poly_9;
    unsigned long _gf_ep_poly_10;
    unsigned long _gf_ep_poly_11;
    unsigned long _gf_ep_poly_12;
    unsigned long _gf_ep_poly_13;
    unsigned long _gf_ep_poly_14;
    unsigned long _gf_ep_poly_15;
    unsigned long _gf_ep_poly_16;
    unsigned long _gf_ep_poly_17;

} __internal_derf_la_data_t;
static __constant __internal_derf_la_data_t __internal_derf_la_data = {

    {
     0x0000000000000000uL, 0x3ff20dd750429b6duL,
     0x3f820dbf3deb1340uL, 0x3ff20d8f1975c85duL,
     0x3f920d77083f17a0uL, 0x3ff20cb67bd452c7uL,
     0x3f9b137e0cf584dcuL, 0x3ff20b4d8bac36c1uL,
     0x3fa20c5645dd2538uL, 0x3ff209546ad13ccfuL,
     0x3fa68e5d3bbc9526uL, 0x3ff206cb4897b148uL,
     0x3fab0fafef135745uL, 0x3ff203b261cd0053uL,
     0x3faf902a77bd3821uL, 0x3ff2000a00ae3804uL,
     0x3fb207d480e90658uL, 0x3ff1fbd27cdc72d3uL,
     0x3fb44703e87e8593uL, 0x3ff1f70c3b4f2cc8uL,
     0x3fb68591a1e83b5duL, 0x3ff1f1b7ae44867fuL,
     0x3fb8c36beb8a8d23uL, 0x3ff1ebd5552f795buL,
     0x3fbb0081148a873auL, 0x3ff1e565bca400d4uL,
     0x3fbd3cbf7e70a4b3uL, 0x3ff1de697e413d29uL,
     0x3fbf78159ec8bb50uL, 0x3ff1d6e14099944auL,
     0x3fc0d939005f65e5uL, 0x3ff1cecdb718d61cuL,
     0x3fc1f5e1a35c3b89uL, 0x3ff1c62fa1e869b6uL,
     0x3fc311fc15f56d14uL, 0x3ff1bd07cdd189acuL,
     0x3fc42d7fc2f64959uL, 0x3ff1b357141d95d5uL,
     0x3fc548642321d7c6uL, 0x3ff1a91e5a748165uL,
     0x3fc662a0bdf7a89fuL, 0x3ff19e5e92b964abuL,
     0x3fc77c2d2a765f9euL, 0x3ff19318bae53a04uL,
     0x3fc895010fdbdbfduL, 0x3ff1874ddcdfce24uL,
     0x3fc9ad142662e14duL, 0x3ff17aff0e56ec10uL,
     0x3fcac45e37fe2526uL, 0x3ff16e2d7093cd8cuL,
     0x3fcbdad72110a648uL, 0x3ff160da304ed92fuL,
     0x3fccf076d1233237uL, 0x3ff153068581b781uL,
     0x3fce05354b96ff36uL, 0x3ff144b3b337c90cuL,
     0x3fcf190aa85540e2uL, 0x3ff135e3075d076buL,
     0x3fd015f78a3dcf3duL, 0x3ff12695da8b5bdeuL,
     0x3fd09eed6982b948uL, 0x3ff116cd8fd67618uL,
     0x3fd127631eb8de32uL, 0x3ff1068b94962e5euL,
     0x3fd1af54e232d609uL, 0x3ff0f5d1602f7e41uL,
     0x3fd236bef825d9a2uL, 0x3ff0e4a073dc1b91uL,
     0x3fd2bd9db0f7827fuL, 0x3ff0d2fa5a70c168uL,
     0x3fd343ed6989b7d9uL, 0x3ff0c0e0a8223359uL,
     0x3fd3c9aa8b84bedauL, 0x3ff0ae54fa490723uL,
     0x3fd44ed18d9f6462uL, 0x3ff09b58f724416buL,
     0x3fd4d35ef3e5372euL, 0x3ff087ee4d9ad247uL,
     0x3fd5574f4ffac98euL, 0x3ff07416b4fbfe7cuL,
     0x3fd5da9f415ff23fuL, 0x3ff05fd3ecbec298uL,
     0x3fd65d4b75b00471uL, 0x3ff04b27bc403d30uL,
     0x3fd6df50a8dff772uL, 0x3ff03613f2812dafuL,
     0x3fd760aba57a76bfuL, 0x3ff0209a65e29545uL,
     0x3fd7e15944d9d3e4uL, 0x3ff00abcf3e187a9uL,
     0x3fd861566f5fd3c0uL, 0x3fefe8fb01a47307uL,
     0x3fd8e0a01cab516buL, 0x3fefbbbbef34b4b2uL,
     0x3fd95f3353cbb146uL, 0x3fef8dc092d58ff8uL,
     0x3fd9dd0d2b721f39uL, 0x3fef5f0cdaf15313uL,
     0x3fda5a2aca209394uL, 0x3fef2fa4c16c0019uL,
     0x3fdad68966569a87uL, 0x3feeff8c4b1375dbuL,
     0x3fdb522646bbda68uL, 0x3feecec7870ebca8uL,
     0x3fdbccfec24855b8uL, 0x3fee9d5a8e4c934euL,
     0x3fdc4710406a65fcuL, 0x3fee6b4982f158b9uL,
     0x3fdcc058392a6d2duL, 0x3fee38988fc46e72uL,
     0x3fdd38d4354c3bd0uL, 0x3fee054be79d3042uL,
     0x3fddb081ce6e2a48uL, 0x3fedd167c4cf9d2auL,
     0x3fde275eaf25e458uL, 0x3fed9cf06898cdafuL,
     0x3fde9d68931ae650uL, 0x3fed67ea1a8b5368uL,
     0x3fdf129d471eabb1uL, 0x3fed325927fb9d89uL,
     0x3fdf86faa9428f9duL, 0x3fecfc41e36c7df9uL,
     0x3fdffa7ea8eb5fd0uL, 0x3fecc5a8a3fbea40uL,
     0x3fe03693a371519cuL, 0x3fec8e91c4d01368uL,
     0x3fe06f794ab2cae7uL, 0x3fec5701a484ef9duL,
     0x3fe0a7ef5c18edd2uL, 0x3fec1efca49a5011uL,
     0x3fe0dff4f247f6c6uL, 0x3febe68728e29d5euL,
     0x3fe1178930ada115uL, 0x3febada596f25436uL,
     0x3fe14eab43841b55uL, 0x3feb745c55905bf8uL,
     0x3fe1855a5fd3dd50uL, 0x3feb3aafcc27502euL,
     0x3fe1bb95c3746199uL, 0x3feb00a46237d5beuL,
     0x3fe1f15cb50bc4deuL, 0x3feac63e7ecc1411uL,
     0x3fe226ae840d4d70uL, 0x3fea8b8287ec6a09uL,
     0x3fe25b8a88b6dd7fuL, 0x3fea5074e2157620uL,
     0x3fe28ff0240d52cduL, 0x3fea1519efaf889euL,
     0x3fe2c3debfd7d6c1uL, 0x3fe9d97610879642uL,
     0x3fe2f755ce9a21f4uL, 0x3fe99d8da149c13fuL,
     0x3fe32a54cb8db67buL, 0x3fe96164fafd8de3uL,
     0x3fe35cdb3a9a144duL, 0x3fe925007283d7aauL,
     0x3fe38ee8a84beb71uL, 0x3fe8e86458169af8uL,
     0x3fe3c07ca9cb4f9euL, 0x3fe8ab94f6caa71duL,
     0x3fe3f196dcd0f135uL, 0x3fe86e9694134b9euL,
     0x3fe42236e79a5fa6uL, 0x3fe8316d6f48133duL,
     0x3fe4525c78dd5966uL, 0x3fe7f41dc12c9e89uL,
     0x3fe4820747ba2dc2uL, 0x3fe7b6abbb7aaf19uL,
     0x3fe4b13713ad3513uL, 0x3fe7791b886e7403uL,
     0x3fe4dfeba47f63ccuL, 0x3fe73b714a552763uL,
     0x3fe50e24ca35fd2cuL, 0x3fe6fdb11b1e0c34uL,
     0x3fe53be25d016a4fuL, 0x3fe6bfdf0beddaf5uL,
     0x3fe569243d2b3a9buL, 0x3fe681ff24b4ab04uL,
     0x3fe595ea53035283uL, 0x3fe6441563c665d4uL,
     0x3fe5c2348ecc4dc3uL, 0x3fe60625bd75d07buL,
     0x3fe5ee02e8a71a53uL, 0x3fe5c8341bb23767uL,
     0x3fe61955607dd15duL, 0x3fe58a445da7c74cuL,
     0x3fe6442bfdedd397uL, 0x3fe54c5a57629db0uL,
     0x3fe66e86d0312e82uL, 0x3fe50e79d1749ac9uL,
     0x3fe69865ee075011uL, 0x3fe4d0a6889dfd9fuL,
     0x3fe6c1c9759d0e5fuL, 0x3fe492e42d78d2c5uL,
     0x3fe6eab18c74091buL, 0x3fe4553664273d24uL,
     0x3fe7131e5f496a5auL, 0x3fe417a0c4049fd0uL,
     0x3fe73b1021fc0cb8uL, 0x3fe3da26d759aef5uL,
     0x3fe762870f720c6fuL, 0x3fe39ccc1b136d5auL,
     0x3fe78983697dc96fuL, 0x3fe35f93fe7d1b3duL,
     0x3fe7b00578c26037uL, 0x3fe32281e2fd1a92uL,
     0x3fe7d60d8c979f7buL, 0x3fe2e5991bd4cbfcuL,
     0x3fe7fb9bfaed8078uL, 0x3fe2a8dcede3673buL,
     0x3fe820b1202f27fbuL, 0x3fe26c508f6bd0ffuL,
     0x3fe8454d5f25760duL, 0x3fe22ff727dd6f7buL,
     0x3fe8697120d92a4auL, 0x3fe1f3d3cf9ffe5auL,
     0x3fe88d1cd474a2e0uL, 0x3fe1b7e98fe26217uL,
     0x3fe8b050ef253c37uL, 0x3fe17c3b626c7a12uL,
     0x3fe8d30debfc572euL, 0x3fe140cc3173f007uL,
     0x3fe8f5544bd00c04uL, 0x3fe1059ed7740313uL,
     0x3fe91724951b8fc6uL, 0x3fe0cab61f084b93uL,
     0x3fe9387f53df5238uL, 0x3fe09014c2ca74dauL,
     0x3fe959651980da31uL, 0x3fe055bd6d32e8d7uL,
     0x3fe979d67caa6631uL, 0x3fe01bb2b87c6968uL,
     0x3fe999d4192a5715uL, 0x3fdfc3ee5d1524b0uL,
     0x3fe9b95e8fd26abauL, 0x3fdf511a91a67d2auL,
     0x3fe9d8768656cc42uL, 0x3fdedeeee0959518uL,
     0x3fe9f71ca72cffb6uL, 0x3fde6d6ffaa65a25uL,
     0x3fea1551a16aaeafuL, 0x3fddfca26f5bbf88uL,
     0x3fea331628a45b92uL, 0x3fdd8c8aace11e63uL,
     0x3fea506af4cc00f4uL, 0x3fdd1d2cfff91594uL,
     0x3fea6d50c20fa293uL, 0x3fdcae8d93f1d7b7uL,
     0x3fea89c850b7d54duL, 0x3fdc40b0729ed548uL,
     0x3feaa5d265064366uL, 0x3fdbd3998457afdbuL,
     0x3feac16fc7143263uL, 0x3fdb674c8ffc6283uL,
     0x3feadca142b10f98uL, 0x3fdafbcd3afe8ab6uL,
     0x3feaf767a741088buL, 0x3fda911f096fbc26uL,
     0x3feb11c3c79bb424uL, 0x3fda27455e14c93cuL,
     0x3feb2bb679ead19cuL, 0x3fd9be437a7de946uL,
     0x3feb4540978921eeuL, 0x3fd9561c7f23a47buL,
     0x3feb5e62fce16095uL, 0x3fd8eed36b886d93uL,
     0x3feb771e894d602euL, 0x3fd8886b1e5ecfd1uL,
     0x3feb8f741ef54f83uL, 0x3fd822e655b417e7uL,
     0x3feba764a2af2b78uL, 0x3fd7be47af1f5d89uL,
     0x3febbef0fbde6221uL, 0x3fd75a91a7f4d2eduL,
     0x3febd61a1453ab44uL, 0x3fd6f7c69d7d3ef8uL,
     0x3febece0d82d1a5cuL, 0x3fd695e8cd31867euL,
     0x3fec034635b66e23uL, 0x3fd634fa54fa285fuL,
     0x3fec194b1d49a184uL, 0x3fd5d4fd33729015uL,
     0x3fec2ef0812fc1bduL, 0x3fd575f3483021c3uL,
     0x3fec443755820d64uL, 0x3fd517de540ce2a3uL,
     0x3fec5920900b5fd1uL, 0x3fd4babff975a04cuL,
     0x3fec6dad2829ec62uL, 0x3fd45e99bcbb7915uL,
     0x3fec81de16b14cefuL, 0x3fd4036d0468a7a2uL,
     0x3fec95b455cce69duL, 0x3fd3a93b1998736cuL,
     0x3feca930e0e2a825uL, 0x3fd35005285227f1uL,
     0x3fecbc54b476248duL, 0x3fd2f7cc3fe6f423uL,
     0x3feccf20ce0c0d27uL, 0x3fd2a09153529381uL,
     0x3fece1962c0e0d8buL, 0x3fd24a55399ea239uL,
     0x3fecf3b5cdaf0c39uL, 0x3fd1f518ae487dc8uL,
     0x3fed0580b2cfd249uL, 0x3fd1a0dc51a9934duL,
     0x3fed16f7dbe41ca0uL, 0x3fd14da0a961fd14uL,
     0x3fed281c49d818d0uL, 0x3fd0fb6620c550afuL,
     0x3fed38eefdf64fdduL, 0x3fd0aa2d09497f2buL,
     0x3fed4970f9ce00d9uL, 0x3fd059f59af7a906uL,
     0x3fed59a33f19ed42uL, 0x3fd00abff4dec7a3uL,
     0x3fed6986cfa798e7uL, 0x3fcf79183b101c5buL,
     0x3fed791cad3eff01uL, 0x3fcedeb406d9c825uL,
     0x3fed8865d98abe01uL, 0x3fce4652fadcb6b2uL,
     0x3fed97635600bb89uL, 0x3fcdaff4969c0b04uL,
     0x3feda61623cb41e0uL, 0x3fcd1b982c501370uL,
     0x3fedb47f43b2980duL, 0x3fcc893ce1dcbef7uL,
     0x3fedc29fb60715afuL, 0x3fcbf8e1b1ca2279uL,
     0x3fedd0787a8bb39duL, 0x3fcb6a856c3ed54fuL,
     0x3fedde0a90611a0duL, 0x3fcade26b7fbed95uL,
     0x3fedeb56f5f12d28uL, 0x3fca53c4135a6526uL,
     0x3fedf85ea8db188euL, 0x3fc9cb5bd549b111uL,
     0x3fee0522a5dfda73uL, 0x3fc944ec2e4f5630uL,
     0x3fee11a3e8cf4eb8uL, 0x3fc8c07329874652uL,
     0x3fee1de36c75ba58uL, 0x3fc83deeada4d25auL,
     0x3fee29e22a89d766uL, 0x3fc7bd5c7df3fe9cuL,
     0x3fee35a11b9b61ceuL, 0x3fc73eba3b5b07b7uL,
     0x3fee4121370224ccuL, 0x3fc6c205655be720uL,
     0x3fee4c6372cd8927uL, 0x3fc6473b5b15a7a1uL,
     0x3fee5768c3b4a3fcuL, 0x3fc5ce595c455b0auL,
     0x3fee62321d06c5e0uL, 0x3fc5575c8a468362uL,
     0x3fee6cc0709c8a0duL, 0x3fc4e241e912c305uL,
     0x3fee7714aec96534uL, 0x3fc46f066040a832uL,
     0x3fee812fc64db369uL, 0x3fc3fda6bc016994uL,
     0x3fee8b12a44944a8uL, 0x3fc38e1fae1d6a9duL,
     0x3fee94be342e6743uL, 0x3fc3206dceef5f87uL,
     0x3fee9e335fb56f87uL, 0x3fc2b48d9e5dea1cuL,
     0x3feea7730ed0bbb9uL, 0x3fc24a7b84d38971uL,
     0x3feeb07e27a133aauL, 0x3fc1e233d434b813uL,
     0x3feeb9558e6b42ceuL, 0x3fc17bb2c8d41535uL,
     0x3feec1fa258c4beauL, 0x3fc116f48a6476ccuL,
     0x3feeca6ccd709544uL, 0x3fc0b3f52ce8c383uL,
     0x3feed2ae6489ac1euL, 0x3fc052b0b1a174eauL,
     0x3feedabfc7453e63uL, 0x3fbfe6460fef4680uL,
     0x3feee2a1d004692cuL, 0x3fbf2a901ccafb37uL,
     0x3feeea5557137ae0uL, 0x3fbe723726b824a9uL,
     0x3feef1db32a2277cuL, 0x3fbdbd32ac4c99b0uL,
     0x3feef93436bc2daauL, 0x3fbd0b7a0f921e7cuL,
     0x3fef006135426b26uL, 0x3fbc5d0497c09e74uL,
     0x3fef0762fde45ee6uL, 0x3fbbb1c972f23e50uL,
     0x3fef0e3a5e1a1788uL, 0x3fbb09bfb7d11a84uL,
     0x3fef14e8211e8c55uL, 0x3fba64de673e8837uL,
     0x3fef1b6d0fea5f4duL, 0x3fb9c31c6df3b1b8uL,
     0x3fef21c9f12f0677uL, 0x3fb92470a61b6965uL,
     0x3fef27ff89525acfuL, 0x3fb888d1d8e510a3uL,
     0x3fef2e0e9a6a8b09uL, 0x3fb7f036c0107294uL,
     0x3fef33f7e43a706buL, 0x3fb75a96077274bauL,
     0x3fef39bc242e43e6uL, 0x3fb6c7e64e7281cbuL,
     0x3fef3f5c1558b19euL, 0x3fb6381e2980956buL,
     0x3fef44d870704911uL, 0x3fb5ab342383d178uL,
     0x3fef4a31ebcd47dfuL, 0x3fb5211ebf41880buL,
     0x3fef4f693b67bd77uL, 0x3fb499d478bca735uL,
     0x3fef547f10d60597uL, 0x3fb4154bc68d75c3uL,
     0x3fef59741b4b97cfuL, 0x3fb3937b1b31925auL,
     0x3fef5e4907982a07uL, 0x3fb31458e6542847uL,
     0x3fef62fe80272419uL, 0x3fb297db960e4f63uL,
     0x3fef67952cff6282uL, 0x3fb21df9981f8e53uL,
     0x3fef6c0db3c34641uL, 0x3fb1a6a95b1e786fuL,
     0x3fef7068b7b10fd9uL, 0x3fb131e14fa1625duL,
     0x3fef74a6d9a38383uL, 0x3fb0bf97e95f2a64uL,
     0x3fef78c8b812d498uL, 0x3fb04fc3a0481321uL,
     0x3fef7cceef15d631uL, 0x3fafc4b5e32d6259uL,
     0x3fef80ba18636f07uL, 0x3faeeea8c1b1db94uL,
     0x3fef848acb544e95uL, 0x3fae1d4cf1e2450auL,
     0x3fef88419ce4e184uL, 0x3fad508f9a1ea64fuL,
     0x3fef8bdf1fb78370uL, 0x3fac885df3451a07uL,
     0x3fef8f63e416ebffuL, 0x3fabc4a54a84e834uL,
     0x3fef92d077f8d56duL, 0x3fab055303221015uL,
     0x3fef96256700da8euL, 0x3faa4a549829587euL,
     0x3fef99633a838a57uL, 0x3fa993979e14fffeuL,
     0x3fef9c8a7989af0duL, 0x3fa8e109c4622913uL,
     0x3fef9f9ba8d3c733uL, 0x3fa83298d717210euL,
     0x3fefa2974addae45uL, 0x3fa78832c03aa2b1uL,
     0x3fefa57ddfe27376uL, 0x3fa6e1c5893c380buL,
     0x3fefa84fe5e05c8duL, 0x3fa63f3f5c4de13buL,
     0x3fefab0dd89d1309uL, 0x3fa5a08e85af27e0uL,
     0x3fefadb831a9f9c3uL, 0x3fa505a174e9c929uL,
     0x3fefb04f6868a944uL, 0x3fa46e66be002240uL,
     0x3fefb2d3f20f9101uL, 0x3fa3dacd1a8d8cceuL,
     0x3fefb54641aebbc9uL, 0x3fa34ac36ad8dafeuL,
     0x3fefb7a6c834b5a2uL, 0x3fa2be38b6d92415uL,
     0x3fefb9f5f4739170uL, 0x3fa2351c2f2d1449uL,
     0x3fefbc3433260ca5uL, 0x3fa1af5d2e04f3f6uL,
     0x3fefbe61eef4cf6auL, 0x3fa12ceb37ff9bc3uL,
     0x3fefc07f907bc794uL, 0x3fa0adb5fcfa8c75uL,
     0x3fefc28d7e4f9cd0uL, 0x3fa031ad58d56279uL,
     0x3fefc48c1d033c7auL, 0x3f9f7182a851bca2uL,
     0x3fefc67bcf2d7b8fuL, 0x3f9e85c449e377f3uL,
     0x3fefc85cf56ecd38uL, 0x3f9da0005e5f28dfuL,
     0x3fefca2fee770c79uL, 0x3f9cc0180af00a8buL,
     0x3fefcbf5170b578buL, 0x3f9be5ecd2fcb5f9uL,
     0x3fefcdacca0bfb73uL, 0x3f9b1160991ff737uL,
     0x3fefcf57607a6e7cuL, 0x3f9a4255a00b9f03uL,
     0x3fefd0f5317f582fuL, 0x3f9978ae8b55ce1buL,
     0x3fefd2869270a56fuL, 0x3f98b44e6031383euL,
     0x3fefd40bd6d7a785uL, 0x3f97f5188610ddc8uL,
     0x3fefd58550773cb5uL, 0x3f973af0c737bb45uL,
     0x3fefd6f34f52013auL, 0x3f9685bb5134ef13uL,
     0x3fefd85621b0876duL, 0x3f95d55cb54cd53auL,
     0x3fefd9ae142795e3uL, 0x3f9529b9e8cf9a1euL,
     0x3fefdafb719e6a69uL, 0x3f9482b8455dc491uL,
     0x3fefdc3e835500b3uL, 0x3f93e03d891b37deuL,
     0x3fefdd7790ea5bc0uL, 0x3f93422fd6d12e2buL,
     0x3fefdea6e062d0c9uL, 0x3f92a875b5ffab56uL,
     0x3fefdfccb62e52d3uL, 0x3f9212f612dee7fbuL,
     0x3fefe0e9552ebdd6uL, 0x3f9181983e5133dduL,
     0x3fefe1fcfebe2083uL, 0x3f90f443edc5ce49uL,
     0x3fefe307f2b503d0uL, 0x3f906ae13b0d3255uL,
     0x3fefe40a6f70af4buL, 0x3f8fcab1483ea7fcuL,
     0x3fefe504b1d9696cuL, 0x3f8ec72615a894c4uL,
     0x3fefe5f6f568b301uL, 0x3f8dcaf3691fc448uL,
     0x3fefe6e1742f7cf6uL, 0x3f8cd5ec93c12432uL,
     0x3fefe7c466dc57a1uL, 0x3f8be7e5ac24963buL,
     0x3fefe8a004c19ae6uL, 0x3f8b00b38d6b3575uL,
     0x3fefe97483db8670uL, 0x3f8a202bd6372dceuL,
     0x3fefea4218d6594auL, 0x3f894624e78e0fafuL,
     0x3fefeb08f7146046uL, 0x3f887275e3a6869euL,
     0x3fefebc950b3fa75uL, 0x3f87a4f6aca256cbuL,
     0x3fefec835695932euL, 0x3f86dd7fe3358230uL,
     0x3fefed37386190fbuL, 0x3f861beae53b72b7uL,
     0x3fefede5248e38f4uL, 0x3f856011cc3b036duL,
     0x3fefee8d486585eeuL, 0x3f84a9cf6bda3f4cuL,
     0x3fefef2fd00af31auL, 0x3f83f8ff5042a88euL,
     0x3fefefcce6813974uL, 0x3f834d7dbc76d7e5uL,
     0x3feff064b5afffbeuL, 0x3f82a727a89a3f14uL,
     0x3feff0f766697c76uL, 0x3f8205dac02bd6b9uL,
     0x3feff18520700971uL, 0x3f81697560347b26uL,
     0x3feff20e0a7ba8c2uL, 0x3f80d1d69569b82duL,
     0x3feff2924a3f7a83uL, 0x3f803ede1a45bfeeuL,
     0x3feff312046f2339uL, 0x3f7f60d8aa2a88f2uL,
     0x3feff38d5cc4227fuL, 0x3f7e4cc4abf7d065uL,
     0x3feff404760319b4uL, 0x3f7d4143a9dfe965uL,
     0x3feff47772010262uL, 0x3f7c3e1a5f5c077cuL,
     0x3feff4e671a85425uL, 0x3f7b430ecf4a83a8uL,
     0x3feff55194fe19dfuL, 0x3f7a4fe83fb9db25uL,
     0x3feff5b8fb26f5f6uL, 0x3f79646f35a76624uL,
     0x3feff61cc26c1578uL, 0x3f78806d70b2fc36uL,
     0x3feff67d08401202uL, 0x3f77a3ade6c8b3e5uL,
     0x3feff6d9e943c231uL, 0x3f76cdfcbfc1e263uL,
     0x3feff733814af88cuL, 0x3f75ff2750fe7820uL,
     0x3feff789eb6130c9uL, 0x3f7536fc18f7ce5cuL,
     0x3feff7dd41ce2b4duL, 0x3f74754abacdf1dcuL,
     0x3feff82d9e1a76d8uL, 0x3f73b9e3f9d06e3fuL,
     0x3feff87b1913e853uL, 0x3f730499b503957fuL,
     0x3feff8c5cad200a5uL, 0x3f72553ee2a336bfuL,
     0x3feff90dcaba4096uL, 0x3f71aba78ba3af89uL,
     0x3feff9532f846ab0uL, 0x3f7107a8c7323a6euL,
     0x3feff9960f3eb327uL, 0x3f706918b6355624uL,
     0x3feff9d67f51ddbauL, 0x3f6f9f9cfd9c3035uL,
     0x3feffa14948549a7uL, 0x3f6e77448fb66bb9uL,
     0x3feffa506302ebaeuL, 0x3f6d58da68fd1170uL,
     0x3feffa89fe5b3625uL, 0x3f6c4412bf4b8f0buL,
     0x3feffac17988ef4buL, 0x3f6b38a3af2e55b4uL,
     0x3feffaf6e6f4f5c0uL, 0x3f6a3645330550ffuL,
     0x3feffb2a5879f35euL, 0x3f693cb11a30d765uL,
     0x3feffb5bdf67fe6fuL, 0x3f684ba3004a50d0uL,
     0x3feffb8b8c88295fuL, 0x3f6762d84469c18fuL,
     0x3feffbb970200110uL, 0x3f66821000795a03uL,
     0x3feffbe599f4f9d9uL, 0x3f65a90b00981d93uL,
     0x3feffc10194fcb64uL, 0x3f64d78bba8ca5fduL,
     0x3feffc38fcffbb7cuL, 0x3f640d564548fad7uL,
     0x3feffc60535dd7f5uL, 0x3f634a305080681fuL,
     0x3feffc862a501fd7uL, 0x3f628de11c5031ebuL,
     0x3feffcaa8f4c9beauL, 0x3f61d83170fbf6fbuL,
     0x3feffccd8f5c66d1uL, 0x3f6128eb96be8798uL,
     0x3feffcef371ea4d7uL, 0x3f607fdb4dafea5fuL,
     0x3feffd0f92cb6ba7uL, 0x3f5fb99b8b8279e1uL,
     0x3feffd2eae369a07uL, 0x3f5e7f232d9e2630uL,
     0x3feffd4c94d29fdbuL, 0x3f5d4fed7195d7e8uL,
     0x3feffd6951b33686uL, 0x3f5c2b9cf7f893bfuL,
     0x3feffd84ef9009eeuL, 0x3f5b11d702b3deb2uL,
     0x3feffd9f78c7524auL, 0x3f5a024365f771bduL,
     0x3feffdb8f7605ee7uL, 0x3f58fc8c794b03b5uL,
     0x3feffdd1750e1220uL, 0x3f58005f08d6f1efuL,
     0x3feffde8fb314ebfuL, 0x3f570d6a46e07ddauL,
     0x3feffdff92db56e5uL, 0x3f56235fbd7a4345uL,
     0x3feffe1544d01ccbuL, 0x3f5541f340697987uL,
     0x3feffe2a1988857cuL, 0x3f5468dadf4080abuL,
     0x3feffe3e19349dc7uL, 0x3f5397ced7af2b15uL,
     0x3feffe514bbdc197uL, 0x3f52ce898809244euL,
     0x3feffe63b8c8b5f7uL, 0x3f520cc76202c5fbuL,
     0x3feffe7567b7b5e1uL, 0x3f515246dda49d47uL,
     0x3feffe865fac722buL, 0x3f509ec86c75d497uL,
     0x3feffe96a78a04a9uL, 0x3f4fe41cd9bb4eeeuL,
     0x3feffea645f6d6dauL, 0x3f4e97ba3b77f306uL,
     0x3feffeb5415e7c44uL, 0x3f4d57f524723822uL,
     0x3feffec39ff380b9uL, 0x3f4c245d4b99847auL,
     0x3feffed167b12ac2uL, 0x3f4afc85e0f82e12uL,
     0x3feffede9e5d3262uL, 0x3f49e005769dbc1duL,
     0x3feffeeb49896c6duL, 0x3f48ce75e9f6f8a0uL,
     0x3feffef76e956a9fuL, 0x3f47c7744d9378f7uL,
     0x3fefff0312b010b5uL, 0x3f46caa0d3582fe9uL,
     0x3fefff0e3ad91ec2uL, 0x3f45d79eb71e893buL,
     0x3fefff18ebe2b0e1uL, 0x3f44ee1429bf7cc0uL,
     0x3fefff232a72b48euL, 0x3f440daa3c89f5b6uL,
     0x3fefff2cfb0453d9uL, 0x3f43360ccd23db3auL,
     0x3fefff3661e9569duL, 0x3f4266ea71d4f71auL,
     0x3fefff3f634b79f9uL, 0x3f419ff4663ae9dfuL,
     0x3fefff48032dbe40uL, 0x3f40e0de78654d1euL,
     0x3fefff50456dab8cuL, 0x3f40295ef6591848uL,
     0x3fefff582dc48d30uL, 0x3f3ef25d37f49fe1uL,
     0x3fefff5fbfc8a439uL, 0x3f3da01102b5f851uL,
     0x3fefff66feee5129uL, 0x3f3c5b5412dcafaduL,
     0x3fefff6dee89352euL, 0x3f3b23a5a23e4210uL,
     0x3fefff7491cd4af6uL, 0x3f39f8893d8fd1c1uL,
     0x3fefff7aebcff755uL, 0x3f38d986a4187285uL,
     0x3fefff80ff8911fduL, 0x3f37c629a822bc9euL,
     0x3fefff86cfd3e657uL, 0x3f36be02102b3520uL,
     0x3fefff8c5f702ccfuL, 0x3f35c0a378c90bcauL,
     0x3fefff91b102fca8uL, 0x3f34cda5374ea275uL,
     0x3fefff96c717b695uL, 0x3f33e4a23d1f4703uL,
     0x3fefff9ba420e834uL, 0x3f330538fbb77ecduL,
     0x3fefffa04a7928b1uL, 0x3f322f0b496539beuL,
     0x3fefffa4bc63ee9auL, 0x3f3161be46ad3b50uL,
     0x3fefffa8fc0e5f33uL, 0x3f309cfa445b00ffuL,
     0x3fefffad0b901755uL, 0x3f2fc0d55470cf51uL,
     0x3fefffb0ecebee1buL, 0x3f2e577bbcd49935uL,
     0x3fefffb4a210b172uL, 0x3f2cfd4a5adec5c0uL,
     0x3fefffb82cd9dcbfuL, 0x3f2bb1a9657ce465uL,
     0x3fefffbb8f1049c6uL, 0x3f2a740684026555uL,
     0x3fefffbeca6adbe9uL, 0x3f2943d4a1d1ed39uL,
     0x3fefffc1e08f25f5uL, 0x3f28208bc334a6a5uL,
     0x3fefffc4d3120aa1uL, 0x3f2709a8db59f25cuL,
     0x3fefffc7a37857d2uL, 0x3f25feada379d8b7uL,
     0x3fefffca53375ce3uL, 0x3f24ff207314a102uL,
     0x3fefffcce3b57bffuL, 0x3f240a8c1949f75euL,
     0x3fefffcf564ab6b7uL, 0x3f23207fb7420eb9uL,
     0x3fefffd1ac4135f9uL, 0x3f22408e9ba3327fuL,
     0x3fefffd3e6d5cd87uL, 0x3f216a501f0e42cauL,
     0x3fefffd607387b07uL, 0x3f209d5f819c9e29uL,
     0x3fefffd80e8ce0dauL, 0x3f1fb2b792b40a22uL,
     0x3fefffd9fdeabcceuL, 0x3f1e3bcf436a1a95uL,
     0x3fefffdbd65e5ad0uL, 0x3f1cd55277c18d05uL,
     0x3fefffdd98e903b2uL, 0x3f1b7e94604479dcuL,
     0x3fefffdf46816833uL, 0x3f1a36eec00926dduL,
     0x3fefffe0e0140857uL, 0x3f18fdc1b2dcf7b9uL,
     0x3fefffe26683972auL, 0x3f17d2737527c3f9uL,
     0x3fefffe3daa95b18uL, 0x3f16b4702d7d5849uL,
     0x3fefffe53d558ae9uL, 0x3f15a329b7d30748uL,
     0x3fefffe68f4fa777uL, 0x3f149e17724f4d41uL,
     0x3fefffe7d156d244uL, 0x3f13a4b60ba9aa4euL,
     0x3fefffe904222101uL, 0x3f12b6875310f785uL,
     0x3fefffea2860ee1euL, 0x3f11d312098e9dbauL,
     0x3fefffeb3ebb267buL, 0x3f10f9e1b4dd36dfuL,
     0x3fefffec47d19457uL, 0x3f102a8673a94692uL,
     0x3fefffed443e2787uL, 0x3f0ec929a665b449uL,
     0x3fefffee34943b15uL, 0x3f0d4f4b4c8e09eduL,
     0x3fefffef1960d85duL, 0x3f0be6abbb10a5aauL,
     0x3fefffeff32af7afuL, 0x3f0a8e8cc1fadef6uL,
     0x3feffff0c273bea2uL, 0x3f094637d5bacfdbuL,
     0x3feffff187b6bc0euL, 0x3f080cfdc72220cfuL,
     0x3feffff2436a21dcuL, 0x3f06e2367dc27f95uL,
     0x3feffff2f5fefcaauL, 0x3f05c540b4936fd2uL,
     0x3feffff39fe16963uL, 0x3f04b581b8d170fcuL,
     0x3feffff44178c8d2uL, 0x3f03b2652b06c2b2uL,
     0x3feffff4db27f146uL, 0x3f02bb5cc22e5db6uL,
     0x3feffff56d4d5e5euL, 0x3f01cfe010e2052duL,
     0x3feffff5f8435efcuL, 0x3f00ef6c4c84a0feuL,
     0x3feffff67c604180uL, 0x3f001984165a5f36uL,
     0x3feffff6f9f67e55uL, 0x3efe9b5e8d00ce77uL,
     0x3feffff77154e0d6uL, 0x3efd16f5716c6c1auL,
     0x3feffff7e2c6aea2uL, 0x3efba4f035d60e03uL,
     0x3feffff84e93cd75uL, 0x3efa447b7b03f045uL,
     0x3feffff8b500e77cuL, 0x3ef8f4ccca7fc90duL,
     0x3feffff9164f8e46uL, 0x3ef7b5223dac7336uL,
     0x3feffff972be5c59uL, 0x3ef684c227fcacefuL,
     0x3feffff9ca891572uL, 0x3ef562fac4329b48uL,
     0x3feffffa1de8c582uL, 0x3ef44f21e49054f2uL,
     0x3feffffa6d13de73uL, 0x3ef34894a5e24657uL,
     0x3feffffab83e54b8uL, 0x3ef24eb7254ccf83uL,
     0x3feffffaff99bac4uL, 0x3ef160f438c70913uL,
     0x3feffffb43555b5fuL, 0x3ef07ebd2a2d2844uL,
     0x3feffffb839e52f3uL, 0x3eef4f12e9ab070auL,
     0x3feffffbc09fa7cduL, 0x3eedb5ad0b27805cuL,
     0x3feffffbfa82616buL, 0x3eec304efa2c6f4euL,
     0x3feffffc316d9ed0uL, 0x3eeabe09e9144b5euL,
     0x3feffffc6586abf6uL, 0x3ee95df988e76644uL,
     0x3feffffc96f1165euL, 0x3ee80f439b4ee04buL,
     0x3feffffcc5cec0c1uL, 0x3ee6d11788a69c64uL,
     0x3feffffcf23ff5fcuL, 0x3ee5a2adfa0b4bc4uL,
     0x3feffffd1c637b2buL, 0x3ee4834877429b8fuL,
     0x3feffffd4456a10duL, 0x3ee37231085c7d9auL,
     0x3feffffd6a3554a1uL, 0x3ee26eb9daed6f7euL,
     0x3feffffd8e1a2f22uL, 0x3ee1783ceac28910uL,
     0x3feffffdb01e8546uL, 0x3ee08e1badf0fceduL,
     0x3feffffdd05a75eauL, 0x3edf5f7d88472604uL,
     0x3feffffdeee4f810uL, 0x3eddb92b5212fb8duL,
     0x3feffffe0bd3e852uL, 0x3edc282cd3957edauL,
     0x3feffffe273c15b7uL, 0x3edaab7abace48dcuL,
     0x3feffffe41314e06uL, 0x3ed94219bfcb4928uL,
     0x3feffffe59c6698buL, 0x3ed7eb1a2075864euL,
     0x3feffffe710d565euL, 0x3ed6a597219a93dauL,
     0x3feffffe8717232duL, 0x3ed570b69502f313uL,
     0x3feffffe9bf4098cuL, 0x3ed44ba864670882uL,
     0x3feffffeafb377d5uL, 0x3ed335a62115bce2uL,
     0x3feffffec2641a9euL, 0x3ed22df298214423uL,
     0x3feffffed413e5b7uL, 0x3ed133d96ae7e0dduL,
     0x3feffffee4d01cd6uL, 0x3ed046aeabcfcdecuL,
     0x3feffffef4a55bd4uL, 0x3ececb9cfe1d8642uL,
     0x3fefffff039f9e8fuL, 0x3ecd21397ead99cbuL,
     0x3fefffff11ca4876uL, 0x3ecb8d094c86d374uL,
     0x3fefffff1f302bc1uL, 0x3eca0df0f0c626dcuL,
     0x3fefffff2bdb904duL, 0x3ec8a2e269750a39uL,
     0x3fefffff37d63a36uL, 0x3ec74adc8f4064d3uL,
     0x3fefffff43297019uL, 0x3ec604ea819f007cuL,
     0x3fefffff4dde0118uL, 0x3ec4d0231928c6f9uL,
     0x3fefffff57fc4a95uL, 0x3ec3aba85fe22e20uL,
     0x3fefffff618c3da6uL, 0x3ec296a70f414053uL,
     0x3fefffff6a956450uL, 0x3ec1905613b3abf2uL,
     0x3fefffff731ee681uL, 0x3ec097f6156f32c5uL,
     0x3fefffff7b2f8ed6uL, 0x3ebf59a20caf6695uL,
     0x3fefffff82cdcf1buL, 0x3ebd9c73698fb1dcuL,
     0x3fefffff89ffc4aauL, 0x3ebbf716c6168baeuL,
     0x3fefffff90cb3c81uL, 0x3eba6852c6b58392uL,
     0x3fefffff9735b73buL, 0x3eb8eefd70594a89uL,
     0x3fefffff9d446cccuL, 0x3eb789fb715aae95uL,
     0x3fefffffa2fc5015uL, 0x3eb6383f726a8e04uL,
     0x3fefffffa8621251uL, 0x3eb4f8c96f26a26auL,
     0x3fefffffad7a2652uL, 0x3eb3caa61607f920uL,
     0x3fefffffb248c39duL, 0x3eb2acee2f5ecdb8uL,
     0x3fefffffb6d1e95duL, 0x3eb19ec60b1242eduL,
     0x3fefffffbb196132uL, 0x3eb09f5cf4dd2877uL,
     0x3fefffffbf22c1e2uL, 0x3eaf5bd95d8730d8uL,
     0x3fefffffc2f171e3uL, 0x3ead9371e2ff7c35uL,
     0x3fefffffc688a9cfuL, 0x3eabe41de54d155auL,
     0x3fefffffc9eb76acuL, 0x3eaa4c89e08ef4f3uL,
     0x3fefffffcd1cbc28uL, 0x3ea8cb738399b12cuL,
     0x3fefffffd01f36afuL, 0x3ea75fa8dbc84becuL,
     0x3fefffffd2f57d68uL, 0x3ea608078a70dcbcuL,
     0x3fefffffd5a2041fuL, 0x3ea4c37c0394d094uL,
     0x3fefffffd8271d12uL, 0x3ea39100d5687bfeuL,
     0x3fefffffda86faa9uL, 0x3ea26f9df8519bd7uL,
     0x3fefffffdcc3b117uL, 0x3ea15e6827001f18uL,
     0x3fefffffdedf37eduL, 0x3ea05c803e4831c1uL,
     0x3fefffffe0db6b91uL, 0x3e9ed22548cffd35uL,
     0x3fefffffe2ba0ea5uL, 0x3e9d06ad6ecdf971uL,
     0x3fefffffe47ccb60uL, 0x3e9b551c847fbc96uL,
     0x3fefffffe62534d4uL, 0x3e99bc09f112b494uL,
     0x3fefffffe7b4c81euL, 0x3e983a1ff0aa239duL,
     0x3fefffffe92ced93uL, 0x3e96ce1aa3fd7bdduL,
     0x3fefffffea8ef9cfuL, 0x3e9576c72b514859uL,
     0x3fefffffebdc2ec6uL, 0x3e943302cc4a0da8uL,
     0x3fefffffed15bcbauL, 0x3e9301ba221dc9bbuL,
     0x3fefffffee3cc32cuL, 0x3e91e1e857adc568uL,
     0x3fefffffef5251c2uL, 0x3e90d2966b1746f7uL,
     0x3feffffff0576917uL, 0x3e8fa5b4f49cc6b2uL,
     0x3feffffff14cfb92uL, 0x3e8dc3ae30b55c16uL,
     0x3feffffff233ee1duL, 0x3e8bfd7555a3bd68uL,
     0x3feffffff30d18e8uL, 0x3e8a517d9e61628auL,
     0x3feffffff3d9480fuL, 0x3e88be4f8f6c951fuL,
     0x3feffffff4993c46uL, 0x3e874287ded49339uL,
     0x3feffffff54dab72uL, 0x3e85dcd669f2cd34uL,
     0x3feffffff5f74141uL, 0x3e848bfd38302871uL,
     0x3feffffff6969fb8uL, 0x3e834ecf8a3c124auL,
     0x3feffffff72c5fb6uL, 0x3e822430f521cbcfuL,
     0x3feffffff7b91176uL, 0x3e810b1488aeb235uL,
     0x3feffffff83d3d07uL, 0x3e80027c00a263a6uL,
     0x3feffffff8b962beuL, 0x3e7e12ee004efc37uL,
     0x3feffffff92dfba2uL, 0x3e7c3e44ae32b16buL,
     0x3feffffff99b79d2uL, 0x3e7a854ea14102a8uL,
     0x3feffffffa0248e8uL, 0x3e78e6761569f45duL,
     0x3feffffffa62ce54uL, 0x3e77603bac345f65uL,
     0x3feffffffabd69b4uL, 0x3e75f1353cdad001uL,
     0x3feffffffb127525uL, 0x3e74980cb3c80949uL,
     0x3feffffffb624592uL, 0x3e73537f00b6ad4duL,
     0x3feffffffbad2affuL, 0x3e72225b12bffc68uL,
     0x3feffffffbf370cduL, 0x3e710380e1adb7e9uL,
     0x3feffffffc355dfduL, 0x3e6febc107d5efaauL,
     0x3feffffffc733572uL, 0x3e6df0f2a0ee6947uL,
     0x3feffffffcad3626uL, 0x3e6c14b2188bcee4uL,
     0x3feffffffce39b67uL, 0x3e6a553644f7f07duL,
     0x3feffffffd169d0cuL, 0x3e68b0cfce0579e0uL,
     0x3feffffffd466fa5uL, 0x3e6725e7c5dd20f7uL,
     0x3feffffffd7344aauL, 0x3e65b2fe547a1340uL,
     0x3feffffffd9d4aabuL, 0x3e6456a974e92e93uL,
     0x3feffffffdc4ad7auL, 0x3e630f93c3699078uL,
     0x3feffffffde9964euL, 0x3e61dc7b5b978cf8uL,
     0x3feffffffe0c2bf0uL, 0x3e60bc30c5d52f15uL,
     0x3feffffffe2c92dbuL, 0x3e5f5b2be65a0c7fuL,
     0x3feffffffe4aed5euL, 0x3e5d5f3a8dea7357uL,
     0x3feffffffe675bbduL, 0x3e5b82915b03515buL,
     0x3feffffffe81fc4euL, 0x3e59c3517e789488uL,
     0x3feffffffe9aeb97uL, 0x3e581fb7df06136euL,
     0x3feffffffeb24467uL, 0x3e56961b8d641d06uL,
     0x3feffffffec81ff2uL, 0x3e5524ec4d916caeuL,
     0x3feffffffedc95e7uL, 0x3e53cab1343d18d1uL,
     0x3feffffffeefbc85uL, 0x3e52860757487a01uL,
     0x3fefffffff01a8b6uL, 0x3e5155a09065d4f7uL,
     0x3fefffffff126e1euL, 0x3e50384250e4c9fcuL,
     0x3fefffffff221f30uL, 0x3e4e59890b926c78uL,
     0x3fefffffff30cd3fuL, 0x3e4c642116a8a9e3uL,
     0x3fefffffff3e8892uL, 0x3e4a8e405e651ab6uL,
     0x3fefffffff4b606fuL, 0x3e48d5f98114f872uL,
     0x3fefffffff57632duL, 0x3e47397c5a66e307uL,
     0x3fefffffff629e44uL, 0x3e45b71456c5a4c4uL,
     0x3fefffffff6d1e56uL, 0x3e444d26de513197uL,
     0x3fefffffff76ef3fuL, 0x3e42fa31d6371537uL,
     0x3fefffffff801c1fuL, 0x3e41bcca373b7b43uL,
     0x3fefffffff88af67uL, 0x3e40939ab853339fuL,
     0x3fefffffff90b2e3uL, 0x3e3efac5187b2863uL,
     0x3fefffffff982fc1uL, 0x3e3cf1e86235d0e7uL,
     0x3fefffffff9f2e9fuL, 0x3e3b0a68a2128babuL,
     0x3fefffffffa5b790uL, 0x3e39423165bc4444uL,
     0x3fefffffffabd229uL, 0x3e37974e743dea3duL,
     0x3fefffffffb18582uL, 0x3e3607e9eacd1050uL,
     0x3fefffffffb6d844uL, 0x3e34924a74dec729uL,
     0x3fefffffffbbd0aauL, 0x3e3334d19e0c2160uL,
     0x3fefffffffc0748fuL, 0x3e31edfa3c5f5ccauL,
     0x3fefffffffc4c96cuL, 0x3e30bc56f1b54701uL,
     0x3fefffffffc8d462uL, 0x3e2f3d2185e047d9uL,
     0x3fefffffffcc9a41uL, 0x3e2d26cb87945e87uL,
     0x3fefffffffd01f89uL, 0x3e2b334fac4b9f99uL,
     0x3fefffffffd36871uL, 0x3e296076f7918d1cuL,
     0x3fefffffffd678eduL, 0x3e27ac2d72fc2c63uL,
     0x3fefffffffd954aeuL, 0x3e2614801550319euL,
     0x3fefffffffdbff2auL, 0x3e24979ac8b28927uL,
     0x3fefffffffde7ba0uL, 0x3e2333c68e2d0548uL,
     0x3fefffffffe0cd16uL, 0x3e21e767bce37dd7uL,
     0x3fefffffffe2f664uL, 0x3e20b0fc5b6d05a0uL,
     0x3fefffffffe4fa30uL, 0x3e1f1e3523b41d7duL,
     0x3fefffffffe6daf7uL, 0x3e1d00de6608effeuL,
     0x3fefffffffe89b0cuL, 0x3e1b0778b7b3301buL,
     0x3fefffffffea3c9auL, 0x3e192fb04ec0f6cfuL,
     0x3fefffffffebc1a9uL, 0x3e177756ec9f78fauL,
     0x3fefffffffed2c21uL, 0x3e15dc61922d5a06uL,
     0x3fefffffffee7dc8uL, 0x3e145ce65699ff6duL,
     0x3fefffffffefb847uL, 0x3e12f71a5f159970uL,
     0x3feffffffff0dd2buL, 0x3e11a94ff571654fuL,
     0x3feffffffff1ede9uL, 0x3e1071f4bbea09ecuL,
     0x3feffffffff2ebdauL, 0x3e0e9f1ff8ddd774uL,
     0x3feffffffff3d843uL, 0x3e0c818223a202c7uL,
     0x3feffffffff4b453uL, 0x3e0a887bd2b4404duL,
     0x3feffffffff58126uL, 0x3e08b1a336c5eb6buL,
     0x3feffffffff63fc3uL, 0x3e06fab63324088auL,
     0x3feffffffff6f121uL, 0x3e056197e30205bauL,
     0x3feffffffff79626uL, 0x3e03e44e45301b92uL,
     0x3feffffffff82fabuL, 0x3e0281000bfe4c3fuL,
     0x3feffffffff8be77uL, 0x3e0135f28f2d50b4uL,
     0x3feffffffff94346uL, 0x3e000187dded5975uL,
     0x3feffffffff9bec8uL, 0x3dfdc479de0ef001uL,
     0x3feffffffffa319fuL, 0x3dfbad4fdad3caa1uL,
     0x3feffffffffa9c63uL, 0x3df9baed3ed27ab8uL,
     0x3feffffffffaffa4uL, 0x3df7ead9ce4285bbuL,
     0x3feffffffffb5be5uL, 0x3df63ac6b4edc88euL,
     0x3feffffffffbb1a2uL, 0x3df4a88be2a6390cuL,
     0x3feffffffffc014euL, 0x3df332259185f1a0uL,
     0x3feffffffffc4b56uL, 0x3df1d5b1f3793044uL,
     0x3feffffffffc901cuL, 0x3df0916f04b6e18buL,
     0x3feffffffffccfffuL, 0x3deec77101de6926uL,
     0x3feffffffffd0b56uL, 0x3dec960bf23153e0uL,
     0x3feffffffffd4271uL, 0x3dea8bd20fc65ef7uL,
     0x3feffffffffd759duL, 0x3de8a61745ec7d1duL,
     0x3feffffffffda520uL, 0x3de6e25d0e756261uL,
     0x3feffffffffdd13cuL, 0x3de53e4f7d1666cbuL,
     0x3feffffffffdfa2duL, 0x3de3b7c27a7ddb0euL,
     0x3feffffffffe202duL, 0x3de24caf2c32af14uL,
     0x3feffffffffe4371uL, 0x3de0fb3186804d0fuL,
     0x3feffffffffe642auL, 0x3ddf830c0bb41fd7uL,
     0x3feffffffffe8286uL, 0x3ddd3c0f1a91c846uL,
     0x3feffffffffe9eb0uL, 0x3ddb1e5acf351d87uL,
     0x3feffffffffeb8d0uL, 0x3dd92712d259ce66uL,
     0x3feffffffffed10auL, 0x3dd7538c60a04476uL,
     0x3feffffffffee782uL, 0x3dd5a14b04b47879uL,
     0x3feffffffffefc57uL, 0x3dd40dfd87456f4cuL,
     0x3fefffffffff0fa7uL, 0x3dd2977b1172b9d5uL,
     0x3fefffffffff218fuL, 0x3dd13bc07e891491uL,
     0x3fefffffffff3227uL, 0x3dcff1dbb4300811uL,
     0x3fefffffffff4188uL, 0x3dcd9a880f306bd8uL,
     0x3fefffffffff4fc9uL, 0x3dcb6e45220b55e0uL,
     0x3fefffffffff5cfduL, 0x3dc96a0b33f2c4dauL,
     0x3fefffffffff6939uL, 0x3dc78b07e9e924acuL,
     0x3fefffffffff748euL, 0x3dc5ce9ab1670dd2uL,
     0x3fefffffffff7f0duL, 0x3dc4325167006bb0uL,
     0x3fefffffffff88c5uL, 0x3dc2b3e53538ff3fuL,
     0x3fefffffffff91c6uL, 0x3dc15137a7f44864uL,
     0x3fefffffffff9a1buL, 0x3dc0084ff125639duL,
     0x3fefffffffffa1d2uL, 0x3dbdaeb0b7311ec7uL,
     0x3fefffffffffa8f6uL, 0x3dbb7937d1c40c53uL,
     0x3fefffffffffaf92uL, 0x3db96d082f59ab06uL,
     0x3fefffffffffb5b0uL, 0x3db7872d9fa10aaduL,
     0x3fefffffffffbb58uL, 0x3db5c4e8e37bc7d0uL,
     0x3fefffffffffc095uL, 0x3db423ac0df49a40uL,
     0x3fefffffffffc56duL, 0x3db2a117230ad284uL,
     0x3fefffffffffc9e8uL, 0x3db13af4f04f9998uL,
     0x3fefffffffffce0duL, 0x3dafde703724e560uL,
     0x3fefffffffffd1e1uL, 0x3dad77f0c82e7641uL,
     0x3fefffffffffd56cuL, 0x3dab3ee02611d7dduL,
     0x3fefffffffffd8b3uL, 0x3da92ff33023d5bduL,
     0x3fefffffffffdbbauL, 0x3da7481a9e69f53fuL,
     0x3fefffffffffde86uL, 0x3da5847eda620959uL,
     0x3fefffffffffe11duL, 0x3da3e27c1fcc74bduL,
     0x3fefffffffffe380uL, 0x3da25f9ee0b923dcuL,
     0x3fefffffffffe5b6uL, 0x3da0f9a068653200uL,
     0x3fefffffffffe7c0uL, 0x3d9f5cc7718082b0uL,
     0x3fefffffffffe9a2uL, 0x3d9cf7e53d6a2ca5uL,
     0x3fefffffffffeb60uL, 0x3d9ac0f5f3229372uL,
     0x3fefffffffffecfbuL, 0x3d98b498644847eauL,
     0x3fefffffffffee77uL, 0x3d96cfa9bcca59dcuL,
     0x3fefffffffffefd6uL, 0x3d950f411d4fd2cduL,
     0x3feffffffffff11auL, 0x3d9370ab8327af5euL,
     0x3feffffffffff245uL, 0x3d91f167f88c6b6euL,
     0x3feffffffffff359uL, 0x3d908f24085d4597uL,
     0x3feffffffffff457uL, 0x3d8e8f70e181d61auL,
     0x3feffffffffff542uL, 0x3d8c324c20e337dcuL,
     0x3feffffffffff61buL, 0x3d8a03261574b54euL,
     0x3feffffffffff6e3uL, 0x3d87fe903cdf5855uL,
     0x3feffffffffff79buL, 0x3d86215c58da3450uL,
     0x3feffffffffff845uL, 0x3d846897d4b69fc6uL,
     0x3feffffffffff8e2uL, 0x3d82d1877d731b7buL,
     0x3feffffffffff973uL, 0x3d8159a386b11517uL,
     0x3feffffffffff9f8uL, 0x3d7ffd27ae9393ceuL,
     0x3feffffffffffa73uL, 0x3d7d7c593130dd0buL,
     0x3feffffffffffae4uL, 0x3d7b2cd607c79bcfuL,
     0x3feffffffffffb4cuL, 0x3d790ae4d3405651uL,
     0x3feffffffffffbaduL, 0x3d771312dd1759e2uL,
     0x3feffffffffffc05uL, 0x3d75422ef5d8949duL,
     0x3feffffffffffc57uL, 0x3d739544b0ecc957uL,
     0x3feffffffffffca2uL, 0x3d720997f73e73dduL,
     0x3feffffffffffce7uL, 0x3d709ca0eaacd277uL,
     0x3feffffffffffd27uL, 0x3d6e9810295890ecuL,
     0x3feffffffffffd62uL, 0x3d6c2b45b5aa4a1duL,
     0x3feffffffffffd98uL, 0x3d69eee068fa7596uL,
     0x3feffffffffffdcauL, 0x3d67df2b399c10a8uL,
     0x3feffffffffffdf8uL, 0x3d65f8b87a31bd85uL,
     0x3feffffffffffe22uL, 0x3d64385c96e9a2d9uL,
     0x3feffffffffffe49uL, 0x3d629b2933ef4cbcuL,
     0x3feffffffffffe6cuL, 0x3d611e68a6378f8auL,
     0x3feffffffffffe8duL, 0x3d5f7f338086a86buL,
     0x3feffffffffffeabuL, 0x3d5cf8d7d9ce040auL,
     0x3feffffffffffec7uL, 0x3d5aa577251ae485uL,
     0x3feffffffffffee1uL, 0x3d58811d739efb5fuL,
     0x3feffffffffffef8uL, 0x3d568823e52970beuL,
     0x3fefffffffffff0euL, 0x3d54b72ae68e8b4cuL,
     0x3fefffffffffff22uL, 0x3d530b14dbe876bcuL,
     0x3fefffffffffff34uL, 0x3d5181012ef86610uL,
     0x3fefffffffffff45uL, 0x3d501647ba798745uL,
     0x3fefffffffffff54uL, 0x3d4d90e917701675uL,
     0x3fefffffffffff62uL, 0x3d4b2a87e86d0c8auL,
     0x3fefffffffffff6fuL, 0x3d48f53dcb377293uL,
     0x3fefffffffffff7buL, 0x3d46ed2f2515e933uL,
     0x3fefffffffffff86uL, 0x3d450ecc9ed47f19uL,
     0x3fefffffffffff90uL, 0x3d4356cd5ce7799euL,
     0x3fefffffffffff9auL, 0x3d41c229a587ab78uL,
     0x3fefffffffffffa2uL, 0x3d404e15ecc7f3f6uL,
     0x3fefffffffffffaauL, 0x3d3deffc7e6a6017uL,
     0x3fefffffffffffb1uL, 0x3d3b7b040832f310uL,
     0x3fefffffffffffb8uL, 0x3d3938e021f36d76uL,
     0x3fefffffffffffbeuL, 0x3d37258610b3b233uL,
     0x3fefffffffffffc3uL, 0x3d353d3bfc82a909uL,
     0x3fefffffffffffc8uL, 0x3d337c92babdc2fduL,
     0x3fefffffffffffcduL, 0x3d31e06010120f6auL,
     0x3fefffffffffffd1uL, 0x3d3065b9616170d4uL,
     0x3fefffffffffffd5uL, 0x3d2e13dd96b3753buL,
     0x3fefffffffffffd9uL, 0x3d2b950d32467392uL,
     0x3fefffffffffffdcuL, 0x3d294a72263259a5uL,
     0x3fefffffffffffdfuL, 0x3d272fd93e036cdcuL,
     0x3fefffffffffffe2uL, 0x3d254164576929abuL,
     0x3fefffffffffffe4uL, 0x3d237b83c521fe96uL,
     0x3fefffffffffffe7uL, 0x3d21daf033182e96uL,
     0x3fefffffffffffe9uL, 0x3d205ca50205d26auL,
     0x3fefffffffffffebuL, 0x3d1dfbb6235639fauL,
     0x3fefffffffffffeduL, 0x3d1b7807e294781fuL,
     0x3fefffffffffffeeuL, 0x3d19298add70a734uL,
     0x3feffffffffffff0uL, 0x3d170beaf9c7ffb6uL,
     0x3feffffffffffff1uL, 0x3d151b2cd6709222uL,
     0x3feffffffffffff3uL, 0x3d1353a6cf7f7fffuL,
     0x3feffffffffffff4uL, 0x3d11b1fa8cbe84a7uL,
     0x3feffffffffffff5uL, 0x3d10330f0fd69921uL,
     0x3feffffffffffff6uL, 0x3d0da81670f96f9buL,
     0x3feffffffffffff7uL, 0x3d0b24a16b4d09aauL,
     0x3feffffffffffff7uL, 0x3d08d6eeb6efdbd6uL,
     0x3feffffffffffff8uL, 0x3d06ba91ac734786uL,
     0x3feffffffffffff9uL, 0x3d04cb7966770ab5uL,
     0x3feffffffffffff9uL, 0x3d0305e9721d0981uL,
     0x3feffffffffffffauL, 0x3d01667311fff70auL,
     0x3feffffffffffffbuL, 0x3cffd3de10d62855uL,
     0x3feffffffffffffbuL, 0x3cfd1aefbcd48d0cuL,
     0x3feffffffffffffbuL, 0x3cfa9cc93c25aca9uL,
     0x3feffffffffffffcuL, 0x3cf85487ee3ea735uL,
     0x3feffffffffffffcuL, 0x3cf63daf8b4b1e0cuL,
     0x3feffffffffffffduL, 0x3cf45421e69a6ca1uL,
     0x3feffffffffffffduL, 0x3cf294175802d99auL,
     0x3feffffffffffffduL, 0x3cf0fa17bf41068fuL,
     0x3feffffffffffffduL, 0x3cef05e82aae2bb9uL,
     0x3feffffffffffffeuL, 0x3cec578101b29058uL,
     0x3feffffffffffffeuL, 0x3ce9e39dc5dd2f7cuL,
     0x3feffffffffffffeuL, 0x3ce7a553a728bbf2uL,
     0x3feffffffffffffeuL, 0x3ce5982008db1304uL,
     0x3feffffffffffffeuL, 0x3ce3b7e00422e51buL,
     0x3feffffffffffffeuL, 0x3ce200c898d9ee3euL,
     0x3fefffffffffffffuL, 0x3ce06f5f7eb65a56uL,
     0x3fefffffffffffffuL, 0x3cde00e9148a1d25uL,
     0x3fefffffffffffffuL, 0x3cdb623734024e92uL,
     0x3fefffffffffffffuL, 0x3cd8fd4e01891bf8uL,
     0x3fefffffffffffffuL, 0x3cd6cd44c7470d89uL,
     0x3fefffffffffffffuL, 0x3cd4cd9c04158cd7uL,
     0x3fefffffffffffffuL, 0x3cd2fa34bf5c8344uL,
     0x3fefffffffffffffuL, 0x3cd14f4890ff2461uL,
     0x3fefffffffffffffuL, 0x3ccf92c49dfa4df5uL,
     0x3fefffffffffffffuL, 0x3ccccaaea71ab0dfuL,
     0x3fefffffffffffffuL, 0x3cca40829f001197uL,
     0x3ff0000000000000uL, 0x3cc7eef13b59e96cuL,
     0x3ff0000000000000uL, 0x3cc5d11e1a252bf5uL,
     0x3ff0000000000000uL, 0x3cc3e296303b2297uL,
     0x3ff0000000000000uL, 0x3cc21f47009f43ceuL,
     0x3ff0000000000000uL, 0x3cc083768c5e4542uL,
     0x3ff0000000000000uL, 0x3cbe1777d831265fuL,
     0x3ff0000000000000uL, 0x3cbb69f10b0191b5uL,
     0x3ff0000000000000uL, 0x3cb8f8a3a05b5b53uL,
     0x3ff0000000000000uL, 0x3cb6be573c40c8e7uL,
     0x3ff0000000000000uL, 0x3cb4b645ba991fdbuL,

     },

    0x7fffffffffffffffuL,
    0x4017f80000000000uL,
    0x42c0000000000000uL,
    0x2ff0000000000000uL,
    0x8000000000000000uL,
    0x3ff0000000000000uL,

    0xbfa6c16db05bdea5uL,
    0x3fc1111235a363b1uL,
    0x3fcc71ca1c71eb57uL,
    0xbfd9999c2be2dda8uL,
    0xbfc5555800001B4FuL,
    0x3fb9999E2BE2F122uL,
    0xbfd55555555547f6uL,
    0x3fdfffffffffd4cduL,

    0x3fe5555555554b0cuL,
    0xbfd5555555555555uL,
    0xbff0000000000000uL,
    0x3ff0000000000000uL,

    0x00000000ffffffffuL,

    0x3fe55557bbbbdca6uL,
    0xbff0000180001429uL,
    0x3fe0000400005666uL,
    0xbfd5555c88892f64uL,

    0x4030b4ecdf6f65b6uL,
    0x3ff20dd74f71eb5duL,
    0xbfd81273f8ddb7ffuL,
    0x3fbce2e7629aab0cuL,
    0xbf9b826598001be1uL,
    0x3f7563976c008b46uL,
    0xbf4bf4e377408696uL,
    0x3f1f5cf0eb393d76uL,
    0xbeee8de69027d2a9uL,
    0x3eb9e4a472519e7buL,
    0xbe82ee5c00edba27uL,
    0x3e477d06b482dd68uL,
    0xbe082bdef333116fuL,
    0x3dc411ac8394bd60uL,
    0xbd7a0044c6d0f311uL,
    0x3d2925f1398f6ecbuL,
    0xbcd0ffde0775e81duL,
    0x3c6c8e5e1655f4fauL,
    0xbbf65fcb17003d22uL,

};

double __ocl_svml_erf (double a)
{

    double va1;
    double vr1;
    unsigned int vm;

    double r;

    va1 = a;;

    {

        double _AbsMask;
        double _MaxThreshold;
        double _SRound;

        double _TwoM9;
        double _poly1_0;
        double _poly1_1;
        double _poly3_0;
        double _poly3_1;
        double _poly5_0;
        double _poly5_1;
        double _poly1_2;
        double _poly3_2;
        double _poly5_2;
        double _poly1_3;
        double _poly3_3;
        double _poly1_4;
        double _poly3_4;
        double _poly1_5;
        double _poly1_6;

        double Sign;
        double X;
        double Xa;
        double X0;
        double T;
        double Diff;
        unsigned long Index;

        double P1;
        double P3;
        double P5;
        double D2;
        double THL[2];
        double _Erf_H;
        double _Exp_H_D;
        double T2;
        unsigned long _Mask32;
        _AbsMask = as_double (__internal_derf_la_data._AbsMask);
        Xa = as_double ((as_ulong (va1) & as_ulong (_AbsMask)));

        Sign = as_double ((as_ulong (Xa) ^ as_ulong (va1)));

        _MaxThreshold = as_double (__internal_derf_la_data._MaxThreshold);
        X = ((Xa < _MaxThreshold) ? Xa : _MaxThreshold);

        {
            double _U2Threshold;
            double UMask;
            double dIndex;
            _SRound = as_double (__internal_derf_la_data._SRound);
            dIndex = (X + _SRound);
            _U2Threshold = as_double (__internal_derf_la_data._U2Threshold);
            UMask = as_double ((unsigned long) ((X > _U2Threshold) ? 0xffffffffffffffff : 0x0));
            X0 = (dIndex - _SRound);
            Diff = (X - X0);
            T = (X0 * Diff);
            D2 = as_double ((as_ulong (Diff) & as_ulong (UMask)));
            Index = as_ulong (dIndex);
            Index = ((unsigned long) (Index) << (4));
        };

        _Mask32 = (__internal_derf_la_data._Mask32);
        Index = (Index & _Mask32);

        THL[0] = as_double (((__constant unsigned long *) ((__constant double *) (&__internal_derf_la_data._erf_tbl[0])))[Index >> 3]);
        THL[1] = as_double (((__constant unsigned long *) ((__constant double *) (&__internal_derf_la_data._erf_tbl[0])))[(Index >> 3) + 1]);

        Diff = ((Diff < Xa) ? Diff : Xa);

        D2 = (D2 * D2);

        Diff = as_double ((as_ulong (Diff) ^ as_ulong (Sign)));

        _poly1_0 = as_double (__internal_derf_la_data._poly1_0);
        _poly1_1 = as_double (__internal_derf_la_data._poly1_1);
        P1 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (_poly1_0, T, _poly1_1);
        _poly3_0 = as_double (__internal_derf_la_data._poly3_0);
        _poly3_1 = as_double (__internal_derf_la_data._poly3_1);
        P3 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (_poly3_0, T, _poly3_1);
        _poly5_0 = as_double (__internal_derf_la_data._poly5_0);
        _poly5_1 = as_double (__internal_derf_la_data._poly5_1);
        P5 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (_poly5_0, T, _poly5_1);

        _poly1_2 = as_double (__internal_derf_la_data._poly1_2);
        P1 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (P1, T, _poly1_2);
        _poly3_2 = as_double (__internal_derf_la_data._poly3_2);
        P3 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (P3, T, _poly3_2);
        _poly3_3 = as_double (__internal_derf_la_data._poly3_3);
        _poly3_3 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (P5, D2, _poly3_3);

        T2 = (T * T);

        _poly1_3 = as_double (__internal_derf_la_data._poly1_3);
        P1 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (P1, T, _poly1_3);
        P3 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (P3, T, _poly3_3);

        _Erf_H = as_double ((as_ulong (THL[0]) ^ as_ulong (Sign)));

        P1 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (P1, T2, -(T));

        _Exp_H_D = (THL[1] * Diff);

        P1 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (P3, D2, P1);
        vm = 0;
        P1 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (P1, _Exp_H_D, _Exp_H_D);

        vr1 = (P1 + _Erf_H);
        vr1 = as_double ((as_ulong (vr1) | as_ulong (Sign)));
        {
            double _dIsNan;
            _dIsNan = as_double ((unsigned long) (((va1 != va1) | (va1 != va1)) ? 0xffffffffffffffff : 0x0));
            vr1 = as_double ((((~as_ulong (_dIsNan)) & as_ulong (vr1)) | (as_ulong (_dIsNan) & as_ulong (va1))));
        }
    }

    ;
    r = vr1;;

    return r;

}

/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF
typedef struct
{
    unsigned long Log_tbl[16];
    unsigned long One;
    unsigned long C075;
    unsigned long poly_coeff9;
    unsigned long poly_coeff8;
    unsigned long poly_coeff7;
    unsigned long poly_coeff6;
    unsigned long poly_coeff5;
    unsigned long poly_coeff4;
    unsigned long poly_coeff3;
    unsigned long poly_coeff2;
    unsigned long poly_coeff1;
    unsigned long Zero;
    unsigned long IndexMask;
    unsigned long MinNorm;
    unsigned long MaxNorm;
} __internal_dlog2_la_data_avx512_t;
static __constant __internal_dlog2_la_data_avx512_t __internal_dlog2_la_data_avx512 = {
    {
     0x0000000000000000uL, 0xbfb663f6fac91316uL, 0xbfc5c01a39fbd688uL, 0xbfcfbc16b902680auL, 0xbfd49a784bcd1b8buL, 0xbfd91bba891f1709uL,
     0xbfdd6753e032ea0fuL, 0xbfe0c10500d63aa6uL, 0x3fda8ff971810a5euL, 0x3fd6cb0f6865c8eauL, 0x3fd32bfee370ee68uL, 0x3fcf5fd8a9063e35uL,
     0x3fc8a8980abfbd32uL, 0x3fc22dadc2ab3497uL, 0x3fb7d60496cfbb4cuL, 0x3fa77394c9d958d5uL}

    , 0x3ff0000000000000uL, 0x3fe8000000000000uL, 0x3fc4904bda0e1d12uL, 0xbfc71fb84deb5cceuL, 0x3fca617351818613uL, 0xbfcec707e4e3144cuL,
        0x3fd2776c5114d91auL, 0xbfd71547653d0f8duL, 0x3fdec709dc3a029fuL, 0xbfe71547652b82d4uL, 0x3ff71547652b82feuL, 0x0000000000000000uL,
        0x0000000000000078uL, 0x0010000000000000uL, 0x7fefffffffffffffuL
};

typedef struct
{
    unsigned long Log_HA_table[(1 << 10) + 2];
    unsigned long Log_LA_table[(1 << 9) + 1];
    unsigned long ha_poly_coeff[6];
    unsigned long poly_coeff[5];
    unsigned long ExpMask;
    unsigned long Two9;
    unsigned long Two10;
    unsigned long MinNorm;
    unsigned long MaxNorm;
    unsigned long HalfMask;
    unsigned long CLH;
    unsigned long One;
    unsigned long Threshold;
    unsigned long Bias;
    unsigned long Bias1;

    unsigned long dInfs[2];
    unsigned long dOnes[2];
    unsigned long dZeros[2];
} __internal_dlog2_la_data_t;
static __constant __internal_dlog2_la_data_t __internal_dlog2_la_data = {

    {
     0xc08ff00000000000uL, 0x0000000000000000uL, 0xc08ff0040038c920uL, 0x3d52bfc81744e999uL, 0xc08ff007ff0f0190uL, 0xbd59b2cedc63c895uL,
     0xc08ff00bfc839e88uL, 0xbd28e365e6741d71uL, 0xc08ff00ff8979428uL, 0x3d4027998f69a77duL, 0xc08ff013f34bd5a0uL, 0x3d5dd2cb33fe6a89uL,
     0xc08ff017eca15518uL, 0xbd526514cdf2c019uL, 0xc08ff01be49903d8uL, 0xbd44bfeeba165e04uL, 0xc08ff01fdb33d218uL, 0xbd3fa79ee110cec3uL,
     0xc08ff023d072af20uL, 0xbd4eebb642c7fd60uL, 0xc08ff027c4568948uL, 0x3d429b13d7093443uL, 0xc08ff02bb6e04de8uL, 0x3d50f346bd36551euL,
     0xc08ff02fa810e968uL, 0xbd5020bb662f1536uL, 0xc08ff03397e94750uL, 0x3d5de76b56340995uL, 0xc08ff037866a5218uL, 0x3d58065ff3304090uL,
     0xc08ff03b7394f360uL, 0x3d561fc9322fb785uL, 0xc08ff03f5f6a13d0uL, 0x3d0abecd17d0d778uL, 0xc08ff04349ea9b28uL, 0xbd588f3ad0ce4d44uL,
     0xc08ff04733177040uL, 0xbd4454ba4ac5f44duL, 0xc08ff04b1af178f8uL, 0xbd556f78faaa0887uL, 0xc08ff04f01799a58uL, 0x3d49db8976de7469uL,
     0xc08ff052e6b0b868uL, 0xbd5cdb6fce17ef00uL, 0xc08ff056ca97b668uL, 0xbd576de8c0412f09uL, 0xc08ff05aad2f76a0uL, 0x3d30142c7ec6475cuL,
     0xc08ff05e8e78da70uL, 0xbd1e685afc26de72uL, 0xc08ff0626e74c260uL, 0xbd40b64c954078a3uL, 0xc08ff0664d240e10uL, 0xbd5fcde393462d7duL,
     0xc08ff06a2a879c48uL, 0xbd537245eeeecc53uL, 0xc08ff06e06a04ae8uL, 0x3d4ac306eb47b436uL, 0xc08ff071e16ef6e8uL, 0xbd5a1fd9d3758f6buL,
     0xc08ff075baf47c80uL, 0x3d2401fbaaa67e3cuL, 0xc08ff0799331b6f0uL, 0x3d4f8dbef47a4d53uL, 0xc08ff07d6a2780a8uL, 0x3d51215e0abb42d1uL,
     0xc08ff0813fd6b340uL, 0x3d57ce6249eddb35uL, 0xc08ff08514402770uL, 0xbd38a803c7083a25uL, 0xc08ff088e764b528uL, 0x3d42218beba5073euL,
     0xc08ff08cb9453370uL, 0x3d447b66f1c6248fuL, 0xc08ff09089e27880uL, 0xbd53d9297847e995uL, 0xc08ff094593d59c8uL, 0xbd12b6979cc77aa9uL,
     0xc08ff0982756abd0uL, 0xbd55308545ecd702uL, 0xc08ff09bf42f4260uL, 0xbd578fa97c3b936fuL, 0xc08ff09fbfc7f068uL, 0xbd41828408ce869duL,
     0xc08ff0a38a218808uL, 0x3d555da6ce7251a6uL, 0xc08ff0a7533cda88uL, 0xbd41f3cd14bfcb02uL, 0xc08ff0ab1b1ab878uL, 0xbd1f028da6bf1852uL,
     0xc08ff0aee1bbf188uL, 0xbd4cf04de3267f54uL, 0xc08ff0b2a72154a8uL, 0xbd4556e47019db10uL, 0xc08ff0b66b4baff8uL, 0x3d1e7ba00b15fbe4uL,
     0xc08ff0ba2e3bd0d0uL, 0x3d5bfde1c52c2f28uL, 0xc08ff0bdeff283b8uL, 0x3d48d63fe20ee5d6uL, 0xc08ff0c1b0709480uL, 0x3d57f551980838ffuL,
     0xc08ff0c56fb6ce20uL, 0xbd4189091f293c81uL, 0xc08ff0c92dc5fae0uL, 0x3d4d549f05f06169uL, 0xc08ff0ccea9ee428uL, 0xbd5982466074e1e3uL,
     0xc08ff0d0a64252b8uL, 0xbd5d30a6b16c0e4buL, 0xc08ff0d460b10e80uL, 0xbd3138bf3b51a201uL, 0xc08ff0d819ebdea8uL, 0xbd454e680c0801d6uL,
     0xc08ff0dbd1f389a8uL, 0x3d584db361385926uL, 0xc08ff0df88c8d520uL, 0xbd564f2252a82c03uL, 0xc08ff0e33e6c8610uL, 0xbd5c78c35ed5d034uL,
     0xc08ff0e6f2df60a8uL, 0xbd52eb9f29ca3d75uL, 0xc08ff0eaa6222860uL, 0x3d5340c0c01b5ff8uL, 0xc08ff0ee58359fe8uL, 0x3d10c2acaffa64b6uL,
     0xc08ff0f2091a8948uL, 0xbd3fced311301ebeuL, 0xc08ff0f5b8d1a5c8uL, 0x3d41ee5d591af30buL, 0xc08ff0f9675bb5f0uL, 0x3d4873546b0e668cuL,
     0xc08ff0fd14b97998uL, 0x3d5a99928177a119uL, 0xc08ff100c0ebafd8uL, 0x3d378ead132adcacuL, 0xc08ff1046bf31720uL, 0x3d51a538bc597d48uL,
     0xc08ff10815d06d18uL, 0xbd540ee2f35efd7euL, 0xc08ff10bbe846ec8uL, 0xbd59cf94753adaccuL, 0xc08ff10f660fd878uL, 0xbd5201a3d6862895uL,
     0xc08ff1130c7365c0uL, 0x3d383e25d0822d03uL, 0xc08ff116b1afd180uL, 0xbd0b7389bbea8f7buL, 0xc08ff11a55c5d5f0uL, 0xbd4df278087a6617uL,
     0xc08ff11df8b62c98uL, 0xbd48daeb8ec01e26uL, 0xc08ff1219a818e50uL, 0x3d57c9312e0a14dauL, 0xc08ff1253b28b330uL, 0xbd5f0fbc0e4d507euL,
     0xc08ff128daac52c8uL, 0xbd222afdee008687uL, 0xc08ff12c790d23d8uL, 0x3d17c71747bcef8buL, 0xc08ff130164bdc88uL, 0x3d5d69cfd051af50uL,
     0xc08ff133b2693248uL, 0x3d59dff064e9433auL, 0xc08ff1374d65d9e8uL, 0x3d4f71a30db3240buL, 0xc08ff13ae7428788uL, 0xbd5e56afa9524606uL,
     0xc08ff13e7fffeeb0uL, 0xbd44acd84e6f8518uL, 0xc08ff142179ec228uL, 0xbd519845ade5e121uL, 0xc08ff145ae1fb420uL, 0xbd5b3b4a38ddec70uL,
     0xc08ff14943837620uL, 0xbd5ea4bb5bc137c7uL, 0xc08ff14cd7cab910uL, 0x3d5610f3bf8eb6ceuL, 0xc08ff1506af62d20uL, 0x3d57b1170d6184cfuL,
     0xc08ff153fd0681f0uL, 0x3d5791a688a3660euL, 0xc08ff1578dfc6678uL, 0x3d5d41ecf8abac2euL, 0xc08ff15b1dd88908uL, 0x3cf0bd995d64d573uL,
     0xc08ff15eac9b9758uL, 0xbd5e3653cd796d01uL, 0xc08ff1623a463e80uL, 0xbd597573005ef2d8uL, 0xc08ff165c6d92af0uL, 0xbd4ee222d6439c41uL,
     0xc08ff16952550880uL, 0x3d5913b845e75950uL, 0xc08ff16cdcba8258uL, 0xbd558e7ba239077euL, 0xc08ff170660a4328uL, 0x3d5a0e174a2cae66uL,
     0xc08ff173ee44f4d8uL, 0x3d22b8db103db712uL, 0xc08ff177756b40d8uL, 0x3d5cc610480853c4uL, 0xc08ff17afb7dcfe0uL, 0xbd304a8bc84e5c0fuL,
     0xc08ff17e807d4a28uL, 0x3d3639d185da5f7duL, 0xc08ff182046a5738uL, 0xbd534705d06d788fuL, 0xc08ff18587459e10uL, 0xbd540d25b28a51fduL,
     0xc08ff189090fc510uL, 0xbd02d804afa7080auL, 0xc08ff18c89c97200uL, 0x3d5f2a5d305818bauL, 0xc08ff19009734a08uL, 0xbd3a602e9d05c3e4uL,
     0xc08ff193880df1d0uL, 0xbd533d6fdcd54875uL, 0xc08ff197059a0d60uL, 0x3d24eaf0a9490202uL, 0xc08ff19a82184020uL, 0xbd5685666d98eb59uL,
     0xc08ff19dfd892cf8uL, 0xbd509f8745f0868buL, 0xc08ff1a177ed7630uL, 0xbd2dcba340a9d268uL, 0xc08ff1a4f145bd80uL, 0x3d4916fcd0331266uL,
     0xc08ff1a86992a408uL, 0xbd548cd033a49073uL, 0xc08ff1abe0d4ca68uL, 0xbd5252f40e5df1a2uL, 0xc08ff1af570cd0a0uL, 0xbd541d623bd02248uL,
     0xc08ff1b2cc3b5628uL, 0xbd258dc48235c071uL, 0xc08ff1b64060f9e0uL, 0xbd4b4bd8f02ed3f2uL, 0xc08ff1b9b37e5a28uL, 0x3d4e8d20a88cd0a2uL,
     0xc08ff1bd259414c0uL, 0x3d3b669b6380bc55uL, 0xc08ff1c096a2c6e8uL, 0xbd45d54159d51094uL, 0xc08ff1c406ab0d58uL, 0x3d59f684ffbca44duL,
     0xc08ff1c775ad8428uL, 0x3d543b1b1d508399uL, 0xc08ff1cae3aac6f8uL, 0x3d5c30953a12fc6euL, 0xc08ff1ce50a370d0uL, 0xbd1763b04f9aad5fuL,
     0xc08ff1d1bc981c40uL, 0x3d573c6fa54f46c2uL, 0xc08ff1d527896338uL, 0x3d48ccfb9ffd7455uL, 0xc08ff1d89177df30uL, 0x3d42756f80d6f7ceuL,
     0xc08ff1dbfa642910uL, 0xbd3c2bfbc353c5a5uL, 0xc08ff1df624ed940uL, 0x3d1d6064f5dc380buL, 0xc08ff1e2c9388798uL, 0x3ce327c6b30711cfuL,
     0xc08ff1e62f21cb70uL, 0x3d140aa9546525bcuL, 0xc08ff1e9940b3b98uL, 0xbd15c1ff43c21863uL, 0xc08ff1ecf7f56e60uL, 0x3d590ba680120498uL,
     0xc08ff1f05ae0f988uL, 0x3d5390c6b62dff50uL, 0xc08ff1f3bcce7258uL, 0x3d4da0c90878457fuL, 0xc08ff1f71dbe6d90uL, 0x3d30697edc85b98cuL,
     0xc08ff1fa7db17f70uL, 0x3d04d81188510a79uL, 0xc08ff1fddca83bb0uL, 0xbd5f2ddc983ce25cuL, 0xc08ff2013aa33598uL, 0x3d46c22f0fae6844uL,
     0xc08ff20497a2ffd0uL, 0xbd53359b714c3d03uL, 0xc08ff207f3a82ca0uL, 0xbd4aefaa5524f88buL, 0xc08ff20b4eb34dc0uL, 0x3d39bf4a4a73d01duL,
     0xc08ff20ea8c4f468uL, 0x3d44217befdb12e6uL, 0xc08ff21201ddb158uL, 0x3d5219b281d4b6f8uL, 0xc08ff21559fe14c8uL, 0xbd5e3b123373d370uL,
     0xc08ff218b126ae88uL, 0xbd59b525a6edc3cbuL, 0xc08ff21c07580dd8uL, 0xbd4b494e7737c4dcuL, 0xc08ff21f5c92c180uL, 0xbd3989b7d67e3e54uL,
     0xc08ff222b0d757d0uL, 0x3d486c8f098ad3cfuL, 0xc08ff22604265e98uL, 0x3d5254956d8e15b2uL, 0xc08ff22956806330uL, 0x3d3f14730a362959uL,
     0xc08ff22ca7e5f278uL, 0xbd40e8ed02e32ea1uL, 0xc08ff22ff85798d8uL, 0xbd40fb2b9b1e0261uL, 0xc08ff23347d5e238uL, 0xbd5bfeb1e13c8bc3uL,
     0xc08ff23696615a18uL, 0x3d5b891f041e037buL, 0xc08ff239e3fa8b60uL, 0xbd36255027582bb9uL, 0xc08ff23d30a200a8uL, 0x3d56bb5a92a55361uL,
     0xc08ff2407c5843f0uL, 0xbd31902fb4417244uL, 0xc08ff243c71dded8uL, 0xbd5a8a7c3c4a2cc6uL, 0xc08ff24710f35a88uL, 0xbd23be1be6941016uL,
     0xc08ff24a59d93fa8uL, 0x3d55c85afafa1d46uL, 0xc08ff24da1d01668uL, 0xbd5b4b05a0adcbf1uL, 0xc08ff250e8d866a0uL, 0x3d134d191476f74buL,
     0xc08ff2542ef2b798uL, 0x3d5e78ce963395e1uL, 0xc08ff257741f9028uL, 0x3d3f9219a8f57c17uL, 0xc08ff25ab85f76c8uL, 0x3d5cfc6f47ac691buL,
     0xc08ff25dfbb2f168uL, 0x3d4ab3b720b5ca71uL, 0xc08ff2613e1a8598uL, 0x3d54a4ab99feb71auL, 0xc08ff2647f96b868uL, 0xbd42daa69d79d724uL,
     0xc08ff267c0280e88uL, 0xbd344d9115018f45uL, 0xc08ff26affcf0c28uL, 0xbd56673e143d2ac0uL, 0xc08ff26e3e8c3518uL, 0x3d3aac889e91c638uL,
     0xc08ff2717c600ca8uL, 0x3d4cf65b41d006e7uL, 0xc08ff274b94b15c0uL, 0xbd4c821320391e76uL, 0xc08ff277f54dd2e8uL, 0x3d51abd6e2ddc2a1uL,
     0xc08ff27b3068c620uL, 0xbd2f1bdd1264e703uL, 0xc08ff27e6a9c7110uL, 0xbd58437b4f032f15uL, 0xc08ff281a3e954f0uL, 0xbd4f8e063b069a7duL,
     0xc08ff284dc4ff288uL, 0x3d5276d0723a662auL, 0xc08ff28813d0ca28uL, 0xbd5731f7c6d8f6ebuL, 0xc08ff28b4a6c5bd0uL, 0xbd58b587f08307ecuL,
     0xc08ff28e80232708uL, 0x3d57f19a7a352bafuL, 0xc08ff291b4f5aae0uL, 0x3d570d99aff32790uL, 0xc08ff294e8e46610uL, 0x3d4efafaad4f59dbuL,
     0xc08ff2981befd6e0uL, 0xbd41eb1728371564uL, 0xc08ff29b4e187b38uL, 0x3d458465b4e080d7uL, 0xc08ff29e7f5ed088uL, 0x3d46acb4a035a820uL,
     0xc08ff2a1afc353e0uL, 0xbd39fc68238dd5d3uL, 0xc08ff2a4df4681f0uL, 0x3d526d90c6750ddeuL, 0xc08ff2a80de8d6f0uL, 0x3d48505c598278fduL,
     0xc08ff2ab3baacec0uL, 0x3d520fece8e148e8uL, 0xc08ff2ae688ce4d0uL, 0x3d14f7bf38646243uL, 0xc08ff2b1948f9430uL, 0xbd5aa5f693a627dfuL,
     0xc08ff2b4bfb35790uL, 0xbd4725d8e6280861uL, 0xc08ff2b7e9f8a930uL, 0x3d482e0765d44bdauL, 0xc08ff2bb136002e8uL, 0xbd523d745da75cdeuL,
     0xc08ff2be3be9de40uL, 0xbd32e50b4191ef73uL, 0xc08ff2c16396b448uL, 0xbd490856dfe073b2uL, 0xc08ff2c48a66fdb8uL, 0xbd512b526137db4duL,
     0xc08ff2c7b05b32e8uL, 0x3d5bfcdc71b36585uL, 0xc08ff2cad573cbb8uL, 0xbd2c24f2afddb377uL, 0xc08ff2cdf9b13fc0uL, 0xbd5ea60d06da12f6uL,
     0xc08ff2d11d140630uL, 0xbd582f2f9e256dc5uL, 0xc08ff2d43f9c95d0uL, 0xbd4411c269523864uL, 0xc08ff2d7614b6508uL, 0xbd41107eeb7e1093uL,
     0xc08ff2da8220e9e8uL, 0x3d5a4aa491710edauL, 0xc08ff2dda21d9a10uL, 0x3d46e50a14550378uL, 0xc08ff2e0c141ead0uL, 0xbd4881e3bd846de9uL,
     0xc08ff2e3df8e5118uL, 0xbd46d93437bd399duL, 0xc08ff2e6fd034170uL, 0xbd5b4ef1e9713a4cuL, 0xc08ff2ea19a13010uL, 0x3d4a0e31ed25b3efuL,
     0xc08ff2ed356890b8uL, 0xbd5a7a560db90113uL, 0xc08ff2f05059d6f0uL, 0x3d51f5bb5f9072c9uL, 0xc08ff2f36a7575c0uL, 0x3d5ed5225350a585uL,
     0xc08ff2f683bbdfe0uL, 0xbd1c9363d9e745dbuL, 0xc08ff2f99c2d87b8uL, 0x3d329c788e376e0duL, 0xc08ff2fcb3cadf40uL, 0xbd59eb5d29918de0uL,
     0xc08ff2ffca945828uL, 0xbd4a86aac097a06buL, 0xc08ff302e08a63b8uL, 0x3d541c2c97e8b4d1uL, 0xc08ff305f5ad72d8uL, 0x3d43c95dec31821buL,
     0xc08ff30909fdf620uL, 0xbd590abed3d72738uL, 0xc08ff30c1d7c5dd8uL, 0x3d4caefdad90e913uL, 0xc08ff30f302919d0uL, 0xbd4f7ed5e1dcb170uL,
     0xc08ff312420499a0uL, 0x3d3c590edf8c3407uL, 0xc08ff315530f4c70uL, 0x3d5477d46ce838e1uL, 0xc08ff3186349a118uL, 0x3d5e4b00c511fa78uL,
     0xc08ff31b72b40610uL, 0xbd54333e5a0c1658uL, 0xc08ff31e814ee990uL, 0x3d25300b88bfa10auL, 0xc08ff3218f1ab958uL, 0xbd5bfbd520249ed7uL,
     0xc08ff3249c17e2f0uL, 0x3d436b1cdba645b7uL, 0xc08ff327a846d368uL, 0xbd5cb667c2f86eaauL, 0xc08ff32ab3a7f7a0uL, 0x3d5334d06a920d5fuL,
     0xc08ff32dbe3bbbf8uL, 0xbd5407602ab64243uL, 0xc08ff330c8028ca0uL, 0xbd52b12c9cc82316uL, 0xc08ff333d0fcd560uL, 0x3d158d7dd801324buL,
     0xc08ff336d92b01a8uL, 0xbd38b55deae69564uL, 0xc08ff339e08d7ca0uL, 0x3d4a92d51dc43d43uL, 0xc08ff33ce724b110uL, 0x3d5455afbb5de008uL,
     0xc08ff33fecf10970uL, 0x3d3b65694b6f87fbuL, 0xc08ff342f1f2efe8uL, 0xbd3afb8ccc1260ebuL, 0xc08ff345f62ace50uL, 0x3d59c98f7ec71b79uL,
     0xc08ff348f9990e18uL, 0xbd5238294ff3846duL, 0xc08ff34bfc3e1880uL, 0x3d4deba7087bbf7buL, 0xc08ff34efe1a5650uL, 0xbd573e25d2d308e5uL,
     0xc08ff351ff2e3020uL, 0xbd44bc302ffa76fbuL, 0xc08ff354ff7a0e20uL, 0xbd2cad65891df000uL, 0xc08ff357fefe5838uL, 0x3d4b4fe326c05a8auL,
     0xc08ff35afdbb75f8uL, 0x3d0fb5680f67649buL, 0xc08ff35dfbb1cea8uL, 0xbd4af509a9977e57uL, 0xc08ff360f8e1c940uL, 0x3cea69221cfb0ad6uL,
     0xc08ff363f54bcc60uL, 0x3d3d116c159fead5uL, 0xc08ff366f0f03e58uL, 0xbd5e64e8bff70d5euL, 0xc08ff369ebcf8538uL, 0xbd5cc32ce5effb96uL,
     0xc08ff36ce5ea06b8uL, 0x3d57bbe811e4fbdauL, 0xc08ff36fdf402830uL, 0xbcf46d4595033678uL, 0xc08ff372d7d24ec8uL, 0x3d4c4bbec857b9fcuL,
     0xc08ff375cfa0df40uL, 0xbd59d3f339613a2duL, 0xc08ff378c6ac3e28uL, 0x3d58408e1bcb4e24uL, 0xc08ff37bbcf4cfa0uL, 0x3d5fdb793dc8e643uL,
     0xc08ff37eb27af788uL, 0xbd5f0d884b401f1euL, 0xc08ff381a73f1988uL, 0xbd5a7ed37e2c50b4uL, 0xc08ff3849b4198e8uL, 0x3d5b14c1f630b2afuL,
     0xc08ff3878e82d898uL, 0x3d505a9abef02affuL, 0xc08ff38a81033b50uL, 0xbd4a9bbd51a7d1c4uL, 0xc08ff38d72c32380uL, 0x3d4783623464f80euL,
     0xc08ff39063c2f338uL, 0xbd0e2d78f68abcc7uL, 0xc08ff39354030c50uL, 0x3d3e604763e782cbuL, 0xc08ff3964383d048uL, 0xbd4514f0840b6f59uL,
     0xc08ff3993245a060uL, 0xbd5488753d6035a4uL, 0xc08ff39c2048dd90uL, 0x3d5ccc099b5ff97duL, 0xc08ff39f0d8de870uL, 0x3d454ada83325c69uL,
     0xc08ff3a1fa152168uL, 0x3d1e4b27fb754eb1uL, 0xc08ff3a4e5dee890uL, 0x3d58c67819ead583uL, 0xc08ff3a7d0eb9da8uL, 0xbd536d02e85d644buL,
     0xc08ff3aabb3ba048uL, 0x3d5f510ab9e7c184uL, 0xc08ff3ada4cf4f98uL, 0x3d557bc5b296d5f5uL, 0xc08ff3b08da70a90uL, 0xbd48893b8f7f52c9uL,
     0xc08ff3b375c32fe8uL, 0x3d5ca0b69a37d601uL, 0xc08ff3b65d241df0uL, 0xbd519c57fff86872uL, 0xc08ff3b943ca32d8uL, 0x3d048da0e3a8c3c3uL,
     0xc08ff3bc29b5cc68uL, 0xbd5dd05e06ec07d0uL, 0xc08ff3bf0ee74840uL, 0x3d56c52a5c8015dbuL, 0xc08ff3c1f35f0398uL, 0x3d54e1dba9930beduL,
     0xc08ff3c4d71d5b78uL, 0x3d2c5f679a7932b7uL, 0xc08ff3c7ba22aca0uL, 0xbd3f77628aa1aed8uL, 0xc08ff3cd7e03ac60uL, 0xbd5cc8a22f1d8591uL,
     0xc08ff3d33f04e360uL, 0x3d4ae09463e13f6fuL, 0xc08ff3d8fd292dc8uL, 0x3d42736efbec3922uL, 0xc08ff3deb8736390uL, 0xbce0324f8d149b09uL,
     0xc08ff3e470e65870uL, 0xbd52089e4b8dd900uL, 0xc08ff3ea2684dbf0uL, 0xbd5f8e9d5dea127fuL, 0xc08ff3efd951b970uL, 0xbd4b60d79db026b1uL,
     0xc08ff3f5894fb828uL, 0x3d45ff1d6cea2c52uL, 0xc08ff3fb36819b38uL, 0x3d5d56022cd7f5b2uL, 0xc08ff400e0ea21a8uL, 0xbd58d63f09907b27uL,
     0xc08ff406888c0690uL, 0xbd4ce6ea362f7ce0uL, 0xc08ff40c2d6a00f0uL, 0x3d519fc9ad2ef3abuL, 0xc08ff411cf86c3c8uL, 0xbd55fc89e7b55f20uL,
     0xc08ff4176ee4fe40uL, 0xbd53229ca791d9beuL, 0xc08ff41d0b875b88uL, 0x3d5e7733e6fb23d1uL, 0xc08ff422a57082e0uL, 0x3d5871413696b637uL,
     0xc08ff4283ca317c0uL, 0x3d4b118aa7f493b9uL, 0xc08ff42dd121b9c8uL, 0x3d4bdf3692763b50uL, 0xc08ff43362ef04c8uL, 0x3d4867e17476dd63uL,
     0xc08ff438f20d90c8uL, 0xbd5d49b741c778f3uL, 0xc08ff43e7e7ff228uL, 0x3d59ac35724f01e3uL, 0xc08ff4440848b968uL, 0xbd5251ccdc49432duL,
     0xc08ff4498f6a7388uL, 0x3d56cf153ebc9f07uL, 0xc08ff44f13e7a9b8uL, 0x3d503b7a697a659cuL, 0xc08ff45495c2e198uL, 0xbd5fa03da8acd872uL,
     0xc08ff45a14fe9d38uL, 0xbd5e6cfb0b5c38fcuL, 0xc08ff45f919d5b08uL, 0x3d468b1f1269f1cfuL, 0xc08ff4650ba195e0uL, 0xbd313a3a8f72c0f3uL,
     0xc08ff46a830dc528uL, 0x3d205d31eb8d2bd4uL, 0xc08ff46ff7e45cb8uL, 0xbd56cb8ddf5d4a90uL, 0xc08ff4756a27cd00uL, 0x3d272c2d46acdcbfuL,
     0xc08ff47ad9da82e8uL, 0xbd4946efab7a989duL, 0xc08ff48046fee800uL, 0xbd23fabe48cf933cuL, 0xc08ff485b1976268uL, 0x3d4f03b099d80f79uL,
     0xc08ff48b19a654e0uL, 0x3d4fe0c35ab7e9b5uL, 0xc08ff4907f2e1ed0uL, 0xbd54b4843f34fe09uL, 0xc08ff495e2311c58uL, 0xbd5dfa6541236a64uL,
     0xc08ff49b42b1a648uL, 0x3d56fd2c8c418cbbuL, 0xc08ff4a0a0b21218uL, 0x3d5e687ef208418auL, 0xc08ff4a5fc34b210uL, 0x3d4a671ce14c5521uL,
     0xc08ff4ab553bd540uL, 0x3d419d0202e3cd96uL, 0xc08ff4b0abc9c780uL, 0x3d576b941a895781uL, 0xc08ff4b5ffe0d170uL, 0xbd4ea96d88cd1a30uL,
     0xc08ff4bb518338a0uL, 0x3d4d6b405bd43ba6uL, 0xc08ff4c0a0b33f60uL, 0xbcf03382150a56b7uL, 0xc08ff4c5ed7324f8uL, 0xbd400df96beb0937uL,
     0xc08ff4cb37c52590uL, 0xbd5c161714cdebd5uL, 0xc08ff4d07fab7a48uL, 0xbd333e8eda1a8e79uL, 0xc08ff4d5c5285928uL, 0x3d53aba20381d59fuL,
     0xc08ff4db083df530uL, 0xbd45e9b07af4e77cuL, 0xc08ff4e048ee7e70uL, 0xbd533cfdb78a8c41uL, 0xc08ff4e5873c21f0uL, 0xbd5d9b87f4d283f2uL,
     0xc08ff4eac32909c8uL, 0xbd53a677deee97fauL, 0xc08ff4effcb75d18uL, 0xbd5afd9f5dedc208uL, 0xc08ff4f533e94020uL, 0x3ce9dd794d20ab77uL,
     0xc08ff4fa68c0d428uL, 0xbd5eeae84ba1cbf1uL, 0xc08ff4ff9b4037b0uL, 0xbd4f4451587282c8uL, 0xc08ff504cb698648uL, 0xbd4a1fa15087e717uL,
     0xc08ff509f93ed8b0uL, 0xbd5f2f0042b9331auL, 0xc08ff50f24c244e0uL, 0xbd2c2389f8e86341uL, 0xc08ff5144df5ddf0uL, 0xbd556fcb7b48f200uL,
     0xc08ff51974dbb448uL, 0x3d43ba060aa69038uL, 0xc08ff51e9975d578uL, 0x3d477ef38ca20229uL, 0xc08ff523bbc64c60uL, 0x3d49bcaf1aa4168auL,
     0xc08ff528dbcf2120uL, 0xbd51c5609b60687euL, 0xc08ff52df9925930uL, 0xbd51691708d22ce7uL, 0xc08ff5331511f750uL, 0x3d30d05c98ecb3d1uL,
     0xc08ff5382e4ffb90uL, 0xbd423adb056dd244uL, 0xc08ff53d454e6368uL, 0xbd3663607042da50uL, 0xc08ff5425a0f29a8uL, 0x3d42655d3c6187a6uL,
     0xc08ff5476c944680uL, 0xbd028c958ae09d20uL, 0xc08ff54c7cdfaf90uL, 0xbd436eaf17756653uL, 0xc08ff5518af357e8uL, 0x3d5fbbbee66f8d24uL,
     0xc08ff55696d12ff0uL, 0xbd5d93b389497880uL, 0xc08ff55ba07b25b0uL, 0xbd43ff8ff777f337uL, 0xc08ff560a7f32488uL, 0xbcf3568803ec82a4uL,
     0xc08ff565ad3b1560uL, 0xbd50c83eba5cc7eauL, 0xc08ff56ab054deb0uL, 0x3d5becc2411500b7uL, 0xc08ff56fb1426458uL, 0xbd5dac964ffa8b83uL,
     0xc08ff574b00587f0uL, 0x3d1d82f6cc82e69fuL, 0xc08ff579aca02878uL, 0xbd34767c0d40542cuL, 0xc08ff57ea7142298uL, 0xbd52d28e996ed2ceuL,
     0xc08ff5839f635090uL, 0xbd432a85d337086duL, 0xc08ff588958f8a38uL, 0x3d512b06ec20c7fduL, 0xc08ff58d899aa500uL, 0xbd47e2147555e10buL,
     0xc08ff5927b867410uL, 0xbd4d84480a1b301duL, 0xc08ff5976b54c830uL, 0x3d5622146f3a51bduL, 0xc08ff59c59076fc8uL, 0x3d46d485c5f9c392uL,
     0xc08ff5a144a03700uL, 0xbd4562714549f4fduL, 0xc08ff5a62e20e7b8uL, 0x3d541ab67e365a63uL, 0xc08ff5ab158b4970uL, 0xbd5b0855668b2369uL,
     0xc08ff5affae12188uL, 0x3d27de1bc2ed4dd8uL, 0xc08ff5b4de243300uL, 0x3d40f2592d5ed454uL, 0xc08ff5b9bf563ea8uL, 0xbd4ee2f8ba7b3e9euL,
     0xc08ff5be9e790320uL, 0xbd3c2214335c2164uL, 0xc08ff5c37b8e3cc8uL, 0x3d30745623ab1fd9uL, 0xc08ff5c85697a5d0uL, 0xbd326c8fb0ffde38uL,
     0xc08ff5cd2f96f640uL, 0xbd4c83277493b0bcuL, 0xc08ff5d2068de3f8uL, 0x3d39bb1655e6e5bauL, 0xc08ff5d6db7e22a8uL, 0x3d403170b47a5559uL,
     0xc08ff5dbae6963e8uL, 0x3d5801ddf1edc325uL, 0xc08ff5e07f515728uL, 0x3d4b2704c46fe064uL, 0xc08ff5e54e37a9c8uL, 0x3d5a16e99ed6cd83uL,
     0xc08ff5ea1b1e0700uL, 0xbd5353a3ac18c62fuL, 0xc08ff5eee6061810uL, 0x3d567c69c189f21auL, 0xc08ff5f3aef18400uL, 0xbd50dd3220e0b0f2uL,
     0xc08ff5f875e1eff0uL, 0xbd3ab64d80638db2uL, 0xc08ff5fd3ad8fee0uL, 0x3d3ec753439035aauL, 0xc08ff601fdd851c8uL, 0xbd5e10415f5f5e74uL,
     0xc08ff606bee187b0uL, 0xbd55f1048b113faeuL, 0xc08ff60b7df63d90uL, 0x3d1e94e4107406c8uL, 0xc08ff6103b180e60uL, 0xbd4e2eb5d0c36eb5uL,
     0xc08ff614f6489330uL, 0x3d43ec5c714f709auL, 0xc08ff619af896308uL, 0x3d519ec459b62a08uL, 0xc08ff61e66dc1300uL, 0xbd5b93d09dd6161duL,
     0xc08ff6231c423658uL, 0x3d5d72b849dd56beuL, 0xc08ff627cfbd5e38uL, 0xbd276b7e32659173uL, 0xc08ff62c814f1a08uL, 0x3d4fd918f2e7a6b9uL,
     0xc08ff63130f8f730uL, 0x3d5609ba1dcc4c97uL, 0xc08ff635debc8138uL, 0xbd55cab233dbd84cuL, 0xc08ff63a8a9b41d8uL, 0xbd56778ab7aaabc9uL,
     0xc08ff63f3496c0e0uL, 0x3d5b2791da49c370uL, 0xc08ff643dcb08438uL, 0x3d583063ef145f9cuL, 0xc08ff64882ea1000uL, 0xbd484e9cab375fb6uL,
     0xc08ff64d2744e688uL, 0xbd5c430c95c374aauL, 0xc08ff651c9c28848uL, 0xbd57a16d78490bb3uL, 0xc08ff6566a6473e8uL, 0xbd445d70374ea9ecuL,
     0xc08ff65b092c2648uL, 0x3d5c9729142b9d4buL, 0xc08ff65fa61b1a70uL, 0xbd4aaa179d032405uL, 0xc08ff6644132c9c0uL, 0xbd2a3ea300d173deuL,
     0xc08ff668da74abc0uL, 0x3d57809438efb010uL, 0xc08ff66d71e23630uL, 0xbd5e9156720951d6uL, 0xc08ff672077cdd30uL, 0xbd5bab62e8462035uL,
     0xc08ff6769b461310uL, 0xbd05113545431443uL, 0xc08ff67b2d3f4868uL, 0x3d5105eb0607e59buL, 0xc08ff67fbd69ec18uL, 0xbd5e657842b37dc0uL,
     0xc08ff6844bc76b68uL, 0x3d4ad1849705bc4cuL, 0xc08ff688d85931c8uL, 0xbd508b6f92b6e0d6uL, 0xc08ff68d6320a920uL, 0x3d48683cceb5fdfcuL,
     0xc08ff691ec1f3990uL, 0xbd2c25ee290acbf5uL, 0xc08ff696735649a8uL, 0x3d58904932cd46d0uL, 0xc08ff69af8c73e38uL, 0xbd5c964167f0bfebuL,
     0xc08ff69f7c737a90uL, 0xbd43d66937fa06a9uL, 0xc08ff6a3fe5c6040uL, 0xbd54bc302ffa76fbuL, 0xc08ff6a87e834f50uL, 0x3d4609b1487f87a3uL,
     0xc08ff6acfce9a618uL, 0xbd42c0d9af0400b1uL, 0xc08ff6b17990c170uL, 0x3d549a63973d262duL, 0xc08ff6b5f479fc80uL, 0xbd28cde894aa0641uL,
     0xc08ff6ba6da6b0f0uL, 0xbd5acef617609a34uL, 0xc08ff6bee51836d8uL, 0x3d4abb9ff3cf80b8uL, 0xc08ff6c35acfe4a8uL, 0xbd53dcfa1b7697f3uL,
     0xc08ff6c7cecf0f68uL, 0x3d5bcdf4aea18a55uL, 0xc08ff6cc41170a70uL, 0x3d3cad29d4324038uL, 0xc08ff6d0b1a927b0uL, 0x3d56945f9cc2a565uL,
     0xc08ff6d52086b780uL, 0x3d5d20dfc1c668a7uL, 0xc08ff6d98db108b8uL, 0x3d37f20a9bcbbe04uL, 0xc08ff6ddf92968b8uL, 0x3d1e0824a6e3a4d2uL,
     0xc08ff6e262f12358uL, 0xbd469f07bf6322c7uL, 0xc08ff6e6cb0982f8uL, 0xbd5cc593afdbfaefuL, 0xc08ff6eb3173d080uL, 0xbd5ee68d555d7122uL,
     0xc08ff6ef96315360uL, 0xbd144ee1d6a39124uL, 0xc08ff6f3f9435188uL, 0xbd40f2cb308bcd25uL, 0xc08ff6f85aab0f80uL, 0xbd5fd98ced08a73cuL,
     0xc08ff6fcba69d068uL, 0x3d54f2f2a1ea8606uL, 0xc08ff7011880d5d0uL, 0xbd57818234572db7uL, 0xc08ff70574f16008uL, 0x3d52429e823a9a83uL,
     0xc08ff709cfbcadd0uL, 0x3d5d6dc9bb81476cuL, 0xc08ff70e28e3fc90uL, 0x3d57d189e116bcb2uL, 0xc08ff71280688848uL, 0x3d0e18992809fd6duL,
     0xc08ff716d64b8b98uL, 0xbd3b48ac92b8549auL, 0xc08ff71b2a8e3fb8uL, 0xbd4dcfa48040893buL, 0xc08ff71f7d31dc88uL, 0x3d58d945b8e53ef1uL,
     0xc08ff723ce379878uL, 0x3d4f80faef3e15eeuL, 0xc08ff7281da0a8b0uL, 0x3d53edc0fd40d18fuL, 0xc08ff72c6b6e40f0uL, 0xbd4bcac66e0be72fuL,
     0xc08ff730b7a193b0uL, 0xbd44fcf96e2ec967uL, 0xc08ff735023bd208uL, 0x3d57e2ff34b08d86uL, 0xc08ff7394b3e2bb0uL, 0xbd4caedfb10b98dduL,
     0xc08ff73d92a9cf28uL, 0xbd55db1083e5ac6auL, 0xc08ff741d87fe990uL, 0xbd580e83e6d54ed6uL, 0xc08ff7461cc1a6c0uL, 0x3d1688c83e1b0cbauL,
     0xc08ff74a5f703138uL, 0xbd52c398c872b701uL, 0xc08ff74ea08cb240uL, 0xbd49aabc3683b259uL, 0xc08ff752e01851d0uL, 0x3d5ccba8de72495buL,
     0xc08ff7571e143688uL, 0xbd5981cf630f5793uL, 0xc08ff75b5a8185e8uL, 0xbd4f235844e01ebduL, 0xc08ff75f95616410uL, 0xbd5047de7ba8ec62uL,
     0xc08ff763ceb4f3f0uL, 0x3d5fa55e004d6562uL, 0xc08ff768067d5720uL, 0xbd49f386e521a80euL, 0xc08ff76c3cbbae20uL, 0x3d3693551e62fe83uL,
     0xc08ff77071711818uL, 0x3d4ba63b30b6c42cuL, 0xc08ff774a49eb300uL, 0x3d4c26523d32f573uL, 0xc08ff778d6459b98uL, 0x3d3b65e70806143auL,
     0xc08ff77d0666ed68uL, 0xbd5796d9c9f2c2cbuL, 0xc08ff7813503c2d0uL, 0x3d33267b004b912buL, 0xc08ff785621d34e8uL, 0x3d1d5d8a23e33341uL,
     0xc08ff7898db45ba8uL, 0x3d46c95233e60f40uL, 0xc08ff78db7ca4dd0uL, 0x3d362865acc8f43fuL, 0xc08ff791e06020f8uL, 0xbd10e8203e161511uL,
     0xc08ff7960776e988uL, 0xbd5cafe4f4467eaauL, 0xc08ff79a2d0fbac8uL, 0xbd520fddea9ea0cduL, 0xc08ff79e512ba6d0uL, 0x3d5c53d3778dae46uL,
     0xc08ff7a273cbbe80uL, 0xbd5f0f6f88490367uL, 0xc08ff7a694f111c0uL, 0x3d5601aa3f55ec11uL, 0xc08ff7aab49caf20uL, 0xbd4f1a8a2328a4c4uL,
     0xc08ff7aed2cfa438uL, 0xbd4a3d5341c07d0euL, 0xc08ff7b2ef8afd68uL, 0xbd5f4a1f4c525f31uL, 0xc08ff7b70acfc600uL, 0xbd4d594d77b3d775uL,
     0xc08ff7bb249f0828uL, 0x3d2aef47e37e953buL, 0xc08ff7bf3cf9ccf0uL, 0x3d501803b47dfba2uL, 0xc08ff7c353e11c50uL, 0x3d5ed5ec84e5745euL,
     0xc08ff7c76955fd20uL, 0xbd3de249bc9e7f96uL, 0xc08ff7cb7d597538uL, 0x3d5b5794341d1fdfuL, 0xc08ff7cf8fec8938uL, 0xbd519dbd08276359uL,
     0xc08ff7d3a1103cd0uL, 0xbd450129b8038848uL, 0xc08ff7d7b0c59288uL, 0x3d348f00d3bb30fduL, 0xc08ff7dbbf0d8bd8uL, 0xbd43529025720d8auL,
     0xc08ff7dfcbe92938uL, 0x3d5abdaa2b1955d7uL, 0xc08ff7e3d75969f8uL, 0xbd4e8837d4588a98uL, 0xc08ff7e7e15f4c80uL, 0x3d57a782a6df5a1fuL,
     0xc08ff7ebe9fbce08uL, 0x3d304ba3eaa96bf1uL, 0xc08ff7eff12fead8uL, 0xbd47aab17b868a60uL, 0xc08ff7f3f6fc9e28uL, 0xbd5bd858693ba90auL,
     0xc08ff7f7fb62e230uL, 0x3d26abb2c547789auL, 0xc08ff7fbfe63b010uL, 0xbd59d383d543b3f5uL, 0xc08ff80000000000uL, 0x8000000000000000uL}

    , {
       0x0000000000000000uL, 0xbf670f83ff0a7565uL, 0xbf7709c46d7aac77uL, 0xbf8143068125dd0euL, 0xbf86fe50b6ef0851uL, 0xbf8cb6c3abd14559uL,
       0xbf91363117a97b0cuL, 0xbf940f9786685d29uL, 0xbf96e79685c2d22auL, 0xbf99be2f7749acc2uL, 0xbf9c9363ba850f86uL, 0xbf9f6734acf8695auL,
       0xbfa11cd1d5133413uL, 0xbfa2855905ca70f6uL, 0xbfa3ed3094685a26uL, 0xbfa554592bb8cd58uL, 0xbfa6bad3758efd87uL, 0xbfa820a01ac754cbuL,
       0xbfa985bfc3495194uL, 0xbfaaea3316095f72uL, 0xbfac4dfab90aab5fuL, 0xbfadb1175160f3b0uL, 0xbfaf1389833253a0uL, 0xbfb03aa8f8dc854cuL,
       0xbfb0eb389fa29f9buL, 0xbfb19b74069f5f0auL, 0xbfb24b5b7e135a3duL, 0xbfb2faef55ccb372uL, 0xbfb3aa2fdd27f1c3uL, 0xbfb4591d6310d85auL,
       0xbfb507b836033bb7uL, 0xbfb5b600a40bd4f3uL, 0xbfb663f6fac91316uL, 0xbfb7119b876bea86uL, 0xbfb7beee96b8a281uL, 0xbfb86bf07507a0c7uL,
       0xbfb918a16e46335buL, 0xbfb9c501cdf75872uL, 0xbfba7111df348494uL, 0xbfbb1cd1ecae66e7uL, 0xbfbbc84240adabbauL, 0xbfbc73632513bd4fuL,
       0xbfbd1e34e35b82dauL, 0xbfbdc8b7c49a1ddbuL, 0xbfbe72ec117fa5b2uL, 0xbfbf1cd21257e18cuL, 0xbfbfc66a0f0b00a5uL, 0xbfc037da278f2870uL,
       0xbfc08c588cda79e4uL, 0xbfc0e0b05ac848eduL, 0xbfc134e1b489062euL, 0xbfc188ecbd1d16beuL, 0xbfc1dcd197552b7buL, 0xbfc2309065d29791uL,
       0xbfc284294b07a640uL, 0xbfc2d79c6937efdduL, 0xbfc32ae9e278ae1auL, 0xbfc37e11d8b10f89uL, 0xbfc3d1146d9a8a64uL, 0xbfc423f1c2c12ea2uL,
       0xbfc476a9f983f74duL, 0xbfc4c93d33151b24uL, 0xbfc51bab907a5c8auL, 0xbfc56df5328d58c5uL, 0xbfc5c01a39fbd688uL, 0xbfc6121ac74813cfuL,
       0xbfc663f6fac91316uL, 0xbfc6b5aef4aae7dcuL, 0xbfc70742d4ef027fuL, 0xbfc758b2bb6c7b76uL, 0xbfc7a9fec7d05ddfuL, 0xbfc7fb27199df16duL,
       0xbfc84c2bd02f03b3uL, 0xbfc89d0d0ab430cduL, 0xbfc8edcae8352b6cuL, 0xbfc93e6587910444uL, 0xbfc98edd077e70dfuL, 0xbfc9df31868c11d5uL,
       0xbfca2f632320b86buL, 0xbfca7f71fb7bab9duL, 0xbfcacf5e2db4ec94uL, 0xbfcb1f27d7bd7a80uL, 0xbfcb6ecf175f95e9uL, 0xbfcbbe540a3f036fuL,
       0xbfcc0db6cdd94deeuL, 0xbfcc5cf77f860826uL, 0xbfccac163c770dc9uL, 0xbfccfb1321b8c400uL, 0xbfcd49ee4c325970uL, 0xbfcd98a7d8a605a7uL,
       0xbfcde73fe3b1480fuL, 0xbfce35b689cd2655uL, 0xbfce840be74e6a4duL, 0xbfced2401865df52uL, 0xbfcf205339208f27uL, 0xbfcf6e456567fe55uL,
       0xbfcfbc16b902680auL, 0xbfd004e3a7c97cbduL, 0xbfd02baba24d0664uL, 0xbfd0526359bab1b3uL, 0xbfd0790adbb03009uL, 0xbfd09fa235ba2020uL,
       0xbfd0c62975542a8fuL, 0xbfd0eca0a7e91e0buL, 0xbfd11307dad30b76uL, 0xbfd1395f1b5b61a6uL, 0xbfd15fa676bb08ffuL, 0xbfd185ddfa1a7ed0uL,
       0xbfd1ac05b291f070uL, 0xbfd1d21dad295632uL, 0xbfd1f825f6d88e13uL, 0xbfd21e1e9c877639uL, 0xbfd24407ab0e073auL, 0xbfd269e12f346e2cuL,
       0xbfd28fab35b32683uL, 0xbfd2b565cb3313b6uL, 0xbfd2db10fc4d9aafuL, 0xbfd300acd58cbb10uL, 0xbfd32639636b2836uL, 0xbfd34bb6b2546218uL,
       0xbfd37124cea4cdeduL, 0xbfd39683c4a9ce9auL, 0xbfd3bbd3a0a1dcfbuL, 0xbfd3e1146ebc9ff2uL, 0xbfd406463b1b0449uL, 0xbfd42b6911cf5465uL,
       0xbfd4507cfedd4fc4uL, 0xbfd475820e3a4251uL, 0xbfd49a784bcd1b8buL, 0xbfd4bf5fc36e8577uL, 0xbfd4e43880e8fb6auL, 0xbfd509028ff8e0a2uL,
       0xbfd52dbdfc4c96b3uL, 0xbfd5526ad18493ceuL, 0xbfd577091b3378cbuL, 0xbfd59b98e4de271cuL, 0xbfd5c01a39fbd688uL, 0xbfd5e48d25f62ab9uL,
       0xbfd608f1b42948aeuL, 0xbfd62d47efe3ebeeuL, 0xbfd6518fe4677ba7uL, 0xbfd675c99ce81f92uL, 0xbfd699f5248cd4b8uL, 0xbfd6be12866f820duL,
       0xbfd6e221cd9d0cdeuL, 0xbfd7062305156d1duL, 0xbfd72a1637cbc183uL, 0xbfd74dfb70a66388uL, 0xbfd771d2ba7efb3cuL, 0xbfd7959c202292f1uL,
       0xbfd7b957ac51aac4uL, 0xbfd7dd0569c04bffuL, 0xbfd800a563161c54uL, 0xbfd82437a2ee70f7uL, 0xbfd847bc33d8618euL, 0xbfd86b332056db01uL,
       0xbfd88e9c72e0b226uL, 0xbfd8b1f835e0b642uL, 0xbfd8d54673b5c372uL, 0xbfd8f88736b2d4e8uL, 0xbfd91bba891f1709uL, 0xbfd93ee07535f967uL,
       0xbfd961f90527409cuL, 0xbfd98504431717fcuL, 0xbfd9a802391e232fuL, 0xbfd9caf2f1498fa4uL, 0xbfd9edd6759b25e0uL, 0xbfda10acd0095ab4uL,
       0xbfda33760a7f6051uL, 0xbfda56322edd3731uL, 0xbfda78e146f7bef4uL, 0xbfda9b835c98c70auL, 0xbfdabe18797f1f49uL, 0xbfdae0a0a75ea862uL,
       0xbfdb031befe06434uL, 0xbfdb258a5ca28608uL, 0xbfdb47ebf73882a1uL, 0xbfdb6a40c92b203fuL, 0xbfdb8c88dbf8867auL, 0xbfdbaec439144dfduL,
       0xbfdbd0f2e9e79031uL, 0xbfdbf314f7d0f6bauL, 0xbfdc152a6c24cae6uL, 0xbfdc3733502d04f8uL, 0xbfdc592fad295b56uL, 0xbfdc7b1f8c4f51a4uL,
       0xbfdc9d02f6ca47b4uL, 0xbfdcbed9f5bb886auL, 0xbfdce0a4923a587duL, 0xbfdd0262d554051cuL, 0xbfdd2414c80bf27duL, 0xbfdd45ba735baa4fuL,
       0xbfdd6753e032ea0fuL, 0xbfdd88e11777b149uL, 0xbfddaa6222064fb9uL, 0xbfddcbd708b17359uL, 0xbfdded3fd442364cuL, 0xbfde0e9c8d782cbduL,
       0xbfde2fed3d097298uL, 0xbfde5131eba2b931uL, 0xbfde726aa1e754d2uL, 0xbfde939768714a32uL, 0xbfdeb4b847d15bceuL, 0xbfded5cd488f1732uL,
       0xbfdef6d67328e220uL, 0xbfdf17d3d01407afuL, 0xbfdf38c567bcc541uL, 0xbfdf59ab4286576cuL, 0xbfdf7a8568cb06cfuL, 0xbfdf9b53e2dc34c4uL,
       0xbfdfbc16b902680auL, 0xbfdfdccdf37d594cuL, 0xbfdffd799a83ff9buL, 0x3fdfe1e649bb6335uL, 0x3fdfc151b11b3640uL, 0x3fdfa0c8937e7d5duL,
       0x3fdf804ae8d0cd02uL, 0x3fdf5fd8a9063e35uL, 0x3fdf3f71cc1b629cuL, 0x3fdf1f164a15389auL, 0x3fdefec61b011f85uL, 0x3fdede8136f4cbf1uL,
       0x3fdebe47960e3c08uL, 0x3fde9e193073ac06uL, 0x3fde7df5fe538ab3uL, 0x3fde5dddf7e46e0auL, 0x3fde3dd1156507deuL, 0x3fde1dcf4f1c1a9euL,
       0x3fddfdd89d586e2buL, 0x3fddddecf870c4c1uL, 0x3fddbe0c58c3cff2uL, 0x3fdd9e36b6b825b1uL, 0x3fdd7e6c0abc3579uL, 0x3fdd5eac4d463d7euL,
       0x3fdd3ef776d43ff4uL, 0x3fdd1f4d7febf868uL, 0x3fdcffae611ad12buL, 0x3fdce01a12f5d8d1uL, 0x3fdcc0908e19b7bduL, 0x3fdca111cb2aa5c5uL,
       0x3fdc819dc2d45fe4uL, 0x3fdc62346dca1dfeuL, 0x3fdc42d5c4c688b4uL, 0x3fdc2381c08baf4fuL, 0x3fdc043859e2fdb3uL, 0x3fdbe4f9899d326euL,
       0x3fdbc5c5489254ccuL, 0x3fdba69b8fa1ab02uL, 0x3fdb877c57b1b070uL, 0x3fdb686799b00be3uL, 0x3fdb495d4e9185f7uL, 0x3fdb2a5d6f51ff83uL,
       0x3fdb0b67f4f46810uL, 0x3fdaec7cd882b46cuL, 0x3fdacd9c130dd53fuL, 0x3fdaaec59dadadbeuL, 0x3fda8ff971810a5euL, 0x3fda713787ad97a5uL,
       0x3fda527fd95fd8ffuL, 0x3fda33d25fcb1facuL, 0x3fda152f142981b4uL, 0x3fd9f695efbbd0efuL, 0x3fd9d806ebc9921cuL, 0x3fd9b98201a0f405uL,
       0x3fd99b072a96c6b2uL, 0x3fd97c96600672aduL, 0x3fd95e2f9b51f04euL, 0x3fd93fd2d5e1bf1duL, 0x3fd921800924dd3buL, 0x3fd903372e90bee4uL,
       0x3fd8e4f83fa145eeuL, 0x3fd8c6c335d8b966uL, 0x3fd8a8980abfbd32uL, 0x3fd88a76b7e549c6uL, 0x3fd86c5f36dea3dcuL, 0x3fd84e5181475449uL,
       0x3fd8304d90c11fd3uL, 0x3fd812535ef3ff19uL, 0x3fd7f462e58e1688uL, 0x3fd7d67c1e43ae5cuL, 0x3fd7b89f02cf2aaduL, 0x3fd79acb8cf10390uL,
       0x3fd77d01b66fbd37uL, 0x3fd75f417917e02cuL, 0x3fd7418acebbf18fuL, 0x3fd723ddb1346b65uL, 0x3fd7063a1a5fb4f2uL, 0x3fd6e8a004221b1fuL,
       0x3fd6cb0f6865c8eauL, 0x3fd6ad88411abfeauL, 0x3fd6900a8836d0d5uL, 0x3fd6729637b59418uL, 0x3fd6552b49986277uL, 0x3fd637c9b7e64dc2uL,
       0x3fd61a717cac1983uL, 0x3fd5fd2291fc33cfuL, 0x3fd5dfdcf1eeae0euL, 0x3fd5c2a096a135dcuL, 0x3fd5a56d7a370deduL, 0x3fd5884396d90702uL,
       0x3fd56b22e6b578e5uL, 0x3fd54e0b64003b70uL, 0x3fd530fd08f29fa7uL, 0x3fd513f7cfcb68ceuL, 0x3fd4f6fbb2cec598uL, 0x3fd4da08ac46495auL,
       0x3fd4bd1eb680e548uL, 0x3fd4a03dcbd2e1beuL, 0x3fd48365e695d797uL, 0x3fd466970128a987uL, 0x3fd449d115ef7d87uL, 0x3fd42d141f53b646uL,
       0x3fd4106017c3eca3uL, 0x3fd3f3b4f9b3e939uL, 0x3fd3d712bf9c9defuL, 0x3fd3ba7963fc1f8fuL, 0x3fd39de8e1559f6fuL, 0x3fd3816132316520uL,
       0x3fd364e2511cc821uL, 0x3fd3486c38aa29a8uL, 0x3fd32bfee370ee68uL, 0x3fd30f9a4c0d786duL, 0x3fd2f33e6d2120f2uL, 0x3fd2d6eb4152324fuL,
       0x3fd2baa0c34be1ecuL, 0x3fd29e5eedbe4a35uL, 0x3fd28225bb5e64a4uL, 0x3fd265f526e603cbuL, 0x3fd249cd2b13cd6cuL, 0x3fd22dadc2ab3497uL,
       0x3fd21196e87473d1uL, 0x3fd1f588973c8747uL, 0x3fd1d982c9d52708uL, 0x3fd1bd857b14c146uL, 0x3fd1a190a5d674a0uL, 0x3fd185a444fa0a7buL,
       0x3fd169c05363f158uL, 0x3fd14de4cbfd373euL, 0x3fd13211a9b38424uL, 0x3fd11646e7791469uL, 0x3fd0fa848044b351uL, 0x3fd0deca6f11b58buL,
       0x3fd0c318aedff3c0uL, 0x3fd0a76f3ab3c52cuL, 0x3fd08bce0d95fa38uL, 0x3fd070352293d724uL, 0x3fd054a474bf0eb7uL, 0x3fd0391bff2dbcf3uL,
       0x3fd01d9bbcfa61d4uL, 0x3fd00223a943dc19uL, 0x3fcfcd677e5ac81duL, 0x3fcf9697f3bd0ccfuL, 0x3fcf5fd8a9063e35uL, 0x3fcf29299496a889uL,
       0x3fcef28aacd72231uL, 0x3fcebbfbe83901a6uL, 0x3fce857d3d361368uL, 0x3fce4f0ea2509008uL, 0x3fce18b00e13123duL, 0x3fcde26177108d03uL,
       0x3fcdac22d3e441d3uL, 0x3fcd75f41b31b6dduL, 0x3fcd3fd543a4ad5cuL, 0x3fcd09c643f117f0uL, 0x3fccd3c712d31109uL, 0x3fcc9dd7a70ed160uL,
       0x3fcc67f7f770a67euL, 0x3fcc3227facce950uL, 0x3fcbfc67a7fff4ccuL, 0x3fcbc6b6f5ee1c9buL, 0x3fcb9115db83a3dduL, 0x3fcb5b844fb4b3efuL,
       0x3fcb2602497d5346uL, 0x3fcaf08fbfe15c51uL, 0x3fcabb2ca9ec7472uL, 0x3fca85d8feb202f7uL, 0x3fca5094b54d2828uL, 0x3fca1b5fc4e0b465uL,
       0x3fc9e63a24971f46uL, 0x3fc9b123cba27ed3uL, 0x3fc97c1cb13c7ec1uL, 0x3fc94724cca657beuL, 0x3fc9123c1528c6ceuL, 0x3fc8dd62821404a9uL,
       0x3fc8a8980abfbd32uL, 0x3fc873dca68b06f4uL, 0x3fc83f304cdc5aa7uL, 0x3fc80a92f5218accuL, 0x3fc7d60496cfbb4cuL, 0x3fc7a18529635926uL,
       0x3fc76d14a4601225uL, 0x3fc738b2ff50ccaduL, 0x3fc7046031c79f85uL, 0x3fc6d01c335dc9b5uL, 0x3fc69be6fbb3aa6fuL, 0x3fc667c08270b905uL,
       0x3fc633a8bf437ce1uL, 0x3fc5ff9fa9e18595uL, 0x3fc5cba53a0762eduL, 0x3fc597b967789d12uL, 0x3fc563dc29ffacb2uL, 0x3fc5300d796df33auL,
       0x3fc4fc4d4d9bb313uL, 0x3fc4c89b9e6807f5uL, 0x3fc494f863b8df35uL, 0x3fc46163957af02euL, 0x3fc42ddd2ba1b4a9uL, 0x3fc3fa651e276158uL,
       0x3fc3c6fb650cde51uL, 0x3fc3939ff859bf9fuL, 0x3fc36052d01c3dd7uL, 0x3fc32d13e4692eb7uL, 0x3fc2f9e32d5bfdd1uL, 0x3fc2c6c0a316a540uL,
       0x3fc293ac3dc1a668uL, 0x3fc260a5f58c02bduL, 0x3fc22dadc2ab3497uL, 0x3fc1fac39d5b280cuL, 0x3fc1c7e77dde33dcuL, 0x3fc195195c7d125buL,
       0x3fc162593186da70uL, 0x3fc12fa6f550f896uL, 0x3fc0fd02a03727eauL, 0x3fc0ca6c2a9b6b41uL, 0x3fc097e38ce60649uL, 0x3fc06568bf8576b3uL,
       0x3fc032fbbaee6d65uL, 0x3fc0009c779bc7b5uL, 0x3fbf9c95dc1d1165uL, 0x3fbf380e2d9ba4dfuL, 0x3fbed3a1d4cdbebbuL, 0x3fbe6f50c2d9f754uL,
       0x3fbe0b1ae8f2fd56uL, 0x3fbda700385788a2uL, 0x3fbd4300a2524d41uL, 0x3fbcdf1c1839ee74uL, 0x3fbc7b528b70f1c5uL, 0x3fbc17a3ed65b23cuL,
       0x3fbbb4102f925394uL, 0x3fbb5097437cb58euL, 0x3fbaed391ab6674euL, 0x3fba89f5a6dc9accuL, 0x3fba26ccd9981853uL, 0x3fb9c3bea49d3214uL,
       0x3fb960caf9abb7cauL, 0x3fb8fdf1ca8eea6auL, 0x3fb89b33091d6fe8uL, 0x3fb8388ea739470auL, 0x3fb7d60496cfbb4cuL, 0x3fb77394c9d958d5uL,
       0x3fb7113f3259e07auL, 0x3fb6af03c2603bd0uL, 0x3fb64ce26c067157uL, 0x3fb5eadb217198a3uL, 0x3fb588edd4d1ceaauL, 0x3fb5271a78622a0fuL,
       0x3fb4c560fe68af88uL, 0x3fb463c15936464euL, 0x3fb4023b7b26ac9euL, 0x3fb3a0cf56a06c4buL, 0x3fb33f7cde14cf5auL, 0x3fb2de4403ffd4b3uL,
       0x3fb27d24bae824dbuL, 0x3fb21c1ef55f06c2uL, 0x3fb1bb32a600549duL, 0x3fb15a5fbf7270ceuL, 0x3fb0f9a634663adduL, 0x3fb09905f797047cuL,
       0x3fb0387efbca869euL, 0x3fafb02267a1ad2duL, 0x3faeef792508b69duL, 0x3fae2f02159384feuL, 0x3fad6ebd1f1febfeuL, 0x3facaeaa27a02241uL,
       0x3fabeec9151aac2euL, 0x3fab2f19cdaa46dcuL, 0x3faa6f9c377dd31buL, 0x3fa9b05038d84095uL, 0x3fa8f135b8107912uL, 0x3fa8324c9b914bc7uL,
       0x3fa77394c9d958d5uL, 0x3fa6b50e297afcceuL, 0x3fa5f6b8a11c3c61uL, 0x3fa538941776b01euL, 0x3fa47aa07357704fuL, 0x3fa3bcdd9b9f00f3uL,
       0x3fa2ff4b77413dcbuL, 0x3fa241e9ed454683uL, 0x3fa184b8e4c56af8uL, 0x3fa0c7b844ef1795uL, 0x3fa00ae7f502c1c4uL, 0x3f9e9c8fb8a7a900uL,
       0x3f9d23afc49139f9uL, 0x3f9bab2fdcb46ec7uL, 0x3f9a330fd028f75fuL, 0x3f98bb4f6e2bd536uL, 0x3f9743ee861f3556uL, 0x3f95ccece78a4a9euL,
       0x3f94564a62192834uL, 0x3f92e006c59c9c29uL, 0x3f916a21e20a0a45uL, 0x3f8fe9370ef68e1buL, 0x3f8cfee70c5ce5dcuL, 0x3f8a15535d0bab34uL,
       0x3f872c7ba20f7327uL, 0x3f84445f7cbc8fd2uL, 0x3f815cfe8eaec830uL, 0x3f7cecb0f3922091uL, 0x3f7720d9c06a835fuL, 0x3f715676c8c7a8c1uL,
       0x3f671b0ea42e5fdauL, 0x3f57182a894b69c6uL, 0x8000000000000000uL}

    , {
       0xbf9B743CA0B38EEBuL, 0x3fa7BE52039FB7CEuL, 0xbfb563D0FCE18810uL, 0x3fc48DEAD300E7BFuL, 0xbfd6386B9B9B5D42uL, 0x3f4D4C6D2BC5AEA6uL}

    , {
       0x3fd2776E996DA1D2uL, 0xbfd715494C3E7C9BuL, 0x3fdEC709DC39E926uL, 0xbfe71547652B7CF8uL, 0x3ff71547652B82FEuL}

    , 0x000fffffffffffffuL, 0x3f60000000000000uL, 0x3f50000000000000uL, 0x0010000000000000uL, 0x7fefffffffffffffuL, 0xfffffffffc000000uL,
        0x3ff7100000000000uL, 0x3ff0000000000000uL, 0x4086a00000000000uL, 0x408ff80000000000uL, 0x408ff00000000000uL, {0x7ff0000000000000uL,
                                                                                                                       0xfff0000000000000uL}

    , {0x3ff0000000000000uL, 0xbff0000000000000uL}

    , {0x0000000000000000uL, 0x8000000000000000uL}

};

static __constant float __libm_rcp_table_256[256] = {
    0.998046875000, 0.994140625000, 0.990234375000, 0.986328125000, 0.982910156250, 0.979003906250, 0.975097656250, 0.971679687500, 0.967773437500,
        0.964355468750, 0.960449218750, 0.957031250000, 0.953613281250, 0.949707031250, 0.946289062500, 0.942871093750, 0.939453125000,
        0.936035156250, 0.932617187500, 0.929199218750, 0.925781250000, 0.922363281250, 0.919433593750, 0.916015625000, 0.912597656250,
        0.909179687500, 0.906250000000, 0.902832031250, 0.899902343750, 0.896484375000, 0.893554687500, 0.890625000000, 0.887207031250,
        0.884277343750, 0.881347656250, 0.878417968750, 0.875000000000, 0.872070312500, 0.869140625000, 0.866210937500, 0.863281250000,
        0.860351562500, 0.857421875000, 0.854980468750, 0.852050781250, 0.849121093750, 0.846191406250, 0.843261718750, 0.840820312500,
        0.837890625000, 0.835449218750, 0.832519531250, 0.829589843750, 0.827148437500, 0.824707031250, 0.821777343750, 0.819335937500,
        0.816406250000, 0.813964843750, 0.811523437500, 0.809082031250, 0.806152343750, 0.803710937500, 0.801269531250, 0.798828125000,
        0.796386718750, 0.793945312500, 0.791503906250, 0.789062500000, 0.786621093750, 0.784179687500, 0.781738281250, 0.779296875000,
        0.776855468750, 0.774414062500, 0.772460937500, 0.770019531250, 0.767578125000, 0.765136718750, 0.763183593750, 0.760742187500,
        0.758300781250, 0.756347656250, 0.753906250000, 0.751953125000, 0.749511718750, 0.747558593750, 0.745117187500, 0.743164062500,
        0.740722656250, 0.738769531250, 0.736816406250, 0.734375000000, 0.732421875000, 0.730468750000, 0.728515625000, 0.726074218750,
        0.724121093750, 0.722167968750, 0.720214843750, 0.718261718750, 0.716308593750, 0.713867187500, 0.711914062500, 0.709960937500,
        0.708007812500, 0.706054687500, 0.704101562500, 0.702148437500, 0.700195312500, 0.698730468750, 0.696777343750, 0.694824218750,
        0.692871093750, 0.690917968750, 0.688964843750, 0.687011718750, 0.685546875000, 0.683593750000, 0.681640625000, 0.680175781250,
        0.678222656250, 0.676269531250, 0.674804687500, 0.672851562500, 0.670898437500, 0.669433593750, 0.667480468750, 0.666015625000,
        0.664062500000, 0.662597656250, 0.660644531250, 0.659179687500, 0.657226562500, 0.655761718750, 0.653808593750, 0.652343750000,
        0.650390625000, 0.648925781250, 0.647460937500, 0.645507812500, 0.644042968750, 0.642578125000, 0.640625000000, 0.639160156250,
        0.637695312500, 0.636230468750, 0.634277343750, 0.632812500000, 0.631347656250, 0.629882812500, 0.628417968750, 0.626464843750,
        0.625000000000, 0.623535156250, 0.622070312500, 0.620605468750, 0.619140625000, 0.617675781250, 0.616210937500, 0.614746093750,
        0.613281250000, 0.611816406250, 0.610351562500, 0.608886718750, 0.607421875000, 0.605957031250, 0.604492187500, 0.603027343750,
        0.601562500000, 0.600097656250, 0.598632812500, 0.597656250000, 0.596191406250, 0.594726562500, 0.593261718750, 0.591796875000,
        0.590332031250, 0.589355468750, 0.587890625000, 0.586425781250, 0.584960937500, 0.583984375000, 0.582519531250, 0.581054687500,
        0.580078125000, 0.578613281250, 0.577148437500, 0.576171875000, 0.574707031250, 0.573242187500, 0.572265625000, 0.570800781250,
        0.569335937500, 0.568359375000, 0.566894531250, 0.565917968750, 0.564453125000, 0.563476562500, 0.562011718750, 0.560546875000,
        0.559570312500, 0.558105468750, 0.557128906250, 0.556152343750, 0.554687500000, 0.553710937500, 0.552246093750, 0.551269531250,
        0.549804687500, 0.548828125000, 0.547363281250, 0.546386718750, 0.545410156250, 0.543945312500, 0.542968750000, 0.541992187500,
        0.540527343750, 0.539550781250, 0.538574218750, 0.537109375000, 0.536132812500, 0.535156250000, 0.533691406250, 0.532714843750,
        0.531738281250, 0.530761718750, 0.529296875000, 0.528320312500, 0.527343750000, 0.526367187500, 0.524902343750, 0.523925781250,
        0.522949218750, 0.521972656250, 0.520996093750, 0.520019531250, 0.518554687500, 0.517578125000, 0.516601562500, 0.515625000000,
        0.514648437500, 0.513671875000, 0.512695312500, 0.511718750000, 0.510253906250, 0.509277343750, 0.508300781250, 0.507324218750,
        0.506347656250, 0.505371093750, 0.504394531250, 0.503417968750, 0.502441406250, 0.501464843750, 0.500488281250
};

static __constant double __libm_log2_table_256[256 * 2] = {
    1.19457508908561500e-09, 0.00282051786780357360, 4.15261433586624560e-09, 0.00847814977169036870, 3.59807233348338860e-10, 0.01415806263685226400,
        5.71007082023145860e-09, 0.01986041665077209500, 1.00634091262067260e-08, 0.02486853301525116000, 1.15604349446156940e-08,
        0.03061346709728241000, 1.86873140484243590e-08, 0.03638136386871337900, 2.73152543466571610e-09, 0.04144728183746337900,
        1.81104478601008970e-08, 0.04725873470306396500, 2.39851230634258260e-08, 0.05236303806304931600, 1.03440741639801970e-08,
        0.05821874737739563000, 5.86246877380269930e-08, 0.06336200237274169900, 1.50291779350067910e-09, 0.06852376461029052700,
        1.44530753582381470e-08, 0.07444554567337036100, 2.07712897661080520e-08, 0.07964712381362915000, 1.40277127216935640e-08,
        0.08486753702163696300, 1.17232747081360080e-08, 0.09010690450668335000, 3.28537915329039100e-08, 0.09536534547805786100,
        3.83271941386521520e-08, 0.10064303874969482000, 5.01950943220167510e-08, 0.10594010353088379000, 3.24661818120877250e-08,
        0.11125671863555908000, 1.03386200210685470e-08, 0.11659300327301025000, 3.52557668651177750e-08, 0.12118268013000488000,
        8.70466962135350020e-08, 0.12655580043792725000, 7.91390582647337590e-08, 0.13194906711578369000, 4.30683425217912900e-08,
        0.13736259937286377000, 9.06497105941239720e-08, 0.14201891422271729000, 1.63526400597317170e-08, 0.14747047424316406000,
        7.27216427834115140e-08, 0.15215957164764404000, 2.03175763247284680e-08, 0.15764963626861572000, 7.37637148231099900e-08,
        0.16237199306488037000, 1.95571821528777400e-08, 0.16710996627807617000, 4.44078519767160660e-08, 0.17265725135803223000,
        8.90403716378772760e-08, 0.17742908000946045000, 1.14729067738761400e-07, 0.18221676349639893000, 1.07720380578606930e-07,
        0.18702042102813721000, 5.00538417380803070e-09, 0.19264507293701172000, 9.52974783059106700e-08, 0.19748353958129883000,
        1.70386959518529010e-08, 0.20233845710754395000, 1.18601768151854900e-07, 0.20720958709716797000, 3.42212393297866190e-08,
        0.21209740638732910000, 1.14520138131889520e-07, 0.21700167655944824000, 1.15223247772919170e-07, 0.22192275524139404000,
        8.29519402575600560e-08, 0.22603654861450195000, 5.71912544644907730e-08, 0.23098862171173096000, 4.09560471233151370e-08,
        0.23595774173736572000, 3.29391345321577440e-08, 0.24094402790069580000, 3.30600649952030970e-08, 0.24594759941101074000,
        1.57640387463250820e-07, 0.25013041496276855000, 1.08728481745700080e-07, 0.25516605377197266000, 1.44887417675160320e-07,
        0.25937581062316895000, 1.01729334327749200e-07, 0.26444387435913086000, 4.33272540770786510e-08, 0.26952981948852539000,
        6.42395787239092990e-08, 0.27378177642822266000, 1.76460019346161920e-08, 0.27804636955261230000, 6.34350656210765950e-08,
        0.28318047523498535000, 3.62270468739892340e-08, 0.28747296333312988000, 1.81883961000007130e-07, 0.29264068603515625000,
        6.50693693380927510e-08, 0.29696154594421387000, 5.27731125198814750e-08, 0.30129528045654297000, 2.23206404609173310e-07,
        0.30564188957214355000, 1.79284740033198180e-07, 0.31087541580200195000, 2.91755422917620730e-08, 0.31525135040283203000,
        7.84607745619669510e-08, 0.31964039802551270000, 1.69969628130872920e-07, 0.32404279708862305000, 1.47277373468039570e-07,
        0.32845878601074219000, 9.31331021406610870e-08, 0.33288836479187012000, 9.10504516228461360e-08, 0.33733153343200684000,
        2.25317052919078350e-07, 0.34178829193115234000, 1.04166966369250880e-07, 0.34625911712646484000, 5.18833202795616210e-08,
        0.35074377059936523000, 1.55133889544086780e-07, 0.35524225234985352000, 2.45625663266857720e-08, 0.35975503921508789000,
        2.25311042123008050e-07, 0.36428165435791016000, 1.31261963945943500e-07, 0.36882281303405762000, 2.81858995904894870e-08,
        0.37246608734130859000, 5.99854907264093490e-08, 0.37703299522399902000, 5.11013788910911640e-08, 0.38161444664001465000,
        9.39380642601806990e-08, 0.38621044158935547000, 1.13666595355435260e-07, 0.38989782333374023000, 1.99192969484915510e-07,
        0.39452028274536133000, 1.23085232983934770e-07, 0.39915776252746582000, 4.83375067320489270e-08, 0.40287852287292480000,
        1.87774518069251940e-08, 0.40754294395446777000, 2.02445690390050840e-07, 0.41128516197204590000, 5.85522727171294350e-08,
        0.41597700119018555000, 4.03645920356002190e-08, 0.41974139213562012000, 1.03675113088766740e-07, 0.42446064949035645000,
        1.43132807100525790e-07, 0.42824721336364746000, 2.56422487515387260e-08, 0.43299460411071777000, 1.48452818500844160e-07,
        0.43680357933044434000, 1.02924244461125860e-07, 0.44062280654907227000, 1.81449193682516690e-07, 0.44541096687316895000,
        1.32341366149935300e-07, 0.44925308227539063000, 9.04358496636715680e-08, 0.45310544967651367000, 1.10668224137026960e-07,
        0.45696806907653809000, 2.27375320819775620e-09, 0.46181106567382813000, 6.74507650089300900e-08, 0.46569705009460449000,
        1.37968070961447430e-07, 0.46959352493286133000, 3.22562563110769560e-08, 0.47350072860717773000, 4.60461952885146820e-08,
        0.47741842269897461000, 2.37118358131403740e-07, 0.48134660720825195000, 6.89950724184867920e-08, 0.48627233505249023000,
        1.57416226973777570e-07, 0.49022483825683594000, 1.37364585332241940e-07, 0.49418830871582031000, 6.86682598241645830e-08,
        0.49816274642944336000, 1.16511771214375290e-08, 0.50214815139770508000, 2.71385720427471570e-08, 0.50614452362060547000,
        1.76462557661903540e-07, 0.51015186309814453000, 4.46306147914539410e-08, 0.51417064666748047000, 1.56371476807608010e-07,
        0.51719188690185547000, 1.59729558806538050e-07, 0.52123022079467773000, 5.46942207296917760e-08, 0.52527999877929688000,
        3.81920893935962760e-07, 0.52934074401855469000, 2.52093987753188090e-07, 0.53341341018676758000, 2.06955958491636320e-07,
        0.53749752044677734000, 3.11964883293800020e-07, 0.54159307479858398000, 2.90529179529616970e-07, 0.54467248916625977000,
        3.40651802056192370e-07, 0.54878854751586914000, 2.46995914175642370e-07, 0.55291652679443359000, 1.97541569319625320e-07,
        0.55602025985717773000, 3.72885062510271580e-07, 0.56016874313354492000, 1.14887423931589890e-07, 0.56432962417602539000,
        4.23677903581649540e-07, 0.56745767593383789000, 2.89484429363447430e-07, 0.57163953781127930000, 3.90564224098592560e-07,
        0.57583332061767578000, 2.26387428681055870e-08, 0.57898712158203125000, 1.10333393116624080e-07, 0.58320236206054688000,
        1.72324704488119510e-07, 0.58637189865112305000, 4.67060540431375620e-07, 0.59060859680175781000, 1.71888274775148440e-07,
        0.59379482269287109000, 4.20670679666055940e-07, 0.59805345535278320000, 4.14354531369273150e-07, 0.60125589370727539000,
        3.67676059871207870e-07, 0.60553693771362305000, 3.45203905323568620e-07, 0.60875606536865234000, 2.33954253911667300e-07,
        0.61305952072143555000, 3.69848945822403700e-07, 0.61629533767700195000, 4.28552517152109730e-07, 0.62062120437622070000,
        4.23254343089712610e-07, 0.62387418746948242000, 1.39930136859270380e-07, 0.62713479995727539000, 4.46909056366625440e-07,
        0.63149309158325195000, 2.80539715059925380e-07, 0.63477087020874023000, 4.24873156957887200e-07, 0.63805580139160156000,
        4.69261057599590290e-08, 0.64244794845581055000, 9.54412338374796060e-08, 0.64575052261352539000, 9.22877977445148590e-08,
        0.64906072616577148000, 7.23193166685411440e-08, 0.65237855911254883000, 1.86193964616284000e-07, 0.65681409835815430000,
        4.22835590117935030e-07, 0.66014957427978516000, 2.83756640213809520e-07, 0.66349315643310547000, 2.81708426500541790e-07,
        0.66684436798095703000, 4.52855958376746330e-07, 0.67020320892333984000, 6.79386242482464170e-09, 0.67469453811645508000,
        4.06241788042754670e-07, 0.67807149887084961000, 1.47603184396618210e-07, 0.68145704269409180000, 2.21831834390344260e-07,
        0.68485021591186523000, 1.89633712973027910e-07, 0.68825149536132813000, 8.88181122768987940e-08, 0.69166088104248047000,
        4.34300163461136150e-07, 0.69507789611816406000, 3.10754755682054950e-07, 0.69850349426269531000, 2.33642067460942410e-07,
        0.70193719863891602000, 2.41861546821714940e-07, 0.70537900924682617000, 3.74591713687216400e-07, 0.70882892608642578000,
        1.94455677561643720e-07, 0.71228742599487305000, 2.18035320174771090e-07, 0.71575403213500977000, 8.52540331822054530e-09,
        0.71922922134399414000, 8.30849753931319140e-08, 0.72271251678466797000, 5.49154589971328400e-09, 0.72620439529418945000,
        2.93492559607314350e-07, 0.72970438003540039000, 3.46224814488586690e-08, 0.73321342468261719000, 2.24065814910702190e-07,
        0.73673057556152344000, 4.26799941150757500e-07, 0.74025630950927734000, 2.72236547447588440e-07, 0.74261188507080078000,
        1.14042137796114090e-07, 0.74615240097045898000, 8.21547027466345170e-08, 0.74970149993896484000, 2.19530538385485990e-07,
        0.75325918197631836000, 9.26073713469996060e-08, 0.75682592391967773000, 2.21818991683718330e-07, 0.76040124893188477000,
        3.12987654311589180e-07, 0.76278972625732422000, 1.31817042036738900e-08, 0.76638031005859375000, 8.73656549679098210e-08,
        0.76997947692871094000, 1.03506380351506510e-07, 0.77358770370483398000, 2.10262344186059030e-07, 0.77599811553955078000,
        7.12000356424406850e-08, 0.77962160110473633000, 4.72065924605183660e-07, 0.78325366973876953000, 1.08325210761939410e-07,
        0.78568077087402344000, 8.10801012024986490e-08, 0.78932857513427734000, 2.40521930442387470e-07, 0.79298543930053711000,
        1.02851626471447270e-07, 0.79542875289916992000, 4.22699054053110360e-08, 0.79910135269165039000, 2.94499811096807080e-07,
        0.80278301239013672000, 1.30288445484845110e-07, 0.80524301528930664000, 3.74854329996211730e-07, 0.80894041061401367000,
        1.07280460595769850e-07, 0.81264781951904297000, 1.45372966138720040e-07, 0.81512451171875000000, 8.68844335167546390e-08,
        0.81884765625000000000, 3.10126340729734290e-07, 0.82133483886718750000, 1.21637504777241870e-07, 0.82507419586181641000,
        1.45620874940014580e-07, 0.82757234573364258000, 4.43513277384383030e-07, 0.83132743835449219000, 5.19314899337670600e-08,
        0.83509302139282227000, 3.33840751379287490e-07, 0.83760833740234375000, 1.78881469049437510e-07, 0.84139013290405273000,
        3.07059472462442470e-08, 0.84391689300537109000, 2.36507748775666570e-08, 0.84644794464111328000, 2.52168901922968230e-07,
        0.85025262832641602000, 4.27840974291743070e-07, 0.85279464721679688000, 2.88970384615554660e-07, 0.85661649703979492000,
        2.69205026502131580e-07, 0.85916996002197266000, 3.88774262608257440e-07, 0.86300849914550781000, 2.73559591479429990e-07,
        0.86557340621948242000, 3.25713860356774080e-07, 0.86942911148071289000, 2.16377121666510710e-07, 0.87200546264648438000,
        4.24170005455948340e-07, 0.87458610534667969000, 3.53356989726350940e-07, 0.87846612930297852000, 2.34344363662760260e-07,
        0.88105869293212891000, 1.38296703606931380e-08, 0.88365602493286133000, 1.25634219246847820e-07, 0.88756036758422852000,
        2.02044448270978800e-07, 0.89016914367675781000, 2.36267748264675110e-07, 0.89278268814086914000, 3.65263515157435020e-07,
        0.89671182632446289000, 3.69277272063745300e-07, 0.89933729171752930000, 3.91557295549829340e-07, 0.90196752548217773000,
        3.78292939108216720e-07, 0.90592193603515625000, 4.61104748160364580e-08, 0.90856456756591797000, 2.70651989609068800e-07,
        0.91121149063110352000, 1.16067661533671510e-07, 0.91386365890502930000, 4.64261362814302340e-07, 0.91785049438476563000,
        1.55748380053811030e-07, 0.92051506042480469000, 8.11274332651846300e-09, 0.92318439483642578000, 3.96278656226667000e-08,
        0.92585849761962891000, 3.43822737912250240e-07, 0.92987871170043945000, 4.26993722238047430e-07, 0.93256521224975586000,
        2.77241931621396480e-07, 0.93525695800781250000, 3.90142058228273920e-07, 0.93795347213745117000, 3.07699744985849430e-07,
        0.94065523147583008000, 4.88638501905191950e-08, 0.94336223602294922000, 3.84864791690785100e-07, 0.94743156433105469000,
        7.06799316034890240e-09, 0.95015144348144531000, 4.73719714489786690e-07, 0.95287561416625977000, 3.73744573906096900e-07,
        0.95560550689697266000, 2.03526359601689800e-07, 0.95834064483642578000, 4.59560236711581400e-07, 0.96108055114746094000,
        2.07942120484113640e-07, 0.96382617950439453000, 4.22229639564732380e-07, 0.96657657623291016000, 1.11288590839636790e-07,
        0.97071266174316406000, 1.47568124945946210e-07, 0.97347640991210938000, 2.43410787989147790e-07, 0.97624540328979492000,
        4.19219567524371770e-07, 0.97901964187622070000, 2.18678229393398740e-07, 0.98179960250854492000, 1.39263703041928050e-07,
        0.98458480834960938000, 2.01735526425732820e-07, 0.98737525939941406000, 4.26973932639365340e-07, 0.99017095565795898000,
        3.59143629152470330e-07, 0.99297237396240234000, 1.93690607745509010e-08, 0.99577951431274414000, 3.82572523642343370e-07,
        0.99859142303466797000
};

static __constant _iml_v2_dp_union_t _P[] = {
    0xbf8613d6, 0x3e994ae0, 0x652b82fe, 0xbfe71547, 0xdc3944e3, 0x3fdec709, 0x652a21f9, 0xbfd71547, 0x978ff6bc, 0x3fd27772, 0x810d0357, 0xbfcec718,
        0x00000000, 0x3ff71547
};

static __constant _iml_v2_dp_union_t _Q1[] = {
    0xbf85ddfa, 0x3e994ae0, 0xbf85a8b6, 0xbe894ae0, 0xdc3a03fc, 0x3fdec709, 0x652b8457, 0xbfd71547, 0x50f00aba, 0x3fd2776c, 0xdba4bedb, 0xbfcec709,
        0x05c58a46, 0x3fca6176, 0x43f9ebf8, 0xbfc71553, 0x9b5898b9, 0x3fc48748, 0x73413b57, 0xbfc227e1, 0x00000000, 0x3ff71547, 0x00000000, 0xbfe71547
};

static __constant _iml_v2_dp_union_t _Q2[] = {
    0xbf85ddee, 0x3e994ae0, 0x652b82fe, 0xbfe71547, 0xdc3fc94f, 0x3fdec709, 0x65323ffe, 0xbfd71547, 0x00000000, 0x3ff71547
};

static __constant _iml_v2_dp_union_t _Q3[] = {
    0xbf85ddf4, 0x3e994ae0, 0x652b82fe, 0xbfe71547, 0xdc3a03fd, 0x3fdec709, 0x00000000, 0x3ff71547
};

static __constant unsigned _TWO_32[] = { 0x00000000, 0x41f00000 };
static __constant unsigned _TWO_32P[] = { 0x00100000, 0x41f00000 };

static __constant _iml_v2_dp_union_t _TWO_55[] = { 0x00000000, 0x43600000 };

static __constant _iml_v2_dp_union_t _zeros[] = { 0x00000000, 0x00000000, 0x00000000, 0x80000000 };
static __constant _iml_v2_dp_union_t _ones[] = { 0x00000000, 0x3ff00000, 0x00000000, 0xbff00000 };
static __constant _iml_v2_dp_union_t _infs[] = { 0x00000000, 0x7ff00000, 0x00000000, 0xfff00000 };

__attribute__((always_inline))
inline int __internal_dlog2_la_cout (double *a, double *res)
{
    int n, i;
    unsigned ix;
    double z__constant = 0.0f;
    double xl, r, rl, y, s, t, result, x = (*a - (z__constant));
    double tv;
    __constant double *table;
    int nRet = 0;

    ix = ((int *) &x)[1];
    if (ix < 0x7ff00000)
    {
        if (ix - 0x3fef8000 < 0x3ff05000 - 0x3fef8000)
        {
            if (ix - 0x3feffff0 < 0x3ff00008 - 0x3feffff0)
            {

                ix = ((ix << 11) | (unsigned) ((int *) &x)[0] >> (32 - 11));
                if (ix - 0x7ffffffe < 0x80000001 - 0x7ffffffe)
                {
                    if (((((int *) &x)[1] - 0x3ff00000) | ((int *) &x)[0]) == 0)
                    {
                        *res = ((__constant double *) _zeros)[0];
                        return nRet;
                    }
                    else
                    {
                        x += ((__constant double *) _ones)[1];
                        *res =
                            (((((__constant double *) _Q3)[2] * x + ((__constant double *) _Q3)[1]) * x + ((__constant double *) _Q3)[0]) * x) +
                            (((__constant double *) _Q3)[3] * x);
                        return nRet;
                    }
                }
                else
                {
                    x += ((__constant double *) _ones)[1];
                    y = x * x;
                    rl = (((__constant double *) _Q2)[3] * y + ((__constant double *) _Q2)[1]) * y + (((__constant double *) _Q2)[2] * y +
                                                                                                      ((__constant double *) _Q2)[0]) * x;
                    xl = x;
                    ((int *) &x)[0] = 0;
                    xl -= x;
                    rl += ((__constant double *) _Q2)[4] * xl;
                    r = ((__constant double *) _Q2)[4] * x;
                    *res = (r + rl);
                    return nRet;
                }
            }
            else
            {
                x += ((__constant double *) _ones)[1];
                s = x;
                y = s * s;
                r = (((((__constant double *) _Q1)[9] * y + ((__constant double *) _Q1)[7]) * y + ((__constant double *) _Q1)[5]) * y +
                     ((__constant double *) _Q1)[3]) * y + (((((__constant double *) _Q1)[8] * y + ((__constant double *) _Q1)[6]) * y +
                                                             ((__constant double *) _Q1)[4]) * y + ((__constant double *) _Q1)[2]) * s;
                xl = x;
                tv = x * (*(__constant double *) _TWO_32P);
                x = x * (*(__constant double *) _TWO_32);
                x = (tv - x);
                xl -= x;
                rl = r;
                tv = r * (*(__constant double *) _TWO_32P);
                r = r * (*(__constant double *) _TWO_32);
                r = (tv - r);
                rl -= r;
                r += ((__constant double *) _Q1)[11];
                rl += ((__constant double *) _Q1)[1];
                rl = (r * xl + rl * s);
                r *= x;
                t = r;
                r += rl;
                tv = r * (*(__constant double *) _TWO_32P);
                r = (tv - r * (*(__constant double *) _TWO_32));
                t -= r;
                rl += t;
                r += ((__constant double *) _Q1)[10];
                rl += ((__constant double *) _Q1)[0];
                t = r;
                r += rl;
                tv = r * (*(__constant double *) _TWO_32P);
                r = (tv - r * (*(__constant double *) _TWO_32));
                t -= r;
                rl += t;
                rl = (r * xl + rl * s);
                r *= x;
                *res = (r + rl);
                return nRet;
            }
        }
        else
        {
            n = (ix >> 20) - 0x3ff;
            ((int *) &x)[1] &= ~0xfff00000;
            if (ix < 0x00100000)
            {
                if ((ix | ((int *) &x)[0]) == 0)
                {
                    *res = (((__constant double *) _ones)[(1)] / ((__constant double *) _zeros)[0]);
                    return 2;
                }
                x *= (*(__constant double *) _TWO_55);
                ix = ((int *) &x)[1];
                n = (ix >> 20) - (55 + 0x3ff);
            }
            i = (ix >> 12) & 0xff;
            ((int *) &x)[1] |= 0x3ff00000;
            r = __libm_rcp_table_256[i];
            xl = x;
            ((int *) &x)[0] = 0;
            xl -= x;
            x *= r;
            xl *= r;
            x += ((__constant double *) _ones)[1];
            s = (x + xl);
            y = s * s;
            table = &__libm_log2_table_256[2 * i];
            t = ((((__constant double *) _P)[5] * y + ((__constant double *) _P)[3]) * y + ((__constant double *) _P)[1]) * y +
                ((((__constant double *) _P)[4] * y + ((__constant double *) _P)[2]) * y + ((__constant double *) _P)[0]) * s;
            xl = ((__constant double *) _P)[6] * xl + t;
            x = x * ((__constant double *) _P)[6];
            t = x;
            tv = (x + (*(__constant double *) _TWO_32));
            x = (tv - (*(__constant double *) _TWO_32));
            t -= x;
            xl += t;
            t = (table[0]);
            xl += t;
            t = (table[1] + (double) n);
            x += t;
            *res = (x + xl);
            return nRet;
        }
    }
    else
    {
        ix &= ~0x80000000;
        if ((ix > 0x7ff00000) || ((ix == 0x7ff00000) && (((int *) &x)[0] != 0)))
        {
            *res = x * ((__constant double *) _ones)[0];
            return nRet;
        }
        else if (((int *) &x)[1] & 0x80000000)
        {
            if ((ix | ((int *) &x)[0]) == 0)
            {
                *res = (((__constant double *) _ones)[(1)] / ((__constant double *) _zeros)[0]);
                return 2;
            }
            else
            {
                *res = (((__constant double *) _infs)[0] * ((__constant double *) _zeros)[0]);
                return 1;
            }
        }
        else
        {
            *res = x;
            return nRet;
        }
    }
}

double __ocl_svml_log2_v2 (double a)
{

    double va1;
    double vr1;
    unsigned int vm;

    double r;

    va1 = a;;

    {

        double ExpMask;
        double Two10;
        double Mantissa;
        double DblRcp;
        unsigned long Expon;
        unsigned int IExpon;
        double FpExpon;
        unsigned long BrMask;
        double BrMask1;
        double BrMask2;
        double MinNorm;
        double MaxNorm;
        double CLH;
        double HalfMask;
        double __libm_rcp_table_256;
        unsigned long Index;
        double THL[2];
        double Kh;
        double Kl;
        double poly_coeff[7];
        double R2;
        double dP;
        double Rh;
        double Rl;

        double Threshold;
        double Bias;
        double Bias1;
        double BMask;
        double DBias;
        double dK;
        double dT;
        double P45;
        double P23;
        double P1;

        ExpMask = as_double (__internal_dlog2_la_data.ExpMask);

        Two10 = as_double (__internal_dlog2_la_data.Two10);

        Mantissa = as_double ((as_ulong (va1) & as_ulong (ExpMask)));
        Mantissa = as_double ((as_ulong (Mantissa) | as_ulong (Two10)));

        MinNorm = as_double (__internal_dlog2_la_data.MinNorm);
        MaxNorm = as_double (__internal_dlog2_la_data.MaxNorm);

        DblRcp = ((double) (1.0f / ((float) (Mantissa))));

        Expon = as_ulong (va1);
        Expon = ((unsigned long) (Expon) >> (52 - 32));
        IExpon = ((unsigned int) ((unsigned long) Expon >> 32));

        BrMask1 = as_double ((unsigned long) ((va1 < MinNorm) ? 0xffffffffffffffff : 0x0));
        BrMask2 = as_double ((unsigned long) (((!(va1 <= MaxNorm)) ? 0xffffffffffffffff : 0x0)));

        CLH = as_double (__internal_dlog2_la_data.One);

        {
            double _rnd_d2p52;
            double _rnd_dRes_ub;
            unsigned long _rnd_i2p52 = 0x4338000000000000;
            _rnd_d2p52 = as_double (_rnd_i2p52);
            _rnd_dRes_ub = (DblRcp + _rnd_d2p52);
            _rnd_dRes_ub = (_rnd_dRes_ub - _rnd_d2p52);
            DblRcp = _rnd_dRes_ub;
        };

        FpExpon = ((double) ((int) (IExpon)));

        BrMask1 = as_double ((as_ulong (BrMask1) | as_ulong (BrMask2)));
        BrMask = as_ulong (BrMask1);

        vm = 0;
        vm = BrMask;

        HalfMask = as_double (__internal_dlog2_la_data.HalfMask);
        {
            double t_HMant;
            double t_LMant;
            t_HMant = as_double ((as_ulong (Mantissa) & as_ulong (HalfMask)));
            t_LMant = (Mantissa - t_HMant);
            t_HMant = (t_HMant * DblRcp);
            t_LMant = (t_LMant * DblRcp);
            __libm_rcp_table_256 = (t_HMant - CLH);
            __libm_rcp_table_256 = (__libm_rcp_table_256 + t_LMant);
        };

        Index = as_ulong (DblRcp);
        Index = ((unsigned long) (Index) >> (52 - 9 - 3));
        dT = as_double (((__constant unsigned long *) ((__constant char *) (&__internal_dlog2_la_data.Log_LA_table[0]) - 0x408000))[Index >> 3]);

        Threshold = as_double (__internal_dlog2_la_data.Threshold);
        Bias = as_double (__internal_dlog2_la_data.Bias);
        Bias1 = as_double (__internal_dlog2_la_data.Bias1);
        BMask = as_double ((unsigned long) ((Threshold < DblRcp) ? 0xffffffffffffffff : 0x0));
        DBias = as_double ((as_ulong (BMask) & as_ulong (Bias)));
        DBias = as_double ((as_ulong (DBias) | as_ulong (Bias1)));;
        dK = (FpExpon - DBias);
        poly_coeff[5] = as_double (__internal_dlog2_la_data.poly_coeff[0]);
        poly_coeff[4] = as_double (__internal_dlog2_la_data.poly_coeff[1]);
        poly_coeff[3] = as_double (__internal_dlog2_la_data.poly_coeff[2]);
        poly_coeff[2] = as_double (__internal_dlog2_la_data.poly_coeff[3]);
        poly_coeff[1] = as_double (__internal_dlog2_la_data.poly_coeff[4]);
        P45 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly_coeff[5], __libm_rcp_table_256, poly_coeff[4]);
        P23 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly_coeff[3], __libm_rcp_table_256, poly_coeff[2]);
        R2 = (__libm_rcp_table_256 * __libm_rcp_table_256);
        P1 = (__libm_rcp_table_256 * poly_coeff[1]);
        dP = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (P45, R2, P23);
        dP = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dP, R2, P1);
        __libm_rcp_table_256 = (dP + dT);
        vr1 = (__libm_rcp_table_256 + dK);
    }

    if ((vm) != 0)
    {
        double _vapi_arg1[1];
        double _vapi_res1[1];
        ((double *) _vapi_arg1)[0] = va1;
        ((double *) _vapi_res1)[0] = vr1;
        __internal_dlog2_la_cout (_vapi_arg1, _vapi_res1);
        vr1 = ((double *) _vapi_res1)[0];
    };
    r = vr1;;

    return r;

}

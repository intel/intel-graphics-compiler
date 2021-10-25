/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF
typedef struct
{
    unsigned long Log_tbl_H[16];
    unsigned long Log_tbl_L[16];
    unsigned long One;
    unsigned long AbsMask;
    unsigned long AddB5;
    unsigned long RcpBitMask;
    unsigned long poly_coeff8;
    unsigned long poly_coeff7;
    unsigned long poly_coeff6;
    unsigned long poly_coeff5;
    unsigned long poly_coeff4;
    unsigned long poly_coeff3;
    unsigned long poly_coeff2;
    unsigned long poly_coeff1;
    unsigned long poly_coeff0;
    unsigned long Half;
    unsigned long L2H;
    unsigned long L2L;
} __internal_datanh_la_data_avx512_t;
static __constant __internal_datanh_la_data_avx512_t __internal_datanh_la_data_avx512 = {
    {
     0x0000000000000000uL, 0x3faf0a30c0100000uL, 0x3fbe27076e2a0000uL, 0x3fc5ff3070a80000uL, 0x3fcc8ff7c79b0000uL, 0x3fd1675cabab8000uL,
     0x3fd4618bc21c8000uL, 0x3fd739d7f6bc0000uL, 0x3fd9f323ecbf8000uL, 0x3fdc8ff7c79a8000uL, 0x3fdf128f5faf0000uL, 0x3fe0be72e4254000uL,
     0x3fe1e85f5e704000uL, 0x3fe307d7334f0000uL, 0x3fe41d8fe8468000uL, 0x3fe52a2d265bc000uL}
    , {
       0x0000000000000000uL, 0x3d662a6617cc9717uL, 0x3d6e5cbd3d50fffcuL, 0xbd6b0b0de3077d7euL, 0xbd697794f689f843uL, 0x3d630701ce63eab9uL,
       0xbd609ec17a426426uL, 0xbd67fcb18ed9d603uL, 0x3d584bf2b68d766fuL, 0x3d5a21ac25d81ef3uL, 0x3d3bb2cd720ec44cuL, 0xbd657d49676844ccuL,
       0x3d1a07bd8b34be7cuL, 0x3d60be1fb590a1f5uL, 0xbd5aa33736867a17uL, 0x3d46abb9df22bc57uL}

    , 0x3ff0000000000000uL, 0x7fffffffffffffffuL, 0x0000800000000000uL, 0xffff000000000000uL, 0x3fbc81dd40d38142uL, 0xbfc0073cb82e8b70uL,
        0x3fc2492298ffdae8uL, 0xbfc55553f871e5c5uL, 0x3fc9999999cd394auL, 0xbfd00000000c2a01uL, 0x3fd5555555555462uL, 0xbfdfffffffffffc5uL,
        0x3ff0000000000000uL, 0x3fe0000000000000uL, 0x3fe62E42FEFA0000uL, 0x3d7cf79abc9e0000uL
};

typedef struct
{
    unsigned long Log_HA_table[(1 << 10) + 2];
    unsigned long Log_LA_table[(1 << 9) + 1];
    unsigned long poly_coeff[4];
    unsigned long ExpMask;
    unsigned long Two10;
    unsigned long MinLog1p;
    unsigned long MaxLog1p;
    unsigned long HalfMask;
    unsigned long One;
    unsigned long SgnMask;
    unsigned long XThreshold;
    unsigned long XhMask;
    unsigned long L2H;
    unsigned long L2L;
    unsigned long Threshold;
    unsigned long Bias;
    unsigned long Bias1;
    unsigned long ExpMask0;
    unsigned long ExpMask2;
    unsigned long L2;
    unsigned long dBigThreshold;
    unsigned long dC2;
    unsigned long dC3;
    unsigned long dC4;
    unsigned long dC5;
    unsigned long dHalf;
    unsigned long dLargestFinite;
    unsigned long dLittleThreshold;
    unsigned long dSign;
    unsigned long dThirtyOne;
    unsigned long dTopMask12;
    unsigned long dTopMask26;
    unsigned long dTopMask29;
    unsigned long dTopMask41;
    unsigned long XScale;
    unsigned long TinyRange;

    unsigned long dInfs[2];
    unsigned long dOnes[2];
    unsigned long dZeros[2];
} __internal_datanh_la_data_t;
static __constant __internal_datanh_la_data_t __internal_datanh_la_data = {

    {
     0xc086232bdd7a8300uL, 0xbe1ce91eef3fb100uL, 0xc086232fdc7ad828uL, 0xbe1cefcffda73b6auL, 0xc0862333d97d2ba0uL, 0xbe1cef406748f1ffuL,
     0xc0862337d48378e0uL, 0xbe1cef2a9429925auL, 0xc086233bcd8fb878uL, 0xbe1cf138d17ebecbuL, 0xc086233fc4a3e018uL, 0xbe1ceff2dbbbb29euL,
     0xc0862343b9c1e270uL, 0xbe1cf1a42aae437buL, 0xc0862347acebaf68uL, 0xbe1cef3b152048afuL, 0xc086234b9e2333f0uL, 0xbe1cef20e127805euL,
     0xc086234f8d6a5a30uL, 0xbe1cf00ad6052cf4uL, 0xc08623537ac30980uL, 0xbe1cefc4642ee597uL, 0xc0862357662f2660uL, 0xbe1cf1f277d36e16uL,
     0xc086235b4fb092a0uL, 0xbe1ceed009e8d8e6uL, 0xc086235f37492d28uL, 0xbe1cf1e4038cb362uL, 0xc08623631cfad250uL, 0xbe1cf0b0873b8557uL,
     0xc086236700c75b98uL, 0xbe1cf15bb3227c0buL, 0xc086236ae2b09fe0uL, 0xbe1cf151ef8ca9eduL, 0xc086236ec2b87358uL, 0xbe1cefe1dc2cd2eduL,
     0xc0862372a0e0a780uL, 0xbe1cf0d1eec5454fuL, 0xc08623767d2b0b48uL, 0xbe1ceeefd570bbceuL, 0xc086237a57996af0uL, 0xbe1cee99ae91b3a7uL,
     0xc086237e302d9028uL, 0xbe1cf0412830fbd1uL, 0xc086238206e94218uL, 0xbe1ceee898588610uL, 0xc0862385dbce4548uL, 0xbe1cee9a1fbcaaeauL,
     0xc0862389aede5bc0uL, 0xbe1ceed8e7cc1ad6uL, 0xc086238d801b4500uL, 0xbe1cf10c8d059da6uL, 0xc08623914f86be18uL, 0xbe1ceee6c63a8165uL,
     0xc08623951d228180uL, 0xbe1cf0c3592d2ff1uL, 0xc0862398e8f04758uL, 0xbe1cf0026cc4cb1buL, 0xc086239cb2f1c538uL, 0xbe1cf15d48d8e670uL,
     0xc08623a07b28ae60uL, 0xbe1cef359363787cuL, 0xc08623a44196b390uL, 0xbe1cefdf1ab2e82cuL, 0xc08623a8063d8338uL, 0xbe1cefe43c02aa84uL,
     0xc08623abc91ec960uL, 0xbe1cf044f5ae35b7uL, 0xc08623af8a3c2fb8uL, 0xbe1cf0b0b4001e1buL, 0xc08623b349975d98uL, 0xbe1cf1bae76dfbcfuL,
     0xc08623b70731f810uL, 0xbe1cef0a72e13a62uL, 0xc08623bac30da1c8uL, 0xbe1cf184007d2b6buL, 0xc08623be7d2bfb40uL, 0xbe1cf16f4b239e98uL,
     0xc08623c2358ea2a0uL, 0xbe1cf0976acada87uL, 0xc08623c5ec3733d0uL, 0xbe1cf066318a16ffuL, 0xc08623c9a1274880uL, 0xbe1ceffaa7148798uL,
     0xc08623cd54607820uL, 0xbe1cf23ab02e9b6euL, 0xc08623d105e45800uL, 0xbe1cefdfef7d4fdeuL, 0xc08623d4b5b47b20uL, 0xbe1cf17fece44f2buL,
     0xc08623d863d27270uL, 0xbe1cf18f907d0d7cuL, 0xc08623dc103fccb0uL, 0xbe1cee61fe072c98uL, 0xc08623dfbafe1668uL, 0xbe1cf022dd891e2fuL,
     0xc08623e3640eda20uL, 0xbe1ceecc1daf4358uL, 0xc08623e70b73a028uL, 0xbe1cf0173c4fa380uL, 0xc08623eab12deec8uL, 0xbe1cf16a2150c2f4uL,
     0xc08623ee553f4a30uL, 0xbe1cf1bf980b1f4buL, 0xc08623f1f7a93480uL, 0xbe1cef8b731663c2uL, 0xc08623f5986d2dc0uL, 0xbe1cee9a664d7ef4uL,
     0xc08623f9378cb3f0uL, 0xbe1cf1eda2af6400uL, 0xc08623fcd5094320uL, 0xbe1cf1923f9d68d7uL, 0xc086240070e45548uL, 0xbe1cf0747cd3e03auL,
     0xc08624040b1f6260uL, 0xbe1cf22ee855bd6duL, 0xc0862407a3bbe078uL, 0xbe1cf0d57360c00buL, 0xc086240b3abb4398uL, 0xbe1ceebc815cd575uL,
     0xc086240ed01efdd0uL, 0xbe1cf03bfb970951uL, 0xc086241263e87f50uL, 0xbe1cf16e74768529uL, 0xc0862415f6193658uL, 0xbe1cefec64b8becbuL,
     0xc086241986b28f30uL, 0xbe1cf0838d210baauL, 0xc086241d15b5f448uL, 0xbe1cf0ea86e75b11uL, 0xc0862420a324ce28uL, 0xbe1cf1708d11d805uL,
     0xc08624242f008380uL, 0xbe1ceea988c5a417uL, 0xc0862427b94a7910uL, 0xbe1cef166a7bbca5uL, 0xc086242b420411d0uL, 0xbe1cf0c9d9e86a38uL,
     0xc086242ec92eaee8uL, 0xbe1cef0946455411uL, 0xc08624324ecbaf98uL, 0xbe1cefea60907739uL, 0xc0862435d2dc7160uL, 0xbe1cf1ed0934ce42uL,
     0xc086243955624ff8uL, 0xbe1cf191ba746c7duL, 0xc086243cd65ea548uL, 0xbe1ceeec78cf2a7euL, 0xc086244055d2c968uL, 0xbe1cef345284c119uL,
     0xc0862443d3c012b8uL, 0xbe1cf24f77355219uL, 0xc08624475027d5e8uL, 0xbe1cf05bf087e114uL, 0xc086244acb0b65d0uL, 0xbe1cef3504a32189uL,
     0xc086244e446c1398uL, 0xbe1ceff54b2a406fuL, 0xc0862451bc4b2eb8uL, 0xbe1cf0757d54ed4fuL, 0xc086245532aa04f0uL, 0xbe1cf0c8099fdfd5uL,
     0xc0862458a789e250uL, 0xbe1cf0b173796a31uL, 0xc086245c1aec1138uL, 0xbe1cf11d8734540duL, 0xc086245f8cd1da60uL, 0xbe1cf1916a723cebuL,
     0xc0862462fd3c84d8uL, 0xbe1cf19a911e1da7uL, 0xc08624666c2d5608uL, 0xbe1cf23a9ef72e4fuL, 0xc0862469d9a591c0uL, 0xbe1cef503d947663uL,
     0xc086246d45a67a18uL, 0xbe1cf0fceeb1a0b2uL, 0xc0862470b0314fa8uL, 0xbe1cf107e27e4fbcuL, 0xc086247419475160uL, 0xbe1cf03dd9922331uL,
     0xc086247780e9bc98uL, 0xbe1cefce1a10e129uL, 0xc086247ae719cd18uL, 0xbe1ceea47f73c4f6uL, 0xc086247e4bd8bd10uL, 0xbe1ceec0ac56d100uL,
     0xc0862481af27c528uL, 0xbe1cee8a6593278auL, 0xc086248511081c70uL, 0xbe1cf2231dd9dec7uL, 0xc0862488717af888uL, 0xbe1cf0b4b8ed7da8uL,
     0xc086248bd0818d68uL, 0xbe1cf1bd8d835002uL, 0xc086248f2e1d0d98uL, 0xbe1cf259acc107f4uL, 0xc08624928a4eaa20uL, 0xbe1cee897636b00cuL,
     0xc0862495e5179270uL, 0xbe1cee757f20c326uL, 0xc08624993e78f490uL, 0xbe1cefafd3aa54a4uL, 0xc086249c9673fd10uL, 0xbe1cee7298d38b97uL,
     0xc086249fed09d6f8uL, 0xbe1ceedc158d4cebuL, 0xc08624a3423babe0uL, 0xbe1cf2282987cb2euL, 0xc08624a6960aa400uL, 0xbe1cefe7381ecc4buL,
     0xc08624a9e877e600uL, 0xbe1cef328dbbce80uL, 0xc08624ad39849728uL, 0xbe1cefde45f3cc71uL, 0xc08624b08931db58uL, 0xbe1cefa8b89433b9uL,
     0xc08624b3d780d500uL, 0xbe1cef6773c0b139uL, 0xc08624b72472a528uL, 0xbe1cf031c931c11fuL, 0xc08624ba70086b78uL, 0xbe1cf088f49275e7uL,
     0xc08624bdba434630uL, 0xbe1cf17de0eaa86duL, 0xc08624c103245238uL, 0xbe1cefd492f1ba75uL, 0xc08624c44aacab08uL, 0xbe1cf1253e154466uL,
     0xc08624c790dd6ad0uL, 0xbe1cf0fb09ee6d55uL, 0xc08624cad5b7aa58uL, 0xbe1cf1f08dd048feuL, 0xc08624ce193c8120uL, 0xbe1ceeca0809697fuL,
     0xc08624d15b6d0538uL, 0xbe1cef8d5662d968uL, 0xc08624d49c4a4b78uL, 0xbe1cee97b556ed78uL, 0xc08624d7dbd56750uL, 0xbe1cf1b14b6acb75uL,
     0xc08624db1a0f6b00uL, 0xbe1cef1e860623f2uL, 0xc08624de56f96758uL, 0xbe1ceeaf4d156f3duL, 0xc08624e192946bf0uL, 0xbe1ceecc12b400eduL,
     0xc08624e4cce18710uL, 0xbe1cf180c40c794fuL, 0xc08624e805e1c5c8uL, 0xbe1cf185a08f7f65uL, 0xc08624eb3d9633d8uL, 0xbe1cef45fc924078uL,
     0xc08624ee73ffdbb0uL, 0xbe1cf1e4f457f32auL, 0xc08624f1a91fc6a0uL, 0xbe1cf040147b8a5auL, 0xc08624f4dcf6fc98uL, 0xbe1cf1effca0dfb2uL,
     0xc08624f80f868468uL, 0xbe1cf0470146e5bcuL, 0xc08624fb40cf6390uL, 0xbe1cef4dd186e501uL, 0xc08624fe70d29e60uL, 0xbe1ceebe257f66c7uL,
     0xc08625019f9137f0uL, 0xbe1ceefb7a1c395cuL, 0xc0862504cd0c3220uL, 0xbe1cf209dedfed8cuL, 0xc0862507f9448db0uL, 0xbe1cf082da464994uL,
     0xc086250b243b4a18uL, 0xbe1cee88694a73cfuL, 0xc086250e4df165a0uL, 0xbe1cf0b61e8f0531uL, 0xc08625117667dd78uL, 0xbe1cf1106599c962uL,
     0xc08625149d9fad98uL, 0xbe1ceff1ee88af1fuL, 0xc0862517c399d0c8uL, 0xbe1cf0f746994ef6uL, 0xc086251ae85740b8uL, 0xbe1cefe8a1d077e4uL,
     0xc086251e0bd8f5e0uL, 0xbe1cf1a1da036092uL, 0xc08625212e1fe7a8uL, 0xbe1cf0f8a7786fcduL, 0xc08625244f2d0c48uL, 0xbe1cefa1174a07a7uL,
     0xc08625276f0158d8uL, 0xbe1cef1043aa5b25uL, 0xc086252a8d9dc150uL, 0xbe1cf15d521c169duL, 0xc086252dab033898uL, 0xbe1cf220bba8861fuL,
     0xc0862530c732b078uL, 0xbe1cef51e310eae2uL, 0xc0862533e22d1988uL, 0xbe1cf222fcedd8aeuL, 0xc0862536fbf36370uL, 0xbe1cefdb4da4bda8uL,
     0xc086253a14867ca0uL, 0xbe1ceeafc1112171uL, 0xc086253d2be75280uL, 0xbe1cee99dfb4b408uL, 0xc08625404216d160uL, 0xbe1cf22d2536f06buL,
     0xc08625435715e498uL, 0xbe1cef6abbf2e268uL, 0xc08625466ae57648uL, 0xbe1cf093a14789f5uL, 0xc08625497d866fa0uL, 0xbe1cf0f93655603cuL,
     0xc086254c8ef9b8b8uL, 0xbe1cf1cc40c9aafcuL, 0xc086254f9f4038a8uL, 0xbe1ceeea5f4e9157uL, 0xc0862552ae5ad568uL, 0xbe1cefa9f52d4997uL,
     0xc0862555bc4a7400uL, 0xbe1cefa490a638ffuL, 0xc0862558c90ff868uL, 0xbe1cef7fcf797d6fuL, 0xc086255bd4ac4590uL, 0xbe1cf1b4c51113c9uL,
     0xc086255edf203d78uL, 0xbe1cef55e5b4a55duL, 0xc0862561e86cc100uL, 0xbe1cf0d37a25f9dcuL, 0xc0862564f092b028uL, 0xbe1ceebe9efc19d9uL,
     0xc0862567f792e9d8uL, 0xbe1cee8ad30a57b5uL, 0xc086256afd6e4c08uL, 0xbe1cef4e1817b90buL, 0xc086256e0225b3b8uL, 0xbe1cee7fa9229996uL,
     0xc086257105b9fce0uL, 0xbe1cf0b54963d945uL, 0xc0862574082c0298uL, 0xbe1cee5f2f3c7995uL, 0xc0862577097c9ee0uL, 0xbe1cf0828e303a2cuL,
     0xc086257a09acaae0uL, 0xbe1cf172c3078947uL, 0xc086257d08bcfec0uL, 0xbe1cf189252afa22uL, 0xc086258006ae71b8uL, 0xbe1cefdb80426923uL,
     0xc08625830381da08uL, 0xbe1ceef1391a0372uL, 0xc0862585ff380d00uL, 0xbe1cf17720c78d13uL, 0xc0862588f9d1df18uL, 0xbe1ceef1f9027d83uL,
     0xc086258bf35023b8uL, 0xbe1cf06fac99dec9uL, 0xc086258eebb3ad78uL, 0xbe1cf1373eeb45c0uL, 0xc0862591e2fd4e00uL, 0xbe1cef777536bb81uL,
     0xc0862594d92dd600uL, 0xbe1cf0f43ca40766uL, 0xc0862597ce461558uL, 0xbe1cefb2cfc6766buL, 0xc086259ac246daf0uL, 0xbe1ceea49e64ffa2uL,
     0xc086259db530f4c8uL, 0xbe1cf250fa457decuL, 0xc08625a0a7053018uL, 0xbe1cf17d8bb2a44euL, 0xc08625a397c45918uL, 0xbe1cf1d5906d54b7uL,
     0xc08625a6876f3b30uL, 0xbe1cf08fe7b31780uL, 0xc08625a97606a0e0uL, 0xbe1cef13edfc9d11uL, 0xc08625ac638b53c8uL, 0xbe1cef9d2b107219uL,
     0xc08625af4ffe1cb0uL, 0xbe1cf1ddd4ff6160uL, 0xc08625b23b5fc390uL, 0xbe1cefa02a996495uL, 0xc08625b525b10f68uL, 0xbe1cf166a7e37ee5uL,
     0xc08625b80ef2c680uL, 0xbe1cef0b171068a5uL, 0xc08625baf725ae28uL, 0xbe1cf05c80779283uL, 0xc08625bdde4a8af0uL, 0xbe1cf1bbfbffb889uL,
     0xc08625c0c4622090uL, 0xbe1cf0b8666c0124uL, 0xc08625c3a96d31e0uL, 0xbe1cf0a8fcf47a86uL, 0xc08625c68d6c80f0uL, 0xbe1cef46e18cb092uL,
     0xc08625c97060cef0uL, 0xbe1cf1458a350efbuL, 0xc08625cc524adc58uL, 0xbe1ceeea1dadce12uL, 0xc08625cf332b68b0uL, 0xbe1cf0a1bfdc44c7uL,
     0xc08625d2130332d0uL, 0xbe1cef96d02da73euL, 0xc08625d4f1d2f8a8uL, 0xbe1cf2451c3c7701uL, 0xc08625d7cf9b7778uL, 0xbe1cf10d08f83812uL,
     0xc08625daac5d6ba0uL, 0xbe1ceec5b4895c5euL, 0xc08625dd881990b0uL, 0xbe1cf14e1325c5e4uL, 0xc08625e062d0a188uL, 0xbe1cf21d0904be12uL,
     0xc08625e33c835838uL, 0xbe1ceed0839bcf21uL, 0xc08625e615326df0uL, 0xbe1cf1bb944889d2uL, 0xc08625e8ecde9b48uL, 0xbe1cee738e85eeceuL,
     0xc08625ebc38897e0uL, 0xbe1cf25c2bc6ef12uL, 0xc08625ee99311ac8uL, 0xbe1cf132b70a41aduL, 0xc08625f16dd8da28uL, 0xbe1cf1984236a6e3uL,
     0xc08625f441808b78uL, 0xbe1cf19ae74998f9uL, 0xc08625f71428e370uL, 0xbe1cef3e175d61a1uL, 0xc08625f9e5d295f8uL, 0xbe1cf101f9868fd9uL,
     0xc08625fcb67e5658uL, 0xbe1cee69db83dcd2uL, 0xc08625ff862cd6f8uL, 0xbe1cf081b636af51uL, 0xc086260254dec9a8uL, 0xbe1cee62c7d59b3euL,
     0xc08626052294df58uL, 0xbe1cf1b745c57716uL, 0xc0862607ef4fc868uL, 0xbe1cef3d2800ea23uL, 0xc086260abb103458uL, 0xbe1cef480ff1acd2uL,
     0xc086260d85d6d200uL, 0xbe1cf2424c9a17efuL, 0xc08626104fa44f90uL, 0xbe1cf12cfde90fd5uL, 0xc086261318795a68uL, 0xbe1cf21f590dd5b6uL,
     0xc0862615e0569f48uL, 0xbe1cf0c50f9cd28auL, 0xc0862618a73cca30uL, 0xbe1ceedbdb520545uL, 0xc086261b6d2c8668uL, 0xbe1cf0b030396011uL,
     0xc086261e32267e98uL, 0xbe1cf19917010e96uL, 0xc0862620f62b5cb0uL, 0xbe1cf07331355985uL, 0xc0862623b93bc9e8uL, 0xbe1cf01ae921a1c3uL,
     0xc08626267b586ed0uL, 0xbe1cefe5cf0dbf0cuL, 0xc08626293c81f348uL, 0xbe1cf01b258aeb50uL, 0xc086262bfcb8fe88uL, 0xbe1cee6b9e7f4c68uL,
     0xc086262ebbfe3710uL, 0xbe1cee684a9b21c9uL, 0xc08626317a5242b8uL, 0xbe1cf1f8bcde9a8buL, 0xc086263437b5c6c0uL, 0xbe1cf1d063d36238uL,
     0xc0862636f42967a8uL, 0xbe1cf1e31a19075euL, 0xc0862639afadc950uL, 0xbe1cf1d8efdf7e7duL, 0xc086263c6a438ef0uL, 0xbe1cf1812ee72dbauL,
     0xc086263f23eb5b18uL, 0xbe1cf1449a9a2279uL, 0xc0862641dca5cfb8uL, 0xbe1cee96edce5085uL, 0xc086264494738e08uL, 0xbe1cf06797bd03b2uL,
     0xc08626474b5536b8uL, 0xbe1cef91b9b7ffc1uL, 0xc086264a014b69c0uL, 0xbe1cef4b6721278fuL, 0xc086264cb656c678uL, 0xbe1cf1942925eb4auL,
     0xc086264f6a77eba8uL, 0xbe1cefa2c7bc2e39uL, 0xc08626521daf7758uL, 0xbe1cf252595aceb3uL, 0xc0862654cffe0718uL, 0xbe1cee8e9ae47ec2uL,
     0xc0862657816437a8uL, 0xbe1cf1bf913828fauL, 0xc086265a31e2a558uL, 0xbe1cf23475d6b366uL, 0xc086265ce179ebc8uL, 0xbe1cef8df00a922buL,
     0xc086265f902aa5f0uL, 0xbe1cef279bfa43e0uL, 0xc08626623df56e38uL, 0xbe1cf080e10b8365uL, 0xc0862664eadade70uL, 0xbe1cf1a518f9b544uL,
     0xc086266796db8fd0uL, 0xbe1cef9308fed9e9uL, 0xc086266a41f81ae8uL, 0xbe1ceea3ae6b19c9uL, 0xc086266cec3117b8uL, 0xbe1ceef06003d4c2uL,
     0xc086266f95871da8uL, 0xbe1cf0b8457ffb0cuL, 0xc08626723dfac390uL, 0xbe1cf0c526745ad6uL, 0xc0862674e58c9fa8uL, 0xbe1cf0cf91ff7b5duL,
     0xc08626778c3d4798uL, 0xbe1cefe260819380uL, 0xc086267a320d5070uL, 0xbe1ceebd90aa27a3uL, 0xc086267cd6fd4ea8uL, 0xbe1cf0388121dffauL,
     0xc086267f7b0dd630uL, 0xbe1cf1a3881435f1uL, 0xc08626821e3f7a68uL, 0xbe1cef28e9d9ac52uL, 0xc0862684c092ce08uL, 0xbe1cf02d300062dduL,
     0xc086268762086350uL, 0xbe1cefaee1edfa35uL, 0xc086268a02a0cbe0uL, 0xbe1cf0a5a052e936uL, 0xc086268ca25c98d8uL, 0xbe1cee60a4a497eduL,
     0xc086268f413c5ab0uL, 0xbe1cf0e4a5d0cf49uL, 0xc0862691df40a170uL, 0xbe1cf149235a4e6euL, 0xc08626947c69fc80uL, 0xbe1cf215180b9fccuL,
     0xc086269718b8fac8uL, 0xbe1cef9b156a9840uL, 0xc0862699b42e2a90uL, 0xbe1cf054c91441beuL, 0xc086269c4eca19a8uL, 0xbe1cf13ded26512cuL,
     0xc086269ee88d5550uL, 0xbe1cf22ea4d8ac06uL, 0xc08626a181786a40uL, 0xbe1cf2354666ee2euL, 0xc08626a4198be4a8uL, 0xbe1cefef936752b3uL,
     0xc08626a6b0c85020uL, 0xbe1cf1e360a9db68uL, 0xc08626a9472e37d8uL, 0xbe1ceed6aeb812c5uL, 0xc08626abdcbe2650uL, 0xbe1cf227340b4986uL,
     0xc08626ae7178a5b0uL, 0xbe1cf0215a0cbe0duL, 0xc08626b1055e3f70uL, 0xbe1cf256adf0ae26uL, 0xc08626b3986f7ca8uL, 0xbe1ceff3c67aed06uL,
     0xc08626b62aace5c8uL, 0xbe1cf2159fb93652uL, 0xc08626b8bc1702e0uL, 0xbe1cf01e6dbd1c7fuL, 0xc08626bb4cae5b60uL, 0xbe1cf009e75d1c0cuL,
     0xc08626bddc737648uL, 0xbe1ceec10a020e73uL, 0xc08626c06b66da08uL, 0xbe1cf06d5783eee7uL, 0xc08626c2f9890ca0uL, 0xbe1cf0cb8f169ffeuL,
     0xc08626c586da9388uL, 0xbe1cef7de2452430uL, 0xc08626c8135bf3b0uL, 0xbe1cf05da6f783aeuL, 0xc08626ca9f0db198uL, 0xbe1cefcc877d681duL,
     0xc08626cd29f05138uL, 0xbe1cef0531954ab3uL, 0xc08626cfb4045608uL, 0xbe1cf06b8565ea3duL, 0xc08626d23d4a4310uL, 0xbe1cefdc455d9d7euL,
     0xc08626d4c5c29ad0uL, 0xbe1ceefc47e8fa64uL, 0xc08626d74d6ddf48uL, 0xbe1cf1872bf033f2uL, 0xc08626d9d44c9210uL, 0xbe1cf19d91087f9duL,
     0xc08626dc5a5f3438uL, 0xbe1cf012d444c6abuL, 0xc08626dedfa64650uL, 0xbe1cf0ba528ee153uL, 0xc08626e164224880uL, 0xbe1ceeb431709788uL,
     0xc08626e3e7d3ba60uL, 0xbe1cf0b9af31a6a5uL, 0xc08626e66abb1b28uL, 0xbe1cf168fb2e135buL, 0xc08626e8ecd8e990uL, 0xbe1cef9097461c93uL,
     0xc08626eb6e2da3d0uL, 0xbe1cee7a434735d8uL, 0xc08626edeeb9c7a8uL, 0xbe1cf235732b86f2uL, 0xc08626f06e7dd280uL, 0xbe1cefe1510b89e6uL,
     0xc08626f2ed7a4120uL, 0xbe1cf1f64b9b80efuL, 0xc08626f56baf9000uL, 0xbe1cf08f320ca339uL, 0xc08626f7e91e3b08uL, 0xbe1cf1b1de2808a1uL,
     0xc08626fa65c6bdc0uL, 0xbe1cf1976d778b28uL, 0xc08626fce1a99338uL, 0xbe1ceef40a4f076fuL, 0xc08626ff5cc73600uL, 0xbe1cef3e45869ce3uL,
     0xc0862701d7202048uL, 0xbe1ceef601b4c9d6uL, 0xc086270450b4cbc0uL, 0xbe1cf1eaf0b57fd6uL, 0xc0862706c985b1c0uL, 0xbe1cef82a44990f3uL,
     0xc086270941934b10uL, 0xbe1ceefe32981f2cuL, 0xc086270bb8de1018uL, 0xbe1cefbf6f5a0445uL, 0xc086270e2f6678d0uL, 0xbe1cf18dba75792cuL,
     0xc0862710a52cfcc8uL, 0xbe1cf0da64ce995fuL, 0xc08627131a321318uL, 0xbe1cef04ac0fb802uL, 0xc08627158e763268uL, 0xbe1cee9d4e2ad9bduL,
     0xc086271801f9d0f8uL, 0xbe1cefa9b55407b5uL, 0xc086271a74bd64a0uL, 0xbe1cefe6bd329570uL, 0xc086271ce6c162c8uL, 0xbe1cef0b1205dc85uL,
     0xc086271f58064068uL, 0xbe1cef092a785e3fuL, 0xc0862721c88c7210uL, 0xbe1cf050dcdaac30uL, 0xc086272438546be8uL, 0xbe1cf210907ded8buL,
     0xc0862726a75ea1b8uL, 0xbe1cee760be44f99uL, 0xc086272915ab86c0uL, 0xbe1ceeeee07c2bccuL, 0xc086272b833b8df0uL, 0xbe1cf06874992df5uL,
     0xc086272df00f29d0uL, 0xbe1cef8fac5d4899uL, 0xc08627305c26cc70uL, 0xbe1cf1103241cc99uL, 0xc0862732c782e788uL, 0xbe1cf1d35fef83feuL,
     0xc08627353223ec68uL, 0xbe1cef3ec8133e1duL, 0xc08627379c0a4be8uL, 0xbe1cef7261daccd8uL, 0xc086273a05367688uL, 0xbe1cf18656c50806uL,
     0xc086273c6da8dc68uL, 0xbe1cf1c8736e049auL, 0xc086273ed561ed38uL, 0xbe1cf1f93bff4911uL, 0xc08627413c621848uL, 0xbe1cf188a4ea680cuL,
     0xc0862743a2a9cc80uL, 0xbe1cf1d270930c80uL, 0xc086274608397868uL, 0xbe1cf25a328c28e2uL, 0xc08627486d118a28uL, 0xbe1cf106f90aa3b8uL,
     0xc086274ad1326f80uL, 0xbe1cee5e9d2e885auL, 0xc086274d349c95c0uL, 0xbe1cf1c0bac27228uL, 0xc086274f975069f8uL, 0xbe1cf1a1500f9b1cuL,
     0xc0862751f94e58c0uL, 0xbe1cefc30663ac44uL, 0xc08627545a96ce48uL, 0xbe1cf17123e427a2uL, 0xc0862756bb2a3678uL, 0xbe1cefb92749fea4uL,
     0xc08627591b08fcc0uL, 0xbe1cefa40e1ea74auL, 0xc086275b7a338c40uL, 0xbe1cee6f4612c3e9uL, 0xc086275dd8aa4fa8uL, 0xbe1cf1c54a053627uL,
     0xc0862760366db168uL, 0xbe1ceff5eb503d9euL, 0xc0862762937e1b70uL, 0xbe1cf02e47f10ceeuL, 0xc0862764efdbf768uL, 0xbe1ceeb06e1d0daduL,
     0xc08627674b87ae88uL, 0xbe1cf10aadd6dba5uL, 0xc0862769a681a9c0uL, 0xbe1cf24e9913d30fuL, 0xc086276c00ca51a0uL, 0xbe1cef47b301e312uL,
     0xc086276e5a620e48uL, 0xbe1ceeb1cefc2e85uL, 0xc0862770b3494788uL, 0xbe1cf16f1fbbe011uL, 0xc08627730b8064e8uL, 0xbe1ceebdf75174c7uL,
     0xc08627756307cd70uL, 0xbe1cf06e3871a0dauL, 0xc0862777b9dfe7f0uL, 0xbe1cef16799fd554uL, 0xc086277a10091ac0uL, 0xbe1cf248dabf5377uL,
     0xc086277c6583cc00uL, 0xbe1cf0c78d92a2cduL, 0xc086277eba506158uL, 0xbe1cf0b911b029f0uL, 0xc08627810e6f4028uL, 0xbe1cefdc24719766uL,
     0xc086278361e0cd70uL, 0xbe1cefbb6562b7e7uL, 0xc0862785b4a56dd8uL, 0xbe1cf1e0afb349ecuL, 0xc086278806bd85c0uL, 0xbe1cf008292e52fcuL,
     0xc086278a58297918uL, 0xbe1cf053073872bfuL, 0xc086278ca8e9ab88uL, 0xbe1cf17a0a55a947uL, 0xc086278ef8fe8068uL, 0xbe1ceeffb0b60234uL,
     0xc086279148685aa0uL, 0xbe1cf162204794a8uL, 0xc086279397279ce0uL, 0xbe1cf24cc8cb48acuL, 0xc0862795e53ca978uL, 0xbe1cf0c9be68d5c3uL,
     0xc086279832a7e258uL, 0xbe1cf172cd3d7388uL, 0xc086279a7f69a930uL, 0xbe1ceea2465fbce5uL, 0xc086279ccb825f40uL, 0xbe1cf0a386d2500fuL,
     0xc086279f16f26590uL, 0xbe1cf1e338ddc18auL, 0xc08627a161ba1cd0uL, 0xbe1cef1f5049867fuL, 0xc08627a3abd9e548uL, 0xbe1cef96c1ea8b1fuL,
     0xc08627a5f5521f00uL, 0xbe1cf138f6fd3c26uL, 0xc08627a83e2329b0uL, 0xbe1cf0d4fcbfdf3auL, 0xc08627aa864d64b0uL, 0xbe1cf24870c12c81uL,
     0xc08627accdd12f18uL, 0xbe1cf0ae2a56348duL, 0xc08627af14aee7a0uL, 0xbe1cee8ca1a9b893uL, 0xc08627b15ae6eca8uL, 0xbe1cf20414d637b0uL,
     0xc08627b3a0799c60uL, 0xbe1cf0fc6b7b12d8uL, 0xc08627b5e5675488uL, 0xbe1cf152d93c4a00uL, 0xc08627b829b072a0uL, 0xbe1cf1073f9b77c2uL,
     0xc08627ba6d5553d8uL, 0xbe1cee694f97d5a4uL, 0xc08627bcb0565500uL, 0xbe1cf0456b8239d7uL, 0xc08627bef2b3d2b0uL, 0xbe1cf211497127e3uL,
     0xc08627c1346e2930uL, 0xbe1cf01856c0384duL, 0xc08627c37585b468uL, 0xbe1cefa7dd05479euL, 0xc08627c5b5fad000uL, 0xbe1cef3ae8e50b93uL,
     0xc08627c7f5cdd750uL, 0xbe1ceea5f32fdd3auL, 0xc08627ca34ff2560uL, 0xbe1cef424caeb8d9uL, 0xc08627cc738f14f0uL, 0xbe1cf0194d07a81fuL,
     0xc08627ceb17e0070uL, 0xbe1cf20f452000c1uL, 0xc08627d0eecc4210uL, 0xbe1cf00e356218e4uL, 0xc08627d32b7a33a0uL, 0xbe1cef30484b4bcbuL,
     0xc08627d567882eb0uL, 0xbe1ceeea11a6641buL, 0xc08627d7a2f68c80uL, 0xbe1cf13492d5bd7buL, 0xc08627d9ddc5a618uL, 0xbe1ceeb7048fad96uL,
     0xc08627dc17f5d418uL, 0xbe1ceef0666f0477uL, 0xc08627de51876ee8uL, 0xbe1cf060d4b8b5c2uL, 0xc08627e08a7acea8uL, 0xbe1cf0b2a4b6ff8cuL,
     0xc08627e2c2d04b28uL, 0xbe1cf0e34809a875uL, 0xc08627e4fa883bf0uL, 0xbe1cf16bf74a3522uL, 0xc08627e731a2f848uL, 0xbe1cee6a24623d57uL,
     0xc08627e96820d718uL, 0xbe1cefc7b4f1528euL, 0xc08627eb9e022f18uL, 0xbe1cf163051f3548uL, 0xc08627edd34756b8uL, 0xbe1cef36b3366305uL,
     0xc08627f007f0a408uL, 0xbe1cf18134625550uL, 0xc08627f23bfe6cf0uL, 0xbe1cf0ec32ec1a11uL, 0xc08627f46f710700uL, 0xbe1ceeb3b64f3edcuL,
     0xc08627f6a248c778uL, 0xbe1cf0cd15805bc8uL, 0xc08627f8d4860368uL, 0xbe1cf20db3bddebeuL, 0xc08627fb06290f90uL, 0xbe1cf25188430e25uL,
     0xc08627fd37324070uL, 0xbe1ceea1713490f9uL, 0xc08627ff67a1ea28uL, 0xbe1cf159521d234cuL, 0xc0862801977860b8uL, 0xbe1cf24dfe50783buL,
     0xc0862803c6b5f7d0uL, 0xbe1ceef2ef89a60buL, 0xc0862805f55b02c8uL, 0xbe1cee7fc919d62cuL, 0xc08628082367d4c0uL, 0xbe1cf215a7fb513auL,
     0xc086280a50dcc0a8uL, 0xbe1cf0e4401c5ed4uL, 0xc086280c7dba1910uL, 0xbe1cf04ec734d256uL, 0xc086280eaa003050uL, 0xbe1cf010ad787feauL,
     0xc0862810d5af5880uL, 0xbe1cee622478393duL, 0xc086281300c7e368uL, 0xbe1cf01c7482564fuL, 0xc08628152b4a22a0uL, 0xbe1cf0de20d33536uL,
     0xc086281755366778uL, 0xbe1cef2edae5837duL, 0xc08628197e8d02f0uL, 0xbe1cf0a345318cc9uL, 0xc086281ba74e45d8uL, 0xbe1cf20085aa34b8uL,
     0xc086281dcf7a80c0uL, 0xbe1cef5fa845ad83uL, 0xc086281ff71203e0uL, 0xbe1cf050d1df69c4uL, 0xc08628221e151f48uL, 0xbe1ceffe43c035b9uL,
     0xc0862824448422b8uL, 0xbe1cf14f3018d3c2uL, 0xc08628266a5f5dc0uL, 0xbe1cef0a5fbae83duL, 0xc08628288fa71f98uL, 0xbe1ceff8a95b72a1uL,
     0xc086282ab45bb750uL, 0xbe1cef073aa9849buL, 0xc086282cd87d73a8uL, 0xbe1cef69b3835c02uL, 0xc086282efc0ca328uL, 0xbe1cf0bc139379a9uL,
     0xc08628311f099420uL, 0xbe1cef247a9ec596uL, 0xc086283341749490uL, 0xbe1cef74bbcc488auL, 0xc0862835634df248uL, 0xbe1cef4bc42e7b8euL,
     0xc08628378495fad0uL, 0xbe1cf136d4d5a810uL, 0xc0862839a54cfb80uL, 0xbe1cf0d290b24dd8uL, 0xc086283bc5734168uL, 0xbe1ceeebde8e0065uL,
     0xc086283de5091950uL, 0xbe1cf1a09f60aa1euL, 0xc0862840040ecfe0uL, 0xbe1cf0803947a234uL, 0xc08628422284b168uL, 0xbe1cf0abf7638127uL,
     0xc0862844406b0a08uL, 0xbe1cf0f73ee12058uL, 0xc08628465dc225a0uL, 0xbe1cf2079971b26cuL, 0xc08628487a8a4fe0uL, 0xbe1cee74957564b1uL,
     0xc086284a96c3d420uL, 0xbe1ceee77c1b7d43uL, 0xc086284cb26efd90uL, 0xbe1cf23addba6e09uL, 0xc086284ecd8c1730uL, 0xbe1cf199f4a1da60uL,
     0xc0862850e81b6bb0uL, 0xbe1cf09fdea81393uL, 0xc0862853021d4588uL, 0xbe1cf176adb417f7uL, 0xc08628551b91ef00uL, 0xbe1cf0f64f84a8dauL,
     0xc08628573479b220uL, 0xbe1ceec34cf49523uL, 0xc08628594cd4d8a8uL, 0xbe1cf16d60fbe0bbuL, 0xc086285b64a3ac40uL, 0xbe1cee8de7acfc7buL,
     0xc086285d7be67630uL, 0xbe1ceee6256cce8duL, 0xc086285f929d7fa0uL, 0xbe1cee7d66a3d8a5uL, 0xc0862861a8c91170uL, 0xbe1cf0bef8265792uL,
     0xc0862863be697458uL, 0xbe1cf097f890c6f8uL, 0xc0862865d37ef0c8uL, 0xbe1cf09502d5c3fcuL, 0xc0862867e809cf00uL, 0xbe1ceeffb239dac7uL,
     0xc0862869fc0a56f8uL, 0xbe1cf1fbfff95c98uL, 0xc086286c0f80d090uL, 0xbe1cefa57ad3eef7uL, 0xc086286e226d8348uL, 0xbe1cf22c58b9183duL,
     0xc086287034d0b690uL, 0xbe1ceff262d0a248uL, 0xc086287246aab180uL, 0xbe1cefa7bc194186uL, 0xc086287457fbbb08uL, 0xbe1cf06782d784d9uL,
     0xc086287668c419e0uL, 0xbe1cf1d44d0eaa07uL, 0xc086287879041490uL, 0xbe1cf034803c8a48uL, 0xc086287a88bbf158uL, 0xbe1cf08e84916b6fuL,
     0xc086287c97ebf650uL, 0xbe1cf0c4d3dc1bc7uL, 0xc086287ea6946958uL, 0xbe1cefb1e4625943uL, 0xc0862880b4b59010uL, 0xbe1cf143efdd1fd0uL,
     0xc0862882c24faff8uL, 0xbe1cee9896d016dauL, 0xc0862884cf630e38uL, 0xbe1cf2186072f2ccuL, 0xc0862886dbefeff0uL, 0xbe1cef9217633d34uL,
     0xc0862888e7f699e0uL, 0xbe1cf05603549486uL, 0xc086288af37750b0uL, 0xbe1cef50fff513d3uL, 0xc086288cfe7258c0uL, 0xbe1cf127713b32d0uL,
     0xc086288f08e7f650uL, 0xbe1cf05015520f3duL, 0xc086289112d86d58uL, 0xbe1cf12eb458b26fuL, 0xc08628931c4401a8uL, 0xbe1cf22eae2887eduL,
     0xc0862895252af6e0uL, 0xbe1cefdd6656dd2duL, 0xc08628972d8d9058uL, 0xbe1cf1048ea4e646uL, 0xc0862899356c1150uL, 0xbe1ceec4501167e9uL,
     0xc086289b3cc6bcb8uL, 0xbe1cf0ad52becc3fuL, 0xc086289d439dd568uL, 0xbe1cf0daa4e00e35uL, 0xc086289f49f19df8uL, 0xbe1cf00b80de8d6auL,
     0xc08628a14fc258c8uL, 0xbe1cf1bcf2ea8464uL, 0xc08628a355104818uL, 0xbe1cf0435e2782b0uL, 0xc08628a559dbade0uL, 0xbe1cf0e3e1a5f56cuL,
     0xc08628a75e24cbf8uL, 0xbe1cefed9d5a721duL, 0xc08628a961ebe3f8uL, 0xbe1cf0d2d74321e2uL, 0xc08628ab65313750uL, 0xbe1cf24200eb55e9uL,
     0xc08628ad67f50740uL, 0xbe1cf23e9d7cf979uL, 0xc08628af6a3794d0uL, 0xbe1cf23a088f421cuL, 0xc08628b16bf920e0uL, 0xbe1cef2c1de1ab32uL,
     0xc08628b36d39ec08uL, 0xbe1cf1abc231f7b2uL, 0xc08628b56dfa36d0uL, 0xbe1cf2074d5ba303uL, 0xc08628b76e3a4180uL, 0xbe1cf05cd5eed880uL}

    , {
       0x8000000000000000uL, 0xbf5ff802a9ab10e6uL, 0xbf6ff00aa2b10bc0uL, 0xbf77ee11ebd82e94uL, 0xbf7fe02a6b106789uL, 0xbf83e7295d25a7d9uL,
       0xbf87dc475f810a77uL, 0xbf8bcf712c74384cuL, 0xbf8fc0a8b0fc03e4uL, 0xbf91d7f7eb9eebe7uL, 0xbf93cea44346a575uL, 0xbf95c45a51b8d389uL,
       0xbf97b91b07d5b11buL, 0xbf99ace7551cc514uL, 0xbf9b9fc027af9198uL, 0xbf9d91a66c543cc4uL, 0xbf9f829b0e783300uL, 0xbfa0b94f7c196176uL,
       0xbfa1b0d98923d980uL, 0xbfa2a7ec2214e873uL, 0xbfa39e87b9febd60uL, 0xbfa494acc34d911cuL, 0xbfa58a5bafc8e4d5uL, 0xbfa67f94f094bd98uL,
       0xbfa77458f632dcfcuL, 0xbfa868a83083f6cfuL, 0xbfa95c830ec8e3ebuL, 0xbfaa4fe9ffa3d235uL, 0xbfab42dd711971bfuL, 0xbfac355dd0921f2duL,
       0xbfad276b8adb0b52uL, 0xbfae19070c276016uL, 0xbfaf0a30c01162a6uL, 0xbfaffae9119b9303uL, 0xbfb075983598e471uL, 0xbfb0ed839b5526feuL,
       0xbfb16536eea37ae1uL, 0xbfb1dcb263db1944uL, 0xbfb253f62f0a1417uL, 0xbfb2cb0283f5de1fuL, 0xbfb341d7961bd1d1uL, 0xbfb3b87598b1b6eeuL,
       0xbfb42edcbea646f0uL, 0xbfb4a50d3aa1b040uL, 0xbfb51b073f06183fuL, 0xbfb590cafdf01c28uL, 0xbfb60658a93750c4uL, 0xbfb67bb0726ec0fcuL,
       0xbfb6f0d28ae56b4cuL, 0xbfb765bf23a6be13uL, 0xbfb7da766d7b12cduL, 0xbfb84ef898e8282auL, 0xbfb8c345d6319b21uL, 0xbfb9375e55595edeuL,
       0xbfb9ab42462033aduL, 0xbfba1ef1d8061cd4uL, 0xbfba926d3a4ad563uL, 0xbfbb05b49bee43feuL, 0xbfbb78c82bb0eda1uL, 0xbfbbeba818146765uL,
       0xbfbc5e548f5bc743uL, 0xbfbcd0cdbf8c13e1uL, 0xbfbd4313d66cb35duL, 0xbfbdb5270187d927uL, 0xbfbe27076e2af2e6uL, 0xbfbe98b549671467uL,
       0xbfbf0a30c01162a6uL, 0xbfbf7b79fec37ddfuL, 0xbfbfec9131dbeabbuL, 0xbfc02ebb42bf3d4buL, 0xbfc0671512ca596euL, 0xbfc09f561ee719c3uL,
       0xbfc0d77e7cd08e59uL, 0xbfc10f8e422539b1uL, 0xbfc14785846742acuL, 0xbfc17f6458fca611uL, 0xbfc1b72ad52f67a0uL, 0xbfc1eed90e2dc2c3uL,
       0xbfc2266f190a5acbuL, 0xbfc25ded0abc6ad2uL, 0xbfc29552f81ff523uL, 0xbfc2cca0f5f5f251uL, 0xbfc303d718e47fd3uL, 0xbfc33af575770e4fuL,
       0xbfc371fc201e8f74uL, 0xbfc3a8eb2d31a376uL, 0xbfc3dfc2b0ecc62auL, 0xbfc41682bf727bc0uL, 0xbfc44d2b6ccb7d1euL, 0xbfc483bccce6e3dduL,
       0xbfc4ba36f39a55e5uL, 0xbfc4f099f4a230b2uL, 0xbfc526e5e3a1b438uL, 0xbfc55d1ad4232d6fuL, 0xbfc59338d9982086uL, 0xbfc5c940075972b9uL,
       0xbfc5ff3070a793d4uL, 0xbfc6350a28aaa758uL, 0xbfc66acd4272ad51uL, 0xbfc6a079d0f7aad2uL, 0xbfc6d60fe719d21duL, 0xbfc70b8f97a1aa75uL,
       0xbfc740f8f54037a5uL, 0xbfc7764c128f2127uL, 0xbfc7ab890210d909uL, 0xbfc7e0afd630c274uL, 0xbfc815c0a14357ebuL, 0xbfc84abb75865139uL,
       0xbfc87fa06520c911uL, 0xbfc8b46f8223625buL, 0xbfc8e928de886d41uL, 0xbfc91dcc8c340bdeuL, 0xbfc9525a9cf456b4uL, 0xbfc986d3228180cauL,
       0xbfc9bb362e7dfb83uL, 0xbfc9ef83d2769a34uL, 0xbfca23bc1fe2b563uL, 0xbfca57df28244dcduL, 0xbfca8becfc882f19uL, 0xbfcabfe5ae46124cuL,
       0xbfcaf3c94e80bff3uL, 0xbfcb2797ee46320cuL, 0xbfcb5b519e8fb5a4uL, 0xbfcb8ef670420c3buL, 0xbfcbc286742d8cd6uL, 0xbfcbf601bb0e44e2uL,
       0xbfcc2968558c18c1uL, 0xbfcc5cba543ae425uL, 0xbfcc8ff7c79a9a22uL, 0xbfccc320c0176502uL, 0xbfccf6354e09c5dcuL, 0xbfcd293581b6b3e7uL,
       0xbfcd5c216b4fbb91uL, 0xbfcd8ef91af31d5euL, 0xbfcdc1bca0abec7duL, 0xbfcdf46c0c722d2fuL, 0xbfce27076e2af2e6uL, 0xbfce598ed5a87e2fuL,
       0xbfce8c0252aa5a60uL, 0xbfcebe61f4dd7b0buL, 0xbfcef0adcbdc5936uL, 0xbfcf22e5e72f105duL, 0xbfcf550a564b7b37uL, 0xbfcf871b28955045uL,
       0xbfcfb9186d5e3e2buL, 0xbfcfeb0233e607ccuL, 0xbfd00e6c45ad501duL, 0xbfd0274dc16c232fuL, 0xbfd0402594b4d041uL, 0xbfd058f3c703ebc6uL,
       0xbfd071b85fcd590duL, 0xbfd08a73667c57afuL, 0xbfd0a324e27390e3uL, 0xbfd0bbccdb0d24bduL, 0xbfd0d46b579ab74buL, 0xbfd0ed005f657da4uL,
       0xbfd1058bf9ae4ad5uL, 0xbfd11e0e2dad9cb7uL, 0xbfd136870293a8b0uL, 0xbfd14ef67f88685auL, 0xbfd1675cababa60euL, 0xbfd17fb98e15095duL,
       0xbfd1980d2dd4236fuL, 0xbfd1b05791f07b49uL, 0xbfd1c898c16999fbuL, 0xbfd1e0d0c33716beuL, 0xbfd1f8ff9e48a2f3uL, 0xbfd211255986160cuL,
       0xbfd22941fbcf7966uL, 0xbfd241558bfd1404uL, 0xbfd2596010df763auL, 0xbfd27161913f853duL, 0xbfd2895a13de86a3uL, 0xbfd2a1499f762bc9uL,
       0xbfd2b9303ab89d25uL, 0xbfd2d10dec508583uL, 0xbfd2e8e2bae11d31uL, 0xbfd300aead06350cuL, 0xbfd31871c9544185uL, 0xbfd3302c16586588uL,
       0xbfd347dd9a987d55uL, 0xbfd35f865c93293euL, 0xbfd3772662bfd85buL, 0xbfd38ebdb38ed321uL, 0xbfd3a64c556945eauL, 0xbfd3bdd24eb14b6auL,
       0xbfd3d54fa5c1f710uL, 0xbfd3ecc460ef5f50uL, 0xbfd404308686a7e4uL, 0xbfd41b941cce0beeuL, 0xbfd432ef2a04e814uL, 0xbfd44a41b463c47cuL,
       0xbfd4618bc21c5ec2uL, 0xbfd478cd5959b3d9uL, 0xbfd49006804009d1uL, 0xbfd4a7373cecf997uL, 0xbfd4be5f957778a1uL, 0xbfd4d57f8fefe27fuL,
       0xbfd4ec973260026auL, 0xbfd503a682cb1cb3uL, 0xbfd51aad872df82duL, 0xbfd531ac457ee77euL, 0xbfd548a2c3add263uL, 0xbfd55f9107a43ee2uL,
       0xbfd5767717455a6cuL, 0xbfd58d54f86e02f2uL, 0xbfd5a42ab0f4cfe2uL, 0xbfd5baf846aa1b19uL, 0xbfd5d1bdbf5809cauL, 0xbfd5e87b20c2954auL,
       0xbfd5ff3070a793d4uL, 0xbfd615ddb4bec13cuL, 0xbfd62c82f2b9c795uL, 0x3fd61965cdb02c1fuL, 0x3fd602d08af091ecuL, 0x3fd5ec433d5c35aeuL,
       0x3fd5d5bddf595f30uL, 0x3fd5bf406b543db2uL, 0x3fd5a8cadbbedfa1uL, 0x3fd5925d2b112a59uL, 0x3fd57bf753c8d1fbuL, 0x3fd565995069514cuL,
       0x3fd54f431b7be1a9uL, 0x3fd538f4af8f72feuL, 0x3fd522ae0738a3d8uL, 0x3fd50c6f1d11b97cuL, 0x3fd4f637ebba9810uL, 0x3fd4e0086dd8bacauL,
       0x3fd4c9e09e172c3cuL, 0x3fd4b3c077267e9auL, 0x3fd49da7f3bcc41fuL, 0x3fd487970e958770uL, 0x3fd4718dc271c41buL, 0x3fd45b8c0a17df13uL,
       0x3fd44591e0539f49uL, 0x3fd42f9f3ff62642uL, 0x3fd419b423d5e8c7uL, 0x3fd403d086cea79cuL, 0x3fd3edf463c1683euL, 0x3fd3d81fb5946dbauL,
       0x3fd3c25277333184uL, 0x3fd3ac8ca38e5c5fuL, 0x3fd396ce359bbf54uL, 0x3fd3811728564cb2uL, 0x3fd36b6776be1117uL, 0x3fd355bf1bd82c8buL,
       0x3fd3401e12aecba1uL, 0x3fd32a84565120a8uL, 0x3fd314f1e1d35ce4uL, 0x3fd2ff66b04ea9d4uL, 0x3fd2e9e2bce12286uL, 0x3fd2d46602adcceeuL,
       0x3fd2bef07cdc9354uL, 0x3fd2a982269a3dbfuL, 0x3fd2941afb186b7cuL, 0x3fd27ebaf58d8c9duL, 0x3fd269621134db92uL, 0x3fd25410494e56c7uL,
       0x3fd23ec5991eba49uL, 0x3fd22981fbef797buL, 0x3fd214456d0eb8d4uL, 0x3fd1ff0fe7cf47a7uL, 0x3fd1e9e1678899f4uL, 0x3fd1d4b9e796c245uL,
       0x3fd1bf99635a6b95uL, 0x3fd1aa7fd638d33fuL, 0x3fd1956d3b9bc2fauL, 0x3fd180618ef18adfuL, 0x3fd16b5ccbacfb73uL, 0x3fd1565eed455fc3uL,
       0x3fd14167ef367783uL, 0x3fd12c77cd00713buL, 0x3fd1178e8227e47cuL, 0x3fd102ac0a35cc1cuL, 0x3fd0edd060b78081uL, 0x3fd0d8fb813eb1efuL,
       0x3fd0c42d676162e3uL, 0x3fd0af660eb9e279uL, 0x3fd09aa572e6c6d4uL, 0x3fd085eb8f8ae797uL, 0x3fd07138604d5862uL, 0x3fd05c8be0d9635auL,
       0x3fd047e60cde83b8uL, 0x3fd03346e0106062uL, 0x3fd01eae5626c691uL, 0x3fd00a1c6adda473uL, 0x3fcfeb2233ea07cduL, 0x3fcfc218be620a5euL,
       0x3fcf991c6cb3b379uL, 0x3fcf702d36777df0uL, 0x3fcf474b134df229uL, 0x3fcf1e75fadf9bdeuL, 0x3fcef5ade4dcffe6uL, 0x3fceccf2c8fe920auL,
       0x3fcea4449f04aaf5uL, 0x3fce7ba35eb77e2auL, 0x3fce530effe71012uL, 0x3fce2a877a6b2c12uL, 0x3fce020cc6235ab5uL, 0x3fcdd99edaf6d7e9uL,
       0x3fcdb13db0d48940uL, 0x3fcd88e93fb2f450uL, 0x3fcd60a17f903515uL, 0x3fcd38666871f465uL, 0x3fcd1037f2655e7buL, 0x3fcce816157f1988uL,
       0x3fccc000c9db3c52uL, 0x3fcc97f8079d44ecuL, 0x3fcc6ffbc6f00f71uL, 0x3fcc480c0005ccd1uL, 0x3fcc2028ab17f9b4uL, 0x3fcbf851c067555fuL,
       0x3fcbd087383bd8aduL, 0x3fcba8c90ae4ad19uL, 0x3fcb811730b823d2uL, 0x3fcb5971a213acdbuL, 0x3fcb31d8575bce3duL, 0x3fcb0a4b48fc1b46uL,
       0x3fcae2ca6f672bd4uL, 0x3fcabb55c31693aduL, 0x3fca93ed3c8ad9e3uL, 0x3fca6c90d44b704euL, 0x3fca454082e6ab05uL, 0x3fca1dfc40f1b7f1uL,
       0x3fc9f6c407089664uL, 0x3fc9cf97cdce0ec3uL, 0x3fc9a8778debaa38uL, 0x3fc981634011aa75uL, 0x3fc95a5adcf7017fuL, 0x3fc9335e5d594989uL,
       0x3fc90c6db9fcbcd9uL, 0x3fc8e588ebac2dbfuL, 0x3fc8beafeb38fe8cuL, 0x3fc897e2b17b19a5uL, 0x3fc871213750e994uL, 0x3fc84a6b759f512fuL,
       0x3fc823c16551a3c2uL, 0x3fc7fd22ff599d4fuL, 0x3fc7d6903caf5ad0uL, 0x3fc7b0091651528cuL, 0x3fc7898d85444c73uL, 0x3fc7631d82935a86uL,
       0x3fc73cb9074fd14duL, 0x3fc716600c914054uL, 0x3fc6f0128b756abcuL, 0x3fc6c9d07d203fc7uL, 0x3fc6a399dabbd383uL, 0x3fc67d6e9d785771uL,
       0x3fc6574ebe8c133auL, 0x3fc6313a37335d76uL, 0x3fc60b3100b09476uL, 0x3fc5e533144c1719uL, 0x3fc5bf406b543db2uL, 0x3fc59958ff1d52f1uL,
       0x3fc5737cc9018cdduL, 0x3fc54dabc26105d2uL, 0x3fc527e5e4a1b58duL, 0x3fc5022b292f6a45uL, 0x3fc4dc7b897bc1c8uL, 0x3fc4b6d6fefe22a4uL,
       0x3fc4913d8333b561uL, 0x3fc46baf0f9f5db7uL, 0x3fc4462b9dc9b3dcuL, 0x3fc420b32740fdd4uL, 0x3fc3fb45a59928ccuL, 0x3fc3d5e3126bc27fuL,
       0x3fc3b08b6757f2a9uL, 0x3fc38b3e9e027479uL, 0x3fc365fcb0159016uL, 0x3fc340c59741142euL, 0x3fc31b994d3a4f85uL, 0x3fc2f677cbbc0a96uL,
       0x3fc2d1610c86813auL, 0x3fc2ac55095f5c59uL, 0x3fc28753bc11aba5uL, 0x3fc2625d1e6ddf57uL, 0x3fc23d712a49c202uL, 0x3fc2188fd9807263uL,
       0x3fc1f3b925f25d41uL, 0x3fc1ceed09853752uL, 0x3fc1aa2b7e23f72auL, 0x3fc185747dbecf34uL, 0x3fc160c8024b27b1uL, 0x3fc13c2605c398c3uL,
       0x3fc1178e8227e47cuL, 0x3fc0f301717cf0fbuL, 0x3fc0ce7ecdccc28duL, 0x3fc0aa06912675d5uL, 0x3fc08598b59e3a07uL, 0x3fc06135354d4b18uL,
       0x3fc03cdc0a51ec0duL, 0x3fc0188d2ecf6140uL, 0x3fbfe89139dbd566uL, 0x3fbfa01c9db57ce2uL, 0x3fbf57bc7d9005dbuL, 0x3fbf0f70cdd992e3uL,
       0x3fbec739830a1120uL, 0x3fbe7f1691a32d3euL, 0x3fbe3707ee30487buL, 0x3fbdef0d8d466db9uL, 0x3fbda727638446a2uL, 0x3fbd5f55659210e2uL,
       0x3fbd179788219364uL, 0x3fbccfedbfee13a8uL, 0x3fbc885801bc4b23uL, 0x3fbc40d6425a5cb1uL, 0x3fbbf968769fca11uL, 0x3fbbb20e936d6974uL,
       0x3fbb6ac88dad5b1cuL, 0x3fbb23965a52ff00uL, 0x3fbadc77ee5aea8cuL, 0x3fba956d3ecade63uL, 0x3fba4e7640b1bc38uL, 0x3fba0792e9277cacuL,
       0x3fb9c0c32d4d2548uL, 0x3fb97a07024cbe74uL, 0x3fb9335e5d594989uL, 0x3fb8ecc933aeb6e8uL, 0x3fb8a6477a91dc29uL, 0x3fb85fd927506a48uL,
       0x3fb8197e2f40e3f0uL, 0x3fb7d33687c293c9uL, 0x3fb78d02263d82d3uL, 0x3fb746e100226ed9uL, 0x3fb700d30aeac0e1uL, 0x3fb6bad83c1883b6uL,
       0x3fb674f089365a7auL, 0x3fb62f1be7d77743uL, 0x3fb5e95a4d9791cbuL, 0x3fb5a3abb01ade25uL, 0x3fb55e10050e0384uL, 0x3fb518874226130auL,
       0x3fb4d3115d207eacuL, 0x3fb48dae4bc31018uL, 0x3fb4485e03dbdfaduL, 0x3fb403207b414b7fuL, 0x3fb3bdf5a7d1ee64uL, 0x3fb378dd7f749714uL,
       0x3fb333d7f8183f4buL, 0x3fb2eee507b40301uL, 0x3fb2aa04a44717a5uL, 0x3fb26536c3d8c369uL, 0x3fb2207b5c78549euL, 0x3fb1dbd2643d190buL,
       0x3fb1973bd1465567uL, 0x3fb152b799bb3cc9uL, 0x3fb10e45b3cae831uL, 0x3fb0c9e615ac4e17uL, 0x3fb08598b59e3a07uL, 0x3fb0415d89e74444uL,
       0x3faffa6911ab9301uL, 0x3faf723b517fc523uL, 0x3faeea31c006b87cuL, 0x3fae624c4a0b5e1buL, 0x3fadda8adc67ee4euL, 0x3fad52ed6405d86fuL,
       0x3faccb73cdddb2ccuL, 0x3fac441e06f72a9euL, 0x3fabbcebfc68f420uL, 0x3fab35dd9b58baaduL, 0x3faaaef2d0fb10fcuL, 0x3faa282b8a936171uL,
       0x3fa9a187b573de7cuL, 0x3fa91b073efd7314uL, 0x3fa894aa149fb343uL, 0x3fa80e7023d8ccc4uL, 0x3fa788595a3577bauL, 0x3fa70265a550e777uL,
       0x3fa67c94f2d4bb58uL, 0x3fa5f6e73078efb8uL, 0x3fa5715c4c03ceefuL, 0x3fa4ebf43349e26fuL, 0x3fa466aed42de3eauL, 0x3fa3e18c1ca0ae92uL,
       0x3fa35c8bfaa1306buL, 0x3fa2d7ae5c3c5baeuL, 0x3fa252f32f8d183fuL, 0x3fa1ce5a62bc353auL, 0x3fa149e3e4005a8duL, 0x3fa0c58fa19dfaaauL,
       0x3fa0415d89e74444uL, 0x3f9f7a9b16782856uL, 0x3f9e72bf2813ce51uL, 0x3f9d6b2725979802uL, 0x3f9c63d2ec14aaf2uL, 0x3f9b5cc258b718e6uL,
       0x3f9a55f548c5c43fuL, 0x3f994f6b99a24475uL, 0x3f98492528c8cabfuL, 0x3f974321d3d006d3uL, 0x3f963d6178690bd6uL, 0x3f9537e3f45f3565uL,
       0x3f9432a925980cc1uL, 0x3f932db0ea132e22uL, 0x3f9228fb1fea2e28uL, 0x3f912487a5507f70uL, 0x3f90205658935847uL, 0x3f8e38ce3033310cuL,
       0x3f8c317384c75f06uL, 0x3f8a2a9c6c170462uL, 0x3f882448a388a2aauL, 0x3f861e77e8b53fc6uL, 0x3f841929f96832f0uL, 0x3f82145e939ef1e9uL,
       0x3f8010157588de71uL, 0x3f7c189cbb0e27fbuL, 0x3f78121214586b54uL, 0x3f740c8a747878e2uL, 0x3f70080559588b35uL, 0x3f680904828985c0uL,
       0x3f60040155d5889euL, 0x3f50020055655889uL, 0x0000000000000000uL}

    , {
       0x3fc9999CACDB4D0AuL, 0xbfd0000148058EE1uL, 0x3fd55555555543C5uL, 0xbfdFFFFFFFFFF81FuL}

    , 0x000fffffffffffffuL, 0x3f50000000000000uL, 0xbfefffffffffffffuL, 0x7f3ffffffffff000uL, 0xfffffffffc000000uL, 0x3ff0000000000000uL,
        0x7fffffffffffffffuL, 0x3e00000000000000uL, 0xfffffffffffffc00uL, 0x3fe62E42FEFA0000uL, 0x3d7cf79abc9e0000uL, 0x4086a00000000000uL,
        0x408ff80000000000uL, 0x408ff00000000000uL, 0x7ff0000000000000uL, 0x7f40000000000000uL, 0x3fe62E42FEFA39EFuL, 0x41D0000000000000uL,
        0x3FD8000000000000uL, 0x3FD4000000000000uL, 0x3FD1800000000000uL, 0x3FCF800000000000uL, 0x3FE0000000000000uL, 0x7FEFFFFFFFFFFFFFuL,
        0x3F60000000000000uL, 0x8000000000000000uL, 0x403F000000000000uL, 0xFFFFFE0000000000uL, 0xFFFFFFFFF8000000uL, 0xFFFFFFFFFF000000uL,
        0xFFFFFFFFFFFFF000uL, 0x3E10000000000000uL, 0x0350000000000000uL, {0x7ff0000000000000uL, 0xfff0000000000000uL}

    , {0x3ff0000000000000uL, 0xbff0000000000000uL}

    , {0x0000000000000000uL, 0x8000000000000000uL}

};

static __constant _iml_v2_dp_union_t __datanh_la_CoutTab[4] = {

    0x00000000, 0x3FF00000,
    0x00000000, 0x00000000,
    0x00000000, 0x7FF00000,
    0x00000000, 0xFFF00000
};

__attribute__((always_inline))
inline int __internal_datanh_la_cout (double *a, double *r)
{
    int nRet = 0;

    double yM;
    double absx, y, yN;
    double m_hi, m_lo, m_tail, c, c1, c2, z, z1, z2;
    double polS, polC, x, rr[2];
    double d, rs, med, res_int, corr, res_hi, k0, k1, k2, k3, res_lo;

    int N, iAbsExp, i, NR;

    absx = (*a);
    (((_iml_v2_dp_union_t *) & absx)->dwords.hi_dword = (((_iml_v2_dp_union_t *) & absx)->dwords.hi_dword & 0x7FFFFFFF) | ((_iml_uint32_t) (0) << 31));
    {

        if ((((((_iml_v2_dp_union_t *) & *a)->dwords.hi_dword >> 20) & 0x7FF) == 0x7FF) &&
            (((((_iml_v2_dp_union_t *) & *a)->dwords.hi_dword & 0x000FFFFF) != 0x0) || ((((_iml_v2_dp_union_t *) & *a)->dwords.lo_dword) != 0x0)))
        {
            (*r) = (*a) * (*a);
            return nRet;
        }

        if ((((_iml_v2_dp_union_t *) & (absx))->hex[0] == ((__constant _iml_v2_dp_union_t *) & (((__constant double *) __datanh_la_CoutTab)[0]))->hex[0])
            && (((_iml_v2_dp_union_t *) & (absx))->hex[1] ==
                ((__constant _iml_v2_dp_union_t *) & (((__constant double *) __datanh_la_CoutTab)[0]))->hex[1]))
        {
            (*r) = ((*a) >= 0.0) ? ((__constant double *) __datanh_la_CoutTab)[2] : ((__constant double *) __datanh_la_CoutTab)[3];
            nRet = 2;
            return nRet;
        }

        {
            (*r) = ((__constant double *) __datanh_la_CoutTab)[2] * (((__constant double *) __datanh_la_CoutTab)[1]);
            nRet = 1;
            return nRet;
        }
    }
}

double __ocl_svml_atanh (double a)
{

    double va1;
    double vr1;
    unsigned int vm;

    double r;

    va1 = a;;

    {

        double ExpMask;
        double Two10;
        double A;
        double B;
        double X;
        double SgnMask;
        double XThreshold;
        double Xabs;
        double XMask;
        double Xl;
        double Mantissa;
        double DblRcp;
        double DblRcp1;
        unsigned long Expon;
        unsigned long ExpMask0;
        unsigned long ExpMask2;
        unsigned long ExpX;
        double FpExpX;
        unsigned int IExpon;
        double FpExpon;
        double MinLog1p;
        double MaxLog1p;
        double One;
        double XhMask;
        double R;
        unsigned long Index;
        double THL[2];
        double L2H;
        double L2L;
        double Kh;
        double Kl;
        double poly_coeff[5];
        double R2;
        double P34;
        double P12;
        double P;
        double Rh;
        double Rl;
        double Rl2;
        double Rlh;
        double dTinyMask;
        double dTinyRes;
        double dTinyRange;

        double Threshold;
        double Bias;
        double Bias1;
        double L2;
        double BMask;
        double DBias;
        double K;
        double T;

        double dU;
        double dUTmp;
        double dUHi;
        double dULo;
        double dV;
        double dVHi;
        double dVLo;
        double dTmp1;
        double dTmp2;
        double dTmp3;
        double dTmp4;
        double dZ;
        double dR;
        double dE;
        double dTopMask12;
        double dTopMask41;
        double dH;
        double dL;

        double dQHi;
        double dQLo;

        double dE2;
        double dE3;
        double dEE2;
        double dEE3;
        double dE25;
        double dD;
        double dHalf;
        double dResult;

        double dInput;
        double dSign;
        double dSpecialMask;
        unsigned long lSpecialMask;

        One = as_double (__internal_datanh_la_data.One);
        SgnMask = as_double (__internal_datanh_la_data.SgnMask);

        XThreshold = as_double (__internal_datanh_la_data.XThreshold);
        XhMask = as_double (__internal_datanh_la_data.XhMask);

        dInput = as_double ((as_ulong (va1) & as_ulong (SgnMask)));

        dSpecialMask = as_double ((unsigned long) (((!(dInput < One)) ? 0xffffffffffffffff : 0x0)));
        lSpecialMask = as_ulong (dSpecialMask);
        vm = 0;
        vm = lSpecialMask;

        dTinyRange = as_double (__internal_datanh_la_data.TinyRange);
        dTinyMask = as_double ((unsigned long) ((dInput < dTinyRange) ? 0xffffffffffffffff : 0x0));
        dTinyRes = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (va1, va1, va1);

        dSign = as_double (__internal_datanh_la_data.dSign);
        dSign = as_double ((as_ulong (va1) & as_ulong (dSign)));

        dTinyRes = as_double ((as_ulong (dTinyRes) | as_ulong (dSign)));

        dV = (dInput + dInput);

        dU = (One - dInput);
        dUTmp = (One - dU);
        dUTmp = (dUTmp - dInput);

        dTopMask41 = as_double (__internal_datanh_la_data.dTopMask41);

        dUHi = as_double ((as_ulong (dU) & as_ulong (dTopMask41)));
        dULo = (dU - dUHi);
        dULo = (dULo + dUTmp);

        dZ = ((double) (1.0f / ((float) (dUHi))));
        dTopMask12 = as_double (__internal_datanh_la_data.dTopMask12);
        dR = as_double ((as_ulong (dZ) & as_ulong (dTopMask12)));

        dE = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (-(dR), dUHi, One);
        dE = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (-(dR), dULo, dE);
        dVHi = as_double ((as_ulong (dV) & as_ulong (dTopMask41)));
        dVLo = (dV - dVHi);

        dQHi = (dR * dVHi);
        dQLo = (dR * dVLo);

        dE2 = (dE * dE);
        dE3 = (dE * dE2);
        dEE2 = (dE + dE2);
        dEE3 = (dE + dE3);
        dE25 = (dEE3 * dEE2);
        dD = (dE + dE25);
        dTmp1 = (dD * dQHi);
        dTmp2 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dD, dQLo, dQLo);
        dTmp3 = (dTmp1 + dTmp2);

        dH = (dQHi + dTmp3);
        dTmp4 = (dQHi - dH);
        dL = (dTmp4 + dTmp3);

        A = ((One > dH) ? One : dH);
        B = ((One < dH) ? One : dH);

        Xabs = as_double ((as_ulong (dH) & as_ulong (SgnMask)));
        XMask = as_double ((unsigned long) ((Xabs < XThreshold) ? 0xffffffffffffffff : 0x0));
        XhMask = as_double ((as_ulong (XhMask) | as_ulong (XMask)));

        X = (A + B);

        X = as_double ((as_ulong (X) & as_ulong (XhMask)));

        Xl = (A - X);
        Xl = (Xl + B);

        ExpMask = as_double (__internal_datanh_la_data.ExpMask);
        Two10 = as_double (__internal_datanh_la_data.Two10);

        Mantissa = as_double ((as_ulong (X) & as_ulong (ExpMask)));
        Mantissa = as_double ((as_ulong (Mantissa) | as_ulong (Two10)));

        MinLog1p = as_double (__internal_datanh_la_data.MinLog1p);
        MaxLog1p = as_double (__internal_datanh_la_data.MaxLog1p);
        ExpMask0 = (__internal_datanh_la_data.ExpMask0);
        ExpMask2 = (__internal_datanh_la_data.ExpMask2);

        DblRcp = ((double) (1.0f / ((float) (Mantissa))));

        Expon = as_ulong (X);
        ExpX = (Expon & ExpMask0);

        ExpX = (ExpMask2 - ExpX);

        Expon = ((unsigned long) (Expon) >> (52 - 32));
        IExpon = ((unsigned int) ((unsigned long) Expon >> 32));

        {
            double _rnd_d2p52;
            double _rnd_dRes_ub;
            unsigned long _rnd_i2p52 = 0x4338000000000000;
            _rnd_d2p52 = as_double (_rnd_i2p52);
            _rnd_dRes_ub = (DblRcp + _rnd_d2p52);
            _rnd_dRes_ub = (_rnd_dRes_ub - _rnd_d2p52);
            DblRcp = _rnd_dRes_ub;
        };

        FpExpX = as_double (ExpX);
        DblRcp1 = (FpExpX * DblRcp);

        FpExpon = ((double) ((int) (IExpon)));

        Index = as_ulong (DblRcp);
        Index = ((unsigned long) (Index) >> (52 - 9 - 3));
        T = as_double (((__constant unsigned long *) ((__constant char *) (&__internal_datanh_la_data.Log_LA_table[0]) - 0x408000))[Index >> 3]);
        Threshold = as_double (__internal_datanh_la_data.Threshold);
        Bias = as_double (__internal_datanh_la_data.Bias);
        Bias1 = as_double (__internal_datanh_la_data.Bias1);
        L2 = as_double (__internal_datanh_la_data.L2);
        BMask = as_double ((unsigned long) ((Threshold < DblRcp) ? 0xffffffffffffffff : 0x0));
        DBias = as_double ((as_ulong (BMask) & as_ulong (Bias)));
        DBias = as_double ((as_ulong (DBias) | as_ulong (Bias1)));;
        FpExpon = (FpExpon - DBias);
        K = (FpExpon * L2);

        Rh = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (X, DblRcp1, -(One));
        Rl = (Xl * DblRcp1);
        R = (Rh + Rl);

        Rl = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dL, DblRcp1, Rl);

        poly_coeff[4] = as_double (__internal_datanh_la_data.poly_coeff[0]);
        poly_coeff[3] = as_double (__internal_datanh_la_data.poly_coeff[1]);
        poly_coeff[2] = as_double (__internal_datanh_la_data.poly_coeff[2]);
        poly_coeff[1] = as_double (__internal_datanh_la_data.poly_coeff[3]);
        P34 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly_coeff[4], R, poly_coeff[3]);
        P12 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly_coeff[2], R, poly_coeff[1]);
        R2 = (R * R);
        P = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (P34, R2, P12);
        R = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (P, R2, R);
        R = (R + T);
        dResult = (R + K);
        dHalf = as_double (__internal_datanh_la_data.dHalf);
        dHalf = as_double ((as_ulong (dHalf) ^ as_ulong (dSign)));
        vr1 = (dHalf * dResult);
        vr1 = as_double ((((~as_ulong (dTinyMask)) & as_ulong (vr1)) | (as_ulong (dTinyMask) & as_ulong (dTinyRes))));
    }

    if ((vm) != 0)
    {
        double _vapi_arg1[1];
        double _vapi_res1[1];
        ((double *) _vapi_arg1)[0] = va1;
        ((double *) _vapi_res1)[0] = vr1;
        __internal_datanh_la_cout (_vapi_arg1, _vapi_res1);
        vr1 = ((double *) _vapi_res1)[0];
    };
    r = vr1;;

    return r;

}

/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF
typedef struct
{
    unsigned long Log_tbl_H[32];
    unsigned long Log_tbl_L[32];
    unsigned long Exp_tbl_H[16];
    unsigned long One;
    unsigned long C075;
    unsigned long poly_c1h;
    unsigned long poly_c2h;
    unsigned long poly_coeff9;
    unsigned long poly_coeff8;
    unsigned long poly_coeff7;
    unsigned long poly_coeff6;
    unsigned long poly_coeff5;
    unsigned long poly_coeff4;
    unsigned long poly_coeff3;
    unsigned long poly_coeff2;
    unsigned long poly_coeff1;
    unsigned long Shifter;
    unsigned long EMask;
    unsigned long poly_e_coeff6;
    unsigned long poly_e_coeff5;
    unsigned long poly_e_coeff4;
    unsigned long poly_e_coeff3;
    unsigned long poly_e_coeff2;
    unsigned long poly_e_coeff1;

    unsigned long Zero;
    unsigned long LIndexMask;
    unsigned long EIndexMask;
    unsigned long dAbsMask;
    unsigned long dDomainRange;
    unsigned long ExpMask;
} __internal_dpow_la_data_avx512_t;
static __constant __internal_dpow_la_data_avx512_t __internal_dpow_la_data_avx512 = {
    {
     0x0000000000000000uL, 0xbfa6bad3758e0000uL, 0xbfb663f6fac90000uL, 0xbfc08c588cda8000uL, 0xbfc5c01a39fc0000uL, 0xbfcacf5e2db50000uL,
     0xbfcfbc16b9028000uL, 0xbfd24407ab0e0000uL, 0xbfd49a784bcd0000uL, 0xbfd6e221cd9d0000uL, 0xbfd91bba891f0000uL, 0xbfdb47ebf7388000uL,
     0xbfdd6753e0330000uL, 0xbfdf7a8568cb0000uL, 0xbfe0c10500d64000uL, 0xbfe1bf311e95e000uL, 0x3fda8ff971810000uL, 0x3fd8a8980abfc000uL,
     0x3fd6cb0f6865c000uL, 0x3fd4f6fbb2cec000uL, 0x3fd32bfee3710000uL, 0x3fd169c053640000uL, 0x3fcf5fd8a9060000uL, 0x3fcbfc67a8000000uL,
     0x3fc8a8980abf8000uL, 0x3fc563dc29ff8000uL, 0x3fc22dadc2ab0000uL, 0x3fbe0b1ae8f30000uL, 0x3fb7d60496d00000uL, 0x3fb1bb32a6000000uL,
     0x3fa77394c9da0000uL, 0x3f9743ee86200000uL}
    , {
       0x0000000000000000uL, 0xbd5fb0e626c0de13uL, 0xbd33167ccc538261uL, 0x3d2871a7610e40bduL, 0x3d54bc302ffa76fbuL, 0x3d436c101ee13440uL,
       0x3d47f5dc57266758uL, 0xbd3ce60916e52e91uL, 0xbd5b8afe492bf6ffuL, 0xbd49bcaf1aa4168auL, 0xbd5708b4b2b5056cuL, 0xbd250520a377c7ecuL,
       0x3d55f101c141e670uL, 0xbd3b3b3864c60011uL, 0x3d45669df6a2b592uL, 0x3d5fe43895d8ac46uL, 0x3d44bc302ffa76fbuL, 0xbd266cccab240e90uL,
       0x3d41d406db502403uL, 0x3d3661e393a16b95uL, 0xbd51979a5db68722uL, 0xbd4d4f1b95e0ff45uL, 0x3d5f1a4847f7b278uL, 0xbd3667f21fa8423fuL,
       0x3d5e9933354dbf17uL, 0x3d56590643906f2auL, 0x3d5a4b69691d7994uL, 0xbd054cda62d3926euL, 0xbd512ce6312ebb82uL, 0x3d552743318a8a57uL,
       0xbd54e55443478fe0uL, 0xbd495539356f93dcuL}
    , {
       0x3ff0000000000000uL, 0x3ff0b5586cf9890fuL, 0x3ff172b83c7d517buL, 0x3ff2387a6e756238uL, 0x3ff306fe0a31b715uL, 0x3ff3dea64c123422uL,
       0x3ff4bfdad5362a27uL, 0x3ff5ab07dd485429uL, 0x3ff6a09e667f3bcduL, 0x3ff7a11473eb0187uL, 0x3ff8ace5422aa0dbuL, 0x3ff9c49182a3f090uL,
       0x3ffae89f995ad3aduL, 0x3ffc199bdd85529cuL, 0x3ffd5818dcfba487uL, 0x3ffea4afa2a490dauL}

    , 0x3ff0000000000000uL, 0x3ff8000000000000uL, 0x3ff71547652b82feuL, 0xbfe71547652b82feuL, 0x3fc48799f5374b96uL, 0xbfc717e59a559f5duL,
        0x3fca6175fca19d03uL, 0xbfcec709bc8fb8dfuL, 0x3fd2776c50f03272uL, 0xbfd71547652bc9eauL, 0x3fdec709dc3a03fcuL, 0x3c636b5ddffe5b67uL,
        0x3c7777f3ee5e912fuL, 0x42f8000000003ff0uL, 0xbfffffffffffffffuL, 0x3f24a1d7f58c2d59uL, 0x3f55d7472783d279uL, 0x3f83b2ad1b14ebaauL,
        0x3fac6b08d4ad8eb9uL, 0x3fcebfbdff84554duL, 0x3fe62e42fefa398buL, 0x0000000000000000uL, 0x00000000000000f8uL, 0x0000000000000078uL,
        0x7fffffffffffffffuL, 0x408FEC0000000000uL, 0x7ff0000000000000uL
};

typedef struct
{

    unsigned long hsw_log2_table[1026];
    unsigned long hsw_dTe[256];

    unsigned long hsw_dMantMask;
    unsigned long hsw_dOne;
    unsigned long hsw_dCvtMask;
    unsigned long hsw_dMinNorm;
    unsigned long hsw_dMaxNorm;
    unsigned long hsw_lRndBit;
    unsigned long hsw_lRndMask;

    unsigned long hsw_dc6;
    unsigned long hsw_dc5;
    unsigned long hsw_dc4;
    unsigned long hsw_dc3;
    unsigned long hsw_dc1;
    unsigned long hsw_dc1h;
    unsigned long hsw_dc2;

    unsigned long hsw_dAbsMask;
    unsigned long hsw_dDomainRange;
    unsigned long hsw_dShifter;
    unsigned long hsw_dIndexMask;

    unsigned long hsw_dce4;
    unsigned long hsw_dce3;
    unsigned long hsw_dce2;
    unsigned long hsw_dce1;

    unsigned long rcp_t1[1026];
    unsigned long log2_t1[2050];
    unsigned long exp2_tbl[256];

    unsigned long clv[7];
    unsigned long cev[5];

    unsigned long iMantissaMask;
    unsigned long i3fe7fe0000000000;
    unsigned long dbOne;
    unsigned long iffffffff00000000;
    unsigned long db2p20_2p19;
    unsigned long iHighMask;
    unsigned long LHN;
    unsigned long ifff0000000000000;
    unsigned long db2p45_2p44;

    unsigned long NEG_INF;
    unsigned long NEG_ZERO;
    unsigned long d2pow52;
    unsigned long d1div2pow111;

    unsigned int HIDELTA;
    unsigned int LORANGE;
    unsigned int ABSMASK;
    unsigned int INF;
    unsigned int DOMAINRANGE;

    unsigned int iIndexMask;
    unsigned int iIndexAdd;

    unsigned int i3fe7fe00;
    unsigned int i2p20_2p19;
    unsigned int iOne;
    unsigned int jIndexMask;
} __internal_dpow_la_data_t;
static __constant __internal_dpow_la_data_t __internal_dpow_la_data = {

    {

     0xc08ff00000000000uL, 0x0000000000000000uL,
     0xc08ff005c3e0ffc2uL, 0xbd33ab2631d4676duL,
     0xc08ff00b84e236bcuL, 0xbd4563ba56cde925uL,
     0xc08ff01143068126uL, 0x3d11790209e88471uL,
     0xc08ff016fe50b6eeuL, 0xbd408517f8e37b00uL,
     0xc08ff01cb6c3abd0uL, 0xbd44558b51cada94uL,
     0xc08ff0226c622f52uL, 0xbd3ec312ed069b24uL,
     0xc08ff0281f2f0cd0uL, 0xbd374a4cb0be9e8auL,
     0xc08ff02dcf2d0b86uL, 0x3d26eb3ac8ec0ef7uL,
     0xc08ff0337c5eee92uL, 0xbd45984a60ff3d2fuL,
     0xc08ff03926c7750auL, 0xbd0f0cccdd01ee2fuL,
     0xc08ff03ece6959f0uL, 0xbd3a5671e1bd4ae8uL,
     0xc08ff0447347544cuL, 0xbd3a0976c0a2827duL,
     0xc08ff04a1564172auL, 0x3d1e14ebaf30c95euL,
     0xc08ff04fb4c251a0uL, 0xbd46898809d2dc10uL,
     0xc08ff0555164aee2uL, 0xbd4355e6ecb8e0f1uL,
     0xc08ff05aeb4dd63cuL, 0x3cf3c6764fc87b4auL,
     0xc08ff06082806b1cuL, 0xbd4532c412ba94dbuL,
     0xc08ff06616ff0d24uL, 0xbd4465182838ed44uL,
     0xc08ff06ba8cc5824uL, 0xbd47dc6d46384b31uL,
     0xc08ff07137eae42auL, 0xbd35af7a7c7c34f3uL,
     0xc08ff076c45d4584uL, 0x3d18a0e14f76d994uL,
     0xc08ff07c4e260cc8uL, 0xbd44e7e87341aeeeuL,
     0xc08ff081d547c6e4uL, 0xbd153121e9af5428uL,
     0xc08ff08759c4fd14uL, 0xbd3f9ab3cf74babauL,
     0xc08ff08cdba034fauL, 0xbd3f09941811b2eeuL,
     0xc08ff0925adbf09auL, 0xbd3a3c89a2cf3516uL,
     0xc08ff097d77aae66uL, 0x3d291b415eeb24eduL,
     0xc08ff09d517ee940uL, 0x3d2c7a4ff65ddbc9uL,
     0xc08ff0a2c8eb1886uL, 0xbd385a047f97bb3euL,
     0xc08ff0a83dc1b01auL, 0x3d1124ac34b21259uL,
     0xc08ff0adb005205euL, 0xbd34f286d207e2c8uL,
     0xc08ff0b31fb7d648uL, 0xbd33167ccc538261uL,
     0xc08ff0b88cdc3b5euL, 0xbd4542fe4ce30d63uL,
     0xc08ff0bdf774b5c4uL, 0xbd41409e20d7191buL,
     0xc08ff0c35f83a83cuL, 0xbd40638b5ff73edfuL,
     0xc08ff0c8c50b7232uL, 0x3d294aa31b9b6d65uL,
     0xc08ff0ce280e6fbauL, 0xbd38723279ebfab6uL,
     0xc08ff0d3888ef9a4uL, 0xbd124fad116078efuL,
     0xc08ff0d8e68f6572uL, 0xbd437350d69ea580uL,
     0xc08ff0de4212056cuL, 0xbd45dd31d962d373uL,
     0xc08ff0e39b19289euL, 0x3d058b34834a501euL,
     0xc08ff0e8f1a71adcuL, 0xbd06d26859c7991euL,
     0xc08ff0ee45be24d0uL, 0xbd3ddb7886f88587uL,
     0xc08ff0f397608bfcuL, 0xbd42d90e5edaeceeuL,
     0xc08ff0f8e69092beuL, 0xbd40c5eacb577b4auL,
     0xc08ff0fe33507858uL, 0xbce49209a68c72a1uL,
     0xc08ff1037da278f2uL, 0xbd30e0f9c896007duL,
     0xc08ff108c588cda8uL, 0x3d2871a7610e40bduL,
     0xc08ff10e0b05ac84uL, 0xbd31da156756faaduL,
     0xc08ff1134e1b4890uL, 0xbd28b7fcd690403euL,
     0xc08ff1188ecbd1d0uL, 0xbd46be4a29c44115uL,
     0xc08ff11dcd197552uL, 0xbd36f6bd48a860f0uL,
     0xc08ff12309065d28uL, 0xbd47913e788c5887uL,
     0xc08ff1284294b07auL, 0xbd28fe35da2ab291uL,
     0xc08ff12d79c6937euL, 0xbd3fb9b1aaf54bccuL,
     0xc08ff132ae9e278auL, 0xbd3c343ea3e580ebuL,
     0xc08ff137e11d8b10uL, 0xbd3f1140264356b8uL,
     0xc08ff13d1146d9a8uL, 0xbd34c7e0166e1f56uL,
     0xc08ff1423f1c2c12uL, 0xbd3d449e80431d92uL,
     0xc08ff1476a9f983euL, 0xbd474d3138e94164uL,
     0xc08ff14c93d33152uL, 0x3d2370693afbcdb1uL,
     0xc08ff151bab907a6uL, 0x3d1badba7fbb3d20uL,
     0xc08ff156df5328d6uL, 0x3d2cea9347cb6655uL,
     0xc08ff15c01a39fbcuL, 0xbd46879fa00b120auL,
     0xc08ff16121ac7480uL, 0xbd43cf0ff16ff990uL,
     0xc08ff1663f6fac90uL, 0xbd43167ccc538261uL,
     0xc08ff16b5aef4aaeuL, 0xbd2f7081b8e33aaduL,
     0xc08ff170742d4ef0uL, 0xbd13f94e00e7d6bcuL,
     0xc08ff1758b2bb6c8uL, 0x3d22280434bda911uL,
     0xc08ff17a9fec7d06uL, 0x3d1108740d92f890uL,
     0xc08ff17fb27199deuL, 0xbd416d18135d3266uL,
     0xc08ff184c2bd02f0uL, 0xbd1d97ee9124773buL,
     0xc08ff189d0d0ab42uL, 0xbd40ccd0edd00e4cuL,
     0xc08ff18edcae8352uL, 0xbd36d76b9a843329uL,
     0xc08ff193e6587910uL, 0xbd210f7ac89c6f2duL,
     0xc08ff198edd077e6uL, 0xbd40df02face8ca9uL,
     0xc08ff19df31868c0uL, 0xbd41d4cc2f68b868uL,
     0xc08ff1a2f632320cuL, 0x3d2e54d71deb636auL,
     0xc08ff1a7f71fb7bauL, 0xbd373af6b5487f35uL,
     0xc08ff1acf5e2db4euL, 0xbd3927dfc23d9780uL,
     0xc08ff1b1f27d7bd8uL, 0x3d2601ccfac2b557uL,
     0xc08ff1b6ecf175f8uL, 0xbd45e96bed8cce30uL,
     0xc08ff1bbe540a3f0uL, 0xbd1b76a46f31880auL,
     0xc08ff1c0db6cdd94uL, 0xbd3bdc81c4db3134uL,
     0xc08ff1c5cf77f860uL, 0xbd304cc6600a133euL,
     0xc08ff1cac163c770uL, 0xbd3b912d8994b162uL,
     0xc08ff1cfb1321b8cuL, 0xbd20009770ea1465uL,
     0xc08ff1d49ee4c326uL, 0x3d2a40dc2d2a6bf7uL,
     0xc08ff1d98a7d8a60uL, 0xbd269affffe47644uL,
     0xc08ff1de73fe3b14uL, 0xbd301dc37c84e79auL,
     0xc08ff1e35b689cd2uL, 0xbd2953e61f15bd9buL,
     0xc08ff1e840be74e6uL, 0xbd34998f93e7aa3cuL,
     0xc08ff1ed2401865euL, 0x3cf5c14e55f57802uL,
     0xc08ff1f205339208uL, 0xbd3e4e8eea54ce63uL,
     0xc08ff1f6e4565680uL, 0x3d0aaa72ba2c6ba2uL,
     0xc08ff1fbc16b9026uL, 0xbd30144751b3314fuL,
     0xc08ff2009c74f930uL, 0x3d2a15a5b343a140uL,
     0xc08ff205757449a0uL, 0xbd398eec5e85b29fuL,
     0xc08ff20a4c6b3756uL, 0xbd1b361c7dddadb6uL,
     0xc08ff20f215b7606uL, 0xbcc2de0634d33aa9uL,
     0xc08ff213f446b744uL, 0xbce024b5b4e89254uL,
     0xc08ff218c52eaa84uL, 0xbd451d49f63f4830uL,
     0xc08ff21d9414fd24uL, 0x3d1f4c2417f39394uL,
     0xc08ff22260fb5a60uL, 0xbd46eb9612e0b4f3uL,
     0xc08ff2272be36b6cuL, 0xbd1a5bd9bcda22fduL,
     0xc08ff22bf4ced760uL, 0xbd41feb2fc708a78uL,
     0xc08ff230bbbf4350uL, 0x3d13045428f88499uL,
     0xc08ff23580b6523euL, 0xbcfc14a31ce1b7e3uL,
     0xc08ff23a43b5a52auL, 0xbd38c9a2f2dbcaf9uL,
     0xc08ff23f04bedb12uL, 0x3d1ecd417972c083uL,
     0xc08ff243c3d390eeuL, 0xbd38e36471414f76uL,
     0xc08ff24880f561c0uL, 0xbd3ce60916e52e91uL,
     0xc08ff24d3c25e68euL, 0x3d1d406db502402duL,
     0xc08ff251f566b664uL, 0xbd3a0d8c0e85a909uL,
     0xc08ff256acb96662uL, 0xbd2dafbfd96d5335uL,
     0xc08ff25b621f89b2uL, 0xbd455ede26f47b19uL,
     0xc08ff260159ab196uL, 0xbd461f2e47488cf1uL,
     0xc08ff264c72c6d64uL, 0xbd406b35c7c781dbuL,
     0xc08ff26976d64a8cuL, 0xbd20c369fc5a3d9buL,
     0xc08ff26e2499d49auL, 0x3d20993376649b50uL,
     0xc08ff272d078953auL, 0x3d1664deafdbfed5uL,
     0xc08ff2777a74143cuL, 0x3d282b53e791792duL,
     0xc08ff27c228dd794uL, 0x3ccc79237996a42buL,
     0xc08ff280c8c76360uL, 0xbd3125d6cbcd1095uL,
     0xc08ff2856d2239eauL, 0xbd3194cfcc6c23cfuL,
     0xc08ff28a0f9fdbaauL, 0x3cee35952fb0019cuL,
     0xc08ff28eb041c748uL, 0xbd2286fbc7f749ffuL,
     0xc08ff2934f0979a2uL, 0xbd4715fc9257edffuL,
     0xc08ff297ebf86dd0uL, 0xbd35dcccaf649933uL,
     0xc08ff29c87101d1euL, 0xbd46d3f77ae3858buL,
     0xc08ff2a12051ff1cuL, 0xbd0432648cfc8738uL,
     0xc08ff2a5b7bf8992uL, 0xbd3acdf73d83987fuL,
     0xc08ff2aa4d5a3092uL, 0xbd2e6c522ceda3fbuL,
     0xc08ff2aee123666euL, 0xbd4195620f0359d8uL,
     0xc08ff2b3731c9bc4uL, 0xbd3c70f15d3ebabduL,
     0xc08ff2b803473f7auL, 0xbd3a1e7e802c4828uL,
     0xc08ff2bc91a4bec4uL, 0xbd4572ca23a96c48uL,
     0xc08ff2c11e368528uL, 0xbd415b2de01cea41uL,
     0xc08ff2c5a8fdfc7cuL, 0xbd47dc11ebf92a98uL,
     0xc08ff2ca31fc8ceeuL, 0xbd474dca44f1db91uL,
     0xc08ff2ceb9339d04uL, 0x3cfb88755d6ca189uL,
     0xc08ff2d33ea4919auL, 0xbd32e1a3152150d3uL,
     0xc08ff2d7c250cdf0uL, 0xbd206adfcaa4bcf5uL,
     0xc08ff2dc4439b3a2uL, 0x3d290d43956fa5d8uL,
     0xc08ff2e0c460a2aeuL, 0x3d27158a37417c3auL,
     0xc08ff2e542c6f978uL, 0xbd1829434d994a2auL,
     0xc08ff2e9bf6e14ccuL, 0xbd2c3e1e30d370eauL,
     0xc08ff2ee3a574fdeuL, 0xbd4677c8dfd9aa24uL,
     0xc08ff2f2b3840452uL, 0xbd2788eba5c173eeuL,
     0xc08ff2f72af58a34uL, 0xbd4588aec6dfa7dcuL,
     0xc08ff2fba0ad3808uL, 0xbd47fe42f19c5879uL,
     0xc08ff30014ac62c4uL, 0x3d2d5e6a8a4fb059uL,
     0xc08ff30486f45dceuL, 0xbd0edb9d09608783uL,
     0xc08ff308f7867b0cuL, 0xbd18dc7c094eee51uL,
     0xc08ff30d66640adauL, 0xbd46028f37225746uL,
     0xc08ff311d38e5c16uL, 0xbd212d25b3252647uL,
     0xc08ff3163f06bc16uL, 0xbd3906944ba567f4uL,
     0xc08ff31aa8ce76b8uL, 0xbd2b8d59e8492d6euL,
     0xc08ff31f10e6d65auL, 0xbd339eec34ce3ce3uL,
     0xc08ff323775123e2uL, 0xbd3c22d2cad415aeuL,
     0xc08ff327dc0ea6beuL, 0xbd42ce2af5839ab8uL,
     0xc08ff32c3f20a4e8uL, 0xbd03719eb3af5b8duL,
     0xc08ff330a08862e2uL, 0xbd3feed12980ee19uL,
     0xc08ff335004723c4uL, 0xbd2979a5db68721duL,
     0xc08ff3395e5e2932uL, 0x3cf7159b944f7fd7uL,
     0xc08ff33dbaceb364uL, 0xbd377e236c73e71buL,
     0xc08ff342159a012auL, 0xbd4568bb43ac99bbuL,
     0xc08ff3466ec14fecuL, 0xbcf4275f1035e5e8uL,
     0xc08ff34ac645dba6uL, 0xbd3cc58a505d117auL,
     0xc08ff34f1c28def8uL, 0x3d10bad7dfa568f7uL,
     0xc08ff353706b9318uL, 0xbd3c27e675df639duL,
     0xc08ff357c30f2fe4uL, 0x3d06e3cb71b554e7uL,
     0xc08ff35c1414ebd4uL, 0xbd40c353cb7112a5uL,
     0xc08ff360637dfc0cuL, 0xbd30d199805b0aecuL,
     0xc08ff364b14b9450uL, 0xbd381e2a51761f86uL,
     0xc08ff368fd7ee710uL, 0xbd250520a377c7ecuL,
     0xc08ff36d48192564uL, 0xbcef941453836236uL,
     0xc08ff371911b7f10uL, 0xbd39e65cd77582e2uL,
     0xc08ff375d887228auL, 0x3d201640f615fa5cuL,
     0xc08ff37a1e5d3cf2uL, 0xbce855a216719009uL,
     0xc08ff37e629efa1euL, 0xbd3ae66b65d78df9uL,
     0xc08ff382a54d8498uL, 0xbd45cb804b949696uL,
     0xc08ff386e66a05a0uL, 0xbd33de15e265b5d9uL,
     0xc08ff38b25f5a52auL, 0xbd46acfcfdca95deuL,
     0xc08ff38f63f189eauL, 0xbd1a3f6c066ebdd4uL,
     0xc08ff393a05ed948uL, 0xbd3ecf4dff1e8ea2uL,
     0xc08ff397db3eb770uL, 0xbd40d40bb2010158uL,
     0xc08ff39c1492474auL, 0xbd40f992ba145dcfuL,
     0xc08ff3a04c5aaa80uL, 0xbd346fab3fa1a144uL,
     0xc08ff3a48299017euL, 0xbd23ea90adf6a54auL,
     0xc08ff3a8b74e6b74uL, 0xbd449e1389f86468uL,
     0xc08ff3acea7c065cuL, 0xbd441dfc7d7c3321uL,
     0xc08ff3b11c22eef6uL, 0xbd148ad9b560f3b7uL,
     0xc08ff3b54c4440cauL, 0x3cf1bfb62d6a3aa8uL,
     0xc08ff3b97ae1162euL, 0xbd2ac444ea257ffauL,
     0xc08ff3bda7fa8846uL, 0xbd39313aec658458uL,
     0xc08ff3c1d391af06uL, 0x3d2a140de4db9aaeuL,
     0xc08ff3c5fda7a12euL, 0xbd24c06f912ab9d1uL,
     0xc08ff3ca263d7456uL, 0xbd426152c271eb36uL,
     0xc08ff3ce4d543ceauL, 0xbd33483146784bd2uL,
     0xc08ff3d272ed0e28uL, 0xbd44640a8fec6a2euL,
     0xc08ff3d69708fa2auL, 0xbd479ca7cb93cc08uL,
     0xc08ff3dab9a911e2uL, 0xbd3cc65b96825ec6uL,
     0xc08ff3dedace651cuL, 0xbd2103e8f00d41c8uL,
     0xc08ff3e2fa7a0280uL, 0xbd3ebdb1bbaf9ab0uL,
     0xc08ff3e718acf798uL, 0xbd350343f8df4b43uL,
     0xc08ff3eb356850cauL, 0xbd3db11aa6a7cdeauL,
     0xc08ff3ef50ad1960uL, 0xbd3b3b3864c60011uL,
     0xc08ff3f36a7c5b86uL, 0xbd3310f9839f068auL,
     0xc08ff3f782d7204cuL, 0xbd40144751b3314fuL,
     0xc08ff3fb99be6faauL, 0xbd429875b0e43fd8uL,
     0xc08ff3ffaf335080uL, 0x3cf9518ce032f41duL,
     0xc08ff403c336c894uL, 0x3d29ab66b62c5ca8uL,
     0xc08ff407d5c9dc98uL, 0xbd437fc8cafdef46uL,
     0xc08ff40be6ed9030uL, 0xbd2515e1cacac36euL,
     0xc08ff40ff6a2e5e6uL, 0xbd27f33943464056uL,
     0xc08ff41404eadf38uL, 0xbd1cb6f70109b0f1uL,
     0xc08ff41811c67c94uL, 0x3d24dc166e0e0c68uL,
     0xc08ff41c1d36bd58uL, 0xbd3d990d1e0f6657uL,
     0xc08ff420273c9fdcuL, 0xbcfea92d9e0e8ac2uL,
     0xc08ff4242fd92166uL, 0xbd303cf98ab4e537uL,
     0xc08ff428370d3e38uL, 0xbd2fbc00d8d6cbcfuL,
     0xc08ff42c3cd9f18auL, 0xbd2fd3fe3499ea9fuL,
     0xc08ff4304140358euL, 0xbd3532c412ba94dbuL,
     0xc08ff43444410372uL, 0xbd1f5ab329b483ecuL,
     0xc08ff43845dd535euL, 0xbd40444ebaaf2894uL,
     0xc08ff43c46161c7cuL, 0xbd35897d184aaac4uL,
     0xc08ff44044ec54f2uL, 0xbd1d4f639bb5cdf6uL,
     0xc08ff4444260f1e6uL, 0xbd467d28344c2ff0uL,
     0xc08ff4483e74e786uL, 0xbcccb52b4581174duL,
     0xc08ff44c392928fauL, 0xbd449eb852b25382uL,
     0xc08ff450327ea878uL, 0xbd450e785694a8c6uL,
     0xc08ff4542a765738uL, 0xbd2410f5d3161a62uL,
     0xc08ff45821112578uL, 0xbcc81e2b378ff59duL,
     0xc08ff45c16500280uL, 0xbd3e6009faee4be8uL,
     0xc08ff4600a33dca6uL, 0x3d12b628e2d05d76uL,
     0xc08ff463fcbda144uL, 0xbd3cbb828084fcb1uL,
     0xc08ff467edee3cc8uL, 0xbd4085c5870d5301uL,
     0xc08ff46bddc69aaauL, 0xbd4475780e47156buL,
     0xc08ff46fcc47a574uL, 0xbcdbc76a2753b99buL,
     0xc08ff473b97246bcuL, 0xbd2012f1593ee62auL,
     0xc08ff477a547672euL, 0xbd3d30c3d2643639uL,
     0xc08ff47b8fc7ee8auL, 0xbd062c45c4bc31c9uL,
     0xc08ff47f78f4c3a0uL, 0xbd22642415d47384uL,
     0xc08ff48360cecc5auL, 0x3d2372fd3ff3197buL,
     0xc08ff4874756edb4uL, 0xbd4668c543d0b42buL,
     0xc08ff48b2c8e0bcauL, 0xbd33f65cadbe0d26uL,
     0xc08ff48f107509cauL, 0x3cfbfbf899cf2b3cuL,
     0xc08ff492f30cc9feuL, 0xbd307470f69809ccuL,
     0xc08ff496d4562dceuL, 0xbd44115a1a340462uL,
     0xc08ff49ab45215c0uL, 0xbcff5369fdf426cfuL,
     0xc08ff49e93016172uL, 0xbd3fc02bc277071duL,
     0xc08ff4a27064efa8uL, 0xbd4728da988cc139uL,
     0xc08ff4a64c7d9e44uL, 0xbd458147cf67745euL,
     0xc08ff4aa274c4a4auL, 0xbd22100986691daauL,
     0xc08ff4ae00d1cfdeuL, 0xbd36879fa00b120auL,
     0xc08ff4b1d90f0a4cuL, 0xbd40b68fc634db41uL,
     0xc08ff4b5b004d404uL, 0xbd3c03254a7145e3uL,
     0xc08ff4b985b4069cuL, 0xbcf4f144da6e4533uL,
     0xc08ff4bd5a1d7ad0uL, 0x3d1b3d7b0e65d2ceuL,
     0xc08ff4c12d420886uL, 0x3d0dd3d30f5deaa7uL,
     0xc08ff4c4ff2286ceuL, 0x3d20dc60dc5befecuL,
     0xc08ff4c8cfbfcbe0uL, 0xbd47f6a1ab3efbbeuL,
     0xc08ff4cc9f1aad26uL, 0xbd429b21ae4817e9uL,
     0xc08ff4d06d33ff32uL, 0x3d256a9ae5dca5a3uL,
     0xc08ff4d43a0c95c2uL, 0x3cf38bc99b3611ceuL,
     0xc08ff4d805a543c8uL, 0xbd0c6d2c37daf317uL,
     0xc08ff4dbcffedb64uL, 0xbd262404772a151duL,
     0xc08ff4df991a2de8uL, 0xbd11c0de7b779cb3uL,
     0xc08ff4e360f80bd6uL, 0xbd4424a06f870b9euL,
     0xc08ff4e7279944e8uL, 0xbd3a69393bab4fd0uL,
     0xc08ff4eaecfea808uL, 0xbd266cccab240e90uL,
     0xc08ff4eeb1290356uL, 0xbd38e9b57298d22fuL,
     0xc08ff4f27419242cuL, 0x3d2eddd33ea4d6f1uL,
     0xc08ff4f635cfd714uL, 0xbd476e0ed8a042beuL,
     0xc08ff4f9f64de7dcuL, 0xbce66ae2a7ada553uL,
     0xc08ff4fdb5942180uL, 0xbd0cd57d9d86514euL,
     0xc08ff50173a34e3cuL, 0xbd42efafb4bec72buL,
     0xc08ff505307c378auL, 0xbd1a46dbdcc762d3uL,
     0xc08ff508ec1fa61auL, 0xbd354b383b0e8a55uL,
     0xc08ff50ca68e61e0uL, 0x3d2c7d469ea019aduL,
     0xc08ff5105fc93208uL, 0xbd264adb1adca9a8uL,
     0xc08ff51417d0dd04uL, 0x3ce5c601f0626dc8uL,
     0xc08ff517cea62882uL, 0x3d18eb650003fb32uL,
     0xc08ff51b8449d972uL, 0xbd326baaf0b591f8uL,
     0xc08ff51f38bcb408uL, 0xbd461b8d0e43a37fuL,
     0xc08ff522ebff7bbcuL, 0xbd33859a74f0d148uL,
     0xc08ff5269e12f346uL, 0xbd3c57f2495fb7fauL,
     0xc08ff52a4ef7dca8uL, 0xbcd5dc21a39bf974uL,
     0xc08ff52dfeaef926uL, 0x3d0aa0e9e6bca777uL,
     0xc08ff531ad39094cuL, 0xbd47d0fa4fa0c208uL,
     0xc08ff5355a96ccf4uL, 0x3d23bb5921006679uL,
     0xc08ff53906c90336uL, 0xbd21f3e0c466e8f9uL,
     0xc08ff53cb1d06a7cuL, 0xbd39f3ba83f85c08uL,
     0xc08ff5405badc07auL, 0x3d2e77ad7a4b71c0uL,
     0xc08ff5440461c22auL, 0xbd1f1bbd2926f164uL,
     0xc08ff547abed2bd8uL, 0xbd44479667bb79bfuL,
     0xc08ff54b5250b91euL, 0xbd2094ef49b8484buL,
     0xc08ff54ef78d24deuL, 0xbd41fb87566dd18cuL,
     0xc08ff5529ba32950uL, 0xbd3c6d8d86531d56uL,
     0xc08ff5563e937ff8uL, 0xbd323e7492de8d74uL,
     0xc08ff559e05ee1acuL, 0xbcf63d8bd35fdc18uL,
     0xc08ff55d81060692uL, 0xbd3cc78dae939320uL,
     0xc08ff5612089a626uL, 0xbd44cf0e362f4a36uL,
     0xc08ff564beea7736uL, 0xbd3a96d7a36f1545uL,
     0xc08ff5685c292fe2uL, 0xbd4570af1a0bc9f4uL,
     0xc08ff56bf84685a4uL, 0x3d1bdc90791aef03uL,
     0xc08ff56f93432d44uL, 0xbd40d2abacfc0489uL,
     0xc08ff5732d1fdaeauL, 0xbd39e35c1aa7693fuL,
     0xc08ff576c5dd4210uL, 0xbd23c49c247ab6afuL,
     0xc08ff57a5d7c1588uL, 0xbd4374da167aead5uL,
     0xc08ff57df3fd0782uL, 0xbd2aeb8cb1ac05cduL,
     0xc08ff5818960c982uL, 0xbd3b1b8ae4633046uL,
     0xc08ff5851da80c6cuL, 0xbd20899cee46ebe4uL,
     0xc08ff588b0d3807cuL, 0xbcfc4413fd83dec1uL,
     0xc08ff58c42e3d54cuL, 0xbd02101a9685c779uL,
     0xc08ff58fd3d9b9d2uL, 0xbd45c074c957d037uL,
     0xc08ff59363b5dc66uL, 0xbd3f7cc3df8803d1uL,
     0xc08ff596f278eabauL, 0xbd3961ecab44052euL,
     0xc08ff59a802391e2uL, 0xbd1979a5db68721duL,
     0xc08ff59e0cb67e50uL, 0xbd3e4ce321e589a9uL,
     0xc08ff5a198325bdcuL, 0x3d0e321d11f8a0ceuL,
     0xc08ff5a52297d5bauL, 0x3d227ae8037b21bfuL,
     0xc08ff5a8abe79684uL, 0x3d1ebefecd51a1beuL,
     0xc08ff5ac34224836uL, 0xbd372c2fed3f759fuL,
     0xc08ff5afbb489432uL, 0xbd46b82e2a9e810cuL,
     0xc08ff5b3415b2340uL, 0x3d2e59ad84a6a593uL,
     0xc08ff5b6c65a9d86uL, 0xbd249d97df07e357uL,
     0xc08ff5ba4a47aa98uL, 0xbd46d25a5b8a19b2uL,
     0xc08ff5bdcd22f172uL, 0x3d2e859780f0cdc7uL,
     0xc08ff5c14eed186euL, 0xbd4171cf05a99915uL,
     0xc08ff5c4cfa6c55auL, 0xbd41ef9459fef720uL,
     0xc08ff5c84f509d68uL, 0x3d145ccfb66fabd2uL,
     0xc08ff5cbcdeb4530uL, 0xbd46bf2e7459b97duL,
     0xc08ff5cf4b7760beuL, 0xbd36132520b9d027uL,
     0xc08ff5d2c7f59382uL, 0x3d15872350f805d6uL,
     0xc08ff5d643668058uL, 0xbd41835d469035a9uL,
     0xc08ff5d9bdcac98euL, 0xbd47b7378ad99d2euL,
     0xc08ff5dd372310dcuL, 0xbd472d51ea7c162euL,
     0xc08ff5e0af6ff76auL, 0x3d2a8843781eda15uL,
     0xc08ff5e426b21dc8uL, 0xbd44ea36d76b0bd8uL,
     0xc08ff5e79cea2402uL, 0x3d2e03b336c24b74uL,
     0xc08ff5eb1218a986uL, 0xbd45a7bfdb3c98b0uL,
     0xc08ff5ee863e4d40uL, 0xbd37204f55bbf90duL,
     0xc08ff5f1f95bad84uL, 0xbd41b72e122257f1uL,
     0xc08ff5f56b71681euL, 0xbd1488084776534auL,
     0xc08ff5f8dc801a48uL, 0xbd2866405210e49euL,
     0xc08ff5fc4c8860b4uL, 0x3d1d45da26510032uL,
     0xc08ff5ffbb8ad784uL, 0xbd2f386200388584uL,
     0xc08ff60329881a52uL, 0xbd47e32446892fb9uL,
     0xc08ff6069680c42euL, 0xbd4330c4c4a27e40uL,
     0xc08ff60a02756f9cuL, 0xbd0cb6f70109b0f1uL,
     0xc08ff60d6d66b694uL, 0xbd4777531ab1b43fuL,
     0xc08ff610d755328euL, 0x3d118906313e79cfuL,
     0xc08ff61440417c70uL, 0x3d0a5b363a6f499cuL,
     0xc08ff617a82c2c9euL, 0xbd39308437e74325uL,
     0xc08ff61b0f15daf6uL, 0xbd3fef5f3fc61899uL,
     0xc08ff61e74ff1eceuL, 0xbd3b85f3204507b9uL,
     0xc08ff621d9e88ef6uL, 0xbd42fc8ea3276ba0uL,
     0xc08ff6253dd2c1bcuL, 0x3d0d2fe4574e09b9uL,
     0xc08ff628a0be4ce4uL, 0xbd3245829ca653e6uL,
     0xc08ff62c02abc5b4uL, 0xbd42a385b236e315uL,
     0xc08ff62f639bc0eeuL, 0xbd301f1e98d8979cuL,
     0xc08ff632c38ed2ceuL, 0xbd3ded9b44542fd9uL,
     0xc08ff63622858f12uL, 0xbd3d400fd651da9auL,
     0xc08ff639808088f6uL, 0x3d29f78153fcfec0uL,
     0xc08ff63cdd805330uL, 0xbd46af859d47a29auL,
     0xc08ff64039858000uL, 0xbd3667f21fa8423fuL,
     0xc08ff6439490a11euL, 0xbd1b254cabaa042buL,
     0xc08ff646eea247c6uL, 0x3d1ee969a95f528fuL,
     0xc08ff64a47bb04b4uL, 0xbd3821d36e0b7548uL,
     0xc08ff64d9fdb682auL, 0xbd3974e6432d9ee8uL,
     0xc08ff650f70401eauL, 0xbd1d74d044558154uL,
     0xc08ff6544d356138uL, 0xbd371b3a63cddadfuL,
     0xc08ff657a27014e0uL, 0x3d17b6aad08dc210uL,
     0xc08ff65af6b4ab2cuL, 0xbd47d7bfb12454c5uL,
     0xc08ff65e4a03b1f4uL, 0xbd373647bf25fa5fuL,
     0xc08ff6619c5db68euL, 0xbcf742a6b2827cf0uL,
     0xc08ff664edc345d8uL, 0xbd02d3bbd925734cuL,
     0xc08ff6683e34ec38uL, 0xbd03f7a55cd2af4cuL,
     0xc08ff66b8db3359auL, 0xbd308364fa508035uL,
     0xc08ff66edc3ead74uL, 0x3d2b37bd36337985uL,
     0xc08ff67229d7dec0uL, 0x3d22a424c693063duL,
     0xc08ff675767f5404uL, 0xbd166cccab240e90uL,
     0xc08ff678c2359750uL, 0x3d2bce65acc07927uL,
     0xc08ff67c0cfb323auL, 0xbd25651ccd0e0880uL,
     0xc08ff67f56d0ade6uL, 0xbd4533d5b4542c99uL,
     0xc08ff6829fb69304uL, 0xbd22ce6312ebb81duL,
     0xc08ff685e7ad69cauL, 0xbd2b6967f02b01d8uL,
     0xc08ff6892eb5b9feuL, 0xbd3bb55730409355uL,
     0xc08ff68c74d00af2uL, 0xbd4352b18e47fcd2uL,
     0xc08ff68fb9fce386uL, 0xbceed0798d1aa216uL,
     0xc08ff692fe3cca22uL, 0xbd464b702b56565euL,
     0xc08ff696419044c4uL, 0xbd45909799f95e23uL,
     0xc08ff69983f7d8f4uL, 0xbd2bebde1ac6e983uL,
     0xc08ff69cc5740bc8uL, 0xbd18f7aac147fdc1uL,
     0xc08ff6a0060561e8uL, 0x3d2653a2eb403f26uL,
     0xc08ff6a345ac5f8auL, 0x3d1769a8e6b40f5euL,
     0xc08ff6a684698876uL, 0xbd1770535b322bbfuL,
     0xc08ff6a9c23d6004uL, 0xbd434df378df21aduL,
     0xc08ff6acff286920uL, 0xbd398cc3b5d08e15uL,
     0xc08ff6b03b2b2644uL, 0xbd39d941e9e746a4uL,
     0xc08ff6b376461980uL, 0x3d2fd2e802de76aduL,
     0xc08ff6b6b079c472uL, 0xbcf968ab16b0d7bauL,
     0xc08ff6b9e9c6a850uL, 0xbd3fa4a9eb6b8621uL,
     0xc08ff6bd222d45e4uL, 0xbd36ad5bac74b87fuL,
     0xc08ff6c059ae1d8auL, 0x3d057c1b79ee9964uL,
     0xc08ff6c39049af32uL, 0xbd0af5e9bb5386c2uL,
     0xc08ff6c6c6007a64uL, 0xbce8467191344d58uL,
     0xc08ff6c9fad2fe3cuL, 0xbd1148dad646cb9duL,
     0xc08ff6cd2ec1b96cuL, 0xbd4149540d5fceb9uL,
     0xc08ff6d061cd2a40uL, 0xbd117b2f1731efbeuL,
     0xc08ff6d393f5ce96uL, 0x3d25005be8c5610buL,
     0xc08ff6d6c53c23e6uL, 0x3d29a1979619fe2fuL,
     0xc08ff6d9f5a0a740uL, 0x3d15ebe99c4f6416uL,
     0xc08ff6dd2523d54cuL, 0xbd36d25a5b8a19b2uL,
     0xc08ff6e053c62a4cuL, 0xbd47f3f2612caf97uL,
     0xc08ff6e38188221cuL, 0xbd3848e9d1d92d88uL,
     0xc08ff6e6ae6a382euL, 0xbd3b4aada7453897uL,
     0xc08ff6e9da6ce792uL, 0xbd2640ef87ede14buL,
     0xc08ff6ed0590aaf0uL, 0xbd2da89e835cc3d2uL,
     0xc08ff6f02fd5fc8euL, 0x3d2fa6e2ac948d1auL,
     0xc08ff6f3593d5648uL, 0xbd44bf3775fde250uL,
     0xc08ff6f681c731a0uL, 0x3d2924ae921f7ecauL,
     0xc08ff6f9a97407a8uL, 0xbd32994b351f388cuL,
     0xc08ff6fcd0445118uL, 0xbd429af37d1edf2fuL,
     0xc08ff6fff6388644uL, 0x3d2ed5a8a2de89dauL,
     0xc08ff7031b511f16uL, 0xbd474d8b66a69572uL,
     0xc08ff7063f8e9322uL, 0xbd3b20d190c69cffuL,
     0xc08ff70962f15992uL, 0xbcf455bedf4083bcuL,
     0xc08ff70c8579e930uL, 0xbd215844900583deuL,
     0xc08ff70fa728b868uL, 0xbd054cda62d3926euL,
     0xc08ff712c7fe3d44uL, 0x3d2143e9a0cbd481uL,
     0xc08ff715e7faed6euL, 0x3d2a82ed66976b91uL,
     0xc08ff719071f3e30uL, 0xbd318c64f0672cf9uL,
     0xc08ff71c256ba478uL, 0xbd2c760bc9b188c4uL,
     0xc08ff71f42e094d2uL, 0xbd2b88ca364674acuL,
     0xc08ff7225f7e836cuL, 0xbd46361ccd8974a5uL,
     0xc08ff7257b45e41auL, 0xbd24e3eb5884aae7uL,
     0xc08ff72896372a4cuL, 0xbd38b1aff71c8605uL,
     0xc08ff72bb052c91auL, 0xbd429a0a140ddd8auL,
     0xc08ff72ec999333euL, 0xbd43d6bb35ec114fuL,
     0xc08ff731e20adb16uL, 0xbd2bd849ce4dc635uL,
     0xc08ff734f9a832a2uL, 0xbd206c243749114cuL,
     0xc08ff7381071ab88uL, 0xbd3595f2f68d91fduL,
     0xc08ff73b2667b714uL, 0xbd3017eb15bb7de4uL,
     0xc08ff73e3b8ac636uL, 0x3d1c28798c12cc39uL,
     0xc08ff7414fdb4982uL, 0xbd12ce6312ebb81duL,
     0xc08ff7446359b134uL, 0xbd4395510d1e3f81uL,
     0xc08ff74776066d30uL, 0xbd3f86493917b407uL,
     0xc08ff74a87e1ecfeuL, 0xbd10be3a57487484uL,
     0xc08ff74d98ec9fccuL, 0xbd2d5297837adb4buL,
     0xc08ff750a926f472uL, 0xbd43ae4d308b33a5uL,
     0xc08ff753b8915972uL, 0x3d2d54d244e2aaeeuL,
     0xc08ff756c72c3ceeuL, 0xbd35f097b0fe80a3uL,
     0xc08ff759d4f80cbauL, 0xbd3077f1f5f0cc83uL,
     0xc08ff75ce1f5364euL, 0x3d19367107b8e917uL,
     0xc08ff75fee2426cauL, 0xbd33623c81400bcfuL,
     0xc08ff762f9854afcuL, 0xbd33b55bcb161bacuL,
     0xc08ff76604190f5auL, 0x3d2eb3c3bf914b9cuL,
     0xc08ff7690ddfe000uL, 0xbd45a6a7f43f6ec0uL,
     0xc08ff76c16da28beuL, 0xbd3b253dff5e0495uL,
     0xc08ff76f1f085508uL, 0x3d1b08127eec65d2uL,
     0xc08ff772266acffcuL, 0xbd45b1799ceaeb51uL,
     0xc08ff7752d02046cuL, 0xbd2e63bd0fcda210uL,
     0xc08ff77832ce5cceuL, 0xbd148cd0a7bb24b2uL,
     0xc08ff77b37d04348uL, 0x3d11ef56fa3d37b4uL,
     0xc08ff77e3c0821acuL, 0x3d1a768216f872ebuL,
     0xc08ff7813f766178uL, 0xbd44b4a15a96316euL,
     0xc08ff784421b6bdcuL, 0xbd4258a7b2336919uL,
     0xc08ff78743f7a9b2uL, 0x3d03f659faac5a20uL,
     0xc08ff78a450b8380uL, 0xbd2401fbaaa67e3cuL,
     0xc08ff78d4557617euL, 0xbd476fa81cf6a494uL,
     0xc08ff79044dbab94uL, 0xbd44f46b93eece0auL,
     0xc08ff7934398c956uL, 0xbd3c91f073716495uL,
     0xc08ff796418f2208uL, 0xbd3672b0c88d4dd6uL,
     0xc08ff7993ebf1c9euL, 0xbd3fb554647678d1uL,
     0xc08ff79c3b291fbeuL, 0xbd0bb98afdf33295uL,
     0xc08ff79f36cd91bauL, 0xbd3a1c40753a869fuL,
     0xc08ff7a231acd89auL, 0xbd3395510d1e3f81uL,
     0xc08ff7a52bc75a14uL, 0xbcf98fd2dca61c14uL,
     0xc08ff7a8251d7b8euL, 0xbd40e7b8e7574248uL,
     0xc08ff7ab1dafa224uL, 0xbd43f88ff2576e98uL,
     0xc08ff7ae157e32a2uL, 0xbd1f61a96b8ce776uL,
     0xc08ff7b10c899184uL, 0x3cde66be73b9da04uL,
     0xc08ff7b402d222fauL, 0xbd408d5c3f1d5c0duL,
     0xc08ff7b6f8584aeauL, 0xbd3cbebea25ecd9euL,
     0xc08ff7b9ed1c6ceauL, 0xbd2507d6dc1f27efuL,
     0xc08ff7bce11eec44uL, 0x3d2794d4c6c8f327uL,
     0xc08ff7bfd4602bf4uL, 0xbd3f1e32799da52duL,
     0xc08ff7c2c6e08eb0uL, 0xbd35c01818adf4afuL,
     0xc08ff7c5b8a076deuL, 0x3d2cfc4de6d73deauL,
     0xc08ff7c8a9a04696uL, 0xbd4227264a17d460uL,
     0xc08ff7cb99e05faeuL, 0xbd0142b08bb672e8uL,
     0xc08ff7ce896123a8uL, 0xbd2564fcfaea5fb3uL,
     0xc08ff7d17822f3c2uL, 0x3d2aab1b2a41b090uL,
     0xc08ff7d4662630eauL, 0xbd46ac3b83ef359auL,
     0xc08ff7d7536b3bceuL, 0x3d241a2f220ccf53uL,
     0xc08ff7da3ff274c6uL, 0xbd38f5d37680fd7cuL,
     0xc08ff7dd2bbc3becuL, 0x3d048a179268271duL,
     0xc08ff7e016c8f108uL, 0xbd471e548b69f12auL,
     0xc08ff7e30118f3a2uL, 0xbd41a23946dfa58cuL,
     0xc08ff7e5eaaca2f4uL, 0xbd25330d5605f2a6uL,
     0xc08ff7e8d3845df0uL, 0xbd319b14945cf6bauL,
     0xc08ff7ebbba08342uL, 0xbd4702e1863f7c92uL,
     0xc08ff7eea3017150uL, 0xbd437cfeba9ff979uL,
     0xc08ff7f189a78636uL, 0xbd3df6e958e938b0uL,
     0xc08ff7f46f931fcauL, 0xbd37ca15910e7069uL,
     0xc08ff7f754c49b9cuL, 0xbd15cfd00d77e6ecuL,
     0xc08ff7fa393c56f4uL, 0xbd2a025d9e2442e6uL,
     0xc08ff7fd1cfaaed6uL, 0xbd3258e9a821b7ccuL,
     0xc08ff80000000000uL, 0x0000000000000000uL,
     },

    {

     0x3ff0000000000000uL,
     0x3ff00b1afa5abcbfuL,
     0x3ff0163da9fb3335uL,
     0x3ff02168143b0281uL,
     0x3ff02c9a3e778061uL,
     0x3ff037d42e11bbccuL,
     0x3ff04315e86e7f85uL,
     0x3ff04e5f72f654b1uL,
     0x3ff059b0d3158574uL,
     0x3ff0650a0e3c1f89uL,
     0x3ff0706b29ddf6deuL,
     0x3ff07bd42b72a836uL,
     0x3ff0874518759bc8uL,
     0x3ff092bdf66607e0uL,
     0x3ff09e3ecac6f383uL,
     0x3ff0a9c79b1f3919uL,
     0x3ff0b5586cf9890fuL,
     0x3ff0c0f145e46c85uL,
     0x3ff0cc922b7247f7uL,
     0x3ff0d83b23395decuL,
     0x3ff0e3ec32d3d1a2uL,
     0x3ff0efa55fdfa9c5uL,
     0x3ff0fb66affed31buL,
     0x3ff1073028d7233euL,
     0x3ff11301d0125b51uL,
     0x3ff11edbab5e2ab6uL,
     0x3ff12abdc06c31ccuL,
     0x3ff136a814f204abuL,
     0x3ff1429aaea92de0uL,
     0x3ff14e95934f312euL,
     0x3ff15a98c8a58e51uL,
     0x3ff166a45471c3c2uL,
     0x3ff172b83c7d517buL,
     0x3ff17ed48695bbc0uL,
     0x3ff18af9388c8deauL,
     0x3ff1972658375d2fuL,
     0x3ff1a35beb6fcb75uL,
     0x3ff1af99f8138a1cuL,
     0x3ff1bbe084045cd4uL,
     0x3ff1c82f95281c6buL,
     0x3ff1d4873168b9aauL,
     0x3ff1e0e75eb44027uL,
     0x3ff1ed5022fcd91duL,
     0x3ff1f9c18438ce4duL,
     0x3ff2063b88628cd6uL,
     0x3ff212be3578a819uL,
     0x3ff21f49917ddc96uL,
     0x3ff22bdda27912d1uL,
     0x3ff2387a6e756238uL,
     0x3ff2451ffb82140auL,
     0x3ff251ce4fb2a63fuL,
     0x3ff25e85711ece75uL,
     0x3ff26b4565e27cdduL,
     0x3ff2780e341ddf29uL,
     0x3ff284dfe1f56381uL,
     0x3ff291ba7591bb70uL,
     0x3ff29e9df51fdee1uL,
     0x3ff2ab8a66d10f13uL,
     0x3ff2b87fd0dad990uL,
     0x3ff2c57e39771b2fuL,
     0x3ff2d285a6e4030buL,
     0x3ff2df961f641589uL,
     0x3ff2ecafa93e2f56uL,
     0x3ff2f9d24abd886buL,
     0x3ff306fe0a31b715uL,
     0x3ff31432edeeb2fduL,
     0x3ff32170fc4cd831uL,
     0x3ff32eb83ba8ea32uL,
     0x3ff33c08b26416ffuL,
     0x3ff3496266e3fa2duL,
     0x3ff356c55f929ff1uL,
     0x3ff36431a2de883buL,
     0x3ff371a7373aa9cbuL,
     0x3ff37f26231e754auL,
     0x3ff38cae6d05d866uL,
     0x3ff39a401b7140efuL,
     0x3ff3a7db34e59ff7uL,
     0x3ff3b57fbfec6cf4uL,
     0x3ff3c32dc313a8e5uL,
     0x3ff3d0e544ede173uL,
     0x3ff3dea64c123422uL,
     0x3ff3ec70df1c5175uL,
     0x3ff3fa4504ac801cuL,
     0x3ff40822c367a024uL,
     0x3ff4160a21f72e2auL,
     0x3ff423fb2709468auL,
     0x3ff431f5d950a897uL,
     0x3ff43ffa3f84b9d4uL,
     0x3ff44e086061892duL,
     0x3ff45c2042a7d232uL,
     0x3ff46a41ed1d0057uL,
     0x3ff4786d668b3237uL,
     0x3ff486a2b5c13cd0uL,
     0x3ff494e1e192aed2uL,
     0x3ff4a32af0d7d3deuL,
     0x3ff4b17dea6db7d7uL,
     0x3ff4bfdad5362a27uL,
     0x3ff4ce41b817c114uL,
     0x3ff4dcb299fddd0duL,
     0x3ff4eb2d81d8abffuL,
     0x3ff4f9b2769d2ca7uL,
     0x3ff508417f4531eeuL,
     0x3ff516daa2cf6642uL,
     0x3ff5257de83f4eefuL,
     0x3ff5342b569d4f82uL,
     0x3ff542e2f4f6ad27uL,
     0x3ff551a4ca5d920fuL,
     0x3ff56070dde910d2uL,
     0x3ff56f4736b527dauL,
     0x3ff57e27dbe2c4cfuL,
     0x3ff58d12d497c7fduL,
     0x3ff59c0827ff07ccuL,
     0x3ff5ab07dd485429uL,
     0x3ff5ba11fba87a03uL,
     0x3ff5c9268a5946b7uL,
     0x3ff5d84590998b93uL,
     0x3ff5e76f15ad2148uL,
     0x3ff5f6a320dceb71uL,
     0x3ff605e1b976dc09uL,
     0x3ff6152ae6cdf6f4uL,
     0x3ff6247eb03a5585uL,
     0x3ff633dd1d1929fduL,
     0x3ff6434634ccc320uL,
     0x3ff652b9febc8fb7uL,
     0x3ff6623882552225uL,
     0x3ff671c1c70833f6uL,
     0x3ff68155d44ca973uL,
     0x3ff690f4b19e9538uL,
     0x3ff6a09e667f3bcduL,
     0x3ff6b052fa75173euL,
     0x3ff6c012750bdabfuL,
     0x3ff6cfdcddd47645uL,
     0x3ff6dfb23c651a2fuL,
     0x3ff6ef9298593ae5uL,
     0x3ff6ff7df9519484uL,
     0x3ff70f7466f42e87uL,
     0x3ff71f75e8ec5f74uL,
     0x3ff72f8286ead08auL,
     0x3ff73f9a48a58174uL,
     0x3ff74fbd35d7cbfduL,
     0x3ff75feb564267c9uL,
     0x3ff77024b1ab6e09uL,
     0x3ff780694fde5d3fuL,
     0x3ff790b938ac1cf6uL,
     0x3ff7a11473eb0187uL,
     0x3ff7b17b0976cfdbuL,
     0x3ff7c1ed0130c132uL,
     0x3ff7d26a62ff86f0uL,
     0x3ff7e2f336cf4e62uL,
     0x3ff7f3878491c491uL,
     0x3ff80427543e1a12uL,
     0x3ff814d2add106d9uL,
     0x3ff82589994cce13uL,
     0x3ff8364c1eb941f7uL,
     0x3ff8471a4623c7aduL,
     0x3ff857f4179f5b21uL,
     0x3ff868d99b4492eduL,
     0x3ff879cad931a436uL,
     0x3ff88ac7d98a6699uL,
     0x3ff89bd0a478580fuL,
     0x3ff8ace5422aa0dbuL,
     0x3ff8be05bad61778uL,
     0x3ff8cf3216b5448cuL,
     0x3ff8e06a5e0866d9uL,
     0x3ff8f1ae99157736uL,
     0x3ff902fed0282c8auL,
     0x3ff9145b0b91ffc6uL,
     0x3ff925c353aa2fe2uL,
     0x3ff93737b0cdc5e5uL,
     0x3ff948b82b5f98e5uL,
     0x3ff95a44cbc8520fuL,
     0x3ff96bdd9a7670b3uL,
     0x3ff97d829fde4e50uL,
     0x3ff98f33e47a22a2uL,
     0x3ff9a0f170ca07bauL,
     0x3ff9b2bb4d53fe0duL,
     0x3ff9c49182a3f090uL,
     0x3ff9d674194bb8d5uL,
     0x3ff9e86319e32323uL,
     0x3ff9fa5e8d07f29euL,
     0x3ffa0c667b5de565uL,
     0x3ffa1e7aed8eb8bbuL,
     0x3ffa309bec4a2d33uL,
     0x3ffa42c980460ad8uL,
     0x3ffa5503b23e255duL,
     0x3ffa674a8af46052uL,
     0x3ffa799e1330b358uL,
     0x3ffa8bfe53c12e59uL,
     0x3ffa9e6b5579fdbfuL,
     0x3ffab0e521356ebauL,
     0x3ffac36bbfd3f37auL,
     0x3ffad5ff3a3c2774uL,
     0x3ffae89f995ad3aduL,
     0x3ffafb4ce622f2ffuL,
     0x3ffb0e07298db666uL,
     0x3ffb20ce6c9a8952uL,
     0x3ffb33a2b84f15fbuL,
     0x3ffb468415b749b1uL,
     0x3ffb59728de5593auL,
     0x3ffb6c6e29f1c52auL,
     0x3ffb7f76f2fb5e47uL,
     0x3ffb928cf22749e4uL,
     0x3ffba5b030a1064auL,
     0x3ffbb8e0b79a6f1fuL,
     0x3ffbcc1e904bc1d2uL,
     0x3ffbdf69c3f3a207uL,
     0x3ffbf2c25bd71e09uL,
     0x3ffc06286141b33duL,
     0x3ffc199bdd85529cuL,
     0x3ffc2d1cd9fa652cuL,
     0x3ffc40ab5fffd07auL,
     0x3ffc544778fafb22uL,
     0x3ffc67f12e57d14buL,
     0x3ffc7ba88988c933uL,
     0x3ffc8f6d9406e7b5uL,
     0x3ffca3405751c4dbuL,
     0x3ffcb720dcef9069uL,
     0x3ffccb0f2e6d1675uL,
     0x3ffcdf0b555dc3fauL,
     0x3ffcf3155b5bab74uL,
     0x3ffd072d4a07897cuL,
     0x3ffd1b532b08c968uL,
     0x3ffd2f87080d89f2uL,
     0x3ffd43c8eacaa1d6uL,
     0x3ffd5818dcfba487uL,
     0x3ffd6c76e862e6d3uL,
     0x3ffd80e316c98398uL,
     0x3ffd955d71ff6075uL,
     0x3ffda9e603db3285uL,
     0x3ffdbe7cd63a8315uL,
     0x3ffdd321f301b460uL,
     0x3ffde7d5641c0658uL,
     0x3ffdfc97337b9b5fuL,
     0x3ffe11676b197d17uL,
     0x3ffe264614f5a129uL,
     0x3ffe3b333b16ee12uL,
     0x3ffe502ee78b3ff6uL,
     0x3ffe653924676d76uL,
     0x3ffe7a51fbc74c83uL,
     0x3ffe8f7977cdb740uL,
     0x3ffea4afa2a490dauL,
     0x3ffeb9f4867cca6euL,
     0x3ffecf482d8e67f1uL,
     0x3ffee4aaa2188510uL,
     0x3ffefa1bee615a27uL,
     0x3fff0f9c1cb6412auL,
     0x3fff252b376bba97uL,
     0x3fff3ac948dd7274uL,
     0x3fff50765b6e4540uL,
     0x3fff6632798844f8uL,
     0x3fff7bfdad9cbe14uL,
     0x3fff91d802243c89uL,
     0x3fffa7c1819e90d8uL,
     0x3fffbdba3692d514uL,
     0x3fffd3c22b8f71f1uL,
     0x3fffe9d96b2a23d9uL,
     },

    0x000fffffffffffffuL,
    0x3ff0000000000000uL,
    0x4338000000000000uL,
    0x0010000000000000uL,
    0x7fefffffffffffffuL,
    0x0000040000000000uL,
    0xfffff80000000000uL,

    0xbfcec1cfbbc5c90cuL,
    0x3fd2776da3d26e6auL,
    0xbfd71547655d37e0uL,
    0x3fdec709dc39fb02uL,
    0x3c777a3a2c24613duL,
    0x3ff71547652b82feuL,
    0xbfe71547652b82feuL,

    0x7fffffffffffffffuL,
    0x408FEC0000000000uL,
    0x42b800000003ff00uL,
    0x00000000000007f8uL,

    0x3f83b2ab930f15f9uL,
    0x3fac6b090da1e0a9uL,
    0x3fcebfbdff82c54duL,
    0x3fe62e42fefa39b9uL,

    {

     0x3FF7154740000000uL, 0x3FF70F8340000000uL,
     0x3FF709C240000000uL, 0x3FF7040440000000uL,
     0x3FF6FE4900000000uL, 0x3FF6F89080000000uL,
     0x3FF6F2DB00000000uL, 0x3FF6ED2840000000uL,
     0x3FF6E77840000000uL, 0x3FF6E1CB40000000uL,
     0x3FF6DC2100000000uL, 0x3FF6D67980000000uL,
     0x3FF6D0D4C0000000uL, 0x3FF6CB32C0000000uL,
     0x3FF6C593C0000000uL, 0x3FF6BFF780000000uL,
     0x3FF6BA5DC0000000uL, 0x3FF6B4C700000000uL,
     0x3FF6AF32C0000000uL, 0x3FF6A9A180000000uL,
     0x3FF6A41300000000uL, 0x3FF69E8700000000uL,
     0x3FF698FDC0000000uL, 0x3FF6937740000000uL,
     0x3FF68DF380000000uL, 0x3FF6887280000000uL,
     0x3FF682F400000000uL, 0x3FF67D7840000000uL,
     0x3FF677FF40000000uL, 0x3FF67288C0000000uL,
     0x3FF66D1540000000uL, 0x3FF667A400000000uL,
     0x3FF6623580000000uL, 0x3FF65CC9C0000000uL,
     0x3FF6576080000000uL, 0x3FF651FA00000000uL,
     0x3FF64C9600000000uL, 0x3FF6473480000000uL,
     0x3FF641D5C0000000uL, 0x3FF63C7980000000uL,
     0x3FF6372000000000uL, 0x3FF631C900000000uL,
     0x3FF62C7480000000uL, 0x3FF6272280000000uL,
     0x3FF621D340000000uL, 0x3FF61C8640000000uL,
     0x3FF6173C00000000uL, 0x3FF611F440000000uL,
     0x3FF60CAF00000000uL, 0x3FF6076C40000000uL,
     0x3FF6022C00000000uL, 0x3FF5FCEE80000000uL,
     0x3FF5F7B340000000uL, 0x3FF5F27A80000000uL,
     0x3FF5ED4440000000uL, 0x3FF5E81040000000uL,
     0x3FF5E2DF00000000uL, 0x3FF5DDB040000000uL,
     0x3FF5D883C0000000uL, 0x3FF5D359C0000000uL,
     0x3FF5CE3240000000uL, 0x3FF5C90D40000000uL,
     0x3FF5C3EA80000000uL, 0x3FF5BECA40000000uL,
     0x3FF5B9AC80000000uL, 0x3FF5B49100000000uL,
     0x3FF5AF7800000000uL, 0x3FF5AA6180000000uL,
     0x3FF5A54D40000000uL, 0x3FF5A03B40000000uL,
     0x3FF59B2BC0000000uL, 0x3FF5961EC0000000uL,
     0x3FF59113C0000000uL, 0x3FF58C0B80000000uL,
     0x3FF5870540000000uL, 0x3FF58201C0000000uL,
     0x3FF57D0040000000uL, 0x3FF5780140000000uL,
     0x3FF5730480000000uL, 0x3FF56E0A00000000uL,
     0x3FF56911C0000000uL, 0x3FF5641C00000000uL,
     0x3FF55F2880000000uL, 0x3FF55A3740000000uL,
     0x3FF5554840000000uL, 0x3FF5505BC0000000uL,
     0x3FF54B7140000000uL, 0x3FF5468900000000uL,
     0x3FF541A340000000uL, 0x3FF53CBF80000000uL,
     0x3FF537DE40000000uL, 0x3FF532FF00000000uL,
     0x3FF52E2240000000uL, 0x3FF5294780000000uL,
     0x3FF5246F00000000uL, 0x3FF51F98C0000000uL,
     0x3FF51AC4C0000000uL, 0x3FF515F300000000uL,
     0x3FF5112340000000uL, 0x3FF50C5600000000uL,
     0x3FF5078AC0000000uL, 0x3FF502C1C0000000uL,
     0x3FF4FDFAC0000000uL, 0x3FF4F93600000000uL,
     0x3FF4F47380000000uL, 0x3FF4EFB340000000uL,
     0x3FF4EAF500000000uL, 0x3FF4E638C0000000uL,
     0x3FF4E17EC0000000uL, 0x3FF4DCC700000000uL,
     0x3FF4D81180000000uL, 0x3FF4D35DC0000000uL,
     0x3FF4CEAC80000000uL, 0x3FF4C9FD00000000uL,
     0x3FF4C54FC0000000uL, 0x3FF4C0A4C0000000uL,
     0x3FF4BBFBC0000000uL, 0x3FF4B754C0000000uL,
     0x3FF4B2B000000000uL, 0x3FF4AE0D40000000uL,
     0x3FF4A96C80000000uL, 0x3FF4A4CE00000000uL,
     0x3FF4A03140000000uL, 0x3FF49B9700000000uL,
     0x3FF496FE80000000uL, 0x3FF4926800000000uL,
     0x3FF48DD3C0000000uL, 0x3FF4894180000000uL,
     0x3FF484B100000000uL, 0x3FF48022C0000000uL,
     0x3FF47B96C0000000uL, 0x3FF4770C80000000uL,
     0x3FF4728440000000uL, 0x3FF46DFE00000000uL,
     0x3FF46979C0000000uL, 0x3FF464F780000000uL,
     0x3FF4607780000000uL, 0x3FF45BF940000000uL,
     0x3FF4577D00000000uL, 0x3FF45302C0000000uL,
     0x3FF44E8A40000000uL, 0x3FF44A1400000000uL,
     0x3FF4459F80000000uL, 0x3FF4412D40000000uL,
     0x3FF43CBCC0000000uL, 0x3FF4384E40000000uL,
     0x3FF433E180000000uL, 0x3FF42F7700000000uL,
     0x3FF42B0E40000000uL, 0x3FF426A780000000uL,
     0x3FF4224280000000uL, 0x3FF41DDF80000000uL,
     0x3FF4197E80000000uL, 0x3FF4151F40000000uL,
     0x3FF410C200000000uL, 0x3FF40C66C0000000uL,
     0x3FF4080D40000000uL, 0x3FF403B5C0000000uL,
     0x3FF3FF6000000000uL, 0x3FF3FB0C00000000uL,
     0x3FF3F6BA40000000uL, 0x3FF3F26A00000000uL,
     0x3FF3EE1BC0000000uL, 0x3FF3E9CF80000000uL,
     0x3FF3E58500000000uL, 0x3FF3E13C40000000uL,
     0x3FF3DCF580000000uL, 0x3FF3D8B080000000uL,
     0x3FF3D46D40000000uL, 0x3FF3D02C00000000uL,
     0x3FF3CBEC80000000uL, 0x3FF3C7AEC0000000uL,
     0x3FF3C37300000000uL, 0x3FF3BF3900000000uL,
     0x3FF3BB00C0000000uL, 0x3FF3B6CA40000000uL,
     0x3FF3B29580000000uL, 0x3FF3AE62C0000000uL,
     0x3FF3AA3180000000uL, 0x3FF3A60240000000uL,
     0x3FF3A1D4C0000000uL, 0x3FF39DA900000000uL,
     0x3FF3997F40000000uL, 0x3FF3955700000000uL,
     0x3FF3913080000000uL, 0x3FF38D0BC0000000uL,
     0x3FF388E900000000uL, 0x3FF384C7C0000000uL,
     0x3FF380A840000000uL, 0x3FF37C8AC0000000uL,
     0x3FF3786EC0000000uL, 0x3FF3745480000000uL,
     0x3FF3703C00000000uL, 0x3FF36C2540000000uL,
     0x3FF3681040000000uL, 0x3FF363FCC0000000uL,
     0x3FF35FEB40000000uL, 0x3FF35BDB40000000uL,
     0x3FF357CD00000000uL, 0x3FF353C080000000uL,
     0x3FF34FB5C0000000uL, 0x3FF34BAC80000000uL,
     0x3FF347A540000000uL, 0x3FF3439F80000000uL,
     0x3FF33F9B40000000uL, 0x3FF33B9900000000uL,
     0x3FF3379840000000uL, 0x3FF3339900000000uL,
     0x3FF32F9BC0000000uL, 0x3FF32B9FC0000000uL,
     0x3FF327A5C0000000uL, 0x3FF323AD40000000uL,
     0x3FF31FB680000000uL, 0x3FF31BC140000000uL,
     0x3FF317CDC0000000uL, 0x3FF313DBC0000000uL,
     0x3FF30FEB80000000uL, 0x3FF30BFD00000000uL,
     0x3FF3080FC0000000uL, 0x3FF3042480000000uL,
     0x3FF3003AC0000000uL, 0x3FF2FC5280000000uL,
     0x3FF2F86BC0000000uL, 0x3FF2F48700000000uL,
     0x3FF2F0A380000000uL, 0x3FF2ECC1C0000000uL,
     0x3FF2E8E180000000uL, 0x3FF2E502C0000000uL,
     0x3FF2E125C0000000uL, 0x3FF2DD4A40000000uL,
     0x3FF2D97080000000uL, 0x3FF2D59840000000uL,
     0x3FF2D1C180000000uL, 0x3FF2CDEC40000000uL,
     0x3FF2CA1880000000uL, 0x3FF2C64680000000uL,
     0x3FF2C27600000000uL, 0x3FF2BEA700000000uL,
     0x3FF2BAD9C0000000uL, 0x3FF2B70DC0000000uL,
     0x3FF2B34380000000uL, 0x3FF2AF7AC0000000uL,
     0x3FF2ABB340000000uL, 0x3FF2A7ED80000000uL,
     0x3FF2A42980000000uL, 0x3FF2A066C0000000uL,
     0x3FF29CA580000000uL, 0x3FF298E5C0000000uL,
     0x3FF29527C0000000uL, 0x3FF2916B00000000uL,
     0x3FF28DAFC0000000uL, 0x3FF289F640000000uL,
     0x3FF2863E00000000uL, 0x3FF2828740000000uL,
     0x3FF27ED240000000uL, 0x3FF27B1E80000000uL,
     0x3FF2776C40000000uL, 0x3FF273BB80000000uL,
     0x3FF2700C40000000uL, 0x3FF26C5E80000000uL,
     0x3FF268B200000000uL, 0x3FF2650740000000uL,
     0x3FF2615DC0000000uL, 0x3FF25DB5C0000000uL,
     0x3FF25A0F40000000uL, 0x3FF2566A40000000uL,
     0x3FF252C6C0000000uL, 0x3FF24F2480000000uL,
     0x3FF24B83C0000000uL, 0x3FF247E480000000uL,
     0x3FF24446C0000000uL, 0x3FF240AA40000000uL,
     0x3FF23D0F40000000uL, 0x3FF23975C0000000uL,
     0x3FF235DD80000000uL, 0x3FF23246C0000000uL,
     0x3FF22EB180000000uL, 0x3FF22B1D80000000uL,
     0x3FF2278B00000000uL, 0x3FF223FA00000000uL,
     0x3FF2206A40000000uL, 0x3FF21CDC00000000uL,
     0x3FF2194F00000000uL, 0x3FF215C380000000uL,
     0x3FF2123940000000uL, 0x3FF20EB080000000uL,
     0x3FF20B2940000000uL, 0x3FF207A340000000uL,
     0x3FF2041EC0000000uL, 0x3FF2009B80000000uL,
     0x3FF1FD1980000000uL, 0x3FF1F99900000000uL,
     0x3FF1F619C0000000uL, 0x3FF1F29C00000000uL,
     0x3FF1EF1FC0000000uL, 0x3FF1EBA480000000uL,
     0x3FF1E82AC0000000uL, 0x3FF1E4B280000000uL,
     0x3FF1E13B80000000uL, 0x3FF1DDC5C0000000uL,
     0x3FF1DA5180000000uL, 0x3FF1D6DE80000000uL,
     0x3FF1D36CC0000000uL, 0x3FF1CFFC40000000uL,
     0x3FF1CC8D40000000uL, 0x3FF1C91F80000000uL,
     0x3FF1C5B340000000uL, 0x3FF1C24840000000uL,
     0x3FF1BEDE40000000uL, 0x3FF1BB7600000000uL,
     0x3FF1B80EC0000000uL, 0x3FF1B4A900000000uL,
     0x3FF1B14480000000uL, 0x3FF1ADE140000000uL,
     0x3FF1AA7F40000000uL, 0x3FF1A71E80000000uL,
     0x3FF1A3BF40000000uL, 0x3FF1A06140000000uL,
     0x3FF19D0480000000uL, 0x3FF199A900000000uL,
     0x3FF1964EC0000000uL, 0x3FF192F5C0000000uL,
     0x3FF18F9E00000000uL, 0x3FF18C47C0000000uL,
     0x3FF188F280000000uL, 0x3FF1859EC0000000uL,
     0x3FF1824C00000000uL, 0x3FF17EFAC0000000uL,
     0x3FF17BAA80000000uL, 0x3FF1785BC0000000uL,
     0x3FF1750E40000000uL, 0x3FF171C1C0000000uL,
     0x3FF16E76C0000000uL, 0x3FF16B2D00000000uL,
     0x3FF167E440000000uL, 0x3FF1649D00000000uL,
     0x3FF16156C0000000uL, 0x3FF15E11C0000000uL,
     0x3FF15ACE40000000uL, 0x3FF1578BC0000000uL,
     0x3FF1544A80000000uL, 0x3FF1510A80000000uL,
     0x3FF14DCBC0000000uL, 0x3FF14A8E40000000uL,
     0x3FF14751C0000000uL, 0x3FF14416C0000000uL,
     0x3FF140DCC0000000uL, 0x3FF13DA400000000uL,
     0x3FF13A6C80000000uL, 0x3FF1373600000000uL,
     0x3FF1340100000000uL, 0x3FF130CD00000000uL,
     0x3FF12D9A40000000uL, 0x3FF12A68C0000000uL,
     0x3FF1273840000000uL, 0x3FF1240900000000uL,
     0x3FF120DB00000000uL, 0x3FF11DAE40000000uL,
     0x3FF11A8280000000uL, 0x3FF1175800000000uL,
     0x3FF1142EC0000000uL, 0x3FF11106C0000000uL,
     0x3FF10DDFC0000000uL, 0x3FF10AB9C0000000uL,
     0x3FF1079540000000uL, 0x3FF10471C0000000uL,
     0x3FF1014F80000000uL, 0x3FF0FE2E40000000uL,
     0x3FF0FB0E40000000uL, 0x3FF0F7EF40000000uL,
     0x3FF0F4D180000000uL, 0x3FF0F1B500000000uL,
     0x3FF0EE9980000000uL, 0x3FF0EB7F40000000uL,
     0x3FF0E86600000000uL, 0x3FF0E54E00000000uL,
     0x3FF0E23700000000uL, 0x3FF0DF2140000000uL,
     0x3FF0DC0C80000000uL, 0x3FF0D8F900000000uL,
     0x3FF0D5E6C0000000uL, 0x3FF0D2D540000000uL,
     0x3FF0CFC540000000uL, 0x3FF0CCB640000000uL,
     0x3FF0C9A840000000uL, 0x3FF0C69B40000000uL,
     0x3FF0C38F80000000uL, 0x3FF0C08500000000uL,
     0x3FF0BD7B80000000uL, 0x3FF0BA7300000000uL,
     0x3FF0B76BC0000000uL, 0x3FF0B46580000000uL,
     0x3FF0B16040000000uL, 0x3FF0AE5C40000000uL,
     0x3FF0AB5940000000uL, 0x3FF0A85780000000uL,
     0x3FF0A556C0000000uL, 0x3FF0A25700000000uL,
     0x3FF09F5880000000uL, 0x3FF09C5AC0000000uL,
     0x3FF0995E80000000uL, 0x3FF0966300000000uL,
     0x3FF09368C0000000uL, 0x3FF0906F80000000uL,
     0x3FF08D7740000000uL, 0x3FF08A8000000000uL,
     0x3FF0878A00000000uL, 0x3FF0849500000000uL,
     0x3FF081A100000000uL, 0x3FF07EAE40000000uL,
     0x3FF07BBC40000000uL, 0x3FF078CB80000000uL,
     0x3FF075DBC0000000uL, 0x3FF072ED00000000uL,
     0x3FF06FFF80000000uL, 0x3FF06D12C0000000uL,
     0x3FF06A2740000000uL, 0x3FF0673CC0000000uL,
     0x3FF0645340000000uL, 0x3FF0616AC0000000uL,
     0x3FF05E8340000000uL, 0x3FF05B9D00000000uL,
     0x3FF058B780000000uL, 0x3FF055D340000000uL,
     0x3FF052F000000000uL, 0x3FF0500D80000000uL,
     0x3FF04D2C40000000uL, 0x3FF04A4C00000000uL,
     0x3FF0476CC0000000uL, 0x3FF0448E80000000uL,
     0x3FF041B140000000uL, 0x3FF03ED500000000uL,
     0x3FF03BF9C0000000uL, 0x3FF0391FC0000000uL,
     0x3FF0364680000000uL, 0x3FF0336E40000000uL,
     0x3FF0309700000000uL, 0x3FF02DC0C0000000uL,
     0x3FF02AEB80000000uL, 0x3FF0281740000000uL,
     0x3FF0254400000000uL, 0x3FF02271C0000000uL,
     0x3FF01FA080000000uL, 0x3FF01CD040000000uL,
     0x3FF01A00C0000000uL, 0x3FF0173280000000uL,
     0x3FF0146540000000uL, 0x3FF01198C0000000uL,
     0x3FF00ECD80000000uL, 0x3FF00C0300000000uL,
     0x3FF0093980000000uL, 0x3FF0067100000000uL,
     0x3FF003A980000000uL, 0x3FF000E300000000uL,
     0x3FEFFC3A80000000uL, 0x3FEFF6B140000000uL,
     0x3FEFF129C0000000uL, 0x3FEFEBA480000000uL,
     0x3FEFE620C0000000uL, 0x3FEFE09F40000000uL,
     0x3FEFDB1F80000000uL, 0x3FEFD5A180000000uL,
     0x3FEFD02580000000uL, 0x3FEFCAAB80000000uL,
     0x3FEFC53340000000uL, 0x3FEFBFBD00000000uL,
     0x3FEFBA4880000000uL, 0x3FEFB4D600000000uL,
     0x3FEFAF6540000000uL, 0x3FEFA9F680000000uL,
     0x3FEFA48980000000uL, 0x3FEF9F1E40000000uL,
     0x3FEF99B500000000uL, 0x3FEF944DC0000000uL,
     0x3FEF8EE800000000uL, 0x3FEF898440000000uL,
     0x3FEF842280000000uL, 0x3FEF7EC280000000uL,
     0x3FEF796440000000uL, 0x3FEF7407C0000000uL,
     0x3FEF6EAD40000000uL, 0x3FEF695480000000uL,
     0x3FEF63FD80000000uL, 0x3FEF5EA880000000uL,
     0x3FEF595540000000uL, 0x3FEF5403C0000000uL,
     0x3FEF4EB400000000uL, 0x3FEF496640000000uL,
     0x3FEF441A00000000uL, 0x3FEF3ECFC0000000uL,
     0x3FEF398740000000uL, 0x3FEF344080000000uL,
     0x3FEF2EFB80000000uL, 0x3FEF29B880000000uL,
     0x3FEF247700000000uL, 0x3FEF1F3780000000uL,
     0x3FEF19F980000000uL, 0x3FEF14BD80000000uL,
     0x3FEF0F8340000000uL, 0x3FEF0A4AC0000000uL,
     0x3FEF0513C0000000uL, 0x3FEEFFDEC0000000uL,
     0x3FEEFAAB80000000uL, 0x3FEEF57A00000000uL,
     0x3FEEF04A00000000uL, 0x3FEEEB1C00000000uL,
     0x3FEEE5EF80000000uL, 0x3FEEE0C500000000uL,
     0x3FEEDB9C00000000uL, 0x3FEED67500000000uL,
     0x3FEED14F80000000uL, 0x3FEECC2BC0000000uL,
     0x3FEEC709C0000000uL, 0x3FEEC1E940000000uL,
     0x3FEEBCCAC0000000uL, 0x3FEEB7ADC0000000uL,
     0x3FEEB29280000000uL, 0x3FEEAD7900000000uL,
     0x3FEEA86140000000uL, 0x3FEEA34B40000000uL,
     0x3FEE9E36C0000000uL, 0x3FEE992400000000uL,
     0x3FEE941300000000uL, 0x3FEE8F0380000000uL,
     0x3FEE89F5C0000000uL, 0x3FEE84E9C0000000uL,
     0x3FEE7FDF40000000uL, 0x3FEE7AD680000000uL,
     0x3FEE75CF80000000uL, 0x3FEE70CA00000000uL,
     0x3FEE6BC640000000uL, 0x3FEE66C440000000uL,
     0x3FEE61C3C0000000uL, 0x3FEE5CC500000000uL,
     0x3FEE57C7C0000000uL, 0x3FEE52CC40000000uL,
     0x3FEE4DD280000000uL, 0x3FEE48DA00000000uL,
     0x3FEE43E380000000uL, 0x3FEE3EEE80000000uL,
     0x3FEE39FB00000000uL, 0x3FEE350940000000uL,
     0x3FEE301940000000uL, 0x3FEE2B2AC0000000uL,
     0x3FEE263DC0000000uL, 0x3FEE215280000000uL,
     0x3FEE1C68C0000000uL, 0x3FEE178080000000uL,
     0x3FEE129A00000000uL, 0x3FEE0DB540000000uL,
     0x3FEE08D1C0000000uL, 0x3FEE03F000000000uL,
     0x3FEDFF1000000000uL, 0x3FEDFA3140000000uL,
     0x3FEDF55440000000uL, 0x3FEDF07900000000uL,
     0x3FEDEB9F00000000uL, 0x3FEDE6C6C0000000uL,
     0x3FEDE1F040000000uL, 0x3FEDDD1B00000000uL,
     0x3FEDD84780000000uL, 0x3FEDD37580000000uL,
     0x3FEDCEA500000000uL, 0x3FEDC9D600000000uL,
     0x3FEDC508C0000000uL, 0x3FEDC03D00000000uL,
     0x3FEDBB72C0000000uL, 0x3FEDB6AA00000000uL,
     0x3FEDB1E2C0000000uL, 0x3FEDAD1D00000000uL,
     0x3FEDA85900000000uL, 0x3FEDA39680000000uL,
     0x3FED9ED540000000uL, 0x3FED9A15C0000000uL,
     0x3FED9557C0000000uL, 0x3FED909B40000000uL,
     0x3FED8BE040000000uL, 0x3FED8726C0000000uL,
     0x3FED826F00000000uL, 0x3FED7DB880000000uL,
     0x3FED790380000000uL, 0x3FED745000000000uL,
     0x3FED6F9E40000000uL, 0x3FED6AEDC0000000uL,
     0x3FED663EC0000000uL, 0x3FED619140000000uL,
     0x3FED5CE540000000uL, 0x3FED583AC0000000uL,
     0x3FED5391C0000000uL, 0x3FED4EEA40000000uL,
     0x3FED4A4440000000uL, 0x3FED459F80000000uL,
     0x3FED40FC80000000uL, 0x3FED3C5AC0000000uL,
     0x3FED37BAC0000000uL, 0x3FED331C00000000uL,
     0x3FED2E7EC0000000uL, 0x3FED29E300000000uL,
     0x3FED254880000000uL, 0x3FED20AFC0000000uL,
     0x3FED1C1840000000uL, 0x3FED178240000000uL,
     0x3FED12EDC0000000uL, 0x3FED0E5AC0000000uL,
     0x3FED09C900000000uL, 0x3FED0538C0000000uL,
     0x3FED00AA00000000uL, 0x3FECFC1C80000000uL,
     0x3FECF790C0000000uL, 0x3FECF30600000000uL,
     0x3FECEE7D00000000uL, 0x3FECE9F540000000uL,
     0x3FECE56F00000000uL, 0x3FECE0EA40000000uL,
     0x3FECDC66C0000000uL, 0x3FECD7E4C0000000uL,
     0x3FECD36440000000uL, 0x3FECCEE500000000uL,
     0x3FECCA6740000000uL, 0x3FECC5EAC0000000uL,
     0x3FECC16FC0000000uL, 0x3FECBCF640000000uL,
     0x3FECB87E00000000uL, 0x3FECB40740000000uL,
     0x3FECAF91C0000000uL, 0x3FECAB1DC0000000uL,
     0x3FECA6AB00000000uL, 0x3FECA239C0000000uL,
     0x3FEC9DC9C0000000uL, 0x3FEC995B40000000uL,
     0x3FEC94EE00000000uL, 0x3FEC908240000000uL,
     0x3FEC8C17C0000000uL, 0x3FEC87AEC0000000uL,
     0x3FEC834700000000uL, 0x3FEC7EE0C0000000uL,
     0x3FEC7A7BC0000000uL, 0x3FEC761800000000uL,
     0x3FEC71B5C0000000uL, 0x3FEC6D54C0000000uL,
     0x3FEC68F540000000uL, 0x3FEC649700000000uL,
     0x3FEC603A00000000uL, 0x3FEC5BDE80000000uL,
     0x3FEC578440000000uL, 0x3FEC532B80000000uL,
     0x3FEC4ED3C0000000uL, 0x3FEC4A7DC0000000uL,
     0x3FEC4628C0000000uL, 0x3FEC41D540000000uL,
     0x3FEC3D8300000000uL, 0x3FEC393200000000uL,
     0x3FEC34E240000000uL, 0x3FEC309400000000uL,
     0x3FEC2C4700000000uL, 0x3FEC27FB80000000uL,
     0x3FEC23B100000000uL, 0x3FEC1F6800000000uL,
     0x3FEC1B2040000000uL, 0x3FEC16D9C0000000uL,
     0x3FEC1294C0000000uL, 0x3FEC0E50C0000000uL,
     0x3FEC0A0E40000000uL, 0x3FEC05CD00000000uL,
     0x3FEC018D00000000uL, 0x3FEBFD4E40000000uL,
     0x3FEBF91100000000uL, 0x3FEBF4D4C0000000uL,
     0x3FEBF09A00000000uL, 0x3FEBEC6080000000uL,
     0x3FEBE82840000000uL, 0x3FEBE3F140000000uL,
     0x3FEBDFBB80000000uL, 0x3FEBDB8700000000uL,
     0x3FEBD753C0000000uL, 0x3FEBD32200000000uL,
     0x3FEBCEF140000000uL, 0x3FEBCAC1C0000000uL,
     0x3FEBC693C0000000uL, 0x3FEBC266C0000000uL,
     0x3FEBBE3B40000000uL, 0x3FEBBA10C0000000uL,
     0x3FEBB5E7C0000000uL, 0x3FEBB1BFC0000000uL,
     0x3FEBAD9940000000uL, 0x3FEBA973C0000000uL,
     0x3FEBA54FC0000000uL, 0x3FEBA12CC0000000uL,
     0x3FEB9D0B00000000uL, 0x3FEB98EAC0000000uL,
     0x3FEB94CB80000000uL, 0x3FEB90AD80000000uL,
     0x3FEB8C90C0000000uL, 0x3FEB887540000000uL,
     0x3FEB845B00000000uL, 0x3FEB8041C0000000uL,
     0x3FEB7C2A00000000uL, 0x3FEB781340000000uL,
     0x3FEB73FE00000000uL, 0x3FEB6FE9C0000000uL,
     0x3FEB6BD6C0000000uL, 0x3FEB67C500000000uL,
     0x3FEB63B440000000uL, 0x3FEB5FA500000000uL,
     0x3FEB5B96C0000000uL, 0x3FEB5789C0000000uL,
     0x3FEB537E00000000uL, 0x3FEB4F7380000000uL,
     0x3FEB4B6A00000000uL, 0x3FEB476200000000uL,
     0x3FEB435B00000000uL, 0x3FEB3F5540000000uL,
     0x3FEB3B5080000000uL, 0x3FEB374D00000000uL,
     0x3FEB334AC0000000uL, 0x3FEB2F49C0000000uL,
     0x3FEB2B49C0000000uL, 0x3FEB274B40000000uL,
     0x3FEB234D80000000uL, 0x3FEB1F5140000000uL,
     0x3FEB1B5600000000uL, 0x3FEB175C00000000uL,
     0x3FEB136300000000uL, 0x3FEB0F6B80000000uL,
     0x3FEB0B74C0000000uL, 0x3FEB077F80000000uL,
     0x3FEB038B40000000uL, 0x3FEAFF9840000000uL,
     0x3FEAFBA640000000uL, 0x3FEAF7B580000000uL,
     0x3FEAF3C600000000uL, 0x3FEAEFD780000000uL,
     0x3FEAEBEA40000000uL, 0x3FEAE7FE00000000uL,
     0x3FEAE41300000000uL, 0x3FEAE02900000000uL,
     0x3FEADC4040000000uL, 0x3FEAD858C0000000uL,
     0x3FEAD47240000000uL, 0x3FEAD08CC0000000uL,
     0x3FEACCA8C0000000uL, 0x3FEAC8C580000000uL,
     0x3FEAC4E380000000uL, 0x3FEAC102C0000000uL,
     0x3FEABD2300000000uL, 0x3FEAB94480000000uL,
     0x3FEAB56700000000uL, 0x3FEAB18A80000000uL,
     0x3FEAADAF80000000uL, 0x3FEAA9D540000000uL,
     0x3FEAA5FC40000000uL, 0x3FEAA22440000000uL,
     0x3FEA9E4D80000000uL, 0x3FEA9A77C0000000uL,
     0x3FEA96A340000000uL, 0x3FEA92CFC0000000uL,
     0x3FEA8EFD80000000uL, 0x3FEA8B2C40000000uL,
     0x3FEA875C00000000uL, 0x3FEA838CC0000000uL,
     0x3FEA7FBEC0000000uL, 0x3FEA7BF200000000uL,
     0x3FEA782640000000uL, 0x3FEA745B80000000uL,
     0x3FEA7091C0000000uL, 0x3FEA6CC940000000uL,
     0x3FEA6901C0000000uL, 0x3FEA653B40000000uL,
     0x3FEA617600000000uL, 0x3FEA5DB1C0000000uL,
     0x3FEA59EE80000000uL, 0x3FEA562C80000000uL,
     0x3FEA526B80000000uL, 0x3FEA4EAB80000000uL,
     0x3FEA4AECC0000000uL, 0x3FEA472EC0000000uL,
     0x3FEA437200000000uL, 0x3FEA3FB640000000uL,
     0x3FEA3BFBC0000000uL, 0x3FEA384240000000uL,
     0x3FEA3489C0000000uL, 0x3FEA30D240000000uL,
     0x3FEA2D1BC0000000uL, 0x3FEA296680000000uL,
     0x3FEA25B200000000uL, 0x3FEA21FEC0000000uL,
     0x3FEA1E4CC0000000uL, 0x3FEA1A9B80000000uL,
     0x3FEA16EB40000000uL, 0x3FEA133C40000000uL,
     0x3FEA0F8E40000000uL, 0x3FEA0BE140000000uL,
     0x3FEA083540000000uL, 0x3FEA048A40000000uL,
     0x3FEA00E080000000uL, 0x3FE9FD3780000000uL,
     0x3FE9F98FC0000000uL, 0x3FE9F5E900000000uL,
     0x3FE9F24340000000uL, 0x3FE9EE9E80000000uL,
     0x3FE9EAFAC0000000uL, 0x3FE9E75800000000uL,
     0x3FE9E3B640000000uL, 0x3FE9E01580000000uL,
     0x3FE9DC7600000000uL, 0x3FE9D8D740000000uL,
     0x3FE9D539C0000000uL, 0x3FE9D19D00000000uL,
     0x3FE9CE0180000000uL, 0x3FE9CA66C0000000uL,
     0x3FE9C6CD40000000uL, 0x3FE9C33480000000uL,
     0x3FE9BF9D00000000uL, 0x3FE9BC0680000000uL,
     0x3FE9B870C0000000uL, 0x3FE9B4DC40000000uL,
     0x3FE9B148C0000000uL, 0x3FE9ADB600000000uL,
     0x3FE9AA2480000000uL, 0x3FE9A693C0000000uL,
     0x3FE9A30440000000uL, 0x3FE99F7580000000uL,
     0x3FE99BE7C0000000uL, 0x3FE9985B40000000uL,
     0x3FE994CF80000000uL, 0x3FE99144C0000000uL,
     0x3FE98DBB00000000uL, 0x3FE98A3240000000uL,
     0x3FE986AA80000000uL, 0x3FE98323C0000000uL,
     0x3FE97F9E00000000uL, 0x3FE97C1900000000uL,
     0x3FE9789540000000uL, 0x3FE9751240000000uL,
     0x3FE9719080000000uL, 0x3FE96E0F80000000uL,
     0x3FE96A8F80000000uL, 0x3FE9671040000000uL,
     0x3FE9639240000000uL, 0x3FE9601540000000uL,
     0x3FE95C9900000000uL, 0x3FE9591DC0000000uL,
     0x3FE955A380000000uL, 0x3FE9522A40000000uL,
     0x3FE94EB200000000uL, 0x3FE94B3A80000000uL,
     0x3FE947C400000000uL, 0x3FE9444E80000000uL,
     0x3FE940DA00000000uL, 0x3FE93D6640000000uL,
     0x3FE939F3C0000000uL, 0x3FE9368200000000uL,
     0x3FE9331140000000uL, 0x3FE92FA140000000uL,
     0x3FE92C3280000000uL, 0x3FE928C480000000uL,
     0x3FE9255780000000uL, 0x3FE921EB40000000uL,
     0x3FE91E8040000000uL, 0x3FE91B1600000000uL,
     0x3FE917AC80000000uL, 0x3FE9144440000000uL,
     0x3FE910DCC0000000uL, 0x3FE90D7640000000uL,
     0x3FE90A1080000000uL, 0x3FE906ABC0000000uL,
     0x3FE9034800000000uL, 0x3FE8FFE540000000uL,
     0x3FE8FC8340000000uL, 0x3FE8F92240000000uL,
     0x3FE8F5C200000000uL, 0x3FE8F26300000000uL,
     0x3FE8EF0480000000uL, 0x3FE8EBA740000000uL,
     0x3FE8E84AC0000000uL, 0x3FE8E4EF40000000uL,
     0x3FE8E19480000000uL, 0x3FE8DE3AC0000000uL,
     0x3FE8DAE1C0000000uL, 0x3FE8D78A00000000uL,
     0x3FE8D432C0000000uL, 0x3FE8D0DCC0000000uL,
     0x3FE8CD8780000000uL, 0x3FE8CA3300000000uL,
     0x3FE8C6DF80000000uL, 0x3FE8C38D00000000uL,
     0x3FE8C03B40000000uL, 0x3FE8BCEA80000000uL,
     0x3FE8B99A80000000uL, 0x3FE8B64B80000000uL,
     0x3FE8B2FD40000000uL, 0x3FE8AFB000000000uL,
     0x3FE8AC63C0000000uL, 0x3FE8A91840000000uL,
     0x3FE8A5CD80000000uL, 0x3FE8A283C0000000uL,
     0x3FE89F3B00000000uL, 0x3FE89BF300000000uL,
     0x3FE898ABC0000000uL, 0x3FE8956580000000uL,
     0x3FE8922040000000uL, 0x3FE88EDBC0000000uL,
     0x3FE88B9800000000uL, 0x3FE8885540000000uL,
     0x3FE8851380000000uL, 0x3FE881D240000000uL,
     0x3FE87E9240000000uL, 0x3FE87B52C0000000uL,
     0x3FE8781480000000uL, 0x3FE874D6C0000000uL,
     0x3FE8719A00000000uL, 0x3FE86E5E40000000uL,
     0x3FE86B2340000000uL, 0x3FE867E900000000uL,
     0x3FE864AFC0000000uL, 0x3FE8617740000000uL,
     0x3FE85E3F80000000uL, 0x3FE85B08C0000000uL,
     0x3FE857D300000000uL, 0x3FE8549DC0000000uL,
     0x3FE8516980000000uL, 0x3FE84E3640000000uL,
     0x3FE84B03C0000000uL, 0x3FE847D200000000uL,
     0x3FE844A100000000uL, 0x3FE8417100000000uL,
     0x3FE83E4200000000uL, 0x3FE83B1380000000uL,
     0x3FE837E600000000uL, 0x3FE834B940000000uL,
     0x3FE8318D80000000uL, 0x3FE82E6280000000uL,
     0x3FE82B3840000000uL, 0x3FE8280F00000000uL,
     0x3FE824E640000000uL, 0x3FE821BEC0000000uL,
     0x3FE81E97C0000000uL, 0x3FE81B71C0000000uL,
     0x3FE8184C80000000uL, 0x3FE8152800000000uL,
     0x3FE8120480000000uL, 0x3FE80EE1C0000000uL,
     0x3FE80BBFC0000000uL, 0x3FE8089E80000000uL,
     0x3FE8057E40000000uL, 0x3FE8025EC0000000uL,
     0x3FE7FF4000000000uL, 0x3FE7FC2200000000uL,
     0x3FE7F90500000000uL, 0x3FE7F5E8C0000000uL,
     0x3FE7F2CD40000000uL, 0x3FE7EFB280000000uL,
     0x3FE7EC9880000000uL, 0x3FE7E97F80000000uL,
     0x3FE7E66740000000uL, 0x3FE7E34FC0000000uL,
     0x3FE7E03940000000uL, 0x3FE7DD2340000000uL,
     0x3FE7DA0E40000000uL, 0x3FE7D6FA00000000uL,
     0x3FE7D3E680000000uL, 0x3FE7D0D3C0000000uL,
     0x3FE7CDC1C0000000uL, 0x3FE7CAB0C0000000uL,
     0x3FE7C7A080000000uL, 0x3FE7C49100000000uL,
     0x3FE7C18240000000uL, 0x3FE7BE7440000000uL,
     0x3FE7BB6700000000uL, 0x3FE7B85AC0000000uL,
     0x3FE7B54F00000000uL, 0x3FE7B24440000000uL,
     0x3FE7AF3A40000000uL, 0x3FE7AC3100000000uL,
     0x3FE7A92880000000uL, 0x3FE7A620C0000000uL,
     0x3FE7A319C0000000uL, 0x3FE7A013C0000000uL,
     0x3FE79D0E40000000uL, 0x3FE79A09C0000000uL,
     0x3FE7970600000000uL, 0x3FE79402C0000000uL,
     0x3FE7910080000000uL, 0x3FE78DFF00000000uL,
     0x3FE78AFE40000000uL, 0x3FE787FE40000000uL,
     0x3FE784FF00000000uL, 0x3FE7820080000000uL,
     0x3FE77F02C0000000uL, 0x3FE77C05C0000000uL,
     0x3FE77909C0000000uL, 0x3FE7760E40000000uL,
     0x3FE7731380000000uL, 0x3FE77019C0000000uL,
     0x3FE76D2080000000uL, 0x3FE76A2800000000uL,
     0x3FE7673080000000uL, 0x3FE7643980000000uL,
     0x3FE7614340000000uL, 0x3FE75E4E00000000uL,
     0x3FE75B5940000000uL, 0x3FE7586580000000uL,
     0x3FE7557240000000uL, 0x3FE7527FC0000000uL,
     0x3FE74F8E40000000uL, 0x3FE74C9D40000000uL,
     0x3FE749AD00000000uL, 0x3FE746BD80000000uL,
     0x3FE743CEC0000000uL, 0x3FE740E100000000uL,
     0x3FE73DF3C0000000uL, 0x3FE73B0740000000uL,
     0x3FE7381B80000000uL, 0x3FE7353080000000uL,
     0x3FE7324600000000uL, 0x3FE72F5C80000000uL,
     0x3FE72C73C0000000uL, 0x3FE7298B80000000uL,
     0x3FE726A440000000uL, 0x3FE723BD80000000uL,
     0x3FE720D7C0000000uL, 0x3FE71DF280000000uL,
     0x3FE71B0E00000000uL, 0x3FE7182A40000000uL,
     0x3FE7154740000000uL, 0x0000000000000000uL,
     },
    {

     0x0000000000000000uL, 0x0000000000000000uL,
     0x3F5712E100000000uL, 0x3E0EE8A22F7C5987uL,
     0x3F670FC100000000uL, 0x3E17E16043FD7529uL,
     0x3F71497700000000uL, 0x3E239EFB866B119CuL,
     0x3F7709BB00000000uL, 0x3E1B5EA7EE997DC0uL,
     0x3F7CC8AA00000000uL, 0x3E2EFAD156451E8DuL,
     0x3F81430200000000uL, 0x3E204975BF955EE8uL,
     0x3F84210300000000uL, 0x3E2E526353333F9AuL,
     0x3F86FE5800000000uL, 0x3E2DBBC5D9986525uL,
     0x3F89DAE000000000uL, 0x3E211AE127D370F8uL,
     0x3F8CB6BA00000000uL, 0x3E2AF44E8A20FE77uL,
     0x3F8F91E600000000uL, 0x3E1F77BD1CD9FBC7uL,
     0x3F91363100000000uL, 0x3E40F52F789C83A3uL,
     0x3F92A31800000000uL, 0x3E172308C2064B24uL,
     0x3F940F9600000000uL, 0x3E2F342D9EB8AEEDuL,
     0x3F957BBB00000000uL, 0x3E4ABB9A144866B7uL,
     0x3F96E79800000000uL, 0x3E48B85AC72B0200uL,
     0x3F98530C00000000uL, 0x3E2D1E01FBC85D86uL,
     0x3F99BE3600000000uL, 0x3E37D26F00CDA0DDuL,
     0x3F9B28F600000000uL, 0x3E3433218E840F16uL,
     0x3F9C935B00000000uL, 0x3E4F50A107FB8C37uL,
     0x3F9DFD7700000000uL, 0x3E3604E609A9E948uL,
     0x3F9F673700000000uL, 0x3E489F0DE52D1118uL,
     0x3FA0684E00000000uL, 0x3E4D127BD17ABD42uL,
     0x3FA11CD300000000uL, 0x3E3A899B4ECE6057uL,
     0x3FA1D12900000000uL, 0x3E5F0D0F99858CFAuL,
     0x3FA2855A00000000uL, 0x3E58B94E89D977A4uL,
     0x3FA3395D00000000uL, 0x3E402A7F6BF76796uL,
     0x3FA3ED3100000000uL, 0x3E3E342DA3E0AAB6uL,
     0x3FA4A0DE00000000uL, 0x3E58CAE94CD5496BuL,
     0x3FA5545500000000uL, 0x3E3FDC64D89D4032uL,
     0x3FA607AD00000000uL, 0x3E37DFD30F154124uL,
     0x3FA6BAD500000000uL, 0x3E5EB1E05460B0E3uL,
     0x3FA76DCF00000000uL, 0x3E490EAD14C7109DuL,
     0x3FA820A100000000uL, 0x3E5258EAF10715E3uL,
     0x3FA8D34400000000uL, 0x3E242A28E25FB4D0uL,
     0x3FA985BF00000000uL, 0x3DFA4A83C146EC0FuL,
     0x3FAA381200000000uL, 0x3E3C7DE45FE856F6uL,
     0x3FAAEA3500000000uL, 0x3E408258F0914A28uL,
     0x3FAB9C3000000000uL, 0x3E3F9589C628DFE0uL,
     0x3FAC4DFA00000000uL, 0x3E5721556BDE9F1FuL,
     0x3FACFF9C00000000uL, 0x3E5A8867F80F2A46uL,
     0x3FADB11600000000uL, 0x3E4A583C979A598EuL,
     0x3FAE626700000000uL, 0x3E443847800C1405uL,
     0x3FAF138700000000uL, 0x3E1664A168A10688uL,
     0x3FAFC48600000000uL, 0x3E2EB49173242E2EuL,
     0x3FB03AA900000000uL, 0x3E6B1B90DF1D2899uL,
     0x3FB092FB00000000uL, 0x3E6F4828DCE8EF96uL,
     0x3FB0EB3900000000uL, 0x3E57E8A84071ED7CuL,
     0x3FB1436100000000uL, 0x3E6EA26E46FC50E3uL,
     0x3FB19B7500000000uL, 0x3E64D3EC52377554uL,
     0x3FB1F37000000000uL, 0x3E46A5728109990DuL,
     0x3FB24B5900000000uL, 0x3E6B426B10E12CA0uL,
     0x3FB2A32E00000000uL, 0x3E59BBBA7C1B46C7uL,
     0x3FB2FAED00000000uL, 0x3E67F99638784FAFuL,
     0x3FB3529C00000000uL, 0x3E1E52F196858161uL,
     0x3FB3AA3000000000uL, 0x3E67A4FE6DEF19E6uL,
     0x3FB401B000000000uL, 0x3E0302A326E6A3DCuL,
     0x3FB4591D00000000uL, 0x3E6FA21B2E435F49uL,
     0x3FB4B07600000000uL, 0x3E58415E51626967uL,
     0x3FB507B900000000uL, 0x3E3A033D6C5941C4uL,
     0x3FB55EE600000000uL, 0x3E33C8467C54296BuL,
     0x3FB5B60100000000uL, 0x3E5E02F5A12FE65DuL,
     0x3FB60D0600000000uL, 0x3E6ECFC86D9ED70DuL,
     0x3FB663F600000000uL, 0x3E5EB24497A376B8uL,
     0x3FB6BAD400000000uL, 0x3E48C77F72E2B40FuL,
     0x3FB7119B00000000uL, 0x3E68ED7D5E52D89EuL,
     0x3FB7684D00000000uL, 0x3E43FA7EA9D3799BuL,
     0x3FB7BEEC00000000uL, 0x3E60571414F770DBuL,
     0x3FB8157900000000uL, 0x3E68C7D07F316EE3uL,
     0x3FB86BF000000000uL, 0x3E6360F420C77BECuL,
     0x3FB8C25000000000uL, 0x3E6D91C947D50FA1uL,
     0x3FB918A300000000uL, 0x3E4B231BA93BD154uL,
     0x3FB96EDA00000000uL, 0x3E61D38C8099FDDDuL,
     0x3FB9C50300000000uL, 0x3E677EEB9B0174ACuL,
     0x3FBA1B1100000000uL, 0x3E69D6DDD016014CuL,
     0x3FBA711100000000uL, 0x3E626690842B7789uL,
     0x3FBAC6FA00000000uL, 0x3E5830B93095C531uL,
     0x3FBB1CD000000000uL, 0x3E5C2B99518E0D2CuL,
     0x3FBB729300000000uL, 0x3E66279B91823620uL,
     0x3FBBC84400000000uL, 0x3E30ADAFC9057ECCuL,
     0x3FBC1DDD00000000uL, 0x3E461CE45269682AuL,
     0x3FBC736300000000uL, 0x3E5044EF5F2FE276uL,
     0x3FBCC8D600000000uL, 0x3E4EB3DBD5234CE7uL,
     0x3FBD1E3600000000uL, 0x3E2EB70A6E724019uL,
     0x3FBD737E00000000uL, 0x3E5403A5977B9A51uL,
     0x3FBDC8B700000000uL, 0x3E62D343B2886C33uL,
     0x3FBE1DDD00000000uL, 0x3E5F443CFBD572A9uL,
     0x3FBE72EB00000000uL, 0x3E632FF4A08C00ADuL,
     0x3FBEC7EA00000000uL, 0x3E611D934F5C870BuL,
     0x3FBF1CD100000000uL, 0x3E610AFC18ECC7FDuL,
     0x3FBF71A900000000uL, 0x3E4C5DB9D4383F15uL,
     0x3FBFC66800000000uL, 0x3E6A615FE5DCF50AuL,
     0x3FC00D8C00000000uL, 0x3E6F8684B8524B4DuL,
     0x3FC037DA00000000uL, 0x3E7471E52C396096uL,
     0x3FC0621E00000000uL, 0x3E7A1AAD94D3758AuL,
     0x3FC08C5800000000uL, 0x3E7F9B4F573CD19DuL,
     0x3FC0B68900000000uL, 0x3E4E88E925A98AFDuL,
     0x3FC0E0B100000000uL, 0x3E677212D0EEB433uL,
     0x3FC10ACD00000000uL, 0x3E63FF48E459228FuL,
     0x3FC134E100000000uL, 0x3E63A241697ADC33uL,
     0x3FC15EEB00000000uL, 0x3E4F4A7AE82699A0uL,
     0x3FC188EC00000000uL, 0x3E7D83A2E1FE8196uL,
     0x3FC1B2E400000000uL, 0x3E6E765C52C5B577uL,
     0x3FC1DCD100000000uL, 0x3E77EAA5780399BEuL,
     0x3FC206B400000000uL, 0x3E766C5EF95AB1FCuL,
     0x3FC2308F00000000uL, 0x3E703A52D5DB6084uL,
     0x3FC25A6200000000uL, 0x3E51786D7D82F6F1uL,
     0x3FC2842A00000000uL, 0x3E6641EA2DED60B8uL,
     0x3FC2ADE800000000uL, 0x3E4ADDFBEAA772F7uL,
     0x3FC2D79B00000000uL, 0x3E67CDFBBC061E04uL,
     0x3FC3014800000000uL, 0x3E717AD775A7481BuL,
     0x3FC32AE800000000uL, 0x3E7E4F15A673BAF4uL,
     0x3FC3548300000000uL, 0x3E58ECA1813FA934uL,
     0x3FC37E1200000000uL, 0x3E7A3622382E96FBuL,
     0x3FC3A79700000000uL, 0x3E7916BB2A2CEA0AuL,
     0x3FC3D11400000000uL, 0x3E61E6A28AAA11CBuL,
     0x3FC3FA8800000000uL, 0x3E61A3CECA68F920uL,
     0x3FC423F100000000uL, 0x3E705825C8CAF8EDuL,
     0x3FC44D5200000000uL, 0x3E572D6F71F4B037uL,
     0x3FC476AA00000000uL, 0x3E6060FDF3CABB49uL,
     0x3FC49FF700000000uL, 0x3E6DF855C48E67AAuL,
     0x3FC4C93E00000000uL, 0x3E60854767C83D89uL,
     0x3FC4F27700000000uL, 0x3E7C27D2ADFA3CF1uL,
     0x3FC51BAB00000000uL, 0x3E21E96F77A9B8FFuL,
     0x3FC544D500000000uL, 0x3E69B89066DA0127uL,
     0x3FC56DF400000000uL, 0x3E7831AB063F0639uL,
     0x3FC5970B00000000uL, 0x3E62A3FF97F4402EuL,
     0x3FC5C01B00000000uL, 0x3E5CFDEC6AA61224uL,
     0x3FC5E92000000000uL, 0x3E30BF99A341739BuL,
     0x3FC6121900000000uL, 0x3E7589025C069AF7uL,
     0x3FC63B0C00000000uL, 0x3E73E7C70DC28176uL,
     0x3FC663F600000000uL, 0x3E7319225255ED92uL,
     0x3FC68CD700000000uL, 0x3E721D999E92E626uL,
     0x3FC6B5AF00000000uL, 0x3E6FEABA3C111C8AuL,
     0x3FC6DE7E00000000uL, 0x3E67408FFBA276E0uL,
     0x3FC7074100000000uL, 0x3E7B9DE032CB0FD0uL,
     0x3FC72FFE00000000uL, 0x3E6FBAB18DF0F78EuL,
     0x3FC758B100000000uL, 0x3E7EED8F544CC58AuL,
     0x3FC7815C00000000uL, 0x3E5F34382F992A55uL,
     0x3FC7A9FF00000000uL, 0x3E723A0BF2565894uL,
     0x3FC7D29700000000uL, 0x3E6784D72660BF64uL,
     0x3FC7FB2800000000uL, 0x3E53CEF9F2A00FDAuL,
     0x3FC823AD00000000uL, 0x3E6636827E73660EuL,
     0x3FC84C2B00000000uL, 0x3E6E0BC0CE905E5FuL,
     0x3FC874A000000000uL, 0x3E5B40D32CA21B4FuL,
     0x3FC89D0D00000000uL, 0x3E7A968650124684uL,
     0x3FC8C56F00000000uL, 0x3E7724C9F4C54DC2uL,
     0x3FC8EDCA00000000uL, 0x3E6B8D4AB3E3B13CuL,
     0x3FC9161B00000000uL, 0x3E74576BCFDAFE5EuL,
     0x3FC93E6500000000uL, 0x3E7332208C376C3FuL,
     0x3FC966A600000000uL, 0x3DF175E083C82DEBuL,
     0x3FC98EDC00000000uL, 0x3E79EFCE11AA7D30uL,
     0x3FC9B70C00000000uL, 0x3E62AE7840B35985uL,
     0x3FC9DF3200000000uL, 0x3E4E8C13081D57DCuL,
     0x3FCA074E00000000uL, 0x3E60B028BF61097BuL,
     0x3FCA2F6200000000uL, 0x3E7FA41706304E8FuL,
     0x3FCA576D00000000uL, 0x3E7F0E5F94377493uL,
     0x3FCA7F7100000000uL, 0x3E6EDEEABEEEAB1AuL,
     0x3FCAA76D00000000uL, 0x3E6FDF22F0CA6C0DuL,
     0x3FCACF5D00000000uL, 0x3E676D3AEE892F9CuL,
     0x3FCAF74700000000uL, 0x3E7FBC37F3121AB7uL,
     0x3FCB1F2800000000uL, 0x3E7717AF8E5DD5B2uL,
     0x3FCB46FF00000000uL, 0x3E70C006784D6D72uL,
     0x3FCB6ECE00000000uL, 0x3E75EBF2ABE7A8F0uL,
     0x3FCB969600000000uL, 0x3E570772E1AA6F94uL,
     0x3FCBBE5300000000uL, 0x3E7507E05D60E5C4uL,
     0x3FCBE60900000000uL, 0x3E6A479C1C7622D5uL,
     0x3FCC0DB700000000uL, 0x3E6A7653CAD63A6AuL,
     0x3FCC355B00000000uL, 0x3E63C6576AC08E77uL,
     0x3FCC5CF700000000uL, 0x3E696181FF9674A7uL,
     0x3FCC848B00000000uL, 0x3E74C88B88CB08D4uL,
     0x3FCCAC1500000000uL, 0x3E768EE1A3F58613uL,
     0x3FCCD39700000000uL, 0x3E7BC7D00E53901CuL,
     0x3FCCFB1200000000uL, 0x3E4CB8C314503175uL,
     0x3FCD228400000000uL, 0x3E6A40646984129BuL,
     0x3FCD49EE00000000uL, 0x3E77864B48C32B3CuL,
     0x3FCD714E00000000uL, 0x3E76DC470F22F1EEuL,
     0x3FCD98A900000000uL, 0x3E153043B87205ACuL,
     0x3FCDBFF800000000uL, 0x3E7CE2096F5BAED1uL,
     0x3FCDE74000000000uL, 0x3E76B6293B0E2EA0uL,
     0x3FCE0E8000000000uL, 0x3E69E5C03298A8D0uL,
     0x3FCE35B500000000uL, 0x3E7359A4ADD9086CuL,
     0x3FCE5CE400000000uL, 0x3E7FBBA6E4320B0BuL,
     0x3FCE840C00000000uL, 0x3E57A7356760BF17uL,
     0x3FCEAB2B00000000uL, 0x3E5412DD4C71D4AAuL,
     0x3FCED23F00000000uL, 0x3E708CBBD3DE4F64uL,
     0x3FCEF94D00000000uL, 0x3E7ED1EC6FB9EF8FuL,
     0x3FCF205400000000uL, 0x3E4B20911D7E37DBuL,
     0x3FCF474F00000000uL, 0x3E7192AEE74AAF85uL,
     0x3FCF6E4500000000uL, 0x3DE9FF7395251CF5uL,
     0x3FCF953200000000uL, 0x3E418FCF45710FC3uL,
     0x3FCFBC1600000000uL, 0x3E77204D0144751BuL,
     0x3FCFE2F200000000uL, 0x3E7DF662B4D59D8EuL,
     0x3FD004E300000000uL, 0x3E75D25F17B09D21uL,
     0x3FD0184A00000000uL, 0x3E64044284485CA5uL,
     0x3FD02BAB00000000uL, 0x3E80A9A0C732CB2CuL,
     0x3FD03F0900000000uL, 0x3E89A98AD1490635uL,
     0x3FD0526300000000uL, 0x3E897756562A827FuL,
     0x3FD065B900000000uL, 0x3E7F42D1CECD3768uL,
     0x3FD0790A00000000uL, 0x3E8BB6060195A070uL,
     0x3FD08C5900000000uL, 0x3E5C5A7B3A2BD335uL,
     0x3FD09FA100000000uL, 0x3E8A2743F6A4CD20uL,
     0x3FD0B2E700000000uL, 0x3E775F83F99025B0uL,
     0x3FD0C62900000000uL, 0x3E87CA856421A674uL,
     0x3FD0D96600000000uL, 0x3E814D2830EF12FDuL,
     0x3FD0ECA000000000uL, 0x3E62348ECA90F220uL,
     0x3FD0FFD600000000uL, 0x3E812FCF75D18B23uL,
     0x3FD1130700000000uL, 0x3E73B4C2BF9F9DD3uL,
     0x3FD1263600000000uL, 0x3E499EF30070A508uL,
     0x3FD1395F00000000uL, 0x3E61EDB0D9E8DA9BuL,
     0x3FD14C8400000000uL, 0x3E8F23AC3152C264uL,
     0x3FD15FA600000000uL, 0x3E752EC233B712ADuL,
     0x3FD172C400000000uL, 0x3E7A163986A7B84CuL,
     0x3FD185DD00000000uL, 0x3E8F734FDA450672uL,
     0x3FD198F400000000uL, 0x3E7028962C15F52BuL,
     0x3FD1AC0500000000uL, 0x3E8FD23E213F6416uL,
     0x3FD1BF1300000000uL, 0x3E68E4E3166C3339uL,
     0x3FD1D21E00000000uL, 0x3E70EA55E7DA3FECuL,
     0x3FD1E52300000000uL, 0x3E81B9E3403DF05DuL,
     0x3FD1F82500000000uL, 0x3E7E762367A00F4AuL,
     0x3FD20B2400000000uL, 0x3E3388B4DD9D8704uL,
     0x3FD21E1F00000000uL, 0x3E6603BBC7B763E4uL,
     0x3FD2311400000000uL, 0x3E7F38B9F767E1C9uL,
     0x3FD2440700000000uL, 0x3E8361C0E424306BuL,
     0x3FD256F600000000uL, 0x3E53E15A0763E5F5uL,
     0x3FD269E100000000uL, 0x3E5C346E0F5542ABuL,
     0x3FD27CC800000000uL, 0x3E8623BAC0F6E8E5uL,
     0x3FD28FAB00000000uL, 0x3E82D664EA511964uL,
     0x3FD2A28B00000000uL, 0x3E244827751649E1uL,
     0x3FD2B56500000000uL, 0x3E870662732A8325uL,
     0x3FD2C83C00000000uL, 0x3E8DB880F0396C05uL,
     0x3FD2DB1000000000uL, 0x3E8409B34923F5D0uL,
     0x3FD2EDE000000000uL, 0x3E899C121E8496E6uL,
     0x3FD300AD00000000uL, 0x3E7C232F22D20F20uL,
     0x3FD3137500000000uL, 0x3E73683D6C58CA0DuL,
     0x3FD3263900000000uL, 0x3E836D65141862CFuL,
     0x3FD338FA00000000uL, 0x3E75BE12EFC2F601uL,
     0x3FD34BB600000000uL, 0x3E70751869F3B7A6uL,
     0x3FD35E6F00000000uL, 0x3E89F95043BBFC91uL,
     0x3FD3712400000000uL, 0x3E80D499B29F7615uL,
     0x3FD383D500000000uL, 0x3E83DD8F4DE52902uL,
     0x3FD3968400000000uL, 0x3E748A73FA7E46E2uL,
     0x3FD3A92E00000000uL, 0x3E6252112C0E2155uL,
     0x3FD3BBD300000000uL, 0x3E52A1DC831E5AD7uL,
     0x3FD3CE7500000000uL, 0x3E825D1013E78284uL,
     0x3FD3E11400000000uL, 0x3E796F27F8ED6AB1uL,
     0x3FD3F3AF00000000uL, 0x3E81043C4E106F6AuL,
     0x3FD4064500000000uL, 0x3E8723607A748D45uL,
     0x3FD418D900000000uL, 0x3E7C5A76F3C6B991uL,
     0x3FD42B6900000000uL, 0x3E7C13D54B6EDE12uL,
     0x3FD43DF400000000uL, 0x3E7D02DC433313AEuL,
     0x3FD4507C00000000uL, 0x3E8EDBA9F6E1776CuL,
     0x3FD4630100000000uL, 0x3E86E864BF1D1AAAuL,
     0x3FD4758100000000uL, 0x3E7CAE90765ABC31uL,
     0x3FD487FE00000000uL, 0x3E849FE23646E5A5uL,
     0x3FD49A7800000000uL, 0x3E479A36743BE41DuL,
     0x3FD4ACED00000000uL, 0x3E8483E03299B840uL,
     0x3FD4BF5F00000000uL, 0x3E7ABBA144C6B22BuL,
     0x3FD4D1CD00000000uL, 0x3E774D20FDD9F23BuL,
     0x3FD4E43800000000uL, 0x3E871D1F7AA47E01uL,
     0x3FD4F69E00000000uL, 0x3E8F2860BA3B3DB5uL,
     0x3FD5090200000000uL, 0x3E83AF1C17099BFEuL,
     0x3FD51B6200000000uL, 0x3E785FF9DE74A1B4uL,
     0x3FD52DBE00000000uL, 0x3E709325CFAFA80FuL,
     0x3FD5401600000000uL, 0x3E6E6947CCF73D7AuL,
     0x3FD5526A00000000uL, 0x3E738124D5DB9AD7uL,
     0x3FD564BB00000000uL, 0x3E86B2911C62B3A2uL,
     0x3FD5770900000000uL, 0x3E6719BC759EE891uL,
     0x3FD5895200000000uL, 0x3E869A322D9370BCuL,
     0x3FD59B9800000000uL, 0x3E719789A94340E2uL,
     0x3FD5ADDB00000000uL, 0x3E61C3D9786A1C1AuL,
     0x3FD5C01A00000000uL, 0x3E37EF590A213419uL,
     0x3FD5D25400000000uL, 0x3E8D54EB1103130FuL,
     0x3FD5E48D00000000uL, 0x3E52F62A9CC12FD0uL,
     0x3FD5F6C100000000uL, 0x3E6BE9B244784641uL,
     0x3FD608F100000000uL, 0x3E758A521184B277uL,
     0x3FD61B1E00000000uL, 0x3E86042873323471uL,
     0x3FD62D4700000000uL, 0x3E8FBC7D80B47BCFuL,
     0x3FD63F6D00000000uL, 0x3E6E2C82077EA756uL,
     0x3FD6518F00000000uL, 0x3E85CCEF6BF767F4uL,
     0x3FD663AE00000000uL, 0x3E46EAD81DF81E8FuL,
     0x3FD675C900000000uL, 0x3E82DD03F10CD685uL,
     0x3FD687E100000000uL, 0x3E3E902C6DBC1F0CuL,
     0x3FD699F500000000uL, 0x3E84319ABAC9C4B2uL,
     0x3FD6AC0600000000uL, 0x3E5B055166C24B15uL,
     0x3FD6BE1200000000uL, 0x3E7C3BE07B4F7882uL,
     0x3FD6D01B00000000uL, 0x3E8CFD93DD847E5DuL,
     0x3FD6E22100000000uL, 0x3E6ACE863358E8D7uL,
     0x3FD6F42300000000uL, 0x3E83E40C6242BFE9uL,
     0x3FD7062300000000uL, 0x3E610AB6A8479B5DuL,
     0x3FD7181E00000000uL, 0x3E7CD689BCFD9CF6uL,
     0x3FD72A1600000000uL, 0x3E8B1978624662CCuL,
     0x3FD73C0B00000000uL, 0x3E3B1A8D9A80C213uL,
     0x3FD74DFA00000000uL, 0x3E8F44CC629FADC5uL,
     0x3FD75FE900000000uL, 0x3E70D17562376005uL,
     0x3FD771D300000000uL, 0x3E731FBF269B0088uL,
     0x3FD783B900000000uL, 0x3E52AB13F0273736uL,
     0x3FD7959B00000000uL, 0x3E8BA45253B127D6uL,
     0x3FD7A77B00000000uL, 0x3E852FA4783A4DFDuL,
     0x3FD7B95700000000uL, 0x3E6528D527430D54uL,
     0x3FD7CB2F00000000uL, 0x3E84F6C8A8C54418uL,
     0x3FD7DD0500000000uL, 0x3E5F404BA538C133uL,
     0x3FD7EED700000000uL, 0x3E81D08A084632F9uL,
     0x3FD800A500000000uL, 0x3E84E2C39B578D96uL,
     0x3FD8127000000000uL, 0x3E8641178F2C2B02uL,
     0x3FD8243700000000uL, 0x3E781B9C28EE919EuL,
     0x3FD835FA00000000uL, 0x3E8F7B17B6D5775CuL,
     0x3FD847BC00000000uL, 0x3E89DB0C612F1B2EuL,
     0x3FD8597800000000uL, 0x3E8DFFAAE2CBAD0FuL,
     0x3FD86B3300000000uL, 0x3E70F5B6D0513247uL,
     0x3FD87CE900000000uL, 0x3E6699B2D0C42CCAuL,
     0x3FD88E9B00000000uL, 0x3E8EDC16362782B3uL,
     0x3FD8A04B00000000uL, 0x3E83CD771D49FB4BuL,
     0x3FD8B1F800000000uL, 0x3E60B05B11747E4CuL,
     0x3FD8C3A100000000uL, 0x3E7F52C9816DB2C1uL,
     0x3FD8D54600000000uL, 0x3E782D70D541D6C1uL,
     0x3FD8E6E800000000uL, 0x3E57AA91CC153DDEuL,
     0x3FD8F88600000000uL, 0x3E83F65A8E01AFFCuL,
     0x3FD90A2100000000uL, 0x3E8ECAE2475966DFuL,
     0x3FD91BBA00000000uL, 0x3E591F169848D269uL,
     0x3FD92D4F00000000uL, 0x3E3647C7943A8D23uL,
     0x3FD93EE000000000uL, 0x3E8726BF3DB3E718uL,
     0x3FD9506D00000000uL, 0x3E8C1A18FAFA10D5uL,
     0x3FD961F900000000uL, 0x3E5B2740C198F220uL,
     0x3FD9737F00000000uL, 0x3E887FB1536242B8uL,
     0x3FD9850400000000uL, 0x3E7EC5C619B71F3EuL,
     0x3FD9968400000000uL, 0x3E8366D3EB0E5D24uL,
     0x3FD9A80200000000uL, 0x3E88A3C48F5901ADuL,
     0x3FD9B97C00000000uL, 0x3E74A3BB2D70054BuL,
     0x3FD9CAF200000000uL, 0x3E825931E77B3ED9uL,
     0x3FD9DC6600000000uL, 0x3E8AC1BD72BB6920uL,
     0x3FD9EDD600000000uL, 0x3E7D26C9777B80E6uL,
     0x3FD9FF4200000000uL, 0x3E87CDF6B003FE44uL,
     0x3FDA10AD00000000uL, 0x3E32256C5F5257DAuL,
     0x3FDA221200000000uL, 0x3E83B4A3FF1466D0uL,
     0x3FDA337600000000uL, 0x3E673FB048CD2B2FuL,
     0x3FDA44D600000000uL, 0x3E7844F0A7DA3C13uL,
     0x3FDA563100000000uL, 0x3E8BCBA6DA5B37E1uL,
     0x3FDA678B00000000uL, 0x3E7325816E447B2DuL,
     0x3FDA78E100000000uL, 0x3E753DEFC2FB5AA0uL,
     0x3FDA8A3300000000uL, 0x3E8E9F688620242EuL,
     0x3FDA9B8300000000uL, 0x3E650C63633BBEC2uL,
     0x3FDAACCE00000000uL, 0x3E8E38F926FACEDDuL,
     0x3FDABE1800000000uL, 0x3E83EFE3F1BC83EAuL,
     0x3FDACF5D00000000uL, 0x3E809E9D83CD28E8uL,
     0x3FDAE0A000000000uL, 0x3E72F7A9FEEA5B2AuL,
     0x3FDAF1DF00000000uL, 0x3E83762377A3C900uL,
     0x3FDB031B00000000uL, 0x3E7C7818EFDE9C0AuL,
     0x3FDB145500000000uL, 0x3E618FF8CE39A19EuL,
     0x3FDB258900000000uL, 0x3E8FD450B400CDC5uL,
     0x3FDB36BC00000000uL, 0x3E861347926AA708uL,
     0x3FDB47EB00000000uL, 0x3E8BE7104FA3A380uL,
     0x3FDB591700000000uL, 0x3E80FDC35B90EE8DuL,
     0x3FDB6A4100000000uL, 0x3E056415269E9ADCuL,
     0x3FDB7B6600000000uL, 0x3E8DDBE05932E271uL,
     0x3FDB8C8900000000uL, 0x3E73FE21DF4FEA38uL,
     0x3FDB9DA800000000uL, 0x3E60B2E6D80D2CE6uL,
     0x3FDBAEC400000000uL, 0x3E874289E4E1D49CuL,
     0x3FDBBFDD00000000uL, 0x3E87CE1B050AA700uL,
     0x3FDBD0F300000000uL, 0x3E65F3C859448338uL,
     0x3FDBE20400000000uL, 0x3E8FFC7F79678A39uL,
     0x3FDBF31400000000uL, 0x3E824A1EC9BE7496uL,
     0x3FDC042100000000uL, 0x3E8C2B16EC00F182uL,
     0x3FDC152A00000000uL, 0x3E6A92654EC891D7uL,
     0x3FDC263000000000uL, 0x3E7037888B90C7F8uL,
     0x3FDC373200000000uL, 0x3E84E5A090419BC8uL,
     0x3FDC483200000000uL, 0x3E882722E066F64DuL,
     0x3FDC592F00000000uL, 0x3E6894AD710AEF0CuL,
     0x3FDC6A2900000000uL, 0x3E74290C06A50919uL,
     0x3FDC7B1F00000000uL, 0x3E8829EA41109E48uL,
     0x3FDC8C1200000000uL, 0x3E8011FB6AD70668uL,
     0x3FDC9D0200000000uL, 0x3E8D1948F3CB0098uL,
     0x3FDCADEF00000000uL, 0x3E835C4DC117DE0DuL,
     0x3FDCBED900000000uL, 0x3E8E37710C7563B4uL,
     0x3FDCCFC000000000uL, 0x3E81B705B8191331uL,
     0x3FDCE0A400000000uL, 0x3E89474B1CFE31F4uL,
     0x3FDCF18500000000uL, 0x3E71C8D86EE32D3BuL,
     0x3FDD026300000000uL, 0x3E7815019917C831uL,
     0x3FDD133D00000000uL, 0x3E86A58C1D40A370uL,
     0x3FDD241400000000uL, 0x3E70C2FC81BC79C2uL,
     0x3FDD34E900000000uL, 0x3E88BA3405ADB567uL,
     0x3FDD45BA00000000uL, 0x3E5DDBA9ECF26BB9uL,
     0x3FDD568800000000uL, 0x3E3D1EF9E850540FuL,
     0x3FDD675300000000uL, 0x3E80065D34CA0DCEuL,
     0x3FDD781C00000000uL, 0x3E80D733E02D0DD1uL,
     0x3FDD88E100000000uL, 0x3E870EF65B098F9CuL,
     0x3FDD99A300000000uL, 0x3E52C86102E26030uL,
     0x3FDDAA6100000000uL, 0x3E8E80C9EF4C81D3uL,
     0x3FDDBB1E00000000uL, 0x3E7692E19CB2B425uL,
     0x3FDDCBD600000000uL, 0x3E8C462E64521547uL,
     0x3FDDDC8C00000000uL, 0x3E8D5A1DD411035EuL,
     0x3FDDED4000000000uL, 0x3E7C908DF47A8F92uL,
     0x3FDDFDF000000000uL, 0x3E545CF17F40AA9DuL,
     0x3FDE0E9D00000000uL, 0x3E687C172AC42C55uL,
     0x3FDE1F4600000000uL, 0x3E78DA98936314CFuL,
     0x3FDE2FED00000000uL, 0x3E4812E4AC4E8487uL,
     0x3FDE409100000000uL, 0x3E64755453322906uL,
     0x3FDE513100000000uL, 0x3E7528AE2E3EF4FAuL,
     0x3FDE61D000000000uL, 0x3E7501716CF4BE90uL,
     0x3FDE726900000000uL, 0x3E8F3CEA8B8B9869uL,
     0x3FDE830200000000uL, 0x3E7BE69828149B31uL,
     0x3FDE939600000000uL, 0x3E8D5E2937A72435uL,
     0x3FDEA42800000000uL, 0x3E89BFBBE2698141uL,
     0x3FDEB4B800000000uL, 0x3E56D15B8C6D35E8uL,
     0x3FDEC54400000000uL, 0x3E886F8D094B9A13uL,
     0x3FDED5CD00000000uL, 0x3E7B23C5DCA4EFF0uL,
     0x3FDEE65300000000uL, 0x3E7D463BF0218027uL,
     0x3FDEF6D600000000uL, 0x3E8B651C6050E055uL,
     0x3FDF075600000000uL, 0x3E6B46A793B8E626uL,
     0x3FDF17D400000000uL, 0x3E74650236B11F5FuL,
     0x3FDF284E00000000uL, 0x3E77629298EFA0ADuL,
     0x3FDF38C500000000uL, 0x3E87D798BEBCB6ABuL,
     0x3FDF493A00000000uL, 0x3E7CCDE6D2F4C9F7uL,
     0x3FDF59AB00000000uL, 0x3E5186572A5FF9C8uL,
     0x3FDF6A1A00000000uL, 0x3E799D006591C907uL,
     0x3FDF7A8500000000uL, 0x3E841960E73EC979uL,
     0x3FDF8AEE00000000uL, 0x3E630AA8521479FDuL,
     0x3FDF9B5300000000uL, 0x3E8E8B869C429D94uL,
     0x3FDFABB700000000uL, 0x3E4350FC25C8A13BuL,
     0x3FDFBC1700000000uL, 0x3E79009A6EF5D48AuL,
     0x3FDFCC7300000000uL, 0x3E8306349A8ABFEFuL,
     0x3FDFDCCE00000000uL, 0x3E7D9F569F06BC1EuL,
     0x3FDFED2500000000uL, 0x3E65160EC1D12919uL,
     0x3FDFFD7900000000uL, 0x3E5A83FF2555A494uL,
     0x3FE006E500000000uL, 0x3E9AFCA83644DE26uL,
     0x3FE00F0D00000000uL, 0x3E53C49D9079D468uL,
     0x3FE0173200000000uL, 0x3E9AE76BE763882EuL,
     0x3FE01F5700000000uL, 0x3E7F793285E25C81uL,
     0x3FE0277A00000000uL, 0x3E800243639826EEuL,
     0x3FE02F9B00000000uL, 0x3E9B301832F2C8A9uL,
     0x3FE037BC00000000uL, 0x3E54B54B5457AB7CuL,
     0x3FE03FDA00000000uL, 0x3E9A32F3449FA7A6uL,
     0x3FE047F700000000uL, 0x3E8E060E91D41DA5uL,
     0x3FE0501300000000uL, 0x3E8A3F382AA1E82BuL,
     0x3FE0582D00000000uL, 0x3E9DA8B4318C1DD2uL,
     0x3FE0604700000000uL, 0x3E3F9274A07C17A6uL,
     0x3FE0685E00000000uL, 0x3E95804EC5F0FE6DuL,
     0x3FE0707400000000uL, 0x3E9C8EAC786D0112uL,
     0x3FE0788900000000uL, 0x3E958943FB66416BuL,
     0x3FE0809D00000000uL, 0x3E33FB82CEDE51E0uL,
     0x3FE088AE00000000uL, 0x3E9CC27B15563034uL,
     0x3FE090BF00000000uL, 0x3E8581667CA3348DuL,
     0x3FE098CE00000000uL, 0x3E8454ACD057FBFAuL,
     0x3FE0A0DC00000000uL, 0x3E91CF1C5C53F37DuL,
     0x3FE0A8E800000000uL, 0x3E93B2B423F481D0uL,
     0x3FE0B0F300000000uL, 0x3E7A8314E3B62474uL,
     0x3FE0B8FD00000000uL, 0x3E574EEBA208D495uL,
     0x3FE0C10400000000uL, 0x3E961AC74D5ADA6AuL,
     0x3FE0C90B00000000uL, 0x3E926DDDE7AA78B1uL,
     0x3FE0D11000000000uL, 0x3E9F51B91D907509uL,
     0x3FE0D91400000000uL, 0x3E9CA5D77A3BF837uL,
     0x3FE0E11700000000uL, 0x3E84935EF97F078EuL,
     0x3FE0E91800000000uL, 0x3E80395F3D5449D6uL,
     0x3FE0F11800000000uL, 0x3E8A2C7CB38D9ED1uL,
     0x3FE0F91600000000uL, 0x3E9677BA0152CBB4uL,
     0x3FE1011300000000uL, 0x3E9B3A7927AEC2FDuL,
     0x3FE1090F00000000uL, 0x3E707F2889E8B7A9uL,
     0x3FE1110900000000uL, 0x3E93BCF3BA17FB1FuL,
     0x3FE1190200000000uL, 0x3E7CECD182C0B1E4uL,
     0x3FE120F900000000uL, 0x3E95A3C2FB2785B2uL,
     0x3FE128EF00000000uL, 0x3E9EDBCE6A636A11uL,
     0x3FE130E400000000uL, 0x3E972C7DA9B832D3uL,
     0x3FE138D700000000uL, 0x3E9E74EFEB672A03uL,
     0x3FE140CA00000000uL, 0x3E2A1E54F6B89E31uL,
     0x3FE148BA00000000uL, 0x3E90AD737019FD24uL,
     0x3FE150A900000000uL, 0x3E9B639C287D2824uL,
     0x3FE1589700000000uL, 0x3E9495B6DD3DDABDuL,
     0x3FE1608400000000uL, 0x3E7F2AEFFE31B5D0uL,
     0x3FE1686F00000000uL, 0x3E827B385C52CC9FuL,
     0x3FE1705900000000uL, 0x3E71E501D3944026uL,
     0x3FE1784100000000uL, 0x3E99628A2C0E2602uL,
     0x3FE1802800000000uL, 0x3E9C2E52F159A4BFuL,
     0x3FE1880E00000000uL, 0x3E8976D9B0F3DFDDuL,
     0x3FE18FF300000000uL, 0x3E628513CD04695CuL,
     0x3FE197D600000000uL, 0x3E75B2DA605BDDF8uL,
     0x3FE19FB700000000uL, 0x3E95EE648263EE18uL,
     0x3FE1A79700000000uL, 0x3E9F6E601AC91256uL,
     0x3FE1AF7700000000uL, 0x3E5D155A178B90CDuL,
     0x3FE1B75400000000uL, 0x3E9CFBE9DE667B41uL,
     0x3FE1BF3100000000uL, 0x3E744AE80F899FBDuL,
     0x3FE1C70C00000000uL, 0x3E76D96FF1C879C9uL,
     0x3FE1CEE500000000uL, 0x3E9ECB5E2C072EB0uL,
     0x3FE1D6BE00000000uL, 0x3E71C11DBE1DB818uL,
     0x3FE1DE9500000000uL, 0x3E625CBB9559D10FuL,
     0x3FE1E66A00000000uL, 0x3E9841C66176BDDEuL,
     0x3FE1EE3F00000000uL, 0x3E78DD143C97C211uL,
     0x3FE1F61200000000uL, 0x3E309F38F10515B8uL,
     0x3FE1FDE300000000uL, 0x3E9DE1D02B7ACB55uL,
     0x3FE205B400000000uL, 0x3E7D6E666F069F9FuL,
     0x3FE20D8300000000uL, 0x3E80C459B58A9A68uL,
     0x3FE2155100000000uL, 0x3E4B3AC6C4422B43uL,
     0x3FE21D1D00000000uL, 0x3E90A6DABDF57C13uL,
     0x3FE224E800000000uL, 0x3E87A6F05E2E66B4uL,
     0x3FE22CB200000000uL, 0x3E83EBCAAAA786FFuL,
     0x3FE2347A00000000uL, 0x3E933C5177AE38BEuL,
     0x3FE23C4100000000uL, 0x3E9F44E5029B8B1DuL,
     0x3FE2440700000000uL, 0x3E9635C0E894DF30uL,
     0x3FE24BCC00000000uL, 0x3E6E87F9F1F3590CuL,
     0x3FE2538F00000000uL, 0x3E7FEACB86A3B429uL,
     0x3FE25B5100000000uL, 0x3E8CFDCF4E10A41AuL,
     0x3FE2631100000000uL, 0x3E9F73A21FDDE641uL,
     0x3FE26AD100000000uL, 0x3E7A8B8011D56D3BuL,
     0x3FE2728F00000000uL, 0x3E6F84BF7D5B34D0uL,
     0x3FE27A4C00000000uL, 0x3E6985CC1C8F11B0uL,
     0x3FE2820700000000uL, 0x3E88D25A6A02C803uL,
     0x3FE289C100000000uL, 0x3E975FD4C3433E76uL,
     0x3FE2917A00000000uL, 0x3E8825154781D2C2uL,
     0x3FE2993200000000uL, 0x3E62791595E60D25uL,
     0x3FE2A0E800000000uL, 0x3E605B4C41D5635BuL,
     0x3FE2A89D00000000uL, 0x3E68E92900528496uL,
     0x3FE2B05000000000uL, 0x3E9970145DF6A281uL,
     0xBFDA8FF900000000uL, 0xBE86302155DF0DE3uL,
     0xBFDA809600000000uL, 0xBE8D2B316176FAD0uL,
     0xBFDA713700000000uL, 0xBE824DB2F6ACEB96uL,
     0xBFDA61DA00000000uL, 0xBE67117A804DA234uL,
     0xBFDA527F00000000uL, 0xBE7F97F60FF5807BuL,
     0xBFDA432700000000uL, 0xBE809D5C44ADAA28uL,
     0xBFDA33D200000000uL, 0xBE70E2C7DE9AC83BuL,
     0xBFDA247F00000000uL, 0xBE8781011952FB40uL,
     0xBFDA152F00000000uL, 0xBE6794C0EDAF9F16uL,
     0xBFDA05E100000000uL, 0xBE77DDF6E9895B08uL,
     0xBFD9F69600000000uL, 0xBE73AEF455AE3DA8uL,
     0xBFD9E74D00000000uL, 0xBE6EAF442C7BA9BEuL,
     0xBFD9D80600000000uL, 0xBE8DC93243F14070uL,
     0xBFD9C8C300000000uL, 0xBE78D1BA7956F02DuL,
     0xBFD9B98100000000uL, 0xBE8B8C1E78260310uL,
     0xBFD9AA4300000000uL, 0xBE5CE27FC9D31391uL,
     0xBFD99B0700000000uL, 0xBE634B6355F4087AuL,
     0xBFD98BCD00000000uL, 0xBE6C94B4572FEF43uL,
     0xBFD97C9600000000uL, 0xBE5846721DE94267uL,
     0xBFD96D6100000000uL, 0xBE88B74ACDDE1F6AuL,
     0xBFD95E2F00000000uL, 0xBE801A3E03F6B280uL,
     0xBFD94F0000000000uL, 0xBE4B35095482043FuL,
     0xBFD93FD200000000uL, 0xBE856437D9BB4A5CuL,
     0xBFD930A800000000uL, 0xBE5DB5B388B06A65uL,
     0xBFD9218000000000uL, 0xBE79C93768C0E5D4uL,
     0xBFD9125A00000000uL, 0xBE27F0E9D0AAF77AuL,
     0xBFD9033700000000uL, 0xBE6E085F7C5942F1uL,
     0xBFD8F41600000000uL, 0xBE81B98DF5F47569uL,
     0xBFD8E4F700000000uL, 0xBE8F3428AC4DDEECuL,
     0xBFD8D5DC00000000uL, 0xBE7127EF6092650EuL,
     0xBFD8C6C300000000uL, 0xBE7C262E6C66CDB8uL,
     0xBFD8B7AC00000000uL, 0xBE876FAFFFF4AF15uL,
     0xBFD8A89800000000uL, 0xBE635FDEAD9EF9A2uL,
     0xBFD8998600000000uL, 0xBE7DFC6109E45CEBuL,
     0xBFD88A7600000000uL, 0xBE8D94A9416E4721uL,
     0xBFD87B6900000000uL, 0xBE80C9BD35322FA9uL,
     0xBFD86C5F00000000uL, 0xBE45BD4714C8FFCFuL,
     0xBFD85D5700000000uL, 0xBE7F0AC6ABBA5180uL,
     0xBFD84E5100000000uL, 0xBE74A1D4FC76C4E2uL,
     0xBFD83F4E00000000uL, 0xBE58C7BBD43EA059uL,
     0xBFD8304D00000000uL, 0xBE8A18240481523AuL,
     0xBFD8214E00000000uL, 0xBE8E4115E0E87309uL,
     0xBFD8125300000000uL, 0xBE4067FCC9C54454uL,
     0xBFD8035A00000000uL, 0xBE5519044060B3CAuL,
     0xBFD7F46200000000uL, 0xBE81F1C2BAB3EFA5uL,
     0xBFD7E56E00000000uL, 0xBE2F4F8116A92F1FuL,
     0xBFD7D67C00000000uL, 0xBE7D00EBAF755412uL,
     0xBFD7C78C00000000uL, 0xBE757CB332AA9B04uL,
     0xBFD7B89F00000000uL, 0xBE6B67957924A221uL,
     0xBFD7A9B400000000uL, 0xBE749441F289397FuL,
     0xBFD79ACB00000000uL, 0xBE853E207739B243uL,
     0xBFD78BE500000000uL, 0xBE6F940FB688810DuL,
     0xBFD77D0100000000uL, 0xBE8B3DF7AD1F744BuL,
     0xBFD76E2000000000uL, 0xBE86B033AD082BC9uL,
     0xBFD75F4100000000uL, 0xBE8A6AFC121884DAuL,
     0xBFD7506500000000uL, 0xBE6A7683B47C1884uL,
     0xBFD7418A00000000uL, 0xBE8B777E34575FD6uL,
     0xBFD732B200000000uL, 0xBE8927FBBCB9EE5DuL,
     0xBFD723DD00000000uL, 0xBE88C68D7090566BuL,
     0xBFD7150B00000000uL, 0xBE4A2B2A2A0EB191uL,
     0xBFD7063900000000uL, 0xBE8AFBF68DE6383BuL,
     0xBFD6F76B00000000uL, 0xBE86DDF093045EA8uL,
     0xBFD6E89F00000000uL, 0xBE8C8C435CC0756EuL,
     0xBFD6D9D600000000uL, 0xBE786D3AE8F9661FuL,
     0xBFD6CB0F00000000uL, 0xBE6832E43F6D9D88uL,
     0xBFD6BC4A00000000uL, 0xBE747CB81361877FuL,
     0xBFD6AD8800000000uL, 0xBE82035808F1C0F3uL,
     0xBFD69EC800000000uL, 0xBE76FF1399DB6922uL,
     0xBFD6900A00000000uL, 0xBE7FCDB431863DD3uL,
     0xBFD6814E00000000uL, 0xBE8F693D13FBB8D9uL,
     0xBFD6729600000000uL, 0xBE834EB29036FAD3uL,
     0xBFD663DF00000000uL, 0xBE899B456A12CE2EuL,
     0xBFD6552B00000000uL, 0xBE772618A503C189uL,
     0xBFD6467900000000uL, 0xBE72CC529275C5A3uL,
     0xBFD637C900000000uL, 0xBE8344C9B19A2513uL,
     0xBFD6291C00000000uL, 0xBE72BE4C963D47B8uL,
     0xBFD61A7100000000uL, 0xBE77CB0653B68DE6uL,
     0xBFD60BC800000000uL, 0xBE8B082FAEDC50D1uL,
     0xBFD5FD2200000000uL, 0xBE86F7868080F7BCuL,
     0xBFD5EE7E00000000uL, 0xBE6A9FB569E79A60uL,
     0xBFD5DFDC00000000uL, 0xBE8CBDD5BF453A04uL,
     0xBFD5D13D00000000uL, 0xBE6BB6EE545183DCuL,
     0xBFD5C2A000000000uL, 0xBE87EC26C29AA221uL,
     0xBFD5B40500000000uL, 0xBE8D5DA983E3CBEDuL,
     0xBFD5A56D00000000uL, 0xBE80B6E1BFE5EC04uL,
     0xBFD596D700000000uL, 0xBE8228784608B2DFuL,
     0xBFD5884300000000uL, 0xBE7116419622027EuL,
     0xBFD579B200000000uL, 0xBE6AEE6A38F29592uL,
     0xBFD56B2200000000uL, 0xBE8A36AF180D0F15uL,
     0xBFD55C9500000000uL, 0xBE8C853372CA57CCuL,
     0xBFD54E0B00000000uL, 0xBE7BB00EE04486C4uL,
     0xBFD53F8300000000uL, 0xBE7CC02B891628DAuL,
     0xBFD530FD00000000uL, 0xBE63794FE93C7F63uL,
     0xBFD5227900000000uL, 0xBE75D7854E0DE2C5uL,
     0xBFD513F800000000uL, 0xBE372DA45519DCE0uL,
     0xBFD5057800000000uL, 0xBE79F8D2DA727BF4uL,
     0xBFD4F6FC00000000uL, 0xBE56CEC60358C3FDuL,
     0xBFD4E88000000000uL, 0xBE8602E65C350140uL,
     0xBFD4DA0800000000uL, 0xBE8328C92737A9B0uL,
     0xBFD4CB9200000000uL, 0xBE6DC3078767B5B5uL,
     0xBFD4BD1E00000000uL, 0xBE79203927CD12CCuL,
     0xBFD4AEAD00000000uL, 0xBE55C17DA1B07B42uL,
     0xBFD4A03D00000000uL, 0xBE80825C25CBDDA8uL,
     0xBFD491D000000000uL, 0xBE7F601BA1CB823BuL,
     0xBFD4836600000000uL, 0xBE2CAEBE06773E1BuL,
     0xBFD474FD00000000uL, 0xBE72AFC887224809uL,
     0xBFD4669700000000uL, 0xBE60B454DABABFEEuL,
     0xBFD4583200000000uL, 0xBE8777E382EF584FuL,
     0xBFD449D000000000uL, 0xBE8D0DEFA65E43F7uL,
     0xBFD43B7100000000uL, 0xBE8520E465F01125uL,
     0xBFD42D1400000000uL, 0xBE68A9DB3066F3ADuL,
     0xBFD41EB900000000uL, 0xBE7418CD285C77E6uL,
     0xBFD4106000000000uL, 0xBE6CE1F66985CEA7uL,
     0xBFD4020900000000uL, 0xBE8798904973EF89uL,
     0xBFD3F3B500000000uL, 0xBE4967D2AB8251D8uL,
     0xBFD3E56200000000uL, 0xBE8F242D496E3D08uL,
     0xBFD3D71200000000uL, 0xBE86A393BBA964C4uL,
     0xBFD3C8C500000000uL, 0xBE507570CACEF7BFuL,
     0xBFD3BA7900000000uL, 0xBE6EFE0FA4F69A96uL,
     0xBFD3AC3000000000uL, 0xBE4B827373E0A286uL,
     0xBFD39DE800000000uL, 0xBE864AB3E2FB43D9uL,
     0xBFD38FA300000000uL, 0xBE8F81504EB31318uL,
     0xBFD3816100000000uL, 0xBE5D3164FB917590uL,
     0xBFD3732000000000uL, 0xBE8CCB836B329F7FuL,
     0xBFD364E200000000uL, 0xBE8133990D5010C8uL,
     0xBFD356A600000000uL, 0xBE404BC113420455uL,
     0xBFD3486C00000000uL, 0xBE697514CF0A57DCuL,
     0xBFD33A3400000000uL, 0xBE6DCE5B769A0EB8uL,
     0xBFD32BFE00000000uL, 0xBE8E6E1DD018CC95uL,
     0xBFD31DCB00000000uL, 0xBE817B505F20E7F3uL,
     0xBFD30F9A00000000uL, 0xBE3835DF86199AB1uL,
     0xBFD3016B00000000uL, 0xBE69CF10D769BDDBuL,
     0xBFD2F33E00000000uL, 0xBE7168482A60BB7CuL,
     0xBFD2E51400000000uL, 0xBE4BD6CDF5BCF5C4uL,
     0xBFD2D6EA00000000uL, 0xBE8D924633FFF084uL,
     0xBFD2C8C500000000uL, 0xBE7542C49A05EE8FuL,
     0xBFD2BAA000000000uL, 0xBE8AC97C411279DBuL,
     0xBFD2AC7F00000000uL, 0xBE536ACCE9910BF7uL,
     0xBFD29E5F00000000uL, 0xBE6E5F25492F16F4uL,
     0xBFD2904100000000uL, 0xBE74DF4847FE96F4uL,
     0xBFD2822500000000uL, 0xBE763798F43090EBuL,
     0xBFD2740C00000000uL, 0xBE5FB975AD3295A5uL,
     0xBFD265F400000000uL, 0xBE8AFCC065467993uL,
     0xBFD257E000000000uL, 0xBE751F024A4452FEuL,
     0xBFD249CC00000000uL, 0xBE8E6279A0249A31uL,
     0xBFD23BBC00000000uL, 0xBE7631798BCDA203uL,
     0xBFD22DAD00000000uL, 0xBE869D668FF512CDuL,
     0xBFD21FA100000000uL, 0xBE4179CAE9BEEE0DuL,
     0xBFD2119700000000uL, 0xBE63FA3A108EC52DuL,
     0xBFD2038E00000000uL, 0xBE7BBAE8D6FB8A1CuL,
     0xBFD1F58800000000uL, 0xBE807F90E4C2EC69uL,
     0xBFD1E78400000000uL, 0xBE82BC2F5BABE119uL,
     0xBFD1D98200000000uL, 0xBE84BAA4D8E71F1CuL,
     0xBFD1CB8200000000uL, 0xBE86A24FC7020B2BuL,
     0xBFD1BD8500000000uL, 0xBE8302982DFE3735uL,
     0xBFD1AF8900000000uL, 0xBE8536EECE3209FAuL,
     0xBFD1A19000000000uL, 0xBE823ACE8FC13621uL,
     0xBFD1939900000000uL, 0xBE7F9B761181CC82uL,
     0xBFD185A400000000uL, 0xBE7C2E82ADD30FBFuL,
     0xBFD177B100000000uL, 0xBE7A7DEFB44845FCuL,
     0xBFD169C000000000uL, 0xBE7AD8FC5EFE4B5FuL,
     0xBFD15BD100000000uL, 0xBE7D8EFA5836733AuL,
     0xBFD14DE400000000uL, 0xBE8177A6D8101FB8uL,
     0xBFD13FFA00000000uL, 0xBE8030B69AB39BD3uL,
     0xBFD1321100000000uL, 0xBE86067085D42483uL,
     0xBFD1242A00000000uL, 0xBE8DA8A239A3D693uL,
     0xBFD1164700000000uL, 0xBE4D72294066A603uL,
     0xBFD1086400000000uL, 0xBE7B1BA1DC449B96uL,
     0xBFD0FA8400000000uL, 0xBE862896725DE3DDuL,
     0xBFD0ECA600000000uL, 0xBE6A4D928A11E457uL,
     0xBFD0DECA00000000uL, 0xBE843A36B9D55575uL,
     0xBFD0D0F000000000uL, 0xBE73F2208D19FE75uL,
     0xBFD0C31800000000uL, 0xBE8D4BFE81A344C0uL,
     0xBFD0B54200000000uL, 0xBE88FF16F1F6621DuL,
     0xBFD0A76F00000000uL, 0xBE829E78B22B06AAuL,
     0xBFD0999D00000000uL, 0xBE84E64B365FEC9AuL,
     0xBFD08BCD00000000uL, 0xBE8AB2BF39987EFFuL,
     0xBFD07E0000000000uL, 0xBE8EF00E6F310240uL,
     0xBFD0703500000000uL, 0xBE7884F5DD34E44BuL,
     0xBFD0626B00000000uL, 0xBE8D92500F14B471uL,
     0xBFD054A400000000uL, 0xBE8307E1DD3AD028uL,
     0xBFD046DF00000000uL, 0xBE79971A63342C6AuL,
     0xBFD0391C00000000uL, 0xBE760B6F55E8DB61uL,
     0xBFD02B5A00000000uL, 0xBE8302CF89E64237uL,
     0xBFD01D9B00000000uL, 0xBE8A9F4C3EFC935AuL,
     0xBFD00FDE00000000uL, 0xBE788F5A8DC51CDFuL,
     0xBFD0022300000000uL, 0xBE8DE87B8DE45C1CuL,
     0xBFCFE8D500000000uL, 0xBE73BC8FEAB63684uL,
     0xBFCFCD6700000000uL, 0xBE766B590D531889uL,
     0xBFCFB1FE00000000uL, 0xBE50BA5E451BFF1AuL,
     0xBFCF969700000000uL, 0xBE5D9E85A4FC1CE1uL,
     0xBFCF7B3600000000uL, 0xBE687FBDAB298DB0uL,
     0xBFCF5FD800000000uL, 0xBE5C831EAF201561uL,
     0xBFCF447E00000000uL, 0xBE6C97CC28A0C985uL,
     0xBFCF292900000000uL, 0xBE4096A784F160C8uL,
     0xBFCF0DD800000000uL, 0xBE463A00E430058BuL,
     0xBFCEF28900000000uL, 0xBE7A9AE40ADF8036uL,
     0xBFCED74100000000uL, 0xBE76178F7389C2B3uL,
     0xBFCEBBFC00000000uL, 0xBE628E408A6030DBuL,
     0xBFCEA0BB00000000uL, 0xBE65370CFCA139E2uL,
     0xBFCE857D00000000uL, 0xBE509B099C44098AuL,
     0xBFCE6A4300000000uL, 0xBE68D5CAF2FAEF74uL,
     0xBFCE4F0E00000000uL, 0xBE4DD08F036B132FuL,
     0xBFCE33DD00000000uL, 0xBE64CCF4CB32E460uL,
     0xBFCE18AF00000000uL, 0xBE64C4C42C4E4661uL,
     0xBFCDFD8700000000uL, 0xBE70B81DE05729DEuL,
     0xBFCDE26000000000uL, 0xBE7A821176A0FE0EuL,
     0xBFCDC74000000000uL, 0xBE669566643C24C3uL,
     0xBFCDAC2200000000uL, 0xBE767C88339625FCuL,
     0xBFCD910900000000uL, 0xBE72DA2735AA6C86uL,
     0xBFCD75F300000000uL, 0xBE644C6D4A5F5AD6uL,
     0xBFCD5AE300000000uL, 0xBE6396DD21FE2514uL,
     0xBFCD3FD400000000uL, 0xBE6CA92AE56A4FCFuL,
     0xBFCD24CB00000000uL, 0xBE7BDC846E0ED386uL,
     0xBFCD09C600000000uL, 0xBE55B88BE3AE865AuL,
     0xBFCCEEC500000000uL, 0xBE3FC6A072116830uL,
     0xBFCCD3C600000000uL, 0xBE7B1A6214562C52uL,
     0xBFCCB8CD00000000uL, 0xBE5F2C91C96636D8uL,
     0xBFCC9DD800000000uL, 0xBE60C3B48651CF97uL,
     0xBFCC82E600000000uL, 0xBE5966F235766DDBuL,
     0xBFCC67F800000000uL, 0xBE78CE14EAE5DCA8uL,
     0xBFCC4D0E00000000uL, 0xBE625479353B5C4AuL,
     0xBFCC322800000000uL, 0xBE6D333A7B285AC2uL,
     0xBFCC174500000000uL, 0xBE7277AFFE5D329AuL,
     0xBFCBFC6700000000uL, 0xBE67FFFD12834EFCuL,
     0xBFCBE18D00000000uL, 0xBE7B862223583BCFuL,
     0xBFCBC6B700000000uL, 0xBE649B874647B1F2uL,
     0xBFCBABE300000000uL, 0xBE78929BF1C864A7uL,
     0xBFCB911600000000uL, 0xBE74D074968F73D7uL,
     0xBFCB764A00000000uL, 0xBE79FB251B935310uL,
     0xBFCB5B8300000000uL, 0xBE769696568E41B9uL,
     0xBFCB40C100000000uL, 0xBE65ED80B7EB91E0uL,
     0xBFCB260200000000uL, 0xBE07D52C3932A2E4uL,
     0xBFCB0B4700000000uL, 0xBE6B8AD7D7A99FE6uL,
     0xBFCAF08F00000000uL, 0xBE7CBC2B9155B770uL,
     0xBFCAD5DB00000000uL, 0xBE6AA03F2514A52BuL,
     0xBFCABB2D00000000uL, 0xBE6CFB1D524B6DAFuL,
     0xBFCAA08000000000uL, 0xBE7A78CD1FBB1E99uL,
     0xBFCA85D900000000uL, 0xBE119017E37D4667uL,
     0xBFCA6B3400000000uL, 0xBE6184B897951F46uL,
     0xBFCA509400000000uL, 0xBE675349E1651FC0uL,
     0xBFCA35F700000000uL, 0xBE71C8ACC30679DDuL,
     0xBFCA1B5F00000000uL, 0xBE72EC1682BF9837uL,
     0xBFCA00CA00000000uL, 0xBE77D09336233C90uL,
     0xBFC9E63A00000000uL, 0xBE7852E40017E39CuL,
     0xBFC9CBAD00000000uL, 0xBE7D1FD8802FB817uL,
     0xBFC9B12400000000uL, 0xBE59D13FAE79743CuL,
     0xBFC9969D00000000uL, 0xBE748D385E0277CFuL,
     0xBFC97C1B00000000uL, 0xBE7F678FA8388A68uL,
     0xBFC9619F00000000uL, 0xBE5D6188E89480ECuL,
     0xBFC9472500000000uL, 0xBE74E4CB139C1E95uL,
     0xBFC92CAF00000000uL, 0xBE6093E9A4239741uL,
     0xBFC9123C00000000uL, 0xBE3C518D850F7BA8uL,
     0xBFC8F7CD00000000uL, 0xBE797B7FC86F1C0CuL,
     0xBFC8DD6200000000uL, 0xBE77D280A0117CFDuL,
     0xBFC8C2FA00000000uL, 0xBE7D078174C6928FuL,
     0xBFC8A89800000000uL, 0xBE357F7A64CCD537uL,
     0xBFC88E3800000000uL, 0xBE6A22CD1F2E8F29uL,
     0xBFC873DC00000000uL, 0xBE1C582D297FF644uL,
     0xBFC8598400000000uL, 0xBE73CD87CE24F758uL,
     0xBFC83F3000000000uL, 0xBE6EB716BAC42623uL,
     0xBFC824DF00000000uL, 0xBE73592A0F410400uL,
     0xBFC80A9300000000uL, 0xBE78343174876BA5uL,
     0xBFC7F04900000000uL, 0xBE6BA4F9B930430EuL,
     0xBFC7D60400000000uL, 0xBE5367DD3B0B6B0BuL,
     0xBFC7BBC200000000uL, 0xBE556265A1DC7A8EuL,
     0xBFC7A18500000000uL, 0xBE5F71ACA38241C4uL,
     0xBFC7874B00000000uL, 0xBE746381F987646BuL,
     0xBFC76D1500000000uL, 0xBE665804BC056069uL,
     0xBFC752E200000000uL, 0xBE68E83E5955BBC6uL,
     0xBFC738B200000000uL, 0xBE787A19887D1E81uL,
     0xBFC71E8800000000uL, 0xBE5FD1054D6E1895uL,
     0xBFC7045F00000000uL, 0xBE6471E7650BE845uL,
     0xBFC6EA3B00000000uL, 0xBE707E9D9296377FuL,
     0xBFC6D01C00000000uL, 0xBE7B1BB94E9CC3B2uL,
     0xBFC6B5FF00000000uL, 0xBE7936CECA9AFDC8uL,
     0xBFC69BE600000000uL, 0xBE4CB3A881ABFDF7uL,
     0xBFC681D100000000uL, 0xBE732151A8286C6FuL,
     0xBFC667C000000000uL, 0xBE6EFC2E3E9CED23uL,
     0xBFC64DB200000000uL, 0xBE78EB86AC9EF252uL,
     0xBFC633A800000000uL, 0xBE6F50DF1ABE0FC9uL,
     0xBFC619A100000000uL, 0xBE73F3AEFE930C8FuL,
     0xBFC5FF9F00000000uL, 0xBE7EDC30C01B141DuL,
     0xBFC5E59F00000000uL, 0xBE7F08ED31FE1628uL,
     0xBFC5CBA500000000uL, 0xBE5983B170E6C68FuL,
     0xBFC5B1AD00000000uL, 0xBE7C5342DDBB7371uL,
     0xBFC597BA00000000uL, 0xBE31F13B9ECB2DA6uL,
     0xBFC57DC900000000uL, 0xBE75038FC82FBC24uL,
     0xBFC563DC00000000uL, 0xBE783FF5AD081783uL,
     0xBFC549F300000000uL, 0xBE662723A6715875uL,
     0xBFC5300D00000000uL, 0xBE6B7B7CC9AF768AuL,
     0xBFC5162B00000000uL, 0xBE1F78D1162B410DuL,
     0xBFC4FC4D00000000uL, 0xBE7CB37679326801uL,
     0xBFC4E27200000000uL, 0xBE7065FA9470590BuL,
     0xBFC4C89C00000000uL, 0xBE6C3A0233EDA037uL,
     0xBFC4AEC800000000uL, 0xBE4E014055897901uL,
     0xBFC494F900000000uL, 0xBE4FB8E003C2F3B1uL,
     0xBFC47B2B00000000uL, 0xBE7C8996199D6EEAuL,
     0xBFC4616400000000uL, 0xBE0FAF0BC81E4B94uL,
     0xBFC4479D00000000uL, 0xBE7CC047F1F25C83uL,
     0xBFC42DDD00000000uL, 0xBE53D0DA516B147FuL,
     0xBFC4141F00000000uL, 0xBE7FCB190ACB1C29uL,
     0xBFC3FA6400000000uL, 0xBE7414EC0C60BAD1uL,
     0xBFC3E0AE00000000uL, 0xBE74E9BA984A9A60uL,
     0xBFC3C6FC00000000uL, 0xBE624337CCC1362DuL,
     0xBFC3AD4B00000000uL, 0xBE7774B4CC0EC2A8uL,
     0xBFC393A000000000uL, 0xBE732B380B7EFC7CuL,
     0xBFC379F700000000uL, 0xBE62DAC931C2E190uL,
     0xBFC3605300000000uL, 0xBE6B470FA43DC529uL,
     0xBFC346B100000000uL, 0xBE69ABF6162BFC32uL,
     0xBFC32D1300000000uL, 0xBE2BA4B334A02879uL,
     0xBFC3137A00000000uL, 0xBE4D8BE297E30D03uL,
     0xBFC2F9E300000000uL, 0xBE415BFDA1644C22uL,
     0xBFC2E04F00000000uL, 0xBE763BBE948B1AC0uL,
     0xBFC2C6C000000000uL, 0xBE016A3F42B0E0F2uL,
     0xBFC2AD3400000000uL, 0xBE00B500D8B4466EuL,
     0xBFC293AB00000000uL, 0xBE767834AAD3C38FuL,
     0xBFC27A2700000000uL, 0xBE4B3FB7DED60421uL,
     0xBFC260A600000000uL, 0xBE5CC6018F3BCD49uL,
     0xBFC2472700000000uL, 0xBE603B59BC184860uL,
     0xBFC22DAD00000000uL, 0xBE7A556695FCA0D7uL,
     0xBFC2143600000000uL, 0xBE64434576D52CB7uL,
     0xBFC1FAC400000000uL, 0xBE6796CA377EA74EuL,
     0xBFC1E15400000000uL, 0xBE66F7798C85559DuL,
     0xBFC1C7E800000000uL, 0xBE4BDE34965F6984uL,
     0xBFC1AE7D00000000uL, 0xBE79E4AB7003A0E6uL,
     0xBFC1951900000000uL, 0xBE49FD11E39ABAACuL,
     0xBFC17BB800000000uL, 0xBE56B7B48B95C15BuL,
     0xBFC1625900000000uL, 0xBE5CC36D3E3CCA65uL,
     0xBFC148FE00000000uL, 0xBE41CE485761F69CuL,
     0xBFC12FA600000000uL, 0xBE770A1F05316811uL,
     0xBFC1165300000000uL, 0xBE578D49DC1AFE94uL,
     0xBFC0FD0300000000uL, 0xBE6E0DCA31CD9E54uL,
     0xBFC0E3B500000000uL, 0xBE784E650E0A2FD5uL,
     0xBFC0CA6B00000000uL, 0xBE7C536D57D9DAB9uL,
     0xBFC0B12500000000uL, 0xBE7B57A5578D01FDuL,
     0xBFC097E300000000uL, 0xBE759CC0CF3DA52AuL,
     0xBFC07EA300000000uL, 0xBE70DC7F7C36AAB7uL,
     0xBFC0656900000000uL, 0xBE43057726EEA6F9uL,
     0xBFC04C3000000000uL, 0xBE75532713B0B555uL,
     0xBFC032FC00000000uL, 0xBE51F736F8234297uL,
     0xBFC019C900000000uL, 0xBE757A9427127E28uL,
     0xBFC0009C00000000uL, 0xBE7DD37909D634E1uL,
     0xBFBFCEE400000000uL, 0xBE60E50B92227F37uL,
     0xBFBF9C9700000000uL, 0xBE10744B2BBD5C34uL,
     0xBFBF6A4D00000000uL, 0xBE6576FB1AB66AD7uL,
     0xBFBF380F00000000uL, 0xBE6B5374D31A91EEuL,
     0xBFBF05D600000000uL, 0xBE4DB610EEE1B81BuL,
     0xBFBED3A000000000uL, 0xBE6A19B7978E8BB8uL,
     0xBFBEA17600000000uL, 0xBE6F4CB6BF56F18EuL,
     0xBFBE6F5100000000uL, 0xBE57F67E0BD3B63FuL,
     0xBFBE3D3300000000uL, 0xBE666A27D6A83D6CuL,
     0xBFBE0B1A00000000uL, 0xBE523CBF0C85FA27uL,
     0xBFBDD90800000000uL, 0xBE6A7CED811F7DA6uL,
     0xBFBDA6FF00000000uL, 0xBE5615E1BD550182uL,
     0xBFBD74FD00000000uL, 0xBE6B4DA043725D03uL,
     0xBFBD430000000000uL, 0xBE658A49AA2DCA64uL,
     0xBFBD110B00000000uL, 0xBE6066543AD84EF1uL,
     0xBFBCDF1A00000000uL, 0xBE66073D700E9F19uL,
     0xBFBCAD3500000000uL, 0xBE63A29CD758D759uL,
     0xBFBC7B5100000000uL, 0xBE49B8777D6BBC9DuL,
     0xBFBC497800000000uL, 0xBE623F87F4487FE4uL,
     0xBFBC17A400000000uL, 0xBE55196CB4C66620uL,
     0xBFBBE5D800000000uL, 0xBE496E785A0317A3uL,
     0xBFBBB41000000000uL, 0xBE5EE49501957B40uL,
     0xBFBB825000000000uL, 0xBE6CF6DF4849748BuL,
     0xBFBB509500000000uL, 0xBE688F964BD70C8FuL,
     0xBFBB1EE600000000uL, 0xBE6072C317519BB4uL,
     0xBFBAED3800000000uL, 0xBE05B3290A662BD0uL,
     0xBFBABB9500000000uL, 0xBE5B246AD0582C09uL,
     0xBFBA89F700000000uL, 0xBE55372721811F66uL,
     0xBFBA585D00000000uL, 0xBE67C995FE88BCE3uL,
     0xBFBA26CC00000000uL, 0xBE596605E161E768uL,
     0xBFB9F54300000000uL, 0xBE53BD6EA8CDCABFuL,
     0xBFB9C3BE00000000uL, 0xBE6873A6488F239EuL,
     0xBFB9924200000000uL, 0xBE6038DB2539E54EuL,
     0xBFB960CA00000000uL, 0xBE6A3576F0EB47EAuL,
     0xBFB92F5B00000000uL, 0xBE5CA16578E782D8uL,
     0xBFB8FDF000000000uL, 0xBE6571DD058C9404uL,
     0xBFB8CC8E00000000uL, 0xBE4E8172926B3912uL,
     0xBFB89B3400000000uL, 0xBE458EB8A49A1ED9uL,
     0xBFB869DE00000000uL, 0xBE67736434037B3EuL,
     0xBFB8388D00000000uL, 0xBE6E2728B7069E85uL,
     0xBFB8074500000000uL, 0xBE61C6BCD5B504DEuL,
     0xBFB7D60500000000uL, 0xBE62D9F791FD12F7uL,
     0xBFB7A4CA00000000uL, 0xBE53B18B476F88BFuL,
     0xBFB7739300000000uL, 0xBE671B2AD71BBA2EuL,
     0xBFB7426500000000uL, 0xBE6329422BBD68E8uL,
     0xBFB7113F00000000uL, 0xBE6E8B3C2FE4ECAEuL,
     0xBFB6E01F00000000uL, 0xBE2795EDD5ED58E9uL,
     0xBFB6AF0200000000uL, 0xBE6C4C07447A13FAuL,
     0xBFB67DEF00000000uL, 0xBE4F2EA58340E81EuL,
     0xBFB64CE400000000uL, 0xBE4203398A8FFDA4uL,
     0xBFB61BDA00000000uL, 0xBE2D4147AD124EAAuL,
     0xBFB5EADC00000000uL, 0xBE539C66835B9867uL,
     0xBFB5B9DF00000000uL, 0xBE6317F3D15A9860uL,
     0xBFB588EF00000000uL, 0xBE503474104B244EuL,
     0xBFB557FF00000000uL, 0xBE6F1DFAE0BD2E94uL,
     0xBFB5271900000000uL, 0xBE541889EF09D7C8uL,
     0xBFB4F63B00000000uL, 0xBE52DC76D475D4D1uL,
     0xBFB4C56200000000uL, 0xBE433458770A1735uL,
     0xBFB4948D00000000uL, 0xBE6C8223B5C8B49BuL,
     0xBFB463C200000000uL, 0xBE540D91E2302042uL,
     0xBFB432FB00000000uL, 0xBE64B47F064D986FuL,
     0xBFB4023900000000uL, 0xBE6CE4D526C81E43uL,
     0xBFB3D18000000000uL, 0xBE6C41714A091D46uL,
     0xBFB3A0D000000000uL, 0xBE63540DB8C80703uL,
     0xBFB3702100000000uL, 0xBE5F8CF1A845A25CuL,
     0xBFB33F7B00000000uL, 0xBE430A65C7A2686FuL,
     0xBFB30EDD00000000uL, 0xBE62D26A7215665CuL,
     0xBFB2DE4500000000uL, 0xBE1BFF57E3BAB991uL,
     0xBFB2ADB100000000uL, 0xBE5E8ADFC156E82DuL,
     0xBFB27D2200000000uL, 0xBE6E5D041C5F1A05uL,
     0xBFB24C9D00000000uL, 0xBE50A21095DF344CuL,
     0xBFB21C2000000000uL, 0xBE5B57C218054E22uL,
     0xBFB1EBA400000000uL, 0xBE6B1806F4988888uL,
     0xBFB1BB3200000000uL, 0xBE430029DC60A716uL,
     0xBFB18AC400000000uL, 0xBE611E8ED29C4BEAuL,
     0xBFB15A5F00000000uL, 0xBE6AAE4E1E1CD7E9uL,
     0xBFB12A0000000000uL, 0xBE4F2855166A96D5uL,
     0xBFB0F9A500000000uL, 0xBE68CCC743692647uL,
     0xBFB0C95400000000uL, 0xBE50C2B8FF93EEA0uL,
     0xBFB0990400000000uL, 0xBE329700306849F4uL,
     0xBFB068C000000000uL, 0xBE661C7597DFA0CFuL,
     0xBFB0387E00000000uL, 0xBE64F950C199FDD6uL,
     0xBFB0084500000000uL, 0xBE6434BDA55A11E5uL,
     0xBFAFB02300000000uL, 0xBE537435DBA745C1uL,
     0xBFAF4FC600000000uL, 0xBE4793720209C664uL,
     0xBFAEEF7B00000000uL, 0xBE3E845C9D0173B4uL,
     0xBFAE8F3A00000000uL, 0xBE527188BD53B8BFuL,
     0xBFAE2F0400000000uL, 0xBE49E4E1F2D00CB9uL,
     0xBFADCED800000000uL, 0xBE57DB5B6132809AuL,
     0xBFAD6EBF00000000uL, 0xBE43C7FBABDF571FuL,
     0xBFAD0EB000000000uL, 0xBE4C086873F1531FuL,
     0xBFACAEAC00000000uL, 0xBE33D01264312288uL,
     0xBFAC4EB200000000uL, 0xBE4ED73A1B11C287uL,
     0xBFABEECB00000000uL, 0xBE328D5761EA48D2uL,
     0xBFAB8EEE00000000uL, 0xBE4E2759579AC08AuL,
     0xBFAB2F1C00000000uL, 0xBE4EEA927B8DE26EuL,
     0xBFAACF5500000000uL, 0xBE3A03EC4341A4ACuL,
     0xBFAA6F9800000000uL, 0xBE54EFB9656181BFuL,
     0xBFAA0FEE00000000uL, 0xBE529AA680456564uL,
     0xBFA9B04F00000000uL, 0xBE42B60FBBF05015uL,
     0xBFA950BA00000000uL, 0xBE59EA4D388956ACuL,
     0xBFA8F13800000000uL, 0xBE5C820F8DDADCD6uL,
     0xBFA891BA00000000uL, 0xBE27E05A334C58F7uL,
     0xBFA8324D00000000uL, 0xBE5D3229B2BA0376uL,
     0xBFA7D2EC00000000uL, 0xBE545E77C08ED94CuL,
     0xBFA7739600000000uL, 0xBE427656B6F95551uL,
     0xBFA7144A00000000uL, 0xBE5C82A193D30405uL,
     0xBFA6B50A00000000uL, 0xBE4DDEBD1F3C284AuL,
     0xBFA655DC00000000uL, 0xBE599C108199CFD8uL,
     0xBFA5F6BA00000000uL, 0xBE348E1F3828F0D8uL,
     0xBFA597A200000000uL, 0xBE5240BEB8DF56CAuL,
     0xBFA5389600000000uL, 0xBE1AED65370B9099uL,
     0xBFA4D99400000000uL, 0xBE5429166D091C5DuL,
     0xBFA47A9E00000000uL, 0xBE44D5DB06B75692uL,
     0xBFA41BBA00000000uL, 0xBE5E4FF2E670387AuL,
     0xBFA3BCDA00000000uL, 0xBE5E73DF6E675ED2uL,
     0xBFA35E0D00000000uL, 0xBE5DF2994AF6BBF0uL,
     0xBFA2FF4C00000000uL, 0xBE31A09F65BFDEF1uL,
     0xBFA2A09500000000uL, 0xBE5290BAFE6A7061uL,
     0xBFA241EA00000000uL, 0xBE425151C43B4181uL,
     0xBFA1E34A00000000uL, 0xBE41D8DBC0646431uL,
     0xBFA184B500000000uL, 0xBE5298AC777C8C9DuL,
     0xBFA1263400000000uL, 0xBE10A2F9D7E8035AuL,
     0xBFA0C7B600000000uL, 0xBE0BBC4C660FD088uL,
     0xBFA0694B00000000uL, 0xBE3CC374B7950D13uL,
     0xBFA00AEB00000000uL, 0xBE5AA058ACDC0265uL,
     0xBF9F592000000000uL, 0xBE149B4D7E5DF2C0uL,
     0xBF9E9C8F00000000uL, 0xBE10A7A7E78BDBA3uL,
     0xBF9DE01500000000uL, 0xBDE02A1D978DB2F1uL,
     0xBF9D23B100000000uL, 0xBE4E9227A287068EuL,
     0xBF9C676500000000uL, 0xBE4E8561096793F8uL,
     0xBF9BAB3100000000uL, 0xBE0968E122179F22uL,
     0xBF9AEF1300000000uL, 0xBE328465C0DBA24FuL,
     0xBF9A330C00000000uL, 0xBE47051E31E0D70BuL,
     0xBF99771D00000000uL, 0xBE38B8D275FF3A9AuL,
     0xBF98BB5500000000uL, 0xBE122BDB89883925uL,
     0xBF97FF9400000000uL, 0xBE36FBF85D50FECBuL,
     0xBF9743EB00000000uL, 0xBDF87CBA8ECCAC44uL,
     0xBF96886800000000uL, 0xBE4BD57D800C1470uL,
     0xBF95CCEE00000000uL, 0xBE3BE2933856D62EuL,
     0xBF95118B00000000uL, 0xBE409620E0F1BE7BuL,
     0xBF94564F00000000uL, 0xBE4E4325CF62B811uL,
     0xBF939B1C00000000uL, 0xBE2ADEE9AF6A25C0uL,
     0xBF92E00000000000uL, 0xBE20CE46D28F63C9uL,
     0xBF92250B00000000uL, 0xBE41F6AA9FB6FE0BuL,
     0xBF916A1E00000000uL, 0xBE4E41409957601BuL,
     0xBF90AF5900000000uL, 0xBE4E53E5A63658ADuL,
     0xBF8FE93900000000uL, 0xBE3EDED24D629D7DuL,
     0xBF8E73EF00000000uL, 0xBE3A29D2EA7D362BuL,
     0xBF8CFEF500000000uL, 0xBE1E2E79FE4AA765uL,
     0xBF8B8A0A00000000uL, 0xBE3E8785027A216BuL,
     0xBF8A155000000000uL, 0xBE37A174D5A8BDEDuL,
     0xBF88A0C600000000uL, 0xBE35DDE88F39D7CEuL,
     0xBF872C6C00000000uL, 0xBE3C41EA3F44A785uL,
     0xBF85B86300000000uL, 0xBE194C69FFD7F42DuL,
     0xBF84446A00000000uL, 0xBE1A5E4E0D24AF39uL,
     0xBF82D0A100000000uL, 0xBE381611EB6C3818uL,
     0xBF815D0900000000uL, 0xBE3DD5DA9CC5F987uL,
     0xBF7FD34500000000uL, 0xBE25BD80E0B0590EuL,
     0xBF7CEC9900000000uL, 0xBE1CE47BB0EEA510uL,
     0xBF7A068E00000000uL, 0xBE26DBE100877575uL,
     0xBF7720E600000000uL, 0xBD9AA4F614B9E1ACuL,
     0xBF743B5F00000000uL, 0xBE271A96B1EB7842uL,
     0xBF71567B00000000uL, 0xBE2318F60005710DuL,
     0xBF6CE37400000000uL, 0xBE0C7A4E122B1762uL,
     0xBF671B3600000000uL, 0xBE1C85D1E3D214D1uL,
     0xBF61533F00000000uL, 0xBE0E793B61AA1F54uL,
     0xBF57181C00000000uL, 0xBE01296A4555AF78uL,
     0xBF47168E00000000uL, 0xBDF30D6F34EBFA1CuL,
     0x0000000000000000uL, 0x0000000000000000uL,
     },
    {

     0x3FF0000000000000uL, 0x0000000000000000uL,
     0x3FF0163DA9FB3335uL, 0x3C9B61299AB8CDB7uL,
     0x3FF02C9A3E778061uL, 0xBC719083535B085DuL,
     0x3FF04315E86E7F85uL, 0xBC90A31C1977C96EuL,
     0x3FF059B0D3158574uL, 0x3C8D73E2A475B465uL,
     0x3FF0706B29DDF6DEuL, 0xBC8C91DFE2B13C26uL,
     0x3FF0874518759BC8uL, 0x3C6186BE4BB284FFuL,
     0x3FF09E3ECAC6F383uL, 0x3C91487818316135uL,
     0x3FF0B5586CF9890FuL, 0x3C98A62E4ADC610AuL,
     0x3FF0CC922B7247F7uL, 0x3C901EDC16E24F71uL,
     0x3FF0E3EC32D3D1A2uL, 0x3C403A1727C57B52uL,
     0x3FF0FB66AFFED31BuL, 0xBC6B9BEDC44EBD7BuL,
     0x3FF11301D0125B51uL, 0xBC96C51039449B39uL,
     0x3FF12ABDC06C31CCuL, 0xBC51B514B36CA5C7uL,
     0x3FF1429AAEA92DE0uL, 0xBC932FBF9AF1369EuL,
     0x3FF15A98C8A58E51uL, 0x3C82406AB9EEAB09uL,
     0x3FF172B83C7D517BuL, 0xBC819041B9D78A75uL,
     0x3FF18AF9388C8DEAuL, 0xBC911023D1970F6BuL,
     0x3FF1A35BEB6FCB75uL, 0x3C8E5B4C7B4968E4uL,
     0x3FF1BBE084045CD4uL, 0xBC995386352EF607uL,
     0x3FF1D4873168B9AAuL, 0x3C9E016E00A2643CuL,
     0x3FF1ED5022FCD91DuL, 0xBC91DF98027BB78BuL,
     0x3FF2063B88628CD6uL, 0x3C8DC775814A8494uL,
     0x3FF21F49917DDC96uL, 0x3C82A97E9494A5EDuL,
     0x3FF2387A6E756238uL, 0x3C99B07EB6C70572uL,
     0x3FF251CE4FB2A63FuL, 0x3C8AC155BEF4F4A4uL,
     0x3FF26B4565E27CDDuL, 0x3C82BD339940E9D9uL,
     0x3FF284DFE1F56381uL, 0xBC9A4C3A8C3F0D7DuL,
     0x3FF29E9DF51FDEE1uL, 0x3C8612E8AFAD1255uL,
     0x3FF2B87FD0DAD990uL, 0xBC410ADCD6381AA3uL,
     0x3FF2D285A6E4030BuL, 0x3C90024754DB41D4uL,
     0x3FF2ECAFA93E2F56uL, 0x3C71CA0F45D52383uL,
     0x3FF306FE0A31B715uL, 0x3C86F46AD23182E4uL,
     0x3FF32170FC4CD831uL, 0x3C8A9CE78E18047CuL,
     0x3FF33C08B26416FFuL, 0x3C932721843659A5uL,
     0x3FF356C55F929FF1uL, 0xBC8B5CEE5C4E4628uL,
     0x3FF371A7373AA9CBuL, 0xBC963AEABF42EAE1uL,
     0x3FF38CAE6D05D866uL, 0xBC9E958D3C9904BCuL,
     0x3FF3A7DB34E59FF7uL, 0xBC75E436D661F5E2uL,
     0x3FF3C32DC313A8E5uL, 0xBC9EFFF8375D29C3uL,
     0x3FF3DEA64C123422uL, 0x3C8ADA0911F09EBBuL,
     0x3FF3FA4504AC801CuL, 0xBC97D023F956F9F3uL,
     0x3FF4160A21F72E2AuL, 0xBC5EF3691C309278uL,
     0x3FF431F5D950A897uL, 0xBC81C7DDE35F7998uL,
     0x3FF44E086061892DuL, 0x3C489B7A04EF80CFuL,
     0x3FF46A41ED1D0057uL, 0x3C9C944BD1648A76uL,
     0x3FF486A2B5C13CD0uL, 0x3C73C1A3B69062F0uL,
     0x3FF4A32AF0D7D3DEuL, 0x3C99CB62F3D1BE56uL,
     0x3FF4BFDAD5362A27uL, 0x3C7D4397AFEC42E2uL,
     0x3FF4DCB299FDDD0DuL, 0x3C98ECDBBC6A7833uL,
     0x3FF4F9B2769D2CA7uL, 0xBC94B309D25957E3uL,
     0x3FF516DAA2CF6642uL, 0xBC8F768569BD93EEuL,
     0x3FF5342B569D4F82uL, 0xBC807ABE1DB13CACuL,
     0x3FF551A4CA5D920FuL, 0xBC8D689CEFEDE59AuL,
     0x3FF56F4736B527DAuL, 0x3C99BB2C011D93ACuL,
     0x3FF58D12D497C7FDuL, 0x3C8295E15B9A1DE7uL,
     0x3FF5AB07DD485429uL, 0x3C96324C054647ACuL,
     0x3FF5C9268A5946B7uL, 0x3C3C4B1B816986A2uL,
     0x3FF5E76F15AD2148uL, 0x3C9BA6F93080E65DuL,
     0x3FF605E1B976DC09uL, 0xBC93E2429B56DE47uL,
     0x3FF6247EB03A5585uL, 0xBC9383C17E40B496uL,
     0x3FF6434634CCC320uL, 0xBC8C483C759D8932uL,
     0x3FF6623882552225uL, 0xBC9BB60987591C33uL,
     0x3FF68155D44CA973uL, 0x3C6038AE44F73E64uL,
     0x3FF6A09E667F3BCDuL, 0xBC9BDD3413B26455uL,
     0x3FF6C012750BDABFuL, 0xBC72895667FF0B0CuL,
     0x3FF6DFB23C651A2FuL, 0xBC6BBE3A683C88AAuL,
     0x3FF6FF7DF9519484uL, 0xBC883C0F25860EF6uL,
     0x3FF71F75E8EC5F74uL, 0xBC816E4786887A99uL,
     0x3FF73F9A48A58174uL, 0xBC90A8D96C65D53BuL,
     0x3FF75FEB564267C9uL, 0xBC90245957316DD3uL,
     0x3FF780694FDE5D3FuL, 0x3C9866B80A02162CuL,
     0x3FF7A11473EB0187uL, 0xBC841577EE04992FuL,
     0x3FF7C1ED0130C132uL, 0x3C9F124CD1164DD5uL,
     0x3FF7E2F336CF4E62uL, 0x3C705D02BA15797EuL,
     0x3FF80427543E1A12uL, 0xBC927C86626D972AuL,
     0x3FF82589994CCE13uL, 0xBC9D4C1DD41532D7uL,
     0x3FF8471A4623C7ADuL, 0xBC88D684A341CDFBuL,
     0x3FF868D99B4492EDuL, 0xBC9FC6F89BD4F6BAuL,
     0x3FF88AC7D98A6699uL, 0x3C9994C2F37CB53AuL,
     0x3FF8ACE5422AA0DBuL, 0x3C96E9F156864B26uL,
     0x3FF8CF3216B5448CuL, 0xBC70D55E32E9E3AAuL,
     0x3FF8F1AE99157736uL, 0x3C85CC13A2E3976CuL,
     0x3FF9145B0B91FFC6uL, 0xBC9DD6792E582523uL,
     0x3FF93737B0CDC5E5uL, 0xBC675FC781B57EBBuL,
     0x3FF95A44CBC8520FuL, 0xBC764B7C96A5F039uL,
     0x3FF97D829FDE4E50uL, 0xBC9D185B7C1B85D0uL,
     0x3FF9A0F170CA07BAuL, 0xBC9173BD91CEE632uL,
     0x3FF9C49182A3F090uL, 0x3C7C7C46B071F2BEuL,
     0x3FF9E86319E32323uL, 0x3C7824CA78E64C6EuL,
     0x3FFA0C667B5DE565uL, 0xBC9359495D1CD532uL,
     0x3FFA309BEC4A2D33uL, 0x3C96305C7DDC36ABuL,
     0x3FFA5503B23E255DuL, 0xBC9D2F6EDB8D41E1uL,
     0x3FFA799E1330B358uL, 0x3C9BCB7ECAC563C6uL,
     0x3FFA9E6B5579FDBFuL, 0x3C90FAC90EF7FD31uL,
     0x3FFAC36BBFD3F37AuL, 0xBC8F9234CAE76CD0uL,
     0x3FFAE89F995AD3ADuL, 0x3C97A1CD345DCC81uL,
     0x3FFB0E07298DB666uL, 0xBC9BDEF54C80E424uL,
     0x3FFB33A2B84F15FBuL, 0xBC62805E3084D707uL,
     0x3FFB59728DE5593AuL, 0xBC9C71DFBBBA6DE3uL,
     0x3FFB7F76F2FB5E47uL, 0xBC75584F7E54AC3AuL,
     0x3FFBA5B030A1064AuL, 0xBC9EFCD30E54292EuL,
     0x3FFBCC1E904BC1D2uL, 0x3C823DD07A2D9E84uL,
     0x3FFBF2C25BD71E09uL, 0xBC9EFDCA3F6B9C72uL,
     0x3FFC199BDD85529CuL, 0x3C811065895048DDuL,
     0x3FFC40AB5FFFD07AuL, 0x3C9B4537E083C60AuL,
     0x3FFC67F12E57D14BuL, 0x3C92884DFF483CACuL,
     0x3FFC8F6D9406E7B5uL, 0x3C71ACBC48805C44uL,
     0x3FFCB720DCEF9069uL, 0x3C7503CBD1E949DBuL,
     0x3FFCDF0B555DC3FAuL, 0xBC8DD83B53829D72uL,
     0x3FFD072D4A07897CuL, 0xBC9CBC3743797A9CuL,
     0x3FFD2F87080D89F2uL, 0xBC9D487B719D8577uL,
     0x3FFD5818DCFBA487uL, 0x3C82ED02D75B3706uL,
     0x3FFD80E316C98398uL, 0xBC911EC18BEDDFE8uL,
     0x3FFDA9E603DB3285uL, 0x3C9C2300696DB532uL,
     0x3FFDD321F301B460uL, 0x3C92DA5778F018C2uL,
     0x3FFDFC97337B9B5FuL, 0xBC91A5CD4F184B5BuL,
     0x3FFE264614F5A129uL, 0xBC97B627817A1496uL,
     0x3FFE502EE78B3FF6uL, 0x3C839E8980A9CC8FuL,
     0x3FFE7A51FBC74C83uL, 0x3C92D522CA0C8DE1uL,
     0x3FFEA4AFA2A490DAuL, 0xBC9E9C23179C2893uL,
     0x3FFECF482D8E67F1uL, 0xBC9C93F3B411AD8CuL,
     0x3FFEFA1BEE615A27uL, 0x3C9DC7F486A4B6B0uL,
     0x3FFF252B376BBA97uL, 0x3C93A1A5BF0D8E43uL,
     0x3FFF50765B6E4540uL, 0x3C99D3E12DD8A18AuL,
     0x3FFF7BFDAD9CBE14uL, 0xBC9DBB12D0063509uL,
     0x3FFFA7C1819E90D8uL, 0x3C874853F3A5931EuL,
     0x3FFFD3C22B8F71F1uL, 0x3C62EB74966579E7uL,
     },
    {

     0x3F903950CF599C56uL,
     0xBF9B4EA0E9419F52uL,
     0x3FA7A334DDFC9F86uL,
     0xBFB550472A8BB463uL,
     0x3FC47FD462B3B816uL,
     0xBFD62E4346694107uL,
     0x3E79C3A6966457EEuL,
     },
    {

     0x3F55D87FE78A6731uL,
     0x3F83B2AB6FBA4E77uL,
     0x3FAC6B08D704A0BFuL,
     0x3FCEBFBDFF82C58EuL,
     0x3FE62E42FEFA39EFuL,
     },

    0x000fffffffffffffuL,
    0x3fe7fe0000000000uL,
    0x3ff0000000000000uL,
    0xffffffff00000000uL,
    0x4138000000000000uL,
    0xfffffffff8000000uL,
    0xBFF7154740000000uL,
    0xfff0000000000000uL,
    0x42C8000000000000uL,

    0xFFF0000000000000uL,
    0x8000000000000000uL,
    0x4330000000000000uL,
    0x3900000000000000uL,

    0x00100000u,
    0x00200000u,
    0x7fffffffu,
    0x7f800000u,
    0x408f3fffu,

    0x000ffe00u,
    0x00000200u,

    0x3fe7fe00u,
    0x41380000u,
    0x3ff00000u,
    0x0000007fu,
};

static __constant _iml_v2_dp_union_t __dpow_la_CoutTab[860] = {

    0x00000000, 0x3FF00000,
    0x00000000, 0x3FEF07C0,
    0x00000000, 0x3FEE1E00,
    0x00000000, 0x3FED41C0,
    0x00000000, 0x3FEC71C0,
    0x00000000, 0x3FEBAD00,
    0x00000000, 0x3FEAF280,
    0x00000000, 0x3FEA41C0,
    0x00000000, 0x3FE99980,
    0x00000000, 0x3FE8F9C0,
    0x00000000, 0x3FE86180,
    0x00000000, 0x3FE7D040,
    0x00000000, 0x3FE745C0,
    0x00000000, 0x3FE6C180,
    0x00000000, 0x3FE642C0,
    0x00000000, 0x3FE5C980,
    0x00000000, 0x3FE55540,
    0x00000000, 0x3FE4E600,
    0x00000000, 0x3FE47B00,
    0x00000000, 0x3FE41400,
    0x00000000, 0x3FE3B140,
    0x00000000, 0x3FE35200,
    0x00000000, 0x3FE2F680,
    0x00000000, 0x3FE29E40,
    0x00000000, 0x3FE24940,
    0x00000000, 0x3FE1F700,
    0x00000000, 0x3FE1A7C0,
    0x00000000, 0x3FE15B00,
    0x00000000, 0x3FE11100,
    0x00000000, 0x3FE0C980,
    0x00000000, 0x3FE08440,
    0x00000000, 0x3FE04100,
    0x00000000, 0x3FE00000,

    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0xA01F0000, 0x3FA6BB01,
    0x7439DB71, 0x3D3C995E,
    0x4FF80000, 0x3FB66568,
    0x8DA93FB0, 0x3D3084F2,
    0x820EC000, 0x3FC08CD7,
    0x11B40207, 0x3D3543C5,
    0x64906000, 0x3FC5C048,
    0x7E5F3668, 0x3D28C5D4,
    0x032BE000, 0x3FCACF30,
    0xDEBF9166, 0x3D2E3733,
    0xE396E000, 0x3FCFBC44,
    0x28665438, 0x3D47B3F9,
    0x90D2B000, 0x3FD243A5,
    0xE8E9D45D, 0x3D3C9B75,
    0xA118D000, 0x3FD49AD4,
    0x7302CCA6, 0x3D45CD37,
    0x92EF1000, 0x3FD6E227,
    0x0E7E9039, 0x3D314F24,
    0x9E695000, 0x3FD91BD1,
    0xE4F6C667, 0x3D4DBB3E,
    0x273ED000, 0x3FDB4865,
    0x099E1F61, 0x3D4AB54A,
    0x20231000, 0x3FDD6799,
    0x96E87504, 0x3D18ED50,
    0x9E747000, 0x3FDF7A34,
    0x81D99120, 0x3D4A6E70,
    0x50CF0000, 0x3FE0C116,
    0xEB1152A5, 0x3D461752,
    0x6E8E9800, 0x3FE1BF42,
    0x6C055F56, 0x3D376AFF,
    0x1C354000, 0xBFDA8F9D,
    0x4F4F9854, 0xBD4604F5,
    0x8A043000, 0xBFD8A922,
    0xCF8DD884, 0xBD49BC20,
    0xE7AA4000, 0xBFD6CB99,
    0xD5A7002B, 0xBD412B5A,
    0x5D830000, 0xBFD4F69F,
    0xD24BAE46, 0xBD38F36D,
    0xF8B57000, 0xBFD32C15,
    0xE01D9232, 0xBD0D6EE2,
    0xD34FB000, 0xBFD16935,
    0x348D84A5, 0xBD2151C6,
    0x7E71A000, 0xBFCF5FAA,
    0x20C552C2, 0xBD3D1576,
    0x1D5BE000, 0xBFCBFC5C,
    0x0E42B538, 0xBD278490,
    0x0948A000, 0xBFC8A9AD,
    0x64F25A56, 0xBD4C89BA,
    0xFF6B0000, 0xBFC563AD,
    0x079422C3, 0xBD4D0837,
    0x02746000, 0xBFC22DF3,
    0xC2505D3D, 0xBD3048E3,
    0x92EE4000, 0xBFBE0894,
    0xFCD57F87, 0xBD405589,
    0x41A08000, 0xBFB7D493,
    0xBCF7AA55, 0xBD4EEEF8,
    0xCF5A4000, 0xBFB1BC75,
    0x139E8397, 0xBD4D7DB2,
    0xC1828000, 0xBFA778FD,
    0xF2AF5333, 0xBD34378A,
    0x317A0000, 0xBF97427D,
    0x4B03B094, 0xBD4700F1,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,

    0x00000000, 0x3FF00000,
    0x00000000, 0x3FEFF000,
    0x00000000, 0x3FEFE000,
    0x00000000, 0x3FEFD040,
    0x00000000, 0x3FEFC080,
    0x00000000, 0x3FEFB0C0,
    0x00000000, 0x3FEFA100,
    0x00000000, 0x3FEF9180,
    0x00000000, 0x3FEF8200,
    0x00000000, 0x3FEF7280,
    0x00000000, 0x3FEF6300,
    0x00000000, 0x3FEF53C0,
    0x00000000, 0x3FEF4480,
    0x00000000, 0x3FEF3540,
    0x00000000, 0x3FEF2600,
    0x00000000, 0x3FF04540,
    0x00000000, 0x3FF04100,
    0x00000000, 0x3FF03D00,
    0x00000000, 0x3FF038C0,
    0x00000000, 0x3FF034C0,
    0x00000000, 0x3FF03080,
    0x00000000, 0x3FF02C80,
    0x00000000, 0x3FF02880,
    0x00000000, 0x3FF02440,
    0x00000000, 0x3FF02040,
    0x00000000, 0x3FF01C40,
    0x00000000, 0x3FF01840,
    0x00000000, 0x3FF01400,
    0x00000000, 0x3FF01000,
    0x00000000, 0x3FF00C00,
    0x00000000, 0x3FF00800,
    0x00000000, 0x3FF00400,
    0x00000000, 0x3FF00000,

    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0xA4280000, 0x3F671B0E,
    0x9876EF47, 0x3D497F68,
    0xC0680000, 0x3F7720D9,
    0x3778C7CC, 0x3D441AF5,
    0x8A7A0000, 0x3F8145C6,
    0x9AC06488, 0x3D44BDD1,
    0x61D20000, 0x3F86FDF4,
    0x46D9F6F7, 0x3D3C9EFC,
    0x355C0000, 0x3F8CB8F9,
    0xADFBB459, 0x3D3275C9,
    0xEC350000, 0x3F913B6B,
    0x4FC32ADB, 0x3D0F25B4,
    0x7B760000, 0x3F941016,
    0xA8ED5317, 0x3D3F880C,
    0x317A0000, 0x3F96E625,
    0x645614DB, 0x3D453F0E,
    0x6D010000, 0x3F99BD99,
    0x341A2DAB, 0x3D2CD686,
    0x8ED00000, 0x3F9C9674,
    0x66D10B04, 0x3D4EF88D,
    0xD4200000, 0x3F9F64ED,
    0x828828DA, 0x3D4511C3,
    0x97920000, 0x3FA11A62,
    0xD7D436D6, 0x3D4D925C,
    0xFAD70000, 0x3FA282FD,
    0xA58B8D6E, 0x3D49EEE0,
    0xBFC20000, 0x3FA3EC49,
    0x2E0E0086, 0x3D4DEAC3,
    0x31230000, 0xBF98C493,
    0x5EFCABFA, 0xBD49AD07,
    0x317A0000, 0xBF97427D,
    0x4B03B094, 0xBD4700F1,
    0xEE910000, 0xBF95D6C0,
    0x535202A3, 0xBD4A5115,
    0xADB60000, 0xBF9453E6,
    0xE102F731, 0xBD415A44,
    0x57080000, 0xBF92E771,
    0x5EE9AD86, 0xBD4C7ED8,
    0x0D100000, 0xBF9163D2,
    0x664FE33F, 0xBD46E8B9,
    0xCBCC0000, 0xBF8FED45,
    0x43464056, 0xBD37F339,
    0xC5EA0000, 0xBF8D1232,
    0xB0BDC8DF, 0xBD17CF34,
    0x28680000, 0xBF8A08A8,
    0xF02B9CCF, 0xBD35A529,
    0x4CF00000, 0xBF872C1F,
    0x580FE573, 0xBD2B4934,
    0xA6F20000, 0xBF844EE0,
    0xFF314317, 0xBD24C8CB,
    0xDC1C0000, 0xBF8170EB,
    0x2CC5232F, 0xBD447DB0,
    0x97D40000, 0xBF7CC89F,
    0x90330E7B, 0xBD43AC9C,
    0x6D780000, 0xBF7709C4,
    0x56CDE925, 0xBD4563BA,
    0xCCF40000, 0xBF71497A,
    0xDDD3E770, 0xBD4F08E7,
    0xFF080000, 0xBF670F83,
    0x31D4676D, 0xBD33AB26,
    0x37400000, 0xBF571265,
    0xFD4FCA1D, 0xBD2FA2A0,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,

    0x40000000, 0x3FF71547,
    0x00000000, 0x3FF71519,
    0x00000000, 0x3FF714EB,
    0xC0000000, 0x3FF714BC,
    0x80000000, 0x3FF7148E,
    0x80000000, 0x3FF71460,
    0x40000000, 0x3FF71432,
    0x40000000, 0x3FF71404,
    0x00000000, 0x3FF713D6,
    0xC0000000, 0x3FF713A7,
    0xC0000000, 0x3FF71379,
    0x80000000, 0x3FF7134B,
    0x80000000, 0x3FF7131D,
    0x40000000, 0x3FF712EF,
    0x40000000, 0x3FF712C1,
    0x00000000, 0x3FF71293,
    0x00000000, 0x3FF71265,
    0xC0000000, 0x3FF71236,
    0xC0000000, 0x3FF71208,
    0x80000000, 0x3FF711DA,
    0x80000000, 0x3FF711AC,
    0x80000000, 0x3FF7117E,
    0x40000000, 0x3FF71150,
    0x40000000, 0x3FF71122,
    0x00000000, 0x3FF710F4,
    0x00000000, 0x3FF710C6,
    0x00000000, 0x3FF71098,
    0xC0000000, 0x3FF71069,
    0xC0000000, 0x3FF7103B,
    0x80000000, 0x3FF7100D,
    0x80000000, 0x3FF70FDF,
    0x80000000, 0x3FF70FB1,
    0x40000000, 0x3FF70F83,
    0x40000000, 0x3FF70F55,
    0x40000000, 0x3FF70F27,
    0x40000000, 0x3FF70EF9,
    0x00000000, 0x3FF70ECB,
    0x00000000, 0x3FF70E9D,
    0x00000000, 0x3FF70E6F,
    0x00000000, 0x3FF70E41,
    0xC0000000, 0x3FF70E12,
    0xC0000000, 0x3FF70DE4,
    0xC0000000, 0x3FF70DB6,
    0xC0000000, 0x3FF70D88,
    0xC0000000, 0x3FF70D5A,
    0x80000000, 0x3FF70D2C,
    0x80000000, 0x3FF70CFE,
    0x80000000, 0x3FF70CD0,
    0x80000000, 0x3FF70CA2,
    0x80000000, 0x3FF70C74,
    0x80000000, 0x3FF70C46,
    0x80000000, 0x3FF70C18,
    0x80000000, 0x3FF70BEA,
    0x80000000, 0x3FF70BBC,
    0x40000000, 0x3FF70B8E,
    0x40000000, 0x3FF70B60,
    0x40000000, 0x3FF70B32,
    0x40000000, 0x3FF70B04,
    0x40000000, 0x3FF70AD6,
    0x40000000, 0x3FF70AA8,
    0x40000000, 0x3FF70A7A,
    0x80000000, 0x3FF71B53,
    0x40000000, 0x3FF71B3C,
    0x40000000, 0x3FF71B25,
    0x00000000, 0x3FF71B0E,
    0x00000000, 0x3FF71AF7,
    0xC0000000, 0x3FF71ADF,
    0xC0000000, 0x3FF71AC8,
    0x80000000, 0x3FF71AB1,
    0x80000000, 0x3FF71A9A,
    0x40000000, 0x3FF71A83,
    0x40000000, 0x3FF71A6C,
    0x00000000, 0x3FF71A55,
    0x00000000, 0x3FF71A3E,
    0xC0000000, 0x3FF71A26,
    0xC0000000, 0x3FF71A0F,
    0x80000000, 0x3FF719F8,
    0x80000000, 0x3FF719E1,
    0x40000000, 0x3FF719CA,
    0x40000000, 0x3FF719B3,
    0x00000000, 0x3FF7199C,
    0x00000000, 0x3FF71985,
    0xC0000000, 0x3FF7196D,
    0xC0000000, 0x3FF71956,
    0x80000000, 0x3FF7193F,
    0x80000000, 0x3FF71928,
    0x40000000, 0x3FF71911,
    0x40000000, 0x3FF718FA,
    0x40000000, 0x3FF718E3,
    0x00000000, 0x3FF718CC,
    0x00000000, 0x3FF718B5,
    0xC0000000, 0x3FF7189D,
    0xC0000000, 0x3FF71886,
    0x80000000, 0x3FF7186F,
    0x80000000, 0x3FF71858,
    0x80000000, 0x3FF71841,
    0x40000000, 0x3FF7182A,
    0x40000000, 0x3FF71813,
    0x00000000, 0x3FF717FC,
    0x00000000, 0x3FF717E5,
    0xC0000000, 0x3FF717CD,
    0xC0000000, 0x3FF717B6,
    0xC0000000, 0x3FF7179F,
    0x80000000, 0x3FF71788,
    0x80000000, 0x3FF71771,
    0x40000000, 0x3FF7175A,
    0x40000000, 0x3FF71743,
    0x40000000, 0x3FF7172C,
    0x00000000, 0x3FF71715,
    0x00000000, 0x3FF716FE,
    0xC0000000, 0x3FF716E6,
    0xC0000000, 0x3FF716CF,
    0xC0000000, 0x3FF716B8,
    0x80000000, 0x3FF716A1,
    0x80000000, 0x3FF7168A,
    0x80000000, 0x3FF71673,
    0x40000000, 0x3FF7165C,
    0x40000000, 0x3FF71645,
    0x00000000, 0x3FF7162E,
    0x00000000, 0x3FF71617,
    0x00000000, 0x3FF71600,
    0xC0000000, 0x3FF715E8,
    0xC0000000, 0x3FF715D1,
    0xC0000000, 0x3FF715BA,
    0x80000000, 0x3FF715A3,
    0x80000000, 0x3FF7158C,
    0x80000000, 0x3FF71575,
    0x40000000, 0x3FF7155E,
    0x40000000, 0x3FF71547,

    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x50000000, 0x3F072017,
    0x641F4F36, 0x3D099925,
    0x3B000000, 0x3F17102E,
    0xC162D124, 0x3D120082,
    0x0C800000, 0x3F215034,
    0x09125FF4, 0x3D441EE7,
    0x92000000, 0x3F27185C,
    0x8BABF46F, 0x3CEECD1C,
    0x5D000000, 0x3F2CD890,
    0xF0BBF0CF, 0x3D3BEE60,
    0xFF400000, 0x3F315067,
    0x8A9648DD, 0x3D3FDBDF,
    0x63400000, 0x3F34308D,
    0xAEE1B670, 0x3D3681CB,
    0xC2800000, 0x3F3714B8,
    0xBF3D153E, 0x3D4102E6,
    0xED400000, 0x3F39F8E9,
    0x33794ABC, 0x3D365EF2,
    0x93400000, 0x3F3CD920,
    0x3D384508, 0x3D4FDF0B,
    0x4D400000, 0x3F3FBD5D,
    0x782C348A, 0x3D19A166,
    0xB9400000, 0x3F414ECF,
    0x39DDE07F, 0x3D48ABA0,
    0xDDE00000, 0x3F42C0F3,
    0x9D5FD823, 0x3D435F8A,
    0xB0600000, 0x3F44311A,
    0x12458BEC, 0x3D47963A,
    0x9CE00000, 0x3F45A344,
    0x2C8C13FD, 0x3D47E151,
    0x2F600000, 0x3F471371,
    0xCB0BD4FA, 0x3D48D0BE,
    0xE4000000, 0x3F4885A0,
    0xDCE1A474, 0x3D39DB21,
    0x36A00000, 0x3F49F5D3,
    0x55660916, 0x3D489166,
    0xB3800000, 0x3F4B6808,
    0xC862A7D0, 0x3D3D7958,
    0xC6800000, 0x3F4CD840,
    0xD43B70F4, 0x3D4312BA,
    0xB7C00000, 0x3F4E487B,
    0x975E2C41, 0x3D46363F,
    0xDF600000, 0x3F4FBAB9,
    0x850FC6C3, 0x3D224E63,
    0x48A00000, 0x3F50957D,
    0x17A21AA6, 0x3D256896,
    0x40C00000, 0x3F514E9F,
    0xA4582824, 0x3D32AE5F,
    0x7A100000, 0x3F5206C2,
    0x84B0FD57, 0x3D45C3F8,
    0x22A00000, 0x3F52BEE7,
    0x4AAD1649, 0x3D4A525C,
    0x70800000, 0x3F53780D,
    0x275071DE, 0x3D1DEB18,
    0xF9900000, 0x3F543034,
    0xD5D75FB6, 0x3D48EAE4,
    0x2C000000, 0x3F54E95E,
    0xC1291B85, 0x3D3728C5,
    0x95B00000, 0x3F55A188,
    0xABECF0D7, 0x3D4F699A,
    0x6ED00000, 0x3F5659B4,
    0x8F2D1FA9, 0x3D22ED25,
    0xF7400000, 0x3F5712E1,
    0x8B30E580, 0x3D3445EF,
    0xB1100000, 0x3F57CB10,
    0xDC75FAC6, 0x3D431D52,
    0xDA500000, 0x3F588340,
    0xA135BD69, 0x3D3FFBCB,
    0x73000000, 0x3F593B72,
    0x2D63E5DB, 0x3D387100,
    0xC3200000, 0x3F59F4A5,
    0x1C0BB062, 0x3D45AFDD,
    0x3CC00000, 0x3F5AACDA,
    0xDCD040AE, 0x3D41914F,
    0x25E00000, 0x3F5B6510,
    0xDB245B1F, 0x3D3E7387,
    0x7E800000, 0x3F5C1D47,
    0x593D6B3F, 0x3D4A1E1B,
    0x96C00000, 0x3F5CD680,
    0xCC31FC26, 0x3CEA7D5C,
    0xD0800000, 0x3F5D8EBA,
    0x762A3069, 0x3D2BEFBC,
    0x79D00000, 0x3F5E46F6,
    0x952BE02C, 0x3D4D39FF,
    0x92D00000, 0x3F5EFF33,
    0x3B1A1CB4, 0x3D3D3437,
    0x1B700000, 0x3F5FB772,
    0x57218470, 0x3D2A49F2,
    0x36D80000, 0x3F603859,
    0x602BA3B5, 0x3D43ADB6,
    0xEBD00000, 0x3F609479,
    0x0591EE13, 0x3D4A6056,
    0x58A80000, 0x3F60F09B,
    0x4FAFF44B, 0x3D343EEC,
    0x7D580000, 0x3F614CBD,
    0x130DF139, 0x3D3CC4C6,
    0x59E80000, 0x3F61A8E0,
    0x9380107F, 0x3D42A0AD,
    0xEE600000, 0x3F620503,
    0xB9035A2A, 0x3D12915A,
    0x3AB80000, 0x3F626128,
    0x20CAACA3, 0x3D31DDED,
    0x3EF80000, 0x3F62BD4D,
    0x281079C7, 0x3D41EF6E,
    0xFB280000, 0x3F631972,
    0x6B98497F, 0x3D31986B,
    0xA5400000, 0x3F637619,
    0x91F2B430, 0x3D4C3697,
    0xD2500000, 0x3F63D240,
    0xCE1C0762, 0x3D40ECA7,
    0xB7500000, 0x3F642E68,
    0x073B1E2E, 0x3D45FA86,
    0x54480000, 0x3F648A91,
    0x05A622FD, 0x3D46D9E9,
    0xA9380000, 0x3F64E6BA,
    0x4B55A072, 0x3D4F04CC,
    0xB6280000, 0x3F6542E4,
    0x16094E0E, 0x3D49F570,
    0x7B180000, 0x3F659F0F,
    0x616CF239, 0x3D432659,
    0x69800000, 0xBF582DD5,
    0x3ED708F7, 0xBD3AB541,
    0x91900000, 0xBF57D0ED,
    0x7B358E46, 0xBD4135C6,
    0x1B300000, 0xBF577505,
    0xF6ED6FDE, 0xBCFEF3C0,
    0x89400000, 0xBF57181C,
    0x5AF7807F, 0xBD46A455,
    0x5AE00000, 0xBF56BC33,
    0x919C892D, 0xBD452637,
    0x0F000000, 0xBF565F4A,
    0x806E8ED0, 0xBD46D380,
    0x28A00000, 0xBF560360,
    0xFEDC7D6B, 0xBD4E545E,
    0x22D00000, 0xBF55A676,
    0xFDAB27BC, 0xBD28C45A,
    0x84700000, 0xBF554A8B,
    0x94F1D540, 0xBD40EFC0,
    0xC4900000, 0xBF54EDA0,
    0x6E5D7E55, 0xBD492AA1,
    0x6E400000, 0xBF5491B5,
    0xAE3A4995, 0xBD065C31,
    0xF4600000, 0xBF5434C9,
    0x90B30C51, 0xBD20B591,
    0xE6000000, 0xBF53D8DD,
    0x5C64292C, 0xBD208E1F,
    0xB2100000, 0xBF537BF1,
    0xF606F2E6, 0xBD4BA65D,
    0xEBB00000, 0xBF532004,
    0x1A593CA0, 0xBD3B2BD5,
    0xFDC00000, 0xBF52C317,
    0xF5360F7D, 0xBD440246,
    0x7F500000, 0xBF52672A,
    0x7BBFCA47, 0xBD422981,
    0xD7600000, 0xBF520A3C,
    0xCF30F123, 0xBD0AD92A,
    0xA0E00000, 0xBF51AE4E,
    0x693830AD, 0xBD292A75,
    0x3ED00000, 0xBF515160,
    0xE0391426, 0xBD49146E,
    0x50400000, 0xBF50F571,
    0x57002344, 0xBD4E6549,
    0x34300000, 0xBF509882,
    0x8F311F09, 0xBD3D4587,
    0x8D900000, 0xBF503C92,
    0x28FE4EA6, 0xBD3DCA96,
    0x6EC00000, 0xBF4FBF45,
    0x0F0A4C7F, 0xBD46C434,
    0xB1600000, 0xBF4F0764,
    0x4F8F13F5, 0xBD386C45,
    0x90E00000, 0xBF4E4D83,
    0x4F099D6A, 0xBD279073,
    0x63400000, 0xBF4D95A1,
    0x2BF804AC, 0xBD358615,
    0x7E800000, 0xBF4CDDBE,
    0xEDA75E42, 0xBD43F2DC,
    0x30A00000, 0xBF4C23DB,
    0xD23862D0, 0xBD4EF6F7,
    0xDBC00000, 0xBF4B6BF6,
    0x2300F78C, 0xBD3DE7D1,
    0x19A00000, 0xBF4AB212,
    0xBB645928, 0xBD4D3C99,
    0x54800000, 0xBF49FA2C,
    0xADF18185, 0xBD3C87D4,
    0x1E200000, 0xBF494046,
    0x1A0618B0, 0xBD49FE5A,
    0xE8C00000, 0xBF48885E,
    0xAAEEF6A6, 0xBD2934A5,
    0xFC200000, 0xBF47D076,
    0xECBB5462, 0xBD39CFC0,
    0x98600000, 0xBF47168E,
    0xBFA1C16B, 0xBD36F34E,
    0x3B800000, 0xBF465EA5,
    0x9088CA01, 0xBD22ED1C,
    0x63600000, 0xBF45A4BB,
    0x2D6DE9B5, 0xBD423320,
    0x96200000, 0xBF44ECD0,
    0x10D84808, 0xBD485871,
    0x49C00000, 0xBF4432E5,
    0x5992900A, 0xBD3DC614,
    0x0C200000, 0xBF437AF9,
    0xF611F4F2, 0xBD4C28B1,
    0x17600000, 0xBF42C30C,
    0x12496DA4, 0xBD49B472,
    0x9D800000, 0xBF42091E,
    0xB07B63E5, 0xBD22545F,
    0x38600000, 0xBF415130,
    0x6B4DAA19, 0xBCF3FC74,
    0x4A000000, 0xBF409741,
    0x9F7943AC, 0xBD2819AD,
    0xE8C00000, 0xBF3FBEA2,
    0x05018F01, 0xBD4ACC92,
    0xCF400000, 0xBF3E4EC1,
    0x5DECAD9C, 0xBD48B028,
    0x97400000, 0xBF3CDADF,
    0x63115207, 0xBD4CA2DD,
    0x9D000000, 0xBF3B6AFB,
    0xA7FC9363, 0xBD354003,
    0x7C000000, 0xBF39F716,
    0x407A7831, 0xBD4A8AE0,
    0xA0C00000, 0xBF38872F,
    0x91A8939D, 0xBD42DD8B,
    0x57000000, 0xBF371747,
    0x2F243D55, 0xBD3A4925,
    0xDAC00000, 0xBF35A35D,
    0x22A2581B, 0xBD2741DF,
    0xB0000000, 0xBF343372,
    0x3C22E0D2, 0xBD0CAB75,
    0x16800000, 0xBF32C386,
    0x5FC1D4DD, 0xBD4B6E11,
    0x3EC00000, 0xBF314F98,
    0x97121E28, 0xBD3F929A,
    0x89000000, 0xBF2FBF51,
    0x806FDBDA, 0xBD13EA1E,
    0x07000000, 0xBF2CD770,
    0x51E95ECB, 0xBD370EB1,
    0x50000000, 0xBF29F78B,
    0xFF247507, 0xBD418759,
    0xBC000000, 0xBF2717A3,
    0xC6C6F140, 0xBD3C0D6B,
    0x82800000, 0xBF242FB9,
    0xB8D3C162, 0xBD4CF2EF,
    0x2C800000, 0xBF214FCC,
    0x12CFE97E, 0xBD206CE3,
    0xF1000000, 0xBF1CDFB7,
    0x1056AF68, 0xBD4D7734,
    0x10000000, 0xBF170FD2,
    0x29329AEF, 0xBD24CAA4,
    0x24000000, 0xBF114FE6,
    0x06B71311, 0xBCF284C3,
    0xFA000000, 0xBF071FE8,
    0xDD98569A, 0xBD33BB0C,
    0xAC000000, 0xBEF6FFF4,
    0x0B7407C7, 0xBD4D54D7,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,

    0x00000000, 0x3FF00000,
    0x00000000, 0x00000000,
    0xA9FB3335, 0x3FF0163D,
    0x9AB8CDB7, 0x3C9B6129,
    0x3E778061, 0x3FF02C9A,
    0x535B085D, 0xBC719083,
    0xE86E7F85, 0x3FF04315,
    0x1977C96E, 0xBC90A31C,
    0xD3158574, 0x3FF059B0,
    0xA475B465, 0x3C8D73E2,
    0x29DDF6DE, 0x3FF0706B,
    0xE2B13C26, 0xBC8C91DF,
    0x18759BC8, 0x3FF08745,
    0x4BB284FF, 0x3C6186BE,
    0xCAC6F383, 0x3FF09E3E,
    0x18316135, 0x3C914878,
    0x6CF9890F, 0x3FF0B558,
    0x4ADC610A, 0x3C98A62E,
    0x2B7247F7, 0x3FF0CC92,
    0x16E24F71, 0x3C901EDC,
    0x32D3D1A2, 0x3FF0E3EC,
    0x27C57B52, 0x3C403A17,
    0xAFFED31B, 0x3FF0FB66,
    0xC44EBD7B, 0xBC6B9BED,
    0xD0125B51, 0x3FF11301,
    0x39449B39, 0xBC96C510,
    0xC06C31CC, 0x3FF12ABD,
    0xB36CA5C7, 0xBC51B514,
    0xAEA92DE0, 0x3FF1429A,
    0x9AF1369E, 0xBC932FBF,
    0xC8A58E51, 0x3FF15A98,
    0xB9EEAB09, 0x3C82406A,
    0x3C7D517B, 0x3FF172B8,
    0xB9D78A75, 0xBC819041,
    0x388C8DEA, 0x3FF18AF9,
    0xD1970F6B, 0xBC911023,
    0xEB6FCB75, 0x3FF1A35B,
    0x7B4968E4, 0x3C8E5B4C,
    0x84045CD4, 0x3FF1BBE0,
    0x352EF607, 0xBC995386,
    0x3168B9AA, 0x3FF1D487,
    0x00A2643C, 0x3C9E016E,
    0x22FCD91D, 0x3FF1ED50,
    0x027BB78B, 0xBC91DF98,
    0x88628CD6, 0x3FF2063B,
    0x814A8494, 0x3C8DC775,
    0x917DDC96, 0x3FF21F49,
    0x9494A5ED, 0x3C82A97E,
    0x6E756238, 0x3FF2387A,
    0xB6C70572, 0x3C99B07E,
    0x4FB2A63F, 0x3FF251CE,
    0xBEF4F4A4, 0x3C8AC155,
    0x65E27CDD, 0x3FF26B45,
    0x9940E9D9, 0x3C82BD33,
    0xE1F56381, 0x3FF284DF,
    0x8C3F0D7D, 0xBC9A4C3A,
    0xF51FDEE1, 0x3FF29E9D,
    0xAFAD1255, 0x3C8612E8,
    0xD0DAD990, 0x3FF2B87F,
    0xD6381AA3, 0xBC410ADC,
    0xA6E4030B, 0x3FF2D285,
    0x54DB41D4, 0x3C900247,
    0xA93E2F56, 0x3FF2ECAF,
    0x45D52383, 0x3C71CA0F,
    0x0A31B715, 0x3FF306FE,
    0xD23182E4, 0x3C86F46A,
    0xFC4CD831, 0x3FF32170,
    0x8E18047C, 0x3C8A9CE7,
    0xB26416FF, 0x3FF33C08,
    0x843659A5, 0x3C932721,
    0x5F929FF1, 0x3FF356C5,
    0x5C4E4628, 0xBC8B5CEE,
    0x373AA9CB, 0x3FF371A7,
    0xBF42EAE1, 0xBC963AEA,
    0x6D05D866, 0x3FF38CAE,
    0x3C9904BC, 0xBC9E958D,
    0x34E59FF7, 0x3FF3A7DB,
    0xD661F5E2, 0xBC75E436,
    0xC313A8E5, 0x3FF3C32D,
    0x375D29C3, 0xBC9EFFF8,
    0x4C123422, 0x3FF3DEA6,
    0x11F09EBB, 0x3C8ADA09,
    0x04AC801C, 0x3FF3FA45,
    0xF956F9F3, 0xBC97D023,
    0x21F72E2A, 0x3FF4160A,
    0x1C309278, 0xBC5EF369,
    0xD950A897, 0x3FF431F5,
    0xE35F7998, 0xBC81C7DD,
    0x6061892D, 0x3FF44E08,
    0x04EF80CF, 0x3C489B7A,
    0xED1D0057, 0x3FF46A41,
    0xD1648A76, 0x3C9C944B,
    0xB5C13CD0, 0x3FF486A2,
    0xB69062F0, 0x3C73C1A3,
    0xF0D7D3DE, 0x3FF4A32A,
    0xF3D1BE56, 0x3C99CB62,
    0xD5362A27, 0x3FF4BFDA,
    0xAFEC42E2, 0x3C7D4397,
    0x99FDDD0D, 0x3FF4DCB2,
    0xBC6A7833, 0x3C98ECDB,
    0x769D2CA7, 0x3FF4F9B2,
    0xD25957E3, 0xBC94B309,
    0xA2CF6642, 0x3FF516DA,
    0x69BD93EE, 0xBC8F7685,
    0x569D4F82, 0x3FF5342B,
    0x1DB13CAC, 0xBC807ABE,
    0xCA5D920F, 0x3FF551A4,
    0xEFEDE59A, 0xBC8D689C,
    0x36B527DA, 0x3FF56F47,
    0x011D93AC, 0x3C99BB2C,
    0xD497C7FD, 0x3FF58D12,
    0x5B9A1DE7, 0x3C8295E1,
    0xDD485429, 0x3FF5AB07,
    0x054647AC, 0x3C96324C,
    0x8A5946B7, 0x3FF5C926,
    0x816986A2, 0x3C3C4B1B,
    0x15AD2148, 0x3FF5E76F,
    0x3080E65D, 0x3C9BA6F9,
    0xB976DC09, 0x3FF605E1,
    0x9B56DE47, 0xBC93E242,
    0xB03A5585, 0x3FF6247E,
    0x7E40B496, 0xBC9383C1,
    0x34CCC320, 0x3FF64346,
    0x759D8932, 0xBC8C483C,
    0x82552225, 0x3FF66238,
    0x87591C33, 0xBC9BB609,
    0xD44CA973, 0x3FF68155,
    0x44F73E64, 0x3C6038AE,
    0x667F3BCD, 0x3FF6A09E,
    0x13B26455, 0xBC9BDD34,
    0x750BDABF, 0x3FF6C012,
    0x67FF0B0C, 0xBC728956,
    0x3C651A2F, 0x3FF6DFB2,
    0x683C88AA, 0xBC6BBE3A,
    0xF9519484, 0x3FF6FF7D,
    0x25860EF6, 0xBC883C0F,
    0xE8EC5F74, 0x3FF71F75,
    0x86887A99, 0xBC816E47,
    0x48A58174, 0x3FF73F9A,
    0x6C65D53B, 0xBC90A8D9,
    0x564267C9, 0x3FF75FEB,
    0x57316DD3, 0xBC902459,
    0x4FDE5D3F, 0x3FF78069,
    0x0A02162C, 0x3C9866B8,
    0x73EB0187, 0x3FF7A114,
    0xEE04992F, 0xBC841577,
    0x0130C132, 0x3FF7C1ED,
    0xD1164DD5, 0x3C9F124C,
    0x36CF4E62, 0x3FF7E2F3,
    0xBA15797E, 0x3C705D02,
    0x543E1A12, 0x3FF80427,
    0x626D972A, 0xBC927C86,
    0x994CCE13, 0x3FF82589,
    0xD41532D7, 0xBC9D4C1D,
    0x4623C7AD, 0x3FF8471A,
    0xA341CDFB, 0xBC88D684,
    0x9B4492ED, 0x3FF868D9,
    0x9BD4F6BA, 0xBC9FC6F8,
    0xD98A6699, 0x3FF88AC7,
    0xF37CB53A, 0x3C9994C2,
    0x422AA0DB, 0x3FF8ACE5,
    0x56864B26, 0x3C96E9F1,
    0x16B5448C, 0x3FF8CF32,
    0x32E9E3AA, 0xBC70D55E,
    0x99157736, 0x3FF8F1AE,
    0xA2E3976C, 0x3C85CC13,
    0x0B91FFC6, 0x3FF9145B,
    0x2E582523, 0xBC9DD679,
    0xB0CDC5E5, 0x3FF93737,
    0x81B57EBB, 0xBC675FC7,
    0xCBC8520F, 0x3FF95A44,
    0x96A5F039, 0xBC764B7C,
    0x9FDE4E50, 0x3FF97D82,
    0x7C1B85D0, 0xBC9D185B,
    0x70CA07BA, 0x3FF9A0F1,
    0x91CEE632, 0xBC9173BD,
    0x82A3F090, 0x3FF9C491,
    0xB071F2BE, 0x3C7C7C46,
    0x19E32323, 0x3FF9E863,
    0x78E64C6E, 0x3C7824CA,
    0x7B5DE565, 0x3FFA0C66,
    0x5D1CD532, 0xBC935949,
    0xEC4A2D33, 0x3FFA309B,
    0x7DDC36AB, 0x3C96305C,
    0xB23E255D, 0x3FFA5503,
    0xDB8D41E1, 0xBC9D2F6E,
    0x1330B358, 0x3FFA799E,
    0xCAC563C6, 0x3C9BCB7E,
    0x5579FDBF, 0x3FFA9E6B,
    0x0EF7FD31, 0x3C90FAC9,
    0xBFD3F37A, 0x3FFAC36B,
    0xCAE76CD0, 0xBC8F9234,
    0x995AD3AD, 0x3FFAE89F,
    0x345DCC81, 0x3C97A1CD,
    0x298DB666, 0x3FFB0E07,
    0x4C80E424, 0xBC9BDEF5,
    0xB84F15FB, 0x3FFB33A2,
    0x3084D707, 0xBC62805E,
    0x8DE5593A, 0x3FFB5972,
    0xBBBA6DE3, 0xBC9C71DF,
    0xF2FB5E47, 0x3FFB7F76,
    0x7E54AC3A, 0xBC75584F,
    0x30A1064A, 0x3FFBA5B0,
    0x0E54292E, 0xBC9EFCD3,
    0x904BC1D2, 0x3FFBCC1E,
    0x7A2D9E84, 0x3C823DD0,
    0x5BD71E09, 0x3FFBF2C2,
    0x3F6B9C72, 0xBC9EFDCA,
    0xDD85529C, 0x3FFC199B,
    0x895048DD, 0x3C811065,
    0x5FFFD07A, 0x3FFC40AB,
    0xE083C60A, 0x3C9B4537,
    0x2E57D14B, 0x3FFC67F1,
    0xFF483CAC, 0x3C92884D,
    0x9406E7B5, 0x3FFC8F6D,
    0x48805C44, 0x3C71ACBC,
    0xDCEF9069, 0x3FFCB720,
    0xD1E949DB, 0x3C7503CB,
    0x555DC3FA, 0x3FFCDF0B,
    0x53829D72, 0xBC8DD83B,
    0x4A07897C, 0x3FFD072D,
    0x43797A9C, 0xBC9CBC37,
    0x080D89F2, 0x3FFD2F87,
    0x719D8577, 0xBC9D487B,
    0xDCFBA487, 0x3FFD5818,
    0xD75B3706, 0x3C82ED02,
    0x16C98398, 0x3FFD80E3,
    0x8BEDDFE8, 0xBC911EC1,
    0x03DB3285, 0x3FFDA9E6,
    0x696DB532, 0x3C9C2300,
    0xF301B460, 0x3FFDD321,
    0x78F018C2, 0x3C92DA57,
    0x337B9B5F, 0x3FFDFC97,
    0x4F184B5B, 0xBC91A5CD,
    0x14F5A129, 0x3FFE2646,
    0x817A1496, 0xBC97B627,
    0xE78B3FF6, 0x3FFE502E,
    0x80A9CC8F, 0x3C839E89,
    0xFBC74C83, 0x3FFE7A51,
    0xCA0C8DE1, 0x3C92D522,
    0xA2A490DA, 0x3FFEA4AF,
    0x179C2893, 0xBC9E9C23,
    0x2D8E67F1, 0x3FFECF48,
    0xB411AD8C, 0xBC9C93F3,
    0xEE615A27, 0x3FFEFA1B,
    0x86A4B6B0, 0x3C9DC7F4,
    0x376BBA97, 0x3FFF252B,
    0xBF0D8E43, 0x3C93A1A5,
    0x5B6E4540, 0x3FFF5076,
    0x2DD8A18A, 0x3C99D3E1,
    0xAD9CBE14, 0x3FFF7BFD,
    0xD0063509, 0xBC9DBB12,
    0x819E90D8, 0x3FFFA7C1,
    0xF3A5931E, 0x3C874853,
    0x2B8F71F1, 0x3FFFD3C2,
    0x966579E7, 0x3C62EB74,

    0x966457E8, 0x3E79C3A6,
    0x46694107, 0xBFD62E43,
    0x62B6DEE1, 0x3FC47FD4,
    0x2A9012D8, 0xBFB55047,

    0xFEFA39EF, 0x3FE62E42,
    0xFF82C58F, 0x3FCEBFBD,
    0xD704A0C0, 0x3FAC6B08,
    0x6FBA4E77, 0x3F83B2AB,
    0xE78A6731, 0x3F55D87F,

    0x00000000, 0x7FE00000,
    0x00000000, 0x00100000,
    0x00000000, 0x00000000,
    0x00000000, 0x3FF00000,
    0x00000000, 0xBFF00000,

    0x00000000, 0x42C80000,

    0x40000000, 0x3FF71547,

    0x02000000, 0x41A00000,

    0x00000000, 0x4C700000,
    0x00000000, 0x33700000,
};

__attribute__((always_inline))
inline int __internal_dpow_la_cout (double *a, double *b, double *r)
{
    int nRet = 0;
    double dbVTmp1, dbVTmp2, dbVPHH, dbVPHL;
    double dX, dY, dR, dbAX, dbSignRes, dbX1, dbRcp1, dbL1Hi, dbL1Lo, dbX2, dbRcp2, dbL2Hi, dbL2Lo,
        dbX3, dbRcp3C, dbL3Hi, dbL3Lo, dbK, dbT, dbD, dbR1, dbCQ, dbRcpC, dbX1Hi, dbX1Lo,
        dbRcpCHi, dbRcpCLo, dbTmp1, dbE, dbT_CQHi, dbCQLo, dbR, dbLogPart3, dbLog2Poly,
        dbHH, dbHL, dbHLL, dbYHi, dbYLo, dbTmp2, dbTmp3, dbPH, dbPL, dbPLL, dbZ, dbExp2Poly, dbExp2PolyT, dbResLo, dbResHi, dbRes, dbTwoPowN, dbAY;
    int i, iEXB, iEYB, iSignX, iSignY, iYHi, iYLo, iYIsFinite, iEY, iYIsInt, iXIsFinite,
        iDenoExpAdd, iXHi, k, i1, i2, i3, iELogAX, iN, j, iERes, iSign, iIsSigZeroX, iIsSigZeroY;

    dX = ((*a) * ((__constant double *) __dpow_la_CoutTab)[853 + (0)]);
    dY = ((*b) * ((__constant double *) __dpow_la_CoutTab)[853 + (0)]);

    iEXB = ((((_iml_v2_dp_union_t *) & dX)->dwords.hi_dword >> 20) & 0x7FF);

    iEYB = ((((_iml_v2_dp_union_t *) & dY)->dwords.hi_dword >> 20) & 0x7FF);

    iSignX = (((_iml_v2_dp_union_t *) & dX)->dwords.hi_dword >> 31);

    iSignY = (((_iml_v2_dp_union_t *) & dY)->dwords.hi_dword >> 31);

    iIsSigZeroX = (((((_iml_v2_dp_union_t *) & dX)->dwords.hi_dword & 0x000FFFFF) == 0) && ((((_iml_v2_dp_union_t *) & dX)->dwords.lo_dword) == 0));
    iIsSigZeroY = (((((_iml_v2_dp_union_t *) & dY)->dwords.hi_dword & 0x000FFFFF) == 0) && ((((_iml_v2_dp_union_t *) & dY)->dwords.lo_dword) == 0));

    iYHi = (iEYB << 20) | (((_iml_v2_dp_union_t *) & dY)->dwords.hi_dword & 0x000FFFFF);

    iYLo = (((_iml_v2_dp_union_t *) & dY)->dwords.lo_dword);

    iYIsFinite = (((((_iml_v2_dp_union_t *) & dY)->dwords.hi_dword >> 20) & 0x7FF) != 0x7FF);

    if (iYHi | iYLo)
    {

        iEY = iEYB - 0x3FF;

        if ((0x3FF <= iEYB) && iYIsFinite)
        {

            if (iEY <= 20)
            {

                if (((iYHi << iEY) << 12) | iYLo)
                {

                    iYIsInt = 0;
                }
                else
                {

                    if ((iYHi << (iEY + 11)) & 0x80000000)
                    {
                        iYIsInt = 1;
                    }
                    else
                    {
                        iYIsInt = 2;
                    }
                }
            }
            else
            {
                if (iEY < 53)
                {

                    if ((iYLo << (iEY + 12 - 32 - 1)) << 1)
                    {

                        iYIsInt = 0;
                    }
                    else
                    {

                        if ((iYLo << (iEY + 12 - 32 - 1)) & 0x80000000)
                        {
                            iYIsInt = 1;
                        }
                        else
                        {
                            iYIsInt = 2;
                        }
                    }
                }
                else
                {

                    iYIsInt = 2;
                }
            }
        }
        else
        {

            iYIsInt = 0;
        }
    }
    else
    {

        iYIsInt = 2;

    }

    if (!((iSignX == 0) && (iEXB == 0x3FF) && iIsSigZeroX) && !((iEYB == 0) && iIsSigZeroY))
    {

        iXIsFinite = (((((_iml_v2_dp_union_t *) & dX)->dwords.hi_dword >> 20) & 0x7FF) != 0x7FF);

        if ((iXIsFinite || iIsSigZeroX) && (iYIsFinite || iIsSigZeroY))
        {

            if (dX != ((__constant double *) __dpow_la_CoutTab)[852])
            {

                if (!((dX == ((__constant double *) __dpow_la_CoutTab)[854]) && (iYIsInt || !iYIsFinite)))
                {

                    if (iXIsFinite && iYIsFinite)
                    {

                        if ((dX > ((__constant double *) __dpow_la_CoutTab)[852]) || iYIsInt)
                        {

                            dbSignRes = ((__constant double *) __dpow_la_CoutTab)[853 + (iSignX & iYIsInt)];
                            iDenoExpAdd = 0;
                            dbAX = dX;
                            (((_iml_v2_dp_union_t *) & dbAX)->dwords.hi_dword =
                             (((_iml_v2_dp_union_t *) & dbAX)->dwords.hi_dword & 0x7FFFFFFF) | ((_iml_uint32_t) (0) << 31));

                            if (iEXB == 0)
                            {

                                dbAX = dbAX * ((__constant double *) __dpow_la_CoutTab)[858];
                                iDenoExpAdd = iDenoExpAdd - 200;
                            }

                            dbX1 = dbAX;
                            (((_iml_v2_dp_union_t *) & dbX1)->dwords.hi_dword =
                             (((_iml_v2_dp_union_t *) & dbX1)->dwords.hi_dword & 0x800FFFFF) | (((_iml_uint32_t) (0x3FF) & 0x7FF) << 20));

                            iXHi = ((((_iml_v2_dp_union_t *) & dbAX)->dwords.hi_dword >> 20) & 0x7FF);
                            iXHi = iXHi << 20;
                            iXHi = iXHi | (((_iml_v2_dp_union_t *) & dbAX)->dwords.hi_dword & 0x000FFFFF);

                            k = iXHi - 0x3FE7C000;
                            k = k >> 20;
                            k = k + iDenoExpAdd;

                            i1 = (((_iml_v2_dp_union_t *) & dbX1)->dwords.hi_dword & 0x000FFFFF);
                            i1 = i1 & 0xFC000;
                            i1 = i1 + 0x4000;
                            i1 = i1 >> 15;

                            dbRcp1 = ((__constant double *) __dpow_la_CoutTab)[0 + i1];

                            dbL1Hi = ((__constant double *) __dpow_la_CoutTab)[33 + 2 * (i1) + 0];
                            dbL1Lo = ((__constant double *) __dpow_la_CoutTab)[33 + 2 * (i1) + 1];

                            dbX2 = dbX1 * dbRcp1;

                            i2 = (((_iml_v2_dp_union_t *) & dbX2)->dwords.hi_dword & 0x000FFFFF);
                            i2 = i2 & 0xFC00;
                            i2 = i2 + 0x400;
                            i2 = i2 >> 11;

                            dbRcp2 = ((__constant double *) __dpow_la_CoutTab)[99 + i2];

                            dbL2Hi = ((__constant double *) __dpow_la_CoutTab)[132 + 2 * (i2) + 0];
                            dbL2Lo = ((__constant double *) __dpow_la_CoutTab)[132 + 2 * (i2) + 1];

                            dbX3 = dbX2 * dbRcp2;

                            i3 = (((_iml_v2_dp_union_t *) & dbX3)->dwords.hi_dword & 0x000FFFFF);
                            i3 = i3 & 0xFF0;
                            i3 = i3 + 0x10;
                            i3 = i3 >> 5;

                            dbRcp3C = ((__constant double *) __dpow_la_CoutTab)[198 + i3];

                            dbL3Hi = ((__constant double *) __dpow_la_CoutTab)[327 + 2 * (i3) + 0];
                            dbL3Lo = ((__constant double *) __dpow_la_CoutTab)[327 + 2 * (i3) + 1];

                            dbK = (double) k;
                            dbT = (dbK + dbL1Hi);
                            dbT = (dbT + dbL2Hi);
                            dbT = (dbT + dbL3Hi);

                            dbD = (dbL2Lo + dbL3Lo);
                            dbD = (dbD + dbL1Lo);

                            dbR1 = (dbX3 * dbRcp3C);
                            dbCQ = (dbR1 - ((__constant double *) __dpow_la_CoutTab)[856]);

                            dbRcpC = (dbRcp1 * dbRcp2);
                            dbRcpC = (dbRcpC * dbRcp3C);

                            dbVTmp1 = ((dbX1) * (((__constant double *) __dpow_la_CoutTab)[857]));
                            dbVTmp2 = (dbVTmp1 - (dbX1));
                            dbVTmp1 = (dbVTmp1 - dbVTmp2);
                            dbVTmp2 = ((dbX1) - dbVTmp1);
                            dbX1Hi = dbVTmp1;
                            dbX1Lo = dbVTmp2;

                            dbVTmp1 = ((dbRcpC) * (((__constant double *) __dpow_la_CoutTab)[857]));
                            dbVTmp2 = (dbVTmp1 - (dbRcpC));
                            dbVTmp1 = (dbVTmp1 - dbVTmp2);
                            dbVTmp2 = ((dbRcpC) - dbVTmp1);
                            dbRcpCHi = dbVTmp1;
                            dbRcpCLo = dbVTmp2;

                            dbTmp1 = (dbX1Hi * dbRcpCHi);
                            dbE = (dbTmp1 - dbR1);
                            dbTmp1 = (dbX1Lo * dbRcpCHi);
                            dbE = (dbE + dbTmp1);
                            dbTmp1 = (dbX1Hi * dbRcpCLo);
                            dbE = (dbE + dbTmp1);
                            dbTmp1 = (dbX1Lo * dbRcpCLo);
                            dbE = (dbE + dbTmp1);

                            dbVTmp1 = ((dbT) + (dbCQ));
                            dbTmp1 = ((dbT) - dbVTmp1);
                            dbVTmp2 = (dbTmp1 + (dbCQ));
                            dbT_CQHi = dbVTmp1;
                            dbCQLo = dbVTmp2;

                            iELogAX = ((((_iml_v2_dp_union_t *) & dbT_CQHi)->dwords.hi_dword >> 20) & 0x7FF);

                            if (iELogAX + iEYB < 11 + 2 * 0x3FF)
                            {

                                if (iELogAX + iEYB > -62 + 2 * 0x3FF)
                                {

                                    dbR = (dbCQ + dbE);

                                    dbLog2Poly =
                                        ((((((__constant double *) __dpow_la_CoutTab)[844]) * dbR +
                                           ((__constant double *) __dpow_la_CoutTab)[843]) * dbR +
                                          ((__constant double *) __dpow_la_CoutTab)[842]) * dbR +
                                         ((__constant double *) __dpow_la_CoutTab)[841]) * dbR;

                                    dbLogPart3 = (dbCQLo + dbE);
                                    dbLogPart3 = (dbD + dbLogPart3);

                                    dbVTmp1 = ((dbT_CQHi) + (dbLog2Poly));
                                    dbTmp1 = ((dbT_CQHi) - dbVTmp1);
                                    dbVTmp2 = (dbTmp1 + (dbLog2Poly));
                                    dbHH = dbVTmp1;
                                    dbHL = dbVTmp2;

                                    dbVTmp1 = ((dbHH) + (dbLogPart3));
                                    dbTmp1 = ((dbHH) - dbVTmp1);
                                    dbVTmp2 = (dbTmp1 + (dbLogPart3));
                                    dbHH = dbVTmp1;
                                    dbHLL = dbVTmp2;

                                    dbHLL = (dbHLL + dbHL);

                                    dbVTmp1 = ((dbHH) * (((__constant double *) __dpow_la_CoutTab)[857]));
                                    dbVTmp2 = (dbVTmp1 - (dbHH));
                                    dbVTmp1 = (dbVTmp1 - dbVTmp2);
                                    dbVTmp2 = ((dbHH) - dbVTmp1);
                                    dbHH = dbVTmp1;
                                    dbHL = dbVTmp2;

                                    dbVTmp1 = ((dY) * (((__constant double *) __dpow_la_CoutTab)[857]));
                                    dbVTmp2 = (dbVTmp1 - (dY));
                                    dbVTmp1 = (dbVTmp1 - dbVTmp2);
                                    dbVTmp2 = ((dY) - dbVTmp1);
                                    dbYHi = dbVTmp1;
                                    dbYLo = dbVTmp2;

                                    dbTmp1 = ((dbYHi) * (dbHH));
                                    dbTmp2 = ((dbYLo) * (dbHL));
                                    dbTmp2 = (dbTmp2 + (dbYHi) * (dbHL));
                                    dbTmp3 = (dbTmp2 + (dbYLo) * (dbHH));
                                    dbPH = dbTmp1;
                                    dbPL = dbTmp3;

                                    dbPLL = (dY * dbHLL);

                                    dbVTmp1 = (dbPH + ((__constant double *) __dpow_la_CoutTab)[855]);
                                    iN = (((_iml_v2_dp_union_t *) & dbVTmp1)->dwords.lo_dword);
                                    j = iN & 0x7F;
                                    iN = iN >> 7;
                                    dbVPHH = (dbVTmp1 - ((__constant double *) __dpow_la_CoutTab)[855]);
                                    dbVPHL = (dbPH - dbVPHH);

                                    dbZ = (dbPLL + dbPL);
                                    dbZ = (dbZ + dbVPHL);

                                    dbExp2Poly =
                                        (((((((__constant double *) __dpow_la_CoutTab)[849]) * dbZ +
                                            ((__constant double *) __dpow_la_CoutTab)[848]) * dbZ +
                                           ((__constant double *) __dpow_la_CoutTab)[847]) * dbZ +
                                          ((__constant double *) __dpow_la_CoutTab)[846]) * dbZ +
                                         ((__constant double *) __dpow_la_CoutTab)[845]) * dbZ;

                                    dbExp2PolyT = (dbExp2Poly * ((__constant double *) __dpow_la_CoutTab)[585 + 2 * (j) + 0]);
                                    dbResLo = (dbExp2PolyT + ((__constant double *) __dpow_la_CoutTab)[585 + 2 * (j) + 1]);
                                    dbResHi = ((__constant double *) __dpow_la_CoutTab)[585 + 2 * (j) + 0];

                                    dbRes = (dbResHi + dbResLo);
                                    iERes = ((((_iml_v2_dp_union_t *) & dbRes)->dwords.hi_dword >> 20) & 0x7FF) - 0x3FF;
                                    iERes = (iERes + iN);

                                    if (iERes < 1024)
                                    {
                                        if (iERes >= -1022)
                                        {

                                            (((_iml_v2_dp_union_t *) & dbRes)->dwords.hi_dword =
                                             (((_iml_v2_dp_union_t *) & dbRes)->dwords.
                                              hi_dword & 0x800FFFFF) | (((_iml_uint32_t) (iERes + 0x3FF) & 0x7FF) << 20));

                                            dbRes = dbRes * dbSignRes;
                                            dR = dbRes;
                                        }
                                        else
                                        {

                                            if (iERes >= -1022 - 10)
                                            {
                                                dbVTmp1 = ((dbResHi) + (dbResLo));
                                                dbTmp1 = ((dbResHi) - dbVTmp1);
                                                dbVTmp2 = (dbTmp1 + (dbResLo));
                                                dbResHi = dbVTmp1;
                                                dbResLo = dbVTmp2;

                                                dbVTmp1 = ((dbResHi) * (((__constant double *) __dpow_la_CoutTab)[857]));
                                                dbVTmp2 = (dbVTmp1 - (dbResHi));
                                                dbVTmp1 = (dbVTmp1 - dbVTmp2);
                                                dbVTmp2 = ((dbResHi) - dbVTmp1);
                                                dbResHi = dbVTmp1;
                                                dbTmp2 = dbVTmp2;

                                                dbResLo = (dbResLo + dbTmp2);

                                                dbSignRes *= ((__constant double *) __dpow_la_CoutTab)[859];
                                                iN = (iN + 200);

                                                dbTwoPowN = ((__constant double *) __dpow_la_CoutTab)[853];
                                                (((_iml_v2_dp_union_t *) & dbTwoPowN)->dwords.hi_dword =
                                                 (((_iml_v2_dp_union_t *) & dbTwoPowN)->dwords.
                                                  hi_dword & 0x800FFFFF) | (((_iml_uint32_t) (iN + 0x3FF) & 0x7FF) << 20));

                                                dbResHi = (dbResHi * dbTwoPowN);

                                                dbResLo = (dbResLo * dbTwoPowN);

                                                dbRes = (dbResHi + dbResLo);
                                                dbRes = (dbRes * dbSignRes);

                                                dbVTmp1 = ((__constant double *) __dpow_la_CoutTab)[851];
                                                dbVTmp1 = (dbVTmp1 * dbVTmp1);
                                                dbRes = (dbRes + dbVTmp1);

                                                dR = dbRes;
                                            }
                                            else
                                            {
                                                if (iERes >= -1074 - 10)
                                                {
                                                    dbSignRes *= ((__constant double *) __dpow_la_CoutTab)[859];
                                                    iN = iN + 200;

                                                    dbTwoPowN = ((__constant double *) __dpow_la_CoutTab)[853];
                                                    (((_iml_v2_dp_union_t *) & dbTwoPowN)->dwords.hi_dword =
                                                     (((_iml_v2_dp_union_t *) & dbTwoPowN)->dwords.
                                                      hi_dword & 0x800FFFFF) | (((_iml_uint32_t) (iN + 0x3FF) & 0x7FF) << 20));

                                                    dbRes = (dbRes * dbTwoPowN);
                                                    dbRes = (dbRes * dbSignRes);

                                                    dbVTmp1 = ((__constant double *) __dpow_la_CoutTab)[851];
                                                    dbVTmp1 *= dbVTmp1;
                                                    dbRes = (dbRes + dbVTmp1);

                                                    dR = dbRes;
                                                }
                                                else
                                                {

                                                    dbVTmp1 = ((__constant double *) __dpow_la_CoutTab)[851];
                                                    dbVTmp1 *= dbVTmp1;
                                                    dbRes = (dbVTmp1 * dbSignRes);
                                                    dR = dbRes;
                                                }
                                            }
                                        }
                                    }
                                    else
                                    {

                                        dbVTmp1 = ((__constant double *) __dpow_la_CoutTab)[850];
                                        dbVTmp1 = (dbVTmp1 * dbVTmp1);
                                        dbRes = (dbVTmp1 * dbSignRes);
                                        dR = dbRes;

                                    }
                                }
                                else
                                {

                                    dbVTmp1 = ((__constant double *) __dpow_la_CoutTab)[853];
                                    dbVTmp1 = (dbVTmp1 + ((__constant double *) __dpow_la_CoutTab)[851]);
                                    dR = (dbVTmp1 * dbSignRes);
                                }
                            }
                            else
                            {

                                iSign = iSignY ^ (((_iml_v2_dp_union_t *) & dbT_CQHi)->dwords.hi_dword >> 31);

                                dbTmp1 = ((__constant double *) __dpow_la_CoutTab)[850 + (iSign)];

                                dbTmp1 = (dbTmp1 * dbTmp1);

                                dbTmp1 = (dbTmp1 * dbSignRes);
                                dR = dbTmp1;

                                if ((!(((((_iml_v2_dp_union_t *) & dR)->dwords.hi_dword >> 20) & 0x7FF) != 0x7FF)
                                     && (((((_iml_v2_dp_union_t *) & dR)->dwords.hi_dword & 0x000FFFFF) == 0)
                                         && ((((_iml_v2_dp_union_t *) & dR)->dwords.lo_dword) == 0))))
                                {
                                    nRet = 3;
                                }

                            }
                        }
                        else
                        {

                            dbVTmp1 = ((__constant double *) __dpow_la_CoutTab)[852];
                            dbVTmp1 = dbVTmp1 / dbVTmp1;
                            dR = dbVTmp1;
                            nRet = 1;
                        }
                    }
                    else
                    {

                        if (iEXB < 0x3FF)
                        {

                            if (iSignY)
                            {

                                dR = dY * dY;
                            }
                            else
                            {

                                dR = ((__constant double *) __dpow_la_CoutTab)[852];
                            }
                        }
                        else
                        {

                            if (iSignY)
                            {

                                dR = ((__constant double *) __dpow_la_CoutTab)[852] * ((__constant double *) __dpow_la_CoutTab)[853 +
                                                                                                                                (iYIsInt & iSignX)];
                            }
                            else
                            {

                                dbTmp1 = dX * dX;
                                dbTmp1 = dbTmp1 * dY;
                                dR = dbTmp1 * ((__constant double *) __dpow_la_CoutTab)[853 + (iYIsInt & iSignX)];
                            }
                        }
                    }
                }
                else
                {

                    dR = ((__constant double *) __dpow_la_CoutTab)[853 + (iYIsInt & 1)];
                }
            }
            else
            {

                dbTmp1 = dX * dX;

                if (iSignY)
                {

                    dR = ((__constant double *) __dpow_la_CoutTab)[853 + (iYIsInt & iSignX)] / dbTmp1;
                    nRet = 1;
                }
                else
                {

                    dR = ((__constant double *) __dpow_la_CoutTab)[853 + (iYIsInt & iSignX)] * dbTmp1;
                }
            }
        }
        else
        {

            dR = dX + dY;
        }
    }
    else
    {

        dbVTmp1 = dX + dY;
        iSign = (((_iml_v2_dp_union_t *) & dbVTmp1)->dwords.hi_dword >> 31);
        dbVTmp2 = ((__constant double *) __dpow_la_CoutTab)[853];
        (((_iml_v2_dp_union_t *) & dbVTmp2)->dwords.hi_dword =
         (((_iml_v2_dp_union_t *) & dbVTmp2)->dwords.hi_dword & 0x7FFFFFFF) | ((_iml_uint32_t) (iSign) << 31));

        dR = dbVTmp2 * dbVTmp2;
    }

    *r = dR;
    return nRet;
}

double __ocl_svml_pow (double a, double b)
{

    double va1;
    double va2;
    double vr1;
    unsigned int vm;

    double r;

    va1 = a;
    va2 = b;

    {

        unsigned int _HIDELTA;
        unsigned int _LORANGE;
        unsigned int _ABSMASK;
        unsigned int _INF;
        unsigned int _DOMAINRANGE;
        unsigned long lX;
        unsigned long lY;
        unsigned long lZ;
        unsigned int iXHi;
        unsigned int iYHi;
        unsigned int iZ;
        unsigned int iSpecX;
        unsigned int iSpecY;
        unsigned int iSpecZ;
        unsigned int iAbsZ;
        unsigned int iRangeMask;
        double dMantissaMask;
        double dX1;
        double dOne;
        unsigned int iIndex;
        unsigned int iIndexMask;
        unsigned int iIndexAdd;
        unsigned int iIndexR;
        unsigned int iIndexL;
        double dRcp;
        double dL1[2];
        unsigned int iK;
        unsigned int i3fe7fe00;
        unsigned int i2p20_2p19;
        unsigned long lK;
        double dK;
        double dHighHalfMask;
        double db2p20_2p19;
        double dX1Hi;
        double dX1Lo;
        double dHighMask;
        double dR1;
        double LHN;
        double dCq;
        double dE;
        double dT;
        double dTRh;
        double dRl;
        double dTRhEh;
        double dHLL;
        double dA[8];
        double dPoly;
        double dTRhEhHLLhi;
        double dHLLHi;
        double dHH;
        double dHL;

        double dYHi;
        double dYLo;
        double dPH;
        double dPL;
        double dPLL;
        double dPHH;
        double dPHL;
        double db2p45_2p44;
        unsigned long lPHH;
        unsigned int iPHH;
        unsigned int iN;
        unsigned int iOne;
        unsigned long lN;
        double dN;
        double dExpSignMask;

        unsigned int jIndex;
        unsigned int jIndexMask;
        double dExpT[2];
        double dT2;
        double dT4;
        double dB[6];
        double dRes;
        double dTmp;
        double dEN;
        double dTEN;

        lX = as_ulong (va1);
        lY = as_ulong (va2);
        iXHi = ((unsigned int) ((unsigned long) lX >> 32));
        iYHi = ((unsigned int) ((unsigned long) lY >> 32));

        _HIDELTA = (__internal_dpow_la_data.HIDELTA);
        _LORANGE = (__internal_dpow_la_data.LORANGE);
        _ABSMASK = (__internal_dpow_la_data.ABSMASK);
        _INF = (__internal_dpow_la_data.INF);

        iSpecX = (iXHi + _HIDELTA);
        iSpecX = ((unsigned int) (-(signed int) ((signed int) iSpecX < (signed int) _LORANGE)));

        iSpecY = (iYHi & _ABSMASK);
        iSpecY = ((unsigned int) (-(signed int) ((signed int) iSpecY >= (signed int) _INF)));
        iRangeMask = (iSpecX | iSpecY);

        dMantissaMask = as_double (__internal_dpow_la_data.iMantissaMask);

        dX1 = as_double ((as_ulong (va1) & as_ulong (dMantissaMask)));
        dOne = as_double (__internal_dpow_la_data.dbOne);
        dX1 = as_double ((as_ulong (dX1) | as_ulong (dOne)));

        iIndex = ((unsigned int) ((unsigned long) lX >> 32));
        iIndexMask = (__internal_dpow_la_data.iIndexMask);

        iIndex = (iIndex & iIndexMask);
        iIndexAdd = (__internal_dpow_la_data.iIndexAdd);
        iIndex = (iIndex + iIndexAdd);

        iIndex = ((unsigned int) (iIndex) >> (10));

        iIndexR = ((unsigned int) (iIndex) << (3));

        iIndexL = ((unsigned int) (iIndex) << (4));

        dRcp = as_double (((__constant unsigned long *) (__internal_dpow_la_data.rcp_t1))[iIndexR >> 3]);
        dL1[0] = as_double (((__constant unsigned long *) (__internal_dpow_la_data.log2_t1))[iIndexL >> 3]);
        dL1[1] = as_double (((__constant unsigned long *) (__internal_dpow_la_data.log2_t1))[(iIndexL >> 3) + 1]);

        i3fe7fe00 = (__internal_dpow_la_data.i3fe7fe00);
        iK = (iXHi - i3fe7fe00);
        iK = ((signed int) iK >> (20));
        i2p20_2p19 = (__internal_dpow_la_data.i2p20_2p19);
        iK = (iK + i2p20_2p19);
        lK = (((unsigned long) (unsigned int) iK << 32) | (unsigned long) (unsigned int) iK);
        dK = as_double (lK);
        dHighHalfMask = as_double (__internal_dpow_la_data.iffffffff00000000);
        dK = as_double ((as_ulong (dK) & as_ulong (dHighHalfMask)));
        db2p20_2p19 = as_double (__internal_dpow_la_data.db2p20_2p19);
        dK = (dK - db2p20_2p19);

        dHighMask = as_double (__internal_dpow_la_data.iHighMask);

        dX1Hi = as_double ((as_ulong (dX1) & as_ulong (dHighMask)));
        dX1Lo = (dX1 - dX1Hi);

        dR1 = (dX1 * dRcp);

        LHN = as_double (__internal_dpow_la_data.LHN);
        dCq = (dR1 + LHN);

        dE = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dX1Hi, dRcp, -(dR1));

        dE = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dX1Lo, dRcp, dE);

        dT = (dK + dL1[0]);

        dTRh = (dT + dCq);

        dRl = (dT - dTRh);

        dRl = (dRl + dCq);

        dTRhEh = (dTRh + dE);

        dHLL = (dTRh - dTRhEh);

        dHLL = (dHLL + dE);

        dHLL = (dHLL + dRl);

        dHLL = (dHLL + dL1[1]);

        dCq = (dCq + dE);

        dA[6] = as_double (__internal_dpow_la_data.clv[1]);
        dA[5] = as_double (__internal_dpow_la_data.clv[2]);

        dPoly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dA[6], dCq, dA[5]);
        dA[4] = as_double (__internal_dpow_la_data.clv[3]);
        dPoly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dPoly, dCq, dA[4]);
        dA[3] = as_double (__internal_dpow_la_data.clv[4]);
        dPoly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dPoly, dCq, dA[3]);
        dA[2] = as_double (__internal_dpow_la_data.clv[5]);
        dPoly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dPoly, dCq, dA[2]);
        dA[1] = as_double (__internal_dpow_la_data.clv[6]);
        dPoly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dPoly, dCq, dA[1]);
        dHLL = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dPoly, dCq, dHLL);

        dTRhEhHLLhi = (dTRhEh + dHLL);

        dHLLHi = (dTRhEhHLLhi - dTRhEh);

        dHLL = (dHLL - dHLLHi);

        dHH = as_double ((as_ulong (dTRhEhHLLhi) & as_ulong (dHighMask)));

        dHL = (dTRhEhHLLhi - dHH);

        dYHi = as_double ((as_ulong (va2) & as_ulong (dHighMask)));

        dYLo = (va2 - dYHi);

        dPH = (dYHi * dHH);

        lZ = as_ulong (dPH);
        iZ = ((unsigned int) ((unsigned long) lZ >> 32));
        iAbsZ = (iZ & _ABSMASK);
        _DOMAINRANGE = (__internal_dpow_la_data.DOMAINRANGE);
        iSpecZ = ((unsigned int) (-(signed int) ((signed int) iAbsZ >= (signed int) _DOMAINRANGE)));
        iRangeMask = (iRangeMask | iSpecZ);
        vm = 0;
        vm = iRangeMask;

        dPL = (dYLo * dHL);
        dPL = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dYHi, dHL, dPL);
        dPL = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dYLo, dHH, dPL);

        dPLL = (va2 * dHLL);
        db2p45_2p44 = as_double (__internal_dpow_la_data.db2p45_2p44);

        dPHH = (dPH + db2p45_2p44);
        lPHH = as_ulong (dPHH);
        iPHH = (((unsigned int) lPHH & (unsigned int) -1));

        jIndexMask = (__internal_dpow_la_data.jIndexMask);

        jIndex = (iPHH & jIndexMask);
        jIndex = ((unsigned int) (jIndex) << (4));

        iN = ((unsigned int) (iPHH) << (13));
        iOne = (__internal_dpow_la_data.iOne);
        iN = (iN + iOne);
        lN = (((unsigned long) (unsigned int) iN << 32) | (unsigned long) (unsigned int) iN);
        dN = as_double (lN);
        dExpSignMask = as_double (__internal_dpow_la_data.ifff0000000000000);
        dN = as_double ((as_ulong (dN) & as_ulong (dExpSignMask)));
        dExpT[0] = as_double (((__constant unsigned long *) (__internal_dpow_la_data.exp2_tbl))[jIndex >> 3]);

        dPHH = (dPHH - db2p45_2p44);

        dPHL = (dPH - dPHH);

        dT = (dPL + dPLL);
        dT = (dT + dPHL);

        dEN = (dN * dExpT[0]);
        dTEN = (dEN * dT);

        dB[5] = as_double (__internal_dpow_la_data.cev[0]);
        dB[4] = as_double (__internal_dpow_la_data.cev[1]);
        dRes = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dB[5], dT, dB[4]);
        dB[3] = as_double (__internal_dpow_la_data.cev[2]);
        dRes = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dRes, dT, dB[3]);
        dB[2] = as_double (__internal_dpow_la_data.cev[3]);
        dRes = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dRes, dT, dB[2]);
        dB[1] = as_double (__internal_dpow_la_data.cev[4]);
        dRes = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dRes, dT, dB[1]);
        vr1 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dRes, dTEN, dEN);
    }

    if ((vm) != 0)
    {
        double _vapi_arg1[1];
        double _vapi_arg2[1];
        double _vapi_res1[1];
        ((double *) _vapi_arg1)[0] = va1;
        ((double *) _vapi_arg2)[0] = va2;
        ((double *) _vapi_res1)[0] = vr1;
        __internal_dpow_la_cout (_vapi_arg1, _vapi_arg2, _vapi_res1);
        vr1 = ((double *) _vapi_res1)[0];
    };
    r = vr1;;

    return r;

}

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
    unsigned long L2;
    unsigned long Zero;
    unsigned long IndexMask;
    unsigned long MinNorm;
    unsigned long MaxNorm;
} __internal_dlog10_la_data_avx512_t;
static __constant __internal_dlog10_la_data_avx512_t __internal_dlog10_la_data_avx512 = {
    {
     0x0000000000000000uL, 0xbf9af5f92b00e610uL, 0xbfaa30a9d609efeauL, 0xbfb31b3055c47118uL, 0xbfb8cf183886480duL, 0xbfbe3bc1ab0e19feuL,
     0xbfc1b3e71ec94f7buL, 0xbfc42c7e7fe3fc02uL, 0x3fbffbfc2bbc7803uL, 0x3fbb721cd17157e3uL, 0x3fb715d0ce367afcuL, 0x3fb2e3a740b7800fuL,
     0x3fadb11ed766abf4uL, 0x3fa5e3966b7e9295uL, 0x3f9cb38fccd8bfdbuL, 0x3f8c3d0837784c41uL}

    , 0x3ff0000000000000uL, 0x3fe8000000000000uL, 0x3fa8c2d828480370uL, 0xbfabd80d96029814uL, 0x3fafc3f6f38b58a2uL, 0xbfb287a63464dc80uL,
        0x3fb63c62777f27d9uL, 0xbfbbcb7b153c06a3uL, 0x3fc287a7636f428cuL, 0xbfcbcb7b1526e4dbuL, 0x3fdbcb7b1526e50euL, 0x3fd34413509f79ffuL,
        0x0000000000000000uL, 0x0000000000000078uL, 0x0010000000000000uL, 0x7fefffffffffffffuL
};

typedef struct
{
    unsigned long Log_HA_table[(1 << 9) + 2];
    unsigned long Log_LA_table[(1 << 9) + 1];
    unsigned long ha_poly_coeff[6];
    unsigned long poly_coeff[5];
    unsigned long ExpMask;
    unsigned long Two10;
    unsigned long MinNorm;
    unsigned long MaxNorm;
    unsigned long HalfMask;
    unsigned long CLH;
    unsigned long One;
    unsigned long L2H;
    unsigned long L2L;
    unsigned long Threshold;
    unsigned long Bias;
    unsigned long Bias1;
    unsigned long L2;

    unsigned long dInfs[2];
    unsigned long dOnes[2];
    unsigned long dZeros[2];
} __internal_dlog10_la_data_t;
static __constant __internal_dlog10_la_data_t __internal_dlog10_la_data = {

    {
     0xc0733a7146f6b080uL, 0xbe1e707ce619c200uL, 0xc0733a7547771970uL, 0xbe1e79c6c06d6f51uL, 0xc0733a7945aacb70uL, 0xbe1e78e225fad29cuL,
     0xc0733a7d41946970uL, 0xbe1e76d607f9693buL, 0xc0733a813b3691f0uL, 0xbe1e7704b3e0685buL, 0xc0733a853293df00uL, 0xbe1e79c1216a27fauL,
     0xc0733a8927aee660uL, 0xbe1e76dce5734a81uL, 0xc0733a8d1a8a3920uL, 0xbe1e782ee2ca4dbauL, 0xc0733a910b286430uL, 0xbe1e7812d1a0a61fuL,
     0xc0733a94f98bf010uL, 0xbe1e77e1b5ecbc61uL, 0xc0733a98e5b76100uL, 0xbe1e76635cac1586uL, 0xc0733a9ccfad36f0uL, 0xbe1e7638f7968f32uL,
     0xc0733aa0b76feda0uL, 0xbe1e7840ee76e365uL, 0xc0733aa49d01fcb0uL, 0xbe1e79f3fd01907euL, 0xc0733aa88065d7a0uL, 0xbe1e77bbb3a9c38auL,
     0xc0733aac619dedb0uL, 0xbe1e7742719bf41duL, 0xc0733ab040acaa20uL, 0xbe1e79bcedaf79cbuL, 0xc0733ab41d947450uL, 0xbe1e762d63cb7ca0uL,
     0xc0733ab7f857af50uL, 0xbe1e77a07be83403uL, 0xc0733abbd0f8ba80uL, 0xbe1e7763ff836ad0uL, 0xc0733abfa779f130uL, 0xbe1e7737720ead39uL,
     0xc0733ac37bddaad0uL, 0xbe1e7776a08e55e7uL, 0xc0733ac74e263af0uL, 0xbe1e793e3c52dd36uL, 0xc0733acb1e55f160uL, 0xbe1e788a94695051uL,
     0xc0733aceec6f1a10uL, 0xbe1e76508114a813uL, 0xc0733ad2b873fd20uL, 0xbe1e76909457d23euL, 0xc0733ad68266df10uL, 0xbe1e7664a24f9ca4uL,
     0xc0733ada4a4a0090uL, 0xbe1e7a07b3d44b18uL, 0xc0733ade101f9ee0uL, 0xbe1e76d87594704duL, 0xc0733ae1d3e9f340uL, 0xbe1e79563595a182uL,
     0xc0733ae595ab33b0uL, 0xbe1e771880c3c6abuL, 0xc0733ae955659250uL, 0xbe1e78c171f517d4uL, 0xc0733aed131b3df0uL, 0xbe1e77eac3874666uL,
     0xc0733af0cece61b0uL, 0xbe1e790db479d8f6uL, 0xc0733af488812550uL, 0xbe1e7965d1aa5c90uL, 0xc0733af84035ad10uL, 0xbe1e78ceb398ba47uL,
     0xc0733afbf5ee19c0uL, 0xbe1e779cc0dcb5aauL, 0xc0733affa9ac88c0uL, 0xbe1e7871053953eduL, 0xc0733b035b731420uL, 0xbe1e7a082cffa71auL,
     0xc0733b070b43d2a0uL, 0xbe1e7904b4382faduL, 0xc0733b0ab920d790uL, 0xbe1e79b458d0b4f3uL, 0xc0733b0e650c3310uL, 0xbe1e79d0ded414c6uL,
     0xc0733b120f07f200uL, 0xbe1e763c357a1943uL, 0xc0733b15b7161dd0uL, 0xbe1e78b80ba6daaauL, 0xc0733b195d38bd00uL, 0xbe1e7998e23b8ffduL,
     0xc0733b1d0171d2c0uL, 0xbe1e7974aa65ee8cuL, 0xc0733b20a3c35f20uL, 0xbe1e76ccfde752abuL, 0xc0733b24442f5ef0uL, 0xbe1e77b4ff19debbuL,
     0xc0733b27e2b7cc10uL, 0xbe1e7772ee478542uL, 0xc0733b2b7f5e9d30uL, 0xbe1e781d81b58b44uL, 0xc0733b2f1a25c600uL, 0xbe1e78350d967565uL,
     0xc0733b32b30f3720uL, 0xbe1e783888e48152uL, 0xc0733b364a1cde30uL, 0xbe1e78367bf7c111uL, 0xc0733b39df50a5d0uL, 0xbe1e7959e57ca47duL,
     0xc0733b3d72ac75c0uL, 0xbe1e777322423222uL, 0xc0733b41043232b0uL, 0xbe1e767ce42a60aauL, 0xc0733b4493e3be70uL, 0xbe1e781d445aea19uL,
     0xc0733b4821c2f800uL, 0xbe1e7922fca18e18uL, 0xc0733b4badd1bb80uL, 0xbe1e76fed3d40647uL, 0xc0733b4f3811e210uL, 0xbe1e793948c9eabcuL,
     0xc0733b52c0854240uL, 0xbe1e76e487656b8cuL, 0xc0733b56472daf90uL, 0xbe1e780ab2f71223uL, 0xc0733b59cc0cfaf0uL, 0xbe1e77189120b09cuL,
     0xc0733b5d4f24f270uL, 0xbe1e7644a0343a12uL, 0xc0733b60d0776160uL, 0xbe1e78f2a3e4733duL, 0xc0733b6450061080uL, 0xbe1e7913b2f73ae5uL,
     0xc0733b67cdd2c5c0uL, 0xbe1e7882d08393b5uL, 0xc0733b6b49df4470uL, 0xbe1e765e1b209979uL, 0xc0733b6ec42d4d20uL, 0xbe1e785c9c4620d4uL,
     0xc0733b75b394f240uL, 0xbe1e78878cd0e956uL, 0xc0733b7c9c178630uL, 0xbe1e789a4112d90buL, 0xc0733b837dc2b0f0uL, 0xbe1e79050b8a1766uL,
     0xc0733b8a58a3f220uL, 0xbe1e7790dffc47aauL, 0xc0733b912cc8a180uL, 0xbe1e77174593b06auL, 0xc0733b97fa3defb0uL, 0xbe1e7677de2d2eccuL,
     0xc0733b9ec110e6b0uL, 0xbe1e76cff477ca18uL, 0xc0733ba5814e6a80uL, 0xbe1e78f8644dec7buL, 0xc0733bac3b0339d0uL, 0xbe1e764e1361788duL,
     0xc0733bb2ee3bee30uL, 0xbe1e78c913e738deuL, 0xc0733bb99b04fd30uL, 0xbe1e76666f5bddaauL, 0xc0733bc0416ab850uL, 0xbe1e77e87cbd8ab6uL,
     0xc0733bc6e1794e10uL, 0xbe1e76f18ba1c966uL, 0xc0733bcd7b3cca10uL, 0xbe1e777c9461b8dbuL, 0xc0733bd40ec115d0uL, 0xbe1e78b78526ffacuL,
     0xc0733bda9c11f920uL, 0xbe1e7942abecfedeuL, 0xc0733be1233b1aa0uL, 0xbe1e76d8a684fd8cuL, 0xc0733be7a4480010uL, 0xbe1e79622b539ac9uL,
     0xc0733bee1f440f30uL, 0xbe1e7978e7cc20eauL, 0xc0733bf4943a8de0uL, 0xbe1e765c9c9de825uL, 0xc0733bfb0336a290uL, 0xbe1e775d8b138ee2uL,
     0xc0733c016c435500uL, 0xbe1e78bf33465c2fuL, 0xc0733c07cf6b8e80uL, 0xbe1e78164f7cc441uL, 0xc0733c0e2cba1a50uL, 0xbe1e7824e64d0b23uL,
     0xc0733c148439a630uL, 0xbe1e78373ae7dd81uL, 0xc0733c1ad5f4c2c0uL, 0xbe1e7704513e0afeuL, 0xc0733c2121f5e3d0uL, 0xbe1e7914aa84200fuL,
     0xc0733c2768476110uL, 0xbe1e76b1cde25cf6uL, 0xc0733c2da8f37600uL, 0xbe1e796120e3862duL, 0xc0733c33e40442e0uL, 0xbe1e78ec836d7e7buL,
     0xc0733c3a1983cca0uL, 0xbe1e77fb13b7dabbuL, 0xc0733c40497bfd70uL, 0xbe1e783c6fcb2404uL, 0xc0733c4673f6a530uL, 0xbe1e7628bb93dce8uL,
     0xc0733c4c98fd7990uL, 0xbe1e7857a47b5001uL, 0xc0733c52b89a16d0uL, 0xbe1e76708dc2831fuL, 0xc0733c58d2d5ffa0uL, 0xbe1e77b6038651f1uL,
     0xc0733c5ee7ba9de0uL, 0xbe1e792e855bb5b2uL, 0xc0733c64f75142d0uL, 0xbe1e776cacd5c105uL, 0xc0733c6b01a32740uL, 0xbe1e77f8a8011315uL,
     0xc0733c7106b96c30uL, 0xbe1e765cf3efcfdeuL, 0xc0733c77069d1ad0uL, 0xbe1e78d837d2efacuL, 0xc0733c7d01572530uL, 0xbe1e78b615cf772cuL,
     0xc0733c82f6f06640uL, 0xbe1e7650bbbd7a25uL, 0xc0733c88e771a220uL, 0xbe1e78bcf3495872uL, 0xc0733c8ed2e386c0uL, 0xbe1e792266832e84uL,
     0xc0733c94b94eabd0uL, 0xbe1e79c1c3c2ca52uL, 0xc0733c9a9abb9340uL, 0xbe1e78aa61e5807duL, 0xc0733ca07732a970uL, 0xbe1e7620fc4cf156uL,
     0xc0733ca64ebc4570uL, 0xbe1e76b914a832c5uL, 0xc0733cac2160a970uL, 0xbe1e79227f72020euL, 0xc0733cb1ef280300uL, 0xbe1e77ac972cc008uL,
     0xc0733cb7b81a6b10uL, 0xbe1e798089be41f4uL, 0xc0733cbd7c3fe6a0uL, 0xbe1e77942ae037feuL, 0xc0733cc33ba06690uL, 0xbe1e7956ae6463d9uL,
     0xc0733cc8f643c850uL, 0xbe1e7918a50c7942uL, 0xc0733cceac31d5d0uL, 0xbe1e78308eeab604uL, 0xc0733cd45d7245e0uL, 0xbe1e76dd4ea88445uL,
     0xc0733cda0a0cbc60uL, 0xbe1e77e7c1aa5909uL, 0xc0733cdfb208caa0uL, 0xbe1e7804b9d20e54uL, 0xc0733ce5556def70uL, 0xbe1e78f88e99d49cuL,
     0xc0733ceaf4439780uL, 0xbe1e787d74682d68uL, 0xc0733cf08e911d80uL, 0xbe1e76edc24fe6e7uL, 0xc0733cf6245dca50uL, 0xbe1e79b347ec86d2uL,
     0xc0733cfbb5b0d580uL, 0xbe1e797cceb2c39buL, 0xc0733d0142916530uL, 0xbe1e783adbdc6aa1uL, 0xc0733d06cb068e70uL, 0xbe1e76e4c20e3d9euL,
     0xc0733d0c4f175570uL, 0xbe1e77070bf3cf61uL, 0xc0733d11cecaadc0uL, 0xbe1e781c43502734uL, 0xc0733d174a277a80uL, 0xbe1e78b11268ea72uL,
     0xc0733d1cc1348e90uL, 0xbe1e7754b83bfc7duL, 0xc0733d2233f8acb0uL, 0xbe1e7756c29bf5e9uL, 0xc0733d27a27a87d0uL, 0xbe1e7952fc1d9333uL,
     0xc0733d2d0cc0c350uL, 0xbe1e778c76ae6077uL, 0xc0733d3272d1f2e0uL, 0xbe1e7a1896ba8f43uL, 0xc0733d37d4b49b30uL, 0xbe1e76dafdf432d8uL,
     0xc0733d3d326f3180uL, 0xbe1e795330184013uL, 0xc0733d428c081c80uL, 0xbe1e763cc774d30fuL, 0xc0733d47e185b3d0uL, 0xbe1e77030a779c0auL,
     0xc0733d4d32ee40b0uL, 0xbe1e7908af2a2d7euL, 0xc0733d528047fe00uL, 0xbe1e78c4953b797duL, 0xc0733d57c9991850uL, 0xbe1e78b43b096579uL,
     0xc0733d5d0ee7ae30uL, 0xbe1e7824ae0a4804uL, 0xc0733d625039d040uL, 0xbe1e79d2b2fbb740uL, 0xc0733d678d958190uL, 0xbe1e7662de59a1a6uL,
     0xc0733d6cc700b760uL, 0xbe1e76b251d59aaauL, 0xc0733d71fc8159b0uL, 0xbe1e7a00cfd1f487uL, 0xc0733d772e1d4360uL, 0xbe1e77f4d246167euL,
     0xc0733d7c5bda4200uL, 0xbe1e767a4ee8e6fcuL, 0xc0733d8185be1640uL, 0xbe1e777ccf0a8aeduL, 0xc0733d86abce7420uL, 0xbe1e767d7e279adauL,
     0xc0733d8bce1102d0uL, 0xbe1e7a05cef4bb90uL, 0xc0733d90ec8b5d40uL, 0xbe1e78f75369be5buL, 0xc0733d96074311d0uL, 0xbe1e77b9612e8c8auL,
     0xc0733d9b1e3da2b0uL, 0xbe1e794518b9adebuL, 0xc0733da031808620uL, 0xbe1e7810626fb934uL, 0xc0733da541112650uL, 0xbe1e76d87223fa6duL,
     0xc0733daa4cf4e1a0uL, 0xbe1e794c5e7ca3b5uL, 0xc0733daf55310af0uL, 0xbe1e789856ef816fuL, 0xc0733db459cae970uL, 0xbe1e77d2004effbduL,
     0xc0733db95ac7b8f0uL, 0xbe1e78467d31eb9cuL, 0xc0733dbe582caa00uL, 0xbe1e79aaa4e25787uL, 0xc0733dc351fee220uL, 0xbe1e762de8f107bfuL,
     0xc0733dc848437b90uL, 0xbe1e7670670a63feuL, 0xc0733dcd3aff85d0uL, 0xbe1e795ca237c6ccuL, 0xc0733dd22a3805b0uL, 0xbe1e77e55c53c1d9uL,
     0xc0733dd715f1f520uL, 0xbe1e78a806213ac4uL, 0xc0733ddbfe3243b0uL, 0xbe1e77743a2bc615uL, 0xc0733de0e2fdd660uL, 0xbe1e78b8b45b0b7duL,
     0xc0733de5c4598800uL, 0xbe1e78d635f2f4b9uL, 0xc0733deaa24a2920uL, 0xbe1e7758c396a11euL, 0xc0733def7cd48020uL, 0xbe1e7a17a8cc454cuL,
     0xc0733df453fd49a0uL, 0xbe1e783caa73f616uL, 0xc0733df927c93820uL, 0xbe1e7932cfa29664uL, 0xc0733dfdf83cf490uL, 0xbe1e777d265c72a6uL,
     0xc0733e02c55d1e10uL, 0xbe1e7775e7c03c60uL, 0xc0733e078f2e4a40uL, 0xbe1e79f65d52d232uL, 0xc0733e0c55b50570uL, 0xbe1e76e7e7464b4euL,
     0xc0733e1118f5d250uL, 0xbe1e77be81cad877uL, 0xc0733e15d8f52a80uL, 0xbe1e79dd25b5fb3auL, 0xc0733e1a95b77e80uL, 0xbe1e78e45f1418efuL,
     0xc0733e1f4f4135a0uL, 0xbe1e78eb7289505buL, 0xc0733e240596ae50uL, 0xbe1e78a468c07caduL, 0xc0733e28b8bc3e20uL, 0xbe1e776b558a4009uL,
     0xc0733e2d68b631d0uL, 0xbe1e77412eb9941euL, 0xc0733e321588cd80uL, 0xbe1e76b2853f845euL, 0xc0733e36bf384cb0uL, 0xbe1e76aa7184273cuL,
     0xc0733e3b65c8e260uL, 0xbe1e7832027f78fauL, 0xc0733e40093eb930uL, 0xbe1e7a1c7da131f5uL, 0xc0733e44a99df380uL, 0xbe1e76a0bc2ae4bcuL,
     0xc0733e4946eaab30uL, 0xbe1e78dff13b6f5duL, 0xc0733e4de128f250uL, 0xbe1e765a226dea2cuL, 0xc0733e52785cd290uL, 0xbe1e78509b989111uL,
     0xc0733e570c8a4de0uL, 0xbe1e7916a4e9803duL, 0xc0733e5b9db55e30uL, 0xbe1e7950c15758ccuL, 0xc0733e602be1f5a0uL, 0xbe1e7922ba1ad420uL,
     0xc0733e64b713fe90uL, 0xbe1e794cbaabcef6uL, 0xc0733e693f4f5bc0uL, 0xbe1e7837bf883feduL, 0xc0733e6dc497e850uL, 0xbe1e76f198ddbbdfuL,
     0xc0733e7246f177d0uL, 0xbe1e7a18c1067764uL, 0xc0733e76c65fd6a0uL, 0xbe1e76b845a8fd9duL, 0xc0733e7b42e6c970uL, 0xbe1e7714012df506uL,
     0xc0733e7fbc8a0de0uL, 0xbe1e7765612922cduL, 0xc0733e84334d5a50uL, 0xbe1e7688f5424a00uL, 0xc0733e88a7345df0uL, 0xbe1e769d011f6663uL,
     0xc0733e8d1842c0e0uL, 0xbe1e79914acbfaf7uL, 0xc0733e91867c2460uL, 0xbe1e79a85e189bd7uL, 0xc0733e95f1e422a0uL, 0xbe1e79ea7c726432uL,
     0xc0733e9a5a7e4f10uL, 0xbe1e768a6fbb8e6euL, 0xc0733e9ec04e3620uL, 0xbe1e793c75bcc9fcuL, 0xc0733ea323575dd0uL, 0xbe1e797f78da13d4uL,
     0xc0733ea7839d4550uL, 0xbe1e78d8c9cda978uL, 0xc0733eabe1236540uL, 0xbe1e77028d480fffuL, 0xc0733eb03bed2fa0uL, 0xbe1e7a0d0f74ff7cuL,
     0xc0733eb493fe1040uL, 0xbe1e76732e8a35fbuL, 0xc0733eb8e9596c30uL, 0xbe1e77220caeabebuL, 0xc0733ebd3c02a260uL, 0xbe1e797438b645efuL,
     0xc0733ec18bfd0b80uL, 0xbe1e79207c5fd6e8uL, 0xc0733ec5d94bf9f0uL, 0xbe1e781c7df8f946uL, 0xc0733eca23f2b9f0uL, 0xbe1e76736284e2dbuL,
     0xc0733ece6bf49190uL, 0xbe1e7a109cc0c3f5uL, 0xc0733ed2b154c120uL, 0xbe1e767f14a16d50uL, 0xc0733ed6f4168290uL, 0xbe1e789cd22acaf0uL,
     0xc0733edb343d0a40uL, 0xbe1e764355ca28aduL, 0xc0733edf71cb8660uL, 0xbe1e79e4c7a81c45uL, 0xc0733ee3acc51fb0uL, 0xbe1e761e26b644c2uL,
     0xc0733ee7e52cf8c0uL, 0xbe1e793e9f8fbdd3uL, 0xc0733eec1b062ed0uL, 0xbe1e78c432991c20uL, 0xc0733ef04e53d940uL, 0xbe1e78cdd025f4d8uL,
     0xc0733ef47f1909f0uL, 0xbe1e778310c6446euL, 0xc0733ef8ad58cd20uL, 0xbe1e7871af3d6e17uL, 0xc0733efcd91629b0uL, 0xbe1e77e0e906f697uL,
     0xc0733f01025420f0uL, 0xbe1e7a1ae9b27892uL, 0xc0733f052915af00uL, 0xbe1e76ac64c88f9duL, 0xc0733f094d5dca60uL, 0xbe1e779a815589c4uL,
     0xc0733f0d6f2f6480uL, 0xbe1e788f39a4864cuL, 0xc0733f118e8d6980uL, 0xbe1e79fc51263525uL, 0xc0733f15ab7ac060uL, 0xbe1e783501f19e90uL,
     0xc0733f19c5fa4ae0uL, 0xbe1e767e82c327abuL, 0xc0733f1dde0ee5a0uL, 0xbe1e7a1785d66123uL, 0xc0733f21f3bb6870uL, 0xbe1e7936d07203dauL,
     0xc0733f260702a5e0uL, 0xbe1e7a010a7ac699uL, 0xc0733f2a17e76bb0uL, 0xbe1e7975e4e16312uL, 0xc0733f2e266c82b0uL, 0xbe1e7654b5422330uL,
     0xc0733f323294aeb0uL, 0xbe1e77f8a4909d35uL, 0xc0733f363c62aee0uL, 0xbe1e792c8e30d226uL, 0xc0733f3a43d93da0uL, 0xbe1e76f6ac67a1ffuL,
     0xc0733f3e48fb1070uL, 0xbe1e775c2e97715auL, 0xc0733f424bcad840uL, 0xbe1e781cd54ae100uL}

    , {
       0x0000000000000000uL, 0xbf4bc48a867884b7uL, 0xbf5bbd9e9482af09uL, 0xbf64c9096b94befduL, 0xbf6bafd47221ed26uL, 0xbf714999e2ad8ea6uL,
       0xbf74b99563d2a1bduL, 0xbf7827de6b310350uL, 0xbf7b9476a4fcd10fuL, 0xbf7eff5fbaf25781uL, 0xbf81344daa2d7553uL, 0xbf82e8158b08d957uL,
       0xbf849b0851443684uL, 0xbf864d26cce610dduL, 0xbf87fe71ccc4e6b0uL, 0xbf89aeea1e897fdfuL, 0xbf8b5e908eb13790uL, 0xbf8d0d65e890405auL,
       0xbf8ebb6af653e2eeuL, 0xbf90345040825baduL, 0xbf910a83a8446c78uL, 0xbf91e05015d30a71uL, 0xbf92b5b5ec0209d3uL, 0xbf938ab58d173e91uL,
       0xbf945f4f5acb8be0uL, 0xbf953383b64bf13fuL, 0xbf960753003a94efuL, 0xbf96dabd98afcc05uL, 0xbf97adc3df3b1ff8uL, 0xbf98806632e451d0uL,
       0xbf9952a4f22c5ae9uL, 0xbf9a24807b0e6b5cuL, 0xbf9af5f92b00e610uL, 0xbf9bc70f5ef65a77uL, 0xbf9c97c3735e7c0auL, 0xbf9d6815c4271775uL,
       0xbf9e3806acbd058fuL, 0xbf9f0796880d1c19uL, 0xbf9fd6c5b0851c4cuL, 0xbfa052ca400a4f9buL, 0xbfa0ba01a8170000uL, 0xbfa121093ce3a205uL,
       0xbfa187e12aad8077uL, 0xbfa1ee899d74a03euL, 0xbfa25502c0fc314cuL, 0xbfa2bb4cc0cafe8duL, 0xbfa32167c82bdcdauL, 0xbfa38754022e18e2uL,
       0xbfa3ed1199a5e425uL, 0xbfa452a0b92cc0ecuL, 0xbfa4b8018b21ed4fuL, 0xbfa51d3439aacd4auL, 0xbfa58238eeb353dauL, 0xbfa5e70fd3ee6b34uL,
       0xbfa64bb912d65c07uL, 0xbfa6b034d4ad33dfuL, 0xbfa71483427d2a99uL, 0xbfa778a4851906f3uL, 0xbfa7dc98c51c8242uL, 0xbfa840602aecab3duL,
       0xbfa8a3fadeb847f4uL, 0xbfa90769087836e4uL, 0xbfa96aaacfefcf3cuL, 0xbfa9cdc05cad4042uL, 0xbfaa30a9d609efeauL, 0xbfaa9367632ad897uL,
       0xbfaaf5f92b00e610uL, 0xbfab585f544951a4uL, 0xbfabba9a058dfd84uL, 0xbfac1ca96525cf56uL, 0xbfac7e8d993509f9uL, 0xbface046c7ada68duL,
       0xbfad41d5164facb4uL, 0xbfada338aaa98a0cuL, 0xbfae0471aa1868f5uL, 0xbfae658039c88690uL, 0xbfaec6647eb58808uL, 0xbfaf271e9daacf20uL,
       0xbfaf87aebb43ce06uL, 0xbfafe814fbec5a77uL, 0xbfb02428c1f08016uL, 0xbfb054323b97a948uL, 0xbfb08426fcdb1ee7uL, 0xbfb0b40717932b96uL,
       0xbfb0e3d29d81165euL, 0xbfb11389a04f4a2euL, 0xbfb1432c31917d08uL, 0xbfb172ba62c4d6deuL, 0xbfb1a23445501816uL, 0xbfb1d199ea83bfbeuL,
       0xbfb200eb639a3173uL, 0xbfb23028c1b7daeduL, 0xbfb25f5215eb594auL, 0xbfb28e67712d9dfcuL, 0xbfb2bd68e4621371uL, 0xbfb2ec568056c16fuL,
       0xbfb31b3055c47118uL, 0xbfb349f6754ed0b4uL, 0xbfb378a8ef84971euL, 0xbfb3a747d4dfa6f5uL, 0xbfb3d5d335c53179uL, 0xbfb4044b2285d925uL,
       0xbfb432afab5dd3ffuL, 0xbfb46100e0750da1uL, 0xbfb48f3ed1df48fbuL, 0xbfb4bd698f9c41cfuL, 0xbfb4eb812997cde4uL, 0xbfb51985afa9fdfduL,
       0xbfb5477731973e85uL, 0xbfb57555bf1077f5uL, 0xbfb5a32167b32f02uL, 0xbfb5d0da3b09a47euL, 0xbfb5fe80488af4fduL, 0xbfb62c139f9b3837uL,
       0xbfb659944f8ba02duL, 0xbfb68702679a980auL, 0xbfb6b45df6f3e2c9uL, 0xbfb6e1a70cb0b99auL, 0xbfb70eddb7d7ea07uL, 0xbfb73c02075df3e5uL,
       0xbfb769140a2526fduL, 0xbfb79613cefdc07duL, 0xbfb7c30164a60836uL, 0xbfb7efdcd9ca6d8fuL, 0xbfb81ca63d05a44auL, 0xbfb8495d9ce0c10cuL,
       0xbfb8760307d355abuL, 0xbfb8a2968c438d41uL, 0xbfb8cf183886480duL, 0xbfb8fb881adf3713uL, 0xbfb927e64180f790uL, 0xbfb95432ba8d2e2fuL,
       0xbfb9806d9414a209uL, 0xbfb9ac96dc175776uL, 0xbfb9d8aea084aa9cuL, 0xbfba04b4ef3b69d8uL, 0xbfba30a9d609efeauL, 0xbfba5c8d62ae3decuL,
       0xbfba885fa2d6151euL, 0xbfbab420a41f1076uL, 0xbfbadfd07416be07uL, 0xbfbb0b6f203ab82cuL, 0xbfbb36fcb5f8be8auL, 0xbfbb627942aecedduL,
       0xbfbb8de4d3ab3d98uL, 0xbfbbb93f762cce4fuL, 0xbfbbe4893762cbf7uL, 0xbfbc0fc2246d20f5uL, 0xbfbc3aea4a5c6effuL, 0xbfbc6601b63226cbuL,
       0xbfbc910874e09f98uL, 0xbfbcbbfe934b2e81uL, 0xbfbce6e41e463da5uL, 0xbfbd11b92297632buL, 0xbfbd3c7dacf5780buL, 0xbfbd6731ca08aeb9uL,
       0xbfbd91d5866aa99cuL, 0xbfbdbc68eea6915buL, 0xbfbde6ec0f392b05uL, 0xbfbe115ef490ee07uL, 0xbfbe3bc1ab0e19feuL, 0xbfbe66143f02cc5duL,
       0xbfbe9056bcb315e8uL, 0xbfbeba893055100buL, 0xbfbee4aba610f204uL, 0xbfbf0ebe2a0125ebuL, 0xbfbf38c0c8325d86uL, 0xbfbf62b38ca3a706uL,
       0xbfbf8c9683468191uL, 0xbfbfb669b7fef1a8uL, 0xbfbfe02d36a3956duL, 0xbfc004f0857edc5cuL, 0xbfc019c2a064b486uL, 0xbfc02e8cf1dac4b8uL,
       0xbfc0434f7fb1f307uL, 0xbfc0580a4fb4a3dfuL, 0xbfc06cbd67a6c3b6uL, 0xbfc08168cd45d0a9uL, 0xbfc0960c8648e406uL, 0xbfc0aaa89860bbcfuL,
       0xbfc0bf3d0937c41cuL, 0xbfc0d3c9de722078uL, 0xbfc0e84f1dadb526uL, 0xbfc0fccccc823059uL, 0xbfc11142f0811357uL, 0xbfc125b18f35bb8euL,
       0xbfc13a18ae256b99uL, 0xbfc14e7852cf5430uL, 0xbfc162d082ac9d10uL, 0xbfc1772143306dc6uL, 0xbfc18b6a99c7f679uL, 0xbfc19fac8bda7897uL,
       0xbfc1b3e71ec94f7buL, 0xbfc1c81a57eff8fduL, 0xbfc1dc463ca41df8uL, 0xbfc1f06ad2359abduL, 0xbfc204881dee8777uL, 0xbfc2189e25134081uL,
       0xbfc22cacece26eaduL, 0xbfc240b47a950f79uL, 0xbfc254b4d35e7d3cuL, 0xbfc268adfc6c773euL, 0xbfc27c9ffae729c1uL, 0xbfc2908ad3f13603uL,
       0xbfc2a46e8ca7ba2auL, 0xbfc2b84b2a225923uL, 0xbfc2cc20b1734279uL, 0xbfc2dfef27a73a18uL, 0xbfc2f3b691c5a001uL, 0xbfc30776f4d077f7uL,
       0xbfc31b3055c47118uL, 0xbfc32ee2b998ed6euL, 0xbfc3428e2540096duL, 0x3fc331f403985097uL, 0x3fc31e56798a910auL, 0x3fc30abfd8f333b6uL,
       0x3fc2f7301cf4e87buL, 0x3fc2e3a740b7800fuL, 0x3fc2d0253f67e4cbuL, 0x3fc2bcaa14381386uL, 0x3fc2a935ba5f1479uL, 0x3fc295c82d18f434uL,
       0x3fc2826167a6bc9cuL, 0x3fc26f01654e6df6uL, 0x3fc25ba8215af7fcuL, 0x3fc24855971c3307uL, 0x3fc23509c1e6d937uL, 0x3fc221c49d147fb3uL,
       0x3fc20e8624038feduL, 0x3fc1fb4e521740f4uL, 0x3fc1e81d22b790d4uL, 0x3fc1d4f291513e01uL, 0x3fc1c1ce9955c0c6uL, 0x3fc1aeb1363b44c8uL,
       0x3fc19b9a637ca295uL, 0x3fc1888a1c995931uL, 0x3fc175805d1587c1uL, 0x3fc1627d2079e731uL, 0x3fc14f806253c3eduL, 0x3fc13c8a1e34f7a0uL,
       0x3fc1299a4fb3e306uL, 0x3fc116b0f26b67bbuL, 0x3fc103ce01fae223uL, 0x3fc0f0f17a062353uL, 0x3fc0de1b56356b04uL, 0x3fc0cb4b9235619auL,
       0x3fc0b88229b71227uL, 0x3fc0a5bf186fe483uL, 0x3fc093025a19976cuL, 0x3fc0804bea723aa9uL, 0x3fc06d9bc53c2941uL, 0x3fc05af1e63e03b4uL,
       0x3fc0484e4942aa43uL, 0x3fc035b0ea19373buL, 0x3fc02319c494f951uL, 0x3fc01088d48d6e03uL, 0x3fbffbfc2bbc7803uL, 0x3fbfd6f308ce5b52uL,
       0x3fbfb1f6381856f4uL, 0x3fbf8d05b16a6d47uL, 0x3fbf68216c9cc727uL, 0x3fbf4349618fa91auL, 0x3fbf1e7d882b689auL, 0x3fbef9bdd860616buL,
       0x3fbed50a4a26eafcuL, 0x3fbeb062d57f4de8uL, 0x3fbe8bc77271b97auL, 0x3fbe6738190e394cuL, 0x3fbe42b4c16caaf3uL, 0x3fbe1e3d63acb3bauL,
       0x3fbdf9d1f7f5b674uL, 0x3fbdd5727676c959uL, 0x3fbdb11ed766abf4uL, 0x3fbd8cd71303bd26uL, 0x3fbd689b2193f133uL, 0x3fbd446afb64c7e5uL,
       0x3fbd204698cb42bduL, 0x3fbcfc2df223db2duL, 0x3fbcd820ffd278f3uL, 0x3fbcb41fba42686duL, 0x3fbc902a19e65111uL, 0x3fbc6c4017382beauL,
       0x3fbc4861aab93a23uL, 0x3fbc248eccf1fba6uL, 0x3fbc00c7767225cbuL, 0x3fbbdd0b9fd09a10uL, 0x3fbbb95b41ab5ce6uL, 0x3fbb95b654a78c87uL,
       0x3fbb721cd17157e3uL, 0x3fbb4e8eb0bbf58fuL, 0x3fbb2b0beb419ad0uL, 0x3fbb079479c372aduL, 0x3fbae4285509950buL, 0x3fbac0c775e2fde6uL,
       0x3fba9d71d5258484uL, 0x3fba7a276badd2c8uL, 0x3fba56e8325f5c87uL, 0x3fba33b4222456f1uL, 0x3fba108b33edb005uL, 0x3fb9ed6d60b30612uL,
       0x3fb9ca5aa1729f45uL, 0x3fb9a752ef316149uL, 0x3fb9845642fac8f0uL, 0x3fb9616495e0e1e8uL, 0x3fb93e7de0fc3e80uL, 0x3fb91ba21d6bef77uL,
       0x3fb8f8d144557bdfuL, 0x3fb8d60b4ee4d901uL, 0x3fb8b350364c6257uL, 0x3fb8909ff3c4d191uL, 0x3fb86dfa808d36a0uL, 0x3fb84b5fd5eaefd8uL,
       0x3fb828cfed29a215uL, 0x3fb8064abf9b30f1uL, 0x3fb7e3d04697b704uL, 0x3fb7c1607b7d7e32uL, 0x3fb79efb57b0f803uL, 0x3fb77ca0d49cb608uL,
       0x3fb75a50ebb1624auL, 0x3fb7380b9665b7c8uL, 0x3fb715d0ce367afcuL, 0x3fb6f3a08ca67270uL, 0x3fb6d17acb3e5f5euL, 0x3fb6af5f838cf654uL,
       0x3fb68d4eaf26d7eeuL, 0x3fb66b4847a68997uL, 0x3fb6494c46ac6e4duL, 0x3fb6275aa5debf81uL, 0x3fb605735ee985f1uL, 0x3fb5e3966b7e9295uL,
       0x3fb5c1c3c5557799uL, 0x3fb59ffb662b815cuL, 0x3fb57e3d47c3af7buL, 0x3fb55c8963e6adebuL, 0x3fb53adfb462ce16uL, 0x3fb51940330c000buL,
       0x3fb4f7aad9bbcbafuL, 0x3fb4d61fa2514a00uL, 0x3fb4b49e86b11e5fuL, 0x3fb4932780c56fe2uL, 0x3fb471ba8a7de2b7uL, 0x3fb450579dcf9186uL,
       0x3fb42efeb4b506e9uL, 0x3fb40dafc92e36e2uL, 0x3fb3ec6ad5407868uL, 0x3fb3cb2fd2f67ef1uL, 0x3fb3a9febc60540auL, 0x3fb388d78b9350ffuL,
       0x3fb367ba3aaa1883uL, 0x3fb346a6c3c49066uL, 0x3fb3259d2107db54uL, 0x3fb3049d4c9e52a0uL, 0x3fb2e3a740b7800fuL, 0x3fb2c2baf78817b7uL,
       0x3fb2a1d86b49f1e2uL, 0x3fb280ff963c04fcuL, 0x3fb2603072a25f82uL, 0x3fb23f6afac6220auL, 0x3fb21eaf28f57941uL, 0x3fb1fdfcf7839804uL,
       0x3fb1dd5460c8b16fuL, 0x3fb1bcb55f21f307uL, 0x3fb19c1fecf17ee0uL, 0x3fb17b94049e65d0uL, 0x3fb15b11a094a1aauL, 0x3fb13a98bb450f81uL,
       0x3fb11a294f2569f6uL, 0x3fb0f9c356b04389uL, 0x3fb0d966cc6500fauL, 0x3fb0b913aac7d3a7uL, 0x3fb098c9ec61b3ffuL, 0x3fb078898bc05bf4uL,
       0x3fb0585283764178uL, 0x3fb03824ce1a9101uL, 0x3fb0180066492817uL, 0x3fafefca8d451fd6uL, 0x3fafafa6d397efdbuL, 0x3faf6f9594de60f0uL,
       0x3faf2f96c6754aeeuL, 0x3faeefaa5dc2b239uL, 0x3faeafd05035bd3buL, 0x3fae70089346a9e6uL, 0x3fae30531c76c34auL, 0x3fadf0afe1505738uL,
       0x3fadb11ed766abf4uL, 0x3fad719ff455f5f7uL, 0x3fad32332dc34dbduL, 0x3facf2d8795ca5a5uL, 0x3facb38fccd8bfdbuL, 0x3fac74591df72456uL,
       0x3fac3534628016dduL, 0x3fabf62190448d22uL, 0x3fabb7209d1e24e5uL, 0x3fab78317eef1a29uL, 0x3fab39542ba23d73uL, 0x3faafa88992aea19uL,
       0x3faabbcebd84fca0uL, 0x3faa7d268eb4c924uL, 0x3faa3e9002c711d2uL, 0x3faa000b0fd0fd6buL, 0x3fa9c197abf00dd7uL, 0x3fa98335cd4a16c3uL,
       0x3fa944e56a0d3450uL, 0x3fa906a6786fc1cbuL, 0x3fa8c878eeb05074uL, 0x3fa88a5cc3159e53uL, 0x3fa84c51ebee8d15uL, 0x3fa80e585f9218fcuL,
       0x3fa7d070145f4fd7uL, 0x3fa7929900bd4809uL, 0x3fa754d31b1b179cuL, 0x3fa7171e59efcb5fuL, 0x3fa6d97ab3ba5e10uL, 0x3fa69be81f01af99uL,
       0x3fa65e6692547c4euL, 0x3fa620f604495440uL, 0x3fa5e3966b7e9295uL, 0x3fa5a647be9a54f6uL, 0x3fa56909f44a72feuL, 0x3fa52bdd034475b8uL,
       0x3fa4eec0e2458f30uL, 0x3fa4b1b588129203uL, 0x3fa474baeb77e904uL, 0x3fa437d103498eecuL, 0x3fa3faf7c663060euL, 0x3fa3be2f2ba7501fuL,
       0x3fa381772a00e604uL, 0x3fa344cfb861afaeuL, 0x3fa30838cdc2fbfduL, 0x3fa2cbb2612578b4uL, 0x3fa28f3c69912a74uL, 0x3fa252d6de1564c1uL,
       0x3fa21681b5c8c213uL, 0x3fa1da3ce7c91bf8uL, 0x3fa19e086b3b8333uL, 0x3fa161e4374c37f4uL, 0x3fa125d0432ea20euL, 0x3fa0e9cc861d4944uL,
       0x3fa0add8f759cd95uL, 0x3fa071f58e2cdf9buL, 0x3fa0362241e638ecuL, 0x3f9ff4be13b92920uL, 0x3f9f7d57badb4ee8uL, 0x3f9f061167fc31e8uL,
       0x3f9e8eeb09f2f6cbuL, 0x3f9e17e48fa48962uL, 0x3f9da0fde8038de9uL, 0x3f9d2a3702105259uL, 0x3f9cb38fccd8bfdbuL, 0x3f9c3d0837784c41uL,
       0x3f9bc6a03117eb97uL, 0x3f9b5057a8ee01ceuL, 0x3f9ada2e8e3e546fuL, 0x3f9a6424d059fc68uL, 0x3f99ee3a5e9f57e8uL, 0x3f99786f2879fc53uL,
       0x3f9902c31d62a843uL, 0x3f988d362cdf359euL, 0x3f9817c846828bbduL, 0x3f97a27959ec91aauL, 0x3f972d4956ca2067uL, 0x3f96b8382cd4f551uL,
       0x3f964345cbd3a491uL, 0x3f95ce7223998b98uL, 0x3f9559bd2406c3bauL, 0x3f94e526bd0814d1uL, 0x3f9470aede96e7f2uL, 0x3f93fc5578b93a38uL,
       0x3f93881a7b818f9euL, 0x3f9313fdd70ee5e8uL, 0x3f929fff7b8ca79duL, 0x3f922c1f59329f1buL, 0x3f91b85d6044e9aeuL, 0x3f9144b98113eac0uL,
       0x3f90d133abfc3f1buL, 0x3f905dcbd166b033uL, 0x3f8fd503c3904f1duL, 0x3f8eeeab9b43445duL, 0x3f8e088f0b004827uL, 0x3f8d22adf3f9579duL,
       0x3f8c3d0837784c41uL, 0x3f8b579db6dec358uL, 0x3f8a726e53a6056euL, 0x3f898d79ef5eedf0uL, 0x3f88a8c06bb1d2f4uL, 0x3f87c441aa5e6d15uL,
       0x3f86dffd8d3bbf70uL, 0x3f85fbf3f637ffc5uL, 0x3f851824c7587eb0uL, 0x3f84348fe2b99002uL, 0x3f8351352a8e733fuL, 0x3f826e1481213c2euL,
       0x3f818b2dc8d2bb91uL, 0x3f80a880e41a67f6uL, 0x3f7f8c1b6b0c8d4euL, 0x3f7dc7a83f75a96duL, 0x3f7c03a80ae5e054uL, 0x3f7a401a92ff827euL,
       0x3f787cff9d9147a5uL, 0x3f76ba56f09621bcuL, 0x3f74f8205235102duL, 0x3f73365b88c0f347uL, 0x3f7175085ab85ff0uL, 0x3f6f684d1d8ae702uL,
       0x3f6be76bd77b4fc3uL, 0x3f68676c71434fb9uL, 0x3f64e84e793a474auL, 0x3f616a117e0d4b30uL, 0x3f5bd96a1d7d9cbcuL, 0x3f54e071754c98bauL,
       0x3f4bd27045bfd025uL, 0x3f3bcef518e29612uL, 0x8000000000000000uL}

    , {
       0xc025C91A15F7EB9FuL, 0x4016ABA863A166DBuL, 0xc008930964D2FA9BuL, 0x3ffC6A02DC954978uL, 0xbff27AF2DC77B115uL, 0x3f5A7A6CBF2E410CuL}

    , {
       0x3fb63C65231FBD16uL, 0xbfbBCB7D4EFBE80BuL, 0x3fc287A7636F341EuL, 0xbfcBCB7B1526DE36uL, 0x3fdBCB7B1526E50EuL}

    , 0x000fffffffffffffuL, 0x3f50000000000000uL, 0x0010000000000000uL, 0x7fefffffffffffffuL, 0xfffffffffc000000uL, 0x3fdbc00000000000uL,
        0x3ff0000000000000uL, 0x3fd34413509F0000uL, 0x3d7e7fbcc47c0000uL, 0x4086a00000000000uL, 0x408ff80000000000uL, 0x408ff00000000000uL,
        0x3fd34413509f79ffuL, {0x7ff0000000000000uL, 0xfff0000000000000uL}

    , {0x3ff0000000000000uL, 0xbff0000000000000uL}

    , {0x0000000000000000uL, 0x8000000000000000uL}

};

static __constant _iml_v2_dp_union_t __dlog10_la_CoutTab[212] = {
    0x00000000, 0x3FDBC000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x3FDB5100,
    0x0B000000, 0x3F7C03A8,
    0x7D2AE08E, 0xBD7A1FAC,
    0x00000000, 0x3FDAE200,
    0x37800000, 0x3F8C3D08,
    0x8D01E8AD, 0xBD6ECEFD,
    0x00000000, 0x3FDA7300,
    0x24000000, 0x3F9559BD,
    0x2A435ABF, 0x3D7B0EE8,
    0x00000000, 0x3FDA0400,
    0xCCE00000, 0x3F9CB38F,
    0x5A535AE7, 0xBD7D0092,
    0x00000000, 0x3FD9CC80,
    0x41E80000, 0x3FA03622,
    0x092C91B2, 0xBD6C713A,
    0x00000000, 0x3FD95D80,
    0xC6640000, 0x3FA3FAF7,
    0xE873F056, 0xBD5F3E4C,
    0x00000000, 0x3FD8EE80,
    0x14600000, 0x3FA7D070,
    0x958F7A7E, 0xBD560516,
    0x00000000, 0x3FD8B700,
    0xABF00000, 0x3FA9C197,
    0x3D6E4566, 0x3D1BADCF,
    0x00000000, 0x3FD84800,
    0xD7680000, 0x3FADB11E,
    0xD23C3E45, 0xBD6540BC,
    0x00000000, 0x3FD81080,
    0xD3980000, 0x3FAFAFA6,
    0xD08CE301, 0xBD2024E9,
    0x00000000, 0x3FD7A180,
    0x60C90000, 0x3FB1DD54,
    0xAC3F3D12, 0xBD53A44F,
    0x00000000, 0x3FD76A00,
    0x40B80000, 0x3FB2E3A7,
    0xEF53F2ED, 0xBD5FFC5A,
    0x00000000, 0x3FD6FB00,
    0xD9BC0000, 0x3FB4F7AA,
    0xD69E3919, 0xBD4A2895,
    0x00000000, 0x3FD6C380,
    0x5EEA0000, 0x3FB60573,
    0x0DC5E826, 0xBD5E83C3,
    0x00000000, 0x3FD68C00,
    0xCE360000, 0x3FB715D0,
    0x67BF155A, 0x3D5EBEEB,
    0x00000000, 0x3FD61D00,
    0xE0FC0000, 0x3FB93E7D,
    0xE5FCD7D1, 0x3D4F3FC5,
    0x00000000, 0x3FD5E580,
    0x325F0000, 0x3FBA56E8,
    0xBB1780F3, 0x3D5721BB,
    0x00000000, 0x3FD5AE00,
    0xD1710000, 0x3FBB721C,
    0x7D982E77, 0x3D55F8A5,
    0x00000000, 0x3FD57680,
    0x19E60000, 0x3FBC902A,
    0xE4249335, 0x3D54445C,
    0x00000000, 0x3FD53F00,
    0xD7670000, 0x3FBDB11E,
    0x48F0F913, 0xBD5502F3,
    0x00000000, 0x3FD4D000,
    0x2BBC0000, 0x3FBFFBFC,
    0x60DF12C3, 0x3D5E00DD,
    0x00000000, 0x3FD49880,
    0x5A198000, 0x3FC09302,
    0xB578FCD3, 0x3D476B97,
    0x00000000, 0x3FD46100,
    0x4FB40000, 0x3FC1299A,
    0x9B211E99, 0xBD4CFA73,
    0x00000000, 0x3FD42980,
    0x9955C000, 0x3FC1C1CE,
    0xB6D05A73, 0x3CF8B891,
    0x00000000, 0x3FD3F200,
    0x215B0000, 0x3FC25BA8,
    0x2D8BF07A, 0xBD3007BF,
    0x00000000, 0x3FD3BA80,
    0x1CF50000, 0x3FC2F730,
    0x7972CE60, 0xBD47851C,
    0x00000000, 0x3FD38300,
    0x0F794000, 0x3FC39470,
    0xEF234E4D, 0x3D43FD27,
    0x00000000, 0x3FD34B80,
    0xCDE08000, 0x3FC43371,
    0xAAED5307, 0xBD327C6E,
    0x00000000, 0x3FD31400,
    0x8275C000, 0x3FC4D43F,
    0x9750CE36, 0xBD4B7CD5,
    0x00000000, 0x3FD2DC80,
    0xB0BE0000, 0x3FC576E3,
    0x4FCE8E8C, 0xBD4F58FE,
    0x00000000, 0x3FD2A500,
    0x39984000, 0x3FC61B69,
    0x69C572DA, 0xBD3F70DC,
    0x00000000, 0x3FD26D80,
    0x5F9BC000, 0x3FC6C1DB,
    0x986FE812, 0xBD3994B8,
    0x00000000, 0x3FD23600,
    0xCBB80000, 0x3FC76A45,
    0x8732D38D, 0xBD490138,
    0x00000000, 0x3FD23600,
    0xCBB80000, 0x3FC76A45,
    0x8732D38D, 0xBD490138,
    0x00000000, 0x3FD1FE80,
    0x921BC000, 0x3FC814B4,
    0xDFFA64AD, 0x3D452B6B,
    0x00000000, 0x3FD1C700,
    0x37694000, 0x3FC8C134,
    0x3596594C, 0x3D4531EB,
    0x00000000, 0x3FD18F80,
    0xB63A0000, 0x3FC96FD1,
    0xE74E02A0, 0xBD1FB7D8,
    0x00000000, 0x3FD15800,
    0x84FBC000, 0x3FCA209A,
    0x1F12B358, 0x3D3FEF31,
    0x00000000, 0x3FD15800,
    0x84FBC000, 0x3FCA209A,
    0x1F12B358, 0x3D3FEF31,
    0x00000000, 0x3FD12080,
    0x9C2C8000, 0x3FCAD39C,
    0x5296C839, 0xBD4F8064,
    0x00000000, 0x3FD0E900,
    0x7CF98000, 0x3FCB88E6,
    0xBB0EBF1B, 0xBD2A01B9,
    0x00000000, 0x3FD0B180,
    0x384F4000, 0x3FCC4087,
    0x3F7C716E, 0x3D3F006E,
    0x00000000, 0x3FD0B180,
    0x384F4000, 0x3FCC4087,
    0x3F7C716E, 0x3D3F006E,
    0x00000000, 0x3FD07A00,
    0x765CC000, 0x3FCCFA8E,
    0xD8B819B8, 0xBD22386A,
    0x00000000, 0x3FD04280,
    0x7E970000, 0x3FCDB70C,
    0x43117D8D, 0xBD480CFA,
    0x00000000, 0x3FD00B00,
    0x4046C000, 0x3FCE7612,
    0x23FE0FB6, 0xBD381A1E,
    0x00000000, 0x3FD00B00,
    0x4046C000, 0x3FCE7612,
    0x23FE0FB6, 0xBD381A1E,
    0x00000000, 0x3FCFA700,
    0x5BAB0000, 0x3FCF37B1,
    0x3B7D53CE, 0x3D31A1FC,
    0x00000000, 0x3FCF3800,
    0x2BBC8000, 0x3FCFFBFC,
    0xF20ED3D2, 0xBD2FF229,
    0x00000000, 0x3FCF3800,
    0x2BBC8000, 0x3FCFFBFC,
    0xF20ED3D2, 0xBD2FF229,
    0x00000000, 0x3FCEC900,
    0xE84FD000, 0x3FD06182,
    0x38BC2CA6, 0x3D32AF65,
    0x00000000, 0x3FCEC900,
    0xE84FD000, 0x3FD06182,
    0x38BC2CA6, 0x3D32AF65,
    0x00000000, 0x3FCE5A00,
    0x1D6AC000, 0x3FD0C671,
    0xE59E7D9C, 0xBD242D6A,
    0x00000000, 0x3FCDEB00,
    0x1B9CA000, 0x3FD12CD3,
    0xAA0FA56F, 0xBD380449,
    0x00000000, 0x3FCDEB00,
    0x1B9CA000, 0x3FD12CD3,
    0xAA0FA56F, 0xBD380449,
    0x00000000, 0x3FCD7C00,
    0xBDEF7000, 0x3FD194B3,
    0x6FBDEE94, 0xBD31881D,
    0x00000000, 0x3FCD7C00,
    0xBDEF7000, 0x3FD194B3,
    0x6FBDEE94, 0xBD31881D,
    0x00000000, 0x3FCD0D00,
    0x5AF2C000, 0x3FD1FE1E,
    0xB53054C3, 0x3D140EB9,
    0x00000000, 0x3FCD0D00,
    0x5AF2C000, 0x3FD1FE1E,
    0xB53054C3, 0x3D140EB9,
    0x00000000, 0x3FCC9E00,
    0xCC29F000, 0x3FD2691E,
    0x4DAAE9F9, 0x3CF09CA5,
    0x00000000, 0x3FCC9E00,
    0xCC29F000, 0x3FD2691E,
    0x4DAAE9F9, 0x3CF09CA5,
    0x00000000, 0x3FCC2F00,
    0x760B8000, 0x3FD2D5C1,
    0xBE0F08BF, 0x3D3AEADE,
    0x00000000, 0x3FCC2F00,
    0x760B8000, 0x3FD2D5C1,
    0xBE0F08BF, 0x3D3AEADE,
    0x00000000, 0x3FCBC000,
    0x509F8000, 0x3FD34413,
    0xB83B532A, 0xBD380433,

    0x509F8000, 0x3FD34413,
    0xB83B532A, 0xBD380433,

    0x00000040, 0x42D00000,

    0x00000000, 0x41600000,

    0x00000000, 0x3F780000,

    0x00000000, 0x43B00000,

    0x00000000, 0x00000000,
    0x00000000, 0x3FF00000,
    0x00000000, 0x3FDBC000,

    0xBF2E4107, 0x3F5A7A6C,
    0xDC77B115, 0xBFF27AF2,
    0xDC963A31, 0x3FFC6A02,
    0x64D42479, 0xC0089309,
    0x7B1EFE5C, 0x4016AB9F,
    0x698CFB68, 0xC025C90E,
    0xE1B1AA78, 0x40358992,
    0x4E75EEA2, 0xC045B321,
};

__attribute__((always_inline))
inline int __internal_dlog10_la_cout (double *a, double *r)
{
    double x, y, u, q;
    double dbP;
    double dbAbsU;
    double dbN, dbNLg2Hi, dbNLg2Lo;
    double dbB, dbLgRcprYHi, dbLgRcprYLo, dbWHi, dbWLo;
    double dbYHi, dbYLo, dbUHi, dbULo, dbResHi, dbResLo;
    double dbQHi, dbQLo;
    double dbTmp;
    int iN, j;
    int i;
    int nRet = 0;

    if ((((((_iml_v2_dp_union_t *) & (*a))->dwords.hi_dword >> 20) & 0x7FF) != 0x7FF))
    {

        x = (*a);
        iN = 0;

        if (((((_iml_v2_dp_union_t *) & x)->dwords.hi_dword >> 20) & 0x7FF) == 0)
        {

            x = (x * ((__constant double *) __dlog10_la_CoutTab)[200]);
            iN = (iN - 60);
        }

        if (x > ((__constant double *) __dlog10_la_CoutTab)[201])
        {

            u = (x - 1.0);
            dbAbsU = u;
            (((_iml_v2_dp_union_t *) & dbAbsU)->dwords.hi_dword =
             (((_iml_v2_dp_union_t *) & dbAbsU)->dwords.hi_dword & 0x7FFFFFFF) | ((_iml_uint32_t) (0) << 31));

            if (dbAbsU > ((__constant double *) __dlog10_la_CoutTab)[199])
            {

                iN = (iN + ((((_iml_v2_dp_union_t *) & x)->dwords.hi_dword >> 20) & 0x7FF) - 0x3FF);
                dbN = (double) iN;

                dbNLg2Hi = (dbN * ((__constant double *) __dlog10_la_CoutTab)[195]);
                dbNLg2Lo = (dbN * ((__constant double *) __dlog10_la_CoutTab)[196]);

                y = x;
                (((_iml_v2_dp_union_t *) & y)->dwords.hi_dword =
                 (((_iml_v2_dp_union_t *) & y)->dwords.hi_dword & 0x800FFFFF) | (((_iml_uint32_t) (0x3FF) & 0x7FF) << 20));

                dbTmp = (y + ((__constant double *) __dlog10_la_CoutTab)[197]);
                j = ((((_iml_v2_dp_union_t *) & dbTmp)->dwords.lo_dword) & ((1 << (6 + 1)) - 1));

                dbB = ((__constant double *) __dlog10_la_CoutTab)[3 * (j)];
                dbLgRcprYHi = ((__constant double *) __dlog10_la_CoutTab)[3 * (j) + 1];
                dbLgRcprYLo = ((__constant double *) __dlog10_la_CoutTab)[3 * (j) + 2];

                dbWHi = (dbNLg2Hi + dbLgRcprYHi);
                dbWLo = (dbNLg2Lo + dbLgRcprYLo);

                dbTmp = (y + ((__constant double *) __dlog10_la_CoutTab)[198]);
                dbYHi = (dbTmp - ((__constant double *) __dlog10_la_CoutTab)[198]);
                dbYLo = (y - dbYHi);

                dbQHi = ((dbB * dbYHi) - ((__constant double *) __dlog10_la_CoutTab)[203]);
                dbQLo = (dbB * dbYLo);
                q = (dbQHi + dbQLo);

                dbP =
                    (((((((((__constant double *) __dlog10_la_CoutTab)[211] * q + ((__constant double *) __dlog10_la_CoutTab)[210]) * q +
                          ((__constant double *) __dlog10_la_CoutTab)[209]) * q + ((__constant double *) __dlog10_la_CoutTab)[208]) * q +
                        ((__constant double *) __dlog10_la_CoutTab)[207]) * q + ((__constant double *) __dlog10_la_CoutTab)[206]) * q +
                      ((__constant double *) __dlog10_la_CoutTab)[205]) * q + ((__constant double *) __dlog10_la_CoutTab)[204]);

                dbResHi = (dbWHi + dbQHi);
                dbResLo = (dbWLo + (dbP * dbQLo));
                dbResLo = (dbResLo + dbQLo);
                dbResLo = (dbResLo + dbP * dbQHi);

                (*r) = (dbResHi + dbResLo);
            }
            else
            {

                q = (u * ((__constant double *) __dlog10_la_CoutTab)[203]);

                dbP =
                    (((((((((__constant double *) __dlog10_la_CoutTab)[211] * q + ((__constant double *) __dlog10_la_CoutTab)[210]) * q +
                          ((__constant double *) __dlog10_la_CoutTab)[209]) * q + ((__constant double *) __dlog10_la_CoutTab)[208]) * q +
                        ((__constant double *) __dlog10_la_CoutTab)[207]) * q + ((__constant double *) __dlog10_la_CoutTab)[206]) * q +
                      ((__constant double *) __dlog10_la_CoutTab)[205]) * q + ((__constant double *) __dlog10_la_CoutTab)[204]);

                dbP = (dbP * q);
                dbP = (dbP + q);

                (*r) = dbP;
            }
        }
        else
        {

            if (x == ((__constant double *) __dlog10_la_CoutTab)[201])
            {

                (*r) = -((__constant double *) __dlog10_la_CoutTab)[202] / ((__constant double *) __dlog10_la_CoutTab)[201];
                nRet = 2;
            }
            else
            {

                (*r) = ((__constant double *) __dlog10_la_CoutTab)[201] / ((__constant double *) __dlog10_la_CoutTab)[201];
                nRet = 1;
            }
        }
    }
    else
    {

        if (((((_iml_v2_dp_union_t *) & (*a))->dwords.hi_dword >> 31) == 1)
            && (((((_iml_v2_dp_union_t *) & (*a))->dwords.hi_dword & 0x000FFFFF) == 0) && ((((_iml_v2_dp_union_t *) & (*a))->dwords.lo_dword) == 0)))
        {

            (*r) = ((__constant double *) __dlog10_la_CoutTab)[201] / ((__constant double *) __dlog10_la_CoutTab)[201];
            nRet = 1;
        }
        else
        {

            (*r) = (*a) * (*a);
        }
    }

    return nRet;
}

double __ocl_svml_log10_v2 (double a)
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
        double dR;
        unsigned long Index;
        double THL[2];
        double L2H;
        double L2L;
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
        double L2;
        double BMask;
        double DBias;
        double dK;
        double dT;
        double P45;
        double P23;
        double P1;

        ExpMask = as_double (__internal_dlog10_la_data.ExpMask);
        Two10 = as_double (__internal_dlog10_la_data.Two10);

        Mantissa = as_double ((as_ulong (va1) & as_ulong (ExpMask)));
        Mantissa = as_double ((as_ulong (Mantissa) | as_ulong (Two10)));

        MinNorm = as_double (__internal_dlog10_la_data.MinNorm);
        MaxNorm = as_double (__internal_dlog10_la_data.MaxNorm);

        DblRcp = ((double) (1.0f / ((float) (Mantissa))));

        Expon = as_ulong (va1);
        Expon = ((unsigned long) (Expon) >> (52 - 32));
        IExpon = ((unsigned int) ((unsigned long) Expon >> 32));

        BrMask1 = as_double ((unsigned long) ((va1 < MinNorm) ? 0xffffffffffffffff : 0x0));
        BrMask2 = as_double ((unsigned long) (((!(va1 <= MaxNorm)) ? 0xffffffffffffffff : 0x0)));

        CLH = as_double (__internal_dlog10_la_data.One);

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

        HalfMask = as_double (__internal_dlog10_la_data.HalfMask);
        {
            double t_HMant;
            double t_LMant;
            t_HMant = as_double ((as_ulong (Mantissa) & as_ulong (HalfMask)));
            t_LMant = (Mantissa - t_HMant);
            t_HMant = (t_HMant * DblRcp);
            t_LMant = (t_LMant * DblRcp);
            dR = (t_HMant - CLH);
            dR = (dR + t_LMant);
        };

        Index = as_ulong (DblRcp);
        Index = ((unsigned long) (Index) >> (52 - 9 - 3));
        dT = as_double (((__constant unsigned long *) ((__constant char *) (&__internal_dlog10_la_data.Log_LA_table[0]) - 0x408000))[Index >> 3]);
        Threshold = as_double (__internal_dlog10_la_data.Threshold);
        Bias = as_double (__internal_dlog10_la_data.Bias);
        Bias1 = as_double (__internal_dlog10_la_data.Bias1);
        L2 = as_double (__internal_dlog10_la_data.L2);
        BMask = as_double ((unsigned long) ((Threshold < DblRcp) ? 0xffffffffffffffff : 0x0));
        DBias = as_double ((as_ulong (BMask) & as_ulong (Bias)));
        DBias = as_double ((as_ulong (DBias) | as_ulong (Bias1)));;
        FpExpon = (FpExpon - DBias);
        dK = (FpExpon * L2);
        poly_coeff[5] = as_double (__internal_dlog10_la_data.poly_coeff[0]);
        poly_coeff[4] = as_double (__internal_dlog10_la_data.poly_coeff[1]);
        poly_coeff[3] = as_double (__internal_dlog10_la_data.poly_coeff[2]);
        poly_coeff[2] = as_double (__internal_dlog10_la_data.poly_coeff[3]);
        poly_coeff[1] = as_double (__internal_dlog10_la_data.poly_coeff[4]);
        P45 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly_coeff[5], dR, poly_coeff[4]);
        P23 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly_coeff[3], dR, poly_coeff[2]);
        R2 = (dR * dR);
        P1 = (dR * poly_coeff[1]);
        dP = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (P45, R2, P23);
        dP = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dP, R2, P1);
        dR = (dP + dT);
        vr1 = (dR + dK);
    }

    if ((vm) != 0)
    {
        double _vapi_arg1[1];
        double _vapi_res1[1];
        ((double *) _vapi_arg1)[0] = va1;
        ((double *) _vapi_res1)[0] = vr1;
        __internal_dlog10_la_cout (_vapi_arg1, _vapi_res1);
        vr1 = ((double *) _vapi_res1)[0];
    };
    r = vr1;;

    return r;

}

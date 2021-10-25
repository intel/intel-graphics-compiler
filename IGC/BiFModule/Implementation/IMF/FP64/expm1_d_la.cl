/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF
typedef struct
{
    unsigned long Exp_tbl_H[16];
    unsigned long Exp_tbl_L[16];
    unsigned long L2E;
    unsigned long Shifter;
    unsigned long Threshold;
    unsigned long SgnMask;
    unsigned long L2H;
    unsigned long L2L;
    unsigned long ZThres;
    unsigned long EMask;
    unsigned long poly_coeff8;
    unsigned long poly_coeff7;
    unsigned long poly_coeff6;
    unsigned long poly_coeff5;
    unsigned long poly_coeff4;
    unsigned long poly_coeff3;
    unsigned long poly_coeff2;
    unsigned long One;
    unsigned long MOne;
} __internal_dexpm1_la_data_avx512_t;
static __constant __internal_dexpm1_la_data_avx512_t __internal_dexpm1_la_data_avx512 = {
    {
     0x3ff0000000000000uL, 0x3ff0b5586cf9890fuL, 0x3ff172b83c7d517buL, 0x3ff2387a6e756238uL, 0x3ff306fe0a31b715uL, 0x3ff3dea64c123422uL,
     0x3ff4bfdad5362a27uL, 0x3ff5ab07dd485429uL, 0x3ff6a09e667f3bcduL, 0x3ff7a11473eb0187uL, 0x3ff8ace5422aa0dbuL, 0x3ff9c49182a3f090uL,
     0x3ffae89f995ad3aduL, 0x3ffc199bdd85529cuL, 0x3ffd5818dcfba487uL, 0x3ffea4afa2a490dauL}
    , {
       0x0000000000000000uL, 0x3c979aa65d837b6duL, 0xbc801b15eaa59348uL, 0x3c968efde3a8a894uL, 0x3c834d754db0abb6uL, 0x3c859f48a72a4c6duL,
       0x3c7690cebb7aafb0uL, 0x3c9063e1e21c5409uL, 0xbc93b3efbf5e2228uL, 0xbc7b32dcb94da51duL, 0x3c8db72fc1f0eab4uL, 0x3c71affc2b91ce27uL,
       0x3c8c1a7792cb3387uL, 0x3c736eae30af0cb3uL, 0x3c74a385a63d07a7uL, 0xbc8ff7128fd391f0uL}

    , 0x3ff71547652B82FEuL, 0x42f8000000003ff0uL, 0x40861DA04CBAFE44uL, 0x8000000000000000uL, 0x3fe62e42fefa39efuL, 0x3c7abc9e3b39803fuL,
        0xc060000000000000uL, 0xbfffffffffffffffuL, 0x3efa01f8f4be0bb2uL, 0x3f2a020410303d8auL, 0x3f56c1c38e164a2fuL, 0x3f81111110865214uL,
        0x3fa5555554ad3d06uL, 0x3fc5555555555656uL, 0x3fe00000000000a2uL, 0x3ff0000000000000uL, 0xbff0000000000000uL
};

typedef struct
{

    unsigned long Expm1_HA_table[(1 << 8)];

    unsigned long poly_coeff[4];
    unsigned long Log2e;
    unsigned long L2H;
    unsigned long L2L;
    unsigned long ExpAddConst;
    unsigned long IndexMask;
    unsigned long ExpMask;
    unsigned long HalfMask;
    unsigned long MOne;
    unsigned long AbsMask;
    unsigned long Threshold;
    unsigned long L2;
    unsigned long ExpAddConst2;
    unsigned long IndexMask2;
    unsigned long ExpMask2;
} __internal_dexpm1_la_data_t;
static __constant __internal_dexpm1_la_data_t __internal_dexpm1_la_data = {

    {
     0x0000000000000000uL, 0x0000000000000000uL, 0x0000163da8000000uL, 0x3e3fb33356d84a67uL, 0x00002c9a40000000uL, 0xbe3887f9f1190835uL,
     0x00004315e8000000uL, 0x3e1b9fe12f5ce3e7uL, 0x000059b0d0000000uL, 0x3e48ac2ba1d73e2auL, 0x0000706b28000000uL, 0x3e3ddf6ddc6dc404uL,
     0x0000874518000000uL, 0x3e1d66f20230d7c9uL, 0x00009e3ec8000000uL, 0x3e46379c1a290f03uL, 0x0000b55870000000uL, 0xbe4833b784eb3a37uL,
     0x0000cc9228000000uL, 0x3e4b923fba03db83uL, 0x0000e3ec30000000uL, 0x3e469e8d10103a17uL, 0x0000fb66b0000000uL, 0xbdb2ce50dcdf6e22uL,
     0x00011301d0000000uL, 0x3df25b50a4ebbf1buL, 0x00012abdc0000000uL, 0x3e1b0c72fee4aeb5uL, 0x0001429ab0000000uL, 0xbe356d2204cbefe7uL,
     0x00015a98c8000000uL, 0x3e24b1ca24901aaeuL, 0x000172b840000000uL, 0xbe4c15742919041cuL, 0x00018af938000000uL, 0x3e2191bd3777ee17uL,
     0x0001a35be8000000uL, 0x3e4b7e5ba9e5b4c8uL, 0x0001bbe088000000uL, 0xbe4fdd19632a70c7uL, 0x0001d48730000000uL, 0x3e368b9aa7805b80uL,
     0x0001ed5020000000uL, 0x3e47e6c8e5c40d00uL, 0x0002063b88000000uL, 0x3e18a3358ee3bac1uL, 0x00021f4990000000uL, 0x3e37ddc962552fd3uL,
     0x0002387a70000000uL, 0xbe38a9dc7993e052uL, 0x000251ce50000000uL, 0xbe135670329f5521uL, 0x00026b4568000000uL, 0xbe40ec1916d42cc6uL,
     0x000284dfe0000000uL, 0x3e3f5638096cf15duL, 0x00029e9df8000000uL, 0xbe470108f69ed175uL, 0x0002b87fd0000000uL, 0x3e2b5b31ffbbd48duL,
     0x0002d285a8000000uL, 0xbe31bfcf4bff6e2buL, 0x0002ecafa8000000uL, 0x3e33e2f5611ca0f4uL, 0x000306fe08000000uL, 0x3e418db8a96f46aduL,
     0x0003217100000000uL, 0xbe4d993e76563187uL, 0x00033c08b0000000uL, 0x3e4320b7fa64e431uL, 0x000356c560000000uL, 0xbe1b5803cdae772euL,
     0x000371a738000000uL, 0xbe28aac6ab1d7560uL, 0x00038cae70000000uL, 0xbe47d13cd3d2b1a8uL, 0x0003a7db38000000uL, 0xbe48d30048af21b7uL,
     0x0003c32dc0000000uL, 0x3e489d47242000f9uL, 0x0003dea650000000uL, 0xbe4f6e5eee525f6fuL, 0x0003fa4508000000uL, 0xbe4a9bff22fa047fuL,
     0x0004160a20000000uL, 0x3e3f72e29f84325cuL, 0x000431f5d8000000uL, 0x3e350a896dc70444uL, 0x00044e0860000000uL, 0x3e18624b40c4dbd0uL,
     0x00046a41f0000000uL, 0xbe4717fd446d7686uL, 0x000486a2b8000000uL, 0xbe41f6197f61f2e2uL, 0x0004a32af0000000uL, 0x3e2afa7bcce5b17auL,
     0x0004bfdad8000000uL, 0xbe464eaec715e343uL, 0x0004dcb298000000uL, 0x3e3fddd0d63b36efuL, 0x0004f9b278000000uL, 0xbe362d35952cc275uL,
     0x000516daa0000000uL, 0x3e467b320e0897a9uL, 0x0005342b58000000uL, 0xbe362b07e20f57c4uL, 0x000551a4c8000000uL, 0x3e42ec9076297631uL,
     0x00056f4738000000uL, 0xbe34ad8259913500uL, 0x00058d12d8000000uL, 0xbe4b41c016d6a1eauL, 0x0005ab07e0000000uL, 0xbe45bd5eb539b67fuL,
     0x0005c92688000000uL, 0x3e42ca35b80e258euL, 0x0005e76f18000000uL, 0xbe4296f5bc8b20dauL, 0x000605e1b8000000uL, 0x3e376dc08b076f59uL,
     0x0006247eb0000000uL, 0x3e0d2ac258f87d03uL, 0x0006434638000000uL, 0xbe4999e701c483c7uL, 0x0006623880000000uL, 0x3e42a91124893ecfuL,
     0x00068155d8000000uL, 0xbe4d9ab467bf1d47uL, 0x0006a09e68000000uL, 0xbe380c4336f74d05uL, 0x0006c01278000000uL, 0xbe47a12a08944ab3uL,
     0x0006dfb240000000uL, 0xbe4cd72e886ef8eauL, 0x0006ff7df8000000uL, 0x3e3519483cf87e1buL, 0x00071f75e8000000uL, 0x3e2d8bee7ba46e1euL,
     0x00073f9a48000000uL, 0x3e24b02e77ab934auL, 0x00075feb58000000uL, 0xbe3bd98374091656uL, 0x0007806950000000uL, 0xbe00d1604f328fecuL,
     0x0007a11470000000uL, 0x3e4f580c36bea881uL, 0x0007c1ed00000000uL, 0x3e330c1327c49334uL, 0x0007e2f338000000uL, 0xbe330b19defa2fd4uL,
     0x0008042758000000uL, 0xbe4e0f2f724f90ccuL, 0x0008258998000000uL, 0x3e34cce128acf88buL, 0x0008471a48000000uL, 0xbe3dc385331ad094uL,
     0x000868d998000000uL, 0x3e4a2497640720eduL, 0x00088ac7d8000000uL, 0x3e38a669966530bduL, 0x0008ace540000000uL, 0x3e415506dadd3e2buL,
     0x0008cf3218000000uL, 0xbe34abb7410d55e3uL, 0x0008f1ae98000000uL, 0x3e31577362b98274uL, 0x0009145b08000000uL, 0x3e4c8ffe2c4530dauL,
     0x00093737b0000000uL, 0x3e29b8bc9e8a0388uL, 0x00095a44c8000000uL, 0x3e4e4290774da41buL, 0x00097d82a0000000uL, 0xbe00d8d83a30b6f8uL,
     0x0009a0f170000000uL, 0x3e2940f737462137uL, 0x0009c49180000000uL, 0x3e451f8480e3e236uL, 0x0009e86318000000uL, 0x3e3e323231824ca8uL,
     0x000a0c6678000000uL, 0x3e4aef2b2594d6d4uL, 0x000a309bf0000000uL, 0xbe4dae966539f470uL, 0x000a5503b0000000uL, 0x3e41f12ae45a1225uL,
     0x000a799e10000000uL, 0x3e49859ac3796fd9uL, 0x000a9e6b58000000uL, 0xbe44301205e0a6deuL, 0x000ac36bc0000000uL, 0xbe0606431f9234cbuL,
     0x000ae89f98000000uL, 0x3e35ad3ad5e8734duL, 0x000b0e0728000000uL, 0x3e38db66590842aduL, 0x000b33a2b8000000uL, 0x3e13c57ebdaff43auL,
     0x000b597290000000uL, 0xbe40d536338e3bf7uL, 0x000b7f76f0000000uL, 0x3e47daf237553d84uL, 0x000ba5b030000000uL, 0x3e2420c930819679uL,
     0x000bcc1e90000000uL, 0x3e12f074891ee83duL, 0x000bf2c258000000uL, 0x3e4eb8f0442046b8uL, 0x000c199be0000000uL, 0xbe43d56b1eeef9a7uL,
     0x000c40ab60000000uL, 0xbd87c2c975903ef8uL, 0x000c67f130000000uL, 0xbe3a82eb4b5dec80uL, 0x000c8f6d98000000uL, 0xbe4fc8c257729a1euL,
     0x000cb720e0000000uL, 0xbe48837cb757e1a1uL, 0x000cdf0b58000000uL, 0xbe4511e031dd83b5uL, 0x000d072d48000000uL, 0x3e403c4bdc687918uL,
     0x000d2f8708000000uL, 0x3deb13e315bc2473uL, 0x000d5818e0000000uL, 0xbe4822dbc6d12fd3uL, 0x000d80e318000000uL, 0xbe3367c68447b063uL,
     0x000da9e600000000uL, 0x3e4ed9942b84600duL, 0x000dd321f0000000uL, 0x3e480da3025b4aefuL, 0x000dfc9730000000uL, 0x3e4bdcdaf5cb4656uL,
     0x000e264618000000uL, 0xbe4852f6baf6c4f0uL, 0x000e502ee8000000uL, 0xbe1d30027630bb40uL, 0x000e7a51f8000000uL, 0x3e4e3a641a5aa459uL,
     0x000ea4afa0000000uL, 0x3e452486cc2c7b9duL, 0x000ecf4830000000uL, 0xbe438cc07b927e77uL, 0x000efa1bf0000000uL, 0xbe39ea5d888e02deuL,
     0x000f252b38000000uL, 0xbe2288ad162f2d20uL, 0x000f507658000000uL, 0x3e4b722a033a7c26uL, 0x000f7bfdb0000000uL, 0xbe431a0f63b7625auL,
     0x000fa7c180000000uL, 0x3e39e90d82e90a7euL, 0x000fd3c228000000uL, 0x3e4c7b8f884badd2uL}
    , {
       0x3f81111168877F38uL, 0x3fa55555C2A9C0F3uL, 0x3fc555555555541DuL, 0x3fdFFFFFFFFFFE5CuL}

    , 0x40671547652B82FEuL, 0x3f762e42fef80000uL, 0x3d41cf79abc9e3b4uL, 0x42f80000001ff800uL, 0x00000000000007f0uL, 0x00000000003ff800uL,
        0xfffffffff8000000uL, 0xbff0000000000000uL, 0x7fffffffffffffffuL, 0x40861DA04CBAFE43uL, 0x3f762e42fefa39efuL, 0x43080000000ffc00uL,
        0x00000000000003f8uL, 0x00000000001ffc00uL
};

static __constant int_double __dexpm1_la_Tbl_exp[] = {
    {0x0000000000000000ull}, {0x0000000000000000ull},
    {0x0000d9b0d3158574ull}, {0x3c8cd2523567f613ull},
    {0x0001b5586cf9890full}, {0x3c979aa65d837b6dull},
    {0x00009301d0125b51ull}, {0xbc9556522a2fbd0eull},
    {0x000372b83c7d517bull}, {0xbc801b15eaa59348ull},
    {0x000354873168b9aaull}, {0x3c9aecf73e3a2f60ull},
    {0x0001387a6e756238ull}, {0x3c968efde3a8a894ull},
    {0x00011e9df51fdee1ull}, {0x3c82f7e16d09ab31ull},
    {0x000706fe0a31b715ull}, {0x3c834d754db0abb6ull},
    {0x0007f1a7373aa9cbull}, {0xbc924aedcc4b5069ull},
    {0x0006dea64c123422ull}, {0x3c859f48a72a4c6dull},
    {0x0001ce086061892dull}, {0x3c4363ed60c2ac12ull},
    {0x0002bfdad5362a27ull}, {0x3c7690cebb7aafb0ull},
    {0x0003b42b569d4f82ull}, {0xbc78dec6bd0f3860ull},
    {0x0002ab07dd485429ull}, {0x3c9063e1e21c5409ull},
    {0x0001a47eb03a5585ull}, {0xbc8c33c53bef4da8ull},
    {0x000ea09e667f3bcdull}, {0xbc93b3efbf5e2229ull},
    {0x000f9f75e8ec5f74ull}, {0xbc781f647e5a3eceull},
    {0x000ea11473eb0187ull}, {0xbc7b32dcb94da51dull},
    {0x0001a589994cce13ull}, {0xbc9369b6f13b3734ull},
    {0x0002ace5422aa0dbull}, {0x3c8db72fc1f0eab5ull},
    {0x0003b737b0cdc5e5ull}, {0xbc5da9b88b6c1e29ull},
    {0x0002c49182a3f090ull}, {0x3c71affc2b91ce27ull},
    {0x0001d503b23e255dull}, {0xbc91bbd1d3bcbb15ull},
    {0x0006e89f995ad3adull}, {0x3c8c1a7792cb3386ull},
    {0x0007ff76f2fb5e47ull}, {0xbc68d6f438ad9334ull},
    {0x0001199bdd85529cull}, {0x3c736eae30af0cb3ull},
    {0x00013720dcef9069ull}, {0x3c676b2c6c921967ull},
    {0x00035818dcfba487ull}, {0x3c74a385a63d07a8ull},
    {0x00037c97337b9b5full}, {0xbc82d52107b43e20ull},
    {0x0001a4afa2a490daull}, {0xbc8ff7128fd391f0ull},
    {0x0000d0765b6e4540ull}, {0x3c8a64a931d185eeull},

};

static __constant int_double __dexpm1_la_dc5 = { 0x3f56c17256147174UL };
static __constant int_double __dexpm1_la_dc4 = { 0x3f811115c0928f3bUL };
static __constant int_double __dexpm1_la_dc3 = { 0x3fa5555555547138UL };
static __constant int_double __dexpm1_la_dc2 = { 0x3fc5555555547d38UL };
static __constant int_double __dexpm1_la_dc1 = { 0x3fe0000000000000UL };
static __constant int_double __dexpm1_la_dc0 = { 0x3c6712f33cb068a3UL };

static __constant int_float __dexpm1_la_fL2E = { 0x3FB8AA3Bu };
static __constant int_float __dexpm1_la_fShifter = { 0x48c07fe0u };

static __constant int_double __dexpm1_la_p_NL2H = { 0xbfe62e42fefa39efUL };

static __constant int_double __dexpm1_la_p_NL2L = { 0xbc7abc9e3b39803fUL };

__attribute__((always_inline))
inline int __internal_dexpm1_la_cout (double *pxin, double *pres)
{
    int nRet = 0;
    double xin = *pxin;
    int_double x, T, Tlr, sc, xa, res;
    double dN, R, poly, Th, poly_t;
    int_float x0f, fN, fR, fS;
    int index;

    x0f.f = (float) xin;

    fS.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (x0f.f, __dexpm1_la_fL2E.f, __dexpm1_la_fShifter.f);

    fN.f = fS.f - __dexpm1_la_fShifter.f;

    dN = (double) fN.f;
    R = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dN, __dexpm1_la_p_NL2H.f, xin);
    R = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dN, __dexpm1_la_p_NL2L.f, R);

    index = (fS.w & 0x1f) << 1;

    T.w32[1] = (fS.w << (20 - 5)) ^ __dexpm1_la_Tbl_exp[index].w32[1];
    T.w32[0] = __dexpm1_la_Tbl_exp[index].w32[0];

    Tlr.w32[1] = __dexpm1_la_Tbl_exp[index + 1].w32[1];
    Tlr.w32[0] = 0;

    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (__dexpm1_la_dc5.f, R, __dexpm1_la_dc4.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dexpm1_la_dc3.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dexpm1_la_dc2.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dexpm1_la_dc1.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, 1.0);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, Tlr.f);

    Th = T.f - 1.0;

    res.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (T.f, poly, Th);

    if (SPIRV_OCL_BUILTIN(fabs, _f32, ) (x0f.f) <= 708.0f)
    {
        *pres = res.f;
        return nRet;
    }

    if (x0f.f < 0)
    {
        *pres = -1.0;
        return nRet;
    }

    if (!(x0f.f < 1024.0f))
    {

        x.f = xin;
        xa.w = x.w & 0x7fffffffffffffffUL;
        if (xa.w > 0x7ff0000000000000UL)
        {
            *pres = x.f + res.f;
            return nRet;
        }

        res.w = (res.w & 0x0007ffffffffffffUL) | 0x7fd0000000000000UL;
        res.f = res.f * xin;
        nRet = 3;
        {
            *pres = res.f;
            return nRet;
        }
    }

    T.w32[1] = ((fS.w - 512 * 32) << (20 - 5)) ^ __dexpm1_la_Tbl_exp[index].w32[1];
    T.w32[0] = __dexpm1_la_Tbl_exp[index].w32[0];

    res.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (T.f, poly, T.f);
    sc.w = 0x5ff0000000000000UL;
    res.f *= sc.f;

    if (res.w == 0x7ff0000000000000UL)
        nRet = 3;

    *pres = res.f;

    return nRet;
}

double __ocl_svml_expm1 (double a)
{

    double va1;
    double vr1;
    unsigned int vm;

    double r;

    va1 = a;;

    __internal_dexpm1_la_cout (&va1, &vr1);
    r = vr1;;

    return r;

}

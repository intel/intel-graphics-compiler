/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF
typedef struct
{

    unsigned int _sC[32];
    unsigned int _sP0[32];
    unsigned int _sP1[32];
    unsigned int _sP2[32];
    unsigned int _sP3[32];
    unsigned int _sP4[32];
    unsigned int _sP5[32];
    unsigned int _sP6[32];
    unsigned int _sP7[32];
    unsigned int _iExpMantMask_UISA;
    unsigned int _iMinIdxOfsMask_UISA;
    unsigned int _iMaxIdxMask_UISA;

    unsigned int _sSignMask;
    unsigned int _sAbsMask;
    unsigned int _iExpMantMask;
    unsigned int _iExpMask;
    unsigned int _iMinIdxOfsMask;
    unsigned int _iMaxIdxMask;

} __internal_stanh_la_data_t;
static __constant __internal_stanh_la_data_t __internal_stanh_la_data = {
    {
     0x00000000u, 0x3d700000u, 0x3d900000u, 0x3db00000u,
     0x3dd00000u, 0x3df00000u, 0x3e100000u, 0x3e300000u,
     0x3e500000u, 0x3e700000u, 0x3e900000u, 0x3eb00000u,
     0x3ed00000u, 0x3ef00000u, 0x3f100000u, 0x3f300000u,
     0x3f500000u, 0x3f700000u, 0x3f900000u, 0x3fb00000u,
     0x3fd00000u, 0x3ff00000u, 0x40100000u, 0x40300000u,
     0x40500000u, 0x40700000u, 0x40900000u, 0x40b00000u,
     0x40d00000u, 0x40f00000u, 0x41100000u, 0x00000000u},
    {
     0x00000000u, 0x3d6fb9c9u, 0x3d8fc35fu, 0x3daf9169u,
     0x3dcf49abu, 0x3deee849u, 0x3e0f0ee8u, 0x3e2e4984u,
     0x3e4d2f8eu, 0x3e6bb32eu, 0x3e8c51cdu, 0x3ea96163u,
     0x3ec543f1u, 0x3edfd735u, 0x3f028438u, 0x3f18abf0u,
     0x3f2bc480u, 0x3f3bec1cu, 0x3f4f2e5bu, 0x3f613c53u,
     0x3f6ce37du, 0x3f743c4fu, 0x3f7a5febu, 0x3f7dea85u,
     0x3f7f3b3du, 0x3f7fb78cu, 0x3f7fefd4u, 0x3f7ffdd0u,
     0x3f7fffb4u, 0x3f7ffff6u, 0x3f7fffffu, 0x3f800000u},
    {
     0x00000000u, 0xb0a1501eu, 0xb11d0b9eu, 0xaf932f10u,
     0x30fa467cu, 0x31249ef0u, 0x312b9ae0u, 0xb1dd8650u,
     0xb044cc00u, 0x31f9ba98u, 0xb0ca4580u, 0xb21e2644u,
     0xb234cc14u, 0x32397534u, 0xb2d0938eu, 0xb1303240u,
     0xb2ef278au, 0x32e33e9cu, 0xb20b98c0u, 0xb2d5b8c2u,
     0x32fe70b8u, 0x32cec21au, 0x32c28572u, 0xb2230d78u,
     0xb2a2c238u, 0xb2315220u, 0xb2dddb64u, 0xb2d020d0u,
     0x321fe128u, 0xb2875bb0u, 0x32fa6146u, 0x00000000u},
    {
     0x3f800000u, 0x3f7f1f84u, 0x3f7ebd11u, 0x3f7e1e5fu,
     0x3f7d609fu, 0x3f7c842du, 0x3f7b00e5u, 0x3f789580u,
     0x3f75b8adu, 0x3f726fd9u, 0x3f6cc59bu, 0x3f63fb92u,
     0x3f59ff97u, 0x3f4f11d7u, 0x3f3d7573u, 0x3f24f360u,
     0x3f0cbfe7u, 0x3eec1a69u, 0x3eb0a801u, 0x3e6753a2u,
     0x3e132f1au, 0x3db7e7d3u, 0x3d320845u, 0x3c84d3d4u,
     0x3bc477b7u, 0x3b10d3dau, 0x3a01601eu, 0x388c1a3bu,
     0x3717b0dau, 0x35a43bceu, 0x338306c6u, 0x00000000u},
    {
     0xb0343c7bu, 0xbd6ee69du, 0xbd8f0da7u, 0xbdae477du,
     0xbdcd2a1fu, 0xbdeba80du, 0xbe0c443bu, 0xbe293cf3u,
     0xbe44f282u, 0xbe5f3651u, 0xbe81c7c0u, 0xbe96d7cau,
     0xbea7fb8eu, 0xbeb50e9eu, 0xbec12efeu, 0xbec4be92u,
     0xbebce070u, 0xbead510eu, 0xbe8ef7d6u, 0xbe4b8704u,
     0xbe083237u, 0xbdaf7449u, 0xbd2e1ec4u, 0xbc83bf06u,
     0xbbc3e0b5u, 0xbb10aadcu, 0xba0157dbu, 0xb88c18f2u,
     0xb717b096u, 0xb5a43baeu, 0xb383012cu, 0x00000000u},
    {
     0xbeaaaaa5u, 0xbeab0612u, 0xbea7f01fu, 0xbea4e120u,
     0xbea387b7u, 0xbea15962u, 0xbe9d57f7u, 0xbe976b5au,
     0xbe90230du, 0xbe880dffu, 0xbe7479b3u, 0xbe4c3d88u,
     0xbe212482u, 0xbdeb8cbau, 0xbd5e78adu, 0x3c6b5e6eu,
     0x3d839143u, 0x3dc21ee1u, 0x3de347afu, 0x3dcbec96u,
     0x3d99ef2du, 0x3d542ea1u, 0x3cdde701u, 0x3c2cca67u,
     0x3b81cb27u, 0x3ac073a1u, 0x39ac3032u, 0x383a94d9u,
     0x36ca081du, 0x355abd4cu, 0x332b3cb6u, 0x00000000u},
    {
     0xb76dd6b9u, 0xbe1c276du, 0x3c1dcf2fu, 0x3dc1a78du,
     0x3d96f985u, 0x3da2b61bu, 0x3dc13397u, 0x3dd2f670u,
     0x3df48a0au, 0x3e06c5a8u, 0x3e1a3abau, 0x3e27c405u,
     0x3e2e78d0u, 0x3e2c3e44u, 0x3e1d3097u, 0x3df4a8f4u,
     0x3da38508u, 0x3d31416au, 0x3b562657u, 0xbcaeeac9u,
     0xbcce9419u, 0xbcaaeac4u, 0xbc49e7d0u, 0xbba71dddu,
     0xbb003b0eu, 0xba3f9a05u, 0xb92c08a7u, 0xb7ba9232u,
     0xb64a0b0fu, 0xb4dac169u, 0xb2ab78acu, 0x00000000u},
    {
     0x3e0910e9u, 0x43761143u, 0x4165ecdcu, 0xc190f756u,
     0xc08c097du, 0xc02ba813u, 0xbf7f6bdau, 0x3f2b1dc0u,
     0x3ece105du, 0x3f426a94u, 0xbadb0dc4u, 0x3da43b17u,
     0xbd51ab88u, 0xbcaea23du, 0xbd3b6d8du, 0xbd6caaadu,
     0xbd795bedu, 0xbd5fdddau, 0xbd038f3bu, 0xbc1cad63u,
     0x3abb4766u, 0x3b95f10bu, 0x3b825873u, 0x3afaea66u,
     0x3a49f878u, 0x39996bf3u, 0x388f3e6cu, 0x371bb0e3u,
     0x35a8a5e6u, 0x34369b17u, 0x322487b0u, 0x00000000u},
    {
     0xbc0e2f66u, 0x460bda12u, 0x43d638efu, 0xc3e11c3eu,
     0xc2baa4e9u, 0xc249da2du, 0xc1859b82u, 0x40dd5b57u,
     0x40494640u, 0x40c730a8u, 0xbf0f160eu, 0x3e30e76fu,
     0xbea81387u, 0xbdb26a1cu, 0xbd351e57u, 0xbb4c01a0u,
     0x3c1d7bfbu, 0x3c722cd1u, 0x3c973f1cu, 0x3c33a31bu,
     0x3b862ef4u, 0x3a27b3d0u, 0xba3b5907u, 0xba0efc22u,
     0xb97f9f0fu, 0xb8c8af50u, 0xb7bdddfbu, 0xb64f2950u,
     0xb4e085b1u, 0xb3731dfau, 0xb15a1f04u, 0x00000000u},
    0x7fe00000u,
    0x3d400000u,
    0x03e00000u,
    0x80000000u,
    0x7fffffffu,
    0x7ff80000u,
    0x7f000000u,
    0x3cf80000u,
    0x04280000u,

};

static __constant _iml_v2_sp_union_t _imlsTanhHATab[2] = {
    0x3F800000,
    0xBF800000,
};

__attribute__((always_inline))
inline int __internal_stanh_la_cout (float *a, float *r)
{
    int nRet = 0;

    float sSign;

    sSign = ((__constant float *) _imlsTanhHATab)[((((_iml_v2_sp_union_t *) & a[0])->hex[0] >> 31))];

    if ((((((_iml_v2_sp_union_t *) & a[0])->hex[0] >> 23) & 0xFF) != 0xFF) || ((((_iml_v2_sp_union_t *) & a[0])->hex[0] & 0x007FFFFF) == 0))
    {

        r[0] = sSign;
    }
    else
    {

        r[0] = a[0] + a[0];
    }

    return nRet;
}

float __ocl_svml_tanhf (float a)
{

    float va1;
    float vr1;
    unsigned int vm;

    float r;

    va1 = a;;

    {

        float sPoly;

        float sP[8];
        float sC;

        float sAbsX;
        float sSignX;

        unsigned int iMaskedIn;
        unsigned int iSpecIndex;

        unsigned int iX;
        unsigned int iZero;
        unsigned int iMask1;
        unsigned int iMask2;

        unsigned int iIndex;

        float sSignMask;
        float sAbsMask;
        unsigned int iExpMantMask;
        unsigned int iExpMask;
        unsigned int iMinIdxOfsMask;
        unsigned int iMaxIdxMask;

        sSignMask = as_float (__internal_stanh_la_data._sSignMask);
        sAbsMask = as_float (__internal_stanh_la_data._sAbsMask);
        iExpMask = (__internal_stanh_la_data._iExpMask);

        iExpMantMask = (__internal_stanh_la_data._iExpMantMask_UISA);
        iMinIdxOfsMask = (__internal_stanh_la_data._iMinIdxOfsMask_UISA);
        iMaxIdxMask = (__internal_stanh_la_data._iMaxIdxMask_UISA);

        iZero = 0;

        sAbsX = as_float ((as_uint (va1) & as_uint (sAbsMask)));
        sSignX = as_float ((as_uint (va1) & as_uint (sSignMask)));

        iX = as_uint (va1);

        iMaskedIn = (iX & iExpMantMask);
        iSpecIndex = ((unsigned int) (-(signed int) ((signed int) iMaskedIn > (signed int) iExpMask)));
        vm = 0;
        vm = iSpecIndex;

        iIndex = (iMaskedIn - iMinIdxOfsMask);

        iIndex = (((int) (iIndex) > (int) (iZero)) ? iIndex : iZero);
        iIndex = (((int) (iIndex) < (int) (iMaxIdxMask)) ? iIndex : iMaxIdxMask);
        iIndex = ((unsigned int) (iIndex) >> (19));

        sC = as_float (((__constant unsigned int *) ((__constant float *) (&__internal_stanh_la_data._sC)))[iIndex >> 2]);
        sAbsX = (sAbsX - sC);

        sP[0] = as_float (((__constant unsigned int *) ((__constant float *) (&__internal_stanh_la_data._sP0)))[iIndex >> 2]);
        sP[2] = as_float (((__constant unsigned int *) ((__constant float *) (&__internal_stanh_la_data._sP2)))[iIndex >> 2]);
        sP[3] = as_float (((__constant unsigned int *) ((__constant float *) (&__internal_stanh_la_data._sP3)))[iIndex >> 2]);
        sP[4] = as_float (((__constant unsigned int *) ((__constant float *) (&__internal_stanh_la_data._sP4)))[iIndex >> 2]);

        sP[5] = as_float (((__constant unsigned int *) ((__constant float *) (&__internal_stanh_la_data._sP5)))[iIndex >> 2]);
        sP[6] = as_float (((__constant unsigned int *) ((__constant float *) (&__internal_stanh_la_data._sP6)))[iIndex >> 2]);
        sP[7] = as_float (((__constant unsigned int *) ((__constant float *) (&__internal_stanh_la_data._sP7)))[iIndex >> 2]);
        sPoly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sP[7], sAbsX, sP[6]);
        sPoly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sPoly, sAbsX, sP[5]);
        sPoly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sPoly, sAbsX, sP[4]);
        sPoly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sPoly, sAbsX, sP[3]);
        sPoly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sPoly, sAbsX, sP[2]);
        sPoly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sPoly, sAbsX, sP[0]);
        vr1 = as_float ((as_uint (sPoly) | as_uint (sSignX)));
    }

    if ((vm) != 0)
    {
        float _vapi_arg1[1];
        float _vapi_res1[1];
        ((float *) _vapi_arg1)[0] = va1;
        ((float *) _vapi_res1)[0] = vr1;
        __internal_stanh_la_cout (_vapi_arg1, _vapi_res1);
        vr1 = ((float *) _vapi_res1)[0];
    };
    r = vr1;;

    return r;

}

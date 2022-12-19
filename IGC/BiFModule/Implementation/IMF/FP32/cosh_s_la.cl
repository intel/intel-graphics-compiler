/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF
typedef struct
{

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

    unsigned int _sShifter;
    unsigned int _iIndexMask;
    unsigned int _iDomainRange;

    unsigned int _sPC1;
    unsigned int _sPC2;
    unsigned int _sPC3;
    unsigned int _sPC4;

    unsigned int _sPC5;
    unsigned int _sPC6;

    unsigned int _iHalf;

} __internal_scosh_la_data_t;

static __constant __internal_scosh_la_data_t __internal_scosh_la_data = {
    {
     0x3f000000u, 0x3f02cd87u, 0x3f05aac3u, 0x3f08980fu,
     0x3f0b95c2u, 0x3f0ea43au, 0x3f11c3d3u, 0x3f14f4f0u,
     0x3f1837f0u, 0x3f1b8d3au, 0x3f1ef532u, 0x3f227043u,
     0x3f25fed7u, 0x3f29a15bu, 0x3f2d583fu, 0x3f3123f6u,
     0x3f3504f3u, 0x3f38fbafu, 0x3f3d08a4u, 0x3f412c4du,
     0x3f45672au, 0x3f49b9beu, 0x3f4e248cu, 0x3f52a81eu,
     0x3f5744fdu, 0x3f5bfbb8u, 0x3f60ccdfu, 0x3f65b907u,
     0x3f6ac0c7u, 0x3f6fe4bau, 0x3f75257du, 0x3f7a83b3u},
    {
     0x00000000u, 0xb2cea7a9u, 0x32cf9891u, 0xb2feda4bu,
     0xb1e0aba1u, 0xb2e97465u, 0x32e75624u, 0xb2ae0212u,
     0x32a31b71u, 0xb28c5563u, 0x32c12342u, 0x3043125au,
     0xb2ac9d5eu, 0xb2962b08u, 0xb1adeaf6u, 0xb2fc5aa8u,
     0x324fe77au, 0x328ec5f7u, 0xb2c14fe8u, 0xb256663eu,
     0x318aa837u, 0xb2f323a2u, 0x31a8fc24u, 0xb2dc1daau,
     0xb254a58au, 0xb2d04a1cu, 0xb19eab59u, 0xb1c41be6u,
     0xb1c116deu, 0xb2c8464au, 0x31a92436u, 0xb2123758u},
    {
     0x3f000000u, 0x3efa83b3u, 0x3ef5257du, 0x3eefe4bau,
     0x3eeac0c7u, 0x3ee5b907u, 0x3ee0ccdfu, 0x3edbfbb8u,
     0x3ed744fdu, 0x3ed2a81eu, 0x3ece248cu, 0x3ec9b9beu,
     0x3ec5672au, 0x3ec12c4du, 0x3ebd08a4u, 0x3eb8fbafu,
     0x3eb504f3u, 0x3eb123f6u, 0x3ead583fu, 0x3ea9a15bu,
     0x3ea5fed7u, 0x3ea27043u, 0x3e9ef532u, 0x3e9b8d3au,
     0x3e9837f0u, 0x3e94f4f0u, 0x3e91c3d3u, 0x3e8ea43au,
     0x3e8b95c2u, 0x3e88980fu, 0x3e85aac3u, 0x3e82cd87u},
    0x48c00000u,
    0x0000001fu,
    0x42AEAC4Eu,
    0x3F800000u,
    0x3f00010fu,
    0x3e2aaacdu,

    0x3FB8AA3Bu,
    0x32A57060u,
    0x3F317000u,
    0x3805fdf4u,
    0x80000000u,
    0x3f800000u,
    0xbf800000u,
    0x7f800000u,
    0x4b400000u,
    0x0000001fu,
    0x42AEAC4Eu,

    0x3F800000u,
    0x3f000000u,
    0x3e2aaa57u,
    0x3d2aaa72u,
    0x3c091461u,
    0x3ab6a8a3u,

    0x3f000000u
};

static __constant union
{
    unsigned int w;
    float f;
} __scosh_la_Shifter = { 0x4ac000feu };

static __constant union
{
    unsigned int w;
    float f;
} __scosh_la_L2E = { 0x3FB8AA3Bu };

static __constant union
{
    unsigned int w;
    float f;
} __scosh_la_L2H = { 0x3f317218u };

static __constant union
{
    unsigned int w;
    float f;
} __scosh_la_L2L = { 0xb102E308u };

static __constant union
{
    unsigned int w;
    float f;
} __scosh_la_c5 = { 0x3c08ba8bu };

static __constant union
{
    unsigned int w;
    float f;
} __scosh_la_c4 = { 0x3d2aec4eu };

static __constant union
{
    unsigned int w;
    float f;
} __scosh_la_c3 = { 0x3e2aaa9cu };

static __constant union
{
    unsigned int w;
    float f;
} __scosh_la_c2 = { 0x3effffe8u };

static __constant union
{
    unsigned int w;
    float f;
} __scosh_la_c1 = { 0x3f800000u };

__attribute__((always_inline))
inline int __internal_scosh_la_cout (float *a, float *r)
{
    int nRet = 0;
    float x = SPIRV_OCL_BUILTIN(fabs, _f32, ) (*a);
    union
    {
        unsigned int w;
        float f;
    } S, Th, Tlr, Th2, xin, xa, res;
    float N, R, poly;
    int index_mask;

    S.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (x, __scosh_la_L2E.f, __scosh_la_Shifter.f);
    N = S.f - __scosh_la_Shifter.f;

    R = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (-N, __scosh_la_L2H.f, x);
    R = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (-N, __scosh_la_L2L.f, R);

    Th.w = S.w << 22;

    index_mask = 0 - (S.w & 1);

    Th.w ^= (index_mask & 0x7504F3u);

    Tlr.w = index_mask & 0x329302AEu;

    poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (R, __scosh_la_c5.f, __scosh_la_c4.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (R, poly, __scosh_la_c3.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (R, poly, __scosh_la_c2.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (R, poly, __scosh_la_c1.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (R, poly, Tlr.f);

    xin.f = x;
    xa.w = xin.w & 0x7fffffffu;

    if (xa.w > 0x42AEAC4Fu)
        goto COSHF_SPECIAL;

    res.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly, Th.f, Th.f);

    *r = 0.5f * res.f;
    return nRet;

  COSHF_SPECIAL:
    if (xa.w > 0x42b2d4fcu)
    {
        if (xa.w > 0x7f800000u)
        {
            *r = x + x;
            return nRet;
        }

        res.w = 0x7f800000;
        *r = res.f;
        nRet = 3;
        return nRet;
    }

    S.w += 0xfe;
    Th2.w = (S.w >> 2) & 0xff;
    S.w -= (Th2.w << 1);

    Th2.w <<= 23;

    Th.w = S.w << 22;

    Th.w ^= (index_mask & 0x7504F3u);

    res.f = 0.5f * SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (poly, Th.f, Th.f);
    res.f *= Th2.f;

    *r = res.f;
    return nRet;
}

float __ocl_svml_coshf (float a)
{

    float va1;
    float vr1;
    unsigned int vm;

    float r;

    va1 = a;;

    {

        float sN;
        float sM;
        float sR;
        float sR2;
        float sSinh_r;
        float sOut;
        float sG1;
        float sG2;
        float sXSign;
        float sAbsX;

        unsigned int iM;
        unsigned int iAbsX;
        unsigned int iRangeMask;

        float sInvLn2;
        float sShifter;
        float sPC[6];
        unsigned int iHalf;
        unsigned int iDomainRange;

        sInvLn2 = as_float (__internal_scosh_la_data._sInvLn2);
        sShifter = as_float (__internal_scosh_la_data._sShifter);
        sPC[0] = as_float (__internal_scosh_la_data._sPC1);
        sPC[1] = as_float (__internal_scosh_la_data._sPC2);
        sPC[2] = as_float (__internal_scosh_la_data._sPC3);
        sPC[3] = as_float (__internal_scosh_la_data._sPC4);

        sPC[4] = as_float (__internal_scosh_la_data._sPC5);
        sPC[5] = as_float (__internal_scosh_la_data._sPC6);

        sXSign = as_float (__internal_scosh_la_data._sSign);
        iHalf = (__internal_scosh_la_data._iHalf);
        iDomainRange = (__internal_scosh_la_data._iDomainRange);

        sAbsX = as_float ((~(as_uint (sXSign)) & as_uint (va1)));

        sM = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sAbsX, sInvLn2, sShifter);

        iAbsX = as_uint (sAbsX);

        iRangeMask = ((unsigned int) (-(signed int) ((signed int) iAbsX >= (signed int) iDomainRange)));
        vm = 0;
        vm = iRangeMask;

        sN = (sM - sShifter);
        sOut = as_float (__internal_scosh_la_data._sLn2hi);
        sR = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (-(sOut), sN, sAbsX);
        sOut = as_float (__internal_scosh_la_data._sLn2lo);
        sR = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (-(sOut), sN, sR);

        iM = as_uint (sM);
        iM = ((unsigned int) (iM) << (23));
        sR2 = (sR * sR);

        iAbsX = (iHalf + iM);
        sG1 = as_float (iAbsX);
        iAbsX = (iHalf - iM);
        sG2 = as_float (iAbsX);
        sM = sG1;
        sG1 = (sG1 - sG2);
        sG2 = (sM + sG2);

        sSinh_r = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sPC[4], sR2, sPC[2]);
        sSinh_r = (sSinh_r * sR2);
        sSinh_r = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sSinh_r, sR, sR);

        sOut = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sPC[5], sR2, sPC[3]);
        sOut = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sOut, sR2, sPC[1]);
        sOut = (sOut * sR2);
        sOut = (sOut * sG2);
        sOut = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sG1, sSinh_r, sOut);
        vr1 = (sOut + sG2);
    }

    if ((vm) != 0)
    {
        float _vapi_arg1[1];
        float _vapi_res1[1];
        ((float *) _vapi_arg1)[0] = va1;
        ((float *) _vapi_res1)[0] = vr1;
        __internal_scosh_la_cout (_vapi_arg1, _vapi_res1);
        vr1 = ((float *) _vapi_res1)[0];
    };
    r = vr1;;

    return r;

}

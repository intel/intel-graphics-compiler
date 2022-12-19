/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF
typedef struct
{
    unsigned int SgnBit;
    unsigned int OneHalf;
    unsigned int sRsqrtMsk;
    unsigned int MOne;
    unsigned int HalfMask;
    unsigned int Two;
    unsigned int sqrt_coeff[2];
    unsigned int poly_coeff[5];
    unsigned int InvPiH;
    unsigned int InvPiL;
    unsigned int One;
    unsigned int InvPi;
    unsigned int SgnMask;
    unsigned int ep_coeff[3];
} __internal_sacospi_la_data_t;
static __constant __internal_sacospi_la_data_t __internal_sacospi_la_data = {

    0x80000000u, 0x3f000000u, 0xfffff000u, 0xbf800000u, 0xffffe000u, 0x40000000u, {
                                                                                   0xbdC00004u, 0x3e800001u}

    , {
       0x3c5EA355u, 0x3bF87E20u, 0x3c6D978Cu, 0x3cC36C37u, 0x3D594D1Bu}

    , 0x3eA30000u, 0xb84F93C2u, 0x3f800000u, 0x3eA2F983u, 0x80000000u, {
                                                                        0x3cFA8B09u, 0x3d567E9Du, 0x3eA2FAD7u}

};

__attribute__((always_inline))
inline int __internal_sacospi_la_cout (float *a, float *r)
{
    int nRet = 0;

    if ((((((_iml_v2_sp_union_t *) & *a)->hex[0] >> 23) & 0xFF) != 0xFF))
    {

        if (((*a) <= 1.0f) && ((*a) >= -1.0f))
        {

            if ((*a) == 1.0f)
                *r = 0;
            if ((*a) == -1.0f)
                *r = 1.0f;

        }
        else
        {

            {

                *r = (*a - *a) / (*a - *a);

                nRet = 1;
            }
        }
    }
    else
    {

        *r = *a / *a;

        if (((((_iml_v2_sp_union_t *) & *a)->hex[0] & 0x007FFFFF) == 0))
        {

            nRet = 1;
        }

    }

    return nRet;
}

float __ocl_svml_acospif (float a)
{

    float va1;
    float vr1;
    unsigned int vm;

    float r;

    va1 = a;;

    {

        float SgnBit;
        float Sgn_X;
        float OneHalf;
        float MOne;
        float X;
        float BrMask;
        unsigned int BrMaskL;
        float SelMask;
        float PiMask;
        float Y;
        float X2;
        float Sh;
        float Sl;
        float poly_coeff[13];
        float X4;
        float X8;
        float One;
        float HalfMask;
        float InvPiH;
        float InvPiL;
        float InvPi;
        float Xh;
        float Xl;
        float Shl;
        float Shh;
        float SQ;
        float Low;
        float High;
        float Poly;
        float P23;
        float P78;
        float P56;
        float P58;
        float P34;
        float P12;

        SgnBit = as_float (__internal_sacospi_la_data.SgnBit);
        OneHalf = as_float (__internal_sacospi_la_data.OneHalf);
        MOne = as_float (__internal_sacospi_la_data.MOne);

        X = as_float ((as_uint (SgnBit) | as_uint (va1)));
        Sgn_X = as_float ((as_uint (SgnBit) & as_uint (va1)));

        BrMask = as_float (((unsigned int) (-(signed int) (X <= MOne))));

        Y = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (X, OneHalf, OneHalf);

        X2 = (X * X);

        X2 = ((X2 < Y) ? X2 : Y);
        BrMaskL = as_uint (BrMask);
        vm = 0;
        vm = BrMaskL;
        SelMask = as_float (((unsigned int) (-(signed int) (!(X2 < Y)))));
        {
            SQ = (SPIRV_OCL_BUILTIN(sqrt, _f32, ) (Y));
            SQ = (SQ + SQ);
        };

        poly_coeff[5] = as_float (__internal_sacospi_la_data.poly_coeff[0]);
        poly_coeff[4] = as_float (__internal_sacospi_la_data.poly_coeff[1]);
        poly_coeff[3] = as_float (__internal_sacospi_la_data.poly_coeff[2]);
        poly_coeff[2] = as_float (__internal_sacospi_la_data.poly_coeff[3]);
        Poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (X2, poly_coeff[5], poly_coeff[4]);
        P23 = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (X2, poly_coeff[3], poly_coeff[2]);
        X4 = (X2 * X2);
        poly_coeff[1] = as_float (__internal_sacospi_la_data.poly_coeff[4]);
        Poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (Poly, X4, P23);
        Poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (Poly, X2, poly_coeff[1]);
        InvPi = as_float (__internal_sacospi_la_data.InvPi);
        Poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (Poly, X2, InvPi);

        PiMask = as_float (((unsigned int) (-(signed int) (va1 < X2))));
        One = as_float (__internal_sacospi_la_data.One);
        One = as_float ((as_uint (One) & as_uint (SelMask)));
        One = as_float ((as_uint (One) & as_uint (PiMask)));
        OneHalf = as_float ((~(as_uint (SelMask)) & as_uint (OneHalf)));
        OneHalf = (OneHalf + One);

        X = as_float ((((~as_uint (SelMask)) & as_uint (X)) | (as_uint (SelMask) & as_uint (SQ))));
        X = as_float ((as_uint (X) ^ as_uint (Sgn_X)));

        vr1 = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (Poly, X, OneHalf);
    }

    if ((vm) != 0)
    {
        float _vapi_arg1[1];
        float _vapi_res1[1];
        ((float *) _vapi_arg1)[0] = va1;
        ((float *) _vapi_res1)[0] = vr1;
        __internal_sacospi_la_cout (_vapi_arg1, _vapi_res1);
        vr1 = ((float *) _vapi_res1)[0];
    };
    r = vr1;;

    return r;

}

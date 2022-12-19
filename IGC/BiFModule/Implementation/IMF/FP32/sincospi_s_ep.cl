/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF
typedef struct
{
    unsigned int _AbsMask;
    unsigned int _Half;
    unsigned int _SgnMask;
    unsigned int _zero;
    unsigned int _c3;
    unsigned int _c2;
    unsigned int _c1;
    unsigned int _c0;

} __internal_ssincospi_ep_data_t;
static __constant __internal_ssincospi_ep_data_t __internal_ssincospi_ep_data = {
    0x7FFFFFFFu,
    0x3F000000u,
    0x80000000u,
    0x00000000u,
    0xbf0f3abbu,
    0x4022d13du,
    0xc0a55b67u,
    0x40490fd1u,

};

void __ocl_svml_sincospif_ep (float a,  __private float *b, __private float *c)
{

    float va1;
    float vr1;
    float vr2;

    va1 = a;

    {

        float AbsMask;
        float fN;
        unsigned int iN;
        float Rs;
        float Rs2;
        float Rc;
        float Rc2;
        float aRs;
        float Half;
        float c4;
        float c3;
        float c2;
        float c1;
        float c0;
        float sgn_N;
        float spoly;
        float cpoly;
        float zero;
        float sgn_x;
        float sgn_c;
        float sgn_s;
        float SgnMask;

        float Rs_msk;
        float Rc_msk;

        AbsMask = as_float (__internal_ssincospi_ep_data._AbsMask);

        fN = SPIRV_OCL_BUILTIN(rint, _f32, ) (va1);

        Rs = (va1 - fN);

        iN = ((int) ((-SPIRV_OCL_BUILTIN(fabs, _f64, ) (fN))));

        Half = as_float (__internal_ssincospi_ep_data._Half);
        aRs = as_float ((as_uint (Rs) & as_uint (AbsMask)));
        Rc = (Half - aRs);

        Rs2 = (Rs * Rs);
        Rc2 = (Rc * Rc);

        iN = ((unsigned int) (iN) << (31));
        sgn_N = as_float (iN);
        c3 = as_float (__internal_ssincospi_ep_data._c3);
        c2 = as_float (__internal_ssincospi_ep_data._c2);

        spoly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (c3, Rs2, c2);
        cpoly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (c3, Rc2, c2);

        zero = as_float (__internal_ssincospi_ep_data._zero);
        SgnMask = as_float (__internal_ssincospi_ep_data._SgnMask);

        Rs_msk = as_float (((unsigned int) (-(signed int) (!(Rs == zero)))));
        sgn_x = as_float ((as_uint (va1) & as_uint (SgnMask)));
        Rc_msk = as_float (((unsigned int) (-(signed int) (!(Rc == zero)))));
        sgn_s = as_float ((((~as_uint (Rs_msk)) & as_uint (sgn_x)) | (as_uint (Rs_msk) & as_uint (sgn_N))));
        sgn_c = as_float ((as_uint (sgn_N) & as_uint (Rc_msk)));

        c1 = as_float (__internal_ssincospi_ep_data._c1);
        c0 = as_float (__internal_ssincospi_ep_data._c0);

        spoly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (spoly, Rs2, c1);
        cpoly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (cpoly, Rc2, c1);
        spoly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (spoly, Rs2, c0);
        cpoly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (cpoly, Rc2, c0);

        Rs = as_float ((as_uint (Rs) ^ as_uint (sgn_s)));
        Rc = as_float ((as_uint (Rc) ^ as_uint (sgn_c)));

        vr1 = (spoly * Rs);
        vr2 = (cpoly * Rc);
    }

    ((float *) b)[0] = vr1;
    ((float *) c)[0] = vr2;

    return;

}

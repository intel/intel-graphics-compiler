/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF
typedef struct
{
    unsigned long _AbsMask;
    unsigned long _Shifter;
    unsigned long _Two53;
    unsigned long _Half;
    unsigned long _SgnMask;
    unsigned long _zero;
    unsigned long _c8;
    unsigned long _c7;
    unsigned long _c6;
    unsigned long _c5;
    unsigned long _c4;
    unsigned long _c3;
    unsigned long _c2;
    unsigned long _c1;
    unsigned long _c0;
    unsigned long _cx;
    unsigned long _one;
} __internal_dsincospi_data_t;
static __constant __internal_dsincospi_data_t __internal_dsincospi_data = {
    0x7FFFFFFFFFFFFFFFuL,
    0x4330000000000000uL,
    0x4340000000000000uL,
    0x3FE0000000000000uL,
    0x8000000000000000uL,
    0x0000000000000000uL,
    0x3ea9d46b06ce620euL,
    0xbef6f7ad23b5cd51uL,
    0x3f3e8f3677c334d3uL,
    0xbf7e3074dfb5bb14uL,
    0x3fb50783485523f4uL,
    0xbfe32d2cce627c9euL,
    0x400466bc6775aa7duL,
    0xc014abbce625be52uL,
    0x400921fb54442d18uL,
    0x400466bc6775aa7cuL,
    0x3ff0000000000000uL,
};

void __ocl_svml_sincospi (double a,  __private double *b, __private double *c)
{

    double va1;
    double vr1;
    double vr2;

    va1 = a;

    {

        double AbsMask;
        double fN;
        double Rs;
        double Rs2;
        double Rc;
        double Rc2;
        double aRs;
        double aN;
        double Half;
        double c8;
        double c7;
        double c6;
        double c5;
        double c4;
        double c3;
        double c2;
        double c1;
        double c0;
        double sgn_N;
        double spoly;
        double cpoly;

        double zero;
        double one;
        double sgn_x;
        double sgn_c;
        double sgn_s;
        double SgnMask;

        double Rs_msk;
        double Rc_msk;
        double Shifter;
        double Nmask;
        double Two53;

        AbsMask = as_double (__internal_dsincospi_data._AbsMask);

        {
            double _rnd_d2p52;
            double _rnd_dSignMask;
            double _rnd_dSign;
            double _rnd_dAbsArg;
            double _rnd_dRes_ub;
            double _rnd_dRange;
            unsigned long _rnd_i2p52 = 0x4330000000000000;
            unsigned long _rnd_iSignMask = 0x8000000000000000;
            _rnd_d2p52 = as_double (_rnd_i2p52);
            _rnd_dSignMask = as_double (_rnd_iSignMask);
            _rnd_dSign = as_double (((as_ulong (_rnd_dSignMask)) & (as_ulong (va1))));
            _rnd_dAbsArg = as_double ((~(as_ulong (_rnd_dSignMask)) & as_ulong (va1)));
            _rnd_dRange = as_double (((unsigned long) (-(signed long) (_rnd_dAbsArg > _rnd_d2p52))));
            _rnd_dRes_ub = (_rnd_dAbsArg + _rnd_d2p52);
            _rnd_dRes_ub = (_rnd_dRes_ub - _rnd_d2p52);
            _rnd_dRes_ub = as_double (((as_ulong (_rnd_dRes_ub)) | (as_ulong (_rnd_dSign))));
            _rnd_dRes_ub = as_double ((((~as_ulong (_rnd_dRange)) & as_ulong (_rnd_dRes_ub)) | (as_ulong (_rnd_dRange) & as_ulong (va1))));
            fN = _rnd_dRes_ub;
        };

        Rs = (va1 - fN);

        aN = as_double ((as_ulong (fN) & as_ulong (AbsMask)));
        Shifter = as_double (__internal_dsincospi_data._Shifter);

        Nmask = as_double ((unsigned long) ((aN < Shifter) ? 0xffffffffffffffff : 0x0));
        Shifter = as_double ((as_ulong (Shifter) & as_ulong (Nmask)));
        sgn_N = (aN + Shifter);
        Two53 = as_double (__internal_dsincospi_data._Two53);
        sgn_N = ((sgn_N < Two53) ? sgn_N : Two53);

        Half = as_double (__internal_dsincospi_data._Half);
        aRs = as_double ((as_ulong (Rs) & as_ulong (AbsMask)));
        Rc = (Half - aRs);

        Rs2 = (Rs * Rs);
        Rc2 = (Rc * Rc);

        sgn_N = as_double (((unsigned long) as_ulong (sgn_N) << (63)));

        c8 = as_double (__internal_dsincospi_data._c8);
        c7 = as_double (__internal_dsincospi_data._c7);
        c6 = as_double (__internal_dsincospi_data._c6);

        c5 = as_double (__internal_dsincospi_data._c5);
        c4 = as_double (__internal_dsincospi_data._c4);
        c3 = as_double (__internal_dsincospi_data._c3);

        c2 = as_double (__internal_dsincospi_data._cx);

        c1 = as_double (__internal_dsincospi_data._c1);
        c0 = as_double (__internal_dsincospi_data._c0);

        spoly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (c8, Rs2, c7);
        cpoly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (c8, Rc2, c7);
        spoly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (spoly, Rs2, c6);
        cpoly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly, Rc2, c6);
        spoly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (spoly, Rs2, c5);
        cpoly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly, Rc2, c5);
        spoly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (spoly, Rs2, c4);
        cpoly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly, Rc2, c4);

        spoly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (spoly, Rs2, c3);
        cpoly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly, Rc2, c3);

        zero = as_double (__internal_dsincospi_data._zero);
        SgnMask = as_double (__internal_dsincospi_data._SgnMask);

        Rs_msk = as_double ((unsigned long) (((!(Rs == zero)) ? 0xffffffffffffffff : 0x0)));
        sgn_x = as_double ((as_ulong (va1) & as_ulong (SgnMask)));
        Rc_msk = as_double ((unsigned long) (((!(Rc == zero)) ? 0xffffffffffffffff : 0x0)));
        sgn_s = as_double ((((~as_ulong (Rs_msk)) & as_ulong (sgn_x)) | (as_ulong (Rs_msk) & as_ulong (sgn_N))));
        sgn_c = as_double ((as_ulong (sgn_N) & as_ulong (Rc_msk)));

        spoly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (spoly, Rs2, c2);
        cpoly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly, Rc2, c2);
        spoly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (spoly, Rs2, c1);
        cpoly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly, Rc2, c1);
        spoly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (spoly, Rs2, c0);
        cpoly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly, Rc2, c0);

        Rs = as_double ((as_ulong (Rs) ^ as_ulong (sgn_s)));
        Rc = as_double ((as_ulong (Rc) ^ as_ulong (sgn_c)));

        vr1 = (spoly * Rs);
        vr2 = (cpoly * Rc);
    }

    ((double *) b)[0] = vr1;
    ((double *) c)[0] = vr2;

    return;

}

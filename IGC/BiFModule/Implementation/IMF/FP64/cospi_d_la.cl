/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF
typedef struct
{
    unsigned long _dAbsMask;
    unsigned long _dHalf;
    unsigned long _dSignBit;
    unsigned long _dC1_h;
    unsigned long _dC2;
    unsigned long _dC3;
    unsigned long _dC4;
    unsigned long _dC5;
    unsigned long _dC6;
    unsigned long _dC7;
    unsigned long _dC8;
    unsigned long _dC9;
} __internal_dcospi_la_data_avx512_t;
static __constant __internal_dcospi_la_data_avx512_t __internal_dcospi_la_data_avx512 = {
    0x7FFFFFFFFFFFFFFFuL,
    0x3fe0000000000000uL,
    0x8000000000000000uL,
    0xc00921fb54442d18uL,
    0x4014abbce625be52uL,
    0xc00466bc6775aa7duL,
    0x3fe32d2cce627c9euL,
    0xbfb50783485523f4uL,
    0x3f7e3074dfb5bb14uL,
    0xbf3e8f3677c334d3uL,
    0x3ef6f7ad23b5cd51uL,
    0xbea9d46b06ce620euL,
};

typedef struct
{
    unsigned long _dAbsMask;
    unsigned long _dReductionRangeVal;
    unsigned long _dRangeVal;
    unsigned long _dRShifter;
    unsigned long _dOneHalf;

    unsigned long _dC0;

    unsigned long _dC1;
    unsigned long _dC2;
    unsigned long _dC3;
    unsigned long _dC4;

    unsigned long _dC5;
    unsigned long _dC6;
    unsigned long _dC7;
    unsigned long _dC8;
    unsigned long _dThres;
    unsigned long _dSgnExp;

} __internal_dcospi_la_data_t;
static __constant __internal_dcospi_la_data_t __internal_dcospi_la_data = {
    0x7FFFFFFFFFFFFFFFuL,
    0x42A0000000000000uL,
    0x7FF0000000000000uL,
    0x4338000000000000uL,
    0x3FE0000000000000uL,

    0x400921fb54442d18uL,
    0xc014abbce625be52uL,
    0x400466bc6775aa7duL,
    0xbfe32d2cce627c9euL,
    0x3fb50783485523f4uL,
    0xbf7e3074dfb5bb14uL,
    0x3f3e8f3677c334d3uL,
    0xbef6f7ad23b5cd51uL,
    0x3ea9d46b06ce620euL,

};

static __constant _iml_v2_dp_union_t __dcospi_la_CoutTab[3] = {
    0x00000000, 0x00000000,
    0x00000000, 0x7FF00000,
};

#pragma float_control(push)
#pragma float_control(precise,on)
__attribute__((always_inline))
inline int __internal_dcospi_la_cout (double *a, double *r)
{
    double absx;
    int nRet = 0;

    absx = (*a);
    (((_iml_v2_dp_union_t *) & absx)->dwords.hi_dword = (((_iml_v2_dp_union_t *) & absx)->dwords.hi_dword & 0x7FFFFFFF) | ((_iml_uint32_t) (0) << 31));
    if (!(((((_iml_v2_dp_union_t *) & (*a))->dwords.hi_dword >> 20) & 0x7FF) != 0x7FF))
    {

        if ((((_iml_v2_dp_union_t *) & (absx))->hex[0] == ((__constant _iml_v2_dp_union_t *) & (((__constant double *) __dcospi_la_CoutTab)[1]))->hex[0])
            && (((_iml_v2_dp_union_t *) & (absx))->hex[1] ==
                ((__constant _iml_v2_dp_union_t *) & (((__constant double *) __dcospi_la_CoutTab)[1]))->hex[1]))
        {

            (*r) = (*a) * ((__constant double *) __dcospi_la_CoutTab)[0];

            nRet = 1;
        }
        else
        {

            (*r) = ((*a) + (*a));
        }
    }
    return nRet;
}

#pragma float_control(pop)
double __ocl_svml_cospi (double a)
{

    double va1;
    double vr1;
    unsigned int vm;

    double r;

    va1 = a;;

    {

        double dX;
        double dAbsX;
        double dExp;
        double dRangeMask;
        unsigned long lRangeMask;
        double dReductionRangeMask;
        unsigned long lReductionRangeMask;
        unsigned int mReductionRangeMask;
        double dSign;
        double dSignReduced;
        double dSignRes;
        double dN;
        double dY;
        double dR;
        double dRp2;
        double dPoly;

        double dAbsMask;
        double dRangeVal;
        double dReductionRangeVal;
        double dRShifter;
        double dInvPI;
        double dPI;
        double dHalfPI;
        double dOneHalf;
        double dC0;
        double dPiToRad;

        double dC1;
        double dC2;
        double dC3;
        double dC4;
        double dC5;
        double dC6;
        double dC7;
        double dC8;

        dX = va1;

        vm = 0;

        dAbsMask = as_double (__internal_dcospi_la_data._dAbsMask);
        dAbsX = as_double ((as_ulong (dX) & as_ulong (dAbsMask)));

        dR = dAbsX;

        dReductionRangeVal = as_double (__internal_dcospi_la_data._dReductionRangeVal);
        dReductionRangeMask = as_double ((unsigned long) (((!(dAbsX <= dReductionRangeVal)) ? 0xffffffffffffffff : 0x0)));
        lReductionRangeMask = as_ulong (dReductionRangeMask);
        mReductionRangeMask = 0;
        mReductionRangeMask = lReductionRangeMask;
        if ((mReductionRangeMask) != 0)
        {

            dRangeVal = as_double (__internal_dcospi_la_data._dRangeVal);
            dExp = as_double ((as_ulong (dRangeVal) & as_ulong (dAbsX)));
            dRangeMask = as_double ((unsigned long) ((dExp == dRangeVal) ? 0xffffffffffffffff : 0x0));
            lRangeMask = as_ulong (dRangeMask);
            vm = 0;
            vm = lRangeMask;

            {

                double dX;
                double dShifterMod;
                double dShifterThreshold;
                double dShifterMask;
                double dShifterPos;
                double dShifter;
                double dDirect;
                double dInverse;
                double dShiftedN;
                double dN;
                double dZero;
                dX = dAbsX;
                dShifterThreshold = as_double (0x43A0000000000000uL);
                dShifterMask = as_double ((unsigned long) ((dX < dShifterThreshold) ? 0xffffffffffffffff : 0x0));

                dShifterPos = as_double (0x43B8000000000000uL);
                dZero = as_double (0x0000000000000000uL);
                dShifter = as_double ((((~as_ulong (dShifterMask)) & as_ulong (dZero)) | (as_ulong (dShifterMask) & as_ulong (dShifterPos))));

                dShiftedN = (dShifter + dAbsX);
                dN = (dShiftedN - dShifter);
                dR = (dAbsX - dN);

            }

            dR = as_double ((((~as_ulong (dReductionRangeMask)) & as_ulong (dAbsX)) | (as_ulong (dReductionRangeMask) & as_ulong (dR))));
        }

        dOneHalf = as_double (__internal_dcospi_la_data._dOneHalf);
        dX = (dR + dOneHalf);

        dRShifter = as_double (__internal_dcospi_la_data._dRShifter);
        dY = (dX + dRShifter);
        dN = (dY - dRShifter);

        dSignRes = as_double (((unsigned long) as_ulong (dY) << (63)));

        dN = (dN - dOneHalf);
        dR = (dR - dN);

        dR = as_double ((as_ulong (dR) ^ as_ulong (dSignRes)));
        dRp2 = (dR * dR);

        dC8 = as_double (__internal_dcospi_la_data._dC8);
        dC7 = as_double (__internal_dcospi_la_data._dC7);
        dC6 = as_double (__internal_dcospi_la_data._dC6);
        dPoly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dC8, dRp2, dC7);
        dPoly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dPoly, dRp2, dC6);
        dC5 = as_double (__internal_dcospi_la_data._dC5);
        dPoly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dPoly, dRp2, dC5);
        dC4 = as_double (__internal_dcospi_la_data._dC4);
        dPoly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dPoly, dRp2, dC4);
        dC3 = as_double (__internal_dcospi_la_data._dC3);
        dPoly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dPoly, dRp2, dC3);

        dC2 = as_double (__internal_dcospi_la_data._dC2);
        dPoly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dPoly, dRp2, dC2);
        dC1 = as_double (__internal_dcospi_la_data._dC1);
        dPoly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dPoly, dRp2, dC1);
        dC0 = as_double (__internal_dcospi_la_data._dC0);
        dC0 = (dC0 * dR);
        dPoly = (dPoly * dRp2);

        vr1 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dPoly, dR, dC0);
    }

    if ((vm) != 0)
    {
        double _vapi_arg1[1];
        double _vapi_res1[1];
        ((double *) _vapi_arg1)[0] = va1;
        ((double *) _vapi_res1)[0] = vr1;
        __internal_dcospi_la_cout (_vapi_arg1, _vapi_res1);
        vr1 = ((double *) _vapi_res1)[0];
    };
    r = vr1;;

    return r;

}

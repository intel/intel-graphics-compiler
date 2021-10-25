/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../imf.h"
#include "../imf_data.h"
#pragma OPENCL FP_CONTRACT OFF

typedef struct
{
    unsigned long _dAbsMask;
    unsigned long _dRangeVal;
    unsigned long _dInvPI;
    unsigned long _dRShifter;
    unsigned long _dZero;
    unsigned long _lNZero;
    unsigned long _dPI1;
    unsigned long _dPI2;
    unsigned long _dPI3;
    unsigned long _dPI4;
    unsigned long _dPI1_FMA;
    unsigned long _dPI2_FMA;
    unsigned long _dPI3_FMA;
    unsigned long _dC1;
    unsigned long _dC2;
    unsigned long _dC3;
    unsigned long _dC4;
    unsigned long _dC5;
    unsigned long _dC6;
    unsigned long _dC7;
} __internal_dsin_la_data_t;
static __constant __internal_dsin_la_data_t __internal_dsin_la_data = {

    0x7FFFFFFFFFFFFFFFuL,
    0x4170000000000000uL,
    0x3FD45F306DC9C883uL,
    0x4338000000000000uL,
    0x0000000000000000uL,
    0x8000000000000000uL,

    0x400921FB40000000uL,
    0x3E84442D00000000uL,
    0x3D08469880000000uL,
    0x3B88CC51701B839AuL,

    0x400921fb54442d18uL,
    0x3ca1a62633145c06uL,
    0x395c1cd129024e09uL,

    0xBFC55555555554A8uL,
    0x3F8111111110A573uL,
    0xBF2A01A019A659DDuL,
    0x3EC71DE3806ADD1AuL,
    0xBE5AE6355AAA4A53uL,
    0x3DE60E6BEE01D83EuL,
    0xBD69F1517E9F65F0uL,
};

static __constant _iml_v2_dp_union_t __dsin_la_CoutTab[2] = {
    0x00000000, 0x00000000,
    0x00000000, 0x7FF00000
};

__attribute__((always_inline))
inline int __internal_dsin_la_cout (double *a, double *r)
{
    int nRet = 0;
    double absx;

    absx = (*a);
    (((_iml_v2_dp_union_t *) & absx)->dwords.hi_dword = (((_iml_v2_dp_union_t *) & absx)->dwords.hi_dword & 0x7FFFFFFF) | ((_iml_uint32_t) (0) << 31));

    if (!((((((_iml_v2_dp_union_t *) & (*a))->dwords.hi_dword >> 20) & 0x7FF) != 0x7FF)))
    {
        if ((((_iml_v2_dp_union_t *) & (absx))->hex[0] == ((__constant _iml_v2_dp_union_t *) & (((__constant double *) __dsin_la_CoutTab)[1]))->hex[0])
            && (((_iml_v2_dp_union_t *) & (absx))->hex[1] == ((__constant _iml_v2_dp_union_t *) & (((__constant double *) __dsin_la_CoutTab)[1]))->hex[1]))
        {

            (*r) = (*a) * ((__constant double *) __dsin_la_CoutTab)[0];

            nRet = 1;
            return nRet;
        }
        else
        {

            (*r) = (*a) * (*a);
            return nRet;
        }
    }
    return nRet;
}

double __ocl_svml_sin (double a)
{

    double va1;
    double vr1;
    unsigned int vm;

    double r;

    va1 = a;;

    {

        double dAbsX;
        double dExpX;
        double dSignX;
        double dReductionRangeMask;
        unsigned long lReductionRangeMask;
        unsigned int mReductionRangeMask;
        double dRangeMask;
        unsigned long lRangeMask;
        double dSignRes;
        double dR;
        double dRp2;
        double dPoly;
        double dN;
        double dY;
        unsigned long lY;
        unsigned long lIndex;
        double dMLo;
        double dZ;
        double dZp2;
        double dZ1;
        double dE;
        double dPS;
        double dPC;
        double dZSigma;
        double dMed;
        double dResInt;
        double dD;
        double dCorr;
        double dK0;
        double dK1;
        double dK2;
        double dK3;
        double dResHi;
        double dResLo;

        double dAbsMask;
        double dReductionRangeVal;
        double dRangeVal;
        double dPIu;
        unsigned long lIndexMask;
        double dPIoHi;
        double dPIoLo;
        double dPIoTail;
        double dSigma;
        double dCHL;
        double dSHi;
        double dSLo;
        double dRShifter;
        double dInvPI;
        double dPI1;
        double dPI2;
        double dPI3;
        double dPI4;
        double dS1;
        double dS2;
        double dC1;
        double dC2;
        double dC3;
        double dC4;
        double dC5;
        double dC6;
        double dC7;

        vm = 0;

        dAbsMask = as_double (__internal_dsin_la_data._dAbsMask);

        dAbsX = as_double ((as_ulong (va1) & as_ulong (dAbsMask)));

        dInvPI = as_double (__internal_dsin_la_data._dInvPI);
        dRShifter = as_double (__internal_dsin_la_data._dRShifter);

        dY = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dAbsX, dInvPI, dRShifter);

        dN = (dY - dRShifter);

        dSignRes = as_double (((unsigned long) as_ulong (dY) << (63)));

        dPI1 = as_double (__internal_dsin_la_data._dPI1);

        dR = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (-(dN), dPI1, dAbsX);
        dPI2 = as_double (__internal_dsin_la_data._dPI2);

        dR = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (-(dN), dPI2, dR);
        dPI3 = as_double (__internal_dsin_la_data._dPI3);

        dR = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (-(dN), dPI3, dR);

        dPI4 = as_double (__internal_dsin_la_data._dPI4);

        dR = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (-(dN), dPI4, dR);
        dRp2 = (dR * dR);

        dR = as_double ((as_ulong (dR) ^ as_ulong (dSignRes)));

        dC7 = as_double (__internal_dsin_la_data._dC7);
        dC6 = as_double (__internal_dsin_la_data._dC6);
        dPoly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dC7, dRp2, dC6);
        dC5 = as_double (__internal_dsin_la_data._dC5);
        dPoly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dPoly, dRp2, dC5);
        dC4 = as_double (__internal_dsin_la_data._dC4);
        dPoly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dPoly, dRp2, dC4);
        dC3 = as_double (__internal_dsin_la_data._dC3);

        dPoly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dPoly, dRp2, dC3);

        dSignX = as_double ((~(as_ulong (dAbsMask)) & as_ulong (va1)));

        dC2 = as_double (__internal_dsin_la_data._dC2);
        dPoly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dPoly, dRp2, dC2);
        dC1 = as_double (__internal_dsin_la_data._dC1);
        dPoly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dPoly, dRp2, dC1);
        dPoly = (dPoly * dRp2);

        dPoly = (dPoly * dR);
        dPoly = (dPoly + dR);

        vr1 = as_double ((as_ulong (dPoly) ^ as_ulong (dSignX)));

        dReductionRangeVal = as_double (__internal_dsin_la_data._dRangeVal);
        dReductionRangeMask = as_double ((unsigned long) (((!(dAbsX <= dReductionRangeVal)) ? 0xffffffffffffffff : 0x0)));
        lReductionRangeMask = as_ulong (dReductionRangeMask);
        mReductionRangeMask = 0;
        mReductionRangeMask = lReductionRangeMask;
        if ((mReductionRangeMask) != 0)
        {
            double dResLarge;

            dRangeVal = as_double (__internal_dsincos_la_data_ha._dRangeVal);

            dExpX = as_double ((as_ulong (dRangeVal) & as_ulong (dAbsX)));
            dRangeMask = as_double ((unsigned long) ((dExpX == dRangeVal) ? 0xffffffffffffffff : 0x0));
            lRangeMask = as_ulong (dRangeMask);
            vm = 0;
            vm = lRangeMask;

            {

                double dExponent;
                unsigned long lInput;
                unsigned long lExponent;
                unsigned long lSignificand;

                unsigned long lIntegerBit;

                double dP_hi;
                double dP_med;
                double dP_lo;

                unsigned long lP_hi;
                unsigned long lP_med;
                unsigned long lP_lo;

                unsigned long lLowMask;

                unsigned long lP5;
                unsigned long lP4;
                unsigned long lP3;
                unsigned long lP2;
                unsigned long lP1;
                unsigned long lP0;

                unsigned long lM1;
                unsigned long lM0;

                unsigned long lM15;
                unsigned long lM14;
                unsigned long lM13;
                unsigned long lM12;
                unsigned long lM11;
                unsigned long lM10;
                unsigned long lM05;
                unsigned long lM04;
                unsigned long lM03;
                unsigned long lM02;
                unsigned long lM01;
                unsigned long lM00;

                unsigned long lN14;
                unsigned long lN13;
                unsigned long lN12;
                unsigned long lN11;

                unsigned long lP15;
                unsigned long lP14;
                unsigned long lP13;
                unsigned long lP12;
                unsigned long lP11;

                unsigned long lQ14;
                unsigned long lQ13;
                unsigned long lQ12;
                unsigned long lQ11;

                unsigned long lReducedHi;
                unsigned long lReducedMed;
                unsigned long lReducedLo;

                unsigned long lRoundBump;
                unsigned long lShiftedN;
                unsigned long lNMask;

                double dReducedHi;
                double dReducedMed;
                double dReducedLo;

                unsigned long lExponentPart;
                unsigned long lShiftedSig;

                double dShifter;
                double dIntegerPart;

                double dRHi;
                double dRLo;
                unsigned long lSignBit;

                double d2pi_full;
                double d2pi_lead;
                double d2pi_trail;

                double dLeadmask;
                double dRHi_lead;
                double dRHi_trail;

                double dPir1;
                double dPir2;
                double dPir3;
                double dPir4;
                double dPir12;
                double dPir34;
                double dRedPreHi;
                double dRedHi;
                double dRedLo;

                double dMinInput;
                double dAbs;
                double dMultiplex;
                double dNotMultiplex;
                double dMultiplexedInput;
                double dMultiplexedOutput;

                lInput = as_ulong (dAbsX);

                lExponent = 0x7ff0000000000000uL;;
                lExponent = (lExponent & lInput);
                lExponent = ((unsigned long) (lExponent) >> (52));

                dP_hi =
                    as_double (((__constant unsigned long *) (__internal_dsincos_la_reduction_data._dPtable))[(((0 + lExponent) * (3 * 8)) >> (3)) + 0]);
                dP_med =
                    as_double (((__constant unsigned long *) (__internal_dsincos_la_reduction_data._dPtable))[(((0 + lExponent) * (3 * 8)) >> (3)) + 1]);
                dP_lo =
                    as_double (((__constant unsigned long *) (__internal_dsincos_la_reduction_data._dPtable))[(((0 + lExponent) * (3 * 8)) >> (3)) + 2]);

                lP_hi = as_ulong (dP_hi);
                lP_med = as_ulong (dP_med);
                lP_lo = as_ulong (dP_lo);

                lSignificand = 0x000fffffffffffffuL;;
                lIntegerBit = 0x0010000000000000uL;
                lSignificand = (lSignificand & lInput);
                lSignificand = (lSignificand + lIntegerBit);

                lLowMask = 0x00000000FFFFFFFFuL;
                lP5 = ((unsigned long) (lP_hi) >> (32));
                lP4 = (lP_hi & lLowMask);
                lP3 = ((unsigned long) (lP_med) >> (32));
                lP2 = (lP_med & lLowMask);
                lP1 = ((unsigned long) (lP_lo) >> (32));
                lP0 = (lP_lo & lLowMask);
                lM1 = ((unsigned long) (lSignificand) >> (32));
                lM0 = (lSignificand & lLowMask);

                lM15 = (lM1 * lP5);
                lM14 = (lM1 * lP4);
                lM13 = (lM1 * lP3);
                lM12 = (lM1 * lP2);
                lM11 = (lM1 * lP1);
                lM10 = (lM1 * lP0);
                lM05 = (lM0 * lP5);
                lM04 = (lM0 * lP4);
                lM03 = (lM0 * lP3);
                lM02 = (lM0 * lP2);
                lM01 = (lM0 * lP1);
                lM00 = (lM0 * lP0);

                lN11 = ((unsigned long) (lM01) >> (32));
                lN12 = ((unsigned long) (lM02) >> (32));
                lN13 = ((unsigned long) (lM03) >> (32));
                lN14 = ((unsigned long) (lM04) >> (32));

                lN11 = (lM11 + lN11);
                lN12 = (lM12 + lN12);
                lN13 = (lM13 + lN13);
                lN14 = (lM14 + lN14);

                lP11 = (lM02 & lLowMask);
                lP12 = (lM03 & lLowMask);
                lP13 = (lM04 & lLowMask);
                lP14 = (lM05 & lLowMask);
                lP15 = ((unsigned long) (lM05) >> (32));

                lP11 = (lP11 + lN11);
                lP12 = (lP12 + lN12);
                lP13 = (lP13 + lN13);
                lP14 = (lP14 + lN14);
                lP15 = (lP15 + lM15);

                lQ11 = ((unsigned long) (lM10) >> (32));
                lQ11 = (lQ11 + lP11);

                lQ12 = ((unsigned long) (lQ11) >> (32));
                lQ12 = (lQ12 + lP12);

                lQ13 = ((unsigned long) (lQ12) >> (32));
                lQ13 = (lQ13 + lP13);

                lQ14 = ((unsigned long) (lQ13) >> (32));
                lQ14 = (lQ14 + lP14);

                lQ11 = (lQ11 & lLowMask);
                lQ13 = (lQ13 & lLowMask);

                lReducedHi = ((unsigned long) (lQ14) << (32));
                lReducedLo = ((unsigned long) (lQ12) << (32));

                lReducedHi = (lReducedHi + lQ13);
                lReducedLo = (lReducedLo + lQ11);

                lSignBit = 0x8000000000000000uL;;
                lSignBit = (lSignBit & lInput);

                lExponentPart = 0x3FF0000000000000uL;
                lExponentPart = (lSignBit ^ lExponentPart);
                lShiftedSig = ((unsigned long) (lReducedHi) >> (12));
                lShiftedSig = (lShiftedSig | lExponentPart);
                dReducedHi = as_double (lShiftedSig);
                dShifter = as_double (0x42A8000000000000uL);
                dIntegerPart = (dShifter + dReducedHi);
                dN = (dIntegerPart - dShifter);
                dReducedHi = (dReducedHi - dN);

                lIndex = as_ulong (dIntegerPart);
                lNMask = 0x00000000000001FFuL;
                lIndex = (lIndex & lNMask);
                lExponentPart = 0x3970000000000000uL;
                lExponentPart = (lSignBit ^ lExponentPart);
                lShiftedSig = 0x0000000000FFFFFFuL;
                lShiftedSig = (lShiftedSig & lReducedLo);
                lShiftedSig = ((unsigned long) (lShiftedSig) << (28));
                lShiftedSig = (lShiftedSig | lExponentPart);
                dReducedLo = as_double (lShiftedSig);
                dShifter = as_double (lExponentPart);
                dReducedLo = (dReducedLo - dShifter);

                lExponentPart = 0x3CB0000000000000uL;
                lExponentPart = (lSignBit ^ lExponentPart);
                lShiftedSig = 0x0000000000000FFFuL;
                lShiftedSig = (lShiftedSig & lReducedHi);
                lShiftedSig = ((unsigned long) (lShiftedSig) << (40));
                lReducedLo = ((unsigned long) (lReducedLo) >> (24));
                lShiftedSig = (lShiftedSig | lReducedLo);
                lShiftedSig = (lShiftedSig | lExponentPart);
                dReducedMed = as_double (lShiftedSig);
                dShifter = as_double (lExponentPart);
                dReducedMed = (dReducedMed - dShifter);

                dRHi = (dReducedHi + dReducedMed);
                dReducedHi = (dReducedHi - dRHi);
                dReducedMed = (dReducedMed + dReducedHi);
                dRLo = (dReducedMed + dReducedLo);
                d2pi_full = as_double (0x401921FB54442D18uL);
                d2pi_lead = as_double (0x401921FB54000000uL);
                d2pi_trail = as_double (0x3E310B4611A62633uL);

                dLeadmask = as_double (0xFFFFFFFFF8000000uL);
                dRHi_lead = as_double ((as_ulong (dRHi) & as_ulong (dLeadmask)));
                dRHi_trail = (dRHi - dRHi_lead);

                dRedPreHi = (d2pi_lead * dRHi_lead);
                dPir1 = (d2pi_lead * dRHi_trail);
                dPir2 = (d2pi_trail * dRHi_lead);
                dPir3 = (d2pi_full * dRLo);
                dPir4 = (d2pi_trail * dRHi_trail);

                dPir12 = (dPir1 + dPir2);
                dPir34 = (dPir3 + dPir4);
                dRedLo = (dPir12 + dPir34);

                dRedHi = (dRedPreHi + dRedLo);
                dRedPreHi = (dRedPreHi - dRedHi);
                dRedLo = (dRedPreHi + dRedLo);
                dAbsMask = as_double (0x7FFFFFFFFFFFFFFFuL);
                dMinInput = as_double (0x3EB0000000000000uL);
                dAbs = as_double ((as_ulong (dAbsX) & as_ulong (dAbsMask)));
                dMultiplex = as_double ((unsigned long) ((dAbs > dMinInput) ? 0xffffffffffffffff : 0x0));
                dNotMultiplex = as_double ((unsigned long) ((dAbs <= dMinInput) ? 0xffffffffffffffff : 0x0));

                dMultiplexedInput = as_double ((as_ulong (dNotMultiplex) & as_ulong (dAbsX)));
                dMultiplexedOutput = as_double ((as_ulong (dMultiplex) & as_ulong (dRedHi)));
                dZ = as_double ((as_ulong (dMultiplexedInput) | as_ulong (dMultiplexedOutput)));

                dE = as_double ((as_ulong (dMultiplex) & as_ulong (dRedLo)));

            }

            dCHL = as_double (((__constant unsigned long *) (__internal_dsincos_la_data_ha._dT))[(((0 + lIndex) * (4 * 8)) >> (3)) + 0]);
            dSigma = as_double (((__constant unsigned long *) (__internal_dsincos_la_data_ha._dT))[(((0 + lIndex) * (4 * 8)) >> (3)) + 1]);
            dSHi = as_double (((__constant unsigned long *) (__internal_dsincos_la_data_ha._dT))[(((0 + lIndex) * (4 * 8)) >> (3)) + 2]);

            dMed = (dCHL * dZ);
            dZSigma = (dZ * dSigma);
            dResInt = (dSHi + dZSigma);
            dResHi = (dMed + dResInt);

            dK0 = (dSHi - dResInt);
            dK2 = (dK0 + dZSigma);
            dK1 = (dResInt - dResHi);
            dK3 = (dK1 + dMed);
            dResLo = (dK3 + dK2);
            dZp2 = (dZ * dZ);

            dS2 = as_double (__internal_dsincos_la_data_ha._dS2);
            dS1 = as_double (__internal_dsincos_la_data_ha._dS1);

            dPS = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dS2, dZp2, dS1);

            dPS = (dPS * dZp2);

            dPS = (dPS * dZ);

            dD = (dCHL + dSigma);

            dResLo = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dPS, dD, dResLo);

            dC3 = as_double (__internal_dsincos_la_data_ha._dC3);
            dC2 = as_double (__internal_dsincos_la_data_ha._dC2);

            dPC = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dC3, dZp2, dC2);
            dC1 = as_double (__internal_dsincos_la_data_ha._dC1);

            dPC = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dPC, dZp2, dC1);

            dPC = (dPC * dZp2);

            dResLo = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dPC, dSHi, dResLo);

            dD = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (-(dZ), dSHi, dD);
            dSLo = as_double (((__constant unsigned long *) (__internal_dsincos_la_data_ha._dT))[(((0 + lIndex) * (4 * 8)) >> (3)) + 3]);

            dCorr = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dE, dD, dSLo);

            dResLo = (dResLo + dCorr);

            dResLarge = (dResHi + dResLo);

            dResLarge = as_double ((as_ulong (dResLarge) ^ as_ulong (dSignX)));

            vr1 = as_double ((((~as_ulong (dReductionRangeMask)) & as_ulong (vr1)) | (as_ulong (dReductionRangeMask) & as_ulong (dResLarge))));

        }
    }

    if ((vm) != 0)
    {
        double _vapi_arg1[1];
        double _vapi_res1[1];
        ((double *) _vapi_arg1)[0] = va1;
        ((double *) _vapi_res1)[0] = vr1;
        __internal_dsin_la_cout (_vapi_arg1, _vapi_res1);
        vr1 = ((double *) _vapi_res1)[0];
    };
    r = vr1;;

    return r;

}

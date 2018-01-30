/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/

#include "include/ia32e_cos_d_g_cout.cl"
#include "include/ia32e_cos_d_g_data.cl"

__attribute__((always_inline))
double1_ref
__ocl_svml_rf_cos1 (double1_ref a)
{
  double va1;
  double vr1;
  VUINT32 vm;
  double1_ref r;

  va1 = a;;

  {
    {

      double dX;
      double dReductionRangeMask;
      VUINT64 lReductionRangeMask;
      VUINT32 mReductionRangeMask;
      double dRangeMask;
      VUINT64 lRangeMask;
      double dN;
      double dY;
      VUINT64 lIndex;
      double dZ;
      double dZp2;
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
      double dXAddHalfPi;
      double dSignRes;
      double dR;
      double dRp2;
      double dPoly;

      double dAbsMask;
      double dReductionRangeVal;
      double dRangeVal;
      double dSigma;
      double dCHL;
      double dSHi;
      double dSLo;
      double dS1;
      double dS2;
      double dRShifter;
      double dOneHalf;
      double dHalfPI;
      double dInvPI;
      double dPI1;
      double dPI2;
      double dPI3;
      double dPI4;
      double dC1;
      double dC2;
      double dC3;
      double dC4;
      double dC5;
      double dC6;
      double dC7;

      vm = 0;

      dAbsMask =
        _castu64_f64 ((*(__constant VUINT64 *) &__ocl_svml_dcos_data._dAbsMask));
      dReductionRangeVal =
        _castu64_f64 ((*(__constant VUINT64 *) &__ocl_svml_dcos_data._dRangeVal));
      dHalfPI =
        _castu64_f64 ((*(__constant VUINT64 *) &__ocl_svml_dcos_data._dHalfPI));

      dXAddHalfPi = (va1 + dHalfPI);

      dX =
        _castu64_f64 ((_castf64_u64 (dXAddHalfPi) & _castf64_u64 (dAbsMask)));

      dReductionRangeMask =
        _castu64_f64 ((VUINT64)
                      (((!(dX <=
                           dReductionRangeVal)) ? 0xffffffffffffffff : 0x0)));
      lReductionRangeMask = _castf64_u64 (dReductionRangeMask);

      dX = va1;

      dInvPI =
        _castu64_f64 ((*(__constant VUINT64 *) &__ocl_svml_dcos_data._dInvPI));
      dRShifter =
        _castu64_f64 ((*(__constant VUINT64 *) &__ocl_svml_dcos_data._dRShifter));

      dY = ((dXAddHalfPi * dInvPI) + dRShifter);

      dN = (dY - dRShifter);
      dOneHalf =
        _castu64_f64 ((*(__constant VUINT64 *) &__ocl_svml_dcos_data._dOneHalf));

      dN = (dN - dOneHalf);

      dSignRes = _castu64_f64 (((VUINT64) _castf64_u64 (dY) << (63)));

      dPI1 = _castu64_f64 ((*(__constant VUINT64 *) &__ocl_svml_dcos_data._dPI1));

      dR = (dX - (dN * dPI1));
      dPI2 = _castu64_f64 ((*(__constant VUINT64 *) &__ocl_svml_dcos_data._dPI2));

      dR = (dR - (dN * dPI2));
      dPI3 = _castu64_f64 ((*(__constant VUINT64 *) &__ocl_svml_dcos_data._dPI3));

      dR = (dR - (dN * dPI3));

      dPI4 = _castu64_f64 ((*(__constant VUINT64 *) &__ocl_svml_dcos_data._dPI4));
      dR = (dR - (dN * dPI4));

      dRp2 = (dR * dR);

      dC7 = _castu64_f64 ((*(__constant VUINT64 *) &__ocl_svml_dcos_data._dC7));
      dC6 = _castu64_f64 ((*(__constant VUINT64 *) &__ocl_svml_dcos_data._dC6));
      dPoly = ((dC7 * dRp2) + dC6);
      dC5 = _castu64_f64 ((*(__constant VUINT64 *) &__ocl_svml_dcos_data._dC5));
      dPoly = ((dPoly * dRp2) + dC5);
      dC4 = _castu64_f64 ((*(__constant VUINT64 *) &__ocl_svml_dcos_data._dC4));
      dPoly = ((dPoly * dRp2) + dC4);
      dC3 = _castu64_f64 ((*(__constant VUINT64 *) &__ocl_svml_dcos_data._dC3));

      dPoly = ((dPoly * dRp2) + dC3);

      dC2 = _castu64_f64 ((*(__constant VUINT64 *) &__ocl_svml_dcos_data._dC2));
      dPoly = ((dPoly * dRp2) + dC2);
      dC1 = _castu64_f64 ((*(__constant VUINT64 *) &__ocl_svml_dcos_data._dC1));
      dPoly = ((dPoly * dRp2) + dC1);
      dPoly = (dPoly * dRp2);
      dPoly = ((dPoly * dR) + dR);

      vr1 = _castu64_f64 ((_castf64_u64 (dPoly) ^ _castf64_u64 (dSignRes)));

      mReductionRangeMask = 0;
      mReductionRangeMask = (VUINT32)lReductionRangeMask;
      if ((mReductionRangeMask) != 0)
        {
          double dResLarge;

          dRangeVal =
            _castu64_f64 ((*(__constant VUINT64 *) &__ocl_svml_dcos_data_ha.
                           _dRangeVal));

          dX = _castu64_f64 ((_castf64_u64 (dRangeVal) & _castf64_u64 (dX)));
          dRangeMask =
            _castu64_f64 ((VUINT64)
                          ((dX == dRangeVal) ? 0xffffffffffffffff : 0x0));
          lRangeMask = _castf64_u64 (dRangeMask);
          vm = 0;
          vm = (VUINT32)lRangeMask;

          {

            VUINT64 lInput;
            VUINT64 lExponent;
            VUINT64 lSignificand;

            VUINT64 lIntegerBit;

            double dP_hi;
            double dP_med;
            double dP_lo;

            VUINT64 lP_hi;
            VUINT64 lP_med;
            VUINT64 lP_lo;

            VUINT64 lLowMask;

            VUINT64 lP5;
            VUINT64 lP4;
            VUINT64 lP3;
            VUINT64 lP2;
            VUINT64 lP1;
            VUINT64 lP0;

            VUINT64 lM1;
            VUINT64 lM0;

            VUINT64 lM15;
            VUINT64 lM14;
            VUINT64 lM13;
            VUINT64 lM12;
            VUINT64 lM11;
            VUINT64 lM10;
            VUINT64 lM05;
            VUINT64 lM04;
            VUINT64 lM03;
            VUINT64 lM02;
            VUINT64 lM01;

            VUINT64 lN14;
            VUINT64 lN13;
            VUINT64 lN12;
            VUINT64 lN11;

            VUINT64 lP15;
            VUINT64 lP14;
            VUINT64 lP13;
            VUINT64 lP12;
            VUINT64 lP11;

            VUINT64 lQ14;
            VUINT64 lQ13;
            VUINT64 lQ12;
            VUINT64 lQ11;

            VUINT64 lReducedHi;
            VUINT64 lReducedLo;

            VUINT64 lNMask;

            double dReducedHi;
            double dReducedMed;
            double dReducedLo;

            VUINT64 lExponentPart;
            VUINT64 lShiftedSig;

            double dShifter;
            double dIntegerPart;

            double dRHi;
            double dRLo;
            VUINT64 lSignBit;

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

            lInput = _castf64_u64 (va1);

            lExponent = 0x7ff0000000000000uLL;;
            lExponent = (lExponent & lInput);
            lExponent = ((VUINT64) (lExponent) >> (52));

            dP_hi =
              _castu64_f64 (((__constant VUINT64
                              *) (__ocl_svml_dsincostan_reduction_data.
                                  _dPtable))[(((0 +
                                                lExponent) * (3 *
                                                              8)) >> (3)) +
                                             0]);
            dP_med =
              _castu64_f64 (((__constant VUINT64
                              *) (__ocl_svml_dsincostan_reduction_data.
                                  _dPtable))[(((0 +
                                                lExponent) * (3 *
                                                              8)) >> (3)) +
                                             1]);
            dP_lo =
              _castu64_f64 (((__constant VUINT64
                              *) (__ocl_svml_dsincostan_reduction_data.
                                  _dPtable))[(((0 +
                                                lExponent) * (3 *
                                                              8)) >> (3)) +
                                             2]);

            lP_hi = _castf64_u64 (dP_hi);
            lP_med = _castf64_u64 (dP_med);
            lP_lo = _castf64_u64 (dP_lo);

            lSignificand = 0x000fffffffffffffuLL;;
            lIntegerBit = 0x0010000000000000uLL;
            lSignificand = (lSignificand & lInput);
            lSignificand = (lSignificand + lIntegerBit);

            lLowMask = 0x00000000FFFFFFFFuLL;
            lP5 = ((VUINT64) (lP_hi) >> (32));
            lP4 = (lP_hi & lLowMask);
            lP3 = ((VUINT64) (lP_med) >> (32));
            lP2 = (lP_med & lLowMask);
            lP1 = ((VUINT64) (lP_lo) >> (32));
            lP0 = (lP_lo & lLowMask);
            lM1 = ((VUINT64) (lSignificand) >> (32));
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

            lN11 = ((VUINT64) (lM01) >> (32));
            lN12 = ((VUINT64) (lM02) >> (32));
            lN13 = ((VUINT64) (lM03) >> (32));
            lN14 = ((VUINT64) (lM04) >> (32));

            lN11 = (lM11 + lN11);
            lN12 = (lM12 + lN12);
            lN13 = (lM13 + lN13);
            lN14 = (lM14 + lN14);

            lP11 = (lM02 & lLowMask);
            lP12 = (lM03 & lLowMask);
            lP13 = (lM04 & lLowMask);
            lP14 = (lM05 & lLowMask);
            lP15 = ((VUINT64) (lM05) >> (32));

            lP11 = (lP11 + lN11);
            lP12 = (lP12 + lN12);
            lP13 = (lP13 + lN13);
            lP14 = (lP14 + lN14);
            lP15 = (lP15 + lM15);

            lQ11 = ((VUINT64) (lM10) >> (32));
            lQ11 = (lQ11 + lP11);

            lQ12 = ((VUINT64) (lQ11) >> (32));
            lQ12 = (lQ12 + lP12);

            lQ13 = ((VUINT64) (lQ12) >> (32));
            lQ13 = (lQ13 + lP13);

            lQ14 = ((VUINT64) (lQ13) >> (32));
            lQ14 = (lQ14 + lP14);

            lQ11 = (lQ11 & lLowMask);
            lQ13 = (lQ13 & lLowMask);

            lReducedHi = ((VUINT64) (lQ14) << (32));
            lReducedLo = ((VUINT64) (lQ12) << (32));

            lReducedHi = (lReducedHi + lQ13);
            lReducedLo = (lReducedLo + lQ11);

            lSignBit = 0x8000000000000000uLL;;
            lSignBit = (lSignBit & lInput);

            lExponentPart = 0x3FF0000000000000uLL;
            lExponentPart = (lSignBit ^ lExponentPart);
            lShiftedSig = ((VUINT64) (lReducedHi) >> (12));
            lShiftedSig = (lShiftedSig | lExponentPart);
            dReducedHi = _castu64_f64 (lShiftedSig);

            dShifter = _castu64_f64 (0x42A8000000000000uLL);
            dIntegerPart = (dShifter + dReducedHi);
            dN = (dIntegerPart - dShifter);
            dReducedHi = (dReducedHi - dN);

            lIndex = _castf64_u64 (dIntegerPart);
            lNMask = 0x00000000000001FFuLL;
            lIndex = (lIndex & lNMask);

            lExponentPart = 0x3970000000000000uLL;
            lExponentPart = (lSignBit ^ lExponentPart);
            lShiftedSig = 0x0000000000FFFFFFuLL;
            lShiftedSig = (lShiftedSig & lReducedLo);
            lShiftedSig = ((VUINT64) (lShiftedSig) << (28));
            lShiftedSig = (lShiftedSig | lExponentPart);
            dReducedLo = _castu64_f64 (lShiftedSig);
            dShifter = _castu64_f64 (lExponentPart);
            dReducedLo = (dReducedLo - dShifter);

            lExponentPart = 0x3CB0000000000000uLL;
            lExponentPart = (lSignBit ^ lExponentPart);
            lShiftedSig = 0x0000000000000FFFuLL;
            lShiftedSig = (lShiftedSig & lReducedHi);
            lShiftedSig = ((VUINT64) (lShiftedSig) << (40));
            lReducedLo = ((VUINT64) (lReducedLo) >> (24));
            lShiftedSig = (lShiftedSig | lReducedLo);
            lShiftedSig = (lShiftedSig | lExponentPart);
            dReducedMed = _castu64_f64 (lShiftedSig);
            dShifter = _castu64_f64 (lExponentPart);
            dReducedMed = (dReducedMed - dShifter);

            dRHi = (dReducedHi + dReducedMed);
            dReducedHi = (dReducedHi - dRHi);
            dReducedMed = (dReducedMed + dReducedHi);
            dRLo = (dReducedMed + dReducedLo);

            d2pi_full = _castu64_f64 (0x401921FB54442D18uLL);
            d2pi_lead = _castu64_f64 (0x401921FB54000000uLL);
            d2pi_trail = _castu64_f64 (0x3E310B4611A62633uLL);

            dLeadmask = _castu64_f64 (0xFFFFFFFFF8000000uLL);
            dRHi_lead =
              _castu64_f64 ((_castf64_u64 (dRHi) & _castf64_u64 (dLeadmask)));
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

            dAbsMask = _castu64_f64 (0x7FFFFFFFFFFFFFFFuLL);
            dMinInput = _castu64_f64 (0x3EB0000000000000uLL);
            dAbs =
              _castu64_f64 ((_castf64_u64 (va1) & _castf64_u64 (dAbsMask)));
            dMultiplex =
              _castu64_f64 ((VUINT64)
                            ((dAbs > dMinInput) ? 0xffffffffffffffff : 0x0));
            dNotMultiplex =
              _castu64_f64 ((VUINT64)
                            ((dAbs <= dMinInput) ? 0xffffffffffffffff : 0x0));

            dMultiplexedInput =
              _castu64_f64 ((_castf64_u64 (dNotMultiplex) &
                             _castf64_u64 (va1)));
            dMultiplexedOutput =
              _castu64_f64 ((_castf64_u64 (dMultiplex) &
                             _castf64_u64 (dRedHi)));
            dZ =
              _castu64_f64 ((_castf64_u64 (dMultiplexedInput) |
                             _castf64_u64 (dMultiplexedOutput)));

            dE =
              _castu64_f64 ((_castf64_u64 (dMultiplex) &
                             _castf64_u64 (dRedLo)));

          }

          dCHL =
            _castu64_f64 (((__constant VUINT64 *) (__ocl_svml_dcos_data_ha.
                                              _dT))[(((0 +
                                                       lIndex) * (4 *
                                                                  8)) >> (3))
                                                    + 0]);
          dSHi =
            _castu64_f64 (((__constant VUINT64 *) (__ocl_svml_dcos_data_ha.
                                              _dT))[(((0 +
                                                       lIndex) * (4 *
                                                                  8)) >> (3))
                                                    + 2]);
          dSigma =
            _castu64_f64 (((__constant VUINT64 *) (__ocl_svml_dcos_data_ha.
                                              _dT))[(((0 +
                                                       lIndex) * (4 *
                                                                  8)) >> (3))
                                                    + 1]);

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

          dS2 =
            _castu64_f64 ((*(__constant VUINT64 *) &__ocl_svml_dcos_data_ha._dS2));
          dS1 =
            _castu64_f64 ((*(__constant VUINT64 *) &__ocl_svml_dcos_data_ha._dS1));

          dPS = ((dS2 * dZp2) + dS1);

          dPS = (dPS * dZp2);

          dPS = (dPS * dZ);

          dD = (dCHL + dSigma);

          dResLo = ((dPS * dD) + dResLo);

          dC3 =
            _castu64_f64 ((*(__constant VUINT64 *) &__ocl_svml_dcos_data_ha._dC3));
          dC2 =
            _castu64_f64 ((*(__constant VUINT64 *) &__ocl_svml_dcos_data_ha._dC2));

          dPC = ((dC3 * dZp2) + dC2);
          dC1 =
            _castu64_f64 ((*(__constant VUINT64 *) &__ocl_svml_dcos_data_ha._dC1));

          dPC = ((dPC * dZp2) + dC1);

          dPC = (dPC * dZp2);

          dResLo = ((dPC * dSHi) + dResLo);

          dD = (dD - (dZ * dSHi));
          dSLo =
            _castu64_f64 (((__constant VUINT64 *) (__ocl_svml_dcos_data_ha.
                                              _dT))[(((0 +
                                                       lIndex) * (4 *
                                                                  8)) >> (3))
                                                    + 3]);

          dCorr = ((dE * dD) + dSLo);

          dResLo = (dResLo + dCorr);

          dResLarge = (dResHi + dResLo);

          vr1 =
            _castu64_f64 ((((~_castf64_u64 (dReductionRangeMask)) &
                            _castf64_u64 (vr1)) |
                           (_castf64_u64 (dReductionRangeMask) &
                            _castf64_u64 (dResLarge))));

        }

    }

    ;
  }

  {
    if ((vm) != 0)
      {
        double _vapi_arg1[1];
        double _vapi_res1[1];
        _vapi_arg1[0] = va1;
        _vapi_res1[0] = vr1;
        __ocl_svml_dcos_cout_rare (_vapi_arg1, _vapi_res1);
        vr1 = _vapi_res1[0];
      }
  };
  r = vr1;;

  return r;
}

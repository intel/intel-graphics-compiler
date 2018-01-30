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

#include "include/ia32e_tanpi_d_g_cout.cl"
#include "include/ia32e_tanpi_d_g_data.cl"

__attribute__((always_inline))
double1_ref
__ocl_svml_rf_tanpi1 (double1_ref a)
{
  double va1;
  double vr1;
  VUINT32 vm;
  double1_ref r;

  va1 = a;;

  {
    {

      double dX;
      double dN;
      VUINT64 lIndex;
      double dZ;
      double dAbsMask;
      double dShiftedN;
      double dPi;
      VUINT64 lIndexPeriodMask;
      double dB_hi;
      double dR_full;
      double dOne;
      double dTau;
      double dRecip_hi;
      double dEr;
      double dE_1;
      double dE2_1;
      double dE2;
      double dRE;
      double dZ2;
      double dZ4;
      double dH1;
      double dH2;
      double dH4;
      double dC0_hi;
      double dC1_hi;
      double dC1_lo;
      double dC2;
      double dC3;
      double dC4;
      double dC5;
      double dC6;
      double dC7;
      double dP1;
      double dP2;
      double dP3;
      double dP4;
      double dP5;
      double dP6;
      double dP7;
      double dP8;
      double dP9;
      double dP10;
      double dP11;
      double dP12;
      double dP14;

      double dExpMask;
      double dEMax;
      double dSpecialMask;
      VUINT64 lSpecialMask;

      double dShifterMask;
      double dShifterPos;
      double dShifterThreshold;
      double dShifter;

      double dSignMask;
      double dZero;
      double dSingular;
      double dCotangent;
      double dPi2Multiple;
      double dP17;
      double dInfinity;
      double dSignedInfinity;

      dExpMask = _castu64_f64 (0x7ff0000000000000uLL);;
      dZ = _castu64_f64 ((_castf64_u64 (va1) & _castf64_u64 (dExpMask)));
      dEMax = _castu64_f64 (0x7FF0000000000000uLL);
      dSpecialMask =
        _castu64_f64 ((VUINT64) ((dZ == dEMax) ? 0xffffffffffffffff : 0x0));
      lSpecialMask = _castf64_u64 (dSpecialMask);

      vm = 0;
      vm = lSpecialMask;

      dAbsMask =
        _castu64_f64 ((*(__constant VUINT64 *) &__ocl_svml_dtanpi_data._dAbsMask));
      dX = _castu64_f64 ((_castf64_u64 (va1) & _castf64_u64 (dAbsMask)));
      dShifterThreshold = _castu64_f64 (0x43A0000000000000uLL);
      dShifterMask =
        _castu64_f64 ((VUINT64)
                      ((dX < dShifterThreshold) ? 0xffffffffffffffff : 0x0));

      dShifterPos = _castu64_f64 (0x43B8000000000000uLL);
      dZero = _castu64_f64 (0x0000000000000000uLL);
      dShifter =
        _castu64_f64 ((((~_castf64_u64 (dShifterMask)) & _castf64_u64 (dZero))
                       | (_castf64_u64 (dShifterMask) &
                          _castf64_u64 (dShifterPos))));

      dShiftedN = (dShifter + va1);
      dN = (dShiftedN - dShifter);
      dZ = (va1 - dN);

      dShifter = _castu64_f64 (0x42B8000000000000uLL);
      dShiftedN = (dShifter + dZ);
      dN = (dShiftedN - dShifter);
      dZ = (dZ - dN);

      dSignedInfinity =
        _castu64_f64 (((VUINT64) _castf64_u64 (dShiftedN) << (55)));
      dSignMask = _castu64_f64 (0x8000000000000000uLL);;
      dSignedInfinity =
        _castu64_f64 ((_castf64_u64 (dSignedInfinity) &
                       _castf64_u64 (dSignMask)));
      dInfinity = _castu64_f64 (0x7FF0000000000000uLL);
      dSignedInfinity =
        _castu64_f64 ((_castf64_u64 (dSignedInfinity) |
                       _castf64_u64 (dInfinity)));

      lIndex = _castf64_u64 (dShiftedN);
      lIndexPeriodMask = 0x00000000000000FFuLL;
      lIndex = (lIndex & lIndexPeriodMask);

      dPi = _castu64_f64 (0x400921FB54442D18uLL);
      dZ = (dZ * dPi);

      dB_hi =
        _castu64_f64 (((__constant VUINT64 *) (__ocl_svml_dtanpi_data.
                                          _dCoeffs))[(((0 +
                                                        lIndex) * (13 *
                                                                   8)) >> (3))
                                                     + 0]);
      dTau =
        _castu64_f64 (((__constant VUINT64 *) (__ocl_svml_dtanpi_data.
                                          _dCoeffs))[(((0 +
                                                        lIndex) * (13 *
                                                                   8)) >> (3))
                                                     + 2]);
      dC0_hi =
        _castu64_f64 (((__constant VUINT64 *) (__ocl_svml_dtanpi_data.
                                          _dCoeffs))[(((0 +
                                                        lIndex) * (13 *
                                                                   8)) >> (3))
                                                     + 3]);
      dC1_hi =
        _castu64_f64 (((__constant VUINT64 *) (__ocl_svml_dtanpi_data.
                                          _dCoeffs))[(((0 +
                                                        lIndex) * (13 *
                                                                   8)) >> (3))
                                                     + 5]);
      dC1_lo =
        _castu64_f64 (((__constant VUINT64 *) (__ocl_svml_dtanpi_data.
                                          _dCoeffs))[(((0 +
                                                        lIndex) * (13 *
                                                                   8)) >> (3))
                                                     + 6]);
      dC2 =
        _castu64_f64 (((__constant VUINT64 *) (__ocl_svml_dtanpi_data.
                                          _dCoeffs))[(((0 +
                                                        lIndex) * (13 *
                                                                   8)) >> (3))
                                                     + 7]);
      dC3 =
        _castu64_f64 (((__constant VUINT64 *) (__ocl_svml_dtanpi_data.
                                          _dCoeffs))[(((0 +
                                                        lIndex) * (13 *
                                                                   8)) >> (3))
                                                     + 8]);
      dC4 =
        _castu64_f64 (((__constant VUINT64 *) (__ocl_svml_dtanpi_data.
                                          _dCoeffs))[(((0 +
                                                        lIndex) * (13 *
                                                                   8)) >> (3))
                                                     + 9]);
      dC5 =
        _castu64_f64 (((__constant VUINT64 *) (__ocl_svml_dtanpi_data.
                                          _dCoeffs))[(((0 +
                                                        lIndex) * (13 *
                                                                   8)) >> (3))
                                                     + 10]);
      dC6 =
        _castu64_f64 (((__constant VUINT64 *) (__ocl_svml_dtanpi_data.
                                          _dCoeffs))[(((0 +
                                                        lIndex) * (13 *
                                                                   8)) >> (3))
                                                     + 11]);
      dC7 =
        _castu64_f64 (((__constant VUINT64 *) (__ocl_svml_dtanpi_data.
                                          _dCoeffs))[(((0 +
                                                        lIndex) * (13 *
                                                                   8)) >> (3))
                                                     + 12]);

      dR_full = (dB_hi - dZ);

      dZero = _castu64_f64 (0);
      dPi2Multiple =
        _castu64_f64 ((VUINT64)
                      ((dR_full == dZero) ? 0xffffffffffffffff : 0x0));
      dCotangent =
        _castu64_f64 ((VUINT64)
                      (((!(dTau == dZero)) ? 0xffffffffffffffff : 0x0)));
      dSingular =
        _castu64_f64 ((_castf64_u64 (dPi2Multiple) &
                       _castf64_u64 (dCotangent)));

      dRecip_hi = ((double) (1.0f / ((float) (dR_full))));

      dOne = _castu64_f64 (0x3FF0000000000000uLL);
      dEr = (dR_full * dRecip_hi);
      dEr = (dOne - dEr);

      dRE = (dRecip_hi * dEr);
      dE2 = (dEr * dEr);
      dE_1 = (dOne + dEr);
      dE2_1 = (dOne + dE2);
      dRE = (dRE * dE_1);
      dRE = (dRE * dE2_1);
      dRecip_hi = (dRecip_hi + dRE);

      dRecip_hi = (dRecip_hi * dTau);

      dH1 = (dC1_hi * dZ);

      dH2 = (dC0_hi + dH1);
      dH4 = (dH2 + dRecip_hi);

      dZ2 = (dZ * dZ);
      dP1 = (dC2 * dZ);
      dP2 = (dC4 * dZ);
      dP3 = (dC6 * dZ);

      dZ4 = (dZ2 * dZ2);
      dP4 = (dC1_lo + dP1);
      dP5 = (dC3 + dP2);
      dP6 = (dC5 + dP3);
      dP7 = (dC7 * dZ2);

      dP8 = (dZ2 * dP5);
      dP9 = (dP6 + dP7);

      dP10 = (dP4 + dP8);
      dP11 = (dZ4 * dP9);

      dP12 = (dP10 + dP11);

      dP14 = (dP12 * dZ);

      dP17 = (dH4 + dP14);

      vr1 =
        _castu64_f64 ((((~_castf64_u64 (dSingular)) & _castf64_u64 (dP17)) |
                       (_castf64_u64 (dSingular) &
                        _castf64_u64 (dSignedInfinity))));

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
        __ocl_svml_dtanpi_cout_rare (_vapi_arg1, _vapi_res1);
        vr1 = (const double)_vapi_res1[0];
      }
  };
  r = vr1;;

  return r;
}

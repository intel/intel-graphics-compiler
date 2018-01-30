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

#include "include/ia32e_rootn_d_g_cout.cl"
#include "include/ia32e_rootn_d_g_data.cl"

__attribute__((always_inline))
double1_ref
__ocl_svml_rf_rootn1 (double1_ref a, int1_ref b)
{
  double va1;
  VUINT32 va2;
  double vr1;
  VUINT32 vm;
  double1_ref r;

  va1 = a;
  va2 = b;

  {
    {

      VUINT32 _HIDELTA;
      VUINT32 _LORANGE;
      VUINT32 _ABSMASK;
      VUINT32 _INF;
      VUINT32 _DOMAINRANGE;
      VUINT64 lX;
      VUINT64 lY;
      VUINT64 lZ;
      VUINT32 iXHi;
      VUINT32 iYHi;
      VUINT32 iZ;
      VUINT32 iSpecX;
      VUINT32 iSpecY;
      VUINT32 iSpecZ;
      VUINT32 iAbsZ;
      VUINT32 iRangeMask;
      double dMantissaMask;
      double dX1;
      double dOne;
      VUINT32 iIndex;
      VUINT32 iIndexMask;
      VUINT32 iIndexAdd;
      VUINT32 iIndexR;
      VUINT32 iIndexL;
      double dRcp;
      double dL1[2];
      VUINT32 iK;
      VUINT32 i3fe7fe00;
      VUINT32 i2p20_2p19;
      VUINT64 lK;
      double dK;
      double dHighHalfMask;
      double db2p20_2p19;
      double dX1Hi;
      double dX1Lo;
      double dHighMask;
      double dR1;
      double LHN;
      double dCq;
      double dE;
      double dT;
      double dTRh;
      double dRl;
      double dTRhEh;
      double dHLL;
      double dA[8];
      double dPoly;
      double dTRhEhHLLhi;
      double dHLLHi;
      double dHH;
      double dHL;

      double dBHL[2];
      double dYHL[2];
      double dPH;
      double dPL;
      double dPLL;
      double dPHH;
      double dPHL;
      double db2p45_2p44;
      VUINT64 lPHH;
      VUINT32 iPHH;
      VUINT32 iN;
      VUINT32 iOne;
      VUINT64 lN;
      double dN;
      double dExpSignMask;

      VUINT32 jIndex;
      VUINT32 jIndexMask;
      double dExpT[2];
      double dT2;
      double dT4;
      double dB[6];
      double dRes;
      double dTmp;

      double dX;
      double dY;
      double dAbsMask;
      VUINT32 iOddY;
      VUINT64 lNegX;
      VUINT32 iNegX;
      VUINT32 iResultSign;
      VUINT64 lResultSign;
      double dResultSign;
      VUINT32 iZero;
      VUINT32 iEvenY;
      VUINT32 iNegXEvenY;

      iZero = 0;

      dX = va1;
      dY = ((double) ((VINT32) (va2)));

      iOddY = ((VUINT32) (va2) << (31));
      lNegX = _castf64_u64 (dX);
      iNegX = ((VUINT32) ((VUINT64) lNegX >> 32));
      iResultSign = (iNegX & iOddY);
      lResultSign = (((VUINT64) (VUINT32) iResultSign << 32));
      dResultSign = _castu64_f64 (lResultSign);

      iNegX = ((VUINT32) (-(VSINT32) ((VSINT32) iNegX < (VSINT32) iZero)));
      iEvenY = ((VUINT32) (-(VSINT32) ((VSINT32) iOddY == (VSINT32) iZero)));
      iNegXEvenY = (iNegX & iEvenY);

      dAbsMask =
        _castu64_f64 ((*(__constant VUINT64 *) &__ocl_svml_drootn_data._dAbsMask));
      dX = _castu64_f64 ((_castf64_u64 (dX) & _castf64_u64 (dAbsMask)));

      dOne = _castu64_f64 (0x3ff0000000000000uLL);

      dBHL[0] = dY;
      dBHL[1] = _castu64_f64 (0);

      {
        double T_t1;
        double T_v2;
        double T_v3;
        double T_ONE;
        double T_C;
        T_ONE = _castu64_f64 (0x3ff0000000000000uLL);
        T_C = _castu64_f64 (0x41A0000002000000uLL);
        T_t1 = (T_ONE / dBHL[0]);
        T_v2 = (T_t1 * T_C);
        T_v3 = (T_v2 - T_t1);
        T_v3 = (T_v2 - T_v3);
        T_t1 = (dBHL[0] * T_v3);
        T_t1 = (T_ONE - T_t1);
        T_v2 = (dBHL[1] * T_v3);
        T_v2 = (T_t1 - T_v2);
        T_t1 = (T_ONE + T_v2);
        dYHL[0] = T_v3;
        T_t1 = (T_t1 * T_v2);
        dYHL[1] = (T_t1 * T_v3);
      };

      lX = _castf64_u64 (dX);
      lY = _castf64_u64 (dYHL[0]);
      iXHi = ((VUINT32) ((VUINT64) lX >> 32));
      iYHi = ((VUINT32) ((VUINT64) lY >> 32));

      _HIDELTA = (*(__constant VUINT32 *) &__ocl_svml_drootn_data.HIDELTA);
      _LORANGE = (*(__constant VUINT32 *) &__ocl_svml_drootn_data.LORANGE);
      _ABSMASK = (*(__constant VUINT32 *) &__ocl_svml_drootn_data.ABSMASK);
      _INF = (*(__constant VUINT32 *) &__ocl_svml_drootn_data.INF);

      iSpecX = (iXHi + _HIDELTA);
      iSpecX =
        ((VUINT32) (-(VSINT32) ((VSINT32) iSpecX < (VSINT32) _LORANGE)));

      iSpecY = (iYHi & _ABSMASK);
      iSpecY = ((VUINT32) (-(VSINT32) ((VSINT32) iSpecY >= (VSINT32) _INF)));
      iRangeMask = (iSpecX | iSpecY);
      iRangeMask = (iRangeMask | iNegXEvenY);

      dMantissaMask =
        _castu64_f64 ((*(__constant VUINT64 *) &__ocl_svml_drootn_data.
                       iMantissaMask));
      dX1 = _castu64_f64 ((_castf64_u64 (dX) & _castf64_u64 (dMantissaMask)));
      dOne =
        _castu64_f64 ((*(__constant VUINT64 *) &__ocl_svml_drootn_data.dbOne));
      dX1 = _castu64_f64 ((_castf64_u64 (dX1) | _castf64_u64 (dOne)));

      iIndex = ((VUINT32) ((VUINT64) lX >> 32));
      iIndexMask = (*(__constant VUINT32 *) &__ocl_svml_drootn_data.iIndexMask);
      iIndex = (iIndex & iIndexMask);
      iIndexAdd = (*(__constant VUINT32 *) &__ocl_svml_drootn_data.iIndexAdd);
      iIndex = (iIndex + iIndexAdd);

      iIndex = ((VUINT32) (iIndex) >> (10));
      iIndexR = ((VUINT32) (iIndex) << (3));
      iIndexL = ((VUINT32) (iIndex) << (4));

      dRcp =
        _castu64_f64 (((__constant VUINT64 *) (__ocl_svml_drootn_data.
                                          rcp_t1))[iIndexR >> 3]);
      dL1[0] =
        _castu64_f64 (((__constant VUINT64 *) (__ocl_svml_drootn_data.
                                          log2_t1))[iIndexL >> 3]);
      dL1[1] =
        _castu64_f64 (((__constant VUINT64 *) (__ocl_svml_drootn_data.
                                    log2_t1))[(iIndexL >> 3) + 1]);

      i3fe7fe00 = (*(__constant VUINT32 *) &__ocl_svml_drootn_data.i3fe7fe00);
      iK = (iXHi - i3fe7fe00);
      iK = ((VSINT32) iK >> (20));
      i2p20_2p19 = (*(__constant VUINT32 *) &__ocl_svml_drootn_data.i2p20_2p19);
      iK = (iK + i2p20_2p19);
      lK = (((VUINT64) (VUINT32) iK << 32) | (VUINT64) (VUINT32) iK);
      dK = _castu64_f64 (lK);
      dHighHalfMask =
        _castu64_f64 ((*(__constant VUINT64 *) &__ocl_svml_drootn_data.
                       iffffffff00000000));
      dK = _castu64_f64 ((_castf64_u64 (dK) & _castf64_u64 (dHighHalfMask)));
      db2p20_2p19 =
        _castu64_f64 ((*(__constant VUINT64 *) &__ocl_svml_drootn_data.
                       db2p20_2p19));
      dK = (dK - db2p20_2p19);

      dHighMask =
        _castu64_f64 ((*(__constant VUINT64 *) &__ocl_svml_drootn_data.iHighMask));
      dX1Hi = _castu64_f64 ((_castf64_u64 (dX1) & _castf64_u64 (dHighMask)));
      dX1Lo = (dX1 - dX1Hi);

      dR1 = (dX1 * dRcp);

      LHN = _castu64_f64 ((*(__constant VUINT64 *) &__ocl_svml_drootn_data.LHN));
      dCq = (dR1 + LHN);

      dE = ((dX1Hi * dRcp) - dR1);

      dE = ((dX1Lo * dRcp) + dE);

      dT = (dK + dL1[0]);

      dTRh = (dT + dCq);

      dRl = (dT - dTRh);

      dRl = (dRl + dCq);

      dTRhEh = (dTRh + dE);

      dHLL = (dTRh - dTRhEh);

      dHLL = (dHLL + dE);

      dHLL = (dHLL + dRl);

      dHLL = (dHLL + dL1[1]);

      dCq = (dCq + dE);

      dA[6] =
        _castu64_f64 ((*(__constant VUINT64 *) &__ocl_svml_drootn_data.clv[1]));
      dA[5] =
        _castu64_f64 ((*(__constant VUINT64 *) &__ocl_svml_drootn_data.clv[2]));
      dPoly = ((dA[6] * dCq) + dA[5]);
      dA[4] =
        _castu64_f64 ((*(__constant VUINT64 *) &__ocl_svml_drootn_data.clv[3]));
      dPoly = ((dPoly * dCq) + dA[4]);
      dA[3] =
        _castu64_f64 ((*(__constant VUINT64 *) &__ocl_svml_drootn_data.clv[4]));
      dPoly = ((dPoly * dCq) + dA[3]);
      dA[2] =
        _castu64_f64 ((*(__constant VUINT64 *) &__ocl_svml_drootn_data.clv[5]));
      dPoly = ((dPoly * dCq) + dA[2]);
      dA[1] =
        _castu64_f64 ((*(__constant VUINT64 *) &__ocl_svml_drootn_data.clv[6]));
      dPoly = ((dPoly * dCq) + dA[1]);
      dHLL = ((dPoly * dCq) + dHLL);

      dTRhEhHLLhi = (dTRhEh + dHLL);

      dHLLHi = (dTRhEhHLLhi - dTRhEh);

      dHLL = (dHLL - dHLLHi);

      dHH =
        _castu64_f64 ((_castf64_u64 (dTRhEhHLLhi) &
                       _castf64_u64 (dHighMask)));

      dHL = (dTRhEhHLLhi - dHH);

      dPH = (dYHL[0] * dHH);

      lZ = _castf64_u64 (dPH);
      iZ = ((VUINT32) ((VUINT64) lZ >> 32));
      iAbsZ = (iZ & _ABSMASK);
      _DOMAINRANGE = (*(__constant VUINT32 *) &__ocl_svml_drootn_data.DOMAINRANGE);
      iSpecZ =
        ((VUINT32) (-(VSINT32) ((VSINT32) iAbsZ >= (VSINT32) _DOMAINRANGE)));
      iRangeMask = (iRangeMask | iSpecZ);
      vm = 0;
      vm = iRangeMask;

      dPL = (dYHL[1] * dHL);
      dPL = ((dYHL[0] * dHL) + dPL);
      dPL = ((dYHL[1] * dHH) + dPL);

      dPLL = (dYHL[0] * dHLL);

      db2p45_2p44 =
        _castu64_f64 ((*(__constant VUINT64 *) &__ocl_svml_drootn_data.
                       db2p45_2p44));
      dPHH = (dPH + db2p45_2p44);
      lPHH = _castf64_u64 (dPHH);
      iPHH = (((VUINT32) lPHH & (VUINT32) - 1));

      jIndexMask = (*(__constant VUINT32 *) &__ocl_svml_drootn_data.jIndexMask);
      jIndex = (iPHH & jIndexMask);
      jIndex = ((VUINT32) (jIndex) << (4));

      iN = ((VUINT32) (iPHH) << (13));
      iOne = (*(__constant VUINT32 *) &__ocl_svml_drootn_data.iOne);
      iN = (iN + iOne);
      lN = (((VUINT64) (VUINT32) iN << 32) | (VUINT64) (VUINT32) iN);
      dN = _castu64_f64 (lN);
      dExpSignMask =
        _castu64_f64 ((*(__constant VUINT64 *) &__ocl_svml_drootn_data.
                       ifff0000000000000));
      dN = _castu64_f64 ((_castf64_u64 (dN) & _castf64_u64 (dExpSignMask)));

      dExpT[0] =
        _castu64_f64 (((__constant VUINT64 *) (__ocl_svml_drootn_data.
                                          exp2_tbl))[jIndex >> 3]);
      dExpT[1] =
        _castu64_f64 (((__constant VUINT64 *) (__ocl_svml_drootn_data.
                                    exp2_tbl))[(jIndex >> 3) + 1]);

      dPHH = (dPHH - db2p45_2p44);

      dPHL = (dPH - dPHH);

      dT = (dPL + dPLL);
      dT = (dT + dPHL);

      dT2 = (dT * dT);
      dT4 = (dT2 * dT2);
      dB[5] =
        _castu64_f64 ((*(__constant VUINT64 *) &__ocl_svml_drootn_data.cev[0]));
      dB[4] =
        _castu64_f64 ((*(__constant VUINT64 *) &__ocl_svml_drootn_data.cev[1]));
      dRes = ((dB[5] * dT) + dB[4]);
      dRes = (dRes * dT4);
      dRes = (dRes * dExpT[0]);

      dRes = (dRes + dExpT[1]);

      dB[3] =
        _castu64_f64 ((*(__constant VUINT64 *) &__ocl_svml_drootn_data.cev[2]));
      dB[2] =
        _castu64_f64 ((*(__constant VUINT64 *) &__ocl_svml_drootn_data.cev[3]));
      dTmp = ((dB[3] * dT) + dB[2]);
      dTmp = (dTmp * dT2);
      dTmp = (dTmp * dExpT[0]);

      dRes = (dRes + dTmp);

      dB[1] =
        _castu64_f64 ((*(__constant VUINT64 *) &__ocl_svml_drootn_data.cev[4]));
      dTmp = (dB[1] * dT);
      dTmp = (dTmp * dExpT[0]);

      dRes = (dRes + dTmp);

      dRes = (dRes + dExpT[0]);

      vr1 = (dRes * dN);

      vr1 = _castu64_f64 ((_castf64_u64 (vr1) | _castf64_u64 (dResultSign)));

    }

    ;
  }

  {
    if ((vm) != 0)
      {
        double _vapi_arg1[1];
        __int32 _vapi_arg2[1];
        double _vapi_res1[1];
        _vapi_arg1[0] = va1;
        ((__private VUINT32 *) _vapi_arg2)[0] = va2;
        _vapi_res1[0] = vr1;
        __ocl_svml_drootn_cout_rare (_vapi_arg1, _vapi_arg2, _vapi_res1);
        vr1 = (const double) _vapi_res1[0];
      }
  };
  r = vr1;;

  return r;
}

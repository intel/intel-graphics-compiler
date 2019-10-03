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
// SVML code
/*
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//    Copyright (C) 1996-2010 Intel Corporation. All Rights Reserved.
//
*/

#include "include/svml_power_data.cl"
#include "include/svml_test_int.cl"

#if defined(cl_khr_fp64)
# include "../MathDouble/rf_rootn_d_1_g.cl"
#endif

__attribute__((always_inline))
int __ocl_svml_srootn_cout_rare (private const float *a, private const int *b, private float *r)
{
  int nRet = 0;

  float flVTmp1, flVTmp2, flVPHH, flVPHL;
  float flAX, flSignRes, flX1, flRcp1, flL1Hi, flL1Lo, flX2, flRcp2, flL2Hi,
  flL2Lo, flX3, flRcp3C, flL3Hi, flL3Lo, flK, flT, flD, flR1, flCQ, flRcpC,
  flX1Hi, flX1Lo, flRcpCHi, flRcpCLo, flTmp1, flE, flT_CQHi, flCQLo, flR,
  flLogPart3, flLog2Poly, flHH, flHL, flHLL, flYHi, flYLo, flTmp2, flTmp3,
  flPH, flPL, flPLL, flZ, flExp2Poly, flExp2PolyT, flResLo, flResHi, flRes,
  flTwoPowN, flAi, flBi, flVTmp3;
  float flT_lo_1, flT_lo_2, flT_lo_3;
  float fResSign = 1.0f, flBOrig_Lo;

  int iEXB, iEYB, iSignX, iSignY, iYIsFinite, iEY, iYIsInt,
  iXIsFinite, iDenoExpAdd, iXHi, k, i1, i2, i3, iELogAX, iN, j, iERes,
  iSign, iIsSigZeroX, iIsSigZeroY, iYMantissa, iEX, iBi = *b;
  float flBOrig = (float) (*b);

  flBOrig_Lo = (float) (iBi - (int) flBOrig);

  flAi = *a;
  flBi = 1.0f / (float) (*b);

  iEXB = ((as_uint(flAi) >> 23) & 0xFF);
  iEYB = ((as_uint(flBi) >> 23) & 0xFF);

  iEX = iEXB - 0x7F;
  iEY = iEYB - 0x7F;

  iSignX = (as_uint(flAi) >> 31);
  iSignY = (as_uint(flBi) >> 31);

  iIsSigZeroX = ((as_uint(flAi) & 0x007FFFFF) == 0);
  iIsSigZeroY = ((as_uint(flBi) & 0x007FFFFF) == 0);

  iYIsFinite = (((as_uint(flBi) >> 23) & 0xFF) != 0xFF);

  int iXisZero = ((iEXB == 0) & (iIsSigZeroX));
    int iYisZero = (*b == 0);
  int iXisINF = (!((((as_uint(flAi) >> 23) & 0xFF) != 0xFF))) & ((iIsSigZeroX));
  int iYisOdd = (*b) & 1;
  int iYisEven = iYisOdd ^ 1;

  uint resMask = ~0;
  uint testMask = 0;
  uint rUI = 0;

  #define TEST_AND_SET(MASK, VAL)\
     testMask = (MASK)&resMask;\
     rUI = (testMask & (VAL)) | ((~testMask) & rUI);\
     resMask = resMask & (~testMask);

  #define TO_MASK(X) -(unsigned int)(X)

  uint YisZeroMask = TO_MASK(iYisZero);
  uint XisZeroMask = TO_MASK(iXisZero);
  uint YisEvenMask = TO_MASK(iYisEven);
  uint SignXMask = TO_MASK(iSignX);
  uint SignYMask = TO_MASK(iSignY);
  uint XisInfMask = TO_MASK(iXisINF);
  uint IsSigZeroXMask = TO_MASK(iIsSigZeroX);
  uint IsSigZeroYMask = TO_MASK(iIsSigZeroY);

  float rootnHATab370 = ((__constant float *) _vmlsRootnHATab)[370];
  float rootnHATab371 = ((__constant float *) _vmlsRootnHATab)[371];
  if(YisZeroMask | XisZeroMask | XisInfMask | (SignXMask & YisEvenMask))
    {
      float rootnHATab370Div = rootnHATab370/rootnHATab370;
      float rootnHATab371Div370 = rootnHATab371/rootnHATab370;
      float rootnHATab370Div370 = rootnHATab370/rootnHATab370;
      float rootnHATab371DivA   = rootnHATab371/(*a);
      uint rootnHATab370UI  = as_uint(rootnHATab370);
      uint rootnHATab370DivUI = as_uint(rootnHATab370Div);
      uint rootnHATab371Div370UI = as_uint(rootnHATab371Div370);
      uint rootnHATab370Div370UI = as_uint(rootnHATab370Div370);
      uint rootnHATab371DivAUI   = as_uint(rootnHATab371DivA);
      uint aUI = as_uint(*a);

      TEST_AND_SET(YisZeroMask, rootnHATab370DivUI);

      TEST_AND_SET(XisZeroMask & YisEvenMask & SignYMask, rootnHATab371Div370UI);
      TEST_AND_SET(XisZeroMask & YisEvenMask & (~SignYMask), rootnHATab370UI);

      TEST_AND_SET(XisZeroMask & (~YisEvenMask) & (SignYMask), rootnHATab371DivAUI);
      TEST_AND_SET(XisZeroMask & (~YisEvenMask) & (~SignYMask), aUI);

      TEST_AND_SET(XisInfMask & (~YisEvenMask) & (SignYMask), rootnHATab371DivAUI);
      TEST_AND_SET(XisInfMask & (~YisEvenMask) & (~SignYMask), aUI);

      TEST_AND_SET(SignXMask & YisEvenMask, rootnHATab370Div370UI);
    }

  if (iSignX & iYisOdd)
    {
      flAi = -flAi;
      fResSign = -1.0f;
    iSignX = 0;
    SignXMask = 0;
  }

  iYMantissa = (as_uint(flBi) & 0x007FFFFF);

  iYIsInt = _TestInt (flBi);
  uint YIsIntMask = TO_MASK(iYIsInt);

  flVTmp1 = flAi + flBi;
  iSign   = (as_uint(flVTmp1) >> 31);
  flVTmp2 = rootnHATab371;
  flVTmp2 = as_float((as_uint(flVTmp2) & 0x7FFFFFFF) | ((uint) (iSign) << 31));
  flVTmp2 = flVTmp2 * flVTmp2;
  uint abSumUI = as_uint(flVTmp2*fResSign);

  iXIsFinite = (((as_uint(flAi) >> 23) & 0xFF) != 0xFF);
  uint XIsFiniteMask = TO_MASK(iXIsFinite);
  uint YIsFiniteMask = TO_MASK(iYIsFinite);

  uint tempMask =  (~SignXMask & TO_MASK(iEXB == 0x7F) & IsSigZeroXMask) | (TO_MASK(iEYB == 0) & IsSigZeroYMask);
  TEST_AND_SET(tempMask, abSumUI);

  uint isRootnHATab370Mask = TO_MASK(flAi == rootnHATab370);
  uint isRootnHATab372Mask = TO_MASK(flAi == ((__constant float *) _vmlsRootnHATab)[372]);
  if(resMask & ((~(XIsFiniteMask | IsSigZeroXMask)) | isRootnHATab370Mask | isRootnHATab372Mask))
  {
      tempMask = ~((XIsFiniteMask | IsSigZeroXMask) & (YIsFiniteMask | IsSigZeroYMask));
      TEST_AND_SET(tempMask, as_uint((*a + *b)*fResSign));

      flTmp1 = flAi * flAi*fResSign;
      TEST_AND_SET(isRootnHATab370Mask & ~SignYMask, as_uint(((__constant float *) _vmlsRootnHATab)[371 +(iYIsInt & iSignX)] * flTmp1));
      tempMask = isRootnHATab370Mask & SignYMask;
      nRet = (tempMask & resMask) & 1;
      TEST_AND_SET(tempMask, as_uint(((__constant float *) _vmlsRootnHATab)[371 + (iYIsInt & iSignX)] / flTmp1));

      tempMask = TO_MASK(isRootnHATab372Mask) & (YIsIntMask | ~YIsFiniteMask);
      TEST_AND_SET(tempMask, as_uint(((__constant float *) _vmlsRootnHATab)[371 + (iYIsInt & 1)]));
  }

  #undef TO_MASK
  #undef TEST_AND_SET

  *r = as_float(rUI);

  if (resMask)
    {
    if (iXIsFinite & iYIsFinite)
      {
      if ((flAi > rootnHATab370) | iYIsInt)
            {
              flSignRes =
              ((__constant float *) _vmlsRootnHATab)[371 +
              (iSignX &
              iYIsInt)];

              iDenoExpAdd = 0;
              flAX = flAi;
              flAX = as_float(
              (as_uint(flAX) & 0x7FFFFFFF) | ((uint) (0) <<
                        31));

              if (iEXB == 0)
              {
                flAX =
                  flAX * ((__constant float *) _vmlsRootnHATab)[376];
                  iDenoExpAdd = iDenoExpAdd - 64;
              }

              flX1 = flAX;
              flX1 = as_float(
               (as_uint(flX1) & 0x807FFFFF) | (((uint) (0x7F) &
            0xFF) << 23));

              iXHi =
                ((as_uint(flAX) >> 23) & 0xFF);
              iXHi = iXHi << 23;
              iXHi =
                iXHi | (as_uint(flAX) & 0x007FFFFF);

              k = iXHi - 0x3F380000;
              k = k >> 23;
              k = k + iDenoExpAdd;

              i1 =
                (as_uint(flX1) & 0x007FFFFF);
              i1 = i1 & 0x780000;
              i1 = i1 + 0x80000;
              i1 = i1 >> 20;

              flRcp1 = ((__constant float *) _vmlsRootnHATab)[0 + i1];

              flL1Hi =
                ((__constant float *) _vmlsRootnHATab)[9 + 2 * (i1) + 0];
              flL1Lo =
                ((__constant float *) _vmlsRootnHATab)[9 + 2 * (i1) + 1];

              flX2 = flX1 * flRcp1;

              i2 =
                (as_uint(flX2) & 0x007FFFFF);
              i2 = i2 & 0x1E0000;
              i2 = i2 + 0x20000;
              i2 = i2 >> 18;

              flRcp2 = ((__constant float *) _vmlsRootnHATab)[27 + i2];

              flL2Hi =
                ((__constant float *) _vmlsRootnHATab)[36 + 2 * (i2) +
                        0];
              flL2Lo =
                ((__constant float *) _vmlsRootnHATab)[36 + 2 * (i2) +
                        1];

              flX3 = (flX2 * flRcp2);

              i3 =
                (as_uint(flX3) & 0x007FFFFF);
              i3 = i3 & 0x7C000;
              i3 = i3 + 0x4000;
              i3 = i3 >> 15;

              flRcp3C = ((__constant float *) _vmlsRootnHATab)[54 + i3];

              flL3Hi =
                ((__constant float *) _vmlsRootnHATab)[71 + 2 * (i3) +
                        0];
              flL3Lo =
                ((__constant float *) _vmlsRootnHATab)[71 + 2 * (i3) +
                        1];

              flK = (float) k;
              flVTmp1 = ((flK) + (flL1Hi));
              flTmp1 = ((flK) - flVTmp1);
              flVTmp2 = (flTmp1 + (flL1Hi));
              flT = flVTmp1;
              flT_lo_1 = flVTmp2;

              flVTmp1 = ((flT) + (flL2Hi));
              flTmp1 = ((flT) - flVTmp1);
              flVTmp2 = (flTmp1 + (flL2Hi));
              flT = flVTmp1;
              flT_lo_2 = flVTmp2;

              flVTmp1 = ((flT) + (flL3Hi));
              flTmp1 = ((flT) - flVTmp1);
              flVTmp2 = (flTmp1 + (flL3Hi));
              flT = flVTmp1;
              flT_lo_3 = flVTmp2;

              flD = (flT_lo_1 + flT_lo_2);
              flD = (flD + flT_lo_3);
              flD = (flD + flL1Lo);
              flD = (flD + flL2Lo);
              flD = (flD + flL3Lo);

              flR1 = (flX3 * flRcp3C);
              flCQ =
                (flR1 - ((__constant float *) _vmlsRootnHATab)[374]);

              flRcpC = (flRcp1 * flRcp2);
              flRcpC = (flRcpC * flRcp3C);

              flVTmp1 =
                ((flX1) * (((__constant float *) _vmlsRootnHATab)[375]));
              flVTmp2 = (flVTmp1 - (flX1));
              flVTmp1 = (flVTmp1 - flVTmp2);
              flVTmp2 = ((flX1) - flVTmp1);
              flX1Hi = flVTmp1;
              flX1Lo = flVTmp2;

              flVTmp1 =
                ((flRcpC) *
                 (((__constant float *) _vmlsRootnHATab)[375]));
              flVTmp2 = (flVTmp1 - (flRcpC));
              flVTmp1 = (flVTmp1 - flVTmp2);
              flVTmp2 = ((flRcpC) - flVTmp1);
              flRcpCHi = flVTmp1;
              flRcpCLo = flVTmp2;

              flTmp1 = (flX1Hi * flRcpCHi);
              flE = (flTmp1 - flR1);
              flTmp1 = (flX1Lo * flRcpCHi);
              flE = (flE + flTmp1);
              flTmp1 = (flX1Hi * flRcpCLo);
              flE = (flE + flTmp1);
              flTmp1 = (flX1Lo * flRcpCLo);
              flE = (flE + flTmp1);

              flVTmp1 = ((flT) + (flCQ));
              flTmp1 = ((flT) - flVTmp1);
              flVTmp2 = (flTmp1 + (flCQ));
              flT_CQHi = flVTmp1;
              flCQLo = flVTmp2;

              iELogAX =
                ((as_uint(flT_CQHi) >> 23) & 0xFF);

              if (iELogAX + iEYB < 11 + 2 * 0x7F)
              {
                if (iELogAX + iEYB > -62 + 2 * 0x7F)
                {
                  flR = (flCQ + flE);

                  flLog2Poly =
                    ((((((__constant float *) _vmlsRootnHATab)[364])
                       * flR +
                       ((__constant float *) _vmlsRootnHATab)[363]) *
                      flR +
                      ((__constant float *) _vmlsRootnHATab)[362]) *
                     flR +
                     ((__constant float *) _vmlsRootnHATab)[361]) *
                    flR;

                  flLogPart3 = (flCQLo + flE);
                  flLogPart3 = (flD + flLogPart3);

                  flVTmp1 = ((flT_CQHi) + (flLog2Poly));
                  flTmp1 = ((flT_CQHi) - flVTmp1);
                  flVTmp2 = (flTmp1 + (flLog2Poly));
                  flHH = flVTmp1;
                  flHL = flVTmp2;

                  flVTmp1 = ((flHH) + (flLogPart3));
                  flTmp1 = ((flHH) - flVTmp1);
                  flVTmp2 = (flTmp1 + (flLogPart3));
                  flHH = flVTmp1;
                  flHLL = flVTmp2;

                  flHLL = (flHLL + flHL);

                  flVTmp1 =
                    ((flHH) *
                     (((__constant float *) _vmlsRootnHATab)[375]));
                  flVTmp2 = (flVTmp1 - (flHH));
                  flVTmp1 = (flVTmp1 - flVTmp2);
                  flVTmp2 = ((flHH) - flVTmp1);
                  flHH = flVTmp1;
                  flHL = flVTmp2;

                  flVTmp1 =
                    (((__constant float *) _vmlsRootnHATab)[371] /
                     flBOrig);
                  flVTmp2 =
                    (flVTmp1 *
                     ((__constant float *) _vmlsRootnHATab)[375]);
                  flVTmp3 = (flVTmp2 - flVTmp1);
                  flVTmp3 = (flVTmp2 - flVTmp3);
                  flVTmp1 = (flBOrig * flVTmp3);
                  flVTmp1 =
                    (((__constant float *) _vmlsRootnHATab)[371] -
                     flVTmp1);
                  flVTmp2 =
                    (((__constant float *) _vmlsRootnHATab)[370] *
                     flVTmp3);
                  flVTmp2 = (flVTmp1 - flVTmp2);
                  flVTmp1 =
                    (((__constant float *) _vmlsRootnHATab)[371] +
                     flVTmp2);
                  flYHi = flVTmp3;
                  flVTmp1 = (flVTmp1 * flVTmp2);
                  flYLo = (flVTmp1 * flVTmp3);

                  flTmp1 = ((flYHi) * (flHH));
                  flTmp2 = ((flYLo) * (flHL));
                  flTmp2 = (flTmp2 + (flYHi) * (flHL));
                  flTmp3 = (flTmp2 + (flYLo) * (flHH));
                  flPH = flTmp1;
                  flPL = flTmp3;

                  flPLL = (flBi * flHLL);

                  flVTmp1 =
                    (flPH +
                     ((__constant float *) _vmlsRootnHATab)[373]);
                  flVPHH =
                    (flVTmp1 -
                     ((__constant float *) _vmlsRootnHATab)[373]);
                  iN =
                    (as_uint(flVTmp1) & 0x007FFFFF);
                  j = iN & 0x7F;

                  iN = iN << 10;
                  iN = iN >> (7 + 10);
                  flVPHL = (flPH - flVPHH);

                  flZ = (flPLL + flPL);
                  flZ = (flZ + flVPHL);

                  flExp2Poly =
                    (((((__constant float *) _vmlsRootnHATab)[367]) *
                      flZ +
                      ((__constant float *) _vmlsRootnHATab)[366]) *
                     flZ +
                     ((__constant float *) _vmlsRootnHATab)[365]) *
                    flZ;

                  flExp2PolyT =
                    (flExp2Poly *
                     ((__constant float *) _vmlsRootnHATab)[105 +
                             2 * (j) +
                             0]);
                  flResLo =
                    (flExp2PolyT +
                     ((__constant float *) _vmlsRootnHATab)[105 +
                             2 * (j) +
                             1]);
                  flResHi =
                    ((__constant float *) _vmlsRootnHATab)[105 +
                            2 * (j) +
                            0];

                  flRes = (flResHi + flResLo);
                  iERes =
                    ((as_uint(flRes) >> 23) & 0xFF);
                  iERes = (iERes - 0x7F);
                  iERes = (iERes + iN);

                  if (iERes < 128)
                  {
                    if (iERes >= -126)
                    {
                      flRes = as_float(
                       (as_uint(flRes) & 0x807FFFFF) |
                       (((uint) (iERes + 0x7F) &
                         0xFF) << 23));

                      flRes = (flRes * flSignRes);
                      *r = flRes;
                    }
                    else
                    {
                      if (iERes >= -126 - 10)
                      {
                        flVTmp1 =
                    ((flResHi) + (flResLo));
                        flTmp1 = ((flResHi) - flVTmp1);
                        flVTmp2 = (flTmp1 + (flResLo));
                        flResHi = flVTmp1;
                        flResLo = flVTmp2;

                        flVTmp1 =
                    ((flResHi) *
                     (((__constant float *)
                       _vmlsRootnHATab)[375]));
                        flVTmp2 = (flVTmp1 - (flResHi));
                        flVTmp1 = (flVTmp1 - flVTmp2);
                        flVTmp2 = ((flResHi) - flVTmp1);
                        flResHi = flVTmp1;
                        flTmp2 = flVTmp2;

                        flResLo = (flResLo + flTmp2);

                        flSignRes *=
                    ((__constant float *)
                     _vmlsRootnHATab)[377];
                        iN = (iN + 64);

                        flTwoPowN =
                    ((__constant float *)
                     _vmlsRootnHATab)[371];
                    flTwoPowN = as_float(
                         (as_uint(flTwoPowN) & 0x807FFFFF) |
                         (((uint) (iN + 0x7F) &
                     0xFF) << 23));

                        flResHi = (flResHi * flTwoPowN);
                        flResHi = (flResHi * flSignRes);

                        flResLo = (flResLo * flTwoPowN);
                        flVTmp1 = (flResLo * flSignRes);

                        flRes = (flResHi + flVTmp1);

                        flVTmp1 =
                    ((__constant float *)
                     _vmlsRootnHATab)[369];
                        flVTmp1 = (flVTmp1 * flVTmp1);
                        flRes = (flRes + flVTmp1);

                        *r = flRes;
                      }
                      else
                      {
                        if (iERes >= -149 - 10)
                        {
                          flSignRes *=
                            ((__constant float *)
                             _vmlsRootnHATab)[377];
                          iN = iN + 64;

                          flTwoPowN =
                            ((__constant float *)
                             _vmlsRootnHATab)[371];
                            flTwoPowN = as_float(
                           (as_uint(flTwoPowN) & 0x807FFFFF) |
                           (((uint)
                             (iN +
                              0x7F) & 0xFF) << 23));

                          flRes = (flRes * flTwoPowN);
                          flRes = (flRes * flSignRes);

                          flVTmp1 =
                            ((__constant float *)
                             _vmlsRootnHATab)[369];
                          flVTmp1 *= flVTmp1;
                          flRes = (flRes + flVTmp1);

                          *r = flRes;
                        }
                        else
                        {
                          flVTmp1 =
                            ((__constant float *)
                             _vmlsRootnHATab)[369];
                          flVTmp1 *= flVTmp1;
                          flRes =
                            (flVTmp1 * flSignRes);
                          *r = flRes;
                        }
                      }
                    }
                  }
                  else
                  {
                    flVTmp1 =
                ((__constant float *) _vmlsRootnHATab)[368];
                    flVTmp1 = (flVTmp1 * flVTmp1);
                    flRes = (flVTmp1 * flSignRes);
                    *r = flRes;
                  }
                }
                else
                {
                  flVTmp1 =
                    ((__constant float *) _vmlsRootnHATab)[371];
                  flVTmp1 =
                    (flVTmp1 +
                     ((__constant float *) _vmlsRootnHATab)[369]);
                  *r = (flVTmp1 * flSignRes);
                }
              }
              else
              {
                iSign =
            iSignY ^ (as_uint(flT_CQHi) >> 31);

                flTmp1 =
            ((__constant float *) _vmlsRootnHATab)[368 +
                    (iSign)];

                flTmp1 = (flTmp1 * flTmp1);

                flTmp1 = (flTmp1 * flSignRes);
                *r = flTmp1;
              }
            }
            else
            {
              flVTmp1 = ((__constant float *) _vmlsRootnHATab)[370];
              flVTmp1 = (flVTmp1 / flVTmp1);
              *r = flVTmp1;
              nRet = 1;
            }
          }
          else
          {
            if (iEXB < 0x7F)
            {
              if (iSignY)
              {
                *r = (flBi * flBi);
              }
              else
              {
                *r = ((__constant float *) _vmlsRootnHATab)[370];
              }
            }
            else
            {
              if (iSignY)
              {
                flRes =
            ((__constant float *) _vmlsRootnHATab)[378 +
                    (iYIsInt &
                     iSignX)];
                *r = flRes;
              }
              else
              {
                flTmp1 = (flAi * flAi);
                flTmp1 = (flTmp1 * flBi);
                *r =
            flTmp1 * ((__constant float *) _vmlsRootnHATab)[371 +
                       (iYIsInt
                        &
                        iSignX)];
              }
            }
          }
    (*r) = (*r) * fResSign;
        }

  return nRet;
}


float __ocl_svml_rootn(x,n)
{
    float va1;
    uint va2;
    float vr1;
    uint vm;
    va1 = x;
    va2 = n;
    {
        float sHiMask;
        float sRSValue;
        float sZ[2];
        float sL[2];
        float sW[2];
        uint _NMINNORM;
        uint _NMAXVAL;
        uint _INF;
        uint iSpecX;
        uint iSpecY;
        uint LFR_iY;
        uint iRangeMask;
        uint LFR_iX;
        float LFR_sXMant;
        float LFR_sM;
        uint LFR_iInd;
        float LFR_sLnRcprYHi;
        float LFR_sLnRcprYLo;
        float LFR_sRcprY;
        float LFR_sYHi;
        float LFR_sYLo;
        float LFR_sYHiRcpY;
        float LFR_sRHi;
        float LFR_sTRHi;
        float LFR_sRLo;
        float LFR_sR;
        float LFR_sP;
        float LFR_sR2;
        uint LFR_iN;
        float LFR_sN;
        uint LFR_iXNearOne;
        float LFR_sXNearOne;
        float LFR_sNLog2Hi;
        float LFR_sNLog2Lo;
        float LFR_sWLo;
        float LFR_alfa;
        float LFR_sResHi;
        float LFR_beta;
        float LFR_sResLo;
        float S_MANT_MASK;
        float S_ONE;
        uint LFR_I_INDEX_MASK;
        uint LFR_I_INDEX_ADD;
        float S_HI10BITS_MASK;
        float LFR_S_P4;
        float LFR_S_P3;
        float LFR_S_P2;
        uint I_BIAS;
        uint LFR_I_NZ_ADD;
        uint LFR_I_NZ_CMP;
        float S_LOG2_HI;
        float S_LOG2_LO;
        float sN;
        float sR;
        float sP;
        float sM;
        uint iAbsZ;
        uint iRes;
        uint iP;
        uint iM;
        float sInvLn2;
        float sShifter;
        float sLn2hi;
        float sLn2lo;
        uint iAbsMask;
        uint iDomainRange;
        float sPC[6];
        float sX;
        float sY;
        uint iOddY;
        float sOddY;
        float sResultSign;
        float sAbsMask;
        uint iZero;
        float sZero;
        uint iNegX;
        uint iEvenY;
        uint iNegXEvenY;
        iZero = 0;
        sZero = as_float(iZero);
        sX = va1;
        sY = ((float)((int)(va2)));
        iOddY = ((uint)va2 << 31);
        sOddY = as_float(iOddY);
        sResultSign = as_float((as_uint(va1) & as_uint(sOddY)));
        iNegX = as_uint(sX);
        iNegX = ((uint)(-(int)((int)iNegX < (int)iZero)));
        iEvenY = ((uint)(-(int)((int)iOddY == (int)iZero)));
        iNegXEvenY = (iNegX & iEvenY);
        iAbsMask = as_uint(__ocl_svml_srootn_data._iAbsMask);
        sAbsMask = as_float(iAbsMask);
        sX = as_float((as_uint(sX) & as_uint(sAbsMask)));
        sW[0] = sY;
        sW[1] = sZero;
        {
            float T_t1;
            float T_v2;
            float T_v3;
            float T_ONE;
            float T_C;
            T_ONE = as_float(0x3f800000u);
            T_C = as_float(0x45800800u);
            T_t1 = (T_ONE / sW[0]);
            T_v2 = (T_t1 * T_C);
            T_v3 = (T_v2 - T_t1);
            T_v3 = (T_v2 - T_v3);
            T_t1 = (sW[0] * T_v3);
            T_t1 = (T_ONE - T_t1);
            T_v2 = (sW[1] * T_v3);
            T_v2 = (T_t1 - T_v2);
            T_t1 = (T_ONE + T_v2);
            sW[0] = T_v3;
            T_t1 = (T_t1 * T_v2);
            sW[1] = (T_t1 * T_v3);
        }
        LFR_iX = as_uint(sX);
        LFR_iY = as_uint(sY);
        _NMINNORM = as_uint(__ocl_svml_srootn_data.NMINNORM);
        _NMAXVAL = as_uint(__ocl_svml_srootn_data.NMAXVAL);
        _INF = as_uint(__ocl_svml_srootn_data.INF);
        iSpecX = (LFR_iX - _NMINNORM);
        iSpecX =
            ((uint)(-(int)((int)iSpecX >= (int)_NMAXVAL)));
        iSpecY = (LFR_iY & iAbsMask);
        iSpecY = ((uint)(-(int)((int)iSpecY >= (int)_INF)));
        iRangeMask = (iSpecX | iSpecY);
        iRangeMask = (iRangeMask | iNegXEvenY);
        LFR_I_NZ_ADD = as_uint(__ocl_svml_srootn_data.LFR_I_NZ_ADD);
        LFR_iXNearOne = (LFR_iX + LFR_I_NZ_ADD);
        LFR_I_NZ_CMP = as_uint(__ocl_svml_srootn_data.LFR_I_NZ_CMP);
        LFR_iXNearOne =
            ((uint)
            (-(int)((int)LFR_iXNearOne > (int)LFR_I_NZ_CMP)));
        LFR_sXNearOne = as_float(LFR_iXNearOne);
        S_MANT_MASK = as_float(__ocl_svml_srootn_data.S_MANT_MASK);
        LFR_sXMant =
            as_float((as_uint(sX) & as_uint(S_MANT_MASK)));
        S_ONE = as_float(__ocl_svml_srootn_data.S_ONE);
        LFR_sM =
            as_float((as_uint(LFR_sXMant) | as_uint(S_ONE)));
        LFR_iN = ((uint)LFR_iX >> 23);
        I_BIAS = as_uint(__ocl_svml_srootn_data.I_BIAS);
        LFR_iN = (LFR_iN - I_BIAS);
        LFR_sN = ((float)((int)(LFR_iN)));
        LFR_I_INDEX_MASK =
            as_uint(__ocl_svml_srootn_data.LFR_I_INDEX_MASK);
        LFR_iInd = (LFR_iX & LFR_I_INDEX_MASK);
        LFR_I_INDEX_ADD =
            as_uint(__ocl_svml_srootn_data.LFR_I_INDEX_ADD);
        LFR_iInd = (LFR_iInd + LFR_I_INDEX_ADD);
        LFR_iInd = ((uint)LFR_iInd >> 17);
        LFR_sLnRcprYHi =
            *((__constant float *) (((__constant char *) (__ocl_svml_srootn_data.LFR_TBL)) +
            ((0 + LFR_iInd) * (3 * 4))) + 0);
        LFR_sLnRcprYLo =
            *((__constant float *) (((__constant char *) (__ocl_svml_srootn_data.LFR_TBL)) +
            ((0 + LFR_iInd) * (3 * 4))) + 1);
        LFR_sRcprY =
            *((__constant float *) (((__constant char *) (__ocl_svml_srootn_data.LFR_TBL)) +
            ((0 + LFR_iInd) * (3 * 4))) + 2);
        S_HI10BITS_MASK =
            as_float(__ocl_svml_srootn_data.S_HI10BITS_MASK);
        LFR_sYHi =
            as_float((as_uint(LFR_sM) & as_uint(S_HI10BITS_MASK)));
        LFR_sYLo = (LFR_sM - LFR_sYHi);
        LFR_sYHiRcpY = (LFR_sYHi * LFR_sRcprY);
        LFR_sRHi = (LFR_sYHiRcpY - S_ONE);
        LFR_sTRHi = (LFR_sRHi + LFR_sLnRcprYHi);
        LFR_sRLo = (LFR_sYLo * LFR_sRcprY);
        LFR_sR = (LFR_sRHi + LFR_sRLo);
        LFR_S_P4 = as_float(__ocl_svml_srootn_data.LFR_S_P4);
        LFR_S_P3 = as_float(__ocl_svml_srootn_data.LFR_S_P3);
        LFR_sP = ((LFR_S_P4 * LFR_sR) + LFR_S_P3);
        LFR_S_P2 = as_float(__ocl_svml_srootn_data.LFR_S_P2);
        LFR_sP = ((LFR_sP * LFR_sR) + LFR_S_P2);
        LFR_sR2 = (LFR_sR * LFR_sR);
        LFR_sP = (LFR_sP * LFR_sR2);
        S_LOG2_HI = as_float(__ocl_svml_srootn_data.S_LOG2_HI);
        LFR_sNLog2Hi = (LFR_sN * S_LOG2_HI);
        S_LOG2_LO = as_float(__ocl_svml_srootn_data.S_LOG2_LO);
        LFR_sNLog2Lo = (LFR_sN * S_LOG2_LO);
        LFR_sResHi = (LFR_sNLog2Hi + LFR_sTRHi);
        LFR_sWLo = (LFR_sNLog2Lo + LFR_sLnRcprYLo);
        LFR_sResLo = (LFR_sP + LFR_sWLo);
        LFR_alfa =
            as_float((as_uint(LFR_sXNearOne) & as_uint(LFR_sRLo)));
        sL[0] = (LFR_sResHi + LFR_alfa);
        LFR_beta =
            as_float((~(as_uint(LFR_sXNearOne)) &
            as_uint(LFR_sRLo)));
        sL[1] = (LFR_sResLo + LFR_beta);
        sRSValue = as_float(__ocl_svml_srootn_data.sRSValue);
        sHiMask = as_float(__ocl_svml_srootn_data.sHiMask);
        {
            float V1;
            float V2;;
            V1 = (sL[0] + sL[1]);
            V2 = (V1 * sRSValue);
            V1 = (V1 + V2);
            V2 = (V1 - V2);
            V1 = (sL[0] - V2);
            V1 = (sL[1] + V1);;
            sL[0] = V2;
            sL[1] = V1;
        }
        {
            float V1;
            float V2;;
            V1 = (sL[0] * sW[0]);
            V2 = (sL[1] * sW[1]);
            V2 = ((sL[0] * sW[1]) + V2);
            V2 = ((sL[1] * sW[0]) + V2);;
            sZ[0] = V1;
            sZ[1] = V2;
        }
        sInvLn2 = as_float(__ocl_svml_srootn_data._sInvLn2);
        sShifter = as_float(__ocl_svml_srootn_data._sShifter);
        sM = ((sZ[0] * sInvLn2) + sShifter);
        sN = (sM - sShifter);
        iAbsZ = as_uint(sZ[0]);
        iAbsZ = (iAbsZ & iAbsMask);
        iDomainRange = as_uint(__ocl_svml_srootn_data._iDomainRange);
        iAbsZ =
            ((uint)(-(int)((int)iAbsZ > (int)iDomainRange)));
        iRangeMask = (iRangeMask | iAbsZ);
        vm = 0;
        vm |= (((uint)iRangeMask >> 31) & 1);
        iM = as_uint(sM);
        iM = ((uint)iM << 23);
        sLn2hi = as_float(__ocl_svml_srootn_data._sLn2hi);
        sR = (sZ[0] - (sN * sLn2hi));
        sLn2lo = as_float(__ocl_svml_srootn_data._sLn2lo);
        sR = (sR - (sN * sLn2lo));
        sR = (sR + sZ[1]);
        sPC[4] = as_float(__ocl_svml_srootn_data._sPC4);
        sPC[5] = as_float(__ocl_svml_srootn_data._sPC5);
        sP = ((sPC[5] * sR) + sPC[4]);
        sPC[3] = as_float(__ocl_svml_srootn_data._sPC3);
        sP = ((sP * sR) + sPC[3]);
        sPC[2] = as_float(__ocl_svml_srootn_data._sPC2);
        sP = ((sP * sR) + sPC[2]);
        sPC[1] = as_float(__ocl_svml_srootn_data._sPC1);
        sP = ((sP * sR) + sPC[1]);
        sPC[0] = as_float(__ocl_svml_srootn_data._sPC0);
        sP = ((sP * sR) + sPC[0]);
        iP = as_uint(sP);
        iRes = (iM + iP);
        vr1 = as_float(iRes);
        vr1 = as_float((as_uint(vr1) | as_uint(sResultSign)));
    }
    if (vm & 1)
    {
        float _vapi_arg1[1];
        int _vapi_arg2[1];
        float _vapi_res1[1];
        ((private float *) _vapi_arg1)[0] = va1;
        ((private uint *) _vapi_arg2)[0] = va2;
        ((private float *) _vapi_res1)[0] = vr1;
        __ocl_svml_srootn_cout_rare(_vapi_arg1, _vapi_arg2, _vapi_res1);
        vr1 = ((private const float *) _vapi_res1)[0];
    }
    return vr1;
}
    
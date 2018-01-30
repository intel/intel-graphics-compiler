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

#ifndef __IA32E_ROOTN_D_G_COUT_CL__
#define __IA32E_ROOTN_D_G_COUT_CL__

#include "ia32e_pow_d_g_cout.cl"

__attribute__((always_inline))
int
__ocl_svml_drootn_cout_rare (__private const double *a, __private int *b, __private double *r)
{
  int nRet = 0;
  double dbVTmp1, dbVTmp2, dbVTmp3, dbVPHH, dbVPHL;
  double dX, dY, dR, dbAX, dbSignRes, dbX1, dbRcp1, dbL1Hi, dbL1Lo, dbX2,
    dbRcp2, dbL2Hi, dbL2Lo, dbX3, dbRcp3C, dbL3Hi, dbL3Lo, dbK, dbT, dbD,
    dbR1, dbCQ, dbRcpC, dbX1Hi, dbX1Lo, dbRcpCHi, dbRcpCLo, dbTmp1, dbE,
    dbT_CQHi, dbCQLo, dbR, dbLogPart3, dbLog2Poly, dbHH, dbHL, dbHLL, dbYHi,
    dbYLo, dbTmp2, dbTmp3, dbPH, dbPL, dbPLL, dbZ, dbExp2Poly, dbExp2PolyT,
    dbResLo, dbResHi, dbRes, dbTwoPowN;
  int iEXB, iEYB, iSignX, iSignY, iYHi, iYLo, iYIsFinite, iEY, iYIsInt,
    iXIsFinite, iDenoExpAdd, iXHi, k, i1, i2, i3, iELogAX, iN, j, iERes,
    iSign, iIsSigZeroX, iIsSigZeroY;
  double dResSign = 1.0;
  double dB = (double) (*b);

  dX = *a;
  dY = 1.0 / dB;

  iEXB = ((((__private _iml_dp_union_t *) & dX)->dwords.hi_dword >> 20) & 0x7FF);

  iEYB = ((((__private _iml_dp_union_t *) & dY)->dwords.hi_dword >> 20) & 0x7FF);

  iSignX = (((__private _iml_dp_union_t *) & dX)->dwords.hi_dword >> 31);

  iSignY = (((__private _iml_dp_union_t *) & dY)->dwords.hi_dword >> 31);

  iIsSigZeroX =
    (((((__private _iml_dp_union_t *) & dX)->dwords.hi_dword & 0x000FFFFF) == 0)
     && ((((__private _iml_dp_union_t *) & dX)->dwords.lo_dword) == 0));
  iIsSigZeroY =
    (((((__private _iml_dp_union_t *) & dY)->dwords.hi_dword & 0x000FFFFF) == 0)
     && ((((__private _iml_dp_union_t *) & dY)->dwords.lo_dword) == 0));

  iYHi =
    (iEYB << 20) | (((__private _iml_dp_union_t *) & dY)->dwords.hi_dword & 0x000FFFFF);

  iYLo = (((__private _iml_dp_union_t *) & dY)->dwords.lo_dword);

  iYIsFinite =
    (((((__private _iml_dp_union_t *) & dY)->dwords.hi_dword >> 20) & 0x7FF) != 0x7FF);

  {
    int iXisZero = ((iEXB == 0) && (iIsSigZeroX));
    int iYisZero = (*b == 0);
    int iXisINF =
      (!((((((__private _iml_dp_union_t *) & dX)->dwords.hi_dword >> 20) & 0x7FF) !=
          0x7FF))) && ((iIsSigZeroX));
    int iYisEven = (((*b) & 1) == 0);
    int iYisOdd = (((*b) & 1) == 1);

    if (iYisZero)
      {
        dbVTmp1 = ((__constant double *) _vmldPowHATab)[852];
        dbVTmp1 = dbVTmp1 / dbVTmp1;
        dR = dbVTmp1;
        *r = (double) (dR);
        return nRet;
      }

    if (iXisZero && iYisEven)
      {
        if (iSignY)
          {
            dbVTmp1 = ((__constant double *) _vmldPowHATab)[853];
            dbVTmp2 = ((__constant double *) _vmldPowHATab)[852];
            *r = (double) (dbVTmp1 / dbVTmp2);
            return nRet;
          }
        else
          {
            dbVTmp1 = ((__constant double *) _vmldPowHATab)[852];
            *r = (double) (dbVTmp1);
            return nRet;
          }
      }

    if (iXisZero && iYisOdd)
      {
        if (iSignY)
          {
            dbVTmp1 = ((__constant double *) _vmldPowHATab)[853];
            *r = (double) (dbVTmp1 / (*a));
            return nRet;
          }
        else
          {
            *r = *a;
            return nRet;
          }
      }

    if (iXisINF && iYisOdd)
      {
        if (iSignY)
          {
            dbVTmp1 = ((__constant double *) _vmldPowHATab)[853];
            *r = (double) (dbVTmp1 / (*a));
            return nRet;
          }
        else
          {

            *r = *a;
            return nRet;
          }
      }

    if (iSignX && iYisEven)
      {
        dbVTmp1 = ((__constant double *) _vmldPowHATab)[852];
        dbVTmp1 = dbVTmp1 / dbVTmp1;
        dR = dbVTmp1;
        *r = (double) (dR);
        return nRet;
      }

    if (iSignX && iYisOdd)
      {
        dX = -dX;
        dResSign = -1.0;
        iSignX = (((__private _iml_dp_union_t *) & dX)->dwords.hi_dword >> 31);
      }

  }

  if (iYHi | iYLo)
    {

      iEY = iEYB - 0x3FF;

      if ((0x3FF <= iEYB) && iYIsFinite)
        {

          if (iEY <= 20)
            {

              if (((iYHi << iEY) << 12) | iYLo)
                {

                  iYIsInt = 0;
                }
              else
                {

                  if ((iYHi << (iEY + 11)) & 0x80000000)
                    {
                      iYIsInt = 1;
                    }
                  else
                    {
                      iYIsInt = 2;
                    }
                }
            }
          else
            {
              if (iEY < 53)
                {

                  if ((iYLo << (iEY + 12 - 32 - 1)) << 1)
                    {

                      iYIsInt = 0;
                    }
                  else
                    {

                      if ((iYLo << (iEY + 12 - 32 - 1)) & 0x80000000)
                        {
                          iYIsInt = 1;
                        }
                      else
                        {
                          iYIsInt = 2;
                        }
                    }
                }
              else
                {

                  iYIsInt = 2;
                }
            }
        }
      else
        {

          iYIsInt = 0;
        }
    }
  else
    {

      iYIsInt = 2;

    }

  if (!((iSignX == 0) && (iEXB == 0x3FF) && iIsSigZeroX) &&
      !((iEYB == 0) && iIsSigZeroY))
    {

      iXIsFinite =
        (((((__private _iml_dp_union_t *) & dX)->dwords.hi_dword >> 20) & 0x7FF) !=
         0x7FF);

      if ((iXIsFinite || iIsSigZeroX) && (iYIsFinite || iIsSigZeroY))
        {

          if (dX != ((__constant double *) _vmldPowHATab)[852])
            {

              if (!
                  ((dX == ((__constant double *) _vmldPowHATab)[854])
                   && (iYIsInt || !iYIsFinite)))
                {

                  if (iXIsFinite && iYIsFinite)
                    {

                      if ((dX > ((__constant double *) _vmldPowHATab)[852])
                          || iYIsInt)
                        {

                          dbSignRes =
                            ((__constant double *) _vmldPowHATab)[853 +
                                                             (iSignX &
                                                              iYIsInt)];

                          iDenoExpAdd = 0;
                          dbAX = dX;
                          (((__private _iml_dp_union_t *) & dbAX)->dwords.hi_dword =
                           (((__private _iml_dp_union_t *) & dbAX)->dwords.
                            hi_dword & 0x7FFFFFFF) | ((_iml_uint32_t) (0) <<
                                                      31));

                          if (iEXB == 0)
                            {

                              dbAX =
                                dbAX * ((__constant double *) _vmldPowHATab)[858];
                              iDenoExpAdd = iDenoExpAdd - 200;
                            }

                          dbX1 = dbAX;
                          (((__private _iml_dp_union_t *) & dbX1)->dwords.hi_dword =
                           (((__private _iml_dp_union_t *) & dbX1)->dwords.
                            hi_dword & 0x800FFFFF) | (((_iml_uint32_t) (0x3FF)
                                                       & 0x7FF) << 20));

                          iXHi =
                            ((((__private _iml_dp_union_t *) & dbAX)->dwords.
                              hi_dword >> 20) & 0x7FF);
                          iXHi = iXHi << 20;
                          iXHi =
                            iXHi | (((__private _iml_dp_union_t *) & dbAX)->dwords.
                                    hi_dword & 0x000FFFFF);

                          k = iXHi - 0x3FE7C000;
                          k = k >> 20;
                          k = k + iDenoExpAdd;

                          i1 =
                            (((__private _iml_dp_union_t *) & dbX1)->dwords.
                             hi_dword & 0x000FFFFF);
                          i1 = i1 & 0xFC000;
                          i1 = i1 + 0x4000;
                          i1 = i1 >> 15;

                          dbRcp1 = ((__constant double *) _vmldPowHATab)[0 + i1];

                          dbL1Hi =
                            ((__constant double *) _vmldPowHATab)[33 + 2 * (i1) +
                                                             0];
                          dbL1Lo =
                            ((__constant double *) _vmldPowHATab)[33 + 2 * (i1) +
                                                             1];

                          dbX2 = dbX1 * dbRcp1;

                          i2 =
                            (((__private _iml_dp_union_t *) & dbX2)->dwords.
                             hi_dword & 0x000FFFFF);
                          i2 = i2 & 0xFC00;
                          i2 = i2 + 0x400;
                          i2 = i2 >> 11;

                          dbRcp2 = ((__constant double *) _vmldPowHATab)[99 + i2];

                          dbL2Hi =
                            ((__constant double *) _vmldPowHATab)[132 + 2 * (i2) +
                                                             0];
                          dbL2Lo =
                            ((__constant double *) _vmldPowHATab)[132 + 2 * (i2) +
                                                             1];

                          dbX3 = dbX2 * dbRcp2;

                          i3 =
                            (((__private _iml_dp_union_t *) & dbX3)->dwords.
                             hi_dword & 0x000FFFFF);
                          i3 = i3 & 0xFF0;
                          i3 = i3 + 0x10;
                          i3 = i3 >> 5;

                          dbRcp3C =
                            ((__constant double *) _vmldPowHATab)[198 + i3];

                          dbL3Hi =
                            ((__constant double *) _vmldPowHATab)[327 + 2 * (i3) +
                                                             0];
                          dbL3Lo =
                            ((__constant double *) _vmldPowHATab)[327 + 2 * (i3) +
                                                             1];

                          dbK = (double) k;
                          dbT = (dbK + dbL1Hi);
                          dbT = (dbT + dbL2Hi);
                          dbT = (dbT + dbL3Hi);

                          dbD = (dbL2Lo + dbL3Lo);
                          dbD = (dbD + dbL1Lo);

                          dbR1 = (dbX3 * dbRcp3C);
                          dbCQ =
                            (dbR1 - ((__constant double *) _vmldPowHATab)[856]);

                          dbRcpC = (dbRcp1 * dbRcp2);
                          dbRcpC = (dbRcpC * dbRcp3C);

                          dbVTmp1 =
                            ((dbX1) *
                             (((__constant double *) _vmldPowHATab)[857]));
                          dbVTmp2 = (dbVTmp1 - (dbX1));
                          dbVTmp1 = (dbVTmp1 - dbVTmp2);
                          dbVTmp2 = ((dbX1) - dbVTmp1);
                          dbX1Hi = dbVTmp1;
                          dbX1Lo = dbVTmp2;

                          dbVTmp1 =
                            ((dbRcpC) *
                             (((__constant double *) _vmldPowHATab)[857]));
                          dbVTmp2 = (dbVTmp1 - (dbRcpC));
                          dbVTmp1 = (dbVTmp1 - dbVTmp2);
                          dbVTmp2 = ((dbRcpC) - dbVTmp1);
                          dbRcpCHi = dbVTmp1;
                          dbRcpCLo = dbVTmp2;

                          dbTmp1 = (dbX1Hi * dbRcpCHi);
                          dbE = (dbTmp1 - dbR1);
                          dbTmp1 = (dbX1Lo * dbRcpCHi);
                          dbE = (dbE + dbTmp1);
                          dbTmp1 = (dbX1Hi * dbRcpCLo);
                          dbE = (dbE + dbTmp1);
                          dbTmp1 = (dbX1Lo * dbRcpCLo);
                          dbE = (dbE + dbTmp1);

                          dbVTmp1 = ((dbT) + (dbCQ));
                          dbTmp1 = ((dbT) - dbVTmp1);
                          dbVTmp2 = (dbTmp1 + (dbCQ));
                          dbT_CQHi = dbVTmp1;
                          dbCQLo = dbVTmp2;

                          iELogAX =
                            ((((__private _iml_dp_union_t *) & dbT_CQHi)->dwords.
                              hi_dword >> 20) & 0x7FF);

                          if (iELogAX + iEYB < 11 + 2 * 0x3FF)
                            {

                              if (iELogAX + iEYB > -62 + 2 * 0x3FF)
                                {

                                  dbR = (dbCQ + dbE);

                                  dbLog2Poly =
                                    ((((((__constant double *) _vmldPowHATab)[844])
                                       * dbR +
                                       ((__constant double *) _vmldPowHATab)[843])
                                      * dbR +
                                      ((__constant double *) _vmldPowHATab)[842]) *
                                     dbR +
                                     ((__constant double *) _vmldPowHATab)[841]) *
                                    dbR;

                                  dbLogPart3 = (dbCQLo + dbE);
                                  dbLogPart3 = (dbD + dbLogPart3);

                                  dbVTmp1 = ((dbT_CQHi) + (dbLog2Poly));
                                  dbTmp1 = ((dbT_CQHi) - dbVTmp1);
                                  dbVTmp2 = (dbTmp1 + (dbLog2Poly));
                                  dbHH = dbVTmp1;
                                  dbHL = dbVTmp2;

                                  dbVTmp1 = ((dbHH) + (dbLogPart3));
                                  dbTmp1 = ((dbHH) - dbVTmp1);
                                  dbVTmp2 = (dbTmp1 + (dbLogPart3));
                                  dbHH = dbVTmp1;
                                  dbHLL = dbVTmp2;

                                  dbHLL = (dbHLL + dbHL);

                                  dbVTmp1 =
                                    ((dbHH) *
                                     (((__constant double *) _vmldPowHATab)[857]));
                                  dbVTmp2 = (dbVTmp1 - (dbHH));
                                  dbVTmp1 = (dbVTmp1 - dbVTmp2);
                                  dbVTmp2 = ((dbHH) - dbVTmp1);
                                  dbHH = dbVTmp1;
                                  dbHL = dbVTmp2;

                                  dbVTmp1 =
                                    (((__constant double *) _vmldPowHATab)[853] /
                                     dB);
                                  dbVTmp2 =
                                    (dbVTmp1 *
                                     ((__constant double *) _vmldPowHATab)[857]);
                                  dbVTmp3 = (dbVTmp2 - dbVTmp1);
                                  dbVTmp3 = (dbVTmp2 - dbVTmp3);
                                  dbVTmp1 = (dB * dbVTmp3);
                                  dbVTmp1 =
                                    (((__constant double *) _vmldPowHATab)[853] -
                                     dbVTmp1);
                                  dbVTmp2 =
                                    (((__constant double *) _vmldPowHATab)[852] *
                                     dbVTmp3);
                                  dbVTmp2 = (dbVTmp1 - dbVTmp2);
                                  dbVTmp1 =
                                    (((__constant double *) _vmldPowHATab)[853] +
                                     dbVTmp2);
                                  dbYHi = dbVTmp3;
                                  dbVTmp1 = (dbVTmp1 * dbVTmp2);
                                  dbYLo = (dbVTmp1 * dbVTmp3);

                                  dbTmp1 = ((dbYHi) * (dbHH));
                                  dbTmp2 = ((dbYLo) * (dbHL));
                                  dbTmp2 = (dbTmp2 + (dbYHi) * (dbHL));
                                  dbTmp3 = (dbTmp2 + (dbYLo) * (dbHH));
                                  dbPH = dbTmp1;
                                  dbPL = dbTmp3;

                                  dbPLL = (dY * dbHLL);

                                  dbVTmp1 =
                                    (dbPH +
                                     ((__constant double *) _vmldPowHATab)[855]);
                                  iN =
                                    (((__private _iml_dp_union_t *) & dbVTmp1)->dwords.
                                     lo_dword);
                                  j = iN & 0x7F;
                                  iN = iN >> 7;
                                  dbVPHH =
                                    (dbVTmp1 -
                                     ((__constant double *) _vmldPowHATab)[855]);
                                  dbVPHL = (dbPH - dbVPHH);

                                  dbZ = (dbPLL + dbPL);
                                  dbZ = (dbZ + dbVPHL);

                                  dbExp2Poly =
                                    (((((((__constant double *)
                                          _vmldPowHATab)[849]) * dbZ +
                                        ((__constant double *) _vmldPowHATab)[848])
                                       * dbZ +
                                       ((__constant double *) _vmldPowHATab)[847])
                                      * dbZ +
                                      ((__constant double *) _vmldPowHATab)[846]) *
                                     dbZ +
                                     ((__constant double *) _vmldPowHATab)[845]) *
                                    dbZ;

                                  dbExp2PolyT =
                                    (dbExp2Poly *
                                     ((__constant double *) _vmldPowHATab)[585 +
                                                                      2 *
                                                                      (j) +
                                                                      0]);
                                  dbResLo =
                                    (dbExp2PolyT +
                                     ((__constant double *) _vmldPowHATab)[585 +
                                                                      2 *
                                                                      (j) +
                                                                      1]);
                                  dbResHi =
                                    ((__constant double *) _vmldPowHATab)[585 +
                                                                     2 * (j) +
                                                                     0];

                                  dbRes = (dbResHi + dbResLo);
                                  iERes =
                                    ((((__private _iml_dp_union_t *) & dbRes)->dwords.
                                      hi_dword >> 20) & 0x7FF) - 0x3FF;
                                  iERes = (iERes + iN);

                                  if (iERes < 1024)
                                    {
                                      if (iERes >= -1022)
                                        {

                                          (((__private _iml_dp_union_t *) & dbRes)->
                                           dwords.hi_dword =
                                           (((__private _iml_dp_union_t *) & dbRes)->
                                            dwords.
                                            hi_dword & 0x800FFFFF) |
                                           (((_iml_uint32_t) (iERes + 0x3FF) &
                                             0x7FF) << 20));

                                          dbRes = dbRes * dbSignRes;
                                          dR = dbRes;
                                        }
                                      else
                                        {

                                          if (iERes >= -1022 - 10)
                                            {

                                              dbVTmp1 =
                                                ((dbResHi) + (dbResLo));
                                              dbTmp1 = ((dbResHi) - dbVTmp1);
                                              dbVTmp2 = (dbTmp1 + (dbResLo));
                                              dbResHi = dbVTmp1;
                                              dbResLo = dbVTmp2;
                                              dbVTmp1 =
                                                ((dbResHi) *
                                                 (((__constant double *)
                                                   _vmldPowHATab)[857]));
                                              dbVTmp2 = (dbVTmp1 - (dbResHi));
                                              dbVTmp1 = (dbVTmp1 - dbVTmp2);
                                              dbVTmp2 = ((dbResHi) - dbVTmp1);
                                              dbResHi = dbVTmp1;
                                              dbTmp2 = dbVTmp2;
                                              dbResLo = (dbResLo + dbTmp2);

                                              dbSignRes *=
                                                ((__constant double *)
                                                 _vmldPowHATab)[859];
                                              iN = (iN + 200);

                                              dbTwoPowN =
                                                ((__constant double *)
                                                 _vmldPowHATab)[853];
                                              (((__private _iml_dp_union_t *) &
                                                dbTwoPowN)->dwords.hi_dword =
                                               (((__private _iml_dp_union_t *) &
                                                 dbTwoPowN)->dwords.
                                                hi_dword & 0x800FFFFF) |
                                               (((_iml_uint32_t) (iN + 0x3FF)
                                                 & 0x7FF) << 20));

                                              dbResHi = (dbResHi * dbTwoPowN);
                                              dbResHi = (dbResHi * dbSignRes);

                                              dbResLo = (dbResLo * dbTwoPowN);
                                              dbVTmp1 = (dbResLo * dbSignRes);

                                              dbRes = (dbResHi + dbVTmp1);

                                              dbVTmp1 =
                                                ((__constant double *)
                                                 _vmldPowHATab)[851];
                                              dbVTmp1 = (dbVTmp1 * dbVTmp1);
                                              dbRes = (dbRes + dbVTmp1);

                                              dR = dbRes;
                                            }
                                          else
                                            {
                                              if (iERes >= -1074 - 10)
                                                {

                                                  dbSignRes *=
                                                    ((__constant double *)
                                                     _vmldPowHATab)[859];
                                                  iN = iN + 200;

                                                  dbTwoPowN =
                                                    ((__constant double *)
                                                     _vmldPowHATab)[853];
                                                  (((__private _iml_dp_union_t *) &
                                                    dbTwoPowN)->dwords.
                                                   hi_dword =
                                                   (((__private _iml_dp_union_t *) &
                                                     dbTwoPowN)->dwords.
                                                    hi_dword & 0x800FFFFF) |
                                                   (((_iml_uint32_t)
                                                     (iN +
                                                      0x3FF) & 0x7FF) << 20));

                                                  dbRes = (dbRes * dbTwoPowN);
                                                  dbRes = (dbRes * dbSignRes);

                                                  dbVTmp1 =
                                                    ((__constant double *)
                                                     _vmldPowHATab)[851];
                                                  dbVTmp1 *= dbVTmp1;
                                                  dbRes = (dbRes + dbVTmp1);

                                                  dR = dbRes;
                                                }
                                              else
                                                {

                                                  dbVTmp1 =
                                                    ((__constant double *)
                                                     _vmldPowHATab)[851];
                                                  dbVTmp1 *= dbVTmp1;
                                                  dbRes =
                                                    (dbVTmp1 * dbSignRes);
                                                  dR = dbRes;
                                                }
                                            }
                                        }
                                    }
                                  else
                                    {

                                      dbVTmp1 =
                                        ((__constant double *) _vmldPowHATab)[850];
                                      dbVTmp1 = (dbVTmp1 * dbVTmp1);
                                      dbRes = (dbVTmp1 * dbSignRes);
                                      dR = dbRes;
                                    }
                                }
                              else
                                {

                                  dbVTmp1 =
                                    ((__constant double *) _vmldPowHATab)[853];
                                  dbVTmp1 =
                                    (dbVTmp1 +
                                     ((__constant double *) _vmldPowHATab)[851]);
                                  dR = (dbVTmp1 * dbSignRes);
                                }
                            }
                          else
                            {

                              iSign =
                                iSignY ^ (((__private _iml_dp_union_t *) & dbT_CQHi)->
                                          dwords.hi_dword >> 31);

                              dbTmp1 =
                                ((__constant double *) _vmldPowHATab)[850 +
                                                                 (iSign)];

                              dbTmp1 = (dbTmp1 * dbTmp1);

                              dbTmp1 = (dbTmp1 * dbSignRes);
                              dR = dbTmp1;
                            }
                        }
                      else
                        {

                          dbVTmp1 = ((__constant double *) _vmldPowHATab)[852];
                          dbVTmp1 = dbVTmp1 / dbVTmp1;
                          dR = dbVTmp1;
                          nRet = 1;
                        }
                    }
                  else
                    {

                      if (iEXB < 0x3FF)
                        {

                          if (iSignY)
                            {

                              dR = dY * dY;
                            }
                          else
                            {

                              dR = ((__constant double *) _vmldPowHATab)[852];
                            }
                        }
                      else
                        {

                          if (iSignY)
                            {

                              dR =
                                ((__constant double *) _vmldPowHATab)[852] *
                                ((__constant double *) _vmldPowHATab)[853 +
                                                                 (iYIsInt &
                                                                  iSignX)];
                            }
                          else
                            {

                              dbTmp1 = dX * dX;
                              dbTmp1 = dbTmp1 * dY;
                              dR =
                                dbTmp1 *
                                ((__constant double *) _vmldPowHATab)[853 +
                                                                 (iYIsInt &
                                                                  iSignX)];
                            }
                        }
                    }
                }
              else
                {

                  dR = ((__constant double *) _vmldPowHATab)[853 + (iYIsInt & 1)];
                }
            }
          else
            {

              dbTmp1 = dX * dX;

              if (iSignY)
                {

                  dR =
                    ((__constant double *) _vmldPowHATab)[853 +
                                                     (iYIsInt & iSignX)] /
                    dbTmp1;
                  nRet = 1;
                }
              else
                {

                  dR =
                    ((__constant double *) _vmldPowHATab)[853 +
                                                     (iYIsInt & iSignX)] *
                    dbTmp1;
                }
            }
        }
      else
        {

          dR = dX + dY;
        }
    }
  else
    {

      dbVTmp1 = dX + dY;
      iSign = (((__private _iml_dp_union_t *) & dbVTmp1)->dwords.hi_dword >> 31);
      dbVTmp2 = ((__constant double *) _vmldPowHATab)[853];
      (((__private _iml_dp_union_t *) & dbVTmp2)->dwords.hi_dword =
       (((__private _iml_dp_union_t *) & dbVTmp2)->dwords.
        hi_dword & 0x7FFFFFFF) | ((_iml_uint32_t) (iSign) << 31));

      dR = dbVTmp2 * dbVTmp2;
    }

  dR *= dResSign;
  *r = dR;
  return nRet;
}

#endif

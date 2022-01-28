/*===================== begin_copyright_notice ==================================

Copyright (c) 2022 Intel Corporation

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

#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF

inline int __dpow_la_pow_lut_cout (double *a, double *b, double *r)
{
    int nRet = 0;
    double dbVTmp1, dbVTmp2, dbVPHH, dbVPHL;
    double dX, dY, dR, dbAX, dbSignRes, dbX1, dbRcp1, dbL1Hi, dbL1Lo, dbX2, dbRcp2, dbL2Hi, dbL2Lo,
        dbX3, dbRcp3C, dbL3Hi, dbL3Lo, dbK, dbT, dbD, dbR1, dbCQ, dbRcpC, dbX1Hi, dbX1Lo,
        dbRcpCHi, dbRcpCLo, dbTmp1, dbE, dbT_CQHi, dbCQLo, dbR, dbLogPart3, dbLog2Poly,
        dbHH, dbHL, dbHLL, dbYHi, dbYLo, dbTmp2, dbTmp3, dbPH, dbPL, dbPLL, dbZ, dbExp2Poly, dbExp2PolyT, dbResLo, dbResHi, dbRes, dbTwoPowN, dbAY;
    int i, iEXB, iEYB, iSignX, iSignY, iYHi, iYLo, iYIsFinite, iEY, iYIsInt, iXIsFinite,
        iDenoExpAdd, iXHi, k, i1, i2, i3, iELogAX, iN, j, iERes, iSign, iIsSigZeroX, iIsSigZeroY;
    dX = ((*a) * ((__constant double *) __dpow_la_CoutTab)[853 + (0)]);
    dY = ((*b) * ((__constant double *) __dpow_la_CoutTab)[853 + (0)]);
    iEXB = ((((_iml_dp_union_t *) & dX)->dwords.hi_dword >> 20) & 0x7FF);
    iEYB = ((((_iml_dp_union_t *) & dY)->dwords.hi_dword >> 20) & 0x7FF);
    iSignX = (((_iml_dp_union_t *) & dX)->dwords.hi_dword >> 31);
    iSignY = (((_iml_dp_union_t *) & dY)->dwords.hi_dword >> 31);
    iIsSigZeroX = (((((_iml_dp_union_t *) & dX)->dwords.hi_dword & 0x000FFFFF) == 0) && ((((_iml_dp_union_t *) & dX)->dwords.lo_dword) == 0));
    iIsSigZeroY = (((((_iml_dp_union_t *) & dY)->dwords.hi_dword & 0x000FFFFF) == 0) && ((((_iml_dp_union_t *) & dY)->dwords.lo_dword) == 0));
    iYHi = (iEYB << 20) | (((_iml_dp_union_t *) & dY)->dwords.hi_dword & 0x000FFFFF);
    iYLo = (((_iml_dp_union_t *) & dY)->dwords.lo_dword);
    iYIsFinite = (((((_iml_dp_union_t *) & dY)->dwords.hi_dword >> 20) & 0x7FF) != 0x7FF);
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
    if (!((iSignX == 0) && (iEXB == 0x3FF) && iIsSigZeroX) && !((iEYB == 0) && iIsSigZeroY))
    {
        iXIsFinite = (((((_iml_dp_union_t *) & dX)->dwords.hi_dword >> 20) & 0x7FF) != 0x7FF);
        if ((iXIsFinite || iIsSigZeroX) && (iYIsFinite || iIsSigZeroY))
        {
            if (dX != ((__constant double *) __dpow_la_CoutTab)[852])
            {
                if (!((dX == ((__constant double *) __dpow_la_CoutTab)[854]) && (iYIsInt || !iYIsFinite)))
                {
                    if (iXIsFinite && iYIsFinite)
                    {
                        if ((dX > ((__constant double *) __dpow_la_CoutTab)[852]) || iYIsInt)
                        {
                            dbSignRes = ((__constant double *) __dpow_la_CoutTab)[853 + (iSignX & iYIsInt)];
                            iDenoExpAdd = 0;
                            dbAX = dX;
                            (((_iml_dp_union_t *) & dbAX)->dwords.hi_dword =
                             (((_iml_dp_union_t *) & dbAX)->dwords.hi_dword & 0x7FFFFFFF) | ((_iml_uint32_t) (0) << 31));
                            if (iEXB == 0)
                            {
                                dbAX = dbAX * ((__constant double *) __dpow_la_CoutTab)[858];
                                iDenoExpAdd = iDenoExpAdd - 200;
                            }
                            dbX1 = dbAX;
                            (((_iml_dp_union_t *) & dbX1)->dwords.hi_dword =
                             (((_iml_dp_union_t *) & dbX1)->dwords.hi_dword & 0x800FFFFF) | (((_iml_uint32_t) (0x3FF) & 0x7FF) << 20));
                            iXHi = ((((_iml_dp_union_t *) & dbAX)->dwords.hi_dword >> 20) & 0x7FF);
                            iXHi = iXHi << 20;
                            iXHi = iXHi | (((_iml_dp_union_t *) & dbAX)->dwords.hi_dword & 0x000FFFFF);
                            k = iXHi - 0x3FE7C000;
                            k = k >> 20;
                            k = k + iDenoExpAdd;
                            i1 = (((_iml_dp_union_t *) & dbX1)->dwords.hi_dword & 0x000FFFFF);
                            i1 = i1 & 0xFC000;
                            i1 = i1 + 0x4000;
                            i1 = i1 >> 15;
                            dbRcp1 = ((__constant double *) __dpow_la_CoutTab)[0 + i1];
                            dbL1Hi = ((__constant double *) __dpow_la_CoutTab)[33 + 2 * (i1) + 0];
                            dbL1Lo = ((__constant double *) __dpow_la_CoutTab)[33 + 2 * (i1) + 1];
                            dbX2 = dbX1 * dbRcp1;
                            i2 = (((_iml_dp_union_t *) & dbX2)->dwords.hi_dword & 0x000FFFFF);
                            i2 = i2 & 0xFC00;
                            i2 = i2 + 0x400;
                            i2 = i2 >> 11;
                            dbRcp2 = ((__constant double *) __dpow_la_CoutTab)[99 + i2];
                            dbL2Hi = ((__constant double *) __dpow_la_CoutTab)[132 + 2 * (i2) + 0];
                            dbL2Lo = ((__constant double *) __dpow_la_CoutTab)[132 + 2 * (i2) + 1];
                            dbX3 = dbX2 * dbRcp2;
                            i3 = (((_iml_dp_union_t *) & dbX3)->dwords.hi_dword & 0x000FFFFF);
                            i3 = i3 & 0xFF0;
                            i3 = i3 + 0x10;
                            i3 = i3 >> 5;
                            dbRcp3C = ((__constant double *) __dpow_la_CoutTab)[198 + i3];
                            dbL3Hi = ((__constant double *) __dpow_la_CoutTab)[327 + 2 * (i3) + 0];
                            dbL3Lo = ((__constant double *) __dpow_la_CoutTab)[327 + 2 * (i3) + 1];
                            dbK = (double) k;
                            dbT = (dbK + dbL1Hi);
                            dbT = (dbT + dbL2Hi);
                            dbT = (dbT + dbL3Hi);
                            dbD = (dbL2Lo + dbL3Lo);
                            dbD = (dbD + dbL1Lo);
                            dbR1 = (dbX3 * dbRcp3C);
                            dbCQ = (dbR1 - ((__constant double *) __dpow_la_CoutTab)[856]);
                            dbRcpC = (dbRcp1 * dbRcp2);
                            dbRcpC = (dbRcpC * dbRcp3C);
                            dbVTmp1 = ((dbX1) * (((__constant double *) __dpow_la_CoutTab)[857]));
                            dbVTmp2 = (dbVTmp1 - (dbX1));
                            dbVTmp1 = (dbVTmp1 - dbVTmp2);
                            dbVTmp2 = ((dbX1) - dbVTmp1);
                            dbX1Hi = dbVTmp1;
                            dbX1Lo = dbVTmp2;
                            dbVTmp1 = ((dbRcpC) * (((__constant double *) __dpow_la_CoutTab)[857]));
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
                            iELogAX = ((((_iml_dp_union_t *) & dbT_CQHi)->dwords.hi_dword >> 20) & 0x7FF);
                            if (iELogAX + iEYB < 11 + 2 * 0x3FF)
                            {
                                if (iELogAX + iEYB > -62 + 2 * 0x3FF)
                                {
                                    dbR = (dbCQ + dbE);
                                    dbLog2Poly =
                                        ((((((__constant double *) __dpow_la_CoutTab)[844]) * dbR +
                                           ((__constant double *) __dpow_la_CoutTab)[843]) * dbR +
                                          ((__constant double *) __dpow_la_CoutTab)[842]) * dbR +
                                         ((__constant double *) __dpow_la_CoutTab)[841]) * dbR;
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
                                    dbVTmp1 = ((dbHH) * (((__constant double *) __dpow_la_CoutTab)[857]));
                                    dbVTmp2 = (dbVTmp1 - (dbHH));
                                    dbVTmp1 = (dbVTmp1 - dbVTmp2);
                                    dbVTmp2 = ((dbHH) - dbVTmp1);
                                    dbHH = dbVTmp1;
                                    dbHL = dbVTmp2;
                                    dbVTmp1 = ((dY) * (((__constant double *) __dpow_la_CoutTab)[857]));
                                    dbVTmp2 = (dbVTmp1 - (dY));
                                    dbVTmp1 = (dbVTmp1 - dbVTmp2);
                                    dbVTmp2 = ((dY) - dbVTmp1);
                                    dbYHi = dbVTmp1;
                                    dbYLo = dbVTmp2;
                                    dbTmp1 = ((dbYHi) * (dbHH));
                                    dbTmp2 = ((dbYLo) * (dbHL));
                                    dbTmp2 = (dbTmp2 + (dbYHi) * (dbHL));
                                    dbTmp3 = (dbTmp2 + (dbYLo) * (dbHH));
                                    dbPH = dbTmp1;
                                    dbPL = dbTmp3;
                                    dbPLL = (dY * dbHLL);
                                    dbVTmp1 = (dbPH + ((__constant double *) __dpow_la_CoutTab)[855]);
                                    iN = (((_iml_dp_union_t *) & dbVTmp1)->dwords.lo_dword);
                                    j = iN & 0x7F;
                                    iN = iN >> 7;
                                    dbVPHH = (dbVTmp1 - ((__constant double *) __dpow_la_CoutTab)[855]);
                                    dbVPHL = (dbPH - dbVPHH);
                                    dbZ = (dbPLL + dbPL);
                                    dbZ = (dbZ + dbVPHL);
                                    dbExp2Poly =
                                        (((((((__constant double *) __dpow_la_CoutTab)[849]) * dbZ +
                                            ((__constant double *) __dpow_la_CoutTab)[848]) * dbZ +
                                           ((__constant double *) __dpow_la_CoutTab)[847]) * dbZ +
                                          ((__constant double *) __dpow_la_CoutTab)[846]) * dbZ +
                                         ((__constant double *) __dpow_la_CoutTab)[845]) * dbZ;
                                    dbExp2PolyT = (dbExp2Poly * ((__constant double *) __dpow_la_CoutTab)[585 + 2 * (j) + 0]);
                                    dbResLo = (dbExp2PolyT + ((__constant double *) __dpow_la_CoutTab)[585 + 2 * (j) + 1]);
                                    dbResHi = ((__constant double *) __dpow_la_CoutTab)[585 + 2 * (j) + 0];
                                    dbRes = (dbResHi + dbResLo);
                                    iERes = ((((_iml_dp_union_t *) & dbRes)->dwords.hi_dword >> 20) & 0x7FF) - 0x3FF;
                                    iERes = (iERes + iN);
                                    if (iERes < 1024)
                                    {
                                        if (iERes >= -1022)
                                        {
                                            (((_iml_dp_union_t *) & dbRes)->dwords.hi_dword =
                                             (((_iml_dp_union_t *) & dbRes)->dwords.
                                              hi_dword & 0x800FFFFF) | (((_iml_uint32_t) (iERes + 0x3FF) & 0x7FF) << 20));
                                            dbRes = dbRes * dbSignRes;
                                            dR = dbRes;
                                        }
                                        else
                                        {
                                            if (iERes >= -1022 - 10)
                                            {
                                                dbVTmp1 = ((dbResHi) + (dbResLo));
                                                dbTmp1 = ((dbResHi) - dbVTmp1);
                                                dbVTmp2 = (dbTmp1 + (dbResLo));
                                                dbResHi = dbVTmp1;
                                                dbResLo = dbVTmp2;
                                                dbVTmp1 = ((dbResHi) * (((__constant double *) __dpow_la_CoutTab)[857]));
                                                dbVTmp2 = (dbVTmp1 - (dbResHi));
                                                dbVTmp1 = (dbVTmp1 - dbVTmp2);
                                                dbVTmp2 = ((dbResHi) - dbVTmp1);
                                                dbResHi = dbVTmp1;
                                                dbTmp2 = dbVTmp2;
                                                dbResLo = (dbResLo + dbTmp2);
                                                dbSignRes *= ((__constant double *) __dpow_la_CoutTab)[859];
                                                iN = (iN + 200);
                                                dbTwoPowN = ((__constant double *) __dpow_la_CoutTab)[853];
                                                (((_iml_dp_union_t *) & dbTwoPowN)->dwords.hi_dword =
                                                 (((_iml_dp_union_t *) & dbTwoPowN)->dwords.
                                                  hi_dword & 0x800FFFFF) | (((_iml_uint32_t) (iN + 0x3FF) & 0x7FF) << 20));
                                                dbResHi = (dbResHi * dbTwoPowN);
                                                dbResLo = (dbResLo * dbTwoPowN);
                                                dbRes = (dbResHi + dbResLo);
                                                dbRes = (dbRes * dbSignRes);
                                                dbVTmp1 = ((__constant double *) __dpow_la_CoutTab)[851];
                                                dbVTmp1 = (dbVTmp1 * dbVTmp1);
                                                dbRes = (dbRes + dbVTmp1);
                                                dR = dbRes;
                                            }
                                            else
                                            {
                                                if (iERes >= -1074 - 10)
                                                {
                                                    dbSignRes *= ((__constant double *) __dpow_la_CoutTab)[859];
                                                    iN = iN + 200;
                                                    dbTwoPowN = ((__constant double *) __dpow_la_CoutTab)[853];
                                                    (((_iml_dp_union_t *) & dbTwoPowN)->dwords.hi_dword =
                                                     (((_iml_dp_union_t *) & dbTwoPowN)->dwords.
                                                      hi_dword & 0x800FFFFF) | (((_iml_uint32_t) (iN + 0x3FF) & 0x7FF) << 20));
                                                    dbRes = (dbRes * dbTwoPowN);
                                                    dbRes = (dbRes * dbSignRes);
                                                    dbVTmp1 = ((__constant double *) __dpow_la_CoutTab)[851];
                                                    dbVTmp1 *= dbVTmp1;
                                                    dbRes = (dbRes - dbVTmp1);
                                                    dR = dbRes;
                                                }
                                                else
                                                {
                                                    dbVTmp1 = ((__constant double *) __dpow_la_CoutTab)[851];
                                                    dbVTmp1 *= dbVTmp1;
                                                    dbRes = (dbVTmp1 * dbSignRes);
                                                    dR = dbRes;
                                                }
                                            }
                                        }
                                    }
                                    else
                                    {
                                        dbVTmp1 = ((__constant double *) __dpow_la_CoutTab)[850];
                                        dbVTmp1 = (dbVTmp1 * dbVTmp1);
                                        dbRes = (dbVTmp1 * dbSignRes);
                                        dR = dbRes;
                                    }
                                }
                                else
                                {
                                    dbVTmp1 = ((__constant double *) __dpow_la_CoutTab)[853];
                                    dbVTmp1 = (dbVTmp1 + ((__constant double *) __dpow_la_CoutTab)[851]);
                                    dR = (dbVTmp1 * dbSignRes);
                                }
                            }
                            else
                            {
                                iSign = iSignY ^ (((_iml_dp_union_t *) & dbT_CQHi)->dwords.hi_dword >> 31);
                                dbTmp1 = ((__constant double *) __dpow_la_CoutTab)[850 + (iSign)];
                                dbTmp1 = (dbTmp1 * dbTmp1);
                                dbTmp1 = (dbTmp1 * dbSignRes);
                                dR = dbTmp1;
                                if ((!(((((_iml_dp_union_t *) & dR)->dwords.hi_dword >> 20) & 0x7FF) != 0x7FF)
                                     && (((((_iml_dp_union_t *) & dR)->dwords.hi_dword & 0x000FFFFF) == 0)
                                         && ((((_iml_dp_union_t *) & dR)->dwords.lo_dword) == 0))))
                                {
                                    nRet = 3;
                                }
                            }
                        }
                        else
                        {
                            dbVTmp1 = ((__constant double *) __dpow_la_CoutTab)[852];
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
                                dR = ((__constant double *) __dpow_la_CoutTab)[852];
                            }
                        }
                        else
                        {
                            if (iSignY)
                            {
                                dR = ((__constant double *) __dpow_la_CoutTab)[852] * ((__constant double *) __dpow_la_CoutTab)[853 +
                                                                                                                                (iYIsInt & iSignX)];
                            }
                            else
                            {
                                dbTmp1 = dX * dX;
                                dbTmp1 = dbTmp1 * dY;
                                dR = dbTmp1 * ((__constant double *) __dpow_la_CoutTab)[853 + (iYIsInt & iSignX)];
                            }
                        }
                    }
                }
                else
                {
                    dR = ((__constant double *) __dpow_la_CoutTab)[853 + (iYIsInt & 1)];
                }
            }
            else
            {
                dbTmp1 = dX * dX;
                if (iSignY)
                {
                    dR = ((__constant double *) __dpow_la_CoutTab)[853 + (iYIsInt & iSignX)] / dbTmp1;
                    nRet = 1;
                }
                else
                {
                    dR = ((__constant double *) __dpow_la_CoutTab)[853 + (iYIsInt & iSignX)] * dbTmp1;
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
        iSign = (((_iml_dp_union_t *) & dbVTmp1)->dwords.hi_dword >> 31);
        dbVTmp2 = ((__constant double *) __dpow_la_CoutTab)[853];
        (((_iml_dp_union_t *) & dbVTmp2)->dwords.hi_dword =
         (((_iml_dp_union_t *) & dbVTmp2)->dwords.hi_dword & 0x7FFFFFFF) | ((_iml_uint32_t) (iSign) << 31));
        dR = dbVTmp2 * dbVTmp2;
    }
    *r = dR;
    return nRet;
}

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpow_la_lc16 = { 0xbfc0eb775ed0d53fUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpow_la_lc15 = { 0x3fc1ea5c772d0f69UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpow_la_lc14 = { 0xbfc243278b687c88UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpow_la_lc13 = { 0x3fc3ac83f2e91adfUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpow_la_lc12 = { 0xbfc55569367812bfUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpow_la_lc11 = { 0x3fc745de6106c97eUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpow_la_lc10 = { 0xbfc99999760c1f82UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpow_la_lc9 = { 0x3fcc71c70a4bb945UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpow_la_lc8 = { 0xbfd00000001076daUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpow_la_lc7 = { 0x3fd24924924f345dUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpow_la_lc6 = { 0xbfd5555555554e88UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpow_la_lc5 = { 0x3fd9999999999815UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpow_la_lc4l = { 0xbc8A6AF5D88E6C6DUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpow_la_lc4 = { 0xbfe0000000000000UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpow_la_lc3l = { 0x3c8751507e77d245UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpow_la_lc3 = { 0x3fe5555555555555UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpow_la_LN2H = { 0x3FE62E42FEFA3800UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpow_la_LN2L = { 0x3D2EF35793C76800UL };

static __constant unsigned long __dpow_la_lTh0 = 0xc086232bdd7abae4UL;
static __constant unsigned long __dpow_la_lTl0 = 0xbdcee3dde7fd844cUL;
static __constant unsigned long __dpow_la_lTh1 = 0xc08624f4dcf7348dUL;
static __constant unsigned long __dpow_la_lTl1 = 0xbdceedff94235c6bUL;
static __constant unsigned long __dpow_la_lTh2 = 0xc086266a41f852d7UL;
static __constant unsigned long __dpow_la_lTl2 = 0xbdcee475cd6a9f39UL;
static __constant unsigned long __dpow_la_lTh3 = 0xc08627a5f55256f4UL;
static __constant unsigned long __dpow_la_lTl3 = 0xbdcee71edfaeeae4UL;
static __constant unsigned long __dpow_la_lTh4 = 0xc08628b76e3a7972UL;
static __constant unsigned long __dpow_la_lTl4 = 0xbdceeb9abde27626UL;
static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpow_la_p_L2E = { 0x3ff71547652B82FEUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpow_la_p_Shifter = { 0x43280000000007feUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpow_la_p_NL2H = { 0xbfe62e42fefa39efUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpow_la_p_NL2L = { 0xbc7abc9e3b39803fUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpow_la_p_c0 = { 0x3fdffffffffffe76UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpow_la_p_c1 = { 0x3fc5555555555462UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpow_la_p_c2 = { 0x3fa55555556228ceUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpow_la_p_c3 = { 0x3f811111111ac486UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpow_la_p_c4 = { 0x3f56c16b8144bd5bUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpow_la_p_c5 = { 0x3f2a019f7560fba3UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpow_la_p_c6 = { 0x3efa072e44b58159UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpow_la_p_c7 = { 0x3ec722bccc270959UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpow_la_p_one = { 0x3ff0000000000000UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpow_la_two_52 = { 0x4330000000000000UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpow_la_two_64 = { 0x43f0000000000000UL };

inline int __dpow_la_pow_lut_cout (double *pxin, double *pyin, double *pres);
inline int __internal_dpow_nolut_cout (double *pxin, double *pyin, double *pres)
{
    int nRet = 0;
    union
    {
        unsigned long w;
        unsigned int w32[2];
        int s32[2];
        double f;
    } x, y, mant_x, rcp, Th, Tl, lTh01, lTl01, lTh34, lTl34;
    union
    {
        unsigned int w;
        float f;
    } mantf, rcpf;
    double lpoly, R, R2h, R2l, RS, RS2, c3Rh, c3Rl, c3R3h, c3R3l, RS2_h, RS2_l;
    double H, L, R_half, expon_x;
    int iexpon, y_is_odd, y_is_even, ires_scale;
    union
    {
        unsigned long w;
        unsigned int w32[2];
        int s32[2];
        double f;
    } xi, zero, res_special, ylx_h, ylx_l, scale, res;
    union
    {
        unsigned long w;
        unsigned int w32[2];
        int s32[2];
        double f;
    } idx, T, Tlr, dI, dyi, res_scale, ylx_h0, ylx_lh;
    double N, R0, poly, ya;
    int expon32, mask32, mask_h, index;
    unsigned int xa32, sgn_x, expon_corr, iexpon_x, res_sgn = 0;
    double xin = (*pxin);
    double yin = (*pyin);
    x.f = xin;
    y.f = yin;
  LOG_MAIN:
    iexpon_x = x.w32[1] >> 20;
    expon_x = (double) (iexpon_x);
  LOG_MAIN_CONT:
    mant_x.w32[1] = (x.w32[1] & 0x000fffffu) | 0x3fc00000u;
    mant_x.w32[0] = x.w32[0];
    mantf.f = (float) mant_x.f;
    rcpf.f = 1.0f / (mantf.f);
    rcpf.f = SPIRV_OCL_BUILTIN(rint, _f32, ) (rcpf.f);
    rcp.f = (double) rcpf.f;
    R = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (rcp.f, mant_x.f, (-1.0));
    index = (rcpf.w >> (23 - 2)) & 0x7;
    index ^= 4;
    lTh01.w = (index & 1) ? __dpow_la_lTh1 : __dpow_la_lTh0;
    lTl01.w = (index & 1) ? __dpow_la_lTl1 : __dpow_la_lTl0;
    lTh34.w = (index & 1) ? __dpow_la_lTh3 : __dpow_la_lTh4;
    lTl34.w = (index & 1) ? __dpow_la_lTl3 : __dpow_la_lTl4;
    Th.w = (index < 2) ? lTh01.w : __dpow_la_lTh2;
    Tl.w = (index < 2) ? lTl01.w : __dpow_la_lTl2;
    Th.w = (index > 2) ? lTh34.w : Th.w;
    Tl.w = (index > 2) ? lTl34.w : Tl.w;
    lpoly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (__dpow_la_lc16.f, R, __dpow_la_lc15.f);
    lpoly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (lpoly, R, __dpow_la_lc14.f);
    lpoly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (lpoly, R, __dpow_la_lc13.f);
    lpoly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (lpoly, R, __dpow_la_lc12.f);
    lpoly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (lpoly, R, __dpow_la_lc11.f);
    lpoly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (lpoly, R, __dpow_la_lc10.f);
    lpoly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (lpoly, R, __dpow_la_lc9.f);
    lpoly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (lpoly, R, __dpow_la_lc8.f);
    lpoly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (lpoly, R, __dpow_la_lc7.f);
    lpoly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (lpoly, R, __dpow_la_lc6.f);
    lpoly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (lpoly, R, __dpow_la_lc5.f);
    lpoly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (lpoly, R, __dpow_la_lc4.f);
    lpoly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (lpoly, R, __dpow_la_lc3l.f);
    R_half = 0.5 * R;
    RS = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) ((-R), R_half, R);
    R2h = R - RS;
    R2l = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) ((-R), R_half, R2h);
    c3Rh = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (__dpow_la_lc3.f, R, 0.0);
    c3Rl = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (__dpow_la_lc3.f, R, (-c3Rh));
    lpoly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (lpoly, R, c3Rl);
    Tl.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) ((-R2l), lpoly, Tl.f);
    Tl.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (expon_x, __dpow_la_LN2L.f, Tl.f);
    RS2 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (c3Rh, R2h, RS);
    c3R3h = RS2 - RS;
    c3R3l = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (c3Rh, R2h, (-c3R3h));
    c3R3l = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) ((-c3Rh), R2l, c3R3l);
    R2l = R2l + c3R3l;
    Th.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (expon_x, __dpow_la_LN2H.f, Th.f);
    H = Th.f + RS2;
    RS2_h = H - Th.f;
    RS2_l = RS2 - RS2_h;
    R2l = R2l + RS2_l;
    Tl.f = Tl.f + R2l;
    L = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (R2h, lpoly, Tl.f);
    ylx_h0.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (y.f, H, 0.0);
    ylx_l.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (y.f, H, (-ylx_h0.f));
    ylx_l.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (y.f, L, ylx_l.f);
    ylx_h.f = ylx_h0.f + ylx_l.f;
    ylx_lh.f = ylx_h.f - ylx_h0.f;
    ylx_l.f = ylx_l.f - ylx_lh.f;
    iexpon_x--;
    if ((iexpon_x >= 0x7fe) || ((ylx_h.w32[1] & 0x7fffffff) >= 0x4086232B))
        return __dpow_la_pow_lut_cout (pxin, pyin, pres);
    idx.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (ylx_h.f, __dpow_la_p_L2E.f, __dpow_la_p_Shifter.f);
    N = idx.f - __dpow_la_p_Shifter.f;
    mask32 = idx.w32[0] << 31;
    expon32 = idx.w32[0] << (20 + 31 - 32);
    R0 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (__dpow_la_p_NL2H.f, N, ylx_h.f);
    R = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (__dpow_la_p_NL2L.f, N, R0);
    R = R + ylx_l.f;
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (__dpow_la_p_c7.f, R, __dpow_la_p_c6.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dpow_la_p_c5.f);
    mask32 = mask32 >> 31;
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dpow_la_p_c4.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dpow_la_p_c3.f);
    mask_h = mask32 & 0x000EA09E;
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dpow_la_p_c2.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dpow_la_p_c1.f);
    T.w32[1] = expon32 ^ mask_h;
    T.w32[0] = mask32 & 0x667F3BCD;
    Tlr.w32[1] = 0x3C6E51C5 ^ (mask32 & (0xBC8FD36E ^ 0x3C6E51C5));
    Tlr.w32[0] = 0;
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dpow_la_p_c0.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dpow_la_p_one.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, Tlr.f);
    res.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (T.f, poly, T.f);
    res.w32[1] ^= res_sgn;
    *pres = res.f;
    return nRet;
  POW_SPECIAL:
    if (y.f == 0)
    {
        *pres = 1.0;
        return nRet;
    }
    if (x.f == 1.0)
    {
        res.w32[1] = res_sgn | 0x3ff00000;
        res.w32[0] = 0;
        *pres = res.f;
        return nRet;
    }
    if (((x.w << 1) > 0xffe0000000000000UL) || ((y.w << 1) > 0xffe0000000000000UL))
    {
        *pres = x.f + y.f;
        return nRet;
    }
    if (y.w == 0xfff0000000000000UL)
    {
        if (x.f == -1.0)
        {
            *pres = 1.0;
            return nRet;
        }
        res.w = (SPIRV_OCL_BUILTIN(fabs, _f64, ) (x.f) < 1.0) ? 0x7ff0000000000000UL : 0;
        nRet = 1;
        *pres = res.f;
        return nRet;
    }
    if (y.w == 0x7ff0000000000000UL)
    {
        if (x.f == -1.0)
        {
            *pres = 1.0;
            return nRet;
        }
        res.w = (SPIRV_OCL_BUILTIN(fabs, _f64, ) (x.f) < 1.0) ? 0 : 0x7ff0000000000000UL;
        *pres = res.f;
        return nRet;
    }
    if (x.w == 0x7ff0000000000000UL)
    {
        res.w = (y.f < 0) ? 0 : 0x7ff0000000000000UL;
        *pres = res.f;
        return nRet;
    }
    ya = SPIRV_OCL_BUILTIN(fabs, _f64, ) (y.f);
    if (ya >= __dpow_la_two_52.f)
    {
        if (ya * 0.5 >= __dpow_la_two_52.f)
        {
            y_is_odd = 0;
            y_is_even = 1;
        }
        else
        {
            y_is_odd = y.w & 1;
            y_is_even = y_is_odd ^ 1;
        }
    }
    else
    {
        y_is_odd = y_is_even = 0;
        dI.f = __dpow_la_two_52.f + ya;
        dyi.f = dI.f - __dpow_la_two_52.f;
        if (dyi.f == ya)
        {
            y_is_odd = dI.w32[0] & 1;
            y_is_even = y_is_odd ^ 1;
        }
    }
    if (x.w == 0xfff0000000000000UL)
    {
        if (y.f < 0)
            res.w = (y_is_odd) ? 0x8000000000000000UL : 0;
        else
            res.w = (y_is_odd) ? 0xfff0000000000000UL : 0x7ff0000000000000UL;
        *pres = res.f;
        return nRet;
    }
    if (x.f == 0)
    {
        if (y.f < 0)
        {
            res.w = 0x7ff0000000000000UL;
            if (y_is_odd)
                res.w |= (x.w & 0x8000000000000000UL);
            nRet = 1;
            *pres = res.f;
            return nRet;
        }
        res.w = (y_is_odd) ? (x.w & 0x8000000000000000UL) : 0;
        *pres = res.f;
        return nRet;
    }
    if (x.f < 0)
    {
        if (y_is_odd | y_is_even)
        {
            if (y_is_odd)
                res_sgn = 0x80000000;
            x.w ^= 0x8000000000000000UL;
            goto LOG_MAIN;
        }
        res.w = 0xfff8000000000000UL;
        nRet = 1;
        *pres = res.f;
        return nRet;
    }
    iexpon_x++;
    if (iexpon_x == 0)
    {
        x.f *= __dpow_la_two_64.f;
        iexpon_x = x.w32[1] >> 20;
        expon_x = (double) (iexpon_x) - 64.0;
        goto LOG_MAIN_CONT;
    }
    res_scale.f = 1.0;
    if (ylx_h0.f < 0)
    {
        if (ylx_h0.f < -746.0)
        {
            res.w32[1] = res_sgn;
            res.w32[0] = 0;
            nRet = 4;
            *pres = res.f;
            return nRet;
        }
        ires_scale = -512 * 2;
        res_scale.w = 0x1ff0000000000000UL;
    }
    else
    {
        if (ylx_h0.f >= 710.0)
        {
            res.w32[1] = res_sgn | 0x7ff00000;
            res.w32[0] = 0;
            nRet = 3;
            *pres = res.f;
            return nRet;
        }
        ires_scale = 512 * 2;
        res_scale.w = 0x5ff0000000000000UL;
    }
    idx.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (ylx_h.f, __dpow_la_p_L2E.f, __dpow_la_p_Shifter.f);
    N = idx.f - __dpow_la_p_Shifter.f;
    mask32 = idx.w32[0] << 31;
    idx.w32[0] -= ires_scale;
    expon32 = idx.w32[0] << (20 + 31 - 32);
    R0 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (__dpow_la_p_NL2H.f, N, ylx_h.f);
    R = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (__dpow_la_p_NL2L.f, N, R0);
    R = R + ylx_l.f;
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (__dpow_la_p_c7.f, R, __dpow_la_p_c6.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dpow_la_p_c5.f);
    mask32 = mask32 >> 31;
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dpow_la_p_c4.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dpow_la_p_c3.f);
    mask_h = mask32 & 0x000EA09E;
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dpow_la_p_c2.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dpow_la_p_c1.f);
    T.w32[1] = expon32 ^ mask_h;
    T.w32[0] = mask32 & 0x667F3BCD;
    Tlr.w32[1] = 0x3C6E51C5 ^ (mask32 & (0xBC8FD36E ^ 0x3C6E51C5));
    Tlr.w32[0] = 0;
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dpow_la_p_c0.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dpow_la_p_one.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, Tlr.f);
    res.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (T.f, poly, T.f);
    res_scale.w32[1] ^= res_sgn;
    res.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (res.f, res_scale.f, 0.0);
    *pres = res.f;
    return nRet;
}

double __ocl_svml_pow_noLUT (double a, double b)
{
    double va1;
    double va2;
    double vr1;
    double r;
    va1 = a;
    va2 = b;
    __internal_dpow_nolut_cout (&va1, &va2, &vr1);
    r = vr1;
    return r;
}

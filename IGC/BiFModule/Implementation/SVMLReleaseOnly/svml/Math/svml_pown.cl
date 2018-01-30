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

float __ocl_svml_spown_cout_rare (const float a, const int b)
{
    float r;

    float flVTmp1, flVTmp2, flVPHH, flVPHL;
    float flAX, flSignRes, flX1, flRcp1, flL1Hi, flL1Lo, flX2, flRcp2, flL2Hi,
        flL2Lo, flX3, flRcp3C, flL3Hi, flL3Lo, flK, flT, flD, flR1, flCQ, flRcpC,
        flX1Hi, flX1Lo, flRcpCHi, flRcpCLo, flTmp1, flE, flT_CQHi, flCQLo, flR,
        flLogPart3, flLog2Poly, flHH, flHL, flHLL, flYHi, flYLo, flTmp2, flTmp3,
        flPH, flPL, flPLL, flZ, flExp2Poly, flExp2PolyT, flResLo, flResHi, flRes,
        flTwoPowN, flAi, flBi;
    float flT_lo_1, flT_lo_2, flT_lo_3;

    int iEXB, iEYB, iSignX, iSignY, iYIsFinite, iEY, iYIsInt,
        iXIsFinite, iDenoExpAdd, iXHi, k, i1, i2, i3, iELogAX, iN, j, iERes,
        iSign, iIsSigZeroX, iIsSigZeroY, iYMantissa, iEX, iBi;

    iBi = b;
    flAi = a;
    flBi = (float) (iBi);

    iEXB = ((as_uint(flAi) >> 23) & 0xFF);
    iEYB = ((as_uint(flBi) >> 23) & 0xFF);

    iEX = iEXB - 0x7F;
    iEY = iEYB - 0x7F;

    iSignX = (as_uint(flAi) >> 31);
    iSignY = (as_uint(flBi) >> 31);

    iIsSigZeroX = ((as_uint(flAi) & 0x007FFFFF) == 0);
    iIsSigZeroY = ((as_uint(flBi) & 0x007FFFFF) == 0);

    iYIsFinite = 1;

    iYMantissa = (as_uint(flBi) & 0x007FFFFF);

    iYIsInt = _TestIntI (iBi);

    if (!((iSignX == 0) && (iEXB == 0x7F) && iIsSigZeroX) &&
        !((iEYB == 0) && iIsSigZeroY))
    {
        iXIsFinite =
        (((as_uint(flAi) >> 23) & 0xFF) != 0xFF);

        if ((iXIsFinite || iIsSigZeroX) && (iYIsFinite || iIsSigZeroY))
        {
            if (flAi != ((__constant float *) _vmlsPowHATab)[370])
            {
                if (!((flAi == ((__constant float *) _vmlsPowHATab)[372])
                && (iYIsInt || !iYIsFinite)))
                {
                    if (iXIsFinite && iYIsFinite)
                    {
                        if ((flAi > ((__constant float *) _vmlsPowHATab)[370])
                        || iYIsInt)
                        {
                            flSignRes =
                                ((__constant float *) _vmlsPowHATab)[371 +
                                                (iSignX & iYIsInt)];
                            iDenoExpAdd = 0;
                            flAX = flAi;
                            flAX = as_float(
                            (as_uint(flAX) & 0x7FFFFFFF)
                            | ((uint) (0) << 31));

                            if (iEXB == 0)
                            {
                                flAX =
                                flAX * ((__constant float *) _vmlsPowHATab)[376];
                                iDenoExpAdd = iDenoExpAdd - 64;
                            }

                            flX1 = flAX;
                            flX1 = as_float((as_uint(flX1) & 0x807FFFFF)
                                    | (((uint) (0x7F) & 0xFF) << 23));

                            iXHi = ((as_uint(flAX) >> 23) & 0xFF);
                            iXHi = iXHi << 23;
                            iXHi = iXHi | (as_uint(flAX) & 0x007FFFFF);

                            k = iXHi - 0x3F380000;
                            k = k >> 23;
                            k = k + iDenoExpAdd;

                            i1 = (as_uint(flX1) & 0x007FFFFF);
                            i1 = i1 & 0x780000;
                            i1 = i1 + 0x80000;
                            i1 = i1 >> 20;

                            flRcp1 = ((__constant float *) _vmlsPowHATab)[0 + i1];

                            flL1Hi =
                                ((__constant float *) _vmlsPowHATab)[9 + 2 * (i1) + 0];
                            flL1Lo =
                                ((__constant float *) _vmlsPowHATab)[9 + 2 * (i1) + 1];

                            flX2 = flX1 * flRcp1;

                            i2 = (as_uint(flX2) & 0x007FFFFF);
                            i2 = i2 & 0x1E0000;
                            i2 = i2 + 0x20000;
                            i2 = i2 >> 18;

                            flRcp2 = ((__constant float *) _vmlsPowHATab)[27 + i2];

                            flL2Hi =
                                ((__constant float *) _vmlsPowHATab)[36 + 2 * (i2) +
                                                0];
                            flL2Lo =
                                ((__constant float *) _vmlsPowHATab)[36 + 2 * (i2) +
                                                1];

                            flX3 = (flX2 * flRcp2);

                            i3 =
                                (as_uint(flX3) & 0x007FFFFF);
                            i3 = i3 & 0x7C000;
                            i3 = i3 + 0x4000;
                            i3 = i3 >> 15;

                            flRcp3C = ((__constant float *) _vmlsPowHATab)[54 + i3];

                            flL3Hi =
                                ((__constant float *) _vmlsPowHATab)[71 + 2 * (i3) +
                                                0];
                            flL3Lo =
                                ((__constant float *) _vmlsPowHATab)[71 + 2 * (i3) +
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
                                (flR1 - ((__constant float *) _vmlsPowHATab)[374]);

                            flRcpC = (flRcp1 * flRcp2);
                            flRcpC = (flRcpC * flRcp3C);

                            flVTmp1 =
                                ((flX1) * (((__constant float *) _vmlsPowHATab)[375]));
                            flVTmp2 = (flVTmp1 - (flX1));
                            flVTmp1 = (flVTmp1 - flVTmp2);
                            flVTmp2 = ((flX1) - flVTmp1);
                            flX1Hi = flVTmp1;
                            flX1Lo = flVTmp2;

                            flVTmp1 = ((flRcpC) *
                                (((__constant float *) _vmlsPowHATab)[375]));
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
                                        ((((((__constant float *) _vmlsPowHATab)[364])
                                        * flR +
                                        ((__constant float *) _vmlsPowHATab)[363]) *
                                        flR +
                                        ((__constant float *) _vmlsPowHATab)[362]) *
                                        flR +
                                        ((__constant float *) _vmlsPowHATab)[361]) *
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
                                        (((__constant float *) _vmlsPowHATab)[375]));
                                    flVTmp2 = (flVTmp1 - (flHH));
                                    flVTmp1 = (flVTmp1 - flVTmp2);
                                    flVTmp2 = ((flHH) - flVTmp1);
                                    flHH = flVTmp1;
                                    flHL = flVTmp2;

                                    flVTmp1 =
                                        ((flBi) *
                                        (((__constant float *) _vmlsPowHATab)[375]));
                                    flVTmp2 = (flVTmp1 - (flBi));
                                    flVTmp1 = (flVTmp1 - flVTmp2);
                                    flVTmp2 = ((flBi) - flVTmp1);
                                    flYHi = flVTmp1;
                                    flYLo = flVTmp2;

                                    flYLo += (float) (iBi - (int) flBi);

                                    flTmp1 = ((flYHi) * (flHH));
                                    flTmp2 = ((flYLo) * (flHL));
                                    flTmp2 = (flTmp2 + (flYHi) * (flHL));
                                    flTmp3 = (flTmp2 + (flYLo) * (flHH));
                                    flPH = flTmp1;
                                    flPL = flTmp3;

                                    flPLL = (flBi * flHLL);

                                    flVTmp1 =
                                        (flPH +
                                        ((__constant float *) _vmlsPowHATab)[373]);
                                    flVPHH =
                                        (flVTmp1 -
                                        ((__constant float *) _vmlsPowHATab)[373]);
                                    iN =
                                        (as_uint(flVTmp1) & 0x007FFFFF);
                                    j = iN & 0x7F;

                                    iN = iN << 10;
                                    iN = iN >> (7 + 10);
                                    flVPHL = (flPH - flVPHH);

                                    flZ = (flPLL + flPL);
                                    flZ = (flZ + flVPHL);

                                    flExp2Poly =
                                        (((((__constant float *) _vmlsPowHATab)[367]) *
                                        flZ +
                                        ((__constant float *) _vmlsPowHATab)[366]) *
                                        flZ +
                                        ((__constant float *) _vmlsPowHATab)[365]) *
                                        flZ;

                                    flExp2PolyT =
                                        (flExp2Poly *
                                        ((__constant float *) _vmlsPowHATab)[105 +
                                                        2 * (j) +
                                                        0]);
                                    flResLo =
                                        (flExp2PolyT +
                                        ((__constant float *) _vmlsPowHATab)[105 +
                                                        2 * (j) +
                                                        1]);
                                    flResHi =
                                        ((__constant float *) _vmlsPowHATab)[105 +
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
                                            r = flRes;
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
                                                _vmlsPowHATab)[375]));
                                                flVTmp2 = (flVTmp1 - (flResHi));
                                                flVTmp1 = (flVTmp1 - flVTmp2);
                                                flVTmp2 = ((flResHi) - flVTmp1);
                                                flResHi = flVTmp1;
                                                flTmp2 = flVTmp2;

                                                flResLo = (flResLo + flTmp2);

                                                flSignRes *=
                                                ((__constant float *)
                                                _vmlsPowHATab)[377];
                                                iN = (iN + 64);

                                                flTwoPowN =
                                                ((__constant float *)
                                                _vmlsPowHATab)[371];
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
                                                _vmlsPowHATab)[369];
                                                flVTmp1 = (flVTmp1 * flVTmp1);

                                                flRes = (flRes + flVTmp1);
                                                if (__FlushDenormals)
                                                {
                                                    r = 0.0f;
                                                }
                                                else
                                                {
                                                    r = flRes;
                                                }
                                            }
                                            else
                                            {
                                                if (iERes >= -149 - 10)
                                                {
                                                    flSignRes *=
                                                        ((__constant float *)
                                                        _vmlsPowHATab)[377];
                                                    iN = iN + 64;

                                                    flTwoPowN =
                                                        ((__constant float *)
                                                        _vmlsPowHATab)[371];
                                                        flTwoPowN = as_float(
                                                    (as_uint(flTwoPowN) & 0x807FFFFF) |
                                                    (((uint)
                                                        (iN + 0x7F) & 0xFF) << 23));

                                                    flRes = (flRes * flTwoPowN);
                                                    flRes = (flRes * flSignRes);

                                                    flVTmp1 =
                                                        ((__constant float *)
                                                        _vmlsPowHATab)[369];
                                                    flVTmp1 *= flVTmp1;
                                                    flRes = (flRes + flVTmp1);

                                                    r = flRes;
                                                }
                                                else
                                                {
                                                    flVTmp1 =
                                                        ((__constant float *)
                                                        _vmlsPowHATab)[369];
                                                    flVTmp1 *= flVTmp1;
                                                    flRes =
                                                        (flVTmp1 * flSignRes);
                                                    r = flRes;
                                                }
                                            }
                                        }
                                    }
                                    else
                                    {
                                        flVTmp1 =
                                        ((__constant float *) _vmlsPowHATab)[368];
                                        flVTmp1 = (flVTmp1 * flVTmp1);
                                        flRes = (flVTmp1 * flSignRes);
                                        r = flRes;
                                    }
                                }
                                else
                                {
                                    flVTmp1 =
                                        ((__constant float *) _vmlsPowHATab)[371];
                                    flVTmp1 =
                                        (flVTmp1 +
                                        ((__constant float *) _vmlsPowHATab)[369]);
                                    r = (flVTmp1 * flSignRes);
                                }
                            }
                            else
                            {
                                iSign =
                                iSignY ^ (as_uint(flT_CQHi) >> 31);

                                flTmp1 =
                                ((__constant float *) _vmlsPowHATab)[368 +
                                                (iSign)];

                                flTmp1 = (flTmp1 * flTmp1);

                                flTmp1 = (flTmp1 * flSignRes);
                                r = flTmp1;
                            }
                        }
                        else
                        {
                            flVTmp1 = ((__constant float *) _vmlsPowHATab)[370];
                            flVTmp1 = (flVTmp1 / flVTmp1);
                            r = flVTmp1;
                        }
                    }
                    else
                    {
                        if (iEXB < 0x7F)
                        {
                            if (iSignY)
                            {
                                r = (flBi * flBi);
                            }
                            else
                            {
                                r = ((__constant float *) _vmlsPowHATab)[370];
                            }
                        }
                        else
                        {
                            if (iSignY)
                            {
                                flRes =
                                ((__constant float *) _vmlsPowHATab)[378 +
                                                (iYIsInt &
                                                iSignX)];
                                r = flRes;
                            }
                            else
                            {
                                int iRes = iYIsInt & iSignX;
                                flTmp1 = (flAi * flAi);
                                flTmp1 = (flTmp1 * flBi);
                                flRes =
                                flTmp1 * ((__constant float *) _vmlsPowHATab)[371 +
                                                    (iRes)];
                                r = flRes;
                            }
                        }
                    }
                }
                else
                {
                    r = ((__constant float *) _vmlsPowHATab)[371 + (iYIsInt & 1)];
                }
            }
            else
            {
                flTmp1 = flAi * flAi;

                if (iSignY)
                {
                    r =
                        ((__constant float *) _vmlsPowHATab)[371 +
                                        (iYIsInt & iSignX)] /
                        flTmp1;
                }
                else
                {
                    r =
                        ((__constant float *) _vmlsPowHATab)[371 +
                                        (iYIsInt & iSignX)] *
                        flTmp1;
                }
            }
        }
        else
        {
            r = a + flBi;
        }
    }
    else
    {
        flVTmp1 = flAi + flBi;
        iSign = (as_uint(flVTmp1) >> 31);
        flVTmp2 = ((__constant float *) _vmlsPowHATab)[371];
        flVTmp2 = as_float(
        (as_uint(flVTmp2) & 0x7FFFFFFF) | ((uint) (iSign) << 31));

        r = flVTmp2 * flVTmp2;
    }

    return r;
}

float __ocl_svml_px_pownf1 (float a, int b)
{
    float va1;
    VUINT32 va2;
    float vr1;
    float r;
    VUINT32 vm;

    va1 = a;
    va2 = b;

    {

        float sHiMask;
        float sRSValue;
        float sZ[2];
        float sL[2];
        float sW[2];
        VUINT32 _NMINNORM;
        VUINT32 _NMAXVAL;
        VUINT32 _INF;
        VUINT32 iSpecX;
        VUINT32 iSpecY;
        VUINT32 LFR_iY;
        VUINT32 iRangeMask;

        VUINT32 LFR_iX;
        float LFR_sXMant;
        float LFR_sM;
        VUINT32 LFR_iInd;
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
        VUINT32 LFR_iN;
        float LFR_sN;
        VUINT32 LFR_iXNearOne;
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
        VUINT32 LFR_I_INDEX_MASK;
        VUINT32 LFR_I_INDEX_ADD;
        float S_HI10BITS_MASK;
        float LFR_S_P4;
        float LFR_S_P3;
        float LFR_S_P2;
        VUINT32 I_BIAS;
        VUINT32 LFR_I_NZ_ADD;
        VUINT32 LFR_I_NZ_CMP;
        float S_LOG2_HI;
        float S_LOG2_LO;

        float sN;
        float sR;
        float sP;
        float sM;
        VUINT32 iAbsZ;
        VUINT32 iRes;
        VUINT32 iP;
        VUINT32 iM;
        float sInvLn2;
        float sShifter;
        float sLn2hi;
        float sLn2lo;
        VUINT32 iAbsMask;
        VUINT32 iDomainRange;
        float sPC[6];

        float sX;
        float sY;
        VUINT32 iY;
        float sYLo;
        VUINT32 iYLo;
        VUINT32 iOddY;
        float sOddY;
        float sResultSign;
        float sAbsMask;

        sX = va1;
        sY = ((float) ((VINT32) (va2)));
        {
        int _cvt_iMantMask = 0x007fffff;
        int _cvt_iRightShifter = 0x4b000000;
        int _cvt_iBias = 0x00000096;
        int _cvt_iHighMantBit = 0x00800000;
        int _cvt_i2p23 = 0x4B000000;
        int _cvt_i2p32 = 0x4F800000;
        int _cvt_iNaN = 0x80000000;
        int _cvt_iX, _cvt_iMant, _cvt_iExp, _cvt_iRes1, _cvt_iRes2,
        _cvt_iRes;
        float _cvt_sAbsX, _cvt_sRes;
        float _cvt_sRightShifter = as_float(_cvt_iRightShifter);
        float _cvt_s2p23 = as_float(_cvt_i2p23);
        float _cvt_s2p32 = as_float(_cvt_i2p32);
        _cvt_sAbsX = (sY > 0) ? sY : (-sY);
        _cvt_sRes = _cvt_sAbsX + _cvt_sRightShifter;
        _cvt_iRes1 = as_uint(_cvt_sRes);
        _cvt_iRes1 = _cvt_iRes1 & _cvt_iMantMask;
        _cvt_iX = as_uint(_cvt_sAbsX);
        _cvt_iExp = _cvt_iX >> 23;
        _cvt_iExp = _cvt_iExp - _cvt_iBias;
        _cvt_iMant = _cvt_iX & _cvt_iMantMask;
        _cvt_iMant = _cvt_iMant | _cvt_iHighMantBit;
        _cvt_iRes2 = _cvt_iMant << _cvt_iExp;
        _cvt_iRes = (_cvt_sAbsX < _cvt_s2p23) ? _cvt_iRes1 : _cvt_iRes2;
        _cvt_iRes = (_cvt_sAbsX < _cvt_s2p32) ? _cvt_iRes : _cvt_iNaN;
        _cvt_iRes = (sY > 0) ? _cvt_iRes : (-_cvt_iRes);
        iY = _cvt_iRes;
        };
        iYLo = (va2 - iY);
        sYLo = ((float) ((VINT32) (iYLo)));

        iOddY = ((VUINT32) va2 << 31);
        sOddY = as_float(iOddY);
        sResultSign = as_float(as_uint(va1) & as_uint(sOddY));

        iAbsMask = as_uint(__ocl_svml_spow_data._iAbsMask);
        sAbsMask = as_float(iAbsMask);
        sX = as_float(as_uint(sX) & as_uint(sAbsMask));

        (LFR_iX) = as_uint(sX);
        (LFR_iY) = as_uint(sY);

        _NMINNORM = as_uint(__ocl_svml_spow_data.NMINNORM);
        _NMAXVAL = as_uint(__ocl_svml_spow_data.NMAXVAL);
        _INF = as_uint(__ocl_svml_spow_data.INF);

        iSpecX = (LFR_iX - _NMINNORM);
        iSpecX =
        ((VUINT32) (-(VSINT32) ((VSINT32) iSpecX >= (VSINT32) _NMAXVAL)));
        iSpecY = (LFR_iY & iAbsMask);
        iSpecY =
        ((VUINT32) (-(VSINT32) ((VSINT32) iSpecY >= (VSINT32) _INF)));
        iRangeMask = (iSpecX | iSpecY);

        LFR_I_NZ_ADD =
        as_uint(__ocl_svml_spow_data.LFR_I_NZ_ADD);
        LFR_iXNearOne = (LFR_iX + LFR_I_NZ_ADD);
        LFR_I_NZ_CMP =
        as_uint(__ocl_svml_spow_data.LFR_I_NZ_CMP);
        LFR_iXNearOne =
        ((VUINT32)
        (-(VSINT32) ((VSINT32) LFR_iXNearOne > (VSINT32) LFR_I_NZ_CMP)));
        LFR_sXNearOne = as_float(LFR_iXNearOne);
        S_MANT_MASK = as_float(__ocl_svml_spow_data.S_MANT_MASK);
        LFR_sXMant = as_float(as_uint(sX) & as_uint(S_MANT_MASK));
        S_ONE = as_float(__ocl_svml_spow_data.S_ONE);
        LFR_sM = as_float(as_uint(LFR_sXMant) | as_uint(S_ONE));
        LFR_iN = ((VUINT32) LFR_iX >> 23);
        I_BIAS = as_uint(__ocl_svml_spow_data.I_BIAS);
        LFR_iN = (LFR_iN - I_BIAS);
        LFR_sN = ((float) ((VINT32) (LFR_iN)));

        LFR_I_INDEX_MASK =
        as_uint(__ocl_svml_spow_data.LFR_I_INDEX_MASK);
        LFR_iInd = (LFR_iX & LFR_I_INDEX_MASK);

        LFR_I_INDEX_ADD =
        as_uint(__ocl_svml_spow_data.LFR_I_INDEX_ADD);
        LFR_iInd = (LFR_iInd + LFR_I_INDEX_ADD);

        LFR_iInd = ((VUINT32) LFR_iInd >> 17);
        {
            __constant char *_vlt_pPtr_[1];;
            {
                VUINT32 _vlt_sIndex_;
                VUINT32 _vlt_nIndex_;;
                {
                    VUINT32 _vsc_op1_tmp_;
                    _vsc_op1_tmp_ = ((VUINT32) LFR_iInd << 1);
                    _vsc_op1_tmp_ = (_vsc_op1_tmp_ + LFR_iInd);
                    _vlt_sIndex_ = ((VUINT32) _vsc_op1_tmp_ << 2);
                };
                _vlt_nIndex_ = _vlt_sIndex_;
                _vlt_pPtr_[0] =
                ((__constant char *) (__ocl_svml_spow_data.LFR_TBL)) + ((0) * (3 * 4)) +
                _vlt_nIndex_;
            }
            LFR_sLnRcprYHi = ((__constant float *) (_vlt_pPtr_[0]))[0];
            LFR_sLnRcprYLo = ((__constant float *) (_vlt_pPtr_[0]))[1];
            LFR_sRcprY = ((__constant float *) (_vlt_pPtr_[0]))[2];
        }

        S_HI10BITS_MASK =
        as_float(__ocl_svml_spow_data.S_HI10BITS_MASK);
        LFR_sYHi = as_float(as_uint(LFR_sM) & as_uint(S_HI10BITS_MASK));
        LFR_sYLo = (LFR_sM - LFR_sYHi);

        LFR_sYHiRcpY = (LFR_sYHi * LFR_sRcprY);
        LFR_sRHi = (LFR_sYHiRcpY - S_ONE);
        LFR_sTRHi = (LFR_sRHi + LFR_sLnRcprYHi);
        LFR_sRLo = (LFR_sYLo * LFR_sRcprY);
        LFR_sR = (LFR_sRHi + LFR_sRLo);

        LFR_S_P4 = as_float(__ocl_svml_spow_data.LFR_S_P4);
        LFR_S_P3 = as_float(__ocl_svml_spow_data.LFR_S_P3);
        LFR_sP = ((LFR_S_P4 * LFR_sR) + LFR_S_P3);

        LFR_S_P2 = as_float(__ocl_svml_spow_data.LFR_S_P2);
        LFR_sP = ((LFR_sP * LFR_sR) + LFR_S_P2);

        LFR_sR2 = (LFR_sR * LFR_sR);
        LFR_sP = (LFR_sP * LFR_sR2);

        S_LOG2_HI = as_float(__ocl_svml_spow_data.S_LOG2_HI);
        LFR_sNLog2Hi = (LFR_sN * S_LOG2_HI);

        S_LOG2_LO = as_float(__ocl_svml_spow_data.S_LOG2_LO);
        LFR_sNLog2Lo = (LFR_sN * S_LOG2_LO);

        LFR_sResHi = (LFR_sNLog2Hi + LFR_sTRHi);
        LFR_sWLo = (LFR_sNLog2Lo + LFR_sLnRcprYLo);
        LFR_sResLo = (LFR_sP + LFR_sWLo);
        LFR_alfa = as_float(as_uint(LFR_sXNearOne) & as_uint(LFR_sRLo));
        sL[0] = (LFR_sResHi + LFR_alfa);
        LFR_beta = as_float((~as_uint(LFR_sXNearOne)) & as_uint(LFR_sRLo));
        sL[1] = (LFR_sResLo + LFR_beta);

        sRSValue = as_float(__ocl_svml_spow_data.sRSValue);
        sHiMask = as_float(__ocl_svml_spow_data.sHiMask);
        {
            float V1;
            float V2;
            V1 = (sL[0] + sL[1]);
            V2 = (V1 * sRSValue);
            V1 = (V1 + V2);
            V2 = (V1 - V2);
            V1 = (sL[0] - V2);
            V1 = (sL[1] + V1);
            sL[0] = V2;
            sL[1] = V1;
        }

        {
            float V1;
            float V2;;
            V1 = (sY * sRSValue);
            V2 = (V1 - sY);
            V1 = (V1 - V2);
            V2 = (sY - V1);
            sW[0] = V1;
            sW[1] = V2;
        }
        sW[1] = (sW[1] + sYLo);

        {
            float V1;
            float V2;
            V1 = (sL[0] * sW[0]);
            V2 = (sL[1] * sW[1]);
            V2 = ((sL[0] * sW[1]) + V2);
            V2 = ((sL[1] * sW[0]) + V2);;
            sZ[0] = V1;
            sZ[1] = V2;
        }

        sInvLn2 = as_float(__ocl_svml_spow_data._sInvLn2);
        sShifter = as_float(__ocl_svml_spow_data._sShifter);
        sM = ((sZ[0] * sInvLn2) + sShifter);
        sN = (sM - sShifter);

        iAbsZ = as_uint(sZ[0]);
        iAbsZ = (iAbsZ & iAbsMask);
        iDomainRange =
        as_uint(__ocl_svml_spow_data._iDomainRange);
        iAbsZ =
        ((VUINT32)
        (-(VSINT32) ((VSINT32) iAbsZ > (VSINT32) iDomainRange)));
        iRangeMask = (iRangeMask | iAbsZ);
        vm = 0;
        vm |= (((VUINT32) iRangeMask >> 31) & 1);

        iM = as_uint(sM);
        iM = ((VUINT32) iM << 23);

        sLn2hi = as_float(__ocl_svml_spow_data._sLn2hi);
        float sR_0 = sN * sLn2hi;
        sR = (sZ[0] - sR_0);
        sLn2lo = as_float(__ocl_svml_spow_data._sLn2lo);
        sR = (sR - (sN * sLn2lo));
        sR = (sR + sZ[1]);

        sPC[4] = as_float(__ocl_svml_spow_data._sPC4);
        sPC[5] = as_float(__ocl_svml_spow_data._sPC5);
        sP = ((sPC[5] * sR) + sPC[4]);
        sPC[3] = as_float(__ocl_svml_spow_data._sPC3);
        sP = ((sP * sR) + sPC[3]);
        sPC[2] = as_float(__ocl_svml_spow_data._sPC2);
        sP = ((sP * sR) + sPC[2]);
        sPC[1] = as_float(__ocl_svml_spow_data._sPC1);
        sP = ((sP * sR) + sPC[1]);
        sPC[0] = as_float(__ocl_svml_spow_data._sPC0);
        sP = ((sP * sR) + sPC[0]);

        iP = as_uint(sP);
        iRes = (iM + iP);
        vr1 = as_float(iRes);

        vr1 = as_float(as_uint(vr1) | as_uint(sResultSign));

    }

    if ((vm & 0x00000001) != 0)
    {
        vr1 = __ocl_svml_spown_cout_rare (va1, va2);
    }
    r = vr1;

    return r;
}

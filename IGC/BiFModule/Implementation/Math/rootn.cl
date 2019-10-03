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

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

#include "../SVMLReleaseOnly/svml/Math/svml_rootn.cl"

float __builtin_spirv_OpenCL_rootn_f32_i32( float x, int n )
{
    float result;

    if(__FastRelaxedMath && (!__APIRS))
    {
        // Defined for x > 0 and n is nonzero.  Derived
        // implementations implement this as:
        //   exp2(log2( x) / n) for x > 0.
        // Defined for x < 0 and n is odd.  Derived
        // implementations implement this as:
        //  -exp2(log2(-x) / n) for x < 0.
        // Defined as +0 for x = +/-0 and y > 0.
        // Undefined for all other cases.

        float   pr = x;

        // TBD: Which is faster?
        // Note that USC has a pattern match optimization to turn
        // log-mul-exp into pow.  Additionally, there are some specific
        // LLVM optimizations for pow.  So, preferring pow for now.
#if 0
        pr = __builtin_spirv_OpenCL_fabs_f32( pr );
        pr = __builtin_spirv_OpenCL_log2_f32( pr );
        pr = pr * 1.0f / n;
        pr = __builtin_spirv_OpenCL_exp2_f32( pr );
#else
        pr = __builtin_spirv_OpenCL_native_powr_f32_f32( pr, 1.0f / n );
#endif

        // For rootn(), we'll return the positive result for both +0.0f and -0.0f.
        float nr = -pr;
        result = ( x >= 0.0f ) ? pr : nr;   // positive result for non-negative x, else negative result
    }
    else
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
            float _vapi_res1[1];;
            ((private float *) _vapi_arg1)[0] = va1;
            ((private uint *) _vapi_arg2)[0] = va2;
            ((private float *) _vapi_res1)[0] = vr1;
            __ocl_svml_srootn_cout_rare(_vapi_arg1, _vapi_arg2, _vapi_res1);
            vr1 = ((private const float *) _vapi_res1)[0];;
        }
        result = vr1;
    }
    return result;
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpenCL_rootn, float, float, int, f32, i32 )

#if defined(cl_khr_fp64)

INLINE double __builtin_spirv_OpenCL_rootn_f64_i32( double y, int x )
{
    return __ocl_svml_rf_rootn1(y, x);
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpenCL_rootn, double, double, int, f64, i32 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_rootn_f16_i32( half y, int x )
{
    return __builtin_spirv_OpenCL_rootn_f32_i32((float)y, x);
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpenCL_rootn, half, half, int, f16, i32 )

#endif // defined(cl_khr_fp16)

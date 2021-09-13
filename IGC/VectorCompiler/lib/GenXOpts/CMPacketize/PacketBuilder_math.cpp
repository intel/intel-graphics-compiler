/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "PacketBuilder.h"

// need to disable this to use INFINITY and NAN values
#pragma warning(disable : 4756 4056)

//#include <math.h>

namespace pktz
{
    //////////////////////////////////////////////////////////////////////////
    /// @brief Computes log2(a) using either scalar log2 function from the runtime
    ///        or vector approximation
    /// @param a - src float vector
    Value* PacketBuilder::VLOG2PS(Value* a)
    {
        Value* result;

        // fast log2 approximation
        // log2(x) = (x.ExpPart - 127) + log(1.xFracPart)
        Value* asInt        = BITCAST(a, mSimdInt32Ty);
        Value* b            = SUB(AND(ASHR(asInt, 23), 255), VIMMED1(127));
        Value* intermResult = SI_TO_FP(b, mSimdFP32Ty);

        Value* fa = OR(AND(asInt, VIMMED1(0x007FFFFF)), VIMMED1(127 << 23));
        fa        = BITCAST(fa, mSimdFP32Ty);
        fa        = FSUB(fa, VIMMED1(1.0f));

        // log(x) = (1.4386183024320163f + (-0.640238532500937f +
        // 0.20444600983623412f*fx)*fx)*fx;
        result = FMUL(fa, VIMMED1(0.20444600983623412f));
        result = FADD(result, VIMMED1(-0.640238532500937f));
        result = FMUL(fa, result);
        result = FADD(result, VIMMED1(1.4386183024320163f));
        result = FMUL(result, fa);
        result = FADD(result, intermResult);

        // handle bad input
        // 0 -> -inf
        Value* zeroInput = FCMP_OEQ(a, VIMMED1(0.0f));
        result           = SELECT(zeroInput, VIMMED1(-INFINITY), result);

        // -F -> NAN
        Value* negInput = FCMP_OLT(a, VIMMED1(0.0f));
        result          = SELECT(negInput, VIMMED1(NAN), result);

        // inf -> inf
        Value* infInput = FCMP_OEQ(a, VIMMED1(INFINITY));
        result          = SELECT(infInput, VIMMED1(INFINITY), result);

        // NAN -> NAN
        Value* nanInput = FCMP_UNO(a, a);
        result          = SELECT(nanInput, VIMMED1(NAN), result);

        result->setName("log2.");
        return result;
    }

    //////////////////////////////////////////////////////////////////////////
    /// @brief Computes a^2.4 using either scalar pow function from the runtime
    ///        or vector approximation
    /// @param a - src float vector
    Value* PacketBuilder::VPOW24PS(Value* a)
    {
        Value* result;
        // approximation algorithm from
        // http://stackoverflow.com/questions/6475373/optimizations-for-pow-with-const-non-integer-exponent
        // computes a^2.4 with approximately 5% overestimate.
        // can reduce the error further with a few more terms

        const float expnum   = 24;
        const float expden   = 10;
        const float coeffnum = 1.0f;
        const float coeffden = 1.0f;

        Value* correctionFactor =
            VIMMED1(exp2f(127.f * expden / expnum - 127.f) *
                    powf(1.f * coeffnum / coeffden, 1.0f * expden / expnum));

        result = FMUL(a, correctionFactor);
        result = SI_TO_FP(BITCAST(result, mSimdInt32Ty), mSimdFP32Ty);
        result = FMUL(result, VIMMED1(1.f * expnum / expden));
        result = BITCAST(FP_TO_SI(result, mSimdInt32Ty), mSimdFP32Ty);

        result->setName("pow24.");
        return result;
    }

#define EXP_POLY_DEGREE 3

#define POLY0(x, c0) VIMMED1(c0)
#define POLY1(x, c0, c1) FADD(FMUL(POLY0(x, c1), x), VIMMED1(c0))
#define POLY2(x, c0, c1, c2) FADD(FMUL(POLY1(x, c1, c2), x), VIMMED1(c0))
#define POLY3(x, c0, c1, c2, c3) FADD(FMUL(POLY2(x, c1, c2, c3), x), VIMMED1(c0))
#define POLY4(x, c0, c1, c2, c3, c4) FADD(FMUL(POLY3(x, c1, c2, c3, c4), x), VIMMED1(c0))
#define POLY5(x, c0, c1, c2, c3, c4, c5) FADD(FMUL(POLY4(x, c1, c2, c3, c4, c5), x), VIMMED1(c0))

    //////////////////////////////////////////////////////////////////////////
    /// @brief Computes 2^x using either scalar pow function from the runtime
    ///        or vector approximation
    /// @param a - src float vector
    Value* PacketBuilder::VEXP2PS(Value* a)
    {
        Value* result;

        // fast exp2 taken from here:
        // http://jrfonseca.blogspot.com/2008/09/fast-sse2-pow-tables-or-polynomials.html

        a = VMINPS(a, VIMMED1(129.0f));
        a = VMAXPS(a, VIMMED1(-126.99999f));

        Value* ipart    = FP_TO_SI(FSUB(a, VIMMED1(0.5f)), mSimdInt32Ty);
        Value* fpart    = FSUB(a, SI_TO_FP(ipart, mSimdFP32Ty));
        Value* expipart = BITCAST(SHL(ADD(ipart, VIMMED1(127)), 23), mSimdFP32Ty);
#if EXP_POLY_DEGREE == 5
        Value* expfpart = POLY5(fpart,
                                9.9999994e-1f,
                                6.9315308e-1f,
                                2.4015361e-1f,
                                5.5826318e-2f,
                                8.9893397e-3f,
                                1.8775767e-3f);
#endif

#if EXP_POLY_DEGREE == 4
        Value* expfpart = POLY4(
            fpart, 1.0000026f, 6.9300383e-1f, 2.4144275e-1f, 5.2011464e-2f, 1.3534167e-2f);
#endif

#if EXP_POLY_DEGREE == 3
        Value* expfpart =
            POLY3(fpart, 9.9992520e-1f, 6.9583356e-1f, 2.2606716e-1f, 7.8024521e-2f);
#endif

#if EXP_POLY_DEGREE == 2
        Value* expfpart = POLY2(fpart, 1.0017247f, 6.5763628e-1f, 3.3718944e-1f);
#endif

        result = FMUL(expipart, expfpart, "exp2.");

        return result;
    }
}

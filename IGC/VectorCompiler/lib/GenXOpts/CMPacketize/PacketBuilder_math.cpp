/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "PacketBuilder.h"

// need to disable this to use INFINITY and NAN values
#pragma warning(disable : 4756 4056)

//#include <math.h>

namespace pktz {
//////////////////////////////////////////////////////////////////////////
/// @brief Computes log2(A) using either scalar log2 function from the runtime
///        or vector approximation
/// @param A - src float vector
Value *PacketBuilder::VLOG2PS(Value *A) {
  Value *Result = nullptr;
  // fast log2 approximation
  // log2(x) = (x.ExpPart - 127) + log(1.xFracPart)
  auto *AsInt = BITCAST(A, SimdInt32Ty);
  auto *B = SUB(AND(ASHR(AsInt, 23), 255), VIMMED1(127));
  auto *IntermResult = SI_TO_FP(B, SimdFP32Ty);
  auto *Fa = OR(AND(AsInt, VIMMED1(0x007FFFFF)), VIMMED1(127 << 23));
  Fa = BITCAST(Fa, SimdFP32Ty);
  Fa = FSUB(Fa, VIMMED1(1.0f));
  // log(x) = (1.4386183024320163f + (-0.640238532500937f +
  // 0.20444600983623412f*fx)*fx)*fx;
  Result = FMUL(Fa, VIMMED1(0.20444600983623412f));
  Result = FADD(Result, VIMMED1(-0.640238532500937f));
  Result = FMUL(Fa, Result);
  Result = FADD(Result, VIMMED1(1.4386183024320163f));
  Result = FMUL(Result, Fa);
  Result = FADD(Result, IntermResult);
  // handle bad input
  // 0 -> -inf
  auto *ZeroInput = FCMP_OEQ(A, VIMMED1(0.0f));
  Result = SELECT(ZeroInput, VIMMED1(-INFINITY), Result);
  // -F -> NAN
  auto *NegInput = FCMP_OLT(A, VIMMED1(0.0f));
  Result = SELECT(NegInput, VIMMED1(NAN), Result);
  // inf -> inf
  auto *InfInput = FCMP_OEQ(A, VIMMED1(INFINITY));
  Result = SELECT(InfInput, VIMMED1(INFINITY), Result);
  // NAN -> NAN
  auto *NanInput = FCMP_UNO(A, A);
  Result = SELECT(NanInput, VIMMED1(NAN), Result);
  Result->setName("log2.");
  return Result;
}

//////////////////////////////////////////////////////////////////////////
/// @brief Computes A^2.4 using either scalar pow function from the runtime
///        or vector approximation
/// @param A - src float vector
Value *PacketBuilder::VPOW24PS(Value *A) {
  Value *Result = nullptr;
  // approximation algorithm from
  // http://stackoverflow.com/questions/6475373/optimizations-for-pow-with-const-non-integer-exponent
  // computes a^2.4 with approximately 5% overestimate.
  // can reduce the error further with a few more terms
  const float EXPNUM = 24;
  const float EXPDEN = 10;
  const float COEFFNUM = 1.0f;
  const float COEFFDEN = 1.0f;
  auto *CorrectionFactor =
      VIMMED1(exp2f(127.f * EXPDEN / EXPNUM - 127.f) *
              powf(1.f * COEFFNUM / COEFFDEN, 1.0f * EXPDEN / EXPNUM));
  Result = FMUL(A, CorrectionFactor);
  Result = SI_TO_FP(BITCAST(Result, SimdInt32Ty), SimdFP32Ty);
  Result = FMUL(Result, VIMMED1(1.f * EXPNUM / EXPDEN));
  Result = BITCAST(FP_TO_SI(Result, SimdInt32Ty), SimdFP32Ty);
  Result->setName("pow24.");
  return Result;
}

#define EXP_POLY_DEGREE 3

#define POLY0(x, c0) VIMMED1(c0)
#define POLY1(x, c0, c1) FADD(FMUL(POLY0(x, c1), x), VIMMED1(c0))
#define POLY2(x, c0, c1, c2) FADD(FMUL(POLY1(x, c1, c2), x), VIMMED1(c0))
#define POLY3(x, c0, c1, c2, c3)                                               \
  FADD(FMUL(POLY2(x, c1, c2, c3), x), VIMMED1(c0))
#define POLY4(x, c0, c1, c2, c3, c4)                                           \
  FADD(FMUL(POLY3(x, c1, c2, c3, c4), x), VIMMED1(c0))
#define POLY5(x, c0, c1, c2, c3, c4, c5)                                       \
  FADD(FMUL(POLY4(x, c1, c2, c3, c4, c5), x), VIMMED1(c0))

//////////////////////////////////////////////////////////////////////////
/// @brief Computes 2^A using either scalar pow function from the runtime
///        or vector approximation
/// @param A - src float vector
Value *PacketBuilder::VEXP2PS(Value *A) {
  Value *Result = nullptr;
  // fast exp2 taken from here:
  // http://jrfonseca.blogspot.com/2008/09/fast-sse2-pow-tables-or-polynomials.html
  A = VMINPS(A, VIMMED1(129.0f));
  A = VMAXPS(A, VIMMED1(-126.99999f));
  auto *IPart = FP_TO_SI(FSUB(A, VIMMED1(0.5f)), SimdInt32Ty);
  auto *FPart = FSUB(A, SI_TO_FP(IPart, SimdFP32Ty));
  auto *ExpIPart = BITCAST(SHL(ADD(IPart, VIMMED1(127)), 23), SimdFP32Ty);
#if EXP_POLY_DEGREE == 5
  auto *ExpFPart = POLY5(FPart, 9.9999994e-1f, 6.9315308e-1f, 2.4015361e-1f,
                         5.5826318e-2f, 8.9893397e-3f, 1.8775767e-3f);
#elif EXP_POLY_DEGREE == 4
  auto *ExpFPart = POLY4(FPart, 1.0000026f, 6.9300383e-1f, 2.4144275e-1f,
                         5.2011464e-2f, 1.3534167e-2f);
#elif EXP_POLY_DEGREE == 3
  auto *ExpFPart =
      POLY3(FPart, 9.9992520e-1f, 6.9583356e-1f, 2.2606716e-1f, 7.8024521e-2f);
#elif EXP_POLY_DEGREE == 2
  auto *ExpFPart = POLY2(FPart, 1.0017247f, 6.5763628e-1f, 3.3718944e-1f);
#endif // EXP_POLY_DEGREE
  Result = FMUL(ExpIPart, ExpFPart, "exp2.");
  return Result;
}
} // namespace pktz

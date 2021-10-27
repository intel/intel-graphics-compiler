/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include <cm-cl/math.h>
#include <cm-cl/vector.h>
using namespace cm;

namespace details {

template <unsigned N>
static CM_NODEBUG CM_INLINE vector<uint64_t, N>
__impl_combineLoHi(vector<uint32_t, N> Lo, vector<uint32_t, N> Hi) {
  vector<uint32_t, 2 * N> Res;
  Res.template select<N, 2>(1) = Hi;
  Res.template select<N, 2>(0) = Lo;
  return Res.template format<uint64_t>();
}

template <unsigned N>
CM_NODEBUG CM_INLINE vector<double, N>
__impl_ui2fp__double__(vector<uint64_t, N> a) {
  const vector<uint32_t, N> Zero(0);
  const vector<uint32_t, N> Ones(0xffffffff);
  const vector<uint32_t, N> One(1);

  vector<uint32_t, 2 *N> LoHi = a.template format<uint32_t>();
  vector<uint32_t, N> Lo = LoHi.template select<N, 2>(0);
  vector<uint32_t, N> Hi = LoHi.template select<N, 2>(1);
  // TODO : replace with clz for 64 bit
  vector<uint32_t, N> LZHi = cm::math::count_leading_zeros(Hi);
  vector<uint32_t, N> LZLo = cm::math::count_leading_zeros(Lo);
  auto ZeroHi = Hi == Zero;
  vector<uint32_t, N> LZ = LZHi;
  LZ.merge(LZ + LZLo, ZeroHi);

  // we need to get that nice first set bit into bit position 51.
  // thus we shift our nice pair of values by 63 - 51 - clz,
  // uint8_t shift = 12 - lz;
  // shift hidden bit too (+1)

  // 64bit Shift - we rely on compiler emulation there
  vector<int32_t, N> Shift = LZ - vector<int32_t, N>(11);
  vector<uint64_t, N> ToShift = __impl_combineLoHi<N>(Lo, Hi);
  vector<uint64_t, N> Shifted64 = ToShift << Shift;
  auto IsRightShift = Shift < vector<int32_t, N>(0);
  Shifted64.merge(ToShift >> -Shift, IsRightShift);

  vector<uint32_t, 2 *N> Shifted = Shifted64.template format<uint32_t>();
  vector<uint32_t, N> LoMant = Shifted.template select<N, 2>(0);
  vector<uint32_t, N> HiMant = Shifted.template select<N, 2>(1);
  // delete hidden bit
  HiMant = HiMant & ~(1u << 20);

  // calculate RS
  vector<uint32_t, N> RMask = (One << (10 - LZ));
  vector<uint32_t, N> R = (RMask & Lo) >> (10 - LZ);
  auto NoR = LZ > vector<uint32_t, N>(10);
  R.merge(Zero, NoR);

  vector<uint32_t, N> SMask = RMask - 1;
  vector<uint32_t, N> S = Zero;
  vector<uint32_t, N> AfterR = Lo & SMask;
  auto ZeroRem = AfterR == Zero;
  S.merge(One, ~ZeroRem);
  auto NoS = LZ > vector<uint32_t, N>(9);
  S.merge(Zero, NoS);

  // R is set but no S, round to even.
  // Mant + R
  auto AddC = cm::math::add_with_carry(LoMant, R);
  LoMant = AddC.first;
  vector<uint32_t, N> CB = AddC.second;
  HiMant = HiMant + CB;

  LoMant &= ~(~S & R);

  vector<uint32_t, N> Exp = vector<uint32_t, N>(1086) - LZ;
  Exp.merge(Zero, LZ == vector<uint32_t, N>(64));
  vector<uint32_t, N> HiRes = Exp << vector<uint32_t, N>(20);
  HiRes += HiMant;
  vector<uint32_t, N> LoRes = LoMant;

  vector<int64_t, N> Result = __impl_combineLoHi<N>(LoRes, HiRes);

  return Result.template format<double>();
};

template <unsigned N>
CM_NODEBUG CM_INLINE vector<float, N> __impl_ui2fp__(vector<uint64_t, N> a) {
  const vector<uint32_t, N> Zero(0);
  const vector<uint32_t, N> Ones(0xffffffff);
  const vector<uint32_t, N> One(1);

  vector<uint32_t, 2 *N> LoHi = a.template format<uint32_t>();

  vector<uint32_t, N> Lo = LoHi.template select<N, 2>(0);
  vector<uint32_t, N> Hi = LoHi.template select<N, 2>(1);
  vector<uint32_t, N> LZ = cm::math::count_leading_zeros(Hi);

  // we need to get that nice first set bit into bit position 23.
  // thus we shift our nice pair of values by 63 - 23 - clz,
  // some bits will be dropped by shift thus we'll add 1 bits as R bit.
  // uint8_t shift = 39 - lz;

  vector<uint32_t, N> DroppedBits = vector<uint32_t, N>(39) - LZ;
  // SI
  vector<uint32_t, N> Sha = DroppedBits & vector<uint32_t, N>(0x3f);
  vector<uint32_t, N> Vec32 = vector<int32_t, N>(32);
  vector<uint32_t, N> Sh32 = Vec32 - Sha;
  auto Flag_large_shift = (Sha >= Vec32);
  auto Flag_zero_shift = (Sha == Zero);
  vector<uint32_t, N> Mask1 = Ones;
  Mask1.merge(Zero, Flag_large_shift);
  vector<uint32_t, N> Mask0 = Ones;
  Mask0.merge(Zero, Flag_zero_shift);

  // partial shift
  vector<uint32_t, N> TmpH1 = ((Hi & Mask0) << Sh32) & Mask1;
  vector<uint32_t, N> TmpH2 = (Hi >> (Sha - Vec32)) & ~Mask1;
  vector<uint32_t, N> TmpL = (Lo >> Sha) & Mask1;
  vector<uint32_t, N> Mant = TmpL | TmpH1 | TmpH2;

  vector<uint32_t, N> TmpSha = One << (-Sh32);
  vector<uint32_t, N> TmpMask = TmpSha - One;
  vector<uint32_t, N> StickyH = Hi & ~Mask1;
  StickyH = StickyH & TmpMask;

  // calculate RS
  vector<uint32_t, N> L1 = Lo & ~Mask1;
  vector<uint32_t, N> L2 = Lo & (Mask1 >> Sh32);
  vector<uint32_t, N> StickyL = L1 | L2;
  vector<uint32_t, N> S1 = StickyH | StickyL;
  auto S = S1 == Zero;
  vector<uint32_t, N> NotS = Zero;
  NotS.merge(Ones, S);

  // R is set but no S, round to even.
  vector<uint32_t, N> R = Mant & One;
  Mant = (Mant + One) >> One;
  Mant &= ~(NotS & R);

  vector<uint32_t, N> Exp = vector<uint32_t, N>(0xbd) - LZ;
  vector<uint32_t, N> ResL = Exp << vector<uint32_t, N>(23);
  ResL += Mant;

  vector<float, N> ResultLarge = ResL.template format<float>();
  vector<float, N> ResultSmall = Lo;

  auto IsSmallPred = Hi == Zero;

  vector<float, N> Result = ResultLarge;
  Result.merge(ResultSmall, IsSmallPred);

  return Result;
}

template <unsigned N>
CM_NODEBUG CM_INLINE vector<half, N>
__impl_ui2fp__half__(vector<uint64_t, N> a) {
  const vector<uint32_t, N> Zero(0);
  const vector<uint32_t, N> Ones(0xffffffff);

  vector<uint32_t, 2 *N> LoHi = a.template format<uint32_t>();
  vector<uint32_t, N> Lo = LoHi.template select<N, 2>(0);
  vector<uint32_t, N> Hi = LoHi.template select<N, 2>(1);
  // max half value is 65504 (0xffe0)
  // so we can use only Low Part
  vector<half, N> Res = Lo;
  // NoZeroHi - should be overflow
  auto NoZeroHi = Hi != Zero;
  Res.merge(vector<half, N>(Ones), NoZeroHi);
  return Res;
};

template <typename T, unsigned N> class __impl_ui2fp_runner {};

template <unsigned N> class __impl_ui2fp_runner<float, N> {
public:
  vector<float, N> run(vector<uint64_t, N> arg) {
    return __impl_ui2fp__<N>(arg);
  }
};
template <unsigned N> class __impl_ui2fp_runner<double, N> {
public:
  vector<double, N> run(vector<uint64_t, N> arg) {
    return __impl_ui2fp__double__<N>(arg);
  }
};
template <unsigned N> class __impl_ui2fp_runner<half, N> {
public:
  vector<half, N> run(vector<uint64_t, N> arg) {
    return __impl_ui2fp__half__<N>(arg);
  }
};

template <typename T, unsigned N>
CM_NODEBUG CM_INLINE vector<float, N> __impl_si2fp__(vector<uint64_t, N> a) {
  const vector<uint32_t, N> Zero(0);

  // NOTE: SIToFP is special, since it does not do the convert by itself,
  // Instead it just creates a sequence of 64.bit operations which
  // are then expanded. As such some type convertion trickery is involved.
  vector<uint32_t, 2 *N> LoHi = a.template format<uint32_t>();
  vector<uint32_t, N> Lo = LoHi.template select<N, 2>(0);
  vector<uint32_t, N> Hi = LoHi.template select<N, 2>(1);
  vector<uint32_t, N> SB = Hi & vector<uint32_t, N>(1u << 31);
  auto IsSignZero = SB == Zero;
  vector<uint64_t, N> b = -a;
  b.merge(a, IsSignZero);
  auto Res = __impl_ui2fp_runner<T, N>().run(b);
  Res.merge(-Res, ~IsSignZero);
  return Res;
}

template <unsigned N, bool isSigned>
CM_NODEBUG CM_INLINE vector<uint64_t, N>
__impl_fp2ui__double__(vector<double, N> a) {
  // vector of floats -> vector of ints
  vector<uint32_t, 2 *N> LoHi = a.template format<uint32_t>();
  const vector<uint32_t, N> MantissaMask((1u << 20) - 1);
  const vector<uint32_t, N> ExpMask(0x7ff);
  const vector<uint32_t, N> Zero(0);
  const vector<uint32_t, N> Ones(0xffffffff);
  const vector<uint32_t, N> One(1);
  vector<uint32_t, N> Lo = LoHi.template select<N, 2>(0);
  vector<uint32_t, N> Hi = LoHi.template select<N, 2>(1);
  vector<uint32_t, N> Exp = (Hi >> 20) & ExpMask;
  // mantissa without hidden bit
  vector<uint32_t, N> LoMant = Lo;
  vector<uint32_t, N> HiMant = Hi & MantissaMask;
  // for normalized numbers (1 + mant/2^52) * 2 ^ (mant-1023)
  vector<int32_t, N> MantShift = Exp - 1023 - 52;
  vector<int32_t, N> OneShift = Exp - 1023;
  auto IsNormalized = Exp == Zero;
  // for denormalized numbers (0 + mant/2^52) * 2 ^ (1-1023)
  MantShift.merge(One - 1023 + 1, IsNormalized);
  OneShift.merge(Ones, IsNormalized);

  // 64bit Shift - we rely on compiler emulation there
  vector<uint64_t, N> ToShift = __impl_combineLoHi<N>(LoMant, HiMant);
  vector<uint64_t, N> Shifted64 = ToShift << MantShift;
  auto IsRightShift = MantShift < vector<int32_t, N>(0);
  Shifted64.merge(ToShift >> -MantShift, IsRightShift);
  auto IsShiftGT64 = (MantShift >= vector<int32_t, N>(64)) |
                     (MantShift <= vector<int32_t, N>(-64));
  Shifted64.merge(vector<uint64_t, N>(0), IsShiftGT64);
  vector<uint32_t, 2 *N> Shifted = Shifted64.template format<uint32_t>();
  vector<uint32_t, N> LoRes = Shifted.template select<N, 2>(0);
  vector<uint32_t, N> HiRes = Shifted.template select<N, 2>(1);

  // add hidden One
  vector<uint32_t, N> OneInLo = One << OneShift;
  auto IsOneInLo =
      (OneShift < vector<int32_t, N>(32)) & (OneShift >= vector<int32_t, N>(0));
  LoRes.merge(LoRes + OneInLo, IsOneInLo);

  vector<uint32_t, N> OneInHi = One << (OneShift - 32);
  auto IsOneInHi = (OneShift >= vector<int32_t, N>(32)) &
                   (OneShift < vector<int32_t, N>(64));
  HiRes.merge(HiRes + OneInHi, IsOneInHi);

  vector<uint32_t, N> SignedBitMask(1u << 31);
  vector<uint32_t, N> SignedBit = Hi & SignedBitMask;

  auto FlagSignSet = (SignedBit != Zero);
  auto FlagNoSignSet = (SignedBit == Zero);
  // check for Exponent overflow (when sign bit set)
  auto FlagExpO = (Exp > vector<uint32_t, N>(1089));
  auto FlagExpUO = FlagNoSignSet & FlagExpO;
  auto IsNaN = (Exp == ExpMask) & ((LoMant != Zero) | (HiMant != Zero));
  if constexpr (isSigned) {
    // calculate (NOT[Lo, Hi] + 1) (integer sign negation)
    vector<uint32_t, N> NegLo = ~LoRes;
    vector<uint32_t, N> NegHi = ~HiRes;

    auto AddC = cm::math::add_with_carry(NegLo, One);
    auto AddcRes = AddC.first;
    auto AddcResCB = AddC.second;
    NegHi = NegHi + AddcResCB;

    // if sign bit is set, alter the result with negated value
    // if (FlagSignSet)
    LoRes.merge(AddcRes, FlagSignSet);
    HiRes.merge(NegHi, FlagSignSet);

    // Here we process overflows
    vector<uint32_t, N> LoOrHi = LoRes | HiRes;
    auto NZ = (LoOrHi != Zero);
    vector<uint32_t, N> HiHBit = HiRes & SignedBitMask;
    auto NZ2 = SignedBit != HiHBit;
    auto Ovrfl1 = NZ2 & NZ;

    // In case of overflow, HW response is : 7fffffffffffffff
    // if (Ovrfl1)
    LoRes.merge(Ones, Ovrfl1);
    HiRes.merge(vector<uint32_t, N>((1u << 31) - 1), Ovrfl1);

    // if (FlagExpO)
    LoRes.merge(Zero, FlagExpO);
    HiRes.merge(vector<uint32_t, N>(1u << 31), FlagExpO);

    // if (FlagExpUO)
    LoRes.merge(Ones, FlagExpUO);
    HiRes.merge(vector<uint32_t, N>((1u << 31) - 1), FlagExpUO);

    // if (IsNaN)
    LoRes.merge(Zero, IsNaN);
    HiRes.merge(Zero, IsNaN);

  } else {
    // if (FlagSignSet)
    LoRes.merge(Zero, FlagSignSet);
    HiRes.merge(Zero, FlagSignSet);

    // if (FlagExpUO)
    LoRes.merge(Ones, FlagExpUO);
    HiRes.merge(Ones, FlagExpUO);

    // if (IsNaN)
    LoRes.merge(Zero, IsNaN);
    HiRes.merge(Zero, IsNaN);
  }
  return __impl_combineLoHi<N>(LoRes, HiRes);
}
template <unsigned N, bool isSigned>
CM_NODEBUG CM_INLINE vector<uint64_t, N> __impl_fp2ui__(vector<float, N> a) {
  // vector of floats -> vector of ints
  vector<uint32_t, N> Uifl = a.template format<uint32_t>();
  const vector<uint32_t, N> ExpMask(0xff);
  const vector<uint32_t, N> MantissaMask((1u << 23) - 1);
  const vector<uint32_t, N> Zero(0);
  const vector<uint32_t, N> Ones(0xffffffff);
  const vector<uint32_t, N> One(1);

  vector<uint32_t, N> Exp = (Uifl >> 23) & ExpMask;
  // mantissa without hidden bit
  vector<uint32_t, N> Pmantissa = Uifl & MantissaMask;
  // take hidden bit into account
  vector<uint32_t, N> Mantissa = Pmantissa | vector<uint32_t, N>(1 << 23);
  vector<uint32_t, N> Data_h = Mantissa << 8;
  vector<uint32_t, N> Data_l = Zero;

  // this block do Logical Shift Right
  vector<uint32_t, N> Shift = vector<uint32_t, N>(0xbe) - Exp;
  vector<uint32_t, N> Sha = Shift & vector<uint32_t, N>(0x3f);
  vector<uint32_t, N> Vec32 = vector<uint32_t, N>(32);
  vector<uint32_t, N> Sh32 = Vec32 - Sha;
  auto Flag_large_shift = (Sha >= Vec32);
  auto Flag_zero_shift = (Sha == Zero);
  vector<uint32_t, N> Mask1 = Ones;
  Mask1.merge(Zero, Flag_large_shift);
  vector<uint32_t, N> Mask0 = Ones;
  Mask0.merge(Zero, Flag_zero_shift);
  vector<uint32_t, N> TmpH1 = ((Data_h & Mask0) << Sh32) & Mask1;
  vector<uint32_t, N> TmpH2 = (Data_h >> (Sha - Vec32)) & ~Mask1;
  vector<uint32_t, N> TmpL = (Data_l >> Sha) & Mask1;
  vector<uint32_t, N> Lo = TmpL | TmpH1 | TmpH2;
  vector<uint32_t, N> Hi = (Data_h >> Sha) & Mask1;

  // Discard results if shift is greater than 63
  vector<uint32_t, N> Mask = Ones;
  auto FlagDiscard = (Shift > vector<uint32_t, N>(63));
  Mask.merge(Zero, FlagDiscard);
  Lo = Lo & Mask;
  Hi = Hi & Mask;
  vector<uint32_t, N> SignedBitMask(1u << 31);
  vector<uint32_t, N> SignedBit = Uifl & SignedBitMask;
  auto FlagSignSet = (SignedBit != Zero);
  auto FlagNoSignSet = (SignedBit == Zero);
  // check for Exponent overflow (when sign bit set)
  auto FlagExpO = (Exp > vector<uint32_t, N>(0xbe));
  auto FlagExpUO = FlagNoSignSet & FlagExpO;
  auto IsNaN = (Exp == ExpMask) & (Pmantissa != Zero);
  if constexpr (isSigned) {
    // calculate (NOT[Lo, Hi] + 1) (integer sign negation)
    vector<uint32_t, N> NegLo = ~Lo;
    vector<uint32_t, N> NegHi = ~Hi;

    auto AddC = cm::math::add_with_carry(NegLo, One);
    auto AddcResVal = AddC.first;
    vector<uint32_t, N> AddcResCB = AddC.second;

    NegHi = NegHi + AddcResCB;

    // if sign bit is set, alter the result with negated value
    // if (FlagSignSet)
    Lo.merge(AddcResVal, FlagSignSet);
    Hi.merge(NegHi, FlagSignSet);

    // Here we process overflows
    vector<uint32_t, N> LoOrHi = Lo | Hi;
    auto NZ = (LoOrHi != Zero);
    vector<uint32_t, N> HiHBit = Hi & SignedBitMask;
    auto NZ2 = SignedBit != HiHBit;
    auto Ovrfl1 = NZ2 & NZ;

    // In case of overflow, HW response is : 7fffffffffffffff
    // if (Ovrfl1)
    Lo.merge(Ones, Ovrfl1);
    Hi.merge(vector<uint32_t, N>((1u << 31) - 1), Ovrfl1);

    // if (FlagExpO)
    Lo.merge(Zero, FlagExpO);
    Hi.merge(vector<uint32_t, N>(1u << 31), FlagExpO);

    // if (FlagExpUO)
    Lo.merge(Ones, FlagExpUO);
    Hi.merge(vector<uint32_t, N>((1u << 31) - 1), FlagExpUO);

    // if (IsNaN)
    Lo.merge(Zero, IsNaN);
    Hi.merge(Zero, IsNaN);
  } else {
    // if (FlagSignSet)
    Lo.merge(Zero, FlagSignSet);
    Hi.merge(Zero, FlagSignSet);

    // if (FlagExpUO)
    Lo.merge(Ones, FlagExpUO);
    Hi.merge(Ones, FlagExpUO);

    // if (IsNaN)
    Lo.merge(Zero, IsNaN);
    Hi.merge(Zero, IsNaN);
  }
  return __impl_combineLoHi<N>(Lo, Hi);
}

template <unsigned N, bool isSigned>
CM_NODEBUG CM_INLINE vector<uint64_t, N>
__impl_fp2ui__half__(vector<half, N> a) {
  vector<uint16_t, N> Val16 = a.template format<uint16_t>();
  vector<uint32_t, N> Val = Val16;
  const vector<uint32_t, N> Zero(0);
  const vector<uint32_t, N> Ones(0xffffffff);
  const vector<uint32_t, N> One(1);
  const vector<uint32_t, N> ExpMask = vector<uint32_t, N>(0x1f);
  const vector<uint32_t, N> MantissaMask = vector<uint32_t, N>(0x3ff);

  vector<uint32_t, N> SignedBitMask(1u << 15);
  vector<uint32_t, N> SignedBit = Val & SignedBitMask;
  vector<uint32_t, N> Exp = (Val >> 10) & ExpMask;
  vector<uint32_t, N> Mant = Val & MantissaMask;
  auto FlagSignSet = (SignedBit != Zero);
  auto FlagNoSignSet = (SignedBit == Zero);

  // check for Exponent overflow (when sign bit set)
  auto FlagExpO = (Exp == vector<uint32_t, N>(0x1f));
  auto FlagExpUO = FlagNoSignSet & FlagExpO;
  auto IsNaN = FlagExpO & (Mant != Zero);
  vector<uint32_t, N> LoRes = a;
  vector<uint32_t, N> HiRes = Zero;
  if constexpr (isSigned) {
    vector<uint32_t, N> IntNegA = -a;
    LoRes.merge(IntNegA, FlagSignSet);
    // calculate (NOT[Lo, Hi] + 1) (integer sign negation)
    vector<uint32_t, N> NegLo = ~LoRes;
    vector<uint32_t, N> NegHi = ~HiRes;

    auto AddC = cm::math::add_with_carry(NegLo, One);
    auto AddcRes = AddC.first;
    auto AddcResCB = AddC.second;
    NegHi = NegHi + AddcResCB;

    // if sign bit is set, alter the result with negated value
    // if (FlagSignSet)
    LoRes.merge(AddcRes, FlagSignSet);
    HiRes.merge(NegHi, FlagSignSet);

    // if (FlagExpO)
    LoRes.merge(Zero, FlagExpO);
    HiRes.merge(vector<uint32_t, N>(1u << 31), FlagExpO);

    // if (FlagExpUO)
    LoRes.merge(Ones, FlagExpUO);
    HiRes.merge(vector<uint32_t, N>((1u << 31) - 1), FlagExpUO);

    // if (IsNaN)
    LoRes.merge(Zero, IsNaN);
    HiRes.merge(Zero, IsNaN);

  } else {
    LoRes.merge(Zero, FlagSignSet);
    HiRes.merge(Zero, FlagSignSet);

    // if (FlagExpUO)
    LoRes.merge(Ones, FlagExpUO);
    HiRes.merge(Ones, FlagExpUO);

    // if (IsNaN)
    LoRes.merge(Zero, IsNaN);
    HiRes.merge(Zero, IsNaN);
  }

  return __impl_combineLoHi<N>(LoRes, HiRes);
}
} // namespace details

#define __FP2UI_D_VECTOR_IMPL(N)                                               \
  CM_NODEBUG CM_NOINLINE extern "C" cl_vector<uint64_t, N>                     \
      __cm_intrinsic_impl_fp2ui__double_##N##_(cl_vector<double, N> a) {       \
    vector<uint64_t, N> b = details::__impl_fp2ui__double__<N, false>(a);      \
    return b.cl_vector();                                                      \
  };

#define __FP2UI_VECTOR_IMPL(N)                                                 \
  CM_NODEBUG CM_NOINLINE extern "C" cl_vector<uint64_t, N>                     \
      __cm_intrinsic_impl_fp2ui_##N##_(cl_vector<float, N> a) {                \
    vector<uint64_t, N> b = details::__impl_fp2ui__<N, false>(a);              \
    return b.cl_vector();                                                      \
  };

#define __FP2UI_H_VECTOR_IMPL(N)                                               \
  CM_NODEBUG CM_NOINLINE extern "C" cl_vector<uint64_t, N>                     \
      __cm_intrinsic_impl_fp2ui__half_##N##_(cl_vector<half, N> a) {           \
    vector<uint64_t, N> b = details::__impl_fp2ui__half__<N, false>(a);        \
    return b.cl_vector();                                                      \
  };

#define __FP2SI_D_VECTOR_IMPL(N)                                               \
  CM_NODEBUG CM_NOINLINE extern "C" cl_vector<int64_t, N>                      \
      __cm_intrinsic_impl_fp2si__double_##N##_(cl_vector<double, N> a) {       \
    vector<int64_t, N> b = details::__impl_fp2ui__double__<N, true>(a);        \
    return b.cl_vector();                                                      \
  };

#define __FP2SI_VECTOR_IMPL(N)                                                 \
  CM_NODEBUG CM_NOINLINE extern "C" cl_vector<int64_t, N>                      \
      __cm_intrinsic_impl_fp2si_##N##_(cl_vector<float, N> a) {                \
    vector<int64_t, N> b = details::__impl_fp2ui__<N, true>(a);                \
    return b.cl_vector();                                                      \
  };

#define __FP2SI_H_VECTOR_IMPL(N)                                               \
  CM_NODEBUG CM_NOINLINE extern "C" cl_vector<int64_t, N>                      \
      __cm_intrinsic_impl_fp2si__half_##N##_(cl_vector<half, N> a) {           \
    vector<int64_t, N> b = details::__impl_fp2ui__half__<N, true>(a);          \
    return b.cl_vector();                                                      \
  };

#define __UI2FP_D_VECTOR_IMPL(N)                                               \
  CM_NODEBUG CM_NOINLINE extern "C" cl_vector<double, N>                       \
      __cm_intrinsic_impl_ui2fp__double_##N##_(cl_vector<uint64_t, N> a) {     \
    vector<double, N> b = details::__impl_ui2fp__double__<N>(a);               \
    return b.cl_vector();                                                      \
  };

#define __UI2FP_VECTOR_IMPL(N)                                                 \
  CM_NODEBUG CM_NOINLINE extern "C" cl_vector<float, N>                        \
      __cm_intrinsic_impl_ui2fp_##N##_(cl_vector<uint64_t, N> a) {             \
    vector<float, N> b = details::__impl_ui2fp__<N>(a);                        \
    return b.cl_vector();                                                      \
  };

#define __UI2FP_H_VECTOR_IMPL(N)                                               \
  CM_NODEBUG CM_NOINLINE extern "C" cl_vector<half, N>                         \
      __cm_intrinsic_impl_ui2fp__half_##N##_(cl_vector<uint64_t, N> a) {       \
    vector<half, N> b = details::__impl_ui2fp__half__<N>(a);                   \
    return b.cl_vector();                                                      \
  };

#define __SI2FP_D_VECTOR_IMPL(N)                                               \
  CM_NODEBUG CM_NOINLINE extern "C" cl_vector<double, N>                       \
      __cm_intrinsic_impl_si2fp__double_##N##_(cl_vector<uint64_t, N> a) {     \
    vector<double, N> b = details::__impl_si2fp__<double, N>(a);               \
    return b.cl_vector();                                                      \
  };

#define __SI2FP_VECTOR_IMPL(N)                                                 \
  CM_NODEBUG CM_NOINLINE extern "C" cl_vector<float, N>                        \
      __cm_intrinsic_impl_si2fp_##N##_(cl_vector<uint64_t, N> a) {             \
    vector<float, N> b = details::__impl_si2fp__<float, N>(a);                 \
    return b.cl_vector();                                                      \
  };

#define __SI2FP_H_VECTOR_IMPL(N)                                               \
  CM_NODEBUG CM_NOINLINE extern "C" cl_vector<half, N>                         \
      __cm_intrinsic_impl_si2fp__half_##N##_(cl_vector<uint64_t, N> a) {       \
    vector<half, N> b = details::__impl_si2fp__<half, N>(a);                   \
    return b.cl_vector();                                                      \
  };

// FP2UI
// special case - input not a vector
CM_NODEBUG CM_NOINLINE extern "C" uint64_t
__cm_intrinsic_impl_fp2ui_1_double_base__(double a) {
  vector<uint64_t, 1> b =
      details::__impl_fp2ui__double__<1, false>(vector<double, 1>(a));
  return b[0];
}

// special case - input not a vector
CM_NODEBUG CM_NOINLINE extern "C" uint64_t
__cm_intrinsic_impl_fp2ui_1_base__(float a) {
  vector<uint64_t, 1> b =
      details::__impl_fp2ui__<1, false>(vector<float, 1>(a));
  return b[0];
}

// special case - input not a vector
CM_NODEBUG CM_NOINLINE extern "C" uint64_t
__cm_intrinsic_impl_fp2ui_1_half_base__(half a) {
  vector<uint64_t, 1> b =
      details::__impl_fp2ui__half__<1, false>(vector<half, 1>(a));
  return b[0];
}

// FP2SI
// special case - input not a vector
CM_NODEBUG CM_NOINLINE extern "C" int64_t
__cm_intrinsic_impl_fp2si_1_double_base__(double a) {
  vector<int64_t, 1> b =
      details::__impl_fp2ui__double__<1, true>(vector<double, 1>(a));
  return b[0];
}

// special case - input not a vector
CM_NODEBUG CM_NOINLINE extern "C" int64_t
__cm_intrinsic_impl_fp2si_1_base__(float a) {
  vector<int64_t, 1> b = details::__impl_fp2ui__<1, true>(vector<float, 1>(a));
  return b[0];
}

// special case - input not a vector
CM_NODEBUG CM_NOINLINE extern "C" int64_t
__cm_intrinsic_impl_fp2si_1_half_base__(half a) {
  vector<int64_t, 1> b =
      details::__impl_fp2ui__half__<1, true>(vector<half, 1>(a));
  return b[0];
}

// UI2FP
// special case - input not a vector
CM_NODEBUG CM_NOINLINE extern "C" double
__cm_intrinsic_impl_ui2fp_1_double_base__(uint64_t a) {
  vector<double, 1> b =
      details::__impl_ui2fp__double__<1>(vector<uint64_t, 1>(a));
  return b[0];
}

// special case - input not a vector
CM_NODEBUG CM_NOINLINE extern "C" float
__cm_intrinsic_impl_ui2fp_1_base__(uint64_t a) {
  vector<float, 1> b = details::__impl_ui2fp__<1>(vector<uint64_t, 1>(a));
  return b[0];
}

// special case - input not a vector
CM_NODEBUG CM_NOINLINE extern "C" half
__cm_intrinsic_impl_ui2fp_1_half_base__(uint64_t a) {
  vector<half, 1> b = details::__impl_ui2fp__half__<1>(vector<uint64_t, 1>(a));
  return b[0];
}

// SI2FP
// special case - input not a vector
CM_NODEBUG CM_NOINLINE extern "C" double
__cm_intrinsic_impl_si2fp_1_double_base__(int64_t a) {
  vector<double, 1> b =
      details::__impl_si2fp__<double, 1>(vector<int64_t, 1>(a));
  return b[0];
}

// special case - input not a vector
CM_NODEBUG CM_NOINLINE extern "C" float
__cm_intrinsic_impl_si2fp_1_base__(int64_t a) {
  vector<float, 1> b = details::__impl_si2fp__<float, 1>(vector<int64_t, 1>(a));
  return b[0];
}

// special case - input not a vector
CM_NODEBUG CM_NOINLINE extern "C" half
__cm_intrinsic_impl_si2fp_1_half_base__(int64_t a) {
  vector<half, 1> b = details::__impl_si2fp__<half, 1>(vector<int64_t, 1>(a));
  return b[0];
}

#define __DEFINE_FP2UI_FUN(N)                                                  \
  __FP2UI_D_VECTOR_IMPL(N);                                                    \
  __FP2UI_VECTOR_IMPL(N);                                                      \
  __FP2UI_H_VECTOR_IMPL(N);                                                    \
  __FP2SI_D_VECTOR_IMPL(N);                                                    \
  __FP2SI_VECTOR_IMPL(N);                                                      \
  __FP2SI_H_VECTOR_IMPL(N);                                                    \
  __UI2FP_D_VECTOR_IMPL(N);                                                    \
  __UI2FP_VECTOR_IMPL(N);                                                      \
  __UI2FP_H_VECTOR_IMPL(N);                                                    \
  __SI2FP_D_VECTOR_IMPL(N);                                                    \
  __SI2FP_VECTOR_IMPL(N);                                                      \
  __SI2FP_H_VECTOR_IMPL(N);

__DEFINE_FP2UI_FUN(1);
__DEFINE_FP2UI_FUN(2);
__DEFINE_FP2UI_FUN(4);
__DEFINE_FP2UI_FUN(8);
__DEFINE_FP2UI_FUN(16);
__DEFINE_FP2UI_FUN(32);

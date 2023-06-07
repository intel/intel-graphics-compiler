/*========================== begin_copyright_notice ============================

Copyright (C) 2021-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include <cm-cl/math.h>
#include <cm-cl/vector.h>

#include "../helpers.h"

using namespace cm;

namespace {
template <bool IsSigned, int N>
CM_NODEBUG CM_INLINE vector<uint64_t, N> __impl_fptoi(vector<double, N> a) {
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
  if constexpr (IsSigned) {
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
} // namespace

CM_NODEBUG CM_NOINLINE extern "C" uint64_t __vc_builtin_fptosi_f64(double a) {
  vector<double, 1> va = a;
  return __impl_fptoi<true>(va)[0];
}

CM_NODEBUG CM_NOINLINE extern "C" uint64_t __vc_builtin_fptoui_f64(double a) {
  vector<double, 1> va = a;
  return __impl_fptoi<false>(va)[0];
}

#define FPTOI(WIDTH)                                                           \
  CM_NODEBUG CM_NOINLINE extern "C" cl_vector<uint64_t, WIDTH>                 \
      __vc_builtin_fptosi_v##WIDTH##f64(cl_vector<double, WIDTH> a) {          \
    vector<double, WIDTH> va{a};                                               \
    return __impl_fptoi<true>(va).cl_vector();                                 \
  }                                                                            \
  CM_NODEBUG CM_NOINLINE extern "C" cl_vector<uint64_t, WIDTH>                 \
      __vc_builtin_fptoui_v##WIDTH##f64(cl_vector<double, WIDTH> a) {          \
    vector<double, WIDTH> va{a};                                               \
    return __impl_fptoi<false>(va).cl_vector();                                \
  }

FPTOI(1)
FPTOI(2)
FPTOI(4)
FPTOI(8)
FPTOI(16)

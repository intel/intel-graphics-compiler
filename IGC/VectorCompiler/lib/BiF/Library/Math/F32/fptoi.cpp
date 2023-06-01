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
CM_NODEBUG CM_INLINE vector<uint64_t, N> __impl_fptoi(vector<float, N> a) {
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
  if constexpr (IsSigned) {
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
} // namespace

CM_NODEBUG CM_NOINLINE extern "C" uint64_t __vc_builtin_fptosi_f32(float a) {
  vector<float, 1> va = a;
  return __impl_fptoi<true>(va)[0];
}

CM_NODEBUG CM_NOINLINE extern "C" uint64_t __vc_builtin_fptoui_f32(float a) {
  vector<float, 1> va = a;
  return __impl_fptoi<false>(va)[0];
}

#define FPTOI(WIDTH)                                                           \
  CM_NODEBUG CM_NOINLINE extern "C" cl_vector<uint64_t, WIDTH>                 \
      __vc_builtin_fptosi_v##WIDTH##f32(cl_vector<float, WIDTH> a) {           \
    vector<float, WIDTH> va{a};                                                \
    return __impl_fptoi<true>(va).cl_vector();                                 \
  }                                                                            \
  CM_NODEBUG CM_NOINLINE extern "C" cl_vector<uint64_t, WIDTH>                 \
      __vc_builtin_fptoui_v##WIDTH##f32(cl_vector<float, WIDTH> a) {           \
    vector<float, WIDTH> va{a};                                                \
    return __impl_fptoi<false>(va).cl_vector();                                \
  }

FPTOI(1)
FPTOI(2)
FPTOI(4)
FPTOI(8)
FPTOI(16)
FPTOI(32)

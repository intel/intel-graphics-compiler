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
CM_NODEBUG CM_INLINE vector<uint64_t, N> __impl_fptoi(vector<half, N> a) {
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
  if constexpr (IsSigned) {
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
} // namespace

CM_NODEBUG CM_NOINLINE extern "C" uint64_t __vc_builtin_fptosi_f16(half a) {
  vector<half, 1> va = a;
  return __impl_fptoi<true>(va)[0];
}

CM_NODEBUG CM_NOINLINE extern "C" uint64_t __vc_builtin_fptoui_f16(half a) {
  vector<half, 1> va = a;
  return __impl_fptoi<false>(va)[0];
}

#define FPTOI(WIDTH)                                                           \
  CM_NODEBUG CM_NOINLINE extern "C" cl_vector<uint64_t, WIDTH>                 \
      __vc_builtin_fptosi_v##WIDTH##f16(cl_vector<half, WIDTH> a) {            \
    vector<half, WIDTH> va{a};                                                 \
    return __impl_fptoi<true>(va).cl_vector();                                 \
  }                                                                            \
  CM_NODEBUG CM_NOINLINE extern "C" cl_vector<uint64_t, WIDTH>                 \
      __vc_builtin_fptoui_v##WIDTH##f16(cl_vector<half, WIDTH> a) {            \
    vector<half, WIDTH> va{a};                                                 \
    return __impl_fptoi<false>(va).cl_vector();                                \
  }

FPTOI(1)
FPTOI(2)
FPTOI(4)
FPTOI(8)
FPTOI(16)
FPTOI(32)

/*========================== begin_copyright_notice ============================

Copyright (C) 2021-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include <cm-cl/math.h>
#include <cm-cl/vector.h>

#include "../helpers.h"

using namespace cm;

namespace {
template <int N>
CM_NODEBUG CM_INLINE vector<double, N> __impl_uitofp(vector<uint64_t, N> a) {
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
}

template <int N>
CM_NODEBUG CM_INLINE vector<double, N> __impl_sitofp(vector<int64_t, N> a) {
  vector<uint64_t, N> Abs = math::absolute(a);
  auto Res = __impl_uitofp(Abs);
  Res.merge(-Res, a < 0);
  return Res;
}
} // namespace

CM_NODEBUG CM_NOINLINE extern "C" double __vc_builtin_uitofp_f64(uint64_t a) {
  vector<uint64_t, 1> va = a;
  return __impl_uitofp(va)[0];
}

CM_NODEBUG CM_NOINLINE extern "C" double __vc_builtin_sitofp_f64(int64_t a) {
  vector<int64_t, 1> va = a;
  return __impl_sitofp(va)[0];
}

#define ITOFP(WIDTH)                                                           \
  CM_NODEBUG CM_NOINLINE extern "C" cl_vector<double, WIDTH>                   \
      __vc_builtin_uitofp_v##WIDTH##f64(cl_vector<uint64_t, WIDTH> a) {        \
    vector<uint64_t, WIDTH> va{a};                                             \
    return __impl_uitofp(va).cl_vector();                                      \
  }                                                                            \
  CM_NODEBUG CM_NOINLINE extern "C" cl_vector<double, WIDTH>                   \
      __vc_builtin_sitofp_v##WIDTH##f64(cl_vector<int64_t, WIDTH> a) {         \
    vector<int64_t, WIDTH> va{a};                                              \
    return __impl_sitofp(va).cl_vector();                                      \
  }

ITOFP(1)
ITOFP(2)
ITOFP(4)
ITOFP(8)
ITOFP(16)

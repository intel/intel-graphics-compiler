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
CM_NODEBUG CM_INLINE vector<float, N> __impl_uitofp(vector<uint64_t, N> a) {
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

template <int N>
CM_NODEBUG CM_INLINE vector<float, N> __impl_sitofp(vector<int64_t, N> a) {
  vector<uint64_t, N> Abs = math::absolute(a);
  auto Res = __impl_uitofp(Abs);
  Res.merge(-Res, a < 0);
  return Res;
}
} // namespace

CM_NODEBUG CM_NOINLINE extern "C" float __vc_builtin_uitofp_f32(uint64_t a) {
  vector<uint64_t, 1> va = a;
  return __impl_uitofp(va)[0];
}

CM_NODEBUG CM_NOINLINE extern "C" float __vc_builtin_sitofp_f32(int64_t a) {
  vector<int64_t, 1> va = a;
  return __impl_sitofp(va)[0];
}

#define ITOFP(WIDTH)                                                           \
  CM_NODEBUG CM_NOINLINE extern "C" cl_vector<float, WIDTH>                    \
      __vc_builtin_uitofp_v##WIDTH##f32(cl_vector<uint64_t, WIDTH> a) {        \
    vector<uint64_t, WIDTH> va{a};                                             \
    return __impl_uitofp(va).cl_vector();                                      \
  }                                                                            \
  CM_NODEBUG CM_NOINLINE extern "C" cl_vector<float, WIDTH>                    \
      __vc_builtin_sitofp_v##WIDTH##f32(cl_vector<int64_t, WIDTH> a) {         \
    vector<int64_t, WIDTH> va{a};                                              \
    return __impl_sitofp(va).cl_vector();                                      \
  }

ITOFP(1)
ITOFP(2)
ITOFP(4)
ITOFP(8)
ITOFP(16)
ITOFP(32)

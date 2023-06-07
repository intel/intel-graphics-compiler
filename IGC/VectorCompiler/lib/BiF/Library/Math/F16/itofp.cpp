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
CM_NODEBUG CM_INLINE vector<half, N> __impl_uitofp(vector<uint64_t, N> a) {
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
}

template <int N>
CM_NODEBUG CM_INLINE vector<half, N> __impl_sitofp(vector<int64_t, N> a) {
  vector<uint64_t, N> Abs = math::absolute(a);
  auto Res = __impl_uitofp(Abs);
  Res.merge(-Res, a < 0);
  return Res;
}
} // namespace

CM_NODEBUG CM_NOINLINE extern "C" half __vc_builtin_uitofp_f16(uint64_t a) {
  vector<uint64_t, 1> va = a;
  return __impl_uitofp(va)[0];
}

CM_NODEBUG CM_NOINLINE extern "C" half __vc_builtin_sitofp_f16(int64_t a) {
  vector<int64_t, 1> va = a;
  return __impl_sitofp(va)[0];
}

#define ITOFP(WIDTH)                                                           \
  CM_NODEBUG CM_NOINLINE extern "C" cl_vector<half, WIDTH>                     \
      __vc_builtin_uitofp_v##WIDTH##f16(cl_vector<uint64_t, WIDTH> a) {        \
    vector<uint64_t, WIDTH> va{a};                                             \
    return __impl_uitofp(va).cl_vector();                                      \
  }                                                                            \
  CM_NODEBUG CM_NOINLINE extern "C" cl_vector<half, WIDTH>                     \
      __vc_builtin_sitofp_v##WIDTH##f16(cl_vector<int64_t, WIDTH> a) {         \
    vector<int64_t, WIDTH> va{a};                                              \
    return __impl_sitofp(va).cl_vector();                                      \
  }

ITOFP(1)
ITOFP(2)
ITOFP(4)
ITOFP(8)
ITOFP(16)
ITOFP(32)

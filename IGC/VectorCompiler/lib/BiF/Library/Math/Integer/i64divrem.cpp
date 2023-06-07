/*========================== begin_copyright_notice ============================

Copyright (C) 2021-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include <cm-cl/math.h>
#include <cm-cl/vector.h>

using namespace cm;
namespace {
template <int N>
CM_NODEBUG CM_INLINE void
__impl_divrem(vector<uint64_t, N> &q, vector<uint64_t, N> &r,
              vector<uint64_t, N> a, vector<uint64_t, N> b) {
  q = 0ul;
  r = 0ul;

  for (int i = 63; i >= 0; --i) {
    unsigned vi = i;

    auto sa = (a >> vi) & 1ul;
    r = (r << 1) | sa;

    auto ge = r >= b;
    r.merge(r - b, ge);
    q.merge(q | (1ul << vi), ge);
  }
}

template <int N>
CM_NODEBUG CM_INLINE void
__impl_divrem(vector<int64_t, N> &q, vector<int64_t, N> &r,
              vector<int64_t, N> a, vector<int64_t, N> b) {
  vector<uint64_t, N> abs_a = math::absolute(a);
  vector<uint64_t, N> abs_b = math::absolute(b);
  vector<uint64_t, N> uq, ur;

  __impl_divrem(uq, ur, abs_a, abs_b);

  vector<uint32_t, N> a_hi =
      a.template format<uint32_t>().template select<N, 2>(1);
  vector<uint32_t, N> b_hi =
      b.template format<uint32_t>().template select<N, 2>(1);
  mask<N> is_quot_negative = ((a_hi ^ b_hi) & (1u << 31)) != 0;
  mask<N> is_rem_negative = (a_hi & (1u << 31)) != 0;

  q = merge(-uq, uq, is_quot_negative);
  r = merge(-ur, ur, is_rem_negative);
}
} // namespace

CM_NODEBUG CM_NOINLINE extern "C" int64_t __vc_builtin_sdiv_i64(int64_t a,
                                                                int64_t b) {
  vector<int64_t, 1> va = a, vb = b, vq, vr;
  __impl_divrem(vq, vr, va, vb);
  return vq[0];
}
CM_NODEBUG CM_NOINLINE extern "C" uint64_t __vc_builtin_udiv_i64(uint64_t a,
                                                                 uint64_t b) {
  vector<uint64_t, 1> va = a, vb = b, vq, vr;
  __impl_divrem(vq, vr, va, vb);
  return vq[0];
}
CM_NODEBUG CM_NOINLINE extern "C" int64_t __vc_builtin_srem_i64(int64_t a,
                                                                int64_t b) {
  vector<int64_t, 1> va = a, vb = b, vq, vr;
  __impl_divrem(vq, vr, va, vb);
  return vr[0];
}
CM_NODEBUG CM_NOINLINE extern "C" uint64_t __vc_builtin_urem_i64(uint64_t a,
                                                                 uint64_t b) {
  vector<uint64_t, 1> va = a, vb = b, vq, vr;
  __impl_divrem(vq, vr, va, vb);
  return vr[0];
}

#define DIVREM(WIDTH)                                                          \
  CM_NODEBUG CM_NOINLINE extern "C" cl_vector<int64_t, WIDTH>                  \
      __vc_builtin_sdiv_v##WIDTH##i64(cl_vector<int64_t, WIDTH> a,             \
                                      cl_vector<int64_t, WIDTH> b) {           \
    vector<int64_t, WIDTH> va = a, vb = b, vq, vr;                             \
    __impl_divrem(vq, vr, va, vb);                                             \
    return vq.cl_vector();                                                     \
  }                                                                            \
  CM_NODEBUG CM_NOINLINE extern "C" cl_vector<uint64_t, WIDTH>                 \
      __vc_builtin_udiv_v##WIDTH##i64(cl_vector<uint64_t, WIDTH> a,            \
                                      cl_vector<uint64_t, WIDTH> b) {          \
    vector<uint64_t, WIDTH> va = a, vb = b, vq, vr;                            \
    __impl_divrem(vq, vr, va, vb);                                             \
    return vq.cl_vector();                                                     \
  }                                                                            \
  CM_NODEBUG CM_NOINLINE extern "C" cl_vector<int64_t, WIDTH>                  \
      __vc_builtin_srem_v##WIDTH##i64(cl_vector<int64_t, WIDTH> a,             \
                                      cl_vector<int64_t, WIDTH> b) {           \
    vector<int64_t, WIDTH> va = a, vb = b, vq, vr;                             \
    __impl_divrem(vq, vr, va, vb);                                             \
    return vr.cl_vector();                                                     \
  }                                                                            \
  CM_NODEBUG CM_NOINLINE extern "C" cl_vector<uint64_t, WIDTH>                 \
      __vc_builtin_urem_v##WIDTH##i64(cl_vector<uint64_t, WIDTH> a,            \
                                      cl_vector<uint64_t, WIDTH> b) {          \
    vector<uint64_t, WIDTH> va = a, vb = b, vq, vr;                            \
    __impl_divrem(vq, vr, va, vb);                                             \
    return vr.cl_vector();                                                     \
  }

DIVREM(1)
DIVREM(2)
DIVREM(4)
DIVREM(8)
DIVREM(16)

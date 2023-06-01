/*========================== begin_copyright_notice ============================

Copyright (C) 2021-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include <cm-cl/math.h>
#include <cm-cl/vector.h>

using namespace cm;

namespace {
// RTZ rounding mode is needed for some of the steps
// It is fine to use RTZ mode throughout the FP computation
template <int N>
CM_NODEBUG CM_INLINE void
__impl_divrem(vector<int32_t, N> &q, vector<int32_t, N> &r,
              vector<int32_t, N> sla, vector<int32_t, N> slb) {
  vector<float, N> ah, al, bh, bl;
  vector<float, N> y, Qh, Ql, Rh, Rl;

  vector<uint32_t, N> la_h, lb_h, la_l, lb_l, lQh, lQl, lR;
  vector<uint32_t, N> la, lb, corr_mask;
  vector<int32_t, N> sgn_a, sgn_b, sgn_q;

  // get signs and |sla|, |slb|
  sgn_a = sla >> 31;
  sgn_b = slb >> 31;
  la = math::absolute(sla);
  lb = math::absolute(slb);

  // uint32 -> single precision convert, with truncation (RZ mode)
  bh = lb;

  // convert back to uint32, to get low part
  lb_h = bh;
  lb_l = lb - lb_h;

  // low part of input, in single precision
  bl = lb_l;

  // uint32 -> single precision convert, with truncation (RZ mode)
  ah = la;

  // convert back to uint32, to get low part
  la_h = ah;
  la_l = la - la_h;

  // low part of input, in single precision
  al = la_l;

  // y = RCP(bh)
  y = 1.0f / bh;

  // y = y*(1 - 3*2^(-23))
  vector<float, N> sf = as_float(0xb4c00000u);
  y = math::mad(sf, y, y);

  // Qh = ah*y
  Qh = ah * y;

  // Qh = (unsigned)Qh, with truncation
  lQh = Qh;

  // convert lQh back to SP, any rounding mode is fine
  Qh = lQh;

  // ah - bh*Qh
  Rh = math::mad(-bh, Qh, ah);

  // al - bl*Qh
  Rl = math::mad(-bl, Qh, al);

  // Ql = y * (Rh + Rl)
  Rl = Rh + Rl;
  Ql = y * Rl;

  // convert Ql to integer, with truncation
  lQl = Ql;

  // integer quotient
  q = lQh + lQl;
  sgn_q = sgn_a ^ sgn_b;

  // integer remainder
  lR = la - lb * q;

  // apply correction if needed
  // if (lR >= lb) { q++;  lR -= lb; }
  corr_mask = merge(0xffffffff, 0u, (lR >= lb));
  q += (corr_mask & 1);
  lR -= (lb & corr_mask);

  // remainder
  r = (sgn_a + lR) ^ sgn_a;
  q = (sgn_q + q) ^ sgn_q;
}

template <int N>
CM_NODEBUG CM_INLINE void
__impl_divrem(vector<int16_t, N> &q, vector<int16_t, N> &r,
              vector<int16_t, N> a, vector<int16_t, N> b) {
  vector<float, N> fa = a;
  vector<float, N> fb = b;
  vector<float, N> fy = 1.0f / fb;

  // s = 1 + 2^(-20)
  vector<float, N> sf = as_float(0x3f800008U);

  // a * (1 + 2^(-20))
  fa = fa * sf;

  // a * (1 + 2^(-20)) * fy
  vector<float, N> fq = fa * fy;

  // quotient: truncate to signed 16-bit integer
  q = fq;

  // remainder
  r = a - q * b;
}

template <int N>
CM_NODEBUG CM_INLINE void
__impl_divrem(vector<int8_t, N> &q, vector<int8_t, N> &r, vector<int8_t, N> a,
              vector<int8_t, N> b) {
  vector<int16_t, N> xa = a, xb = b;
  vector<int16_t, N> xq, xr;
  __impl_divrem(xq, xr, xa, xb);
  r = xr;
  q = xq;
}
} // namespace

CM_NODEBUG CM_NOINLINE extern "C" int32_t
__vc_builtin_sdiv_i32__rtz_(int32_t a, int32_t b) {
  vector<int32_t, 1> va = a, vb = b, vq, vr;
  __impl_divrem(vq, vr, va, vb);
  return vq[0];
}
CM_NODEBUG CM_NOINLINE extern "C" int16_t __vc_builtin_sdiv_i16(int16_t a,
                                                                int16_t b) {
  vector<int16_t, 1> va = a, vb = b, vq, vr;
  __impl_divrem(vq, vr, va, vb);
  return vq[0];
}
CM_NODEBUG CM_NOINLINE extern "C" int8_t __vc_builtin_sdiv_i8(int8_t a,
                                                              int8_t b) {
  vector<int8_t, 1> va = a, vb = b, vq, vr;
  __impl_divrem(vq, vr, va, vb);
  return vq[0];
}

#define SDIV(WIDTH)                                                            \
  CM_NODEBUG CM_NOINLINE extern "C" cl_vector<int32_t, WIDTH>                  \
      __vc_builtin_sdiv_v##WIDTH##i32__rtz_(cl_vector<int32_t, WIDTH> a,       \
                                            cl_vector<int32_t, WIDTH> b) {     \
    vector<int32_t, WIDTH> va = a, vb = b, vq, vr;                             \
    __impl_divrem(vq, vr, va, vb);                                             \
    return vq.cl_vector();                                                     \
  }                                                                            \
  CM_NODEBUG CM_NOINLINE extern "C" cl_vector<int16_t, WIDTH>                  \
      __vc_builtin_sdiv_v##WIDTH##i16(cl_vector<int16_t, WIDTH> a,             \
                                      cl_vector<int16_t, WIDTH> b) {           \
    vector<int16_t, WIDTH> va = a, vb = b, vq, vr;                             \
    __impl_divrem(vq, vr, va, vb);                                             \
    return vq.cl_vector();                                                     \
  }                                                                            \
  CM_NODEBUG CM_NOINLINE extern "C" cl_vector<int8_t, WIDTH>                   \
      __vc_builtin_sdiv_v##WIDTH##i8(cl_vector<int8_t, WIDTH> a,               \
                                     cl_vector<int8_t, WIDTH> b) {             \
    vector<int8_t, WIDTH> va = a, vb = b, vq, vr;                              \
    __impl_divrem(vq, vr, va, vb);                                             \
    return vq.cl_vector();                                                     \
  }

SDIV(1)
SDIV(2)
SDIV(4)
SDIV(8)
SDIV(16)
SDIV(32)

CM_NODEBUG CM_NOINLINE extern "C" int32_t
__vc_builtin_srem_i32__rtz_(int32_t a, int32_t b) {
  vector<int32_t, 1> va = a, vb = b, vq, vr;
  __impl_divrem(vq, vr, va, vb);
  return vr[0];
}
CM_NODEBUG CM_NOINLINE extern "C" int16_t __vc_builtin_srem_i16(int16_t a,
                                                                int16_t b) {
  vector<int16_t, 1> va = a, vb = b, vq, vr;
  __impl_divrem(vq, vr, va, vb);
  return vr[0];
}
CM_NODEBUG CM_NOINLINE extern "C" int8_t __vc_builtin_srem_i8(int8_t a,
                                                              int8_t b) {
  vector<int8_t, 1> va = a, vb = b, vq, vr;
  __impl_divrem(vq, vr, va, vb);
  return vr[0];
}

#define SREM(WIDTH)                                                            \
  CM_NODEBUG CM_NOINLINE extern "C" cl_vector<int32_t, WIDTH>                  \
      __vc_builtin_srem_v##WIDTH##i32__rtz_(cl_vector<int32_t, WIDTH> a,       \
                                            cl_vector<int32_t, WIDTH> b) {     \
    vector<int32_t, WIDTH> va = a, vb = b, vq, vr;                             \
    __impl_divrem(vq, vr, va, vb);                                             \
    return vr.cl_vector();                                                     \
  }                                                                            \
  CM_NODEBUG CM_NOINLINE extern "C" cl_vector<int16_t, WIDTH>                  \
      __vc_builtin_srem_v##WIDTH##i16(cl_vector<int16_t, WIDTH> a,             \
                                      cl_vector<int16_t, WIDTH> b) {           \
    vector<int16_t, WIDTH> va = a, vb = b, vq, vr;                             \
    __impl_divrem(vq, vr, va, vb);                                             \
    return vr.cl_vector();                                                     \
  }                                                                            \
  CM_NODEBUG CM_NOINLINE extern "C" cl_vector<int8_t, WIDTH>                   \
      __vc_builtin_srem_v##WIDTH##i8(cl_vector<int8_t, WIDTH> a,               \
                                     cl_vector<int8_t, WIDTH> b) {             \
    vector<int8_t, WIDTH> va = a, vb = b, vq, vr;                              \
    __impl_divrem(vq, vr, va, vb);                                             \
    return vr.cl_vector();                                                     \
  }

SREM(1)
SREM(2)
SREM(4)
SREM(8)
SREM(16)
SREM(32)

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
__impl_divrem(vector<uint32_t, N> &q, vector<uint32_t, N> &r,
              vector<uint32_t, N> la, vector<uint32_t, N> lb) {
  vector<float, N> ah, al, bh, bl;
  vector<float, N> y, Qh, Ql, Rh, Rl;

  vector<uint32_t, N> la_h, lb_h, la_l, lb_l, lQh, lQl, lQ, lR, corr_mask;

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

  // integer remainder
  lR = la - lb * q;

  // apply correction if needed
  // if (lR >= lb) { q++;  lR -= lb; }
  corr_mask = merge(0xffffffff, 0u, (lR >= lb));
  q += (corr_mask & 1);
  r = lR - (lb & corr_mask);
}

template <int N>
CM_NODEBUG CM_INLINE void
__impl_divrem(vector<uint16_t, N> &q, vector<uint16_t, N> &r,
              vector<uint16_t, N> a, vector<uint16_t, N> b) {
  vector<float, N> fa = a;
  vector<float, N> fb = b;
  vector<float, N> fy = 1.0f / fb;

  // s = 1 + 2^(-20)
  vector<float, N> sf = as_float(0x3f800008U);

  // a * (1 + 2^(-20))
  fa = fa * sf;

  // a * (1 + 2^(-20)) * fy
  vector<float, N> fq = fa * fy;

  // quotient: truncate to unsigned 16-bit integer
  q = fq;

  // remainder
  r = a - q * b;
}

template <int N>
CM_NODEBUG CM_INLINE void
__impl_divrem(vector<uint8_t, N> &q, vector<uint8_t, N> &r,
              vector<uint8_t, N> a, vector<uint8_t, N> b) {
  vector<uint16_t, N> xa = a, xb = b;
  vector<uint16_t, N> xq, xr;
  __impl_divrem(xq, xr, xa, xb);
  r = xr;
  q = xq;
}
} // namespace

CM_NODEBUG CM_NOINLINE extern "C" uint32_t
__vc_builtin_udiv_i32__rtz_(uint32_t a, uint32_t b) {
  vector<uint32_t, 1> va = a, vb = b, vq, vr;
  __impl_divrem(vq, vr, va, vb);
  return vq[0];
}
CM_NODEBUG CM_NOINLINE extern "C" uint16_t __vc_builtin_udiv_i16(uint16_t a,
                                                                 uint16_t b) {
  vector<uint16_t, 1> va = a, vb = b, vq, vr;
  __impl_divrem(vq, vr, va, vb);
  return vq[0];
}
CM_NODEBUG CM_NOINLINE extern "C" uint8_t __vc_builtin_udiv_i8(uint8_t a,
                                                               uint8_t b) {
  vector<uint8_t, 1> va = a, vb = b, vq, vr;
  __impl_divrem(vq, vr, va, vb);
  return vq[0];
}

#define UDIV(WIDTH)                                                            \
  CM_NODEBUG CM_NOINLINE extern "C" cl_vector<uint32_t, WIDTH>                 \
      __vc_builtin_udiv_v##WIDTH##i32__rtz_(cl_vector<uint32_t, WIDTH> a,      \
                                            cl_vector<uint32_t, WIDTH> b) {    \
    vector<uint32_t, WIDTH> va = a, vb = b, vq, vr;                            \
    __impl_divrem(vq, vr, va, vb);                                             \
    return vq.cl_vector();                                                     \
  }                                                                            \
  CM_NODEBUG CM_NOINLINE extern "C" cl_vector<uint16_t, WIDTH>                 \
      __vc_builtin_udiv_v##WIDTH##i16(cl_vector<uint16_t, WIDTH> a,            \
                                      cl_vector<uint16_t, WIDTH> b) {          \
    vector<uint16_t, WIDTH> va = a, vb = b, vq, vr;                            \
    __impl_divrem(vq, vr, va, vb);                                             \
    return vq.cl_vector();                                                     \
  }                                                                            \
  CM_NODEBUG CM_NOINLINE extern "C" cl_vector<uint8_t, WIDTH>                  \
      __vc_builtin_udiv_v##WIDTH##i8(cl_vector<uint8_t, WIDTH> a,              \
                                     cl_vector<uint8_t, WIDTH> b) {            \
    vector<uint8_t, WIDTH> va = a, vb = b, vq, vr;                             \
    __impl_divrem(vq, vr, va, vb);                                             \
    return vq.cl_vector();                                                     \
  }

UDIV(1)
UDIV(2)
UDIV(4)
UDIV(8)
UDIV(16)
UDIV(32)

CM_NODEBUG CM_NOINLINE extern "C" uint32_t
__vc_builtin_urem_i32__rtz_(uint32_t a, uint32_t b) {
  vector<uint32_t, 1> va = a, vb = b, vq, vr;
  __impl_divrem(vq, vr, va, vb);
  return vr[0];
}
CM_NODEBUG CM_NOINLINE extern "C" uint16_t __vc_builtin_urem_i16(uint16_t a,
                                                                 uint16_t b) {
  vector<uint16_t, 1> va = a, vb = b, vq, vr;
  __impl_divrem(vq, vr, va, vb);
  return vr[0];
}
CM_NODEBUG CM_NOINLINE extern "C" uint8_t __vc_builtin_urem_i8(uint8_t a,
                                                               uint8_t b) {
  vector<uint8_t, 1> va = a, vb = b, vq, vr;
  __impl_divrem(vq, vr, va, vb);
  return vr[0];
}

#define UREM(WIDTH)                                                            \
  CM_NODEBUG CM_NOINLINE extern "C" cl_vector<uint32_t, WIDTH>                 \
      __vc_builtin_urem_v##WIDTH##i32__rtz_(cl_vector<uint32_t, WIDTH> a,      \
                                            cl_vector<uint32_t, WIDTH> b) {    \
    vector<uint32_t, WIDTH> va = a, vb = b, vq, vr;                            \
    __impl_divrem(vq, vr, va, vb);                                             \
    return vr.cl_vector();                                                     \
  }                                                                            \
  CM_NODEBUG CM_NOINLINE extern "C" cl_vector<uint16_t, WIDTH>                 \
      __vc_builtin_urem_v##WIDTH##i16(cl_vector<uint16_t, WIDTH> a,            \
                                      cl_vector<uint16_t, WIDTH> b) {          \
    vector<uint16_t, WIDTH> va = a, vb = b, vq, vr;                            \
    __impl_divrem(vq, vr, va, vb);                                             \
    return vr.cl_vector();                                                     \
  }                                                                            \
  CM_NODEBUG CM_NOINLINE extern "C" cl_vector<uint8_t, WIDTH>                  \
      __vc_builtin_urem_v##WIDTH##i8(cl_vector<uint8_t, WIDTH> a,              \
                                     cl_vector<uint8_t, WIDTH> b) {            \
    vector<uint8_t, WIDTH> va = a, vb = b, vq, vr;                             \
    __impl_divrem(vq, vr, va, vb);                                             \
    return vr.cl_vector();                                                     \
  }

UREM(1)
UREM(2)
UREM(4)
UREM(8)
UREM(16)
UREM(32)

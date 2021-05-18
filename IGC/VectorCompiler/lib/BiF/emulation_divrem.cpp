/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include <cm-cl/vector.h>

using namespace cm;

namespace details {

static CM_NODEBUG CM_INLINE float __impl_hex2float(uint32_t val) {
  vector<uint32_t, 1> su = val;
  vector<float, 1> sf = su.format<float>();
  return sf[0];
}

template <int N>
CM_NODEBUG CM_INLINE vector<int16_t, N> __impl_divrem(vector<int16_t, N> ia,
                                                      vector<int16_t, N> ib,
                                                      vector<int16_t, N> &ir) {
  // Exact conversions: 16-bit -> float
  vector<float, N> fa = ia;
  vector<float, N> fb = ib;
  vector<float, N> fy = 1.0f / fb;

  // s = 1 + 2^(-20)
  vector<float, N> sf = __impl_hex2float(0x3f800008U);

  // a * (1 + 2^(-20))
  fa = fa * sf;

  // a * (1 + 2^(-20)) * fy
  vector<float, N> fq = fa * fy;

  // quotient: truncate to signed 16-bit integer
  vector<int16_t, N> iq = fq;

  // remainder
  ir = ia - iq * ib;

  // return the quotient.
  return iq;
}

template <int N>
CM_NODEBUG CM_INLINE vector<int8_t, N> __impl_divrem(vector<int8_t, N> ia,
                                                     vector<int8_t, N> ib,
                                                     vector<int8_t, N> &ir) {
  vector<int16_t, N> _ia = ia;
  vector<int16_t, N> _ib = ib;
  vector<int16_t, N> _ir;
  vector<int16_t, N> _iq = __impl_divrem(_ia, _ib, _ir);
  ir = _ir;
  return _iq;
}

// RTZ rounding mode is needed for some of the steps
// It is fine to use RTZ mode throughout the FP computation
template <int N>
CM_NODEBUG CM_INLINE vector<int32_t, N>
__impl_divrem(vector<int32_t, N> sla, vector<int32_t, N> slb,
              vector<uint32_t, N> &prem) {
  vector<float, N> ah, al, bh, bl;
  vector<float, N> y, Qh, Ql, Rh, Rl;

  vector<uint32_t, N> la_h, lb_h, la_l, lb_l, lQh, lQl, lQ, lR;
  vector<uint32_t, N> la, lb, corr_mask;
  vector<int32_t, N> sgn_a, sgn_b, sgn_q;

  // get signs and |sla|, |slb|
  sgn_a = sla >> 31;
  sgn_b = slb >> 31;
  la = (sla + sgn_a) ^ sgn_a;
  lb = (slb + sgn_b) ^ sgn_b;

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
  // RZ mode not required, used for convenience
  vector<float, N> sf = __impl_hex2float(0xb4c00000u);
  y += sf * y;

  // Qh = ah*y
  Qh = ah * y;

  // Qh = (unsigned)Qh, with truncation
  lQh = Qh;

  // convert lQh back to SP, any rounding mode is fine
  Qh = lQh;

  // ah - bh*Qh
  Rh = ah - bh * Qh;

  // al - bl*Qh
  Rl = al - bl * Qh;

  // Ql = y * (Rh + Rl)
  Rl = Rh + Rl;
  Ql = y * Rl;

  // convert Ql to integer, with truncation
  lQl = Ql;

  // integer quotient
  lQ = lQh + lQl;
  sgn_q = sgn_a ^ sgn_b;

  // integer remainder
  lR = la - lb * lQ;

  // apply correction if needed
  // if (lR >= lb) { lQ++;  lR -= lb; }
  corr_mask = merge(0xffffffff, 0u, (lR >= lb));
  lQ += (corr_mask & 1);
  lR -= (lb & corr_mask);

  // remainder
  prem = (sgn_a + lR) ^ sgn_a;
  lQ = (sgn_q + lQ) ^ sgn_q;

  return lQ;
}

template <int N>
CM_NODEBUG CM_INLINE vector<uint16_t, N>
__impl_udivrem(vector<uint16_t, N> ua, vector<uint16_t, N> ub,
               vector<uint16_t, N> &ur) {
  vector<float, N> fa, fb, fy, fq;
  vector<uint16_t, N> uq;

  // exact conversions: unsigned 16-bit -> float
  fb = ub;
  fa = ua;

  fy = 1.0f / fb;

  // a*(1+2^(-20))
  fa = fa * __impl_hex2float(0x3f800008u);

  // a*(1+2^(-20))*fy
  fq = fa * fy;

  // quotient:  truncate to unsigned 16-bit integer
  uq = fq;

  // remainder
  ur = ua - uq * ub;

  return uq;
}

template <int N>
CM_NODEBUG CM_INLINE vector<uint8_t, N> __impl_udivrem(vector<uint8_t, N> ua,
                                                       vector<uint8_t, N> ub,
                                                       vector<uint8_t, N> &ur) {
  vector<uint16_t, N> _ua = ua;
  vector<uint16_t, N> _ub = ub;
  vector<uint16_t, N> _ur;
  vector<uint8_t, N> uq = __impl_udivrem(_ua, _ub, _ur);
  ur = _ur;
  return uq;
}

// RZ rounding mode is needed for some of the steps
// It is fine to use RZ mode throughout the FP computation
template <int N>
CM_NODEBUG CM_INLINE vector<uint32_t, N>
__impl_udivrem(vector<uint32_t, N> la, vector<uint32_t, N> lb,
               vector<uint32_t, N> &prem) {
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
  // RZ mode not required, used for convenience
  y += y * __impl_hex2float(0xb4c00000u);

  // Qh = ah*y
  Qh = ah * y;

  // Qh = (unsigned)Qh, with truncation
  lQh = Qh;

  // convert lQh back to SP, any rounding mode is fine
  Qh = lQh;

  // ah - bh*Qh
  Rh = ah - bh * Qh;

  // al - bl*Qh
  Rl = al - bl * Qh;

  // Ql = y*(Rh+Rl)
  Rl = Rh + Rl;
  Ql = y * Rl;

  // convert Ql to integer, with truncation
  lQl = Ql;

  // integer quotient
  lQ = lQh + lQl;

  // integer remainder
  lR = la - lb * lQ;

  // apply correction if needed
  // if (lR >= lb) { lQ++;  lR -= lb; }
  corr_mask = merge(0xffffffff, 0u, (lR >= lb));
  lQ += (corr_mask & 1);
  lR -= (lb & corr_mask);

  // remainder
  prem = lR;

  return lQ;
}

template <int N>
CM_NODEBUG CM_INLINE vector<uint64_t, N>
__impl_udivrem(vector<uint64_t, N> divider, vector<uint64_t, N> dividend,
               vector<uint64_t, N> &rem) {

  vector<uint64_t, N> one = 1ull;
  vector<uint64_t, N> negone = ~one;

  vector<uint64_t, N> quotient = 0ull;
  rem = 0ull;

  for (int64_t i = 63; i != -1; --i) {
    uint64_t vi = i;

    rem = rem << one;
    auto andedRem = rem & negone;
    auto shDivider = (divider >> vi) & one;
    rem = andedRem | shDivider;

    auto remGreaterEqDiv = rem >= dividend;
    rem = merge(rem - dividend, rem, remGreaterEqDiv);
    quotient = merge(quotient | (one << vi), quotient, remGreaterEqDiv);
  }
  return quotient;
}

template <int N>
CM_NODEBUG CM_INLINE vector<int64_t, N>
__impl_divrem(vector<int64_t, N> divider, vector<int64_t, N> dividend,
              vector<uint64_t, N> &ir) {

  vector<int64_t, N> zero = 0;
  vector<uint64_t, N> msbMask = 1ull << 63;

  vector<uint64_t, N> absDivider = merge(-divider, divider, divider < zero);
  vector<uint64_t, N> absDividend = merge(-dividend, dividend, dividend < zero);

  vector<uint64_t, N> uquotient = __impl_udivrem(absDivider, absDividend, ir);
  vector<int64_t, N> signBit = (divider ^ dividend) & msbMask;

  vector<int64_t, N> quotient = merge(-uquotient, uquotient, signBit != zero);
  ir = merge(-ir, ir, divider < zero);

  return quotient;
}

} // namespace details

#define __IDIV_SCALAR_IMPL(TYPE, REM_TYPE, N, RND)                             \
  CM_NODEBUG CM_NOINLINE extern "C" TYPE                                       \
      __cm_intrinsic_impl_sdiv__##RND##__s_##TYPE##_##REM_TYPE(TYPE a,         \
                                                               TYPE b) {       \
    vector<TYPE, N> _a = a;                                                    \
    vector<TYPE, N> _b = b;                                                    \
    vector<REM_TYPE, N> _r;                                                    \
    vector<TYPE, N> _q = details::__impl_divrem(_a, _b, _r);                   \
    return _q[0];                                                              \
  }

#define __IREM_SCALAR_IMPL(TYPE, REM_TYPE, N, RND)                             \
  CM_NODEBUG CM_NOINLINE extern "C" REM_TYPE                                   \
      __cm_intrinsic_impl_srem__##RND##__s_##REM_TYPE##_##TYPE(TYPE a,         \
                                                               TYPE b) {       \
    vector<TYPE, N> _a = a;                                                    \
    vector<TYPE, N> _b = b;                                                    \
    vector<REM_TYPE, N> _r;                                                    \
    details::__impl_divrem(_a, _b, _r);                                        \
    return _r[0];                                                              \
  }

#define __UDIV_SCALAR_IMPL(TYPE, REM_TYPE, N, RND)                             \
  CM_NODEBUG CM_NOINLINE extern "C" TYPE                                       \
      __cm_intrinsic_impl_udiv__##RND##__s_##REM_TYPE##_##TYPE(TYPE a,         \
                                                               TYPE b) {       \
    vector<TYPE, N> _a = a;                                                    \
    vector<TYPE, N> _b = b;                                                    \
    vector<REM_TYPE, N> _r;                                                    \
    vector<TYPE, N> _q = details::__impl_udivrem(_a, _b, _r);                  \
    return _q[0];                                                              \
  }

#define __UREM_SCALAR_IMPL(TYPE, REM_TYPE, N, RND)                             \
  CM_NODEBUG CM_NOINLINE extern "C" REM_TYPE                                   \
      __cm_intrinsic_impl_urem__##RND##__s_##REM_TYPE##_##TYPE(TYPE a,         \
                                                               TYPE b) {       \
    vector<TYPE, N> _a = a;                                                    \
    vector<TYPE, N> _b = b;                                                    \
    vector<REM_TYPE, N> _r;                                                    \
    details::__impl_udivrem(_a, _b, _r);                                       \
    return _r[0];                                                              \
  }

#define __IDIV_VECTOR_IMPL(TYPE, REM_TYPE, N, RND)                             \
  CM_NODEBUG CM_NOINLINE extern "C" cl_vector<TYPE, N>                         \
      __cm_intrinsic_impl_sdiv__##RND##__v##N##_##REM_TYPE##_##TYPE(           \
          cl_vector<TYPE, N> a, cl_vector<TYPE, N> b) {                        \
    vector<REM_TYPE, N> r;                                                     \
    vector<TYPE, N> q = details::__impl_divrem<N>(a, b, r);                    \
    return q.cl_vector();                                                      \
  }

#define __IREM_VECTOR_IMPL(TYPE, REM_TYPE, N, RND)                             \
  CM_NODEBUG CM_NOINLINE extern "C" cl_vector<REM_TYPE, N>                     \
      __cm_intrinsic_impl_srem__##RND##__v##N##_##REM_TYPE##_##TYPE(           \
          cl_vector<TYPE, N> a, cl_vector<TYPE, N> b) {                        \
    vector<REM_TYPE, N> r;                                                     \
    details::__impl_divrem<N>(a, b, r);                                        \
    return r.cl_vector();                                                      \
  }

#define __UDIV_VECTOR_IMPL(TYPE, REM_TYPE, N, RND)                             \
  CM_NODEBUG CM_NOINLINE extern "C" cl_vector<TYPE, N>                         \
      __cm_intrinsic_impl_udiv__##RND##__v##N##_##REM_TYPE##_##TYPE(           \
          cl_vector<TYPE, N> a, cl_vector<TYPE, N> b) {                        \
    vector<REM_TYPE, N> r;                                                     \
    vector<TYPE, N> q = details::__impl_udivrem<N>(a, b, r);                   \
    return q.cl_vector();                                                      \
  }

#define __UREM_VECTOR_IMPL(TYPE, REM_TYPE, N, RND)                             \
  CM_NODEBUG CM_NOINLINE extern "C" cl_vector<REM_TYPE, N>                     \
      __cm_intrinsic_impl_urem__##RND##__v##N##_##REM_TYPE##_##TYPE(           \
          cl_vector<TYPE, N> a, cl_vector<TYPE, N> b) {                        \
    vector<REM_TYPE, N> r;                                                     \
    details::__impl_udivrem<N>(a, b, r);                                       \
    return r.cl_vector();                                                      \
  }

#include "emulation_divrem_boilerplate.h"

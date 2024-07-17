/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include <cm-cl/math.h>
#include <cm-cl/vector.h>

#include "f64consts.h"

using namespace cm;

/***********************************************************
 *  Important: Set rounding mode to RNE (RTE in CM);
 *    restore rounding mode before each return statement
 *  (Directed rounding modes could lead to infinite loops)
 **********************************************************/

namespace {

template <int N>
CM_NODEBUG CM_INLINE vector<uint32_t, N> get_sign(vector<double, N> x) {
  auto x_split = x.template format<uint32_t>();
  vector<uint32_t, N> x_hi = x_split.template select<N, 2>(1);
  return x_hi & sign_32bit;
}

template <int N>
CM_NODEBUG CM_INLINE vector<uint32_t, N> get_exp(vector<double, N> x) {
  auto x_split = x.template format<uint32_t>();
  vector<uint32_t, N> x_hi = x_split.template select<N, 2>(1);
  return (x_hi >> exp_shift) & exp_mask;
}

template <int N>
CM_NODEBUG CM_INLINE vector<double, N> and_mantissa_loss(vector<double, N> x) {
  vector<uint32_t, 2 *N> x_split = x.template format<uint32_t>();
  vector<uint32_t, N> x_lo = x_split.template select<N, 2>(0);
  x_lo &= mantissa_32loss;
  x_split.template select<N, 2>(0) = x_lo;
  x = x_split.template format<double>();
  return x;
}

template <int N>
CM_NODEBUG CM_INLINE vector<double, N>
divide_by_mask(vector<double, N> a, vector<double, N> b, mask<N> filled_out) {
  auto local_a = a;
  local_a.merge(0, filled_out);
  auto local_b = b;
  local_b.merge(0, filled_out);
  vector<double, N> q =
      detail::__cm_cl_fdiv_ieee(local_a.cl_vector(), local_b.cl_vector());
  return q;
}

template <int N>
CM_NODEBUG CM_INLINE vector<double, N> round_to_int(vector<double, N> q) {
  vector<double, N> one = 1.0;
  vector<double, N> round = roundInt;
  vector<double, N> S = math::mad(q, one, round);
  q = math::mad(S, one, -round);
  return q;
}

template <int N>
CM_NODEBUG CM_INLINE vector<double, N> xor_sign(vector<double, N> a,
                                                vector<uint32_t, N> sign) {
  vector<uint32_t, 2 *N> a_split = a.template format<uint32_t>();
  vector<uint32_t, N> a_hi = a_split.template select<N, 2>(1);
  a_hi = a_hi ^ sign;
  a_split.template select<N, 2>(1) = a_hi;
  a = a_split.template format<double>();
  return a;
}

template <int N>
CM_NODEBUG CM_INLINE mask<N> check_is_nan_or_inf(vector<double, N> q) {
  auto q_split = q.template format<uint32_t>();
  vector<uint32_t, N> q_hi = q_split.template select<N, 2>(1);
  return (q_hi >= exp_32bitmask);
}

template <int N>
CM_NODEBUG CM_INLINE mask<N> check_is_nan(vector<double, N> q) {
  auto q_split = q.template format<uint32_t>();
  vector<uint32_t, N> q_lo = q_split.template select<N, 2>(0);
  vector<uint32_t, N> q_hi = q_split.template select<N, 2>(1);
  return (q_hi > exp_32bitmask) | (q_hi == exp_32bitmask & q_lo > 0);
}

template <int N>
CM_NODEBUG CM_INLINE void
__impl_rem_special_middle(vector<double, N> &a, vector<double, N> &b,
                          vector<double, N> &q, mask<N> filled_out) {
  auto bs = b * twoPow1023;
  auto bhalf2 = b * twoPow1022;

  q = divide_by_mask(a, bs, filled_out);
  mask<N> q_ge = check_is_nan_or_inf(q) & (filled_out == 0);
  if (q_ge.any()) {
    auto bs2 = bs * twoPow1023;
    vector<double, N> q_local = divide_by_mask(a, bs2, q_ge == 0);
    q_local = round_to_int(q_local);
    auto a_local = math::mad(-bs2, q_local, a);
    q_local = divide_by_mask(a_local, bs, q_ge == 0);
    a.merge(a_local, q_ge);
    q.merge(q_local, q_ge);
  }

  auto q_exp = get_exp(q);
  mask<N> q_exp_ge = (q_exp >= exp_32threshold_shift) & (filled_out == 0);
  do {
    auto q_local = and_mantissa_loss(q);
    // q is an integer
    // a - b*q
    auto a_local = math::mad(-bs, q_local, a);
    q_local = divide_by_mask(a_local, bs, q_exp_ge == 0);
    a.merge(a_local, q_exp_ge);
    q.merge(q_local, q_exp_ge);
    q_exp = get_exp(q_local);
    q_exp_ge &= (q_exp >= exp_32threshold_shift);
  } while (q_exp_ge.any());

  q = round_to_int(q);
  a = math::mad(-bs, q, a);

  vector<double, N> a_mod = math::absolute(a.cl_vector());
  mask<N> ge_bhalf = (a_mod > bhalf2) & (filled_out == 0);
  do {
    vector<double, N> q_local = divide_by_mask(a, bs, ge_bhalf == 0);
    // round q.f to integer
    q_local = round_to_int(q_local);
    // a - b*q
    auto a_local = math::mad(-bs, q_local, a);

    q.merge(q_local, ge_bhalf);
    a.merge(a_local, ge_bhalf);
    a_mod = math::absolute(a.cl_vector());
    ge_bhalf &= a_mod > bhalf2;
  } while (ge_bhalf.any());
  q = divide_by_mask(a, b, filled_out);
}

template <int N>
CM_NODEBUG CM_INLINE vector<double, N>
__impl_rem_body(vector<double, N> a, vector<double, N> b, vector<double, N> q,
                vector<uint32_t, N> sgn_a, mask<N> filled_out) {
  auto q_exp = get_exp(q);

  auto local_a = a;
  auto local_q = q;
  mask<N> q_exp_ge = (q_exp >= exp_32threshold_shift) & (filled_out == 0);
  do {
    local_q = and_mantissa_loss(q);
    local_a = math::mad(-b, local_q, a);
    auto local_b = b;
    local_q = divide_by_mask(local_a, local_b, q_exp_ge == 0);

    q_exp = get_exp(local_q);
    a.merge(local_a, q_exp_ge);
    q.merge(local_q, q_exp_ge);
    q_exp_ge &= (q_exp >= exp_32threshold_shift);
  } while (q_exp_ge.any());

  q = round_to_int(q);
  a = math::mad(-b, q, a);

  vector<double, N> local_abs_2a = math::absolute(a.cl_vector()) * 2;
  mask<N> fabs2gb = (local_abs_2a > b) & (filled_out == 0);
  local_a = a;
  local_q = q;
  do {
    auto local_b = b;
    local_q = divide_by_mask(local_a, local_b, fabs2gb == 0);
    local_q = round_to_int(local_q);
    local_a = math::mad(-b, local_q, local_a);

    q.merge(local_q, fabs2gb);
    a.merge(local_a, fabs2gb);
    local_abs_2a = math::absolute(a.cl_vector()) * 2;
    fabs2gb &= local_abs_2a > b;
  } while (fabs2gb.any());

  // apply sign
  a = xor_sign(a, sgn_a);

  return a;
}

template <int N>
CM_NODEBUG CM_INLINE vector<double, N>
__impl_rem_special(vector<double, N> a, vector<double, N> b,
                   vector<double, N> q, vector<double, N> y,
                   vector<uint32_t, N> sgn_a, mask<N> filled_out) {
  vector<double, N> result;
  // y==0 or x==Inf?
  mask<N> b_isZero = b == 0.0;
  result.merge(math::mad(-b, q, a), b_isZero);
  filled_out |= b_isZero;

  mask<N> a_isNan = check_is_nan_or_inf(a) & (filled_out == 0);
  result.merge(math::mad(-b, q, a), a_isNan);
  filled_out |= a_isNan;

  // y is NaN?
  auto b_exp = get_exp(b);
  mask<N> b_isNan = check_is_nan(b) & (filled_out == 0);
  result.merge(y + y, b_isNan);
  filled_out |= b_isNan;

  // q overflow
  vector<double, N> bhalf = b * 0.5f;
  // b * 0.5 underflows
  mask<N> isBhalf_uderflows = (bhalf == 0.0) & (filled_out == 0);
  if (isBhalf_uderflows.any()) {
    auto result_lo = result.template select<N, 2>(0);
    result_lo.merge(0, isBhalf_uderflows);
    result.template select<N, 2>(0) = result_lo;
    auto result_hi = result.template select<N, 2>(1);
    result_hi.merge(sgn_a, isBhalf_uderflows);
    result.template select<N, 2>(1) = result_hi;
  }
  filled_out |= isBhalf_uderflows;

  if (filled_out.all())
    return result;

  a.merge(0, filled_out);
  b.merge(0, filled_out);
  q.merge(0, filled_out);
  __impl_rem_special_middle(a, b, q, filled_out);
  result.merge(__impl_rem_body(a, b, q, sgn_a, filled_out), filled_out == 0);
  return result;
}

template <int N>
CM_NODEBUG CM_INLINE vector<double, N> __impl_rem_f64(vector<double, N> x,
                                                      vector<double, N> y) {

  vector<double, N> a = x;
  vector<double, N> b = y;

  vector<uint32_t, N> sgn_a = get_sign(a);
  a = math::absolute(a.cl_vector());
  b = math::absolute(b.cl_vector());

  // IEEE DIV, RN mode
  vector<double, N> q = detail::__cm_cl_fdiv_ieee(a.cl_vector(), b.cl_vector());
  // overflow, or special cases?
  vector<double, N> result = 0.0f;
  mask<N> filled_out;

  mask<N> special_case = get_exp(q) == exp_mask;

  if (special_case.any())
    result.merge(__impl_rem_special(a, b, q, y, sgn_a, special_case == 0),
                 special_case);

  filled_out = special_case;
  // fast exit for a <= b
  mask<N> fast_case = (a <= b) & (filled_out == 0);
  if (fast_case.any()) {
    // 2*a > b ?
    mask<N> double_ge = a * 2.0 > b;
    auto local_a = a;
    local_a.merge(a - b, double_ge);
    // using a*1.0 to flush denormal results to 0 in FTZ mode
    const vector<double, N> allones = 1.0f;
    const vector<double, N> zero = 0.0f;
    local_a = math::mad(local_a, allones, zero);
    result.merge(xor_sign(local_a, sgn_a), fast_case);
  }
  filled_out |= fast_case;
  if (filled_out.all())
    return result;

  result.merge(__impl_rem_body(a, b, q, sgn_a, filled_out), (filled_out == 0));
  return result;
}

} // namespace

CM_NODEBUG CM_NOINLINE extern "C" double __vc_builtin_frem_f64__rte_(double a,
                                                                     double b) {
  vector<double, 1> va = a;
  vector<double, 1> vb = b;
  return __impl_rem_f64(va, vb)[0];
}

#define FREM(WIDTH)                                                            \
  CM_NODEBUG CM_NOINLINE extern "C" cl_vector<double, WIDTH>                   \
      __vc_builtin_frem_v##WIDTH##f64__rte_(cl_vector<double, WIDTH> a,        \
                                            cl_vector<double, WIDTH> b) {      \
    vector<double, WIDTH> va{a};                                               \
    vector<double, WIDTH> vb{b};                                               \
    auto r = __impl_rem_f64(va, vb);                                           \
    return r.cl_vector();                                                      \
  }

FREM(1)
FREM(2)
FREM(4)
FREM(8)
/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "f32consts.h"
#include <cm-cl/math.h>
#include <cm-cl/vector.h>

using namespace cm;

namespace {

/***********************************************************
 *  Important: Set rounding mode to RNE;
 *    restore rounding mode before each return statement
 *  (Directed rounding modes could lead to infinite loops)
 **********************************************************/

template <int N>
CM_NODEBUG CM_INLINE vector<uint32_t, N> get_sign(vector<float, N> x) {
  vector<uint32_t, N> x_int = x.template format<uint32_t>();
  return x_int & sign_bit;
}

template <int N>
CM_NODEBUG CM_INLINE vector<uint32_t, N> get_exp(vector<float, N> x) {
  vector<uint32_t, N> x_int = x.template format<uint32_t>();
  return (x_int >> exp_shift) & exp_mask;
}

template <int N>
CM_NODEBUG CM_INLINE vector<float, N> xor_sign(vector<float, N> a,
                                               vector<uint32_t, N> sign) {
  vector<uint32_t, N> a_int = a.template format<uint32_t>();
  a_int = a_int ^ sign;
  return a_int.template format<float>();
}

template <int N>
CM_NODEBUG CM_INLINE mask<N> check_is_nan_or_inf(vector<float, N> q) {
  vector<uint32_t, N> q_int = q.template format<uint32_t>();
  return (q_int >= exp_bitmask);
}

template <int N> CM_NODEBUG CM_INLINE mask<N> check_is_nan(vector<float, N> q) {
  vector<uint32_t, N> q_int = q.template format<uint32_t>();
  return (q_int > exp_bitmask);
}

template <int N>
CM_NODEBUG CM_INLINE vector<float, N>
divide_by_mask(vector<float, N> a, vector<float, N> b, mask<N> filled_out) {
  auto local_a = a;
  local_a.merge(exp_bias << exp_shift, filled_out);
  if (filled_out.all())
    return local_a;
  auto local_b = b;
  local_b.merge(exp_bias << exp_shift, filled_out);
  vector<float, N> q =
      detail::__cm_cl_fdiv_ieee(local_a.cl_vector(), local_b.cl_vector());
  return q;
}

template <int N>
CM_NODEBUG CM_INLINE vector<float, N>
__impl_rem_body(vector<float, N> a, vector<float, N> b, vector<float, N> q,
                vector<uint32_t, N> sgn_a, mask<N> filled_out) {

  // round q.f to integer (use rnde instruction)
  q = math::roundne(q);
  // a - b*q
  a = math::mad(-b, q, a);

  vector<float, N> local_abs_2a = math::absolute(a.cl_vector()) * 2.0f;
  mask<N> fabs2gb = (local_abs_2a > b) & (filled_out == 0);
  // 2*fabs(a) > b
  while (1) {
    auto local_q = divide_by_mask(a, b, fabs2gb == 0);
    // round q.f to integer, using rnde instruction (RNE mode)
    local_q = math::roundne(local_q);
    // a - b*q
    auto local_a = math::mad(-b, local_q, a);
    q.merge(local_q, fabs2gb);
    a.merge(local_a, fabs2gb);
    local_abs_2a = math::absolute(a.cl_vector()) * 2.0f;
    fabs2gb &= local_abs_2a > b;
    if ((fabs2gb == 0).all())
      break;
  }

  // apply sign
  a = xor_sign(a, sgn_a);
  return a;
}

template <int N>
CM_NODEBUG CM_INLINE vector<float, N>
__impl_rem_special(vector<float, N> a, vector<float, N> b, vector<float, N> q,
                   vector<float, N> y, vector<uint32_t, N> sgn_a,
                   mask<N> in_mask) {
  vector<float, N> result;
  mask<N> filled_out = in_mask == 0;

  // y==0 or x==Inf?
  mask<N> b_isZero = (b == 0.0) & (filled_out == 0);
  result.merge(math::mad(-b, q, a), b_isZero);
  filled_out |= b_isZero;

  auto a_isNan = check_is_nan_or_inf(a) & (filled_out == 0);
  result.merge(math::mad(-b, q, a), a_isNan);
  filled_out |= a_isNan;

  // y is NaN?
  mask<N> b_isNan = check_is_nan(b) & (filled_out == 0);
  result.merge(y + y, b_isNan);
  filled_out |= b_isNan;

  if (filled_out.all())
    return result;

  // q.f overflow
  // b* 2*1023
  auto bs = b * twoPow127;
  auto bhalf2 = b * twoPow126;

  q = divide_by_mask(a, bs, filled_out);

  auto q_isNan = check_is_nan_or_inf(q) & (filled_out == 0);
  if (q_isNan.any()) {
    vector<float, N> bs2 = 1.0;
    // b* 2*127 * 2^127
    bs2.merge(bs * twoPow127, q_isNan);

    vector<float, N> local_a = 0;
    local_a.merge(a, q_isNan);

    auto local_q = divide_by_mask(local_a, bs2, q_isNan == 0);
    // round to integral
    local_q = math::roundne(local_q);
    local_a = math::mad(-bs2, local_q, local_a);
    vector<float, N> local_bs = 1.0;
    local_bs.merge(bs, q_isNan);
    local_q = divide_by_mask(local_a, local_bs, q_isNan == 0);
    a.merge(local_a, q_isNan);
    q.merge(local_q, q_isNan);
  }

  // round q.f to integer (use rnde instruction)
  q = math::roundne(q);
  // a - b*q
  a = math::mad(-bs, q, a);

  vector<float, N> a_mod = math::absolute(a.cl_vector());
  mask<N> ge_bhalf = (a_mod > bhalf2) & (filled_out == 0);

  vector<float, N> local_a = 0.0;
  vector<float, N> local_bs = 1.0;
  while (1) {
    local_a.merge(a, ge_bhalf);
    local_bs.merge(bs, ge_bhalf);
    vector<float, N> local_q = divide_by_mask(local_a, local_bs, ge_bhalf == 0);
    // round q.f to integer
    local_q = math::roundne(local_q);
    // a - b*q
    local_a = math::mad(-local_bs, local_q, local_a);

    a.merge(local_a, ge_bhalf);
    q.merge(local_q, ge_bhalf);
    vector<float, N> a_mod = math::absolute(a.cl_vector());
    ge_bhalf &= (a_mod > bhalf2) & ge_bhalf;
    if ((ge_bhalf == 0).all())
      break;
  }

  vector<float, N> local_q = divide_by_mask(a, b, filled_out);
  q.merge(local_q, filled_out == 0);

  result.merge(__impl_rem_body(a, b, q, sgn_a, filled_out), filled_out == 0);
  return result;
}

template <int N>
CM_NODEBUG CM_INLINE vector<float, N> __impl_rem_f32(vector<float, N> x,
                                                     vector<float, N> y) {

  vector<float, N> a = x;
  vector<float, N> b = y;
  vector<float, N> result = 123.0f;
  mask<N> filled_out = false;

  // sign of x
  auto sgn_a = get_sign(a);
  // |x|, |y|
  a = math::absolute(a.cl_vector());
  b = math::absolute(b.cl_vector());

  // IEEE DIV, RN mode
  vector<float, N> q = detail::__cm_cl_fdiv_ieee(a.cl_vector(), b.cl_vector());

  // overflow, or special cases
  auto q_exp = get_exp(q);
  mask<N> special_case = q_exp == exp_mask;

  if (special_case.any()) {
    // here we must copy a, b.. e.t.c to do not spoil values for future
    auto res_special = __impl_rem_special(a, b, q, y, sgn_a, special_case);
    result.merge(res_special, special_case);
    if (special_case.all())
      return result;
  }

  filled_out |= special_case;

  mask<N> fast_case = (a <= b) & (filled_out == 0);
  // fast exit for a <= b
  if (fast_case.any()) {
    mask<N> float_ge = a * 2.0f > b;
    auto local_a = a;
    if (float_ge.any())
      local_a.merge(a - b, float_ge & fast_case);

    const vector<float, N> one = 1.0f;
    const vector<float, N> zero = 0.0f;
    // using a*1.0f to flush denormal results to 0 in FTZ mode
    local_a.merge(math::mad(local_a, one, zero), fast_case);
    result.merge(xor_sign(local_a, sgn_a), fast_case);
    filled_out |= fast_case;
    if (filled_out.all())
      return result;
  }

  auto rem_body_res = __impl_rem_body(a, b, q, sgn_a, filled_out);
  result.merge(rem_body_res, (filled_out == 0));
  return result;
}

} // namespace

CM_NODEBUG CM_NOINLINE extern "C" float __vc_builtin_frem_f32__rte_(float a,
                                                                    float b) {
  vector<float, 1> va = a;
  vector<float, 1> vb = b;
  return __impl_rem_f32(va, vb)[0];
}

#define FREM(WIDTH)                                                            \
  CM_NODEBUG CM_NOINLINE extern "C" cl_vector<float, WIDTH>                    \
      __vc_builtin_frem_v##WIDTH##f32__rte_(cl_vector<float, WIDTH> a,         \
                                            cl_vector<float, WIDTH> b) {       \
    vector<float, WIDTH> va{a};                                                \
    vector<float, WIDTH> vb{b};                                                \
    auto r = __impl_rem_f32(va, vb);                                           \
    return r.cl_vector();                                                      \
  }

FREM(1)
FREM(2)
FREM(4)
FREM(8)
FREM(16)
FREM(32)
/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "f32consts.h"
#include <cm-cl/math.h>
#include <cm-cl/vector.h>

using namespace cm;

namespace {

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
CM_NODEBUG CM_INLINE vector<float, N> xor_sign(vector<float, N> a,
                                               uint32_t sign) {
  vector<uint32_t, N> a_int = a.template format<uint32_t>();
  vector<uint32_t, N> sign_v = sign;
  a_int = a_int ^ sign_v;
  return a_int.template format<float>();
}

template <int N>
CM_NODEBUG CM_INLINE mask<N> check_special(vector<float, N> a,
                                           vector<float, N> b) {
  mask<N> x_ge_fe = (get_exp(a) - 1) >= 0xfe;
  mask<N> y_ge_fe = (get_exp(b) - 1) >= 0xfe;
  return x_ge_fe | y_ge_fe;
}

template <int N> CM_NODEBUG CM_INLINE mask<N> check_is_nan(vector<float, N> q) {
  vector<float, N> q_abs = math::absolute(q.cl_vector());
  vector<uint32_t, N> q_int = q_abs.template format<uint32_t>();
  return (q_int > exp_bitmask);
}

template <int N>
CM_NODEBUG CM_INLINE mask<N> check_is_denormal(vector<float, N> q) {
  vector<uint32_t, N> q_int = q.template format<uint32_t>();
  return ((q_int & exp_bitmask) == 0);
}

template <int N>
CM_NODEBUG CM_INLINE vector<float, N> normalize_exp(vector<float, N> a) {
  // normalize (scale by 2^32)
  vector<uint32_t, N> local_s = normalize_bitmask;
  vector<float, N> local_s_f = local_s.template format<float>();
  auto local_a = a * local_s_f;
  return local_a;
}

template <int N>
CM_NODEBUG CM_INLINE static vector<int32_t, N>
get_exp_diff(vector<float, N> a, vector<float, N> b) {
  vector<float, N> a_abs = math::absolute(a);
  vector<float, N> b_abs = math::absolute(b);
  vector<int32_t, N> a_int = a_abs.template format<int32_t>();
  vector<int32_t, N> b_int = b_abs.template format<int32_t>();
  vector<int32_t, N> diff = a_int - b_int;
  diff = diff >> exp_shift;
  return diff;
}

template <int N>
CM_NODEBUG CM_NOINLINE static cl_vector<float, N>
__impl_div_ieee_step_7__rtz_(cl_vector<float, N> cr1, cl_vector<float, N> cy3,
                             cl_vector<float, N> cq1) {
  vector<float, N> r1 = cr1, y3 = cy3, q1 = cq1;
  return math::mad(r1, y3, q1).cl_vector();
}

template <int N>
CM_NODEBUG CM_INLINE vector<float, N> cut_mantissa_and_sign(vector<float, N> a,
                                                            unsigned or_data) {
  vector<uint32_t, N> a_int = a.template format<uint32_t>();
  auto mantissa_and_sign = a_int & (mantissa_bitmask | sign_bit);
  mantissa_and_sign |= or_data;
  return mantissa_and_sign.template format<float>();
}

template <int N>
CM_NODEBUG CM_INLINE vector<float, N> cut_mantissa(vector<float, N> a) {
  vector<uint32_t, N> a_int = a.template format<uint32_t>();
  auto mantissa = a_int & mantissa_bitmask;
  return mantissa.template format<float>();
}

template <int N>
CM_NODEBUG CM_INLINE vector<float, N> scaling_factor(vector<int32_t, N> a_int) {
  a_int += exp_bias;
  a_int = a_int << exp_shift;
  return a_int.template format<float>();
}

template <int N>
CM_NODEBUG CM_INLINE vector<float, N> or_with_pred(vector<float, N> a,
                                                   mask<N> mask) {
  vector<uint32_t, N> a_int = a.template format<uint32_t>();
  vector<uint32_t, N> mask_or = 0x1;
  mask_or.merge(0x0, mask);
  a_int |= mask_or;
  return a_int.template format<float>();
}

template <int N>
CM_NODEBUG CM_INLINE vector<uint32_t, N> or_with_pred(vector<uint32_t, N> a_int,
                                                      mask<N> mask) {
  vector<uint32_t, N> mask_or = 0x1;
  mask_or.merge(0x0, mask);
  a_int |= mask_or;
  return a_int;
}

template <int N>
CM_NODEBUG CM_INLINE vector<float, N>
__impl_fdiv_ieee_special_div(vector<float, N> a, vector<float, N> b,
                             mask<N> &filled_out) {
  vector<float, N> result;

  vector<float, N> a_abs = math::absolute(a.cl_vector());
  vector<float, N> b_abs = math::absolute(b.cl_vector());

  auto a_isNan = check_is_nan(a_abs);
  // if first input is NaN, return the quietized NaN
  result.merge(a + b, a_isNan);
  filled_out |= a_isNan;

  // if the second input is NaN, return the quietized NaN
  auto b_isNan = check_is_nan(b_abs) & (filled_out == 0);
  result.merge(b + b, b_isNan);
  filled_out |= b_isNan;

  // sign bits
  auto sgn_x = get_sign(a);
  auto sgn_y = get_sign(b);

  auto exp_a = get_exp(a);
  // a is Inf?
  auto a_isInf = (exp_a == exp_mask) & (filled_out == 0);
  // return NaN_Indef if y is also Inf;  Inf with proper sign otherwise
  result.merge(xor_sign(a, sgn_y) - xor_sign(b, sgn_x), a_isInf);
  filled_out |= a_isInf;

  auto exp_b = get_exp(b);
  // b is Inf?
  auto b_isInf = (exp_b == exp_mask) & (filled_out == 0);
  vector<float, N> sgn_y_float = sgn_y.template format<float>();
  result.merge(a * sgn_y_float, b_isInf);
  filled_out |= b_isInf;

  // b is 0?
  mask<N> b_isZero = (b_abs == 0.0f) & (filled_out == 0);
  if (b_isZero.any()) {
    mask<N> a_isZero = (a_abs == 0.0f);
    auto mq = xor_sign(b, exp_bitmask);
    mq.merge(xor_sign(mq, sgn_x), (a_isZero == 0));
    mq.merge(a * mq, a_isZero);
    result.merge(mq, b_isZero);
    filled_out |= b_isZero;
  }
  // a is 0?
  mask<N> a_isZero = (a_abs == 0.0f) & (filled_out == 0);
  result.merge(a * b, a_isZero);
  filled_out |= a_isZero;

  return result;
}

template <int N>
CM_NODEBUG CM_INLINE vector<float, N>
__impl_fdiv_ieee_undeflow_case(vector<float, N> mr1, vector<float, N> y1,
                               vector<float, N> q1, vector<int32_t, N> itmp1) {
  // this path is reached for gradual underflow only
  vector<float, N> result;
  // Step(7), q=q1+r1*y1, but in RZ mode
  vector<float, N> mq = __impl_div_ieee_step_7__rtz_(
      mr1.cl_vector(), y1.cl_vector(), q1.cl_vector());

  // is normal quotient exact?
  // test whether r1==0
  // auto inexact_bit = ((mr1.w << 1)) ? 1 : 0;
  mask<N> r1_is_zero = mr1 == 0.0f;

  // shift amount is -(ediff5+126)=-itmp1
  // shift_amount >= 1
  auto shift_amount = -itmp1;

  // if shift_amount>1, then inexact bit is OR-ed into sticky bit (last mantissa
  // bit) for round-to-nearest and shift_amount==1, last mantissa bit is a round
  // bit and inexact_bit is used to complete the rounding
  mask<N> ammount_ge1 = (shift_amount > 1) | r1_is_zero;
  if (ammount_ge1.any()) {
    // signed mantissa of quotient
    auto mq_u_local = cut_mantissa_and_sign(mq, exp_float_min);
    mq_u_local = or_with_pred(mq_u_local, (r1_is_zero == 0));
    // perform scaling in user rounding mode
    mq_u_local = mq_u_local * scaling_factor(itmp1);
    result.merge(mq_u_local, ammount_ge1);
  }

  // mantissa
  auto mq_u = cut_mantissa(mq);
  auto sgn_q = get_sign(mq);
  // will shift result by 2

  vector<uint32_t, N> mq_u_uint = mq_u.template format<uint32_t>();
  mq_u_uint = (mq_u_uint << 1) | sgn_q;
  mq_u_uint = or_with_pred(mq_u_uint, (r1_is_zero == 0));

  vector<uint32_t, N> s_e2 = 0x3e800000;
  auto s_e1 = sgn_q | exp_float_min;

  mask<N> mq_u_isDenormal = (mq_u_uint & exp_float_min) == 0;
  vector<uint32_t, N> s_e3 = 0x00c00000;
  // ensure mq_u.f is not denormal (so it is not flushed to zero)
  s_e3.merge(0x00a00000, mq_u_isDenormal);
  mq_u_uint |= exp_float_min;
  // add sign to shifted leading bit + correction
  s_e3 |= sgn_q;
  vector<float, N> s_e3_float = s_e3.template format<float>();
  mq = mq_u_uint.template format<float>();

  vector<float, N> s_e2_f = s_e2.template format<float>();
  mq = math::mad(mq, s_e2_f, s_e3_float);
  // eliminate leading bit (but UF flag will not be set)
  vector<float, N> s_e1_float = s_e1.template format<float>();
  mq = mq - s_e1_float;

  // force UF flag to be set
  s_e1_float *= s_e1_float;
  vector<uint32_t, N> mq_u_int = s_e1_float.template format<uint32_t>();

  // artificial dependency
  vector<uint32_t, N> mq_uint = mq.template format<uint32_t>();
  mq_uint |= (mq_u_int & mantissa_loss);
  mq = mq_uint.template format<float>();
  result.merge(mq, ammount_ge1 == 0);
  return result;
}

template <int N>
CM_NODEBUG CM_INLINE vector<float, N>
__impl_fdiv_ieee_long_path(vector<float, N> a, vector<float, N> b) {

  vector<float, N> result;
  vector<int32_t, N> ediff5;

  auto expon_x = get_exp(a);
  auto expon_y = get_exp(b);

  // filter out Inf/NaN, zeroes, denormals
  mask<N> special_div = check_special(a, b);
  mask<N> filled_out = false;
  if (special_div.any()) {
    auto special_div_res = __impl_fdiv_ieee_special_div(a, b, filled_out);
    result.merge(special_div_res, filled_out);
    if (filled_out.all())
      return result;
    special_div &= (filled_out == 0);
    // for denormal inputs, zeroes, NaNs, Inf
    // a denormal, or b denormal
    // initialize scale exponents
    vector<uint32_t, N> isx = 0;
    vector<uint32_t, N> isy = 0;

    auto a_isDenormal = check_is_denormal(a) & special_div;
    if (a_isDenormal.any()) {
      a.merge(normalize_exp(a), a_isDenormal);
      expon_x.merge(get_exp(a) - 32, a_isDenormal);
      isx.merge(32, a_isDenormal);
    }
    auto b_isDenormal = check_is_denormal(b) & special_div;
    if (b_isDenormal.any()) {
      b.merge(normalize_exp(b), b_isDenormal);
      expon_y.merge(get_exp(b) - 32, b_isDenormal);
      isy.merge(32, b_isDenormal);
    }
    // used to detect gradual underflow
    ediff5.merge(get_exp_diff(a, b) - isx + isy, special_div);
  }
  // signed mantissas, needed for long path computation
  // return to long computation path
  vector<float, N> ma = cut_mantissa_and_sign(a, exp_float_zero);
  vector<float, N> mb = cut_mantissa_and_sign(b, exp_float_zero);

  ediff5.merge(get_exp_diff(a, b), (special_div == 0));
  vector<uint32_t, N> ediff = expon_x - expon_y;

  // will check whether quotient is in gradual underflow range
  vector<int32_t, N> itmp1 = ediff5 + 126;
  vector<int32_t, N> itmp2 = ediff5 + 126 + 25;
  vector<int32_t, N> ig_uf = itmp1 & (~itmp2);
  mask<N> gradual_underflow = ig_uf < 0;

  // perform division on signed mantissas:  ma.f/mb.f
  auto y0 = math::reciprocal(mb);
  // Step(1), q0=a*y0
  auto q0 = ma * y0;
  // Step(2), e0=(1-b*y0)
  const vector<float, N> one = 1.0f;
  auto e0 = math::mad(-mb, y0, one);
  // Step(3), y1=y0+e0*y0
  auto y1 = math::mad(e0, y0, y0);
  // Step(4), r0=a-b*q0
  auto r0 = math::mad(-mb, q0, ma);
  // Step(5), q1=q0+r0*y1
  auto q1 = math::mad(r0, y1, q0);
  // Step(6), r1=a-b*q1
  auto mr1 = math::mad(-mb, q1, ma);

  auto isUnderfow = gradual_underflow & (filled_out == 0);
  // take special path for gradual underflow cases
  if (isUnderfow.any())
    result.merge(__impl_fdiv_ieee_undeflow_case(mr1, y1, q1, itmp1),
                 isUnderfow);
  filled_out |= isUnderfow;

  // Step(7), q=q1+r1*y1
  auto mq = math::mad(mr1, y1, q1);

  // scale by 2^exponent_q
  // split exponent difference into smaller parts (so that scale factors can be
  // represented in SP format) three scale factors are needed when inputs may be
  // denormal limit range of e_diff, so that two scale factors are sufficient
  // (to avoid incorrect flag settings)
  mask<N> fix_exp = (ediff + 126) > (126 * 2 + 126);
  if (fix_exp.any()) {
    vector<int32_t, N> sgn_e = ediff.template format<int32_t>();
    sgn_e.merge(0, fix_exp == 0);
    sgn_e = sgn_e >> 11;
    // set e_diff to 124*2 with proper sign
    ediff.merge(sgn_e ^ (124 + 124 + sgn_e), fix_exp);
  }

  vector<int32_t, N> se_diff = ediff.template format<int32_t>();
  auto es1 = (se_diff) >> 1;
  auto es2 = se_diff - es1;

  // q*2^es1 (will be a normal value)
  vector<uint32_t, N> umq = mq.template format<uint32_t>();
  vector<uint32_t, N> ues1 = es1.template format<uint32_t>();
  umq += (ues1 << exp_shift);
  // one more scale factor
  auto s_e2 = scaling_factor(es2);
  mq = umq.template format<float>();
  mq *= s_e2;

  result.merge(mq, (filled_out == 0));
  return result;
}

template <int N>
CM_NODEBUG CM_INLINE vector<float, N> __impl_fdiv_ieee(vector<float, N> a,
                                                       vector<float, N> b) {

  vector<float, N> result;
  // biased exponents
  auto expon_x = get_exp(a);
  auto expon_y = get_exp(b);

  // check exponent ranges
  // Main path will be taken for expon_x in [bias-62, bias+63] and expon_y in
  // [bias-63, bias+63]
  auto exp_x_bias = math::absolute((expon_x + 62 - exp_bias).cl_vector());
  auto exp_y_bias = math::absolute((expon_y + 63 - exp_bias).cl_vector());
  mask<N> x_long_path = exp_x_bias >= (64 + 62);
  mask<N> y_long_path = exp_y_bias >= (64 + 63);
  mask<N> long_path = x_long_path | y_long_path;

  if (long_path.any())
    result.merge(__impl_fdiv_ieee_long_path(a, b), long_path);

  const vector<float, N> one = 1.0f;
  // fast DIV path
  auto y0 = math::reciprocal(b);
  // Step(1), q0=a*y0
  auto q0 = a * y0;
  // Step(2), e0=(1-b*y0)
  auto e0 = math::mad(-b, y0, one);
  // Step(3), y1=y0+e0*y0
  auto y1 = math::mad(e0, y0, y0);
  // Step(4), r0=a-b*q0
  auto r0 = math::mad(-b, q0, a);
  // Step(5), q1=q0+r0*y1
  auto q1 = math::mad(r0, y1, q0);
  // Step(6), r1=a-b*q1
  auto r1 = math::mad(-b, q1, a);
  // Step(7), q=q1+r1*y1
  // Set user rounding mode here (i.e. rmode)
  auto q = math::mad(r1, y1, q1);
  result.merge(q, long_path == 0);

  return result;
}

} // namespace

CM_NODEBUG CM_NOINLINE extern "C" float __vc_builtin_fdiv_f32(float a,
                                                              float b) {
  vector<float, 1> va = a;
  vector<float, 1> vb = b;
  return __impl_fdiv_ieee(va, vb)[0];
}

#define FDIV(WIDTH)                                                            \
  CM_NODEBUG CM_NOINLINE extern "C" cl_vector<float, WIDTH>                    \
      __vc_builtin_fdiv_v##WIDTH##f32(cl_vector<float, WIDTH> a,               \
                                      cl_vector<float, WIDTH> b) {             \
    vector<float, WIDTH> va{a};                                                \
    vector<float, WIDTH> vb{b};                                                \
    auto r = __impl_fdiv_ieee(va, vb);                                         \
    return r.cl_vector();                                                      \
  }

FDIV(1)
FDIV(2)
FDIV(4)
FDIV(8)
FDIV(16)
FDIV(32)
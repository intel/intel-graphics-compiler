/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include <cm-cl/math.h>
#include <cm-cl/vector.h>

#include "f64consts.h"

using namespace cm;

int __cm_cl_TargetSupportsIEEE;

namespace {

template <int N>
CM_NODEBUG CM_INLINE mask<N> check_is_nan_or_inf(vector<double, N> q) {
  vector<uint32_t, 2 * N> q_split = q.template format<uint32_t>();
  vector<uint32_t, N> q_hi = q_split.template select<N, 2>(1);
  return (q_hi >= exp_32bitmask);
}

template <int N>
CM_NODEBUG CM_INLINE vector<uint32_t, N> get_exp(vector<double, N> x) {
  vector<uint32_t, 2 * N> x_split = x.template format<uint32_t>();
  vector<uint32_t, N> x_hi = x_split.template select<N, 2>(1);
  return (x_hi >> exp_shift) & exp_mask;
}

template <int N>
CM_NODEBUG CM_INLINE vector<uint32_t, N> get_sign(vector<double, N> x) {
  vector<uint32_t, 2 * N> x_split = x.template format<uint32_t>();
  vector<uint32_t, N> x_hi = x_split.template select<N, 2>(1);
  return x_hi & sign_32bit;
}

template <int N> CM_NODEBUG CM_INLINE mask<N> is_denormal(vector<double, N> x) {
  vector<uint32_t, 2 * N> x_int = x.template format<uint32_t>();
  vector<uint32_t, N> x_hi = x_int.template select<N, 2>(1);
  return x_hi < min_sign_exp;
}

template <int N>
CM_NODEBUG CM_INLINE vector<uint32_t, N> sep_exp(vector<double, N> x) {
  vector<uint32_t, 2 * N> x_int = x.template format<uint32_t>();
  vector<uint32_t, N> x_hi = x_int.template select<N, 2>(1);
  vector<uint32_t, N> res = (x_hi >> exp_shift) - exp_bias;
  return res >> 1;
}

template <int N>
CM_NODEBUG CM_INLINE vector<double, N> fill_hi_part(vector<uint32_t, N> in) {
  vector<double, N> res = 0;
  vector<uint32_t, 2 * N> res_int = res.template format<uint32_t>();
  res_int.template select<N, 2>(1) = in;
  res = res_int.template format<double>();
  return res;
}

template <int N>
CM_NODEBUG CM_INLINE vector<double, N> call_cl_sqrt(vector<double, N> x) {
  vector<float, N> res = x;
  res = detail::__cm_cl_sqrt(res.cl_vector(), false);
  x = res;
  return x;
}

template <int N>
CM_NODEBUG CM_INLINE vector<double, N> invert_float(vector<double, N> x) {
  vector<float, N> res = x;
  res = math::reciprocal(res);
  x = res;
  return x;
}

template <int N>
CM_NODEBUG CM_INLINE vector<double, N> rsqrt_float(vector<double, N> x) {
  vector<float, N> res = x;
  res = detail::__cm_cl_rsqrt(res.cl_vector());
  x = res;
  return x;
}

template <int N>
CM_NODEBUG CM_INLINE vector<double, N> uint64_sub_hi(vector<double, N> x,
                                                     vector<uint32_t, N> hi) {
  vector<uint32_t, 2 * N> ex_mx_int = 0;
  ex_mx_int.template select<N, 2>(1) = hi;
  vector<uint64_t, N> ex_u64 = ex_mx_int.template format<uint64_t>();
  vector<uint64_t, N> mx_u64 = x.template format<uint64_t>();
  mx_u64 -= ex_u64;
  x = mx_u64.template format<double>();
  return x;
}

template <int N>
CM_NODEBUG CM_INLINE vector<double, N> math_rsqt_dp(vector<double, N> x) {
  // special inputs are treated outside this call
  vector<double, N> mx = math::absolute(x);
  // eliminate sign bit (x<0 is not supposed to be an input to this call,
  // anyway)

  // scale denormal inputs
  mask<N> isDenormal = is_denormal(mx);
  vector<double, N> scale_x = 1.0;
  scale_x.merge(twoPow64, isDenormal);
  vector<double, N> scale_res = 1.0;
  scale_res.merge(twoPow32, isDenormal);

  mx = mx * scale_x;

  // separate exponent
  vector<uint32_t, N> ex = sep_exp(mx);

  // scaled mantissa
  vector<uint32_t, N> ex_mx_int_hi = ex << (53 - 32);
  mx = uint64_sub_hi(mx, ex_mx_int_hi);
  // rsqrt(mx)
  mx = rsqrt_float(mx);

  ex_mx_int_hi = ex << (52 - 32);
  mx = uint64_sub_hi(mx, ex_mx_int_hi);

  return mx * scale_res;
}

template <int N>
CM_NODEBUG CM_INLINE vector<double, N> sqrt_special(vector<double, N> a) {
  vector<double, N> x = a;
  vector<double, N> result = 0;
  mask<N> filled_out = false;

  // exponent fields
  auto ex = get_exp(x);
  auto sgn_x = get_sign(x);

  mask<N> is_Nan_Or_Inf = (ex == exp_mask);

  if (is_Nan_Or_Inf.any()) {
    mask<N> Minus_Inf = (sgn_x == sign_32bit);
    filled_out |= (Minus_Inf == 0) & is_Nan_Or_Inf;
    // NaN or +Inf?
    result.merge(a + a, filled_out);
  }

  mask<N> isZero = (x == 0.0) & (filled_out == 0);
  if (isZero.any()) {
    auto result_hi = result.template select<N, 2>(1);
    result_hi.merge(sgn_x, isZero);
    filled_out |= isZero;
    result.template select<N, 2>(1) = result_hi;
  }

  // negative input
  vector<uint32_t, N> inf_hi_part = inf_hi;
  vector<double, N> res = fill_hi_part(inf_hi_part);
  vector<double, N> y = fill_hi_part(sgn_x);

  result.merge(res * y, (filled_out == 0));

  return result;
}

template <int N>
CM_NODEBUG CM_INLINE vector<double, N> calc_sqrt(vector<double, N> x,
                                                 mask<N> special) {
  // Now start the SQRT computation
  // Use math.rsqtm (emulated here)
  vector<double, N> y0 = math_rsqt_dp(x);
  // predicate is set for 0, neg a, Inf, NaN inputs
  y0.merge(sqrt_special(x), special);

  return y0;
}

template <int N>
CM_NODEBUG CM_INLINE vector<double, N> sqrt_late_steps(vector<double, N> a,
                                                       vector<double, N> y0) {
  // IEEE SQRT computes H0 = 0.5*y0 (can be skipped)
  // Step 3: S0 = a*y0
  vector<double, N> S0 = a * y0;
  // IEEE SQRT, step 4:  d = 0.5 - S0*H0 = 0.5*(1 - S0*y0)
  // Here we compute dx2 = 2*d = 1 - S0*y0 instead
  // Proof comments: y0 = math.rsqtm(a) = 1/sqrt(a)*(1+e0), |e0|<2^(-23)
  //                 S0 = a*y0 = sqrt(a)*(1+e0)*(1+e1),      |e1| < 2^(-53)
  //   dx2 = 1 - (1+e0)*(1+e0)*(1+e1) = -2*e0 - e0*e0 - e1*(1+e0)*(1+e0) =
  //   -2*e0 - e0*e0 - eps, |eps|<2^(-52.999)
  vector<double, N> one = 1.0f;
  vector<double, N> dx2 = math::mad(S0, -y0, one);
  // IEEE SQRT, step 5:  e = 1 + 1.5*d = 1 + 0.75*dx2
  // We compute ehalf = e*0.5 = 0.5 + 0.375*dx2 (so that ehalf*dx2 == e*d)
  vector<double, N> three_eighths = 0.375f;
  vector<double, N> one_half = 0.5f;
  vector<double, N> ehalf = math::mad(dx2, three_eighths, one_half);
  // IEEE SQRT, step 6:  e = e*d
  // We compute  ehalf*dx2 = (e*0.5)*(2*d) = e*d (end up with the same value
  // at this step) Proof comments:  ehalf*dx2 =
  // (0.5-0.75*(e0+e0*e0/2+eps/2))*(-2*e0 - e0*e0 - eps) =
  //   = (1.5*(e0+e0*e0/2+eps/2)-1)*(e0+e0*e0/2+eps/2)
  vector<double, N> ed = ehalf * dx2;
  // IEEE SQRT, step 7:  H1 = H0 + ed*H0= (0.5*y0) + ed*(0.5*y0) produces
  // 0.5*INVSQRT(a) accurate to 1 ulp We compute result = y0 + ed*y0 Proof
  // comments:  result = rsqrt(a)*(1+e0)*(1 - (e0+e0*e0/2+eps/2)
  // + 1.5*(e0+e0*e0/2+eps/2)^2) =
  //  =
  //  rsqrt(a)*(1+e0-(e0+e0*e0/2+eps/2)-(e0^2+e0^3/2+e0*eps/2)+1.5*(e0^2+e0^3+O(2^(-74))+1.5*(e0^3+e0^4+O(2^(-97)))
  //  = = rsqrt(a)*(1-eps/2 + O(e0^3) + O(2^(-73)) =
  //  rsqrt(a)*(1+O(2^(-53.99)) before final rounding
  vector<double, N> res = math::mad(y0, ed, y0);
  return res;
}

template <int N>
CM_NODEBUG CM_INLINE vector<double, N> __impl_rsqrt_f64(vector<double, N> x) {

  vector<double, N> a = x;
  vector<double, N> result = 0;

  if (__cm_cl_TargetSupportsIEEE) {
    // Fast path for targets with rsqtm/invm instructions
    auto mrsqrt_res = detail::mrsqrt(a.cl_vector());
    mask<N> special_case = mrsqrt_res.second;
    vector<double, N> y0 = mrsqrt_res.first;

    result.merge(sqrt_late_steps(a, y0), (special_case == 0));

    vector<double, N> one = 1.0f;
    auto invm = detail::invm(one.cl_vector(), y0.cl_vector());
    result.merge(invm.first, special_case);
    return result;
  }

  mask<N> special_case = (x <= 0.0f) | check_is_nan_or_inf(x);

  // Save user rounding mode here, set new rounding mode to RN  (RTE in CM)
  vector<double, N> y0 = calc_sqrt(a, special_case);

  result.merge(sqrt_late_steps(a, y0), (special_case == 0));

  // This is the path for special inputs (INVM can be used to cover DP inputs)
  result.merge(invert_float(y0), special_case);

  return result;
}

} // namespace

CM_NODEBUG CM_NOINLINE extern "C" double
__vc_builtin_rsqrt_f64__rte_(double a) {
  vector<double, 1> va = a;
  return __impl_rsqrt_f64(va)[0];
}

#define RSQRT(WIDTH)                                                           \
  CM_NODEBUG CM_NOINLINE extern "C" cl_vector<double, WIDTH>                   \
      __vc_builtin_rsqrt_v##WIDTH##f64__rte_(cl_vector<double, WIDTH> a) {     \
    vector<double, WIDTH> va{a};                                               \
    auto r = __impl_rsqrt_f64(va);                                             \
    return r.cl_vector();                                                      \
  }

RSQRT(1)
RSQRT(2)
RSQRT(4)
RSQRT(8)
RSQRT(16)
RSQRT(32)

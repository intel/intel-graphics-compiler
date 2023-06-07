/*========================== begin_copyright_notice ============================

Copyright (C) 2022-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include <cm-cl/math.h>
#include <cm-cl/vector.h>

using namespace cm;

namespace {
// We have to use 32-bit integers when it's possible
constexpr unsigned exp_shift = 52 - 32;
constexpr unsigned exp_mask = 0x7ff;
constexpr unsigned exp_bias = 0x3ff;
constexpr unsigned exp_invmask = ~(exp_mask << exp_shift);

constexpr unsigned nan_hi = 0x7ff80000;
constexpr unsigned inf_hi = 0x7ff00000;

template <bool NNaN, bool NInf, bool NSZ, int N>
CM_NODEBUG CM_INLINE vector<double, N>
__impl_fdiv_special(vector<double, N> a, vector<double, N> b) {
  auto x = a.template format<uint32_t>();
  auto y = b.template format<uint32_t>();

  vector<uint32_t, N> x_lo = x.template select<N, 2>(0);
  vector<uint32_t, N> x_hi = x.template select<N, 2>(1);
  vector<uint32_t, N> y_lo = y.template select<N, 2>(0);
  vector<uint32_t, N> y_hi = y.template select<N, 2>(1);

  vector<uint32_t, N> x_exp = (x_hi >> exp_shift) & exp_mask;
  vector<uint32_t, N> y_exp = (y_hi >> exp_shift) & exp_mask;

  vector<uint32_t, N> x_sgn = x_hi & (1u << 31);
  vector<uint32_t, N> y_sgn = y_hi & (1u << 31);

  vector<uint32_t, N * 2> result = 0;
  auto result_lo = result.template select<N, 2>(0);
  auto result_hi = result.template select<N, 2>(1);

  auto ex_x_max = x_exp == 0x7ff;
  auto ex_y_max = y_exp == 0x7ff;

  if constexpr (!NInf) // Inf / y == Inf
    result_hi.merge(x_sgn ^ y_sgn | inf_hi, ex_x_max);

  auto x_is_nan = ex_x_max & ((x_lo != 0) | ((x_hi & 0x000fffff) != 0));

  if constexpr (!NNaN) // NaN || Inf / Inf == NaN
    result_hi.merge(nan_hi, ex_x_max & (x_is_nan | ex_y_max));

  auto y_special = (ex_x_max == 0) & ex_y_max;

  if constexpr (!NSZ) // x / Inf == 0
    result_hi.merge(x_sgn ^ y_sgn, y_special);

  if constexpr (!NNaN) { // x / NaN == NaN
    auto y_is_nan = (y_lo != 0) | ((y_hi & 0x000fffff) != 0);
    result_hi.merge(nan_hi, y_special & y_is_nan);
  }

  mask<N> x_hi_is_zero = x_hi == x_sgn;
  mask<N> x_lo_is_zero = x_lo == 0;
  mask<N> x_is_zero = x_hi_is_zero & x_lo_is_zero;
  mask<N> y_hi_is_zero = y_hi == y_sgn;
  mask<N> y_lo_is_zero = y_lo == 0;
  mask<N> y_is_zero = y_hi_is_zero & y_lo_is_zero;

  if constexpr (!NSZ) // 0 / y == 0
    result_hi.merge(x_sgn ^ y_sgn, x_is_zero & (y_special == 0));

  if constexpr (!NInf) // x / 0 == Inf
    result_hi.merge(x_sgn ^ y_sgn | inf_hi, (x_is_nan == 0) & y_is_zero);

  if constexpr (!NNaN) // 0 / 0 == NaN
    result_hi.merge(nan_hi, x_is_zero & y_is_zero);

  return result.template format<double>();
}

template <bool NNaN, bool NInf, bool NSZ, int N>
CM_NODEBUG CM_INLINE vector<double, N> __impl_fdiv_fast(vector<double, N> a,
                                                        vector<double, N> b) {
  auto saved_a = a;
  auto saved_b = b;

  auto x = a.template format<uint32_t>();
  auto y = b.template format<uint32_t>();

  auto x_lo = x.template select<N, 2>(0);
  auto x_hi = x.template select<N, 2>(1);
  auto y_lo = y.template select<N, 2>(0);
  auto y_hi = y.template select<N, 2>(1);

  vector<int32_t, N> x_exp =
      (vector<uint32_t, N>(x_hi) >> exp_shift) & exp_mask;
  vector<int32_t, N> y_exp =
      (vector<uint32_t, N>(y_hi) >> exp_shift) & exp_mask;

  // Check exponent ranges. Main path has to be taken for x_exp in
  // [bias-832, bias+896) and [bias-126, bias+126). No overflow/underflow
  // occures for any main path step.
  mask<N> x_long_path = (x_exp + 832 - exp_bias) >= (832 + 896);
  mask<N> y_long_path = (y_exp + 126 - exp_bias) >= (126 + 126);
  mask<N> long_path = x_long_path | y_long_path;

  vector<double, N> scale0 = 0.5, scale1 = 0.0, scale2 = 0.0;

  // Long path, scale is needed
  if (long_path.any()) {
    constexpr double two64 = 0x1p+64;
    constexpr double twom64 = 0x1p-64;

    // Handle subnormal a
    mask<N> x_unorm = x_exp == 0;
    if (x_unorm.any()) {
      a.merge(a * two64, x_unorm);
      x_exp = (vector<uint32_t, N>(x_hi) >> exp_shift) & exp_mask;
      // if exp is still 0, we have zero or FTZ enabled
      scale0.merge(scale0 * twom64, x_unorm & (x_exp != 0));
    }

    // Handle subnormal b
    mask<N> y_unorm = y_exp == 0;
    if (y_unorm.any()) {
      b.merge(b * two64, y_unorm);
      y_exp = (vector<uint32_t, N>(y_hi) >> exp_shift) & exp_mask;
      // if exp is still 0, we have zero or FTZ enabled
      scale0.merge(scale0 * two64, y_unorm & (y_exp != 0));
    }

    auto exp_diff = x_exp - y_exp + 0x7ff;

    auto scale1_hi =
        scale1.template format<uint32_t>().template select<N, 2>(1);
    auto scale2_hi =
        scale2.template format<uint32_t>().template select<N, 2>(1);

    vector<uint32_t, N> scale1_exp = exp_diff >> 1;
    vector<uint32_t, N> scale2_exp = exp_diff - scale1_exp;

    scale1_hi = scale1_exp << exp_shift;
    scale2_hi = scale2_exp << exp_shift;

    auto scale0_w_hi =
        scale0.template format<uint32_t>().template select<N, 2>(1);

    vector<uint32_t, N * 2> ma;
    ma.template select<N, 2>(0) = x_lo;
    vector<uint32_t, N> tmpa = vector<uint32_t, N>(scale0_w_hi) |
                               (vector<uint32_t, N>(x_hi) & exp_invmask);
    ma.template select<N, 2>(1) = tmpa;

    vector<uint32_t, N * 2> mb;
    mb.template select<N, 2>(0) = y_lo;
    vector<uint32_t, N> tmpb =
        (exp_bias << exp_shift) | (vector<uint32_t, N>(y_hi) & exp_invmask);
    mb.template select<N, 2>(1) = tmpb;

    a.merge(ma.template format<double>(), long_path);
    b.merge(mb.template format<double>(), long_path);
  }

  // Should be mapped to math.inv
  vector<float, N> bf = b;
  vector<float, N> y0f = 1.0f / bf;
  vector<double, N> y0 = y0f;

  // step(1): e0 = 1.0 - b * y0
  vector<double, N> one = 1.0;
  auto e0 = math::mad(-b, y0, one);
  // step(2)
  auto q0 = a * y0;
  // step(3): e1 = e0 + e0 * e0
  auto e1 = math::mad(e0, e0, e0);
  // step(4)
  auto qe = q0 * e1;

  // step(5): q = a * y0 + qe
  auto q = math::mad(a, y0, qe);

  if (long_path.any()) {
    auto qscaled = q;
    qscaled = q * scale1;
    qscaled *= scale2;
    q.merge(qscaled, long_path);

    vector<uint32_t, N> x_uexp = x_exp;
    vector<uint32_t, N> y_uexp = y_exp;
    auto special_case = ((x_uexp - 1) >= 0x7fe) | ((y_uexp - 1) >= 0x7fe);

    if (special_case.any()) {
      q.merge(__impl_fdiv_special<NNaN, NInf, NSZ>(saved_a, saved_b),
              special_case);
    }
  }

  return q;
}

template <int N>
CM_NODEBUG CM_NOINLINE static cl_vector<double, N * 3>
__impl_div_ieee_steps__rte_(cl_vector<double, N> ca, cl_vector<double, N> cb) {
  vector<double, N> a = ca;
  vector<double, N> b = cb;

  vector<float, N> bf = b;
  vector<float, N> y0f = 1.0f / bf; // Should be mapped to math.inv
  vector<double, N> y0 = y0f;

  // Because the double-to-float conversion is done in RN mode, we need this fix
  // to ensure corner cases round correctly
  auto y0_lo = y0.template format<uint32_t>().template select<N, 2>(0);
  y0_lo = vector<uint32_t, N>(y0_lo) | 1;

  vector<double, N> one = 1.0;

  // step(1)
  auto q0 = a * y0;
  // step(2): e0 = 1.0 - b * y0
  auto e0 = math::mad(b, -y0, one);
  // step(3): r0 = a - b * q0
  auto r0 = math::mad(b, -q0, a);
  // step(4): y1 = y0 + e0 * y0
  auto y1 = math::mad(y0, e0, y0);
  // step(5): e1 = 1.0 - b * y1
  auto e1 = math::mad(b, -y1, one);
  // step(6): y2 = y0 + e0 * y1
  auto y2 = math::mad(e0, y1, y0);
  // step(7): q1 = q0 + r0 * y1
  auto q1 = math::mad(r0, y1, q0);
  // step(8): y3 = y1 + e1 * y2
  auto y3 = math::mad(e1, y2, y1);
  // step(9): r1 = a - b * q1
  auto r1 = math::mad(b, -q1, a);

  vector<double, N * 3> out;
  out.template select<N, 1>(0 * N) = q1;
  out.template select<N, 1>(1 * N) = y3;
  out.template select<N, 1>(2 * N) = r1;

  return out.cl_vector();
}

template <int N>
CM_NODEBUG CM_NOINLINE static cl_vector<double, N>
__impl_div_ieee_step_10__rtz_(cl_vector<double, N> cr1,
                              cl_vector<double, N> cy3,
                              cl_vector<double, N> cq1) {
  vector<double, N> r1 = cr1, y3 = cy3, q1 = cq1;
  return math::mad(r1, y3, q1).cl_vector();
}

template <bool NNaN, bool NInf, bool NSZ, int N>
CM_NODEBUG CM_INLINE vector<double, N> __impl_fdiv_ieee(vector<double, N> a,
                                                        vector<double, N> b) {
  auto saved_a = a;
  auto saved_b = b;

  auto x = a.template format<uint32_t>();
  auto y = b.template format<uint32_t>();

  auto x_lo = x.template select<N, 2>(0);
  auto x_hi = x.template select<N, 2>(1);
  auto y_lo = y.template select<N, 2>(0);
  auto y_hi = y.template select<N, 2>(1);

  vector<int32_t, N> x_exp =
      (vector<uint32_t, N>(x_hi) >> exp_shift) & exp_mask;
  vector<int32_t, N> y_exp =
      (vector<uint32_t, N>(y_hi) >> exp_shift) & exp_mask;

  vector<int32_t, N> g_ediff = 0;

  // Check exponent ranges. Main path has to be taken for x_exp in
  // [bias-896, bias+896) and [bias-126, bias+126). No overflow/underflow
  // occures for any main path step.
  mask<N> x_long_path = (x_exp + 896 - exp_bias) >= (896 + 896);
  mask<N> y_long_path = (y_exp + 126 - exp_bias) >= (126 + 126);
  mask<N> long_path = x_long_path | y_long_path;

  vector<double, N> scale0 = 0.5, scale1 = 0.0, scale2 = 0.0;

  // Long path, scale is needed
  if (long_path.any()) {
    constexpr double two64 = 0x1p+64;
    constexpr double twom64 = 0x1p-64;

    // Handle subnormal a
    mask<N> x_unorm = x_exp == 0;
    if (x_unorm.any()) {
      a.merge(a * two64, x_unorm);
      x_exp = (vector<uint32_t, N>(x_hi) >> exp_shift) & exp_mask;
      // if exp is still 0, we have zero or FTZ enabled
      scale0.merge(scale0 * twom64, x_unorm & (x_exp != 0));
    }

    // Handle subnormal b
    mask<N> y_unorm = y_exp == 0;
    if (y_unorm.any()) {
      b.merge(b * two64, y_unorm);
      y_exp = (vector<uint32_t, N>(y_hi) >> exp_shift) & exp_mask;
      // if exp is still 0, we have zero or FTZ enabled
      scale0.merge(scale0 * two64, y_unorm & (y_exp != 0));
    }

    auto exp_diff = x_exp - y_exp + 0x7ff;

    auto scale1_hi =
        scale1.template format<uint32_t>().template select<N, 2>(1);
    auto scale2_hi =
        scale2.template format<uint32_t>().template select<N, 2>(1);

    vector<uint32_t, N> scale1_exp = exp_diff >> 1;
    vector<uint32_t, N> scale2_exp = exp_diff - scale1_exp;

    scale1_hi = scale1_exp << exp_shift;
    scale2_hi = scale2_exp << exp_shift;

    auto scale0_w_hi =
        scale0.template format<uint32_t>().template select<N, 2>(1);

    vector<uint32_t, N * 2> ma;
    ma.template select<N, 2>(0) = x_lo;
    vector<uint32_t, N> tmpa = vector<uint32_t, N>(scale0_w_hi) |
                               (vector<uint32_t, N>(x_hi) & exp_invmask);
    ma.template select<N, 2>(1) = tmpa;

    vector<uint32_t, N * 2> mb;
    mb.template select<N, 2>(0) = y_lo;
    vector<uint32_t, N> tmpb =
        (exp_bias << exp_shift) | (vector<uint32_t, N>(y_hi) & exp_invmask);
    mb.template select<N, 2>(1) = tmpb;

    a.merge(ma.template format<double>(), long_path);
    b.merge(mb.template format<double>(), long_path);

    // g_ediff value is needed to detect gradual underflow
    vector<double, N> abs_a = detail::__cm_cl_abs_float(a.cl_vector());
    vector<double, N> abs_b = detail::__cm_cl_abs_float(b.cl_vector());

    vector<int64_t, N> i_abs_a = abs_a.template format<int64_t>();
    vector<int64_t, N> i_abs_b = abs_b.template format<int64_t>();

    // The substruction has to be emulated
    auto idiff = i_abs_a - i_abs_b;
    vector<int32_t, N> idiff_hi =
        idiff.template format<int32_t>().template select<N, 2>(1);
    auto exp_diff2 = idiff_hi >> exp_shift; // arith shift is expected
    g_ediff.merge(exp_diff + exp_diff2, long_path);
  }

  // step(0..9)
  vector<double, N * 3> out =
      __impl_div_ieee_steps__rte_(a.cl_vector(), b.cl_vector());
  vector<double, N> q1 = out.template select<N, 1>(0);
  vector<double, N> y3 = out.template select<N, 1>(N);
  vector<double, N> r1 = out.template select<N, 1>(2 * N);

  // step(10): q = q1 + r1 * y3
  auto q = math::mad(r1, y3, q1);

  if (long_path.any()) {
    mask<N> gradual_underflow =
        (g_ediff < 2046 - 1022) & (g_ediff >= 2046 - 1078);

    // step(10) for gradual underflow case
    if (gradual_underflow.any()) {
      q.merge(__impl_div_ieee_step_10__rtz_(r1.cl_vector(), y3.cl_vector(),
                                            q1.cl_vector()),
              gradual_underflow);
    }

    auto qscaled = q;
    qscaled = q * scale1;

    // gradual underflow, shift amount is 1
    mask<N> last_case = 0;

    if (gradual_underflow.any()) {
      // gradual underflow, normal result is inexact
      mask<N> r1_is_not_zero = r1 != 0.0;
      mask<N> gediff_is_magic = g_ediff == 2046 - 1023;
      mask<N> result_is_inexact =
          gradual_underflow & r1_is_not_zero & (gediff_is_magic == 0);
      auto qscaled_lo =
          qscaled.template format<uint32_t>().template select<N, 2>(0);
      qscaled_lo.merge(vector<uint32_t, N>(qscaled_lo) | 1, result_is_inexact);

      // gradual underflow, shift amount is 1
      last_case = gradual_underflow & r1_is_not_zero & gediff_is_magic;
      if (last_case.any()) {
        vector<double, N> uq = qscaled;
        auto uqi = uq.template format<uint32_t>();
        auto uqi_lo = uqi.template select<N, 2>(0);
        auto uqi_hi = uqi.template select<N, 2>(1);

        vector<double, N> uq1 = 0.0;
        auto uq1i = uq1.template format<uint32_t>();
        auto uq1i_lo = uq1i.template select<N, 2>(0);
        auto uq1i_hi = uq1i.template select<N, 2>(1);

        uq1i_hi = vector<uint32_t, N>(uqi_hi) & 0xfff00000u;
        uq = uq - uq1;

        // add sticky bit and preserve sign after previous sub
        uqi_hi = (vector<uint32_t, N>(uqi_hi) & ~(1u << 31)) |
                 (vector<uint32_t, N>(uq1i_hi) & (1u << 31));
        uqi_lo = vector<uint32_t, N>(uqi_lo) | 1;

        qscaled.merge(uq, last_case);
      }
    }

    qscaled *= scale2;

    auto qscaled_hi =
        qscaled.template format<uint32_t>().template select<N, 2>(1);
    qscaled_hi.merge(vector<uint32_t, N>(qscaled_hi) + 0x00080000u, last_case);

    q.merge(qscaled, long_path);

    vector<uint32_t, N> x_uexp = x_exp;
    vector<uint32_t, N> y_uexp = y_exp;
    auto special_case = ((x_uexp - 1) >= 0x7fe) | ((y_uexp - 1) >= 0x7fe);

    if (special_case.any()) {
      q.merge(__impl_fdiv_special<NNaN, NInf, NSZ>(saved_a, saved_b),
              special_case);
    }
  }

  return q;
}
} // namespace

CM_NODEBUG CM_NOINLINE extern "C" double __vc_builtin_fdiv_f64(double a,
                                                               double b) {
  vector<double, 1> va = a;
  vector<double, 1> vb = b;
  return __impl_fdiv_ieee<false, false, false>(va, vb)[0];
}

CM_NODEBUG CM_NOINLINE extern "C" double __vc_builtin_fdiv_fast_f64(double a,
                                                                    double b) {
  vector<double, 1> va = a;
  vector<double, 1> vb = b;
  return __impl_fdiv_fast<false, false, false>(va, vb)[0];
}

#define FDIV(WIDTH)                                                            \
  CM_NODEBUG CM_NOINLINE extern "C" cl_vector<double, WIDTH>                   \
      __vc_builtin_fdiv_v##WIDTH##f64(cl_vector<double, WIDTH> a,              \
                                      cl_vector<double, WIDTH> b) {            \
    vector<double, WIDTH> va{a};                                               \
    vector<double, WIDTH> vb{b};                                               \
    auto r = __impl_fdiv_ieee<false, false, false>(va, vb);                    \
    return r.cl_vector();                                                      \
  }                                                                            \
  CM_NODEBUG CM_NOINLINE extern "C" cl_vector<double, WIDTH>                   \
      __vc_builtin_fdiv_fast_v##WIDTH##f64(cl_vector<double, WIDTH> a,         \
                                           cl_vector<double, WIDTH> b) {       \
    vector<double, WIDTH> va{a};                                               \
    vector<double, WIDTH> vb{b};                                               \
    auto r = __impl_fdiv_fast<false, false, false>(va, vb);                    \
    return r.cl_vector();                                                      \
  }

FDIV(1)
FDIV(2)
FDIV(4)
FDIV(8)

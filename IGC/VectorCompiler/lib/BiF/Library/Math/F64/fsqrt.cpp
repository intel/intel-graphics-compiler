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

constexpr unsigned nan_hi = 0x7ff80000;
constexpr unsigned inf_hi = 0x7ff00000;

template <bool NNaN, bool NInf, bool NSZ, int N>
CM_NODEBUG CM_INLINE vector<double, N>
__impl_fsqrt_special(vector<double, N> x) {
  vector<uint32_t, N * 2> result = 0;
  auto result_lo = result.template select<N, 2>(0);
  auto result_hi = result.template select<N, 2>(1);

  auto xi = x.template format<uint32_t>();
  vector<uint32_t, N> x_lo = xi.template select<N, 2>(0);
  vector<uint32_t, N> x_hi = xi.template select<N, 2>(1);

  vector<uint32_t, N> exp = (x_hi >> exp_shift) & exp_mask;
  vector<uint32_t, N> sign = x_hi & (1u << 31);

  mask<N> is_pinf = (x_hi == inf_hi) & (x_lo == 0);
  mask<N> is_nan = (is_pinf == 0) & (exp == exp_mask);

  if constexpr (!NInf)
    result_hi.merge(inf_hi, is_pinf);

  if constexpr (!NNaN)
    result_hi.merge(nan_hi, is_nan | (sign != 0));

  if constexpr (NSZ)
    result_hi.merge(vector<uint32_t, N>(0), x == 0.0);
  else
    result_hi.merge(sign, x == 0.0);

  return result.template format<double>();
}

template <int N>
CM_NODEBUG CM_NOINLINE cl_vector<double, N * 3>
__impl_fsqrt_ieee_steps__rte_(cl_vector<double, N> vx) {
  vector<double, N> x = vx;

  // Should be mapped to math.rsqt
  vector<float, N> xf = x;
  vector<float, N> y0f = detail::__cm_cl_rsqrt(xf.cl_vector());
  vector<double, N> y0 = y0f;

  vector<double, N * 3> result;
  auto s1 = result.template select<N, 1>(0 * N);
  auto h1 = result.template select<N, 1>(1 * N);
  auto d1 = result.template select<N, 1>(2 * N);

  auto h0 = 0.5 * y0;
  auto s0 = x * y0;
  auto d = math::mad(s0, -h0, vector<double, N>(0.5));
  auto e = math::mad(d, vector<double, N>(1.5), vector<double, N>(1.0));
  e *= d;
  s1 = math::mad(s0, e, s0);
  h1 = math::mad(h0, e, h0);

  vector<double, N> vs1 = s1;
  d1 = math::mad(vs1, -vs1, x);

  return result.cl_vector();
}

template <bool IEEE, bool NNaN, bool NInf, bool NSZ, int N>
CM_NODEBUG CM_INLINE vector<double, N> __impl_fsqrt(vector<double, N> a) {
  vector<double, N> x = a;
  auto xi = x.template format<uint32_t>();
  vector<uint32_t, N> x_hi = xi.template select<N, 2>(1);

  vector<uint32_t, N> es = x_hi >> (exp_shift + 1);

  vector<double, N> sc0 = 0.0, sc1 = 0.0;
  auto sc0_hi = sc0.template format<uint32_t>().template select<N, 2>(1);
  auto sc1_hi = sc1.template format<uint32_t>().template select<N, 2>(1);

  sc0_hi = (0x3ff + 0x1ff - es) << exp_shift;
  sc1_hi = (0x200 + es) << exp_shift;

  // prescaling
  x *= sc0;
  x *= sc0;

  vector<double, N> s;
  if constexpr (IEEE) {
    vector<double, N * 3> result = __impl_fsqrt_ieee_steps__rte_(x.cl_vector());
    vector<double, N> s1 = result.template select<N, 1>(0 * N);
    vector<double, N> h1 = result.template select<N, 1>(1 * N);
    vector<double, N> d1 = result.template select<N, 1>(2 * N);

    s = math::mad(h1, d1, s1);
  } else { // Fast algorithm, 1ULP
    // Should be mapped to math.rsqt
    vector<float, N> xf = x;
    vector<float, N> y0f = detail::__cm_cl_rsqrt(xf.cl_vector());
    vector<double, N> y0 = y0f;

    auto h0 = -0.5 * y0;
    auto s0 = x * y0;
    auto e = math::mad(s0, h0, vector<double, N>(0.5));
    auto s1 = math::mad(s0, e, s0);
    auto d1 = math::mad(s1, s1, -x);

    s = math::mad(h0, d1, s1);
  }

  // final scaling
  s *= sc1;

  mask<N> special = (a == 0.0) | (x_hi >= inf_hi);

  if (special.any())
    s.merge(__impl_fsqrt_special<NNaN, NInf, NSZ>(a), special);

  return s;
}

constexpr bool _fast = false;
constexpr bool _ieee = true;

constexpr bool _nnan = true;
constexpr bool _ninf = true;
constexpr bool _nsz = true;
constexpr bool _ = false;

} // namespace

CM_NODEBUG CM_NOINLINE extern "C" double __vc_builtin_fsqrt_f64(double a) {
  vector<double, 1> va = a;
  return __impl_fsqrt<true, false, false, false>(va)[0];
}

CM_NODEBUG CM_NOINLINE extern "C" double __vc_builtin_fsqrt_fast_f64(double a) {
  vector<double, 1> va = a;
  return __impl_fsqrt<false, false, false, false>(va)[0];
}

#define FSQRT(WIDTH)                                                           \
  CM_NODEBUG CM_NOINLINE extern "C" cl_vector<double, WIDTH>                   \
      __vc_builtin_fsqrt_v##WIDTH##f64(cl_vector<double, WIDTH> a) {           \
    vector<double, WIDTH> va{a};                                               \
    auto r = __impl_fsqrt<true, false, false, false>(va);                      \
    return r.cl_vector();                                                      \
  }                                                                            \
  CM_NODEBUG CM_NOINLINE extern "C" cl_vector<double, WIDTH>                   \
      __vc_builtin_fsqrt_fast_v##WIDTH##f64(cl_vector<double, WIDTH> a) {      \
    vector<double, WIDTH> va{a};                                               \
    auto r = __impl_fsqrt<false, false, false, false>(va);                     \
    return r.cl_vector();                                                      \
  }

FSQRT(1)
FSQRT(2)
FSQRT(4)
FSQRT(8)

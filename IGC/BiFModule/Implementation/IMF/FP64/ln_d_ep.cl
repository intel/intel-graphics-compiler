/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dln_ep___c9 = {0xbfc11e70a5c9b8f8uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dln_ep___c8 = {0x3fc20827ee8835feuL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dln_ep___c7 = {0xbfbedf6494cc1f86uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dln_ep___c6 = {0x3fc1e531c40397e0uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dln_ep___c5 = {0xbfc55d72615e74d3uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dln_ep___c4 = {0x3fc99dac1eadbf8euL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dln_ep___c3 = {0xbfcfffcff3489b95uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dln_ep___c2 = {0x3fd5554dfa222c07uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dln_ep___c1 = {0xbfe00000145aea06uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dln_ep___c0 = {0x3e30eba0cce2e0b0uL};
__attribute__((always_inline)) inline int
__ocl_svml_internal_dln_ep(double *a, double *r) {
  int nRet = 0;
  union {
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
  } x, expon, expon_r, one, l2;
  double R, d_expon;
  union {
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
  } denorm_scale;
  double poly, res;
  union {
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
  } _res;
  int denorm_scale_exp;
  x.f = *a;
  // special branch for +/-0, negatives, +INFs, +NaNs
  if ((x.w == 0x0uL) || (x.w >= 0x7ff0000000000000uL)) {
    // x = +/-0
    if ((x.w & 0x7fffffffffffffff) == 0x0uL) {
      nRet = 2;
      _res.w = 0xfff0000000000000uL;
      *r = _res.f;
      return nRet;
    }
    // x = any negative
    else if (x.w > 0x8000000000000000uL) {
      nRet = 1;
      _res.w = x.w | 0xfff8000000000000uL;
      *r = _res.f;
      return nRet;
    }
    // x = +NaN or +INF
    else {
      // x = +NaN
      if (x.w > 0x7ff0000000000000uL) {
        _res.f = x.f + x.f;
      }
      // x = +INF
      else {
        _res.w = x.w;
      }
      *r = _res.f;
      return nRet;
    } // x = +NaN or +INF
  }   // special branch for +/-0, negatives, +INFs, +NaNs
      // scale denormals
  denorm_scale.w = 0x43B0000000000000ull;
  denorm_scale_exp = (x.w <= 0x000fffffffffffffuL) ? (60 + 0x3FF) : 0x3FF;
  x.f = (x.w <= 0x000fffffffffffffuL) ? (x.f * denorm_scale.f) : x.f;
  // argument reduction to (-1/3, 1/3)
  // reduced exponent
  expon.w = x.w + 0x000AAAAAAAAAAAAAull;
  expon.w >>= 52;
  expon_r.w = expon.w << 52;
  // reduced mantissa
  one.w = 0x3FF0000000000000ull;
  x.w = (x.w + one.w) - expon_r.w;
  // reduced argument:  reduced_mantissa - 1.0
  R = x.f - one.f;
  // polynomial
  poly = __spirv_ocl_fma(__dln_ep___c9.f, R,
                                                __dln_ep___c8.f);
  poly = __spirv_ocl_fma(poly, R, __dln_ep___c7.f);
  poly = __spirv_ocl_fma(poly, R, __dln_ep___c6.f);
  poly = __spirv_ocl_fma(poly, R, __dln_ep___c5.f);
  poly = __spirv_ocl_fma(poly, R, __dln_ep___c4.f);
  poly = __spirv_ocl_fma(poly, R, __dln_ep___c3.f);
  poly = __spirv_ocl_fma(poly, R, __dln_ep___c2.f);
  poly = __spirv_ocl_fma(poly, R, __dln_ep___c1.f);
  poly = __spirv_ocl_fma(poly, R, __dln_ep___c0.f);
  // prepare exponent
  // scale back denormals
  expon.s32[0] -= denorm_scale_exp;
  // exponent
  d_expon = (double)expon.s32[0];
  // full polynomial = log(1+R)
  poly = __spirv_ocl_fma(poly, R, R);
  // result:  reduced_exponent*log(2)+log(1+R)
  l2.w = 0x3FE62E42FEFA39EFull;
  res = __spirv_ocl_fma(d_expon, l2.f, poly);
  *r = res;
  return nRet;
}
double __ocl_svml_log_ep(double x) {
  double r;
  __ocl_svml_internal_dln_ep(&x, &r);
  return r;
}

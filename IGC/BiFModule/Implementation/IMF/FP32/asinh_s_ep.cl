/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF
static __constant union {
  unsigned int w;
  float f;
  int i;
} __sasinh_ep_large_x = {0x49800000u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __sasinh_ep_small_x = {0x39800000u};
// largest norm
static __constant union {
  unsigned int w;
  float f;
  int i;
} __sasinh_ep_largest_norm = {0x7f7fffffu};
// log(2)
static __constant union {
  unsigned int w;
  float f;
  int i;
} __sasinh_ep_ln2 = {0x3f317218u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __sasinh_ep_c4 = {0x3e1103e9u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __sasinh_ep_c3 = {0xbe84f69cu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __sasinh_ep_c2 = {0x3ead39b3u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __sasinh_ep_c1 = {0xbefff0d2u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __sasinh_ep_c0 = {0x3f7ffcc1u};
// 2^(-6)
static __constant union {
  unsigned int w;
  float f;
  int i;
} __sasinh_ep_small2_x = {0x3c800000u};
__attribute__((always_inline)) inline int
__ocl_svml_internal_sasinh_ep(float *a, float *r) {
  int nRet = 0;
  float x = *a;
  float z2h, Sh, RS, E;
  float poly, R;
  union {
    unsigned int w;
    float f;
    int i;
  } Yh, res, xin, sgn, xa, two_expon;
  int expon, e23, iexpon_corr;
  z2h = __spirv_ocl_fma(x, x, 1.0f);
  RS = 1.0f / __spirv_ocl_sqrt(z2h);
  Sh = __spirv_ocl_fma(z2h, RS, 0.0f);
  xa.f = __spirv_ocl_fabs(x);
  // |x| + Sh + Sl
  Yh.f = xa.f + Sh;
  // set Yh, Yl for large |x|
  // will use exponent correction in log computation, for large x
  Yh.f = (xa.f < __sasinh_ep_large_x.f) ? Yh.f : xa.f * 0.5f;
  // fixup needed for x near largest normal
  iexpon_corr = (xa.f < __sasinh_ep_large_x.f) ? 0 : 2;
  // expon(Yh) + 2
  expon = ((Yh.w + 0x00400000) >> 23) - 0x7f;
  // new expon
  e23 = expon << 23;
  // Yh * 2^(-expon-2)
  Yh.w -= e23;
  // reduced argument
  R = Yh.f - 1.0f;
  // add exponent correction
  expon += iexpon_corr;
  // polynomial
  poly = __spirv_ocl_fma(__sasinh_ep_c4.f, R,
                                                __sasinh_ep_c3.f);
  poly = __spirv_ocl_fma(poly, R, __sasinh_ep_c2.f);
  poly = __spirv_ocl_fma(poly, R, __sasinh_ep_c1.f);
  xin.f = x;
  sgn.w = xin.w ^ xa.w;
  poly *= R;
  poly = __spirv_ocl_fma(poly, R, R);
  res.f = __spirv_ocl_fma(((float)expon),
                                                 __sasinh_ep_ln2.f, poly);
  res.w ^= sgn.w;
  // fixup for small or Inf/NaN
  res.f =
      ((xa.f < __sasinh_ep_small2_x.f) | (xa.w > __sasinh_ep_largest_norm.w))
          ? (x + sgn.f)
          : res.f;
  *r = res.f;
  return nRet;
}
float __ocl_svml_asinhf_ep(float x) {
  float r;
  __ocl_svml_internal_sasinh_ep(&x, &r);
  return r;
}

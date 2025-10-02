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
} __sasinh_ha_large_x = {0x49800000u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __sasinh_ha_small_x = {0x39800000u};
// largest norm
static __constant union {
  unsigned int w;
  float f;
  int i;
} __sasinh_ha_largest_norm = {0x7f7fffffu};
// log(2)
static __constant union {
  unsigned int w;
  float f;
  int i;
} __sasinh_ha_ln2l = {0xb102e308u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __sasinh_ha_ln2h = {0x3f317218u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __sasinh_ha_c9 = {0xbd3bc2cau};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __sasinh_ha_c8 = {0x3dd8bd42u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __sasinh_ha_c7 = {0xbe075e7fu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __sasinh_ha_c6 = {0x3e1445e9u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __sasinh_ha_c5 = {0xbe2a6712u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __sasinh_ha_c4 = {0x3e4cb1a3u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __sasinh_ha_c3 = {0xbe800059u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __sasinh_ha_c2 = {0x3eaaaae2u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __sasinh_ha_c1 = {0xbf000000u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __sasinh_ha_c0 = {0x3f800000u};
static __constant float __sasinh_ha_fc0[] = {
    0x1.fffffep-1, 0x1.dcd7b4p-25, // HI + LO: 0.99999994 + 5.5511784e-08
                                   // [0x3f7fffff + 0x336e6bda]
};
static __constant float __sasinh_ha_fln2[] = {
    0x1.62e42ep-1, 0x1.efa39ep-25, // HI + LO: 0.69314712 + 5.7699989e-08
                                   // [0x3f317217 + 0x3377d1cf]
};
__attribute__((always_inline)) inline int
__ocl_svml_internal_sasinh_ha(float *a, float *r) {
  int nRet = 0;
  float x = *a;
  float x2h, z2h, x2l, z2l, A, B, Bh, Sh, S0h, Sl, RS, E, Yhh;
  float Bl, poly, R, Rl, R0, exponf;
  union {
    unsigned int w;
    float f;
    int i;
  } Yh, Yl, res, xin, sgn, xa, two_expon;
  int expon, e23, iexpon_corr;
  x2h = __spirv_ocl_fma(x, x, 0.0f);
  z2h = x2h + 1.0f;
  A = __spirv_ocl_fmax(x2h, 1.0f);
  B = __spirv_ocl_fmin(x2h, 1.0f);
  x2l = __spirv_ocl_fma(x, x, (-x2h));
  Bh = z2h - A;
  Bl = B - Bh;
  z2l = x2l + Bl;
  RS = 1.0f / __spirv_ocl_sqrt(z2h);
  S0h = __spirv_ocl_fma(z2h, RS, 0.0f);
  // rsqrt(z2h)*0.5
  RS *= 0.5f;
  // (1+x^2) - Sh^2
  E = __spirv_ocl_fma((-S0h), S0h, z2h);
  E = E + z2l;
  // sqrt(1+x^2)_low
  Sl = __spirv_ocl_fma(E, RS, 0.0f);
  Sh = S0h + Sl;
  Yhh = Sh - S0h;
  Sl = Sl - Yhh;
  xa.f = __spirv_ocl_fabs(x);
  // |x| + Sh + Sl
  Yh.f = xa.f + Sh;
  Yhh = Yh.f - Sh;
  Yl.f = xa.f - Yhh;
  Yl.f = Yl.f + Sl;
  // set Yh, Yl for large |x|
  // will use exponent correction in log computation, for large x
  Yh.f = (xa.f < __sasinh_ha_large_x.f) ? Yh.f : xa.f * 0.5f;
  Yl.f = (xa.f < __sasinh_ha_large_x.f) ? Yl.f : 0;
  // fixup needed for x near largest normal
  iexpon_corr = (xa.f < __sasinh_ha_large_x.f) ? 0 : 2;
  // expon(Yh) + 2
  expon = ((Yh.w + 0x00400000) >> 23) - 0x7f;
  // new expon
  e23 = expon << 23;
  // 2^(-expon)
  two_expon.w = 0x3f800000 - e23;
  // Yl * 2^(-expon)
  Yl.f *= two_expon.f;
  // Yh * 2^(-expon-2)
  Yh.w -= e23;
  // reduced argument
  R0 = Yh.f - 1.0f;
  R = Yl.f + R0;
  // add exponent correction
  expon += iexpon_corr;
  // log() polynomial
  poly = __spirv_ocl_fma(__sasinh_ha_c9.f, R,
                                                __sasinh_ha_c8.f);
  poly = __spirv_ocl_fma(poly, R, __sasinh_ha_c7.f);
  poly = __spirv_ocl_fma(poly, R, __sasinh_ha_c6.f);
  poly = __spirv_ocl_fma(poly, R, __sasinh_ha_c5.f);
  poly = __spirv_ocl_fma(poly, R, __sasinh_ha_c4.f);
  poly = __spirv_ocl_fma(poly, R, __sasinh_ha_c3.f);
  poly = __spirv_ocl_fma(poly, R, __sasinh_ha_c2.f);
  poly = __spirv_ocl_fma(poly, R, __sasinh_ha_c1.f);
  float fR[2], fPoly[2], fExpon[2];
  fR[0] = R0;
  fR[1] = Yl.f;
  fPoly[0] = poly;
  fPoly[1] = 0.0f;
  fExpon[0] = __sasinh_ha_fln2[0];
  fExpon[1] = __sasinh_ha_fln2[1];
  {
    float __ph, __phl;
    __ph = __spirv_ocl_fma(fPoly[0], fR[0], 0.0f);
    __phl = __spirv_ocl_fma(fPoly[0], fR[0], -__ph);
    fPoly[1] = __spirv_ocl_fma(fPoly[1], fR[0], __phl);
    fPoly[1] =
        __spirv_ocl_fma(fPoly[0], fR[1], fPoly[1]);
    fPoly[0] = __ph;
  };
  {
    float __ph, __ahl, __ahh;
    __ph = __spirv_ocl_fma(fPoly[0], 1.0f,
                                                  __sasinh_ha_fc0[0]);
    __ahh =
        __spirv_ocl_fma(__ph, 1.0f, -__sasinh_ha_fc0[0]);
    __ahl = __spirv_ocl_fma(fPoly[0], 1.0f, -__ahh);
    fPoly[1] = (fPoly[1] + __sasinh_ha_fc0[1]) + __ahl;
    fPoly[0] = __ph;
  };
  ;
  {
    float __ph, __phl;
    __ph = __spirv_ocl_fma(fExpon[0], expon, 0.0f);
    __phl = __spirv_ocl_fma(fExpon[0], expon, -__ph);
    fExpon[1] = __spirv_ocl_fma(fExpon[1], expon, __phl);
    fExpon[0] = __ph;
  };
  {
    float __ph, __phl;
    __ph = __spirv_ocl_fma(fPoly[0], fR[0], 0.0f);
    __phl = __spirv_ocl_fma(fPoly[0], fR[0], -__ph);
    fPoly[1] = __spirv_ocl_fma(fPoly[1], fR[0], __phl);
    fPoly[1] =
        __spirv_ocl_fma(fPoly[0], fR[1], fPoly[1]);
    fPoly[0] = __ph;
  };
  {
    float __ph, __ahl, __ahh;
    __ph = __spirv_ocl_fma(fPoly[0], 1.0f, fExpon[0]);
    __ahh = __spirv_ocl_fma(__ph, 1.0f, -fExpon[0]);
    __ahl = __spirv_ocl_fma(fPoly[0], 1.0f, -__ahh);
    fPoly[1] = (fPoly[1] + fExpon[1]) + __ahl;
    fPoly[0] = __ph;
  };
  ;
  res.f = fPoly[0] + fPoly[1];
  xin.f = x;
  sgn.w = xin.w ^ xa.w;
  res.w ^= sgn.w;
  // fixup for small or Inf/NaN
  res.f = ((xa.f < __sasinh_ha_small_x.f) | (xa.w > __sasinh_ha_largest_norm.w))
              ? (x + sgn.f)
              : res.f;
  *r = res.f;
  return nRet;
}
float __ocl_svml_asinhf_ha(float x) {
  float r;
  __ocl_svml_internal_sasinh_ha(&x, &r);
  return r;
}

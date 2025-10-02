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
} __sexpm1_ha_c7 = {0x37d2d82du};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __sexpm1_ha_c6 = {0x3952d831u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __sexpm1_ha_c5 = {0x3ab606ceu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __sexpm1_ha_c4 = {0x3c08852cu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __sexpm1_ha_c3 = {0x3d2aaab0u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __sexpm1_ha_c2 = {0x3e2aaab0u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __sexpm1_ha_c1 = {0xb07ec280u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __sexpm1_ha_c0 = {0xb09f34cfu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __sexpm1_ha_fL2E = {0x3FB8AA3Bu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __sexpm1_ha_fShifter = {0x4b40007fu};
// log(2) high, low
static __constant union {
  unsigned int w;
  float f;
  int i;
} __sexpm1_ha_NL2H = {0xbf317218u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __sexpm1_ha_NL2L = {0x3102E308u};
__attribute__((always_inline)) inline int
__ocl_svml_internal_sexpm1_ha(float *a, float *pres) {
  int nRet = 0;
  float xin = *a;
  // float expm1f(float xin)
  union {
    unsigned int w;
    float f;
    int i;
  } xf, fN, xL2E, fS, xa;
  union {
    unsigned int w;
    float f;
    int i;
  } T, sc, res;
  float R, Rh, Rl, A, B, Bh, Th, Tl;
  float H, poly, Rhh, Rhl, ThRh, ThRh_l;
  float H1, H2, Rhh2, Rhl2, poly0;
  xf.f = xin;
  xL2E.f =
      __spirv_ocl_fma(xf.f, __sexpm1_ha_fL2E.f, 0.0f);
  fN.f = __spirv_ocl_trunc(xL2E.f);
  fS.f = __sexpm1_ha_fShifter.f + fN.f;
  // reduced argument
  Rh = __spirv_ocl_fma(fN.f, __sexpm1_ha_NL2H.f, xf.f);
  Rl = __spirv_ocl_fma(fN.f, __sexpm1_ha_NL2L.f, 0.0f);
  R = Rh + Rl;
  // 2^N
  T.w = fS.w << 23;
  // e^R - 1
  poly = __spirv_ocl_fma(__sexpm1_ha_c7.f, R,
                                                __sexpm1_ha_c6.f);
  poly = __spirv_ocl_fma(poly, R, __sexpm1_ha_c5.f);
  poly = __spirv_ocl_fma(poly, R, __sexpm1_ha_c4.f);
  poly = __spirv_ocl_fma(poly, R, __sexpm1_ha_c3.f);
  poly0 = __spirv_ocl_fma(poly, R, __sexpm1_ha_c2.f);
  poly = __spirv_ocl_fma(poly0, R, __sexpm1_ha_c1.f);
  poly = __spirv_ocl_fma(poly, R, __sexpm1_ha_c0.f);
  poly = __spirv_ocl_fma(poly, R, Rl);
  // maxabs(T,-1), minabs(T,-1)
  A = (xin >= 0.0f) ? T.f : -1.0f;
  B = (xin >= 0.0f) ? -1.0f : T.f;
  Th = T.f - 1.0f;
  Bh = Th - A;
  Tl = B - Bh;
  // T*Rh
  ThRh = __spirv_ocl_fma(T.f, Rh, 0.0f);
  ThRh_l = __spirv_ocl_fma(T.f, Rh, -ThRh);
  // Th + Th*Rh
  H = ThRh + Th;
  // 2*H
  H1 = H + H;
  // (Th*Rh)_high
  Rhh = H - Th;
  // (Th*Rh)_low
  Rhl = ThRh - Rhh;
  Tl = Tl + Rhl + ThRh_l;
  // H+H+Rh*Rh
  H2 = __spirv_ocl_fma(ThRh, Rh, H1);
  // Rh^2_high
  Rhh2 = H2 - H1;
  // Rh^2_low
  Rhl2 = __spirv_ocl_fma(ThRh, Rh, -Rhh2);
  // Tl += 0.5*Rhl2 + Rh*Rl
  Tl = __spirv_ocl_fma(ThRh, Rl, Tl);
  Tl = __spirv_ocl_fma(Rhl2, 0.5f, Tl);
  // 2^N*poly + Tl
  res.f = __spirv_ocl_fma(T.f, poly, Tl);
  res.f = __spirv_ocl_fma(H2, 0.5f, res.f);
  // ensure expm1(-0)=-0
  xa.f = __spirv_ocl_fabs(xf.f);
  res.w |= (xf.w ^ xa.w);
  if (__spirv_ocl_fabs(xf.f) <= 87.0f) {
    *pres = res.f;
    return nRet;
  }
  // special case and overflow path
  if (xf.f < 0) {
    *pres = -1.0f;
    return nRet;
  }
  if (!(xf.f < 128.0f)) {
    // +Inf or NaN?
    xa.w = xf.w & 0x7fffffff;
    if (xa.w > 0x7f800000) {
      *pres = xf.f + res.f;
      return nRet;
    }
    // overflow
    res.w = 0x7f800000 - 1;
    res.f = res.f * res.f; // to set OF flag
                           //
    nRet = 3;
    {
      *pres = res.f;
      return nRet;
    }
  }
  // at or near overflow
  // 2^(N-64), N=(int)dN
  T.w = (fS.w - 64) << 23;
  poly = __spirv_ocl_fma(poly0, R, 0.5f);
  poly = __spirv_ocl_fma(poly, R, __sexpm1_ha_c1.f);
  poly = __spirv_ocl_fma(poly, R, Rl);
  poly += Rh;
  res.f = __spirv_ocl_fma(T.f, poly, T.f);
  // final scaling
  sc.w = 0x5f800000u;
  res.f *= sc.f;
  // determine if overflow
  if (res.w == 0x7f800000)
    nRet = 3;
  *pres = res.f;
  return nRet;
}
float __ocl_svml_expm1f_ha(float x) {
  float r;
  __ocl_svml_internal_sexpm1_ha(&x, &r);
  return r;
}

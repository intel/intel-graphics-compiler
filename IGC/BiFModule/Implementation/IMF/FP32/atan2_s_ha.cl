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
} __satan2_ha_c11 = {0xbc8917bfu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __satan2_ha_c10 = {0x3c0afad8u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __satan2_ha_c9 = {0x3cf6afd7u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __satan2_ha_c8 = {0xbd1e9009u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __satan2_ha_c7 = {0xbc0e2ce0u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __satan2_ha_c6 = {0x3d8ee88fu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __satan2_ha_c5 = {0xbd84d3d8u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __satan2_ha_c4 = {0xbd3a0aceu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __satan2_ha_c3 = {0x3e34898cu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __satan2_ha_c2 = {0xbe05f565u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __satan2_ha_c1l = {0x31b69212u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __satan2_ha_c1 = {0xbe8259aeu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __satan2_ha_c0 = {0xbd94e63fu};
// 2.0^32
static __constant union {
  unsigned int w;
  float f;
  int i;
} __satan2_ha_two32 = {0x4f800000u};
// 2.0^64
static __constant union {
  unsigned int w;
  float f;
  int i;
} __satan2_ha_two64 = {0x5f800000u};
// 2.0^(-64)
static __constant union {
  unsigned int w;
  float f;
  int i;
} __satan2_ha_twom64 = {0x1f800000u};
// 0
static __constant union {
  unsigned int w;
  float f;
  int i;
} __satan2_ha_zero = {0x00000000u};
__attribute__((always_inline)) inline int
__ocl_svml_internal_satan2_ha(float *pa, float *pb, float *pres) {
  int nRet = 0;
  float xin = *pb, yin = *pa;
  // float atan2f_ha(float yin, float xin)
  {
    volatile union {
      unsigned int w;
      float f;
      int i;
    } y, x;
    union {
      unsigned int w;
      float f;
      int i;
    } ya, xa, hcorr, lcorr, fx, fy, hcorr2, lcorr2, sres, Q00;
    unsigned sgn_x, sgn_y, sgn_r, sgn_c;
    int sgnx_mask, smask;
    float frcp_x, f_eps, fQl, R, poly;
    volatile float fQ0;
    y.f = yin;
    x.f = xin;
    // absolute values
    xa.w = x.w & 0x7fffffff;
    ya.w = y.w & 0x7fffffff;
    // input signs
    sgn_x = x.w ^ xa.w;
    sgn_y = y.w ^ ya.w;
    // initialize result correction (0, pi, or pi/2)
    sgnx_mask = ((int)sgn_x) >> 31;
    hcorr.w = sgnx_mask & 0x40490FDB;
    lcorr.w = sgnx_mask & 0xB3BBBD2E;
    // initialize sign correction
    // sgn_r = sgn_x;
    // now switch y, x if |y|>|x|
    fy.w = (((xa.w) < (ya.w)) ? (xa.w) : (ya.w));
    fx.w = (((xa.w) >= (ya.w)) ? (xa.w) : (ya.w));
    // set correction term to pi/2 if xa<ya
    smask = ((int)(xa.w - ya.w)) >> 31;
    hcorr2.w = smask & 0x3fc90FDB;
    lcorr2.w = smask & 0xB33BBD2E;
    hcorr.f = hcorr2.f - hcorr.f;
    lcorr.f = lcorr2.f - lcorr.f;
    sgn_c = (smask & 0x80000000);
    hcorr.w ^= sgn_c;
    lcorr.w ^= sgn_c;
    // also apply sign correction
    sgn_r = sgn_c ^ (sgn_x ^ sgn_y);
    // reciprocal: rcp_x ~ 1/x
    frcp_x = 1.0f / (fx.f);
    // refine reciprocal
    f_eps = __spirv_ocl_fma((-frcp_x), fx.f, 1.0f);
    // quotient estimate
    Q00.f = fQ0 = fy.f * frcp_x;
    // redirect special inputs:  NaN/Inf/zero, |x|>2^126
    // testing done on ordered inputs
    if (((unsigned)(fx.w - 0x00800000) > 0x7e000000) ||
        (fy.f == __satan2_ha_zero.f) || (Q00.w < 0x0d800000))
      goto SPECIAL_ATAN2F;
  ATAN2F_MAIN:
    // low part of quotient
    fQl = __spirv_ocl_fma(fy.f, frcp_x, (-fQ0));
    fQl = __spirv_ocl_fma(fQ0, f_eps, fQl);
    // reduced argument
    R = fQ0 - 0.5f;
    R = R + fQl;
    poly = __spirv_ocl_fma(__satan2_ha_c11.f, R,
                                                  __satan2_ha_c10.f);
    poly = __spirv_ocl_fma(poly, R, __satan2_ha_c9.f);
    poly = __spirv_ocl_fma(poly, R, __satan2_ha_c8.f);
    poly = __spirv_ocl_fma(poly, R, __satan2_ha_c7.f);
    poly = __spirv_ocl_fma(poly, R, __satan2_ha_c6.f);
    poly = __spirv_ocl_fma(poly, R, __satan2_ha_c5.f);
    poly = __spirv_ocl_fma(poly, R, __satan2_ha_c4.f);
    poly = __spirv_ocl_fma(poly, R, __satan2_ha_c3.f);
    poly = __spirv_ocl_fma(poly, R, __satan2_ha_c2.f);
    poly = __spirv_ocl_fma(poly, R, __satan2_ha_c1l.f);
    poly = __spirv_ocl_fma(poly, R, __satan2_ha_c0.f);
    poly = __spirv_ocl_fma(__satan2_ha_c1.f, R, poly);
    // fQl + fQl*poly
    sres.f = __spirv_ocl_fma(poly, fQl, fQl);
    // (R+ R*poly)+lcorr
    sres.f = sres.f + lcorr.f;
    // ((R+ R*poly)+lcorr)+Q0*poly
    sres.f = __spirv_ocl_fma(fQ0, poly, sres.f);
    // ((R+ R*poly)+lcorr)+Q0*poly + Q0
    sres.f = sres.f + fQ0;
    // ((R+ R*poly)+lcorr)+Q0*poly + Q0 + hcorr
    sres.f = sres.f + hcorr.f;
    sres.w = sres.w ^ sgn_r;
    *pres = sres.f;
    return nRet;
  SPECIAL_ATAN2F:
    // NaN input
    if (fx.w > 0x7f800000u) {
      if (xa.w > 0x7f800000u)
        sres.w = x.w | 0x00400000u;
      else
        sres.w = y.w | 0x00400000u;
      *pres = sres.f;
      return nRet;
    }
    // zero input?
    if (fy.f == __satan2_ha_zero.f) {
      sres.w = sgn_y ^ (hcorr.w & 0x7fffffff);
      *pres = sres.f;
      return nRet;
    }
    // Inf input?
    if (fx.w == 0x7f800000) {
      if (fy.w < 0x7f800000u) {
        if (ya.w == 0x7f800000u)
          sres.w = sgn_y ^ 0x3fc90FDB;
        else
          sres.w = sgn_r ^ sgn_c ^ hcorr.w;
        *pres = sres.f;
        return nRet;
      }
      // both inputs are +/-Inf
      if (x.w == 0xff800000u)
        sres.w = 0x4016CBE4;
      else // +Inf
        sres.w = 0x3f490FDB;
      sres.w ^= sgn_y;
      *pres = sres.f;
      return nRet;
    }
    // very small |x|, |y|?
    if (fx.w < 0x00800000u) {
      // scale inputs
      fx.f *= __satan2_ha_two32.f;
      fy.f *= __satan2_ha_two32.f;
    } else if (fx.w > 0x7e800000u) {
      // large |x|
      fx.f *= 0.25f;
      fy.f *= 0.25f;
    }
    // reciprocal: rcp_x ~ 1/x
    frcp_x = 1.0f / (fx.f);
    // refine reciprocal
    f_eps = __spirv_ocl_fma((-frcp_x), fx.f, 1.0f);
    // quotient estimate
    Q00.f = fQ0 = fy.f * frcp_x;
    if (Q00.w < 0x0d800000) {
      fy.f *= __satan2_ha_two64.f;
      // reciprocal: rcp_x ~ 1/x
      frcp_x = 1.0f / (fx.f);
      // refine reciprocal
      f_eps = __spirv_ocl_fma((-frcp_x), fx.f, 1.0f);
      // quotient estimate
      fQ0 = fy.f * frcp_x;
      // low part of quotient
      fQl = __spirv_ocl_fma(fy.f, frcp_x, (-fQ0));
      fQl = __spirv_ocl_fma(fQ0, f_eps, fQl);
      sres.f = fQ0 + fQl;
      sres.f = __spirv_ocl_fma(
          sres.f, __satan2_ha_twom64.f, hcorr.f);
      sres.w = sres.w ^ sgn_r;
      *pres = sres.f;
      return nRet;
    }
    // return to main path
    goto ATAN2F_MAIN;
  }
  return nRet;
}
float __ocl_svml_atan2f_ha(float x, float y) {
  float r;
  __ocl_svml_internal_satan2_ha(&x, &y, &r);
  return r;
}

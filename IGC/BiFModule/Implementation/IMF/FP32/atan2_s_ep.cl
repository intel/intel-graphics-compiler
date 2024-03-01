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
} __satan2_ep_c5 = {0xbd84c5f7u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __satan2_ep_c4 = {0x3e0e093au};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __satan2_ep_c3 = {0x3d78fc87u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __satan2_ep_c2 = {0xbeb3af98u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __satan2_ep_c1 = {0x3adc9d97u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __satan2_ep_c0 = {0x3f7ffe56u};
// 2.0^32
static __constant union {
  unsigned int w;
  float f;
  int i;
} __satan2_ep_two32 = {0x4f800000u};
// 0
static __constant union {
  unsigned int w;
  float f;
  int i;
} __satan2_ep_zero = {0x00000000u};
__attribute__((always_inline)) inline int
__ocl_svml_internal_satan2_ep(float *pa, float *pb, float *pres) {
  int nRet = 0;
  float xin = *pb, yin = *pa;
  // float atan2f_la(float yin, float xin)
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
    } ya, xa, hcorr, lcorr, fx, fy, hcorr2, sres, Q00;
    unsigned sgn_x, sgn_y, sgn_r, sgn_c;
    int sgnx_mask, smask;
    float frcp_x, R, poly;
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
    // now switch y, x if |y|>|x|
    fy.w = (((xa.w) < (ya.w)) ? (xa.w) : (ya.w));
    fx.w = (((xa.w) >= (ya.w)) ? (xa.w) : (ya.w));
    // set correction term to pi/2 if xa<ya
    smask = ((int)(xa.w - ya.w)) >> 31;
    hcorr2.w = smask & 0x3fc90FDB;
    hcorr.f = hcorr2.f - hcorr.f;
    sgn_c = (smask & 0x80000000);
    hcorr.w ^= sgn_c;
    // also apply sign correction
    sgn_r = sgn_c ^ (sgn_x ^ sgn_y);
    // redirect special inputs:  NaN/Inf/zero, |x|>2^126
    // testing done on ordered inputs
    if (((unsigned)(fx.w - 0x00800000) > 0x7e000000) ||
        (fy.f == __satan2_ep_zero.f))
      goto SPECIAL_ATAN2F;
  ATAN2F_MAIN:
    // reciprocal: rcp_x ~ 1/x
    frcp_x = 1.0f / (fx.f);
    // quotient estimate
    Q00.f = fy.f * frcp_x;
    // reduced argument
    R = Q00.f;
    poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(__satan2_ep_c5.f, R,
                                                  __satan2_ep_c4.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(poly, R, __satan2_ep_c3.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(poly, R, __satan2_ep_c2.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(poly, R, __satan2_ep_c1.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(poly, R, __satan2_ep_c0.f);
    //  Q0*poly + hcorr
    sres.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(poly, Q00.f, hcorr.f);
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
    if (fy.f == __satan2_ep_zero.f) {
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
      fx.f *= __satan2_ep_two32.f;
      fy.f *= __satan2_ep_two32.f;
    } else if (fx.w > 0x7e800000u) {
      // large |x|
      fx.f *= 0.25f;
      fy.f *= 0.25f;
    }
    // return to main path
    goto ATAN2F_MAIN;
  }
  return nRet;
}
float __ocl_svml_atan2f_ep(float x, float y) {
  float r;
  __ocl_svml_internal_satan2_ep(&x, &y, &r);
  return r;
}

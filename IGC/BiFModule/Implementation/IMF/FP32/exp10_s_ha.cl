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
} __sexp10_ha_Shifter = {0x4ac000feu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __sexp10_ha_L2_10 = {0x40549A78u};
// log10(2) high, low
static __constant union {
  unsigned int w;
  float f;
  int i;
} __sexp10_ha_L2H = {0x3e9A209Bu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __sexp10_ha_L2L = {0xb2760860u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __sexp10_ha_c5 = {0x3f0a4794u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __sexp10_ha_c4 = {0x3f962559u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __sexp10_ha_c3 = {0x40023822u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __sexp10_ha_c2 = {0x4029a917u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __sexp10_ha_c1 = {0x40135d8eu};
__attribute__((always_inline)) inline int
__ocl_svml_internal_sexp10_ha(float *a, float *r) {
  int nRet = 0;
  float x = *a;
  union {
    unsigned int w;
    float f;
    int i;
  } S, Th, Tlr, Th2, xin, xa, res;
  float N, R, poly;
  int index_mask;
  S.f = __spirv_ocl_fma(x, __sexp10_ha_L2_10.f,
                                               __sexp10_ha_Shifter.f);
  N = S.f - __sexp10_ha_Shifter.f;
  R = __spirv_ocl_fma((-N), __sexp10_ha_L2H.f, x);
  R = __spirv_ocl_fma((-N), __sexp10_ha_L2L.f, R);
  // set exponent in place
  Th.w = S.w << 22;
  // index_mask is based on last bit of S.w
  index_mask = 0 - (S.w & 1);
  // set Th mantissa
  Th.w ^= (index_mask & 0x7504F3u);
  // set Tl/Th value
  Tlr.w = index_mask & 0x329302AEu;
  // polynomial
  poly = __spirv_ocl_fma(R, __sexp10_ha_c5.f,
                                                __sexp10_ha_c4.f);
  poly = __spirv_ocl_fma(R, poly, __sexp10_ha_c3.f);
  poly = __spirv_ocl_fma(R, poly, __sexp10_ha_c2.f);
  poly = __spirv_ocl_fma(R, poly, __sexp10_ha_c1.f);
  poly = __spirv_ocl_fma(R, poly, Tlr.f);
  xin.f = x;
  xa.w = xin.w & 0x7fffffffu;
  // redirect special cases
  if (xa.w > 0x4217B818u)
    goto EXPF_SPECIAL;
  res.f = __spirv_ocl_fma(poly, Th.f, Th.f);
  *r = res.f;
  return nRet;
EXPF_SPECIAL:
  if (xa.w > 0x42349E35u) {
    if (xa.w > 0x7f800000u) { // NaN?
      *r = x + x;
      return nRet;
    }
    if (x < 0) {
      *r = 0.0f;
      nRet = 4;
      return nRet; // underflow to 0
    }
    // overflow
    nRet = 3;
    res.w = 0x7f800000;
    *r = res.f;
    return nRet;
  }
  S.w += 0xfe;
  Th2.w = (S.w >> 2) & 0xff;
  S.w -= (Th2.w << 1);
  Th2.w <<= 23; // second exponent scale
  Th.w = S.w << 22;
  // set Th mantissa
  Th.w ^= (index_mask & 0x7504F3u);
  res.f = __spirv_ocl_fma(poly, Th.f, Th.f);
  res.f *= Th2.f;
  *r = res.f;
  return nRet;
}
float __ocl_svml_exp10f_ha(float x) {
  float r;
  __ocl_svml_internal_sexp10_ha(&x, &r);
  return r;
}

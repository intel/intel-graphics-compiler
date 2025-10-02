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
} __slog10_ha_two32 = {0x4f800000u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __slog10_ha_c10 = {0xbca31649u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __slog10_ha_c9 = {0x3d3c41e2u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __slog10_ha_c8 = {0xbd6b2906u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __slog10_ha_c7 = {0x3d80c9d2u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __slog10_ha_c6 = {0xbd940285u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __slog10_ha_c5 = {0x3db1cb7cu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __slog10_ha_c4 = {0xbdde5c73u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __slog10_ha_c3 = {0x3e143d6bu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __slog10_ha_c2h = {0xbe5e5bd8u};
static __constant float __slog10_ha_fc1[] = {
    0x1.bcb7bp-2f,
    0x1.33e4c2p-26f}; // C1 HI+LO:       0.43429446 + 1.7921765e-08 [0x3ede5bd8
                      // + 0x3299f261]
static __constant float __slog10_ha_fl10[] = {
    0x1.344134p-2f,
    0x1.09f7ap-26f}; // Log10(2) HI+LO: 0.30102998 + 1.5481334e-08 [0x3e9a209a +
                     // 0x3284fbd0]
__attribute__((always_inline)) inline int
__ocl_svml_internal_slog10_ha(float *a, float *r) {
  int nRet = 0;
  float xin = *a;
  union {
    unsigned int w;
    float f;
    int i;
  } x, mant, res;
  int iexpon;
  unsigned int xa;
  float R, poly;
  float fpolyh, fpolyl, fexpon, fresh, fresl;
  x.f = xin;
  // normalize mantissa to [0.75, 1.5)
  iexpon = (x.w - 0x3f400000u) & 0xff800000u;
  // normalized mantissa
  mant.w = x.w - iexpon;
  // exponent
  iexpon >>= 23;
  // filter out denormals/zero/negative/Inf/NaN
  if ((unsigned int)(x.w - 0x00800000) >= 0x7f000000)
    goto LOG10F_SPECIAL;
LOG10F_MAIN:
  // reduced argument
  R = mant.f - 1.0f;
  // polynomial
  poly = __spirv_ocl_fma(__slog10_ha_c10.f, R,
                                                __slog10_ha_c9.f);
  poly = __spirv_ocl_fma(poly, R, __slog10_ha_c8.f);
  poly = __spirv_ocl_fma(poly, R, __slog10_ha_c7.f);
  poly = __spirv_ocl_fma(poly, R, __slog10_ha_c6.f);
  poly = __spirv_ocl_fma(poly, R, __slog10_ha_c5.f);
  poly = __spirv_ocl_fma(poly, R, __slog10_ha_c4.f);
  poly = __spirv_ocl_fma(poly, R, __slog10_ha_c3.f);
  poly = __spirv_ocl_fma(poly, R, __slog10_ha_c2h.f);
  fpolyh = __spirv_ocl_fma(poly, R, 0.0f);
  fpolyl = __spirv_ocl_fma(poly, R, -fpolyh);
  fexpon = (float)iexpon;
  fresh = __slog10_ha_fl10[0];
  fresl = __slog10_ha_fl10[1];
  // Multi-precision computationsl part
  {
    float __ph, __phl;
    __ph = __spirv_ocl_fma(fresh, fexpon, 0.0f);
    __phl = __spirv_ocl_fma(fresh, fexpon, -__ph);
    fresl = __spirv_ocl_fma(fresl, fexpon, __phl);
    fresh = __ph;
  };
  {
    float __ph, __ahl, __ahh;
    __ph = __spirv_ocl_fma(fpolyh, 1.0f,
                                                  __slog10_ha_fc1[0]);
    __ahh =
        __spirv_ocl_fma(__ph, 1.0f, -__slog10_ha_fc1[0]);
    __ahl = __spirv_ocl_fma(fpolyh, 1.0f, -__ahh);
    fpolyl = (fpolyl + __slog10_ha_fc1[1]) + __ahl;
    fpolyh = __ph;
  };
  {
    float __ph, __phl;
    __ph = __spirv_ocl_fma(fpolyh, R, 0.0f);
    __phl = __spirv_ocl_fma(fpolyh, R, -__ph);
    fpolyl = __spirv_ocl_fma(fpolyl, R, __phl);
    fpolyh = __ph;
  };
  {
    float __ph, __ahl, __ahh;
    __ph = __spirv_ocl_fma(fpolyh, 1.0f, fresh);
    __ahh = __spirv_ocl_fma(__ph, 1.0f, -fresh);
    __ahl = __spirv_ocl_fma(fpolyh, 1.0f, -__ahh);
    fpolyl = (fpolyl + fresl) + __ahl;
    fpolyh = __ph;
  };
  // Add HI+LO parts of final result
  poly = fpolyh + fpolyl;
  *r = poly;
  return nRet;
LOG10F_SPECIAL:
  xa = x.w & 0x7fffffffu;
  // zero
  if (!xa) {
    nRet = 2;
    res.w = 0xff800000u;
    *r = res.f;
    return nRet;
  }
  // Inf/NaN or negative?
  if (x.w >= 0x7f800000u) {
    // +Inf?
    if (x.w == 0x7f800000u) {
      *r = x.f;
      return nRet;
    }
    nRet = 1;
    // return QNaN
    x.w |= 0x7fc00000u;
    *r = x.f;
    return nRet;
  }
  // positive denormal
  // scale x by 2^32
  x.f *= __slog10_ha_two32.f;
  // normalized, unbiased exponent
  iexpon = (x.w - 0x3f400000u) & 0xff800000u;
  // normalized mantissa
  mant.w = x.w - iexpon;
  // exponent
  iexpon >>= 23;
  iexpon -= 32;
  goto LOG10F_MAIN;
}
float __ocl_svml_log10f_ha(float x) {
  float r;
  __ocl_svml_internal_slog10_ha(&x, &r);
  return r;
}

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
} __slog2_ha_two32 = {0x4f800000u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __slog2_ha_c10 = {0xbd8770dcu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __slog2_ha_c9 = {0x3e1c5827u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __slog2_ha_c8 = {0xbe434bdeu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __slog2_ha_c7 = {0x3e55e9b0u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __slog2_ha_c6 = {0xbe75d6cau};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __slog2_ha_c5 = {0x3e93a7cbu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __slog2_ha_c4 = {0xbeb8aabcu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __slog2_ha_c3 = {0x3ef6389fu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __slog2_ha_c2h = {0xbf38aa3bu};
static __constant float __slog2_ha_c1h = 0x1.715476p+0;
static __constant float __slog2_ha_c1l = 0x1.4ae0cp-26;
__attribute__((always_inline)) inline int
__ocl_svml_internal_slog2_ha(float *a, float *r) {
  float xin = *a;
  union {
    unsigned int w;
    float f;
    int i;
  } x, mant, res;
  int iexpon;
  unsigned int xa;
  float R, poly, polyh, polyl, expon;
  int nRet = 0;
  x.f = xin;
  // normalize mantissa to [0.75, 1.5)
  // normalized, unbiased exponent
  iexpon = (x.w - 0x3f400000u) & 0xff800000u;
  // normalized mantissa
  mant.w = x.w - iexpon;
  // exponent
  iexpon >>= 23;
  // filter out denormals/zero/negative/Inf/NaN
  if ((unsigned int)(x.w - 0x00800000) >= 0x7f000000u)
    goto LOG2F_SPECIAL;
LOG2F_MAIN:
  // reduced argument
  R = mant.f - 1.0f;
  // polynomial
  poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(__slog2_ha_c10.f, R,
                                                __slog2_ha_c9.f);
  poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(poly, R, __slog2_ha_c8.f);
  poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(poly, R, __slog2_ha_c7.f);
  poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(poly, R, __slog2_ha_c6.f);
  poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(poly, R, __slog2_ha_c5.f);
  poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(poly, R, __slog2_ha_c4.f);
  poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(poly, R, __slog2_ha_c3.f);
  poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(poly, R, __slog2_ha_c2h.f);
  expon = (float)iexpon;
  polyh = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(poly, R, 0.0f);
  polyl = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(poly, R, -polyh);
  {
    float __ph, __ahl, __ahh;
    __ph = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(polyh, 1.0f, __slog2_ha_c1h);
    __ahh = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(__ph, 1.0f, -__slog2_ha_c1h);
    __ahl = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(polyh, 1.0f, -__ahh);
    polyl = (polyl + __slog2_ha_c1l) + __ahl;
    polyh = __ph;
  };
  {
    float __ph, __phl;
    __ph = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(polyh, R, 0.0f);
    __phl = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(polyh, R, -__ph);
    polyl = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(polyl, R, __phl);
    polyh = __ph;
  };
  {
    float __ph, __ahl, __ahh;
    __ph = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(polyh, 1.0f, expon);
    __ahh = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(__ph, 1.0f, -expon);
    __ahl = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(polyh, 1.0f, -__ahh);
    polyl = polyl + __ahl;
    polyh = __ph;
  };
  poly = polyh + polyl;
  *r = poly;
  return nRet;
LOG2F_SPECIAL:
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
    // return QNaN
    nRet = 1;
    x.w |= 0x7fc00000u;
    *r = x.f;
    return nRet;
  }
  // positive denormal?
  if (!(xa & 0x7f800000)) {
    // scale x by 2^32
    x.f *= __slog2_ha_two32.f;
    // normalized, unbiased exponent
    iexpon = (x.w - 0x3f400000u) & 0xff800000u;
    // normalized mantissa
    mant.w = x.w - iexpon;
    // exponent
    iexpon >>= 23;
    iexpon -= 32;
  }
  goto LOG2F_MAIN;
}
float __ocl_svml_log2f_ha(float x) {
  float r;
  __ocl_svml_internal_slog2_ha(&x, &r);
  return r;
}

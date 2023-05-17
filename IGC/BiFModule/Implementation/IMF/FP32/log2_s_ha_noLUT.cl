/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF
static __constant union {
  unsigned int w;
  float f;
} __slog2_ha_two32 = {0x4f800000u};
static __constant union {
  unsigned int w;
  float f;
} __slog2_ha_c10 = {0xbd8770dcu};
static __constant union {
  unsigned int w;
  float f;
} __slog2_ha_c9 = {0x3e1c5827u};
static __constant union {
  unsigned int w;
  float f;
} __slog2_ha_c8 = {0xbe434bdeu};
static __constant union {
  unsigned int w;
  float f;
} __slog2_ha_c7 = {0x3e55e9b0u};
static __constant union {
  unsigned int w;
  float f;
} __slog2_ha_c6 = {0xbe75d6cau};
static __constant union {
  unsigned int w;
  float f;
} __slog2_ha_c5 = {0x3e93a7cbu};
static __constant union {
  unsigned int w;
  float f;
} __slog2_ha_c4 = {0xbeb8aabcu};
static __constant union {
  unsigned int w;
  float f;
} __slog2_ha_c3 = {0x3ef6389fu};
static __constant union {
  unsigned int w;
  float f;
} __slog2_ha_c2h = {0xbf38aa3bu};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __slog2_ha_dc1 = {0x3ff715476395bd86uL};

__attribute__((always_inline))
static inline int __ocl_svml_internal_slog2_ha(float *a, float *r)
{
  float xin = *a;
  union {
    unsigned int w;
    float f;
  } x, mant, res;
  int iexpon;
  unsigned int xa;
  float R, poly;
  double dR, dpoly, expon;
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
  expon = (double)iexpon;
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
  dR = (double)R;
  dpoly = (double)poly;
  dpoly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dpoly, dR, __slog2_ha_dc1.f);
  dpoly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dpoly, dR, expon);
  poly = (float)dpoly;
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

float __ocl_svml_log2f_ha_noLUT(float x)
{
  float r;
  __ocl_svml_internal_slog2_ha(&x, &r);
  return r;
}

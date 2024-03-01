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
} __sexp2_ha_c6 = {0x39224c80u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __sexp2_ha_c5 = {0x3aafa463u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __sexp2_ha_c4 = {0x3c1d94cbu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __sexp2_ha_c3 = {0x3d635766u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __sexp2_ha_c2 = {0x3e75fdf1u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __sexp2_ha_c1 = {0x3e45c862u};
__attribute__((always_inline)) inline int
__ocl_svml_internal_sexp2_ha(float *a, float *r) {
  int nRet = 0;
  float x = *a;
  float fN, R, poly, High, Rh, Rl;
  int sN, sN2;
  unsigned int N;
  union {
    unsigned int w;
    float f;
    int i;
  } T, T2, xi, res;
  fN = SPIRV_OCL_BUILTIN(rint, _f32, )(x);
  R = x - fN;
  sN = (int)fN;
  // exponent
  N = sN;
  poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(__sexp2_ha_c6.f, R,
                                                __sexp2_ha_c5.f);
  // 1+0.5*R
  High = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(R, 0.5f, 1.0f);
  poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(poly, R, __sexp2_ha_c4.f);
  // (0.5*R)_high
  Rh = High - 1.0f;
  poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(poly, R, __sexp2_ha_c3.f);
  // (0.5*R)_low
  Rl = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(R, 0.5f, (-Rh));
  poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(poly, R, __sexp2_ha_c2.f);
  poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(poly, R, __sexp2_ha_c1.f);
  poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(poly, R, Rl);
  res.f = High + poly;
  if (((unsigned int)(N + 0x7f - 2)) > 124 + 0x7f)
    goto EXP2F_SPECIAL;
  res.w += (N << 23);
  *r = res.f;
  return nRet;
EXP2F_SPECIAL:
  xi.f = x;
  if ((xi.w & 0x7fffffffu) >= 0x7f800000u) {
    if (xi.w == 0xff800000) {
      *r = 0.0f;
      return nRet;
    } else {
      *r = x + x;
      return nRet; // NaN or +Inf
    }
  }
  x = SPIRV_OCL_BUILTIN(fmin, _f32_f32, )(x, 192.0f);
  x = SPIRV_OCL_BUILTIN(fmax, _f32_f32, )(x, -192.0f);
  fN = SPIRV_OCL_BUILTIN(rint, _f32, )(x);
  sN = (int)fN;
  // split the scaling coefficients
  sN2 = sN >> 1;
  sN -= sN2;
  T.w = (sN + 0x7f) << 23;
  T2.w = (sN2 + 0x7f) << 23;
  res.f *= T.f;
  res.f *= T2.f;
  nRet = (res.w < 0x00800000u) ? 4 : nRet;
  nRet = (res.w == 0x7f800000) ? 3 : nRet;
  *r = res.f;
  return nRet;
}
float __ocl_svml_exp2f_ha(float x) {
  float r;
  __ocl_svml_internal_sexp2_ha(&x, &r);
  return r;
}

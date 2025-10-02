/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dexp2_ha_Shifter = {0x43380000000003ffuL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dexp2_ha_Shifter0 = {0x43380000000007feuL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dexp2_ha_c11 = {0x3dfea1c678ded0efuL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dexp2_ha_c10 = {0x3e3e6228be5a9ffduL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dexp2_ha_c9 = {0x3e7b524ca9ff39ccuL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dexp2_ha_c8 = {0x3eb62bfc2c7be078uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dexp2_ha_c7 = {0x3eeffcbfc7e3f872uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dexp2_ha_c6 = {0x3f2430913112cae8uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dexp2_ha_c5 = {0x3f55d87fe78a0586uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dexp2_ha_c4 = {0x3f83b2ab6fb9f1a3uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dexp2_ha_c3 = {0x3fac6b08d704a0dbuL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dexp2_ha_c2 = {0x3fcebfbdff82c5aeuL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dexp2_ha_c1 = {0x3fc8b90bfbe8e7bcuL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dexp2_ha_thres = {0x408ff00000000000uL};
__attribute__((always_inline)) inline int
__ocl_svml_internal_dexp2_ha(double *a, double *r) {
  int nRet = 0;
  double x = *a;
  union {
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
  } dS, xin, res, T, T2, expon;
  double dN, R, poly, High, Rh, Rl;
  unsigned int xa32;
  int sN, sN2;
  dS.f = x + __dexp2_ha_Shifter.f;
  dN = dS.f - __dexp2_ha_Shifter.f;
  R = x - dN;
  poly = __spirv_ocl_fma(__dexp2_ha_c11.f, R,
                                                __dexp2_ha_c10.f);
  poly = __spirv_ocl_fma(poly, R, __dexp2_ha_c9.f);
  poly = __spirv_ocl_fma(poly, R, __dexp2_ha_c8.f);
  poly = __spirv_ocl_fma(poly, R, __dexp2_ha_c7.f);
  poly = __spirv_ocl_fma(poly, R, __dexp2_ha_c6.f);
  poly = __spirv_ocl_fma(poly, R, __dexp2_ha_c5.f);
  // 1+0.5*R
  High = __spirv_ocl_fma(R, 0.5, 1.0);
  poly = __spirv_ocl_fma(poly, R, __dexp2_ha_c4.f);
  // (0.5*R)_high
  Rh = High - 1.0;
  poly = __spirv_ocl_fma(poly, R, __dexp2_ha_c3.f);
  // (0.5*R)_low
  Rl = __spirv_ocl_fma(R, 0.5, (-Rh));
  poly = __spirv_ocl_fma(poly, R, __dexp2_ha_c2.f);
  poly = __spirv_ocl_fma(poly, R, __dexp2_ha_c1.f);
  poly = __spirv_ocl_fma(poly, R, Rl);
  res.f = High + poly;
  if (__spirv_ocl_fabs(x) >= __dexp2_ha_thres.f)
    goto EXP2_SPECIAL;
  // final scaling
  // res.w32[1] += (dS.w32[0] << 20);
  expon.w = dS.w << 52;
  res.f *= expon.f;
  *r = res.f;
  return nRet;
EXP2_SPECIAL:
  xin.f = x;
  xa32 = xin.w32[1] & 0x7fffffffuL;
  if (xa32 >= 0x7ff00000u) {
    if (xin.w == 0xfff0000000000000uL) {
      *r = 0.0f;
      return nRet;
    } else // NaN or +Inf
    {
      *r = x + x;
      return nRet;
    }
  }
  x = __spirv_ocl_fmin(x, 1536.0);
  x = __spirv_ocl_fmax(x, -1536.0);
  dS.f = x + __dexp2_ha_Shifter0.f;
  sN = dS.w32[0];
  // fix res.f for very large |x|
  res.f = __spirv_ocl_fmin(res.f, 2.0);
  res.f = __spirv_ocl_fmax(res.f, 0.5);
  // split the scaling coefficients
  sN2 = sN >> 1;
  sN -= sN2;
  T.w = (sN /*+ 0x3ff*/);
  T.w <<= 52;
  T2.w = (sN2 /*+ 0x3ff*/);
  T2.w <<= 52;
  res.f *= T.f;
  res.f *= T2.f;
  nRet = (res.w < 0x0010000000000000uL) ? 4 : nRet;
  nRet = (res.w == 0x7ff0000000000000uL) ? 3 : nRet;
  *r = res.f;
  return nRet;
}
double __ocl_svml_exp2_ha(double x) {
  double r;
  __ocl_svml_internal_dexp2_ha(&x, &r);
  return r;
}

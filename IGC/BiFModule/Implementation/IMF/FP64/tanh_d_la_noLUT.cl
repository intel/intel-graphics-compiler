/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dtanh_la___Shifter = {0x43280000000003FFuL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dtanh_la___L2E = {0x3FF71547652B82FEuL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dtanh_la___mL2 = {0xBFE62E42FEFA39EFuL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dtanh_la___mL2L = {0xBC7ABC9E3B39803FuL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dtanh_la___c12 = {0x3ee1eed87b4210b8uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dtanh_la___c11 = {0x3f0af62ca32f0745uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dtanh_la___c10 = {0x3f327e5343c0a3f2uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dtanh_la___c9 = {0x3f571ddf5a65250fuL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dtanh_la___c8 = {0x3f7a01a017bc38bduL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dtanh_la___c7 = {0x3f9a01a01b12e962uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dtanh_la___c6 = {0x3fb6c16c16c1f571uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dtanh_la___c5 = {0x3fd111111110f25euL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dtanh_la___c4 = {0x3fe555555555553cuL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dtanh_la___c3 = {0x3ff555555555555auL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dtanh_la___c2 = {0x4000000000000000uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dtanh_la___c1 = {0x4000000000000000uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dtanh_la___c0 = {0x0000000000000000uL};

__attribute__((always_inline))
static inline int __ocl_svml_internal_dtanh(double *px, double *pres)
{
  int nRet = 0;
  double xin = *px;
  union {
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
  } x, xa, S, res;
  unsigned long sgn_x;
  double dN, Rh, poly, AX, Ah, Al, R, eps, Qh, Ql;
  double BX, Bh, Bl;
  float bf;
  x.f = xin;
  // absolute value
  xa.w = x.w & 0x7fffffffffffffffuL;
  // sign
  sgn_x = x.w ^ xa.w;
  // clamp range + NaN fixup
  xa.f = ((xa.f > 20.0) && (xa.w <= 0x7ff0000000000000uL)) ? 20.0 : xa.f;
  // exp(2*xa.f)
  // Shifter + xa*log2e
  S.f = __spirv_ocl_fma(xa.f, __dtanh_la___L2E.f,
                                               __dtanh_la___Shifter.f);
  dN =
      __spirv_ocl_fma(S.f, 1.0, -__dtanh_la___Shifter.f);
  // Rh ~ N*log(2) + xa
  Rh = __spirv_ocl_fma(dN, __dtanh_la___mL2.f, xa.f);
  Rh = __spirv_ocl_fma(dN, __dtanh_la___mL2L.f, Rh);
  // T = 2^(rint(2*xa*log2e))
  S.w <<= 52;
  // e^(2*Rh)
  poly = __spirv_ocl_fma(Rh, __dtanh_la___c12.f,
                                                __dtanh_la___c11.f);
  poly = __spirv_ocl_fma(poly, Rh, __dtanh_la___c10.f);
  poly = __spirv_ocl_fma(poly, Rh, __dtanh_la___c9.f);
  poly = __spirv_ocl_fma(poly, Rh, __dtanh_la___c8.f);
  poly = __spirv_ocl_fma(poly, Rh, __dtanh_la___c7.f);
  poly = __spirv_ocl_fma(poly, Rh, __dtanh_la___c6.f);
  poly = __spirv_ocl_fma(poly, Rh, __dtanh_la___c5.f);
  poly = __spirv_ocl_fma(poly, Rh, __dtanh_la___c4.f);
  poly = __spirv_ocl_fma(poly, Rh, __dtanh_la___c3.f);
  poly = __spirv_ocl_fma(poly, Rh, __dtanh_la___c2.f);
  poly = __spirv_ocl_fma(poly, Rh, __dtanh_la___c1.f);
  poly = __spirv_ocl_fma(poly, Rh, 0.0);
  // dividend
  AX = __spirv_ocl_fma(S.f, 1.0, -1.0);
  Ah = __spirv_ocl_fma(S.f, poly, AX);
  Al = __spirv_ocl_fma(Ah, 1.0, -AX);
  Al = __spirv_ocl_fma(S.f, poly, -Al);
  // divisor
  BX = __spirv_ocl_fma(S.f, 1.0, 1.0);
  Bh = __spirv_ocl_fma(S.f, poly, BX);
  Bl = __spirv_ocl_fma(Bh, 1.0, -BX);
  Bl = __spirv_ocl_fma(S.f, poly, -Bl);
  // R ~ 1/B
  bf = (float)Bh;
  R = (double)1.0f / (bf);
  eps = __spirv_ocl_fma(Bh, -R, 1.0);
  eps = __spirv_ocl_fma(Bl, -R, eps);
  // eps + eps^2
  eps = __spirv_ocl_fma(eps, eps, eps);
  // A*R high, low
  Qh = __spirv_ocl_fma(Ah, R, 0.0);
  Ql = __spirv_ocl_fma(Ah, R, -Qh);
  Ql = __spirv_ocl_fma(Al, R, Ql);
  // result
  Ql = __spirv_ocl_fma(Qh, eps, Ql);
  res.f = __spirv_ocl_fma(Qh, 1.0, Ql);
  res.w ^= sgn_x;
  *pres = res.f;
  return nRet;
}

double __ocl_svml_tanh_noLUT(double x)
{
  double r;
  __ocl_svml_internal_dtanh(&x, &r);
  return r;
}

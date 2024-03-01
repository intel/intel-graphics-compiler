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
} __dtanh_ha___Shifter = {0x43280000000003FFuL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dtanh_ha___L2E = {0x3FF71547652B82FEuL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dtanh_ha___mL2 = {0xBFE62E42FEFA39EFuL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dtanh_ha___mL2L = {0xBC7ABC9E3B39803FuL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dtanh_ha___c12 = {0x3ee1eed87b4210b8uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dtanh_ha___c11 = {0x3f0af62ca32f0745uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dtanh_ha___c10 = {0x3f327e5343c0a3f2uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dtanh_ha___c9 = {0x3f571ddf5a65250fuL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dtanh_ha___c8 = {0x3f7a01a017bc38bduL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dtanh_ha___c7 = {0x3f9a01a01b12e962uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dtanh_ha___c6 = {0x3fb6c16c16c1f571uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dtanh_ha___c5 = {0x3fd111111110f25euL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dtanh_ha___c4 = {0x3fe555555555553cuL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dtanh_ha___c3 = {0x3ff555555555555auL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dtanh_ha___c2 = {0x4000000000000000uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dtanh_ha___c1 = {0x4000000000000000uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dtanh_ha___c0 = {0x0000000000000000uL};

__attribute__((always_inline)) inline int
__ocl_svml_internal_dtanh_ha(double *px, double *pres) {
  int nRet = 0;
  double xin = *px;
  union {
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
  } x, xa, S, SR, SRl, res;
  unsigned long sgn_x;
  double dN, Rh, Rl, R0, poly, AX, Ah, Al, R, eps, Qh, Ql;
  double BX, Bh, Bl, poly0, poly_l, poly_l2, poly_h;
  float bf;
  x.f = xin;
  // absolute value
  xa.w = x.w & 0x7fffffffffffffffuL;
  // sign
  sgn_x = x.w ^ xa.w;
  // clamp range
  xa.f = ((xa.f > 20.0) && (xa.w <= 0x7ff0000000000000uL)) ? 20.0 : xa.f;
  // exp(2*xa.f)
  // Shifter + xa*log2e
  S.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(xa.f, __dtanh_ha___L2E.f,
                                               __dtanh_ha___Shifter.f);
  dN =
      SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(S.f, 1.0, -__dtanh_ha___Shifter.f);
  // (Rh + Rl) = N*log(2) + xa
  R0 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dN, __dtanh_ha___mL2.f, xa.f);
  Rh = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dN, __dtanh_ha___mL2L.f, R0);
  Rl = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(R0, 1.0, -Rh);
  Rl = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dN, __dtanh_ha___mL2L.f, Rl);
  // T = 2^(rint(2*xa*log2e))
  S.w <<= 52;
  // e^(2*Rh)
  poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(Rh, __dtanh_ha___c12.f,
                                                __dtanh_ha___c11.f);
  poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(poly, Rh, __dtanh_ha___c10.f);
  poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(poly, Rh, __dtanh_ha___c9.f);
  poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(poly, Rh, __dtanh_ha___c8.f);
  poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(poly, Rh, __dtanh_ha___c7.f);
  poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(poly, Rh, __dtanh_ha___c6.f);
  poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(poly, Rh, __dtanh_ha___c5.f);
  poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(poly, Rh, __dtanh_ha___c4.f);
  poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(poly, Rh, __dtanh_ha___c3.f);
  poly0 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(poly, Rh, __dtanh_ha___c2.f);
  poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(poly0, Rh, __dtanh_ha___c1.f);
  poly_l2 =
      SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(poly, 1.0, -__dtanh_ha___c1.f);
  poly_l2 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(poly0, Rh, -poly_l2);
  poly_l = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(poly0, Rl, poly_l2);
  SR.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(S.f, Rh, 0.0);
  SRl.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(S.f, Rl, 0.0);
  poly_h = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(SR.f, poly, 0.0);
  // dividend
  AX = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(S.f, 1.0, -1.0);
  Ah = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(poly_h, 1.0, AX);
  Al = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(Ah, 1.0, -AX);
  Al = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(SR.f, poly, -Al);
  Al = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(SR.f, poly_l, Al);
  Al = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(SRl.f, 2.0, Al);
  Al = (AX == S.f) ? (Al - 1.0) : Al;
  // divisor
  BX = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(S.f, 1.0, 1.0);
  Bh = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(poly_h, 1.0, BX);
  Bl = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(Bh, 1.0, -BX);
  Bl = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(SR.f, poly, -Bl);
  Bl = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(SR.f, poly_l, Bl);
  Bl = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(SRl.f, 2.0, Bl);
  Bl = (BX == S.f) ? (Bl + 1.0) : Bl;
  // R ~ 1/B
  bf = (float)Bh;
  R = (double)1.0f / (bf);
  eps = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(Bh, -R, 1.0);
  eps = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(Bl, -R, eps);
  // eps + eps^2
  eps = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(eps, eps, eps);
  // A*R high, low
  Qh = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(Ah, R, 0.0);
  Ql = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(Ah, R, -Qh);
  Ql = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(Al, R, Ql);
  // result
  Ql = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(Qh, eps, Ql);
  res.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(Qh, 1.0, Ql);
  res.w ^= sgn_x;
  *pres = res.f;
  return nRet;
}
double __ocl_svml_tanh_ha(double x) {
  double r;
  __ocl_svml_internal_dtanh_ha(&x, &r);
  return r;
}

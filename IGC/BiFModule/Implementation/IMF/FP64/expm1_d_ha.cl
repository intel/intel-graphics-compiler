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
} __dexpm1_ha_Tbl_exp[] = {
    {0x0000000000000000uL}, {0x0000000000000000uL}, {0x0001b5586cf9890fuL},
    {0x3c979aa65d837b6duL}, {0x000372b83c7d517buL}, {0xbc801b15eaa59348uL},
    {0x0001387a6e756238uL}, {0x3c968efde3a8a894uL}, {0x000706fe0a31b715uL},
    {0x3c834d754db0abb6uL}, {0x0006dea64c123422uL}, {0x3c859f48a72a4c6duL},
    {0x0002bfdad5362a27uL}, {0x3c7690cebb7aafb0uL}, {0x0002ab07dd485429uL},
    {0x3c9063e1e21c5409uL}, {0x000ea09e667f3bcduL}, {0xbc93b3efbf5e2229uL},
    {0x000ea11473eb0187uL}, {0xbc7b32dcb94da51duL}, {0x0002ace5422aa0dbuL},
    {0x3c8db72fc1f0eab5uL}, {0x0002c49182a3f090uL}, {0x3c71affc2b91ce27uL},
    {0x0006e89f995ad3aduL}, {0x3c8c1a7792cb3386uL}, {0x0001199bdd85529cuL},
    {0x3c736eae30af0cb3uL}, {0x00035818dcfba487uL}, {0x3c74a385a63d07a8uL},
    {0x0001a4afa2a490dauL}, {0xbc8ff7128fd391f0uL},
};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dexpm1_ha_dc7 = {0x3efa01f8f4be0535uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dexpm1_ha_dc6 = {0x3f2a01f8f4b129ccuL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dexpm1_ha_dc5 = {0x3f56c16c16304601uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dexpm1_ha_dc4 = {0x3f81111110a65711uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dexpm1_ha_dc3 = {0x3fa555555555560auL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dexpm1_ha_dc2 = {0x3fc55555555555f9uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dexpm1_ha_dc1 = {0x3fe0000000000000uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dexpm1_ha_dc0 = {0xbc13b588106b310fuL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dexpm1_ha_Shifter = {0x4338000000003ff0uL};
//__STATIC __CONST INT_DOUBLE_TYPE _VSTATIC(L2E) = { 0x3ff71547652B82FEuL };
static __constant union {
  unsigned int w;
  float f;
  int i;
} __dexpm1_ha_fL2E = {0x41B8AA3Bu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __dexpm1_ha_fShifter = {0x4b403ff0u};
// -log(2)_high/16
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dexpm1_ha_p_NL2H = {0xbfa62e42fefa39f0uL};
// -log(2)_low/16
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dexpm1_ha_p_NL2L = {0x3c5950d871319ff0uL};
__attribute__((always_inline)) inline int
__ocl_svml_internal_dexpm1_ha(double *pxin, double *pres) {
  int nRet = 0;
  double xin = *pxin;
  // double expm1(double xin)
  union {
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
  } x, T, Tlr, sc, xa, res;
  double dN, Rh, Rl, R, poly, A, B, Bh, Th, Tl;
  double H, Rhh, Rhl, ThRh, ThRh_l;
  int index;
  union {
    unsigned int w;
    float f;
    int i;
  } xf, fN, xL2E, fS;
  x.f = xin;
  xf.f = (float)xin;
  xL2E.f =
      SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(xf.f, __dexpm1_ha_fL2E.f, 0.0f);
  fN.f = SPIRV_OCL_BUILTIN(trunc, _f32, )(xL2E.f);
  fS.f = __dexpm1_ha_fShifter.f + fN.f;
  dN = (double)fN.f;
  index = (fS.w & 0xf) << 1;
  // reduced argument
  Rh = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dN, __dexpm1_ha_p_NL2H.f, x.f);
  Rl = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dN, __dexpm1_ha_p_NL2L.f, 0.0);
  R = Rh + Rl;
  // 2^N, N=(int)dN
  T.w32[1] = (fS.w << (20 - 4)) ^ __dexpm1_ha_Tbl_exp[index].w32[1];
  T.w32[0] = __dexpm1_ha_Tbl_exp[index].w32[0];
  Tlr.w32[1] = __dexpm1_ha_Tbl_exp[index + 1].w32[1];
  Tlr.w32[0] = 0;
  // Tlr.f = _VSTATIC(Tbl_exp)[index + 1].f + Rl;
  Tlr.f += Rl;
  // e^R - 1
  poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(__dexpm1_ha_dc7.f, R,
                                                __dexpm1_ha_dc6.f);
  poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(poly, R, __dexpm1_ha_dc5.f);
  poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(poly, R, __dexpm1_ha_dc4.f);
  poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(poly, R, __dexpm1_ha_dc3.f);
  poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(poly, R, __dexpm1_ha_dc2.f);
  poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(poly, R, __dexpm1_ha_dc1.f);
  poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(poly, R, __dexpm1_ha_dc0.f);
  poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(poly, R, Tlr.f);
  // maxabs(T,-1), minabs(T,-1)
  A = (xin >= 0.0) ? T.f : -1.0;
  B = (xin >= 0.0) ? -1.0 : T.f;
  Th = T.f - 1.0;
  Bh = Th - A;
  Tl = B - Bh;
  // T*Rh
  ThRh = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(T.f, Rh, 0.0);
  ThRh_l = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(T.f, Rh, -ThRh);
  // Th + Th*Rh
  H = ThRh + Th;
  // (Th*Rh)_high
  Rhh = H - Th;
  // (Th*Rh)_low
  Rhl = ThRh - Rhh;
  Tl = Tl + Rhl + ThRh_l;
  // 2^N*poly + Tl
  res.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(T.f, poly, Tl);
  res.f = res.f + H;
  if (SPIRV_OCL_BUILTIN(fabs, _f32, )(xf.f) <= 708.0f) {
    *pres = res.f;
    return nRet;
  }
  // special case and overflow path
  if (xf.f < 0) {
    *pres = -1.0;
    return nRet;
  }
  if (!(xf.f < 1024.0)) {
    // +Inf or NaN?
    xa.w = x.w & 0x7fffffffffffffffuL;
    if (xa.w > 0x7ff0000000000000uL) {
      *pres = x.f + res.f;
      return nRet;
    }
    // overflow
    res.w = 0x7ff0000000000000uL - 1;
    res.f = res.f * res.f; // to set OF flag
    nRet = 3;
    {
      *pres = res.f;
      return nRet;
    }
  }
  // at or near overflow
  // 2^(N-512), N=(int)dN
  T.w32[1] =
      ((fS.w - 512 * 16) << (20 - 4)) ^ __dexpm1_ha_Tbl_exp[index].w32[1];
  T.w32[0] = __dexpm1_ha_Tbl_exp[index].w32[0];
  // T.w = ((S.w - 512 * 16) << (52 - 4)) ^ _VSTATIC(Tbl_exp)[index].w;
  poly += Rh;
  res.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(T.f, poly, T.f);
  // final scaling
  sc.w = 0x5ff0000000000000uL;
  res.f *= sc.f;
  // determine if overflow
  if (res.w == 0x7ff0000000000000uL)
    nRet = 3;
  *pres = res.f;
  return nRet;
}
double __ocl_svml_expm1_ha(double x) {
  double r;
  __ocl_svml_internal_dexpm1_ha(&x, &r);
  return r;
}

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
} __dexp_ha_p_L2E = {0x3ff71547652B82FEuL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dexp_ha_p_Shifter = {0x43280000000007feuL};
// -log(2)_high
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dexp_ha_p_NL2H = {0xbfe62e42fefa39efuL};
// -log(2)_low
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dexp_ha_p_NL2L = {0xbc7abc9e3b39803fuL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dexp_ha_p_c0 = {0x3fdffffffffffe76uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dexp_ha_p_c1 = {0x3fc5555555555462uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dexp_ha_p_c2 = {0x3fa55555556228ceuL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dexp_ha_p_c3 = {0x3f811111111ac486uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dexp_ha_p_c4 = {0x3f56c16b8144bd5buL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dexp_ha_p_c5 = {0x3f2a019f7560fba3uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dexp_ha_p_c6 = {0x3efa072e44b58159uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dexp_ha_p_c7 = {0x3ec722bccc270959uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dexp_ha_p_one = {0x3ff0000000000000uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dexp_ha_thres = {0x4086232A00000000uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dexp_ha_min_norm = {0x0010000000000000uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dexp_ha_Inf = {0x7ff0000000000000uL};
__attribute__((always_inline)) inline int
__ocl_svml_internal_dexp_ha(double *a, double *r) {
  int nRet = 0;
  double x = *a;
  union {
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
  } xi, zero, res_special, scale;
  union {
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
  } idx, T, Tlr;
  double N, R, R0, poly, res;
  int expon32, mask32, mask_h;
  unsigned int xa32, sgn_x, expon_corr;
  // x*log2(e) + Shifter
  idx.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(x, __dexp_ha_p_L2E.f,
                                                 __dexp_ha_p_Shifter.f);
  // x*log2(e), rounded to 1 fractional bit
  N = idx.f - __dexp_ha_p_Shifter.f;
  // bit mask to select "table" value
  mask32 = idx.w32[0] << 31;
  // prepare exponent
  expon32 = idx.w32[0] << (20 + 31 - 32);
  // initial reduced argument
  R0 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(__dexp_ha_p_NL2H.f, N, x);
  // reduced argument
  R = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(__dexp_ha_p_NL2L.f, N, R0);
  // start polynomial computation
  poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(__dexp_ha_p_c7.f, R,
                                                __dexp_ha_p_c6.f);
  poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(poly, R, __dexp_ha_p_c5.f);
  // bit mask to select "table" value
  mask32 = mask32 >> 31;
  // polynomial
  poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(poly, R, __dexp_ha_p_c4.f);
  poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(poly, R, __dexp_ha_p_c3.f);
  // "table" correction
  // mask.w &= 0x000EA09E667F3BCDuL;
  mask_h = mask32 & 0x000EA09E;
  // polynomial
  poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(poly, R, __dexp_ha_p_c2.f);
  poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(poly, R, __dexp_ha_p_c1.f);
  // combine exponent, "table" value
  T.w32[1] = expon32 ^ mask_h;
  T.w32[0] = mask32 & 0x667F3BCD;
  Tlr.w32[1] = 0x3C6E51C5 ^ (mask32 & (0xBC8FD36E ^ 0x3C6E51C5)); // 0xBC93B3EF;
  Tlr.w32[0] = 0;
  // polynomial
  poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(poly, R, __dexp_ha_p_c0.f);
  poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(poly, R, __dexp_ha_p_one.f);
  poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(poly, R, Tlr.f);
  // if (xa32 > 0x4086232Au)
  if (SPIRV_OCL_BUILTIN(fabs, _f64, )(x) >= __dexp_ha_thres.f)
    goto EXP_SPECIAL_PATH;
  // result
  res = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(T.f, poly, T.f);
  *r = res;
  return nRet;
EXP_SPECIAL_PATH:
  xi.f = x;
  xa32 = xi.w32[1] & 0x7fffffffu;
  // sign of x
  sgn_x = xa32 ^ xi.w32[1];
  if (xa32 < 0x40879127u) {
    expon_corr = sgn_x ? 0x08000000u : 0xF8000000u;
    scale.w = sgn_x ? 0x37f0000000000000uL : 0x47f0000000000000uL;
    // apply correction (+/-128) to exponent embedded in T
    T.w32[1] += expon_corr;
    // result
    res = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(T.f, poly, T.f);
    // final scaling
    res *= scale.f;
  } else {
    // underflow or overflow?
    res_special.w = sgn_x ? 0x0000000000000000uL : 0x7ff0000000000000uL;
    // check for NaNs
    xi.w32[1] = xa32;
    res_special.f = (xi.w <= 0x7ff0000000000000uL) ? res_special.f : x;
    // quietize NaNs
    zero.w = 0;
    res = res_special.f + zero.f;
  }
  nRet = (res < __dexp_ha_min_norm.f) ? 4 : nRet;
  nRet = (res == __dexp_ha_Inf.f) ? 3 : nRet;
  *r = res;
  return nRet;
}
double __ocl_svml_exp_ha(double x) {
  double r;
  __ocl_svml_internal_dexp_ha(&x, &r);
  return r;
}

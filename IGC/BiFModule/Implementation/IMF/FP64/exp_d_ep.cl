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
} __dexp_ep_p_L2Ef = {0x3fB8AA3Bu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __dexp_ep_p_Shifterf0 = {0x4b4003ffu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __dexp_ep_p_fthres = {0x4431195c};
// -log(2)_high
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dexp_ep_p_NL2H = {0xbfe62e42fefa39efuL};
// -log(2)_low
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dexp_ep_p_NL2L = {0xbc7abc9e3b39803fuL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dexp_ep_c6 = {0x3f56dd9818211af0uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dexp_ep_c5 = {0x3f8126fababd1cf2uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dexp_ep_c4 = {0x3fa55541c4c8cb89uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dexp_ep_c3 = {0x3fc55540432ea07buL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dexp_ep_c2 = {0x3fe00000090aa64auL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dexp_ep_c1 = {0x3ff000000a208385uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dexp_ep_c0 = {0xbdd63f26cce7780fuL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dexp_ep_min_norm = {0x0010000000000000uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dexp_ep_Inf = {0x7ff0000000000000uL};
__attribute__((always_inline)) inline int
__ocl_svml_internal_dexp_ep(double *a, double *r) {
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
  } T;
  double N, R, R0, poly, res;
  int expon32, mask32, mask_h;
  unsigned int xa32, sgn_x, expon_corr;
  union {
    unsigned int w;
    float f;
    int i;
  } idx;
  float fN, xf;
  xf = (float)x;
  // x*log2(e) + Shifter
  idx.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(xf, __dexp_ep_p_L2Ef.f,
                                                 __dexp_ep_p_Shifterf0.f);
  // x*log2(e), rounded to integral
  fN = idx.f - __dexp_ep_p_Shifterf0.f;
  N = (double)fN;
  // prepare exponent
  T.w32[1] = idx.w << 20;
  T.w32[0] = 0;
  // reduced argument
  R = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(__dexp_ep_p_NL2H.f, N, x);
  // start polynomial computation
  poly =
      SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(__dexp_ep_c6.f, R, __dexp_ep_c5.f);
  poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(poly, R, __dexp_ep_c4.f);
  poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(poly, R, __dexp_ep_c3.f);
  poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(poly, R, __dexp_ep_c2.f);
  poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(poly, R, __dexp_ep_c1.f);
  poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(poly, R, __dexp_ep_c0.f);
  if (SPIRV_OCL_BUILTIN(fabs, _f32, )(xf) > __dexp_ep_p_fthres.f)
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
  nRet = (res < __dexp_ep_min_norm.f) ? 4 : nRet;
  nRet = (res == __dexp_ep_Inf.f) ? 3 : nRet;
  *r = res;
  return nRet;
}
double __ocl_svml_exp_ep(double x) {
  double r;
  __ocl_svml_internal_dexp_ep(&x, &r);
  return r;
}

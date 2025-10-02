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
} __dexpm1_ep_dc7 = {0x3efa5b0597ae2842uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dexpm1_ep_dc6 = {0x3f2a5b062ac4ca1auL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dexpm1_ep_dc5 = {0x3f56c0d9c1d50009uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dexpm1_ep_dc4 = {0x3f8110a57824968buL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dexpm1_ep_dc3 = {0x3fa555560b7d4cbeuL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dexpm1_ep_dc2 = {0x3fc55555faec57afuL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dexpm1_ep_dc1 = {0x3fdfffffff013d80uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dexpm1_ep_dc0 = {0xbe13e699df82e534uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dexpm1_ep_Shifter = {0x43380000000003ffuL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dexpm1_ep_L2E = {0x3ff71547652B82FEuL};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __dexpm1_ep_fL2E = {0x3FB8AA3Bu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __dexpm1_ep_fShifter = {0x4b4003ffu};
// -log(2)_high
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dexpm1_ep_p_NL2H = {0xbfe62e42fefa39efuL};
// -log(2)_low
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dexpm1_ep_p_NL2L = {0xbc7abc9e3b39803fuL};
__attribute__((always_inline)) inline int
__ocl_svml_internal_dexpm1_ep(double *pxin, double *pres) {
  int nRet = 0;
  double xin = *pxin;
  // double expm1(double xin)
  union {
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
  } x, T, sc, xa, res;
  double dN, R, poly, Th;
  union {
    unsigned int w;
    float f;
    int i;
  } x0f, fS, fN;
  x0f.f = (float)xin;
  // 2^52*1.5 + x*log2(e)
  fS.f = __spirv_ocl_fma(x0f.f, __dexpm1_ep_fL2E.f,
                                                __dexpm1_ep_fShifter.f);
  // dN = rint(x*log2(e))
  fN.f = fS.f - __dexpm1_ep_fShifter.f;
  // reduced argument
  dN = (double)fN.f;
  // reduced argument
  R = __spirv_ocl_fma(dN, __dexpm1_ep_p_NL2H.f, xin);
  // R = DP_FMA(dN, _VSTATIC(p_NL2L).f, R);
  //  2^N, N=(int)dN
  T.w32[1] = fS.w << 20;
  T.w32[0] = 0;
  // e^R - 1
  poly = __spirv_ocl_fma(__dexpm1_ep_dc7.f, R,
                                                __dexpm1_ep_dc6.f);
  poly = __spirv_ocl_fma(poly, R, __dexpm1_ep_dc5.f);
  poly = __spirv_ocl_fma(poly, R, __dexpm1_ep_dc4.f);
  poly = __spirv_ocl_fma(poly, R, __dexpm1_ep_dc3.f);
  poly = __spirv_ocl_fma(poly, R, __dexpm1_ep_dc2.f);
  poly = __spirv_ocl_fma(poly, R, __dexpm1_ep_dc1.f);
  poly = __spirv_ocl_fma(poly, R, __dexpm1_ep_dc0.f);
  poly = __spirv_ocl_fma(poly, R, R);
  // maxabs(T,-1), minabs(T,-1)
  Th = T.f - 1.0;
  // 2^N*poly + Tl
  res.f = __spirv_ocl_fma(T.f, poly, Th);
  if (__spirv_ocl_fabs(x0f.f) <= 708.0f) {
    *pres = res.f;
    return nRet;
  }
  // special path
  if (x0f.f < 0) {
    *pres = -1.0;
    return nRet;
  }
  if (!(x0f.f < 1024.0f)) {
    // +Inf or NaN?
    x.f = xin;
    xa.w = x.w & 0x7fffffffffffffffuL;
    if (xa.w > 0x7ff0000000000000uL) {
      *pres = x.f + res.f;
      return nRet;
    }
    // overflow
    res.w = (res.w & 0x0007ffffffffffffuL) | 0x7fd0000000000000uL;
    res.f = res.f * xin; // to set OF flag
    nRet = 3;
    {
      *pres = res.f;
      return nRet;
    }
  }
  // at or near overflow
  // 2^(N-512), N=(int)dN
  T.w32[1] = (fS.w - 512) << 20;
  T.w32[0] = 0;
  res.f = __spirv_ocl_fma(T.f, poly, T.f);
  // final scaling
  sc.w = 0x5ff0000000000000uL;
  res.f *= sc.f;
  // determine if overflow
  if (res.w == 0x7ff0000000000000uL)
    nRet = 3;
  *pres = res.f;
  return nRet;
}
double __ocl_svml_expm1_ep(double x) {
  double r;
  __ocl_svml_internal_dexpm1_ep(&x, &r);
  return r;
}

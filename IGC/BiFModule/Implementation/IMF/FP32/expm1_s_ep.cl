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
} __sexpm1_ep_c4 = {0x3c0b4a77u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __sexpm1_ep_c3 = {0x3d2e1d2cu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __sexpm1_ep_c2 = {0x3e2aa0ccu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __sexpm1_ep_c1 = {0x3efff2b6u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __sexpm1_ep_c0 = {0x35bb1a40u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __sexpm1_ep_fL2E = {0x3FB8AA3Bu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __sexpm1_ep_fShifter = {0x4b40007fu};
// log(2) high, low
static __constant union {
  unsigned int w;
  float f;
  int i;
} __sexpm1_ep_NL2H = {0xbf317218u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __sexpm1_ep_NL2L = {0x3102E308u};
__attribute__((always_inline)) inline int
__ocl_svml_internal_sexpm1_ep(float *a, float *pres) {
  int nRet = 0;
  float xin = *a;
  // float expm1f(float xin)
  union {
    unsigned int w;
    float f;
    int i;
  } xf, fN, xL2E, fS, xa;
  union {
    unsigned int w;
    float f;
    int i;
  } T, sc, res;
  float R, poly, Th;
  xf.f = xin;
  xL2E.f =
      SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(xf.f, __sexpm1_ep_fL2E.f, 0.0f);
  fN.f = SPIRV_OCL_BUILTIN(trunc, _f32, )(xL2E.f);
  fS.f = __sexpm1_ep_fShifter.f + fN.f;
  // reduced argument
  R = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(fN.f, __sexpm1_ep_NL2H.f, xf.f);
  // R = SP_FMA(fN.f, _VSTATIC(NL2L).f, R);
  //  2^N
  T.w = fS.w << 23;
  // e^R - 1
  poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(__sexpm1_ep_c4.f, R,
                                                __sexpm1_ep_c3.f);
  poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(poly, R, __sexpm1_ep_c2.f);
  poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(poly, R, __sexpm1_ep_c1.f);
  poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(poly, R, __sexpm1_ep_c0.f);
  poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(poly, R, R);
  // maxabs(T,-1), minabs(T,-1)
  Th = T.f - 1.0f;
  // 2^N*poly + 2^N - 1
  res.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(T.f, poly, Th);
  // ensure expm1(-0)=-0
  xa.f = SPIRV_OCL_BUILTIN(fabs, _f32, )(xf.f);
  res.w |= (xf.w ^ xa.w);
  if (SPIRV_OCL_BUILTIN(fabs, _f32, )(xf.f) <= 87.0f) {
    *pres = res.f;
    return nRet;
  }
  // special case and overflow path
  if (xf.f < 0.0f) {
    *pres = -1.0f;
    return nRet;
  }
  if (!(xf.f < 128.0f)) {
    // +Inf or NaN?
    xa.w = xf.w & 0x7fffffff;
    if (xa.w > 0x7f800000) {
      *pres = xf.f + res.f;
      return nRet;
    }
    // overflow
    res.w = 0x7f800000 - 1;
    res.f = res.f * res.f; // to set OF flag
                           //
    nRet = 3;
    {
      *pres = res.f;
      return nRet;
    }
  }
  // at or near overflow
  // 2^(N-64), N=(int)dN
  T.w = (fS.w - 64) << 23;
  res.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(T.f, poly, T.f);
  // final scaling
  sc.w = 0x5f800000u;
  res.f *= sc.f;
  // determine if overflow
  if (res.w == 0x7f800000)
    nRet = 3;
  *pres = res.f;
  return nRet;
}
float __ocl_svml_expm1f_ep(float x) {
  float r;
  __ocl_svml_internal_sexpm1_ep(&x, &r);
  return r;
}

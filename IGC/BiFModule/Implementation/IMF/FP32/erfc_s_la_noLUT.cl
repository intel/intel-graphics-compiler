/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF
static __constant union {
  unsigned int w;
  float f;
} __serfc_la___cp012 = {0x37a106b5u};
static __constant union {
  unsigned int w;
  float f;
} __serfc_la___cp011 = {0xb884ad7au};
static __constant union {
  unsigned int w;
  float f;
} __serfc_la___cp010 = {0x39080063u};
static __constant union {
  unsigned int w;
  float f;
} __serfc_la___cp09 = {0xb9bbdf7du};
static __constant union {
  unsigned int w;
  float f;
} __serfc_la___cp08 = {0x3a89c274u};
static __constant union {
  unsigned int w;
  float f;
} __serfc_la___cp07 = {0xbb3668a2u};
static __constant union {
  unsigned int w;
  float f;
} __serfc_la___cp06 = {0x3be46aacu};
static __constant union {
  unsigned int w;
  float f;
} __serfc_la___cp05 = {0xbc8875d5u};
static __constant union {
  unsigned int w;
  float f;
} __serfc_la___cp04 = {0x3d19e520u};
static __constant union {
  unsigned int w;
  float f;
} __serfc_la___cp03 = {0xbda24203u};
static __constant union {
  unsigned int w;
  float f;
} __serfc_la___cp02 = {0x3e1e1396u};
static __constant union {
  unsigned int w;
  float f;
} __serfc_la___cp01 = {0xbe8be271u};
static __constant union {
  unsigned int w;
  float f;
} __serfc_la___cp00 = {0x3edaec3cu};
static __constant union {
  unsigned int w;
  float f;
} __serfc_la___cp9 = {0x3f806f08u};
static __constant union {
  unsigned int w;
  float f;
} __serfc_la___cp8 = {0xc04175e9u};
static __constant union {
  unsigned int w;
  float f;
} __serfc_la___cp7 = {0x406e4be1u};
static __constant union {
  unsigned int w;
  float f;
} __serfc_la___cp6 = {0xc00857e3u};
static __constant union {
  unsigned int w;
  float f;
} __serfc_la___cp5 = {0x3e3e70e5u};
static __constant union {
  unsigned int w;
  float f;
} __serfc_la___cp4 = {0x3ece9855u};
static __constant union {
  unsigned int w;
  float f;
} __serfc_la___cp3 = {0x3a9d2157u};
static __constant union {
  unsigned int w;
  float f;
} __serfc_la___cp2 = {0xbe9073a2u};
static __constant union {
  unsigned int w;
  float f;
} __serfc_la___cp1 = {0x34f43978u};
static __constant union {
  unsigned int w;
  float f;
} __serfc_la___cp0 = {0x3f106ebau};
static __constant union {
  unsigned int w;
  float f;
} __serfc_la___cp0l = {0x32fc4fe5u};
static __constant union {
  unsigned int w;
  float f;
} __serfc_la___ce6 = {0x3ab6ecc1u};
static __constant union {
  unsigned int w;
  float f;
} __serfc_la___ce5 = {0x3c0937d6u};
static __constant union {
  unsigned int w;
  float f;
} __serfc_la___ce4 = {0x3d2aaa0eu};
static __constant union {
  unsigned int w;
  float f;
} __serfc_la___ce3 = {0x3e2aaa02u};
static __constant union {
  unsigned int w;
  float f;
} __serfc_la___ce2 = {0x3f000000u};
static __constant union {
  unsigned int w;
  float f;
} __serfc_la___ce1 = {0x3f800000u};
static __constant union {
  unsigned int w;
  float f;
} __serfc_la___ce0 = {0xaeb1f936u};
static __constant union {
  unsigned int w;
  float f;
} __serfc_la___L2E = {0x3FB8AA3Bu};
static __constant union {
  unsigned int w;
  float f;
} __serfc_la___Shifter = {0x4b4000feu};
static __constant union {
  unsigned int w;
  float f;
} __serfc_la___L2H = {0x3f317218u};
static __constant union {
  unsigned int w;
  float f;
} __serfc_la___L2L = {0xb102e308u};
static __constant union {
  unsigned int w;
  float f;
} __serfc_la___Smax = {0x4b4000ffu};
static __constant union {
  unsigned int w;
  float f;
} __serfc_la___Inf = {0x7f800000u};

__attribute__((always_inline))
static inline int __ocl_svml_internal_serfc(float *px, float *pres)
{
  int nRet = 0;
  float xin = *px;
  int_float xa, res, apoly, poly, Te, Te2, S, N, two;
  int sgn_x;
  float R, x2h, mx2l;
  xa.f = xin;
  sgn_x = xa.w & 0x80000000;
  // |xin|
  xa.w ^= sgn_x;
  if (xa.f <= 2.0f) {
    R = xa.f - 1.0f;
    apoly.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(__serfc_la___cp012.f, R,
                                                     __serfc_la___cp011.f);
    apoly.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(apoly.f, R,
                                                     __serfc_la___cp010.f);
    apoly.f =
        SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(apoly.f, R, __serfc_la___cp09.f);
    apoly.f =
        SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(apoly.f, R, __serfc_la___cp08.f);
    apoly.f =
        SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(apoly.f, R, __serfc_la___cp07.f);
    apoly.f =
        SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(apoly.f, R, __serfc_la___cp06.f);
    apoly.f =
        SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(apoly.f, R, __serfc_la___cp05.f);
    apoly.f =
        SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(apoly.f, R, __serfc_la___cp04.f);
    apoly.f =
        SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(apoly.f, R, __serfc_la___cp03.f);
    apoly.f =
        SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(apoly.f, R, __serfc_la___cp02.f);
    apoly.f =
        SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(apoly.f, R, __serfc_la___cp01.f);
    apoly.f =
        SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(apoly.f, R, __serfc_la___cp00.f);
    // res.f = apoly.f;
  } else {
    R = 1.0f / xa.f; // get_rcp(xa.f);
    apoly.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(__serfc_la___cp9.f, R,
                                                     __serfc_la___cp8.f);
    apoly.f =
        SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(apoly.f, R, __serfc_la___cp7.f);
    apoly.f =
        SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(apoly.f, R, __serfc_la___cp6.f);
    apoly.f =
        SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(apoly.f, R, __serfc_la___cp5.f);
    apoly.f =
        SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(apoly.f, R, __serfc_la___cp4.f);
    apoly.f =
        SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(apoly.f, R, __serfc_la___cp3.f);
    apoly.f =
        SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(apoly.f, R, __serfc_la___cp2.f);
    apoly.f =
        SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(apoly.f, R, __serfc_la___cp1.f);
    apoly.f =
        SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(apoly.f, R, __serfc_la___cp0l.f);
    // res.f = SP_FMA(apoly.f, R, SP_FMA(R, _VSTATIC(__cp0l).f, 0.0f));
    apoly.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(
        R, __serfc_la___cp0.f,
        SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(apoly.f, R, 0.0f));
  }
  // limit input range for exp() calculation
  xa.f = SPIRV_OCL_BUILTIN(fmin, _f32_f32, )(xa.f, 12.0f);
  // exp(-xin^2)
  x2h = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(xa.f, -xa.f, 0.0f);
  mx2l = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(xa.f, xa.f, x2h);
  // x2h*L2E + Shifter
  S.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(x2h, __serfc_la___L2E.f,
                                               __serfc_la___Shifter.f);
  // (int)(x2h*L2E)
  N.f = S.f - __serfc_la___Shifter.f;
  // x^2 - N*log(2)
  R = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(-N.f, __serfc_la___L2H.f, x2h);
  R = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(-N.f, __serfc_la___L2L.f, R);
  R = R - mx2l;
  // 2^(N)
  Te2.w = S.w & 1;
  Te.w = (S.w ^ Te2.w) << 22;
  Te2.w = (Te2.w << 23) + Te.w;
  // exp(R)-1
  poly.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(__serfc_la___ce6.f, R,
                                                  __serfc_la___ce5.f);
  poly.f =
      SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(poly.f, R, __serfc_la___ce4.f);
  poly.f =
      SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(poly.f, R, __serfc_la___ce3.f);
  poly.f =
      SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(poly.f, R, __serfc_la___ce2.f);
  poly.f =
      SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(poly.f, R, __serfc_la___ce1.f);
  poly.f =
      SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(poly.f, R, __serfc_la___ce0.f);
  // poly*apoly*Te
  poly.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(poly.f, apoly.f, 0.0f);
  poly.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(poly.f, Te.f, 0.0f);
  // Te*apoly + (poly*apoly*Te)
  res.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(Te.f, apoly.f, poly.f);
  // additional scaling (to treat underflow cases)
  res.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(res.f, Te2.f, 0.0f);
  // apply sign
  res.w ^= sgn_x;
  // corection based on sign
  sgn_x = ((int)sgn_x) >> 31;
  two.w = sgn_x & 0x40000000;
  res.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(res.f, 1.0f, two.f);
  nRet = (res.w < 0x00800000) ? 4 : nRet;
  *pres = res.f;
  return nRet;
}

float __ocl_svml_erfcf_noLUT(float x)
{
  float r;
  __ocl_svml_internal_serfc(&x, &r);
  return r;
}

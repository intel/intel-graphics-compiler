/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF
#pragma float_control(precise, on)
#pragma float_control(precise, on)
static __constant union {
  unsigned int w;
  float f;
  int i;
} __serf_ep___b5 = {0x3605524cu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __serf_ep___b4 = {0x39953450u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __serf_ep___b3 = {0x3b7e8d75u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __serf_ep___b2 = {0x3d5983e4u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __serf_ep___b1 = {0x3e4635acu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __serf_ep___b0 = {0x3f906ebau};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __serf_ep___a5 = {0x381cf31fu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __serf_ep___a4 = {0x3a9b6bd9u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __serf_ep___a3 = {0x3c792ec0u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __serf_ep___a2 = {0x3dec40c3u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __serf_ep___a1 = {0x3f013f71u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __serf_ep___a0 = {0x3f800000u};
__attribute__((always_inline)) inline int
__ocl_svml_internal_serf_ep(float *a, float *pres) {
  int nRet = 0;
  float xin = *a;
  union {
    unsigned int w;
    float f;
    int i;
  } x, xa, res;
  int iexpon;
  unsigned int sgn_x;
  float dR, dR2;
  union {
    unsigned int w;
    float f;
    int i;
  } apoly, bpoly, Y;
  xa.f = xin;
  sgn_x = xa.w & 0x80000000;
  // |xin|
  xa.w ^= sgn_x;
  // limit |x| range to [0,4]
  dR = (xa.f > 4.0f) ? 4.0f : xa.f;
  dR2 = __spirv_ocl_fma(dR, dR, 0.0f);
  // fixup for NaNs
  dR = (xa.w > 0x7f800000uL) ? xa.f : dR;
  // polynomial evaluation
  bpoly.f = __spirv_ocl_fma(__serf_ep___b5.f, dR2,
                                                   __serf_ep___b4.f);
  apoly.f = __spirv_ocl_fma(__serf_ep___a5.f, dR2,
                                                   __serf_ep___a4.f);
  bpoly.f =
      __spirv_ocl_fma(bpoly.f, dR2, __serf_ep___b3.f);
  apoly.f =
      __spirv_ocl_fma(apoly.f, dR2, __serf_ep___a3.f);
  bpoly.f =
      __spirv_ocl_fma(bpoly.f, dR2, __serf_ep___b2.f);
  apoly.f =
      __spirv_ocl_fma(apoly.f, dR2, __serf_ep___a2.f);
  bpoly.f =
      __spirv_ocl_fma(bpoly.f, dR2, __serf_ep___b1.f);
  apoly.f =
      __spirv_ocl_fma(apoly.f, dR2, __serf_ep___a1.f);
  bpoly.f =
      __spirv_ocl_fma(bpoly.f, dR2, __serf_ep___b0.f);
  apoly.f =
      __spirv_ocl_fma(apoly.f, dR2, __serf_ep___a0.f);
  Y.f = 1.0f / apoly.f;
  bpoly.f = __spirv_ocl_fma(bpoly.f, dR, 0.0f);
  res.f = __spirv_ocl_fma(bpoly.f, Y.f, 0.0f);
  res.f = (res.f > 1.0f) ? 1.0f : res.f;
  // compiler workaround for NaNs
  res.f = (xa.w <= 0x7f800000) ? res.f : (xa.f + xa.f);
  res.w ^= sgn_x;
  *pres = res.f;
  return nRet;
}
float __ocl_svml_erff_ep(float x) {
  float r;
  __ocl_svml_internal_serf_ep(&x, &r);
  return r;
}

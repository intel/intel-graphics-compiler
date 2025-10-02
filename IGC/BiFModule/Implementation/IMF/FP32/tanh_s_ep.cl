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
} __stanh_ep_nc2 = {0x3c520a84u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __stanh_ep_nc1 = {0x3edef102u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __stanh_ep_nc0 = {0x3f800000u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __stanh_ep_dc2 = {0x3a2fc8e6u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __stanh_ep_dc1 = {0x3dd1c060u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __stanh_ep_dc0 = {0xb859e195u};
__attribute__((always_inline)) inline int
__ocl_svml_internal_stanh_ep(float *a, float *r) {
  int nRet = 0;
  float xin = *a;
  union {
    unsigned int w;
    float f;
    int i;
  } dpoly, npoly, R2;
  union {
    unsigned int w;
    float f;
    int i;
  } x, x0, xa, y;
  unsigned int sgn_x;
  x0.f = xin;
  xa.w = x0.w & 0x7fffffff;
  sgn_x = xa.w ^ x0.w;
  R2.f = xa.f * xa.f;
  npoly.f = __spirv_ocl_fma(R2.f, __stanh_ep_nc2.f,
                                                   __stanh_ep_nc1.f);
  dpoly.f = __spirv_ocl_fma(R2.f, __stanh_ep_dc2.f,
                                                   __stanh_ep_dc1.f);
  npoly.f =
      __spirv_ocl_fma(npoly.f, R2.f, __stanh_ep_nc0.f);
  dpoly.f =
      __spirv_ocl_fma(dpoly.f, R2.f, __stanh_ep_dc0.f);
  dpoly.f = __spirv_ocl_fma(dpoly.f, xa.f, xa.f);
  // 1.0/npoly
  y.f = 1.0f / (npoly.f);
  // dpoly/npoly
  y.f = y.f * dpoly.f;
  // fix bounds
  y.f = (xa.f >= 5.0f) ? 1.0f : y.f;
  // fix sign
  y.w ^= sgn_x;
  *r = y.f;
  return nRet;
}
float __ocl_svml_tanhf_ep(float x) {
  float r;
  __ocl_svml_internal_stanh_ep(&x, &r);
  return r;
}

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
} __satan_ep_c4 = {0xbca5054fu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __satan_ep_c3 = {0x3e49099du};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __satan_ep_c2 = {0xbecbaf63u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __satan_ep_c1 = {0x3bef4e52u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __satan_ep_c0 = {0x3f7ff759u};
__attribute__((always_inline)) inline int
__ocl_svml_internal_satan_ep(float *pxin, float *pres) {
  int nRet = 0;
  float xin = *pxin;
  // float atanf_ep(float xin)
  {
    union {
      unsigned int w;
      float f;
      int i;
    } x, xa, hcorr, ya, R0, sres;
    int sgn_x, smask, sgn_r, diff;
    float poly, R;
    x.f = xin;
    xa.w = x.w & 0x7fffffffu;
    sgn_x = x.w ^ xa.w;
    // y ~ 1/x
    ya.f = 1.0f / (xa.f);
    // smask = (|x|>1.0)? -1 : 0
    diff = ya.w - xa.w;
    smask = ((int)diff) >> 31;
    // will compute pi/2 - atan(1/|x|) for |x|>1
    hcorr.w = smask & 0xbfc90FDB;
    sgn_r = sgn_x ^ (smask & 0x80000000u);
    // reduced argument
    R0.w = xa.w + (diff & smask);
    R = R0.f;
    poly = __spirv_ocl_fma(__satan_ep_c4.f, R,
                                                  __satan_ep_c3.f);
    poly = __spirv_ocl_fma(poly, R, __satan_ep_c2.f);
    poly = __spirv_ocl_fma(poly, R, __satan_ep_c1.f);
    poly = __spirv_ocl_fma(poly, R, __satan_ep_c0.f);
    //  R0*poly + hcorr
    sres.f = __spirv_ocl_fma(poly, R0.f, hcorr.f);
    sres.w = sres.w ^ sgn_r;
    *pres = sres.f;
  }
  return nRet;
}
float __ocl_svml_atanf_ep(float x) {
  float r;
  __ocl_svml_internal_satan_ep(&x, &r);
  return r;
}

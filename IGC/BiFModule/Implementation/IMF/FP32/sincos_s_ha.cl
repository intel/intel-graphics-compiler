/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF
#pragma float_control(precise, on)
static __constant int __ssincos_ha___ip_h = 0x0517CC1B;
static __constant int __ssincos_ha___ip_m = 0x727220A9;
static __constant int __ssincos_ha___ip_l = 0x28;
static __constant union {
  unsigned int w;
  float f;
  int i;
} __ssincos_ha___cc4 = {0x3e6ce1b2u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __ssincos_ha___cc3 = {0xbfaae2beu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __ssincos_ha___cc2 = {0x4081e0eeu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __ssincos_ha___cc1 = {0xc09de9e6u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __ssincos_ha___cc1l = {0xb3e646a5u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __ssincos_ha___cc0 = {0x3f800000u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __ssincos_ha___cs3 = {0xbf16c981u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __ssincos_ha___cs2 = {0x40232f49u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __ssincos_ha___cs1 = {0xc0a55dddu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __ssincos_ha___cs0 = {0x40490fdbu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __ssincos_ha___cs0l = {0xb3d195e9u};
static __constant unsigned int __ssincos_ha_invpi_tbl[] = {
    0,          0x28BE60DB, 0x9391054A, 0x7F09D5F4,
    0x7D4D3770, 0x36D8A566, 0x4F10E410, 0x7F9458EA};
__attribute__((always_inline)) inline int
__ocl_svml_internal_ssincos_ha(float *pa, float *psin, float *pcos) {
  int nRet = 0;
  float xin = *pa;
  unsigned long IP, IP2;
  long IP_s, IP2_s;
  int ip_low_s;
  unsigned int ip_low;
  volatile int_float x;
  int_float Rh, Rl, res, scale, cres, sres, spoly, cpoly, cpoly_l;
  int mx, sgn_x, ex, ip_h, shift, index, j, sgn_p, sgn_xp;
  float High, Low, R2h, R2l, Ph, Pl;
  x.f = xin;
  mx = (x.w & 0x007fffff) | 0x00800000;
  sgn_x = x.w & 0x80000000;
  ex = ((x.w ^ sgn_x) >> 23);
  // redirect large or very small inputs
  if (__builtin_expect(((unsigned)(ex - 0x7f + 12)) > (20 + 12), (0 == 1))) {
    // small input
    if (__builtin_expect((ex < 0x7f - 11), (1 == 1))) {
      *psin = xin;
      *pcos = 1.0f;
      return nRet;
    }
    // Inf/NaN
    if (ex == 0xff) {
      nRet = ((x.w << 1) == 0xff000000) ? 1 : nRet;
      x.w |= 0x00400000;
      *psin = *pcos = x.f;
      return nRet;
    }
    ex = ex - 0x7f - 23;
    index = 1 + (ex >> 5);
    // expon % 32
    j = ex & 0x1f;
    // x/Pi, scaled by 2^(63-j)
    ip_low =
        (((unsigned int)__ssincos_ha_invpi_tbl[index]) * ((unsigned int)mx));
    IP = (((unsigned long)((unsigned int)(__ssincos_ha_invpi_tbl[index + 1]))) *
          ((unsigned int)(mx))) +
         (((unsigned long)ip_low) << 32);
    // scaled by 2^(95-j)
    IP2 =
        (((unsigned long)((unsigned int)(__ssincos_ha_invpi_tbl[index + 2]))) *
         ((unsigned int)(mx))) +
        ((((unsigned long)((unsigned int)(__ssincos_ha_invpi_tbl[index + 3]))) *
          ((unsigned int)(mx))) >>
         32);
    IP = IP + (IP2 >> 32);
    // scale 2^63
    IP <<= j;
    // shift low part by 32-j, j in [0,31]
    ip_low = (unsigned int)IP2;
    ip_low >>= (31 - j);
    ip_low >>= 1;
    IP |= (unsigned long)ip_low;
  } else // main path
  {
    // products are really unsigned; operands are small enough so that signed
    // MuL works as well x*(23-ex)*(1/Pi)*2^28 p[k] products fit in 31 bits each
    IP_s = (((long)((int)(mx))) * ((int)(__ssincos_ha___ip_h)));
    IP = (unsigned long)IP_s;
    IP2_s = (((long)((int)(mx))) * ((int)(__ssincos_ha___ip_m)));
    IP2 = (unsigned long)IP2_s;
    // scale (23-ex)*2^(28+32+7)
    ip_low_s = (((int)mx) * ((int)__ssincos_ha___ip_l));
    ip_low = (unsigned int)ip_low_s;
    IP2 = (IP2 << 7) + ip_low;
    // (x/Pi)*2^63
    IP <<= (ex - 0x7f + 12);
    // IP3 = IP2 << (37 -0x7f + ex);
    IP2 >>= (27 + 0x7f - ex);
    IP += IP2;
  }
  // return to 32-bit, scale 2^31
  ip_h = IP >> 32;
  // fix sign bit
  sgn_xp = ((ip_h + 0x20000000) & 0xc0000000);
  // reduced argument (signed, high-low), scale 2^32
  ip_h <<= 2;
  Rh.f = (float)ip_h;
  // reduced argument will need to be normalized
  shift = 2 + 30 + 0x7f - ((Rh.w >> 23) & 0xff);
  // correction for shift=0
  shift = (shift >= 2) ? shift : 2;
  // normalize
  IP <<= shift; // IP = (IP << shift) | (IP3 >> (64-shift));
  ip_h = IP >> 32;
  Rh.f = (float)ip_h;
  ip_h -= ((int)Rh.f);
  Rl.f = (float)ip_h;
  // adjust scale
  scale.w = (0x7f - 31 - shift) << 23;
  Rh.f = __spirv_ocl_fma(Rh.f, scale.f, 0.0f);
  Rl.f = __spirv_ocl_fma(Rl.f, scale.f, 0.0f);
  // (Rh+Rl)^2
  {
    R2h = __spirv_ocl_fma(Rh.f, Rh.f, 0.0f);
    R2l = __spirv_ocl_fma(Rh.f, Rh.f, -R2h);
    R2l = __spirv_ocl_fma(Rl.f, Rh.f, R2l);
    R2l = __spirv_ocl_fma(Rh.f, Rl.f, R2l);
  };
  cpoly.f = __spirv_ocl_fma(__ssincos_ha___cc4.f, R2h,
                                                   __ssincos_ha___cc3.f);
  spoly.f = __spirv_ocl_fma(__ssincos_ha___cs3.f, R2h,
                                                   __ssincos_ha___cs2.f);
  cpoly.f = __spirv_ocl_fma(cpoly.f, R2h,
                                                   __ssincos_ha___cc2.f);
  cpoly_l.f =
      __spirv_ocl_fma(R2l, __ssincos_ha___cc1.f, 0.0f);
  spoly.f = __spirv_ocl_fma(spoly.f, R2h,
                                                   __ssincos_ha___cs1.f);
  cpoly.f = __spirv_ocl_fma(cpoly.f, R2h,
                                                   __ssincos_ha___cc1l.f);
  spoly.f = __spirv_ocl_fma(spoly.f, R2h,
                                                   __ssincos_ha___cs0l.f);
  // cc0 + cc1*R2h
  Ph = __spirv_ocl_fma(R2h, __ssincos_ha___cc1.f, 0.0f);
  Pl = __spirv_ocl_fma(R2h, __ssincos_ha___cc1.f, -Ph);
  {
    float __ph, __ahl, __ahh;
    __ph =
        __spirv_ocl_fma(Ph, 1.0f, __ssincos_ha___cc0.f);
    __ahh = __spirv_ocl_fma(__ph, 1.0f,
                                                   -__ssincos_ha___cc0.f);
    __ahl = __spirv_ocl_fma(Ph, 1.0f, -__ahh);
    Pl = Pl + __ahl;
    Ph = __ph;
  };
  cpoly_l.f = __spirv_ocl_fma(cpoly_l.f, 1.0f, Pl);
  cpoly.f = __spirv_ocl_fma(cpoly.f, R2h, cpoly_l.f);
  cpoly.f = __spirv_ocl_fma(cpoly.f, 1.0f, Ph);
  sgn_p = sgn_xp & 0x80000000;
  High =
      __spirv_ocl_fma(Rh.f, __ssincos_ha___cs0.f, 0.0f);
  Low =
      __spirv_ocl_fma(Rh.f, __ssincos_ha___cs0.f, -High);
  Low = __spirv_ocl_fma(Rl.f, __ssincos_ha___cs0.f, Low);
  Low = __spirv_ocl_fma(Rh.f, spoly.f, Low);
  spoly.f = __spirv_ocl_fma(High, 1.0f, Low);
  // adjust sign
  cpoly.w ^= sgn_p;
  spoly.w ^= sgn_p ^ (sgn_xp << 1);
  sres.w = (sgn_xp & 0x40000000) ? cpoly.w : spoly.w;
  cres.w = (sgn_xp & 0x40000000) ? spoly.w : cpoly.w;
  sres.w ^= sgn_x;
  *pcos = cres.f;
  *psin = sres.f;
  return nRet;
}
void __ocl_svml_sincosf_ha(float x, float *y, float *z) {
  ;
  __ocl_svml_internal_ssincos_ha(&x, y, z);
  return;
}

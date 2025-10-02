/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
/*
// ALGORITHM DESCRIPTION:
//
//      HIGH LEVEL OVERVIEW
//      Here we use polynomial and reciprocal calculation for 32 subintervals
//      at reduction interval.
//
//      For large arguments ( |a[i]| >= LARGE_ARG_HIBITS,
//      ( where LARGE_ARG_HIBITS = 16 high bits of 12800.0 value )
//      the Payne/Hanek "pre-reduction" performed. Result of this routine
//      becomes argument for regular reduction.
//
//      The regular range reduction scheme is:
//
//          a[i] = N * pi/2^R + z
//          where R = 5 for this implementation - reduction value (2^R = 32),
//          and |z| <= pi/(2*2^R)) - this is reduced argument.
//
//      Also alternative reduction performed in parallel:
//
//          a[i] = NN * pi/2 + zz,
//          (NN = N mod 16)
//
//      The reason is getting remainder modulo Pi/2. The value zz is used
//      for reciprocal calculation.
//
//      Futher tan calculation performed by this way:
//
//         r[i] := TAU * 1/zz + C0 + C1*r + C1*c + C2*r^2 + ... + C15*r^15
//
//         (TAU - multiplier for the reciprocal 1/zz,
//         and always -1 or 0 depending on subinterval)
//
//      For tiny arguments ( |a[i]| < 2^TINY_ARG_EXP )
//      the simple separate branch used:
//
//          r[i] = a[i]
//
//      IEEE SPECIAL CONDITIONS:
//      a[i] = +/-Inf, r[i] = QNaN
//      a[i] = +/-0,   r[i] = +/-0
//      a[i] = QNaN,   r[i] = QNaN
//      a[i] = SNaN,   r[i] = QNaN
//
//
//      ALGORITHM DETAILS
//      Executable parts:
//
//      1) a[i]  = +/-Inf
//         Return r[i] := a[i]*0.0
//         and error handler called with IML_STATUS_ERRDOM error code
//
//      2) a[i]  = NaN
//         Return r[i] := a[i] * a[i]
//
//      3) Tiny arguments path
//         |a[i]| < 2^TINY_ARG_EXP,
//         where TINY_ARG_EXP = -252
//
//         3.1) a[i]  = 0.0
//              Return r[i] := a[i]
//
//         3.2) 0 < |a[i]| < 2^TINY_ARG_EXP
//              Return r[i] := TWOp55 * ( TWOpM55*a[i] - a[i] ),
//              where TWOp55 = 2^55, TWOpM55 = 2^-55
//
//              Here is path where underflow or denormal exceptions can happen
//              during intermediate computations.
//              For correct work in all rounding modes we need to
//              return a[i] - TWOpM55 * a[i]
//              To avoid disappearing of second term we using scaling
//              like this TWOp55 * ( TWOpM55*a[i] - a[i] )
//
//
//      4) Main path (the most frequently used and the most wide)
//         2^TINY_ARG_EXP <= |a[i]| < LARGE_ARG_HIBITS
//
//         a) Pre-reduction.
//
//            For large arguments |a[i]| >= LARGE_ARG_HIBITS
//            special argument pre-range-reduction routine is called:
//            NR := _vml_reduce_pio2d( a[i], rr ),
//            where NR - number of octants of pre-reduction,
//            rr[0], rr[1] - high and low parts of pre-reduced argument.
//            The Payne/Hanek algorithm is used there (not described).
//            Assign   x := rr[0]
//            In case of no pre-reduction   x := a[i]
//
//         b) Main reduction.
//
//            The standard range reduction scheme is
//            zc := x - N * (PIo32_HI + PIo32_LO + PIo32_TAIL)
//            zc - reduced argument
//
//            Integer N obtained by famous "right-shifter" technique -
//            add and subtract RS = 2^52+2^51 value.
//
//            After that we add N := N + NR*(2^(R-1))
//            if large arguments pre-reduction
//            routine called.  NR = result octant number of Pi/2
//            pre-reduction.
//            For a[i] < LARGE_ARG_HIBITS the NR = 0 and N is unchanged.
//
//            PIo32_HI and PIo32_LO are 32-bit numbers (so multiplication
//            by N is exact) and PIo32_TAIL is a 53-bit number. Together, these
//            approximate pi well enough for all cases in this restricted
//            range. Reduction performed in accurate way with low part (c) of
//            result correct processing.
//            For large arguments added c = c + rr[1].
//            Finally we have zc = z + c multiprecision value.
//
//            In parallel we are doing another reduction for
//            getting remainder modulo Pi/2.  Here we perform
//            a sort of "more rounding".
//            It means have the same computation sequences but using N = (N mod
16)
//            that is also obtained by "right shifter" technique,
//            where right shifter value is (2^55+2^56) instead of usual
(2^51+2^52)
//            Pi values presented by 38+38+53 form for accurate multiplication
by
//            14-bit of (N mod 16).
//            The result is zzc = zz+cc multiprecision value.
//
//            For existing large arguments reduction we need to add
//            extra low part rr[1] to c and cc correction terms.
//            c := c + rr[1],  cc := cc + rr[1]
//            but it is necessary to resplit z+c and zz + cc values
//            to preserve proportions betwee high and low parts.
//            Doing it this way:
//
//               v1 := z + c;   v2 := z - v1;   c := v2 + c;   z := v1;
//               v1 := zz + cc; v2 := zz - v1;  cc := v2 + cc; zz := v1;
//
//         c) General computations.
//
//            The whole computation range (Pi/2) is splitted to
//            32 even ranges and for each breakpoint we have
//            unique set of coefficients stored as table.
//            The table lookup performed by index that is 5 least
//            significant bits of integer N (octant number) value.
//
//            The constants are:
//            1) C2 ... C15 polynomial coefficients for r^2 ... r^15
//            2) C0_HI + C0_LO - accurate constant C0 term in power series
//            3) C1_HI + C1_LO - accurate coefficient C1 of r in power series
//            4) TAU - multiplier for the reciprocal, always -1 or 0
//            5) MSK - 35 significant bit mask for the reciprocal
//
//
//            The basic reconstruction formula using these constants is:
//
//               High := TAU*recip_hi + C0_HI
//               Med + Low := C1_HI*r + C1_LO*r (accurate sum)
//               Low  := Low + TAU*recip_lo + C0_LO + (C1_LO+C1_HI)*c + pol,
//                 where pol := C2*r^2 + ... + C15*r^15
//
//            The recip_hi + recip_lo is an accurate reciprocal of the remainder
//            modulo pi/2 = 1/zz
//            Finally we doing a compensated sum High + Med + Low:
//
//            Return r[i] := (High + (Med + Low))
// --
//
*/
#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF
#pragma float_control(precise, on)
static __constant int __stan_ha___ip_h = 0x0517CC1B;
static __constant int __stan_ha___ip_m = 0x727220A9;
static __constant int __stan_ha___ip_l = 0x28;
static __constant union {
  unsigned int w;
  float f;
  int i;
} __stan_ha___cc4 = {0x3e6ce1b2u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __stan_ha___cc3 = {0xbfaae2beu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __stan_ha___cc2 = {0x4081e0eeu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __stan_ha___cc1 = {0xc09de9e6u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __stan_ha___cc1l = {0xb3e646a5u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __stan_ha___cc0 = {0x3f800000u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __stan_ha___cs4 = {0x3da5e12bu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __stan_ha___cs4l = {0xb10bc3e3u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __stan_ha___cs3 = {0xbf196543u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __stan_ha___cs3l = {0x32b355c6u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __stan_ha___cs2 = {0x402335ddu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __stan_ha___cs2l = {0x338bfbf6u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __stan_ha___cs1 = {0xc0a55de7u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __stan_ha___cs1l = {0xb3ac99b0u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __stan_ha___cs0 = {0x40490fdbu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __stan_ha___cs0l = {0xb3bbc50au};
static __constant unsigned int __stan_ha_invpi_tbl[] = {
    0,          0x28BE60DB, 0x9391054A, 0x7F09D5F4,
    0x7D4D3770, 0x36D8A566, 0x4F10E410, 0x7F9458EA};
__attribute__((always_inline)) inline int
__ocl_svml_internal_stan_ha(float *a, float *pres) {
  int nRet = 0;
  float xin = *a;
  unsigned long IP, IP2;
  long IP_s, IP2_s;
  int ip_low_s;
  unsigned int ip_low;
  union {
    unsigned int w;
    float f;
    int i;
  } x, Rh, Rl, res, scale, cres, sres, spoly, cpoly, cpoly_l;
  int mx, sgn_x, ex, ip_h, shift, index, j, sgn_p, sgn_xp;
  float High, Low, R2h, R2l, Ph, Pl, eps;
  union {
    unsigned int w;
    float f;
    int i;
  } cpoly1, spoly1, spoly_l, cres_l, sres_l, tres;
  x.f = xin;
  mx = (x.w & 0x007fffff) | 0x00800000;
  sgn_x = x.w & 0x80000000;
  ex = ((x.w ^ sgn_x) >> 23);
  // redirect large or very small inputs
  if (__builtin_expect(((unsigned)(ex - 0x7f + 12)) > (20 + 12), (0 == 1))) {
    // small input
    if (__builtin_expect((ex < 0x7f - 11), (1 == 1))) {
      *pres = xin;
      return nRet;
    }
    // Inf/NaN
    if (ex == 0xff) {
      nRet = ((x.w << 1) == 0xff000000) ? 1 : nRet;
      x.w |= 0x00400000;
      *pres = x.f;
      return nRet;
    }
    ex = ex - 0x7f - 23;
    index = 1 + (ex >> 5);
    // expon % 32
    j = ex & 0x1f;
    // x/Pi, scaled by 2^(63-j)
    ip_low = (((unsigned int)__stan_ha_invpi_tbl[index]) * ((unsigned int)mx));
    IP = (((unsigned long)((unsigned int)(__stan_ha_invpi_tbl[index + 1]))) *
          ((unsigned int)(mx))) +
         (((unsigned long)ip_low) << 32);
    // scaled by 2^(95-j)
    IP2 = (((unsigned long)((unsigned int)(__stan_ha_invpi_tbl[index + 2]))) *
           ((unsigned int)(mx))) +
          ((((unsigned long)((unsigned int)(__stan_ha_invpi_tbl[index + 3]))) *
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
    IP_s = (((long)((int)(mx))) * ((int)(__stan_ha___ip_h)));
    IP = (unsigned long)IP_s;
    IP2_s = (((long)((int)(mx))) * ((int)(__stan_ha___ip_m)));
    IP2 = (unsigned long)IP2_s;
    // scale (23-ex)*2^(28+32+7)
    ip_low_s = (((int)mx) * ((int)__stan_ha___ip_l));
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
  spoly.f = __spirv_ocl_fma(__stan_ha___cs4.f, R2h,
                                                   __stan_ha___cs3.f);
  cpoly.f = __spirv_ocl_fma(__stan_ha___cc4.f, R2h,
                                                   __stan_ha___cc3.f);
  spoly.f =
      __spirv_ocl_fma(spoly.f, R2h, __stan_ha___cs2.f);
  cpoly1.f =
      __spirv_ocl_fma(cpoly.f, R2h, __stan_ha___cc2.f);
  cpoly.f = __spirv_ocl_fma(cpoly1.f, R2h, 0.0f);
  cpoly_l.f = __spirv_ocl_fma(cpoly1.f, R2h, (-cpoly.f));
  {
    float __ph, __ahl, __ahh;
    __ph = __spirv_ocl_fma(cpoly.f, 1.0f,
                                                  __stan_ha___cc1.f);
    __ahh =
        __spirv_ocl_fma(__ph, 1.0f, -__stan_ha___cc1.f);
    __ahl = __spirv_ocl_fma(cpoly.f, 1.0f, -__ahh);
    cpoly_l.f = (cpoly_l.f + __stan_ha___cc1l.f) + __ahl;
    cpoly.f = __ph;
  };
  {
    float __ph, __phl;
    __ph = __spirv_ocl_fma(cpoly.f, R2h, 0.0f);
    __phl = __spirv_ocl_fma(cpoly.f, R2h, -__ph);
    cpoly_l.f = __spirv_ocl_fma(cpoly_l.f, R2h, __phl);
    cpoly_l.f = __spirv_ocl_fma(cpoly.f, R2l, cpoly_l.f);
    cpoly.f = __ph;
  };
  {
    float __ph, __ahl, __ahh;
    __ph = __spirv_ocl_fma(cpoly.f, 1.0f,
                                                  __stan_ha___cc0.f);
    __ahh =
        __spirv_ocl_fma(__ph, 1.0f, -__stan_ha___cc0.f);
    __ahl = __spirv_ocl_fma(cpoly.f, 1.0f, -__ahh);
    cpoly_l.f = cpoly_l.f + __ahl;
    cpoly.f = __ph;
  };
  spoly1.f =
      __spirv_ocl_fma(spoly.f, R2h, __stan_ha___cs1.f);
  spoly.f = __spirv_ocl_fma(spoly1.f, R2h, 0.0f);
  spoly_l.f = __spirv_ocl_fma(spoly1.f, R2h, (-spoly.f));
  {
    float __ph, __ahl, __ahh;
    __ph = __spirv_ocl_fma(spoly.f, 1.0f,
                                                  __stan_ha___cs0.f);
    __ahh =
        __spirv_ocl_fma(__ph, 1.0f, -__stan_ha___cs0.f);
    __ahl = __spirv_ocl_fma(spoly.f, 1.0f, -__ahh);
    spoly_l.f = (spoly_l.f + __stan_ha___cs0l.f) + __ahl;
    spoly.f = __ph;
  };
  {
    float __ph, __phl;
    __ph = __spirv_ocl_fma(spoly.f, Rh.f, 0.0f);
    __phl = __spirv_ocl_fma(spoly.f, Rh.f, -__ph);
    spoly_l.f = __spirv_ocl_fma(spoly_l.f, Rh.f, __phl);
    spoly_l.f =
        __spirv_ocl_fma(spoly.f, Rl.f, spoly_l.f);
    spoly.f = __ph;
  };
  // adjust sign
  sgn_p = (sgn_xp << 1);
  spoly.w ^= sgn_p;
  spoly_l.w ^= sgn_p;
  sres.w = (sgn_xp & 0x40000000) ? cpoly.w : spoly.w;
  cres.w = (sgn_xp & 0x40000000) ? spoly.w : cpoly.w;
  sres_l.w = (sgn_xp & 0x40000000) ? cpoly_l.w : spoly_l.w;
  cres_l.w = (sgn_xp & 0x40000000) ? spoly_l.w : cpoly_l.w;
  // 1/(cres + cres_l)
  Rh.f = 1.0f / (cres.f);
  eps = __spirv_ocl_fma(cres.f, -Rh.f, 1.0f);
  eps = __spirv_ocl_fma(cres_l.f, -Rh.f, eps);
  Rl.f = __spirv_ocl_fma(Rh.f, eps, 0.0f);
  tres.f = __spirv_ocl_fma(
      sres.f, Rl.f,
      __spirv_ocl_fma(sres_l.f, Rh.f, 0.0f));
  tres.f = __spirv_ocl_fma(sres.f, Rh.f, tres.f);
  tres.w ^= sgn_x;
  *pres = tres.f;
  return nRet;
}
float __ocl_svml_tanf_ha(float x) {
  float r;
  __ocl_svml_internal_stan_ha(&x, &r);
  return r;
}

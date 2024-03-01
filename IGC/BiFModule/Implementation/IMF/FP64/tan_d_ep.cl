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
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dtan_ep_c4 = {0xbf30473bc9951ab8uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dtan_ep_c3 = {0xbf612b43c92fe3c3uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dtan_ep_c2 = {0xbf96c2a27abdad92uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dtan_ep_c1 = {0xbfd55553d0e3f982uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dtan_ep_c0 = {0x3feffffffd92bc1euL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dtan_ep_INV_PI64 = {0x3fe45f306dc9c883uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dtan_ep_SHIFTER = {0x4338000000000000uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dtan_ep_NPI1_BITS = {0xbff921fb54442d18uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dtan_ep_NPI2_BITS = {0xbc91a62633000000uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dtan_ep_NPI3_BITS = {0xba945c06e0e68948uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dtan_ep_two = {0x4000000000000000uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dtan_ep_half = {0x3fe0000000000000uL};
static __constant unsigned long __dtan_ep_AbsMask = 0x7fffffffffffffffuL;
static __constant unsigned long __dtan_ep_zero = 0x0000000000000000uL;
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dtan_ep_dc10 = {0xbdf64c1ef7b9f88fuL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dtan_ep_dc9 = {0xbe2209d9df00def0uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dtan_ep_dc8 = {0xbe57fdc92b36f96fuL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dtan_ep_dc7 = {0xbe8d6b5f16d11bceuL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dtan_ep_dc6 = {0xbec228124e270b4cuL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dtan_ep_dc5 = {0xbef66a8ed675ceb9uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dtan_ep_dc4 = {0xbf2bbd7794a1b07duL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dtan_ep_dc3 = {0xbf61566abbff502auL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dtan_ep_dc2 = {0xbf96c16c16c16e58uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dtan_ep_dc1 = {0xbfd5555555555555uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dtan_ep_dc0 = {0x3ff0000000000000uL};
//////////////
// 2^1152/(2*Pi)
static __constant unsigned int __dtan_ep_InvPi_tbl[] = {
    0,           0,           0x28BE60DBu, 0x9391054Au,
    0x7F09D5F4u, 0x7D4D3770u, 0x36D8A566u, 0x4F10E410u,
    0x7F9458EAu, 0xF7AEF158u, 0x6DC91B8Eu, 0x909374B8u,
    0x01924BBAu, 0x82746487u, 0x3F877AC7u, 0x2C4A69CFu,
    0xBA208D7Du, 0x4BAED121u, 0x3A671C09u, 0xAD17DF90u,
    0x4E64758Eu, 0x60D4CE7Du, 0x272117E2u, 0xEF7E4A0Eu,
    0xC7FE25FFu, 0xF7816603u, 0xFBCBC462u, 0xD6829B47u,
    0xDB4D9FB3u, 0xC9F2C26Du, 0xD3D18FD9u, 0xA797FA8Bu,
    0x5D49EEB1u, 0xFAF97C5Eu, 0xCF41CE7Du, 0xE294A4BAu,
    0x9AFED7ECu, 0x47E35742u, 0x1580CC11u, 0xBF1EDAEAu,
    0,           0,           0,           0};
// unsigned 32-bit shift
// signed 32-bit shift
// unsigned 64-bit shift
// signed 64-bit shift
// reduce argument to (-2*pi/(2^kbits), 2*pi/(2^kbits)) range
static __attribute__((always_inline)) inline double
__dtan_ep_trig_reduction(double x, int kbits, double *py_low, int *pinterv) {
  unsigned long ix, sgn_x, abs_x, mx, R;
  unsigned long S[2], P2, P3, P4, P5, L, L2, Lh, P23, P23_l, Msk;
  long e_yl, e_yh;
  unsigned int mant[2], Sh, Sl;
  volatile unsigned int T[7];
  int Syl;
  int expon_x, j, shift, shift2, scale, interv = 0;
  long int cond;
  double yl, res;
  union {
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
  } ixd;
  // (2*pi)*2^61
  unsigned int two_pi[2] = {0x2168C235u, 0xC90FDAA2u};
  ixd.f = x;
  ix = ixd.w;
  abs_x = ix & 0x7fffffffffffffffuL;
  sgn_x = ix ^ abs_x;
  // biased exponent
  expon_x = (((unsigned int)((unsigned int)(abs_x >> 32))) >> (20));
  // mantissa
  mant[0] = (unsigned int)abs_x;
  mant[1] = (((unsigned int)(abs_x >> 32)) & 0xfffffu) | 0x100000u;
  // eliminate smaller |x|, as well as Inf/NaN
  cond = ((unsigned)(expon_x - 0x400) >= (0x7ff - 0x400));
  if (__builtin_expect(cond, 0)) {
    *py_low = 0;
    *pinterv = 0;
    return x;
  }
  // starting table index for argument reduction
  // j >= 1 for expon_x >= 20+0x3ff
  expon_x = expon_x + 12 - 0x3ff;
  j = (((unsigned int)(expon_x)) >> (5));
  // look up table values
  T[0] = __dtan_ep_InvPi_tbl[j];
  T[1] = __dtan_ep_InvPi_tbl[j + 1];
  T[2] = __dtan_ep_InvPi_tbl[j + 2];
  T[3] = __dtan_ep_InvPi_tbl[j + 3];
  T[4] = __dtan_ep_InvPi_tbl[j + 4];
  T[5] = __dtan_ep_InvPi_tbl[j + 5];
  T[6] = __dtan_ep_InvPi_tbl[j + 6];
  // shift in [0, 31]
  shift = expon_x - (j << 5);
  // shift left
  if (shift) {
    shift2 = 32 - shift;
    T[0] = (T[0] << shift) | (((unsigned int)(T[1])) >> (shift2));
    T[1] = (T[1] << shift) | (((unsigned int)(T[2])) >> (shift2));
    T[2] = (T[2] << shift) | (((unsigned int)(T[3])) >> (shift2));
    T[3] = (T[3] << shift) | (((unsigned int)(T[4])) >> (shift2));
    T[4] = (T[4] << shift) | (((unsigned int)(T[5])) >> (shift2));
    T[5] = (T[5] << shift) | (((unsigned int)(T[6])) >> (shift2));
  }
  // InvPi*mant_x
  S[0] = (((unsigned long)((unsigned int)(T[3]))) * ((unsigned int)(mant[0])));
  P4 = (((unsigned long)((unsigned int)(T[4]))) * ((unsigned int)(mant[1])));
  L = (((unsigned long)((unsigned int)(T[4]))) * ((unsigned int)(mant[0])));
  L2 = (((unsigned long)((unsigned int)(T[5]))) * ((unsigned int)(mant[1])));
  L += L2;
  if (L < L2)
    S[0] += 0x100000000uL;
  P2 = (((unsigned long)((unsigned int)(T[2]))) * ((unsigned int)(mant[0])));
  S[1] =
      (((unsigned long)((unsigned int)(T[1]))) * ((unsigned int)(mant[0]))) +
      (((unsigned long)(((unsigned int)T[0]) * ((unsigned int)mant[0]))) << 32);
  S[0] += P4;
  if (S[0] < P4)
    S[1]++;
  Lh = (((unsigned long)(L)) >> (32));
  L <<= 32;
  S[0] += Lh;
  if (S[0] < Lh)
    S[1]++;
  P3 = (((unsigned long)((unsigned int)(T[3]))) * ((unsigned int)(mant[1])));
  S[1] =
      S[1] +
      (((unsigned long)((unsigned int)(T[2]))) * ((unsigned int)(mant[1]))) +
      (((unsigned long)(((unsigned int)T[1]) * ((unsigned int)mant[1]))) << 32);
  // accumulate terms
  P23 = P2 + P3;
  // add carry
  if (P23 < P3)
    S[1] += 0x100000000uL;
  S[1] += (((unsigned long)(P23)) >> (32));
  P23_l = P23 << 32;
  S[0] += P23_l;
  if (S[0] < P23_l)
    S[1]++;
  if (kbits) {
    shift2 = 32 - kbits;
    interv = (((unsigned long)((((unsigned long)(S[1])) >> (32)))) >> (shift2));
    S[1] = (S[1] << kbits) |
           (((unsigned long)((((unsigned long)(S[0])) >> (32)))) >> (shift2));
    S[0] = (S[0] << kbits) |
           (((unsigned long)((((unsigned long)(L)) >> (32)))) >> (shift2));
    L <<= kbits;
  }
  // round intev to nearest
  Msk = (((long)(S[1])) >> (63));
  interv = interv - (int)Msk;
  S[1] ^= Msk;
  S[0] ^= Msk;
  L ^= Msk;
  // apply sign to interv, then correction to sign of fraction
  sgn_x = (((long)(sgn_x)) >> (63));
  *pinterv = (interv ^ (int)sgn_x) - (int)sgn_x;
  sgn_x = (sgn_x ^ Msk) & 0x8000000000000000uL;
  scale = -64 - kbits;
  // normalization: leading bit of S[1] should be 1
  while (((long)S[1]) > 0) {
    scale--;
    S[1] = (S[1] << 1) | (((unsigned long)(S[0])) >> (63));
    S[0] = (S[0] << 1) | (((unsigned long)(L)) >> (63));
    L <<= 1;
  }
  // multiply by 2*Pi*(2^61)
  Sh = (unsigned int)(((unsigned long)(S[1])) >> (32));
  Sl = (unsigned int)S[1];
  R = (((unsigned long)((unsigned int)(Sh))) * ((unsigned int)(two_pi[1])));
  P2 = (((unsigned long)((unsigned int)(Sh))) * ((unsigned int)(two_pi[0])));
  P3 = (((unsigned long)((unsigned int)(Sl))) * ((unsigned int)(two_pi[1])));
  // accumulate terms
  P23 = P2 + P3;
  // add carry
  if (P23 < P3)
    R++;
  // R is result*2^(scale+3)
  R += (((unsigned long)(P23)) >> (32));
  scale += 3;
  // normalize
  if (((long)R) < 0) {
    R = (((unsigned long)(R)) >> (1));
    scale++;
  }
  // round upper 53 bits
  Syl = (unsigned int)R;
  R += (unsigned long)(1 << 9);
  // determine y_low
  Syl <<= (32 - 10);
  Syl = (((int)(Syl)) >> (32 - 10));
  // SINT32 to double conversion
  yl = (double)Syl;
  // adjust exponent of yl
  e_yl = (long)scale;
  e_yl = ((e_yl + 0x3ff) << 52) | sgn_x;
  // y_low
  *py_low = yl * (*(double *)&e_yl);
  // exponent of high part
  e_yh = (unsigned long)(scale + 62 - 1 + 0x3ff);
  e_yh = (e_yh << 52);
  // high part of result
  R = e_yh + (((unsigned long)(R)) >> (10));
  *(unsigned long *)&res = sgn_x ^ R;
  return res;
}
__attribute__((always_inline)) inline int
__ocl_svml_internal_dtan_ep(double *pxin, double *pres) {
  int nRet = 0;
  double xin = *pxin;
  // double tan(double xin)
  unsigned leading_xh;
  int index;
  double dN, R, Rl, dNP2_h, dR2, dNP3_h;
  union {
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
  } x, x0, dS, poly, res, dA, dB, dR;
  double dRcp, eps;
  unsigned long sgn_x, R_sgn, lindex;
  float fB, fRcp;
  x0.f = xin;
  x.w = x0.w & (__dtan_ep_AbsMask);
  sgn_x = (x0).w ^ (x).w;
  // redirect special cases
  leading_xh = ((unsigned)x.w32[1]);
  if (((unsigned)(leading_xh) >= (0x41300000 - 0)))
    goto TAN_SPECIAL;
  // _VSTATIC(SHIFTER) + x*(1/pi)
  dS.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(x.f, (__dtan_ep_INV_PI64).f,
                                                (__dtan_ep_SHIFTER).f);
  // N ~ x*(1/pi)
  dN = dS.f - (__dtan_ep_SHIFTER).f;
  R_sgn = (dS).w << 63;
  // R = x - N*PI1
  R = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dN, (__dtan_ep_NPI1_BITS).f, x.f);
  // Rm = Rh - N*PI2
  dR.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dN, (__dtan_ep_NPI2_BITS).f, R);
  // R^2
  dR2 = dR.f * dR.f;
TAN_MAIN_PATH:
  poly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dR2, (__dtan_ep_c4).f,
                                                  (__dtan_ep_c3).f);
  poly.f =
      SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dR2, poly.f, (__dtan_ep_c2).f);
  poly.f =
      SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dR2, poly.f, (__dtan_ep_c1).f);
  poly.f =
      SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dR2, poly.f, (__dtan_ep_c0).f);
  dA.w = R_sgn ? poly.w : dR.w;
  dB.w = R_sgn ? dR.w : poly.w;
  fB = (float)dB.f;
  fRcp = 1.0f / fB;
  dRcp = (double)fRcp;
  // refine dRcp ~ 1/fB
  eps = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dRcp, -dB.f, 1.0);
  dRcp = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dRcp, eps, dRcp);
  eps = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dRcp, -dB.f, 1.0);
  dRcp = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dRcp, eps, dRcp);
  res.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dA.f, dRcp, 0.0);
  res.w ^= (R_sgn ^ sgn_x);
  *pres = res.f;
  return nRet;
TAN_SPECIAL:
  // Inf/NaN?
  if (leading_xh >= 0x7ff00000) {
    // NaN?
    if ((x0.w & (__dtan_ep_AbsMask)) > 0x7ff0000000000000uL) {
      *pres = x.f * x.f;
      return nRet;
    }
    res.w = 0xfff8000000000000uL;
    *pres = res.f;
    nRet = 1;
    return nRet;
  }
  R = __dtan_ep_trig_reduction(x.f, 2, &Rl, &index);
  dR2 = R * R;
  lindex = (unsigned long)index;
  lindex <<= 63;
  R_sgn = (lindex);
  poly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dR2, (__dtan_ep_dc10).f,
                                                  (__dtan_ep_dc9).f);
  poly.f =
      SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dR2, poly.f, (__dtan_ep_dc8).f);
  poly.f =
      SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dR2, poly.f, (__dtan_ep_dc7).f);
  poly.f =
      SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dR2, poly.f, (__dtan_ep_dc6).f);
  poly.f =
      SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dR2, poly.f, (__dtan_ep_dc5).f);
  poly.f =
      SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dR2, poly.f, (__dtan_ep_dc4).f);
  poly.f =
      SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dR2, poly.f, (__dtan_ep_dc3).f);
  poly.f =
      SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dR2, poly.f, (__dtan_ep_dc2).f);
  poly.f =
      SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dR2, poly.f, (__dtan_ep_dc1).f);
  poly.f =
      SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dR2, poly.f, (__dtan_ep_dc0).f);
  dA.f = R_sgn ? poly.f : R;
  dB.f = R_sgn ? R : poly.f;
  res.f = dA.f / dB.f;
  res.w ^= (R_sgn ^ sgn_x);
  *pres = res.f;
  return nRet;
}
double __ocl_svml_tan_ep(double x) {
  double r;
  __ocl_svml_internal_dtan_ep(&x, &r);
  return r;
}

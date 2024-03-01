/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
/*
// ALGORITHM DESCRIPTION:
//
//   CONVENTIONS
//   A = B  denotes that A is equal to B.
//   A := B denotes assignment of a value of the expression B to
//          the variable A. All operations and roundings in B are considered
//          to be in the target precision.
//   <A>    denotes the rounding the IEEE-754 floating-point value A to the
//          target precision.
//   {A}    denotes the rounding the IEEE-754 floating-point value A to the
//          nearest integer.
//
//   HIGH LEVEL OVERVIEW
//   *a = Sign * x, where x = |*a|, Sign = sign(*a).
//   Hence, asin(*a) = Sign * asin(x)
//
//   [0,1/2) interval.
//      On [0,1/2) interval arcsine is approximated by a polynomial:
//
//         asin(x) ~= x + a0*x^3 + a1*x^5 + x^7*P0(x^2).
//
//   [1/2,1] interval.
//      On [1/2,1) interval asin(x) can be rewritten as follows:
//
//         asin(x) = Pi/2 - 2*asin( sqrt(y/2) ),
//
//      where y = 1-x. In turn, 2*asin(sqrt(y/2)) is approximated as follows:
//
//         2*asin(sqrt(y/2)) ~= sqrt(y/2) * (b0 + b1*y + b2*y^2 + y^3*P1(y)),
//
//      where b0 + b1*y + b2*y^2 + y^3*P1(y) - polynomial approximation of
//      2*asin(sqrt(y/2)) / sqrt(y/2) on [0,1/2] interval.
//
//   IEEE SPECIAL CONDITIONS
//   |*a| > 1,      *r = QNaN
//    *a  = +/-Inf, *r = QNaN
//    *a  = QNaN,   *r = QNaN
//    *a  = SNaN,   *r = QNaN
//
//   ALGORITHM DETAILS
//   A careful algorithm must be used to realize mathematical ideas accurately.
//   In addition a number of execution paths required to handle special and
//   subtle cases. Below we describe each execution path assuming *a = Sign*x.
//
//   1) *a = [Q,S]NaN
//      *r := *a / *a
//
//   2) *a = [+,-]Inf
//      *r := *a / *a
//      Error handler routine is called with IML_STATUS_ERRDOM status code.
//
//   3) |*a| > 1
//      *r := (*a-*a)/(*a-*a)
//      Error handler routine is called with IML_STATUS_ERRDOM status code.
//
//   4) 0 <= |*a| < 1/2
//      4.1) "Near 0" path, arguments are normalized numbers
//           NEAR0_BOUND_L <= x < NEAR0_BOUND_U, where NEAR0_BOUND_L corresponds
//           to minimum normalized number in target precision
//           (~= 2.2*10^(-308)), NEAR0_BOUND_U = 2^(-53) in this implementation.
//
//           On this path asin(x) is approximated as x*(1+x). Summation 1+x
//           will result in raising inexact flag. In addition such a
//           representation is used to avoid raising underflow.
//
//             *r := Sign*(x * (1+x))
//
//      4.2) Denormalized arguments and 0
//           0 <= x < NEAR0_BOUND_L, where NEAR0_BOUND_L corresponds
//           to minimum normalized number in target precision (~= 2.2*10^(-308))
//
//           In this case asin(x) is approximated as x+x*x. Multiplication
//           x*x will result in raising inexact and underflow flags (if x<>0).
//
//             *r := Sign*(x + x*x)
//
//      4.3) Main path
//           NEAR0_BOUND_U <= x < 1/2, where NEAR0_BOUND_U = 2^(-53) in this
//           implementation.
//
//           Arcsine function is approximated by a polynomial of the form
//
//             asin(x) ~= x + (A0+A0L)*x^3 + (A1+A1L)*x^5 + x^7*P0(x^2),
//
//           where A0 and A0L, A1 and A1L represent a0, a1 with accuracy wider
//           than target precision. To ensure acceptable accuracy we perform
//           all multiplications and additions for terms of order up to x^5 in
//           multiprecision while for rest terms it is sufficient to perform
//           calculations in target precision.
//
//           To perform multiplication c=a*b accurately enough we split a and b
//           into high and low parts ah, al (a=ah+al) and bh, bl (b=bh+bl).
//           We split a and b so that ah*bh is exactly representable in target
//           precision. IML_SPLIT() macro is used for that purpose. Result of
//           multiplication c=a*b is represented by two numbers ch and cl, which
//           are calculated as follows:
//
//             ch := ah*bh
//             cl := al*b + ah*bl
//
//           To perform addition d = e+f (|e|>|f|) accurately IML_ADD_HL() macro
//           is used. Result of the summation is two numbers dh and dl
//           accurately representing d in target precision.
//
//           NOTE: See IML_SPLIT() and IML_ADD_HL() macro descriptions for
//                 further details.
//
//           Series of splittings, multiplications and accurate summations are
//           used for the terms x, (A0+A0L)*x^3 and (A1+A1L)*x^5. Final result
//           is represented as two parts: dbResHi and dbResLo. dbResHi is
//           calculated so that all its bits are significant. dbResLo is a sum
//           of low parts (tails) accurately taken into account performing
//           intermediate calculations.
//
//           Let us illustrate above ideas in more details.
//
//           (1) Split x (dbAbsX variable in the program code) into dbXHi
//               and dbXLo using IML_SPLIT() macro.
//           (2) Calulate x^2 as sum of dbXXHi and dbXXLo:
//
//                 dbXXHi := dbXHi*dbXHi
//                 dbXXLo := (x + dbXHi)*dbXLo
//
//           (3) Calculate P0(x2), where x2 is double precision approximation
//               of x^2:
//
//                 x2 := dbXXHi+dbXXLo
//
//           (4) Split dbXXHi into dbYHi and dbYLo using IML_SPLIT() macro.
//           (5) Calculate x^3 as sum of dbZHi and dbZLo:
//
//                 dbZHi := dbYHi*dbXHi
//                 dbZLo := (dbYLo+dbXXLo)*x + dbYHi*dbXLo
//
//           (6) Split dbZHi into dbWHi and dbWLo using IML_SPLIT() macro.
//           (7) Calculate x^5 as sum of dbVHi and dbVLo:
//
//                 dbVHi := dbWHi*dbYHi
//                 dbVLo := (dbWLo+dbZLo)*dbYHi + (dbYLo+dbXXLo)*(dbZHi+dbZLo)
//
//           (8) Split dbVHi into dbUHi and dbuLo using IML_SPLIT() macro.
//           (9) Calculate P0(x2)*x^7:
//
//                 dbP0 := P0(x2)*(dbVHi+dbVLo)*x2
//
//           (10) Calculate A0L*x^3 + A1L*x^5 + dbP0 in dbResLo
//
//                 dbResLo := A0L*(dbZHi+dbZLo) + A1L*(dbVHi+dbVLo) + dbP0
//
//           (11) Add small term dbZLo*A0 - part of multiprecision calculation
of
//               (A0+A0L)*x^3:
//
//                 dbResLo := dbResLo + dbZLo*A0
//
//           (12) Calculate high part of multiprecision multiplication
//               (A0+A0L)*x^3 in dbT1, and add another small term to dbResLo:
//
//                 dbT1 := dbWHi*A0
//                 dbResLo := dbResLo + dbWLo*A0
//
//           (13) Calculate x+dbT1 exactly in target precision as sum of high
and
//               low parts, dbR1 and dbS1 respectively using IML_ADD_HL() macro.
//               Low part dbS1 is added to dbResLo.
//           (14) Add small term dbVLo*A1 to dbResLo - part of multiprecision
//               calculation of (A1+A1L)*x^5:
//
//                 dbResLo := dbResLo + dbVLo*A1
//
//           (15) Calculate high part of multiprecision multiplication
//               (A1+A1L)*x^5 in dbT2, and add another small term to dbResLo
//
//                 dbT2 := dbUHi*A1
//                 dbResLo = dbResLo + dbuLo*A1
//
//           (16) Calculate dbR1+dbT2 exactly in target precision as sum of high
//               and low parts, dbR2 and dbS2 respectively, using IML_ADD_HL()
//               macro. Low part is added to dbResLo. High part is in dbResHi.
//
//                 dbResHi := dbR2
//                 dbResLo := dbResLo + dbS2
//
//           (17) Calculate final result as sum of dbResHi and dbResLo
//
//                 dbRes := dbResHi + dbResLo
//                 *r := Sign*dbRes
//
//   5) 1/2 <= |*a| <= 1
//      On this path asin(x) can be rewritten as follows:
//
//         asin(x) = Pi/2 - 2*asin( sqrt(y/2) ),
//
//      where y = 1-x. In turn, 2*asin(sqrt(y/2)) is approximated as follows:
//
//         2*asin(sqrt(y/2)) ~= sqrt(y/2) * (b0 + b1*y + b2*y^2 + y^3*P1(y)),
//
//      where b0 + b1*y + b2*y^2 + y^3*P1(y) - polynomial approximation of
//      2*asin(sqrt(y/2)) / sqrt(y/2) on [0,1/2] interval.
//
//      Square root sqrt(y/2) is calculated with accuracy wider than target
//      precision as a pair: dbSqrtHi and dbSqrtLo.
//
//      Polynomial terms b0, b1*y, b2*y^2 are calculated in multiprecision.
//      For this purpose a number of splittings, multiprecision multiplications,
//      and accurate summations are used. Coefficients b0, b1 and b2 are
//      represented by pairs of high and low parts, B0 and B0L, _B1 and B1L,
//      _B2 and B2L respectively.
//
//      Final substraction is also performed in multiprecision. For this purpose
//      constant Pi/2 is presented as high and low part, PI2_HI and PI2_LO
//      respectively:
//
//        PI2_HI = <Pi/2>
//        PI2_LO = <Pi/2 - PI2_HI>
//
//      Below we describe above ideas in more details.
//
//      Calculating s=sqrt(y2) in multiprecision, where y2=y/2
//      ------------------------------------------------------
//           Let y2=M*2^(2*iN), where iN is an integer, 1<=M<4.
//
//             sqrt(y2) = 2^iN * sqrt(M),
//
//           where sqrt(M) is computed using equality
//
//             sqrt(M) = M * 1/sqrt(M).
//
//           Let s0 be approximation to 1/sqrt(M) stored in a table.
//           Let q=M*s0^2-1, then
//
//             1/sqrt(M) = s0 * 1/sqrt(1+q).
//
//           Notice that q is small value (the better s0 approximates
//           1/sqrt(M), the smaller is |q|). Therefore we can approximate
//           1/sqrt(1+q) by a small-degree polynomial of q of the form:
//
//             1/sqrt(1+q) ~= 1 + q * P2(q).
//
//           At last we have
//
//             sqrt(y2) ~= 2^iN * M * (s0 + s0*q*P2(q)).
//
//           Calculation of sqrt(y2) is performed in 3 steps:
//
//           a) Range reduction
//               Decompose y2 into y2=M1*2^iN1, where iN1 is an integer,
1<=M1<2.
//               We obtain iN1 and M1 in the following way:
//
//                   iN1  := DP_EXPONENT(t) - DP_BIAS
//                   M1 := y2 / 2^iN1
//
//               We need to decompose y2 into y2=M2*2^(2*iN), where iN is an
//               integer, 1<=M2<4. Supposing iN1=2*iN+iL, where iL=0 or 1, we
//               can obtain iN and M2 in the following way:
//
//                   iL := iN1 & 1
//                   iN := (iN1 - iL) >> 1
//                   M2 := M1 * 2^iL
//
//               And then we split M2 into sum MHi+MLo so that
//               s0*s0*MHi is exactly representable in target precision:
//
//                   Tmp1 := M2   * RSM
//                   Tmp2 := Tmp1 - M2
//                   MHi  := Tmp1 - Tmp2
//                   Tmp1 := M2   - MHi
//                   MLo  := Tmp1
//
//               where RSM = 2^22+1 in this implementation.
//
//               The index j of the reciprocal square root value s0, which is
//               close to 1/sqrt(M), is obtained as follows:
//
//                   j1 := { (M1-1)*2^SQRT_K },
//                   j  := j1 + iL * 2^SQRT_K
//
//               where number of table entries is 2*2^SQRT_K+1 and
//               0<=j<=2*2^SQRT_K (iL=0 for 1<=M<2, iL=1 for 2<=M<4).
//               In this implementation SQRT_K=8.
//
//               Implementation note: instead of explicit conversion
//               double->integer we use "right shifter" technique:
//
//                   TmpJ1 := <M1Hi + RSJ>,
//
//               where RSJ = 2^44+1 in this implementation.
//
//               The j1 is extracted from the SQRT_K+1 least significant bits
//               of mantissa of TmpJ1.
//
//                   s0 := RSQRT(j)
//
//               We obtain Q~=q in two steps:
//
//                   Q1 := (s0*s0)*MHi - 1
//                   Q  := Q1 + (s0*s0)*MLo
//
//               where Q1 is computed exactly because s0 and MHi are so that
//               s0*s0*MHi is exactly represented in target precision
//               and 1/2 < s0*s0*MHi < 2.
//
//           b) Approximation
//
//               P2(q) approximates (1/sqrt(1+q)-1)/q. In this implementation
//               it is 6-th degree polynomial.
//
//           c) Reconstruction
//               We represent s=sqrt(y2) as sum of two variables
//               dbSqrtHi+dbSqrtLo, where
//
//                   dbSqrtHi := 2^iN * (MHi * s0)
//                   dbSqrtLo := 2^iN * (MLo * s0 + (MHi+MLo)*s0*Q*SqrtPoly)
//
//      b0 + b1*y + b2*y^2 + y^3*P1(y) valuation
//      ----------------------------------------
//           (1) Calculate P1(y)*y*y*y in dbP1 variable.
//           (2) Rebreak dbSqrtHi and dbSqrtLo so that dbSqrtHi has large enough
//               least significant zero bits in mantissa to perform exact
//               multiplication.
//           (3) Calculate dbSqrtLo*dbP1 in dbT2 variable.
//           (4) Split y into dbYHi and dbYLo using IML_SPLIT() macro.
//           (5) Calculate y^2 as sum of dbZHi and dbZLo:
//
//                 dbZHi := dbYHi*dbYHi
//                 dbZLo := dbYLo*(dbYHi + y)
//
//           (6) Split dbZHi into dbWHi and dbWLo using IML_SPLIT() macro.
//           (7) Calculate high parts of (_B1+B1L)*y and (_B2+B2L)*y^2 and
perform
//               accurate summation of these parts using IML_ADD_HL() macro.
//               Result of summation is in dbS1 (high part) and dbS2 (low part).
//               Notice that |_B2*dbWHi| < |_B1*dbYHi|
//
//                 dbTmp := _B1*dbYHi
//                 dbTmp1:= _B2*dbWHi
//
//           (8) Calculate B0+dbS1 accurately using IML_ADD_HL() macro. Result
//               is in dbR1 (high part) and dbR2 (low part).
//               Notice that |dbS1| < |B0|.
//           (9) Split dbR1 into dbVHi and dbVLo using IML_SPLIT() macro.
//           (10) Add low parts we got during accurate summation as well as
//               during splitting dbR1:
//
//                 dbT2 := dbT2 + dbSqrtHi * ( dbVLo + dbS2 + dbR2 + _B1*dbYLo
//                       + _B2*(dbWLo+dbZLo) )
//                 dbT2 := dbT2 + dbSqrtHi * ( B0L + (B1L + B2L*y)*y + dbP1 )
//
//           (11) Calculate dbSqrtHi * dbVHi in dbT1 variable. Notice that
//               multiplication is exact.
//           (12) Calculate PI2_HI-dbT1 accurately using IML_ADD_HL() macro.
//               Rusult is stored into dbResHi (high part) and dbS0 (low part)
//               variables. Notice that |PI2_HI| > |dbT1|.
//           (13) Calculate PI2_HI-dbT2 into dbResLo variable.
//           (14) Add low part dbS0 (we got during accurate summation) to
dbResLo
//           (15) Final result:
//
//                 *r := Sign*(dbResHi+dbResLo)
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
} __dasin_ep_c5 = {0x3fa5db80ed4c0893uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dasin_ep_c4 = {0x3f98654d51d83552uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dasin_ep_c3 = {0x3fa7535682d76f42uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dasin_ep_c2 = {0x3fb32f82392c8a5fuL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dasin_ep_c1 = {0x3fc5555fdd993e8buL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dasin_ep_c0 = {0x3feffffffd9283aduL};
// 2.0
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dasin_ep_two = {0x4000000000000000uL};
// pi/2
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dasin_ep_pi2h = {0x3ff921fb54442d18uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __dasin_ep_pi2l = {0x3c91a62633145c07uL};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __dasin_ep_small_float = {0x01800000u};
__attribute__((always_inline)) inline int
__ocl_svml_internal_dasin_ep(double *pxin, double *pres) {
  int nRet = 0;
  double xin = *pxin;
  union {
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
  } y, res;
  // double asin(double xin)
  {
    union {
      unsigned long w;
      unsigned int w32[2];
      int s32[2];
      double f;
    } x, xa, RS, RS2, Shh2, High, Low, R0;
    double R, E, poly, Sh, Shh;
    unsigned long sgn_x;
    float yf;
    x.f = xin;
    // absolute values
    // xa.w32[1] = x.w32[1] & 0x7fffffff;  xa.w32[0] = x.w32[0];
    xa.f = SPIRV_OCL_BUILTIN(fabs, _f64, )(x.f);
    // input sign
    sgn_x = x.w ^ xa.w;
    // (1-|x|)/2
    y.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )((-0.5), xa.f, 0.5);
    // prepare polynomial argument
    R = xin * xin;
    R = SPIRV_OCL_BUILTIN(fmin, _f64_f64, )(R, y.f);
    // rsqrt((1-|x|)/2), ~23 bits
    yf = (float)y.f;
    yf += __dasin_ep_small_float.f;
    yf = 1.0f / SPIRV_OCL_BUILTIN(sqrt, _f32, )(yf);
    RS.f = (double)(yf);
    // Sh ~ sqrt((1-|x|)/2)
    Sh = y.f * RS.f;
    // -2* Sh
    Shh2.f = -2.0 * Sh;
    // E = 2*(0.5 - 0.5*RS.f*RS.f*y.f)
    E = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )((-RS.f), Sh, 1.0);
    // E*(c1+c2*E)
    R0.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(0.375, E, 0.5);
    R0.f *= E;
    // Shh2 + Shh2*E*(c1+c2*E)
    R0.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(R0.f, Shh2.f, Shh2.f);
    // polynomial
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(__dasin_ep_c5.f, R,
                                                  __dasin_ep_c4.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(poly, R, __dasin_ep_c3.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(poly, R, __dasin_ep_c2.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(poly, R, __dasin_ep_c1.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(poly, R, __dasin_ep_c0.f);
    // R0 = sel_mask? Shh2 : xa.f
    R0.f = (xa.f <= 0.5) ? x.f : R0.f; //((x.w - R0.w) & sel_mask) + R0.w;
    // High = sel_mask ? High : xa.w
    High.f = (xa.f <= 0.5) ? 0.0 : __dasin_ep_pi2h.f;
    // poly*R0 + High
    res.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(poly, R0.f, High.f);
    res.w |= sgn_x;
  }
  *pres = res.f;
  nRet = (y.f >= 0) ? 0 : 1;
  return nRet;
}
double __ocl_svml_asin_ep(double x) {
  double r;
  __ocl_svml_internal_dasin_ep(&x, &r);
  return r;
}

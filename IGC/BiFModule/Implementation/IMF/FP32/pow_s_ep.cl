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
//   ex(A)  denotes unbiased binary exponent of a number A so that
//          A = significand(A) * 2^ex(A).
//
//   HIGH LEVEL OVERVIEW
//
//   Denote x = a[i], y = b[i].
//
//   "Main" path.
//       When input arguments x, y are nonzero finite numbers, |x|<>1, and
//       x>0 or x<0 and y is integer, then we use the formula:
//
//           |x^y| = 2^( y * log2|x| ), where
//
//       x^y>0 if x>0 or x<0 and y is an even integer,
//       x^y<0 if x<0 and y is an odd integer.
//
//   Other paths.
//       Cases for other combinations of input arguments are
//       described in IEEE SPECIAL CONDITIONS table below.
//
//   IEEE SPECIAL CONDITIONS:
//   The following table describes the results for pow(x,y) expected by C99
//   standard. In case of a cell is empty, the result must be computed using
//   mathematical formula and rounded to target precision: pow(x,y) = <x^y>.
//      \   x ||     |      |    |      |    |    |      |    |      |     |
//        \   ||-Inf |finite| -1 |-1<x<0| -0 | +0 |0<x<1 |  1 |finite|+Inf | NaN
//    y     \ ||     | x<-1 |    |      |    |    |      |    |  x>1 |     |
// =========++=====+======+====+======+====+====+======+====+======+=====+=====
//     +Inf   ||+Inf | +Inf |  1 |  +0  | +0 | +0 |  +0  |  1 | +Inf |+Inf | NaN
// ---------++-----+------+----+------+----+----+------+----+------+-----+-----
//      odd   ||     |      |    |      |    |    |      |    |      |     |
//    integer ||-Inf |      | -1 |      | -0 | +0 |      |  1 |      |+Inf | NaN
//      y>0   ||     |      |    |      |    |    |      |    |      |     |
// ---------++-----+------+----+------+----+----+------+----+------+-----+-----
//     even   ||     |      |    |      |    |    |      |    |      |     |
//    integer ||+Inf |      |  1 |      | +0 | +0 |      |  1 |      |+Inf | NaN
//      y>0   ||     |      |    |      |    |    |      |    |      |     |
// ---------++-----+------+----+------+----+----+------+----+------+-----+-----
//    finite  ||     | EDOM |EDOM| EDOM |    |    |      |    |      |     |
//    y>0 not ||+Inf |  NaN | NaN|  NaN | +0 | +0 |      |  1 |      |+Inf | NaN
//    integer ||     |  INV | INV|  INV |    |    |      |    |      |     |
// ---------++-----+------+----+------+----+----+------+----+------+-----+-----
//      +0    ||  1  |   1  |  1 |   1  |  1 |  1 |   1  |  1 |   1  |  1  |  1
// ---------++-----+------+----+------+----+----+------+----+------+-----+-----
//      -0    ||  1  |   1  |  1 |   1  |  1 |  1 |   1  |  1 |   1  |  1  |  1
// ---------++-----+------+----+------+----+----+------+----+------+-----+-----
//    finite  ||     | EDOM |EDOM| EDOM |EDOM|EDOM|      |    |      |     |
//    y<0 not || +0  |  NaN | NaN|  NaN |+Inf|+Inf|      |  1 |      | +0  | NaN
//    integer ||     |  INV | INV|  INV |DIVZ|DIVZ|      |    |      |     |
// ---------++-----+------+----+------+----+----+------+----+------+-----+-----
//     even   ||     |      |    |      |EDOM|EDOM|      |    |      |     |
//    integer || +0  |      |  1 |      |+Inf|+Inf|      |  1 |      | +0  | NaN
//      y<0   ||     |      |    |      |DIVZ|DIVZ|      |    |      |     |
// ---------++-----+------+----+------+----+----+------+----+------+-----+-----
//      odd   ||     |      |    |      |EDOM|EDOM|      |    |      |     |
//    integer || -0  |      | -1 |      |-Inf|+Inf|      |  1 |      | +0  | NaN
//      y<0   ||     |      |    |      |DIVZ|DIVZ|      |    |      |     |
// ---------++-----+------+----+------+----+----+------+----+------+-----+-----
//            ||     |      |    |      |EDOM|EDOM|      |    |      |     |
//     -Inf   || +0  |  +0  |  1 | +Inf |+Inf|+Inf| +Inf |  1 |  +0  | +0  | NaN
//            ||     |      |    |      |DIVZ|DIVZ|      |    |      |     |
// ---------++-----+------+----+------+----+----+------+----+------+-----+-----
//      NaN   || NaN |  NaN | NaN|  NaN | NaN| NaN|  NaN |  1 |  NaN | NaN | NaN
//
//   Here:
//       EDOM means Domain error,
//       INV  means Invalid floating point exception,
//       DIVZ means Divide-by-zero floating point exception.
//
//   Invalid floating point exception is raised when one of arguments is SNaN.
//
//   Possible deviations of the algorithm from C99 standard:
//       Inexact flag can be raised even in some cases when x^y is exact finite
//       normalized number. Inexact and Underflow flags are raised in all cases
//       when x^y is exact denormalized.
//
//
//   ALGORITHM DETAILS
//   A careful algorithm must be used to realize mathematical ideas accurately.
//   In addition a number of execution paths required to handle special and
//   subtle cases.
//
//   At first, we check whether y is integer and save the result of this check
//   into iYIsInt variable.
//       The result is NOTINT when y is not an integer (here NOTINT=0).
//       The result is ODD    when y is an odd  integer (here ODD=1).
//       The result is EVEN   when y is an even integer (here EVEN=2).
//
//       The idea of the algorithm for this check is as follows:
//
//       If y is Inf or NaN then iYIsInt:=NOTINT,
//       else if |y| >= 2^53 then iYIsInt:=EVEN,
//       else if y=0 then iYIsInt:=EVEN,
//       else if |y|<1 then iYIsInt:=NOTINT,
//       else if fractional part of y is not zero then iYIsInt:=NOTINT,
//       else if the lowest bit in integer part of y is zero then iYIsInt:=EVEN,
//       else iYIsInt:=ODD.
//
//   After that pow algorithm follows.
//
//   1)  x=1 or y=0
//       r[i] := 1
//
//   2)  One of arguments is NaN, but x<>1, y<>0
//       r[i] := x + y
//
//   3)  x=0, y<>NaN, y<>0
//
//       3.1) y<0, x=0, y<>NaN
//           If x=-0, y is negative odd integer
//           then r[i] := -Inf
//           else r[i] := +Inf
//           Raise DIVZ
//
//           Error handling routine is called with IML_STATUS_ERRDOM.
//
//       3.2) y>0, x=0, y<>NaN
//           If x=-0, y is positive odd integer
//           then r[i] := -0
//           else r[i] := +0
//
//   4)  x=-1, y is non-zero integer or Inf
//       If y is odd integer
//       then r[i] := -1
//       else r[i] := +1
//
//   5)  one of arguments is Inf, but none of them is zero or NaN, and |x|<>1
//
//       5.1) |x|<1, y=-Inf
//           r[i] := +Inf
//
//       5.2) |x|<1, y=+Inf
//           r[i] := +0
//
//       5.3) |x|>1, y<0
//           If x<0, y is negative odd integer
//           then r[i] := -0
//           else r[i] := +0
//
//       5.4) |x|>1, y>0
//           If x<0, y is positive odd integer
//           then r[i] := -Inf
//           else r[i] := +Inf
//           (Here we use equality x*x*y=+Inf.)
//
//   6)  -Inf<x<0, y is finite non-integer
//       r[i] := 0/0 (resulting with NaN, and raising INV)
//
//       Error handling routine is called with IML_STATUS_ERRDOM.
//
//   7)  "Main" path: x,y are finite nonzero numbers, |x|<>1,
//       and if x<0 then y is integer
//
//       7.a) Get sign of the result into SignRes
//           Sign of result here is negative only if x<0 and y is odd.
//
//       7.b) Start calculating log2|x|
//
//           Here we use the following formula.
//           Let |x|=2^k1*X1, where k1 is integer, 1<=X1<2.
//           Let C ~= 1/ln(2),
//           Rcp1 ~= 1/X1,   X2=Rcp1*X1,
//           Rcp2 ~= 1/X2,   X3=Rcp2*X2,
//           Rcp3 ~= 1/X3,   Rcp3C ~= C/X3.
//           Then
//
//               log2|x| = k1 + log2(1/Rcp1) + log2(1/Rcp2) + log2(C/Rcp3C) +
//                       + log2(X1*Rcp1*Rcp2*Rcp3C/C),
//
//           where X1*Rcp1*Rcp2*Rcp3C = C*(1+q), q is very small.
//
//           The values of Rcp1, log2(1/Rcp1), Rcp2, log2(1/Rcp2),
//           Rcp3C, log2(C/Rcp3C) are taken from tables.
//           Values of Rcp1, Rcp2, Rcp3C are such that RcpC=Rcp1*Rcp2*Rcp3C
//           is exactly represented in target precision.
//
//           log2(X1*Rcp1*Rcp2*Rcp3C/C) = log2(1+q) = ln(1+q)/ln2 =
//               = 1/(ln2)*q - 1/(2ln2)*q^2 + 1/(3ln2)*q^3 - ... =
//               = 1/(C*ln2)*cq - 1/(2*C^2*ln2)*cq^2 + 1/(3*C^3*ln2)*cq^3 - ...
=
//               = (1 + a1)*cq + a2*cq^2 + a3*cq^3 + ...,
//           where
//               cq=X1*Rcp1*Rcp2*Rcp3C-C,
//               a1=1/(C*ln(2))-1 is small,
//               a2=1/(2*C^2*ln2),
//               a3=1/(3*C^3*ln2),
//               ...
//
//           Calculation of log2|x| is performed as follows.
//
//           7.b.1) Getting X1
//               At first, represent |x| in the form |x| = 2^iDenoExpAdd * AX,
//               where AX is normalized.
//
//               Then get X1 by copying
//
//                   X1 := AX
//
//               and setting exponent field of X1 to biased 0.
//
//           7.b.2) Getting k
//               Get high 32 bits of AX into XHi.
//
//               Get k using XHi:
//
//                   k := XHi - K_SUB
//                   k := k >> IML_DP_NUM_HI_SIG_BITS
//                   k := k + DenoExpAdd
//
//               where K_SUB is high 32 bits of (1.5-2^(-rcpK1-1))/2,
//               rcpK1=5 in this implementation.
//
//               So we have:
//
//                   k=k1   if X1< 1.5-2^(-rcpK1-1),
//                   k=k1+1 if X1>=1.5-2^(-rcpK1-1).
//
//               Instead of k1, we will use k.
//
//           7.b.3) Get Rcp1, log2(1/Rcp1) from tables
//               Get index i1 from rcpK1 most significand bits of X1.
//               Get Rcp1.
//               Get log2(1/Rcp1) from a table as sum of two values L1Hi+L1Lo:
//                   L1Hi+L1Lo~=log2(1/Rcp1)   if X1< 1.5-2^(-rcpK1-1),
//                   L1Hi+L1Lo~=log2(1/Rcp1)-1 if X1>=1.5-2^(-rcpK1-1).
//
//           7.b.4) Get Rcp2, log2(1/Rcp2) from tables
//               Get X2.
//               Get index i2 from rcpK2 bits of significand of X2.
//               rcpK2=5 in this implementation.
//               Get Rcp2.
//               Get log2(1/Rcp2) from a table as sum of two values
//               L2Hi+L2Lo ~= log2(1/Rcp2).
//
//           7.b.5) Get Rcp3C, log2(C/Rcp3C) from tables
//               Get X3.
//               Get index i3 from rcpK3 bits of significand of X3.
//               rcpK3=7 in this implementation.
//               Get Rcp3C.
//               Get log2(C/Rcp3C) from a table as sum of two values
//               L3Hi+L3Lo ~= log2(C/Rcp3C).
//
//           7.b.6) Recombine k+log2(1/Rcp1)+log2(1/Rcp2)+log2(C/Rcp3C)
//               T := k + L1Hi + L2Hi + L3Hi
//               D :=     L1Lo + L2Lo + L3Lo
//
//               Now we have
//
//                   log2|x| ~= T + D + log2(Rcp1*Rcp2*Rcp3C*X1/C).
//
//           7.b.7) Get approximation CQ to cq
//               R1 := <<<X1*Rcp1>*Rcp2>*Rcp3C>
//               CQ := R1 - C  (the subrtaction is computed exactly)
//
//           7.b.8) Get the correction term E for CQ
//               We have cq=X1*RcpC-C, CQ=R1-C, cq=CQ+e.
//               So the exact correction term e=X1*RcpC-R1.
//               Approximation E to e is computed in multiprecision:
//
//               RcpC := Rcp1 * Rcp2 * Rcp3C
//
//               Split X1 into sum X1Hi+X1Lo so that X1Hi^2 is exactly
//               representable in target precision.
//
//               Split RcpC into sum RcpCHi+RcpCLo so that RcpCHi^2 is exactly
//               representable in target precision.
//
//               Computing E:
//               E := X1Hi*RcpCHi-R1
//               E := E + X1Lo*RcpCHi
//               E := E + X1Hi*RcpCLo
//               E := E + X1Lo*RcpCLo
//
//               Now we have CQ+E that represent cq more exactly than CQ.
//
//       Now we have
//
//           log2|x| ~= T + D + CQ + E + a1*CQ + a2*CQ^2 + a3*CQ^3 + ...
//
//       7.c) Get high part and exponent of log2|x|
//           Rebreak T + CQ into sum of high and low parts T_CQHi + CQLo.
//           Get exponent of T_CQHi into ELogAX variable.
//
//       7.d) Estimate |y*log2|x||
//           Using EYB=ex(y) and ELogAX, we estimate whether |y*log2|x||
//           is such that 2^(y*log2|x|) rounds to Inf, or 1, or 0 in target
//           precision, or it should be computed more accurately.
//
//       7.1) Here if ex(y) + ex(log2|x|) >= 11.
//           Here we have 2^11 <= |y*log2|x|| < Inf.
//
//           Get sign of y*log|x|.
//
//           If y*log|x|>0 then Tmp1=BIG_VALUE else Tmp1=SMALL_VALUE, where
//           BIG_VALUE=2^1023, SMALL_VALUE=2^(-1022) in this implementation.
//
//           Tmp1 := Tmp1 * Tmp1
//
//           r[i] := Tmp1 * SignRes
//
//       7.2) Here if ex(y) + ex(log2|x|) <= -62.
//           Here we have 0 < |y*log2|x|| <= 4*2^(-62).
//
//           Tmp1 := ONE
//           Tmp1 := Tmp1 + SMALL_VALUE
//           r[i] := Tmp1 * SignRes
//
//
//       7.3) Here if -62 < ex(y) + ex(log2|x|) < 11.
//           Here we have 2^(-61) <= |y*log2|x|| < 4*2^10.
//
//           7.3.a) R := CQ + E.
//               R represents cq more exactly than CQ.
//
//           7.3.b) Polynomial.
//               Log2Poly := A1*R + A2*R^2 + A3*R^3 + A4*R^4,
//
//               where A1=<a1>, ..., A4=<a4>.
//
//           7.3.c) Get 3 parts of log2|x|.
//               We have log2|x| ~= T_CQHi + Log2Poly + D + CQLo + E.
//               Represent log2|x| in the form of sum HH+HL+HLL.
//
//               LogPart3 := CQLo + E + D
//               Rebreak T_CQHi + Log2Poly into HH + HL
//               Now we have HH + HL + LogPart3 ~= log2|x|.
//
//               Rebreak HH + LogPart3 into HH + HLL.
//               HLL := HLL + HL
//
//               Split HH into HH + HL so that HH^2 is exactly representable
//               in target precision.
//
//               Now we have HH+HL+HLL ~= log2|x|.
//
//           7.3.d) Calculation of y*(HH+HL+HLL).
//               Split y into YHi+YLo.
//               Get high PH and medium PL parts of y*log2|x|.
//               Get low PLL part of y*log2|x|.
//               Now we have PH+PL+PLL ~= y*log2|x|.
//
//           7.3.e) Calculation of 2^(PH+PL+PLL).
//
//               Mathematical idea of computing 2^(PH+PL+PLL) is the following.
//               Let's represent PH+PL+PLL in the form N + j/2^expK + Z,
//               where expK=7 in this implementation, N and j are integers,
//               0<=j<=2^expK-1, |Z|<2^(-expK-1). Hence
//
//                   2^(PH+PL+PLL) ~= 2^N * 2^(j/2^expK) * 2^Z,
//
//               where 2^(j/2^expK) is stored in a table, and
//
//                   2^Z ~= 1 + B1*Z + B2*Z^2 ... + B5*Z^5.
//
//               We compute 2^(PH+PL+PLL) as follows.
//
//               Break PH into PHH + PHL, where PHH = N + j/2^expK.
//               Z = PHL + PL + PLL
//               Exp2Poly = B1*Z + B2*Z^2 ... + B5*Z^5
//               Get 2^(j/2^expK) from table in the form THI+TLO.
//               Now we have 2^(PH+PL+PLL) ~= 2^N * (THI + TLO) * (1 +
Exp2Poly).
//
//               Get significand of 2^(PH+PL+PLL) in the form ResHi+ResLo:
//               ResHi := THI
//               ResLo := THI * Exp2Poly + TLO
//
//               Get exponent ERes of the result:
//               Res := ResHi + ResLo:
//               ERes := ex(Res) + N
//
//               Now we can check whether result is normalized, denormalized,
//               overflowed or underflowed.
//
//               7.3.e.1) Here if ERes >= 1024.
//                   The result is overflowed.
//
//                   Tmp1 := BIG_VALUE * BIG_VALUE
//                   r[i] := Tmp1 * SignRes
//
//               7.3.e.2) Here if ERes < -1074-10.
//                   The result is underflowed.
//
//                   Tmp1 := SMALL_VALUE * SMALL_VALUE
//                   r[i] := Tmp1 * SignRes
//
//               7.3.e.3) Here if -1074-10 <= ERes < -1022-10.
//                   The result is a small denormalized number.
//
//                   SignRes := SignRes * DENO_UNSCALE
//                   N       := N + DENO_SCALE_EXP
//
//                   where DENO_UNSCALE=2^(-200),
//                   DENO_SCALE_EXP=200 in this implementation.
//
//                   TwoPowN := 2^N
//                   r[i] := Res * TwoPowN * SignRes
//
//               7.3.e.4) Here if -1022-10 <= ERes < -1022.
//                   The result is a big denormalized number.
//
//                   Rebreak ResHi+ResLo
//
//                   SignRes := SignRes * DENO_UNSCALE
//                   N       := N + DENO_SCALE_EXP
//
//                   TwoPowN := 2^N
//
//                   ResHi := ResHi * TwoPowN * SignRes
//                   ResLo := ResLo * TwoPowN * SignRes
//
//                   Res  := ResHi + ResLo
//                   r[i] := Res + SMALL_VALUE * SMALL_VALUE
//
//               7.3.e.5) Here if -1022 <= ERes <= 1023.
//                   The result is normalized.
//
//                   Res  := 2^N * Res
//                   r[i] := Res * SignRes
//
//
*/
#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF
typedef struct {
  /* LN PART */
  unsigned int sPoly[7];
  unsigned int iHiDelta;
  unsigned int iLoRange;
  unsigned int iBrkValue;
  unsigned int iOffExpoMask;
  unsigned int sOne;
  unsigned int sLn2Hi;
  unsigned int sLn2Lo;
  /* EXP PART */
  unsigned int _sInvLn2;
  unsigned int _sShifter;
  unsigned int _sLn2hi;
  unsigned int _sPC0;
  unsigned int _sPC1;
  unsigned int _sPC2;
  unsigned int _sPC3;
  unsigned int _iBias;
  unsigned int _iAbsMask;
  unsigned int _iDomainRange;
  unsigned int _s2N_2;
  unsigned int _sHuge;
  unsigned int _sTiny;
  unsigned int NMINNORM;
  unsigned int NMAXVAL;
  unsigned int INF;
} __ocl_svml_internal_spow_ep_data_t;
static __ocl_svml_internal_spow_ep_data_t __ocl_svml_internal_spow_ep_data = {
    /* LN PART */
    {
        /*== sPoly[] = SP polynomial ==*/
        0xbf000000u, /* -5.0000000000000000000000000e-01 */
        0x3eaaaee7u, /*  3.3336564898490905761718750e-01 */
        0xbe80061du, /* -2.5004664063453674316406250e-01 */
        0x3e4afb81u, /*  1.9822503626346588134765625e-01 */
        0xbe289358u, /* -1.6462457180023193359375000e-01 */
        0x3e2db86bu, /*  1.6964881122112274169921875e-01 */
        0xbe1b6a22u  /* -1.5177205204963684082031250e-01 */
    },
    0x00800000u /*== iHiDelta = SP 80000000-7f800000 ==*/
    ,
    0x01000000u /*== iLoRange = SP 00800000+iHiDelta ==*/
    ,
    0x3f2aaaabu /*== iBrkValue = SP 2/3 ==*/
    ,
    0x007fffffu /*== iOffExpoMask = SP significand mask ==*/
    ,
    0x3f800000u /*== sOne = SP 1.0 ==*/
    ,
    0x3f317200u,
    0x35bfbe8eu, /*== sLn2 = SP ln(2) ==*/
    /* EXP PART */
    0x3FB8AA3Bu, /* _sInvLn2  k=0 */
    0x4b400000u, /* _sShifter */
    0x3F317218u, /* _sLn2hi   */
    0x3F800000u, /* _sPC0  */
    0x3F8003DEu, /* _sPC1  */
    0x3F00F2D6u, /* _sPC2  */
    0x3E2963ACu, /* _sPC3  */
    0x0000007fu, /* _iBias */
    0x7fffffffu, /* _iAbsMask */
    0x42ae9a00u, /* _iDomainRange */
    0x33800000u, /*_s2N_2*/
    0x7f7fffffu, /*_sHuge*/
    0x00800000u, /*_sTiny*/
    0x80800000u, /* NMINNORM */
    0xfeffffffu, /* NMAXVAL */
    0x7f800000u, /* INF */
};               /*sPow_Table*/
/*
//
// ++
//   Following definitions and table look-up were generated automatically.
//   DO NOT CHANGE THEM.
// --
//
//
// ++
//   Below is table look-up and macro definitions to access its entries.
//   Table contains constants mentioned in ALGORITHM DESCRIPTION section
//   above.
// --
//
*/
static __constant _iml_v2_sp_union_t __spow_ep_CoutTab[380] = {
    0x3F800000, /* RCP1[  0] = +1.000000000000e+00 */
    0x3F640000, /* RCP1[  1] = +8.906250000000e-01 */
    0x3F4C0000, /* RCP1[  2] = +7.968750000000e-01 */
    0x3F3A0000, /* RCP1[  3] = +7.265625000000e-01 */
    0x3F2A0000, /* RCP1[  4] = +6.640625000000e-01 */
    0x3F1E0000, /* RCP1[  5] = +6.171875000000e-01 */
    0x3F120000, /* RCP1[  6] = +5.703125000000e-01 */
    0x3F080000, /* RCP1[  7] = +5.312500000000e-01 */
    0x3F000000, /* RCP1[  8] = +5.000000000000e-01 */
    /* Table of L1HI[j], L1LO[j] */
    0x00000000, /* L1HI[  0] = +0.000000000000e-01 */
    0x00000000, /* L1LO[  0] = +0.000000000000e-01 */
    0x3E2B1E00, /* L1HI[  1] = +1.671066284180e-01 */
    0x36614FFD, /* L1LO[  1] = +3.357417289575e-06 */
    0x3EA7B700, /* L1HI[  2] = +3.275680541992e-01 */
    0x36DD9676, /* L1LO[  2] = +6.603829285660e-06 */
    0x3EEBF300, /* L1HI[  3] = +4.608383178711e-01 */
    0x3640ABC3, /* L1LO[  3] = +2.871020874860e-06 */
    0xBED19B00, /* L1HI[  4] = -4.093856811523e-01 */
    0xB6B053FB, /* L1LO[  4] = -5.254985358006e-06 */
    0xBE9B8900, /* L1HI[  5] = -3.037796020508e-01 */
    0xB599D49A, /* L1LO[  5] = -1.146126321674e-06 */
    0xBE426000, /* L1HI[  6] = -1.898193359375e-01 */
    0xB6AF40BC, /* L1LO[  6] = -5.222942517230e-06 */
    0xBDB31C00, /* L1HI[  7] = -8.745574951172e-02 */
    0xB6EDF592, /* L1LO[  7] = -7.091738620658e-06 */
    0x00000000, /* L1HI[  8] = +0.000000000000e-01 */
    0x00000000, /* L1LO[  8] = +0.000000000000e-01 */
    /* Table of RCP2[j] */
    0x3F800000, /* RCP2[  0] = +1.000000000000e+00 */
    0x3F780000, /* RCP2[  1] = +9.687500000000e-01 */
    0x3F700000, /* RCP2[  2] = +9.375000000000e-01 */
    0x3F8A0000, /* RCP2[  3] = +1.078125000000e+00 */
    0x3F880000, /* RCP2[  4] = +1.062500000000e+00 */
    0x3F860000, /* RCP2[  5] = +1.046875000000e+00 */
    0x3F840000, /* RCP2[  6] = +1.031250000000e+00 */
    0x3F820000, /* RCP2[  7] = +1.015625000000e+00 */
    0x3F800000, /* RCP2[  8] = +1.000000000000e+00 */
    /* Table of L2HI[j], L2LO[j] */
    0x00000000, /* L2HI[  0] = +0.000000000000e-01 */
    0x00000000, /* L2LO[  0] = +0.000000000000e-01 */
    0x3D3B9800, /* L2HI[  1] = +4.579925537109e-02 */
    0x3694C9D9, /* L2LO[  1] = +4.434242031041e-06 */
    0x3DBEB000, /* L2HI[  2] = +9.310913085938e-02 */
    0x3492D9F7, /* L2LO[  2] = +2.735321064707e-07 */
    0xBDDE4000, /* L2HI[  3] = -1.085205078125e-01 */
    0xB684815B, /* L2LO[  3] = -3.948965669054e-06 */
    0xBDB31C00, /* L2HI[  4] = -8.745574951172e-02 */
    0xB6EDF592, /* L2LO[  4] = -7.091738620658e-06 */
    0xBD875800, /* L2HI[  5] = -6.608581542969e-02 */
    0xB6627E8A, /* L2LO[  5] = -3.375028084933e-06 */
    0xBD35D000, /* L2HI[  6] = -4.438781738281e-02 */
    0xB6D3758F, /* L2LO[  6] = -6.301975640938e-06 */
    0xBCB73000, /* L2HI[  7] = -2.236175537109e-02 */
    0xB6CB42E1, /* L2LO[  7] = -6.057657360758e-06 */
    0x00000000, /* L2HI[  8] = +0.000000000000e-01 */
    0x00000000, /* L2LO[  8] = +0.000000000000e-01 */
    /* Table of RCP3C[j] */
    0x3FB88000, /* RCP3C[  0] = +1.441406250000e+00 */
    0x3FB7C000, /* RCP3C[  1] = +1.435546875000e+00 */
    0x3FB70000, /* RCP3C[  2] = +1.429687500000e+00 */
    0x3FB64000, /* RCP3C[  3] = +1.423828125000e+00 */
    0x3FB5C000, /* RCP3C[  4] = +1.419921875000e+00 */
    0x3FBC8000, /* RCP3C[  5] = +1.472656250000e+00 */
    0x3FBC4000, /* RCP3C[  6] = +1.470703125000e+00 */
    0x3FBBC000, /* RCP3C[  7] = +1.466796875000e+00 */
    0x3FBB8000, /* RCP3C[  8] = +1.464843750000e+00 */
    0x3FBB0000, /* RCP3C[  9] = +1.460937500000e+00 */
    0x3FBAC000, /* RCP3C[ 10] = +1.458984375000e+00 */
    0x3FBA4000, /* RCP3C[ 11] = +1.455078125000e+00 */
    0x3FBA0000, /* RCP3C[ 12] = +1.453125000000e+00 */
    0x3FB98000, /* RCP3C[ 13] = +1.449218750000e+00 */
    0x3FB94000, /* RCP3C[ 14] = +1.447265625000e+00 */
    0x3FB8C000, /* RCP3C[ 15] = +1.443359375000e+00 */
    0x3FB88000, /* RCP3C[ 16] = +1.441406250000e+00 */
    /* Table of L3HI[j], L3LO[j] */
    0x00000000, /* L3HI[  0] = +0.000000000000e-01 */
    0x00000000, /* L3LO[  0] = +0.000000000000e-01 */
    0x3BC08000, /* L3HI[  1] = +5.874633789063e-03 */
    0x3601B0EA, /* L3LO[  1] = +1.932547547767e-06 */
    0x3C40E000, /* L3HI[  2] = +1.177215576172e-02 */
    0x36A82CE1, /* L3LO[  2] = +5.012014753447e-06 */
    0x3C910000, /* L3HI[  3] = +1.770019531250e-02 */
    0x35F27427, /* L3LO[  3] = +1.806420982575e-06 */
    0x3CB17000, /* L3HI[  4] = +2.165985107422e-02 */
    0x36BBF0CC, /* L3LO[  4] = +5.601066732197e-06 */
    0xBCFD7000, /* L3HI[  5] = -3.093719482422e-02 */
    0xB6DA84F4, /* L3LO[  5] = -6.512384061352e-06 */
    0xBCEDC000, /* L3HI[  6] = -2.902221679688e-02 */
    0xB6E53CD7, /* L3LO[  6] = -6.831814516772e-06 */
    0xBCCE5000, /* L3HI[  7] = -2.518463134766e-02 */
    0xB6FA51D3, /* L3LO[  7] = -7.460106189683e-06 */
    0xBCBEA000, /* L3HI[  8] = -2.326965332031e-02 */
    0xB4074B50, /* L3LO[  8] = -1.260025328520e-07 */
    0xBC9F1000, /* L3HI[  9] = -1.941680908203e-02 */
    0xB52D128E, /* L3LO[  9] = -6.447452278735e-07 */
    0xBC8F4000, /* L3HI[ 10] = -1.748657226563e-02 */
    0xB5655E44, /* L3LO[ 10] = -8.544632237317e-07 */
    0xBC5F2000, /* L3HI[ 11] = -1.361846923828e-02 */
    0xB59903D9, /* L3LO[ 11] = -1.140050812865e-06 */
    0xBC3F6000, /* L3HI[ 12] = -1.168060302734e-02 */
    0xB5A1551A, /* L3LO[ 12] = -1.202020257551e-06 */
    0xBBFF8000, /* L3HI[ 13] = -7.797241210938e-03 */
    0xB5979427, /* L3LO[ 13] = -1.129349470830e-06 */
    0xBBBFC000, /* L3HI[ 14] = -5.851745605469e-03 */
    0xB5839E88, /* L3LO[ 14] = -9.806399248191e-07 */
    0xBB000000, /* L3HI[ 15] = -1.953125000000e-03 */
    0xB4E32477, /* L3LO[ 15] = -4.230857655330e-07 */
    0x00000000, /* L3HI[ 16] = +0.000000000000e-01 */
    0x00000000, /* L3LO[ 16] = +0.000000000000e-01 */
    /* Table of THI[j], TLO[j] */
    0x3F800000, /* THI[  0] = +1.000000000000e+00 */
    0x00000000, /* TLO[  0] = +0.000000000000e-01 */
    0x3F80B1EE, /* THI[  1] = +1.005429983139e+00 */
    0xB3B02666, /* TLO[  1] = -8.202623526459e-08 */
    0x3F8164D2, /* THI[  2] = +1.010889291763e+00 */
    0xB1C43FD0, /* TLO[  2] = -5.711605204042e-09 */
    0x3F8218B0, /* THI[  3] = +1.016378402710e+00 */
    0xB3BC8C04, /* TLO[  3] = -8.779900789956e-08 */
    0x3F82CD86, /* THI[  4] = +1.021897077560e+00 */
    0x3398AC2C, /* TLO[  4] = +7.109369187355e-08 */
    0x3F83835A, /* THI[  5] = +1.027446031570e+00 */
    0xB3B11049, /* TLO[  5] = -8.245167087377e-08 */
    0x3F843A28, /* THI[  6] = +1.033024787903e+00 */
    0x33C3ACDE, /* TLO[  6] = +9.111839639125e-08 */
    0x3F84F1F6, /* THI[  7] = +1.038634061813e+00 */
    0x332C6F38, /* TLO[  7] = +4.014802429842e-08 */
    0x3F85AAC4, /* THI[  8] = +1.044273853302e+00 */
    0xB39833B8, /* TLO[  8] = -7.087458811280e-08 */
    0x3F866492, /* THI[  9] = +1.049944162369e+00 */
    0xB3A46DC0, /* TLO[  9] = -7.656808714798e-08 */
    0x3F871F62, /* THI[ 10] = +1.055645227432e+00 */
    0xB352C2E6, /* TLO[ 10] = -4.907169381775e-08 */
    0x3F87DB36, /* THI[ 11] = +1.061377286911e+00 */
    0xB3800967, /* TLO[ 11] = -5.962174866124e-08 */
    0x3F88980E, /* THI[ 12] = +1.067140340805e+00 */
    0x338092DB, /* TLO[ 12] = +5.987176990723e-08 */
    0x3F8955EE, /* THI[ 13] = +1.072934865952e+00 */
    0x30D86398, /* TLO[ 13] = +1.574437465448e-09 */
    0x3F8A14D6, /* THI[ 14] = +1.078760862350e+00 */
    0xB38AB691, /* TLO[ 14] = -6.459334407345e-08 */
    0x3F8AD4C6, /* THI[ 15] = +1.084618330002e+00 */
    0x330A58E5, /* TLO[ 15] = +3.221147818313e-08 */
    0x3F8B95C2, /* THI[ 16] = +1.090507745743e+00 */
    0xB260ABA1, /* TLO[ 16] = -1.307754019236e-08 */
    0x3F8C57CA, /* THI[ 17] = +1.096429109573e+00 */
    0xB2EE6E43, /* TLO[ 17] = -2.775698743443e-08 */
    0x3F8D1AE0, /* THI[ 18] = +1.102382659912e+00 */
    0xB3A481A4, /* TLO[ 18] = -7.660426843144e-08 */
    0x3F8DDF04, /* THI[ 19] = +1.108368396759e+00 */
    0x32808B9A, /* TLO[ 19] = +1.496464543488e-08 */
    0x3F8EA43A, /* THI[ 20] = +1.114386796951e+00 */
    0xB3697465, /* TLO[ 20] = -5.435540140900e-08 */
    0x3F8F6A82, /* THI[ 21] = +1.120437860489e+00 */
    0xB3E81937, /* TLO[ 21] = -1.080792849171e-07 */
    0x3F9031DC, /* THI[ 22] = +1.126521587372e+00 */
    0x330628CD, /* TLO[ 22] = +3.123641572792e-08 */
    0x3F90FA4C, /* THI[ 23] = +1.132638454437e+00 */
    0x338BEEE5, /* TLO[ 23] = +6.516146336861e-08 */
    0x3F91C3D4, /* THI[ 24] = +1.138788700104e+00 */
    0xB38C54EE, /* TLO[ 24] = -6.534706811192e-08 */
    0x3F928E72, /* THI[ 25] = +1.144972085953e+00 */
    0x337B2A64, /* TLO[ 25] = +5.847904543033e-08 */
    0x3F935A2C, /* THI[ 26] = +1.151189327240e+00 */
    0xB3D0EC19, /* TLO[ 26] = -9.728700752856e-08 */
    0x3F942700, /* THI[ 27] = +1.157440185547e+00 */
    0xB3F054E4, /* TLO[ 27] = -1.119131239704e-07 */
    0x3F94F4F0, /* THI[ 28] = +1.163724899292e+00 */
    0xB32E0212, /* TLO[ 28] = -4.051441467369e-08 */
    0x3F95C3FE, /* THI[ 29] = +1.170043706894e+00 */
    0x3386D6CC, /* TLO[ 29] = +6.278932928964e-08 */
    0x3F96942E, /* THI[ 30] = +1.176397085190e+00 */
    0xB3C8DFE8, /* TLO[ 30] = -9.353953805965e-08 */
    0x3F97657E, /* THI[ 31] = +1.182784795761e+00 */
    0xB3B60E85, /* TLO[ 31] = -8.477676736851e-08 */
    0x3F9837F0, /* THI[ 32] = +1.189207077026e+00 */
    0x33231B71, /* TLO[ 32] = +3.797635387922e-08 */
    0x3F990B88, /* THI[ 33] = +1.195664405823e+00 */
    0xB26CC9F4, /* TLO[ 33] = -1.378292653167e-08 */
    0x3F99E046, /* THI[ 34] = +1.202156782150e+00 */
    0xB359BE90, /* TLO[ 34] = -5.069756541259e-08 */
    0x3F9AB62A, /* THI[ 35] = +1.208684206009e+00 */
    0x33FC9500, /* TLO[ 35] = +1.176176704445e-07 */
    0x3F9B8D3A, /* THI[ 36] = +1.215247392654e+00 */
    0xB30C5563, /* TLO[ 36] = -3.267395006720e-08 */
    0x3F9C6574, /* THI[ 37] = +1.221846103668e+00 */
    0xB397D13D, /* TLO[ 37] = -7.069545537372e-08 */
    0x3F9D3EDA, /* THI[ 38] = +1.228480577469e+00 */
    0xB331A601, /* TLO[ 38] = -4.136200206462e-08 */
    0x3F9E196E, /* THI[ 39] = +1.235151052475e+00 */
    0x3244EA39, /* TLO[ 39] = +1.146195771976e-08 */
    0x3F9EF532, /* THI[ 40] = +1.241857767105e+00 */
    0x33412342, /* TLO[ 40] = +4.496838150953e-08 */
    0x3F9FD228, /* THI[ 41] = +1.248600959778e+00 */
    0x32959003, /* TLO[ 41] = +1.741137270537e-08 */
    0x3FA0B052, /* THI[ 42] = +1.255380868912e+00 */
    0xB3F0468F, /* TLO[ 42] = -1.118870520745e-07 */
    0x3FA18FAE, /* THI[ 43] = +1.262197256088e+00 */
    0x33CA8545, /* TLO[ 43] = +9.430599387208e-08 */
    0x3FA27044, /* THI[ 44] = +1.269051074982e+00 */
    0xB3FCF3B7, /* TLO[ 44] = -1.177899562306e-07 */
    0x3FA35210, /* THI[ 45] = +1.275941848755e+00 */
    0xB39717FD, /* TLO[ 45] = -7.035849071212e-08 */
    0x3FA43516, /* THI[ 46] = +1.282870054245e+00 */
    0xB323EC33, /* TLO[ 46] = -3.816621683646e-08 */
    0x3FA51958, /* THI[ 47] = +1.289835929871e+00 */
    0xB37282C2, /* TLO[ 47] = -5.646393965652e-08 */
    0x3FA5FED6, /* THI[ 48] = +1.296839475632e+00 */
    0x33A9B151, /* TLO[ 48] = +7.901929579875e-08 */
    0x3FA6E594, /* THI[ 49] = +1.303881168365e+00 */
    0x33CFEEE8, /* TLO[ 49] = +9.682645738295e-08 */
    0x3FA7CD94, /* THI[ 50] = +1.310961246490e+00 */
    0xB3162D36, /* TLO[ 50] = -3.496571417370e-08 */
    0x3FA8B6D6, /* THI[ 51] = +1.318079710007e+00 */
    0xB3E984CE, /* TLO[ 51] = -1.087406498725e-07 */
    0x3FA9A15A, /* THI[ 52] = +1.325236558914e+00 */
    0x33B4EA7C, /* TLO[ 52] = +8.424555672432e-08 */
    0x3FAA8D26, /* THI[ 53] = +1.332432508469e+00 */
    0x3325D921, /* TLO[ 53] = +3.861453351966e-08 */
    0x3FAB7A3A, /* THI[ 54] = +1.339667558670e+00 */
    0xB314AD82, /* TLO[ 54] = -3.461674093995e-08 */
    0x3FAC6896, /* THI[ 55] = +1.346941709518e+00 */
    0x33A4BE40, /* TLO[ 55] = +7.671451321860e-08 */
    0x3FAD583E, /* THI[ 56] = +1.354255437851e+00 */
    0x33EA42A1, /* TLO[ 56] = +1.090859405799e-07 */
    0x3FAE4934, /* THI[ 57] = +1.361608982086e+00 */
    0x3325946B, /* TLO[ 57] = +3.855204311496e-08 */
    0x3FAF3B78, /* THI[ 58] = +1.369002342224e+00 */
    0x33AD690A, /* TLO[ 58] = +8.075046951818e-08 */
    0x3FB02F0E, /* THI[ 59] = +1.376435995102e+00 */
    0xB2D1247F, /* TLO[ 59] = -2.434739861072e-08 */
    0x3FB123F6, /* THI[ 60] = +1.383909940720e+00 */
    0xB37C5AA8, /* TLO[ 60] = -5.875577253731e-08 */
    0x3FB21A32, /* THI[ 61] = +1.391424417496e+00 */
    0xB33333CE, /* TLO[ 61] = -4.172380135191e-08 */
    0x3FB311C4, /* THI[ 62] = +1.398979663849e+00 */
    0x32154889, /* TLO[ 62] = +8.689434187085e-09 */
    0x3FB40AAE, /* THI[ 63] = +1.406575918198e+00 */
    0x33A2654C, /* TLO[ 63] = +7.562138360655e-08 */
    0x3FB504F4, /* THI[ 64] = +1.414213657379e+00 */
    0xB3CC0622, /* TLO[ 64] = -9.500605534182e-08 */
    0x3FB60094, /* THI[ 65] = +1.421892642975e+00 */
    0xB32F4254, /* TLO[ 65] = -4.080568795582e-08 */
    0x3FB6FD92, /* THI[ 66] = +1.429613351822e+00 */
    0xB266B974, /* TLO[ 66] = -1.342992940283e-08 */
    0x3FB7FBF0, /* THI[ 67] = +1.437376022339e+00 */
    0xB2D5CD70, /* TLO[ 67] = -2.488988486190e-08 */
    0x3FB8FBB0, /* THI[ 68] = +1.445180892944e+00 */
    0xB3B89D04, /* TLO[ 68] = -8.596728931746e-08 */
    0x3FB9FCD2, /* THI[ 69] = +1.453027963638e+00 */
    0x330A5817, /* TLO[ 69] = +3.221074690078e-08 */
    0x3FBAFF5A, /* THI[ 70] = +1.460917711258e+00 */
    0x33B2133E, /* TLO[ 70] = +8.292271241834e-08 */
    0x3FBC034A, /* THI[ 71] = +1.468850374222e+00 */
    0x337DE5D4, /* TLO[ 71] = +5.911518016905e-08 */
    0x3FBD08A4, /* THI[ 72] = +1.476826190948e+00 */
    0xB3414FE8, /* TLO[ 72] = -4.500898701674e-08 */
    0x3FBE0F68, /* THI[ 73] = +1.484845161438e+00 */
    0x31986099, /* TLO[ 73] = +4.434764219295e-09 */
    0x3FBF179A, /* THI[ 74] = +1.492907762527e+00 */
    0xB3130B1A, /* TLO[ 74] = -3.423620097111e-08 */
    0x3FC0213A, /* THI[ 75] = +1.501013994217e+00 */
    0x33A1F0D1, /* TLO[ 75] = +7.540950657495e-08 */
    0x3FC12C4C, /* THI[ 76] = +1.509164333344e+00 */
    0x33CA6671, /* TLO[ 76] = +9.424991688039e-08 */
    0x3FC238D2, /* THI[ 77] = +1.517359018326e+00 */
    0x32C478F6, /* TLO[ 77] = +2.287240903475e-08 */
    0x3FC346CC, /* THI[ 78] = +1.525598049164e+00 */
    0x33DA2497, /* TLO[ 78] = +1.015807199475e-07 */
    0x3FC4563E, /* THI[ 79] = +1.533881902695e+00 */
    0x33CC5335, /* TLO[ 79] = +9.514625385336e-08 */
    0x3FC5672A, /* THI[ 80] = +1.542210817337e+00 */
    0x320AA837, /* TLO[ 80] = +8.070904690800e-09 */
    0x3FC67990, /* THI[ 81] = +1.550584793091e+00 */
    0x33B5AA24, /* TLO[ 81] = +8.459417964662e-08 */
    0x3FC78D74, /* THI[ 82] = +1.559004306793e+00 */
    0x33C8ABBA, /* TLO[ 82] = +9.344462407641e-08 */
    0x3FC8A2D8, /* THI[ 83] = +1.567469596863e+00 */
    0x33391FFC, /* TLO[ 83] = +4.310275992431e-08 */
    0x3FC9B9BE, /* THI[ 84] = +1.575980901718e+00 */
    0xB37323A2, /* TLO[ 84] = -5.661025316198e-08 */
    0x3FCAD226, /* THI[ 85] = +1.584538221359e+00 */
    0x333C8521, /* TLO[ 85] = +4.389324080043e-08 */
    0x3FCBEC14, /* THI[ 86] = +1.593142032623e+00 */
    0x33FEF272, /* TLO[ 86] = +1.187189758823e-07 */
    0x3FCD078C, /* THI[ 87] = +1.601792812347e+00 */
    0xB3735F84, /* TLO[ 87] = -5.666471875558e-08 */
    0x3FCE248C, /* THI[ 88] = +1.610490322113e+00 */
    0x3228FC24, /* TLO[ 88] = +9.836217198805e-09 */
    0x3FCF4318, /* THI[ 89] = +1.619235038757e+00 */
    0x33CF1919, /* TLO[ 89] = +9.643753953055e-08 */
    0x3FD06334, /* THI[ 90] = +1.628027439117e+00 */
    0xB2944353, /* TLO[ 90] = -1.726008387378e-08 */
    0x3FD184E0, /* THI[ 91] = +1.636867523193e+00 */
    0xB39DAE96, /* TLO[ 91] = -7.342639488723e-08 */
    0x3FD2A81E, /* THI[ 92] = +1.645755529404e+00 */
    0xB35C1DAA, /* TLO[ 92] = -5.124972167892e-08 */
    0x3FD3CCF0, /* THI[ 93] = +1.654691696167e+00 */
    0x3399859B, /* TLO[ 93] = +7.148920221007e-08 */
    0x3FD4F35A, /* THI[ 94] = +1.663676500320e+00 */
    0x33ABCFEE, /* TLO[ 94] = +8.000630186473e-08 */
    0x3FD61B5E, /* THI[ 95] = +1.672710180283e+00 */
    0xB0303219, /* TLO[ 95] = -6.409961998187e-10 */
    0x3FD744FC, /* THI[ 96] = +1.681792736053e+00 */
    0x33CAD69D, /* TLO[ 96] = +9.445396228919e-08 */
    0x3FD8703A, /* THI[ 97] = +1.690924882889e+00 */
    0xB3B3924D, /* TLO[ 97] = -8.361948876334e-08 */
    0x3FD99D16, /* THI[ 98] = +1.700106382370e+00 */
    0xB2F61D41, /* TLO[ 98] = -2.865147164769e-08 */
    0x3FDACB94, /* THI[ 99] = +1.709337711334e+00 */
    0x335E5594, /* TLO[ 99] = +5.176623431148e-08 */
    0x3FDBFBB8, /* THI[100] = +1.718619346619e+00 */
    0xB3504A1C, /* TLO[100] = -4.849617442812e-08 */
    0x3FDD2D82, /* THI[101] = +1.727951288223e+00 */
    0xB375EF9B, /* TLO[101] = -5.726142903932e-08 */
    0x3FDE60F4, /* THI[102] = +1.737333774567e+00 */
    0x33825E0F, /* TLO[102] = +6.070705585837e-08 */
    0x3FDF9612, /* THI[103] = +1.746767282486e+00 */
    0x33DEB8F0, /* TLO[103] = +1.037132070260e-07 */
    0x3FE0CCDE, /* THI[104] = +1.756252050400e+00 */
    0x33EC2A95, /* TLO[104] = +1.099735192097e-07 */
    0x3FE2055A, /* THI[105] = +1.765788316727e+00 */
    0x33FFFE84, /* TLO[105] = +1.192065882507e-07 */
    0x3FE33F8A, /* THI[106] = +1.775376558304e+00 */
    0xB38D4176, /* TLO[106] = -6.577731175526e-08 */
    0x3FE47B6C, /* THI[107] = +1.785016536713e+00 */
    0x33A0373E, /* TLO[107] = +7.460628849577e-08 */
    0x3FE5B906, /* THI[108] = +1.794708967209e+00 */
    0x33E77C83, /* TLO[108] = +1.077942448817e-07 */
    0x3FE6F85A, /* THI[109] = +1.804454088211e+00 */
    0x33AAEE20, /* TLO[109] = +7.959556431000e-08 */
    0x3FE8396A, /* THI[110] = +1.814252138138e+00 */
    0x33207898, /* TLO[110] = +3.736258137344e-08 */
    0x3FE97C38, /* THI[111] = +1.824103355408e+00 */
    0x3300D89F, /* TLO[111] = +2.999933846725e-08 */
    0x3FEAC0C6, /* THI[112] = +1.834007978439e+00 */
    0x33E7DD24, /* TLO[112] = +1.079700114088e-07 */
    0x3FEC0718, /* THI[113] = +1.843966484070e+00 */
    0x33B64C1D, /* TLO[113] = +8.488880170632e-08 */
    0x3FED4F30, /* THI[114] = +1.853979110718e+00 */
    0x3276CCA1, /* TLO[114] = +1.436561213089e-08 */
    0x3FEE9910, /* THI[115] = +1.864046096802e+00 */
    0xB34FE4BA, /* TLO[115] = -4.840396876769e-08 */
    0x3FEFE4BA, /* THI[116] = +1.874167680740e+00 */
    0xB348464A, /* TLO[116] = -4.663005654398e-08 */
    0x3FF13230, /* THI[117] = +1.884344100952e+00 */
    0x33A7AD09, /* TLO[117] = +7.808018601219e-08 */
    0x3FF28178, /* THI[118] = +1.894576072693e+00 */
    0xB3C3A600, /* TLO[118] = -9.110590545241e-08 */
    0x3FF3D290, /* THI[119] = +1.904863357544e+00 */
    0xB2871670, /* TLO[119] = -1.572627110885e-08 */
    0x3FF5257E, /* THI[120] = +1.915206670761e+00 */
    0xB3EADB79, /* TLO[120] = -1.093639611046e-07 */
    0x3FF67A42, /* THI[121] = +1.925606012344e+00 */
    0xB3938CC0, /* TLO[121] = -6.870823542261e-08 */
    0x3FF7D0E0, /* THI[122] = +1.936061859131e+00 */
    0xB38CF52F, /* TLO[122] = -6.563856492440e-08 */
    0x3FF9295A, /* THI[123] = +1.946574449539e+00 */
    0xB3094457, /* TLO[123] = -3.195995128397e-08 */
    0x3FFA83B2, /* THI[124] = +1.957144021988e+00 */
    0x33DB722A, /* TLO[124] = +1.021874852300e-07 */
    0x3FFBDFEE, /* THI[125] = +1.967771291733e+00 */
    0xB3931A0F, /* TLO[125] = -6.849961230782e-08 */
    0x3FFD3E0C, /* THI[126] = +1.978456020355e+00 */
    0x31CF486C, /* TLO[126] = +6.032726358883e-09 */
    0x3FFE9E12, /* THI[127] = +1.989198923111e+00 */
    0xB3A38470, /* TLO[127] = -7.614369556276e-08 */
    /* Coefficients of Log2Poly */
    0x3A6A6369, /* A1 = +8.9412050813582940e-04 */
    0xBEB1C35D, /* A2 = -3.4719362445792496e-01 */
    0x3E246F69, /* A3 = +1.6058124175028908e-01 */
    0xBDAB1EA1, /* A4 = -8.3554514082290152e-02 */
    /* Coefficients of Exp2Poly */
    0x3F317218, /* B1 = +6.9314718055994531e-01 */
    0x3E75FDF0, /* B2 = +2.4022650695910071e-01 */
    0x3D635847, /* B3 = +5.5504108664821580e-02 */
    /* Other constants */
    0x7F000000, /* BIG_SMALL(0) = BIG_VALUE = 2^(127) */
    0x00800000, /* BIG_SMALL(1) = SMALL_VALUE = 2^(-126) */
    0x00000000, /* ZERO = 0.0 */
    0x3F800000, /* ONES(0) = ONE  = +1.0 */
    0xBF800000, /* ONES(1) = MONE = -1.0 */
    /* Right shifter to obtain index j for getting */
    /* THI[j], TLO[j] from a table */
    0x47C00000, /* RST = 1.5*2^(22) */
    /* High part of 1/ln(2) */
    0x3FB88000, /* C = +1.4414062500000000e+00 */
    /* Constant used to obtain high bits */
    0x45800800, /* T12 = 2^(12)+1 */
    /* Constants to deal with denormals */
    0x5F800000, /* DENO_SCALE   = 2^(+64) */
    0x1F800000, /* DENO_UNSCALE = 2^(-64) */
    0x00000000, /* ZEROS(0) = +0 */
    0x80000000, /* ZEROS(1) = -0 */
};
static int __spow_ep_TestIntFunc(float a) {
  int x = (*(int *)&a) & 0x7fffffff;
  int e;
  if ((x < 0x3f800000) || (x >= 0x7f800000)) {
    return 0; /* |a|<1, |a|=INF, or a=NaN */
  }
  if (x >= 0x4B800000) {
    return 2; /* |a|>=2^24 */
  }
  e = ((x & 0x7f800000) - 0x3f800000) >> 23; /* Exponent of x, 0<=e<=23. */
  x = x << e;
  if ((x << 9) != 0) {
    return 0;
  }
  if ((x << 8) == 0x80000000) {
    return 1;
  }
  return 2;
}
__attribute__((always_inline)) inline int
__ocl_svml_internal_spow_ep(float *a, float *b, float *r) {
  int nRet = 0;
  float flVTmp1, flVTmp2, flVPHH, flVPHL;
  float flAX, flSignRes, flX1, flRcp1, flL1Hi, flL1Lo, flX2, flRcp2, flL2Hi,
      flL2Lo, flX3, flRcp3C, flL3Hi, flL3Lo, flK, flT, flD, flR1, flCQ, flRcpC,
      flX1Hi, flX1Lo, flRcpCHi, flRcpCLo, flTmp1, flE, flT_CQHi, flCQLo, flR,
      flLogPart3, flLog2Poly, flHH, flHL, flHLL, flYHi, flYLo, flTmp2, flTmp3,
      flPH, flPL, flPLL, flZ, flExp2Poly, flExp2PolyT, flResLo, flResHi, flRes,
      flTwoPowN, flAY, flAi, flBi;
  float flT_lo_1, flT_lo_2, flT_lo_3;
  int i, iEXB, iEYB, iSignX, iSignY, iYHi, iYLo, iYIsFinite, iEY, iYIsInt,
      iXIsFinite, iDenoExpAdd, iXHi, k, i1, i2, i3, iELogAX, iN, j, iERes,
      iSign, iIsSigZeroX, iIsSigZeroY, iYMantissa, iEX;
  flAi = *a;
  flBi = *b;
  /* Set up denormals to signed zero when FTZ\DAZ is on*/
  if (flAi == 0)
    flAi = flAi + flAi;
  if (flBi == 0)
    flBi = flBi + flBi;
  /* Get biased exponent of x, y */
  iEXB = ((((_iml_v2_sp_union_t *)&flAi)->hex[0] >> 23) & 0xFF);
  iEYB = ((((_iml_v2_sp_union_t *)&flBi)->hex[0] >> 23) & 0xFF);
  /* Get unbiased exponent of x, y */
  iEX = iEXB - 0x7F;
  iEY = iEYB - 0x7F;
  /* Get sign of x,y */
  iSignX = (((_iml_v2_sp_union_t *)&flAi)->hex[0] >> 31);
  iSignY = (((_iml_v2_sp_union_t *)&flBi)->hex[0] >> 31);
  /* Check whether significands of x,y are zero */
  iIsSigZeroX = ((((_iml_v2_sp_union_t *)&flAi)->hex[0] & 0x007FFFFF) == 0);
  iIsSigZeroY = ((((_iml_v2_sp_union_t *)&flBi)->hex[0] & 0x007FFFFF) == 0);
  /* Check if y is finite number */
  iYIsFinite = (((((_iml_v2_sp_union_t *)&flBi)->hex[0] >> 23) & 0xFF) != 0xFF);
  /* Get y mantissa */
  iYMantissa = (((_iml_v2_sp_union_t *)&flBi)->hex[0] & 0x007FFFFF);
  /* Test whether Y is integer */
  iYIsInt = __spow_ep_TestIntFunc(flBi);
  /* Here if x<>1, y<>0 (x or y can be NaN) */
  if (!((iSignX == 0) && (iEXB == 0x7F) && iIsSigZeroX) &&
      !((iEYB == 0) && iIsSigZeroY)) {
    ;
    /* Check if x is finite number */
    iXIsFinite = (((((_iml_v2_sp_union_t *)&flAi)->hex[0] >> 23) & 0xFF) != 0xFF);
    /* Filter out NaNs */
    if ((iXIsFinite || iIsSigZeroX) && (iYIsFinite || iIsSigZeroY)) {
      ;
      /* Here if x<>NaN, y<>NaN, x<>1, y<>0 */
      if (flAi != ((__constant float *)__spow_ep_CoutTab)[370]) {
        ;
        /* Here if x<>NaN, y<>NaN, x<>0, y<>0, x<>1 */
        if (!((flAi == ((__constant float *)__spow_ep_CoutTab)[372]) &&
              (iYIsInt || !iYIsFinite))) {
          ;
          /* Here if x<>NaN, y<>NaN, x<>0, y<>0, x<>1, and */
          /* if x=-1 then y can only be finite non-integer */
          if (iXIsFinite && iYIsFinite) {
            ;
            /* Here if x,y are finite nonzero numbers, x<>1, */
            /* and                                           */
            /* if x=-1 then y can only be finite non-integer */
            if ((flAi > ((__constant float *)__spow_ep_CoutTab)[370]) ||
                iYIsInt) {
              ;
              /* Path 7). "Main" path: x,y are finite     */
              /*          nonzero numbers, |x|<>1,        */
              /*          and if x<0 then y is integer    */
              /* 7.a) Get sign of the result into dbSignRes */
              flSignRes = ((__constant float *)
                               __spow_ep_CoutTab)[371 + (iSignX & iYIsInt)];
              /* 7.b) Calculating r = log2|x| */
              /* 7.b.1) Getting X1 */
              /* At first, represent |x| in the form  */
              /* |x| = 2^iDenoExpAdd * AX,            */
              /* where AX is normalized               */
              iDenoExpAdd = 0;
              flAX = flAi;
              (((_iml_v2_sp_union_t *)&flAX)->hex[0] =
                   (((_iml_v2_sp_union_t *)&flAX)->hex[0] & 0x7FFFFFFF) |
                   ((_iml_uint32_t)(0) << 31));
              if (iEXB == 0) {
                /* Here if x is denormal */
                flAX = flAX * ((__constant float *)__spow_ep_CoutTab)[376];
                iDenoExpAdd = iDenoExpAdd - 64;
              }
              /* Then get X1 by copying X1 := AX and      */
              /* setting exponent field of X1 to biased 0 */
              flX1 = flAX;
              (((_iml_v2_sp_union_t *)&flX1)->hex[0] =
                   (((_iml_v2_sp_union_t *)&flX1)->hex[0] & 0x807FFFFF) |
                   (((_iml_uint32_t)(0x7F) & 0xFF) << 23));
              /* 7.b.2) Getting k */
              /* Get high 32 bits of AX into XHi */
              iXHi = ((((_iml_v2_sp_union_t *)&flAX)->hex[0] >> 23) & 0xFF);
              iXHi = iXHi << 23;
              iXHi = iXHi | (((_iml_v2_sp_union_t *)&flAX)->hex[0] & 0x007FFFFF);
              /* Get k using XHi */
              k = iXHi - 0x3F380000;
              k = k >> 23;
              k = k + iDenoExpAdd;
              /* 7.b.3) Get Rcp1, log2(1/Rcp1) from tables */
              /* Get index i1 from rcpK1 most */
              /* significand bits of X1 */
              i1 = (((_iml_v2_sp_union_t *)&flX1)->hex[0] & 0x007FFFFF);
              i1 = i1 & 0x780000;
              i1 = i1 + 0x80000;
              i1 = i1 >> 20;
              /* Get Rcp1 */
              flRcp1 = ((__constant float *)__spow_ep_CoutTab)[0 + i1];
              /* Get log2(1/Rcp1) */
              flL1Hi =
                  ((__constant float *)__spow_ep_CoutTab)[9 + 2 * (i1) + 0];
              flL1Lo =
                  ((__constant float *)__spow_ep_CoutTab)[9 + 2 * (i1) + 1];
              /* 7.b.4) Get Rcp2, log2(1/Rcp2) from tables */
              /* Get X2 */
              flX2 = flX1 * flRcp1;
              /* Get index i2 from rcpK2 bits of */
              /* significand of X2 */
              i2 = (((_iml_v2_sp_union_t *)&flX2)->hex[0] & 0x007FFFFF);
              i2 = i2 & 0x1E0000;
              i2 = i2 + 0x20000;
              i2 = i2 >> 18;
              /* Get Rcp2 */
              flRcp2 = ((__constant float *)__spow_ep_CoutTab)[27 + i2];
              /* Get log2(1/Rcp2) */
              flL2Hi =
                  ((__constant float *)__spow_ep_CoutTab)[36 + 2 * (i2) + 0];
              flL2Lo =
                  ((__constant float *)__spow_ep_CoutTab)[36 + 2 * (i2) + 1];
              /* 7.b.5) get Rcp3C, log2(C/Rcp3C)  */
              /*        from tables               */
              /* Get X3 */
              flX3 = (flX2 * flRcp2);
              /* Get index i3 from rcpK3 bits of */
              /* significand of X3 */
              i3 = (((_iml_v2_sp_union_t *)&flX3)->hex[0] & 0x007FFFFF);
              i3 = i3 & 0x7C000;
              i3 = i3 + 0x4000;
              i3 = i3 >> 15;
              /* Get Rcp3C */
              flRcp3C = ((__constant float *)__spow_ep_CoutTab)[54 + i3];
              /* Get log2(C/Rcp3C) */
              flL3Hi =
                  ((__constant float *)__spow_ep_CoutTab)[71 + 2 * (i3) + 0];
              flL3Lo =
                  ((__constant float *)__spow_ep_CoutTab)[71 + 2 * (i3) + 1];
              /* 7.b.6) Recombine                          */
              /* k+log2(1/Rcp1)+log2(1/Rcp2)+log2(C/Rcp3C) */
              /* T := k + L1Hi + L2Hi + L3Hi */
              flK = (float)k;
              flVTmp1 = ((flK) + (flL1Hi));
              flTmp1 = ((flK)-flVTmp1);
              flVTmp2 = (flTmp1 + (flL1Hi));
              flT = flVTmp1;
              flT_lo_1 = flVTmp2;
              flVTmp1 = ((flT) + (flL2Hi));
              flTmp1 = ((flT)-flVTmp1);
              flVTmp2 = (flTmp1 + (flL2Hi));
              flT = flVTmp1;
              flT_lo_2 = flVTmp2;
              flVTmp1 = ((flT) + (flL3Hi));
              flTmp1 = ((flT)-flVTmp1);
              flVTmp2 = (flTmp1 + (flL3Hi));
              flT = flVTmp1;
              flT_lo_3 = flVTmp2;
              flD = (flT_lo_1 + flT_lo_2);
              flD = (flD + flT_lo_3);
              flD = (flD + flL1Lo);
              flD = (flD + flL2Lo);
              flD = (flD + flL3Lo);
              /* 7.b.7) Get approximation CQ to cq */
              flR1 = (flX3 * flRcp3C);
              flCQ = (flR1 - ((__constant float *)__spow_ep_CoutTab)[374]);
              /* 7.b.8) Get the correction term E for CQ */
              /* RcpC := Rcp1 * Rcp2 * Rcp3C */
              flRcpC = (flRcp1 * flRcp2);
              flRcpC = (flRcpC * flRcp3C);
              /* Split X1 into sum X1Hi+X1Lo */
              flVTmp1 =
                  ((flX1) * (((__constant float *)__spow_ep_CoutTab)[375]));
              flVTmp2 = (flVTmp1 - (flX1));
              flVTmp1 = (flVTmp1 - flVTmp2);
              flVTmp2 = ((flX1)-flVTmp1);
              flX1Hi = flVTmp1;
              flX1Lo = flVTmp2;
              /* Split RcpC into sum RcpCHi+RcpCLo */
              flVTmp1 =
                  ((flRcpC) * (((__constant float *)__spow_ep_CoutTab)[375]));
              flVTmp2 = (flVTmp1 - (flRcpC));
              flVTmp1 = (flVTmp1 - flVTmp2);
              flVTmp2 = ((flRcpC)-flVTmp1);
              flRcpCHi = flVTmp1;
              flRcpCLo = flVTmp2;
              /* Computing E */
              flTmp1 = (flX1Hi * flRcpCHi);
              flE = (flTmp1 - flR1);
              flTmp1 = (flX1Lo * flRcpCHi);
              flE = (flE + flTmp1);
              flTmp1 = (flX1Hi * flRcpCLo);
              flE = (flE + flTmp1);
              flTmp1 = (flX1Lo * flRcpCLo);
              flE = (flE + flTmp1);
              /* 7.c) Get high part and exponent of log2|x| */
              /* Rebreak T + CQ into sum of high and low */
              /* parts T_CQHi + CQLo */
              flVTmp1 = ((flT) + (flCQ));
              flTmp1 = ((flT)-flVTmp1);
              flVTmp2 = (flTmp1 + (flCQ));
              flT_CQHi = flVTmp1;
              flCQLo = flVTmp2;
              /* Get exponent of T_CQHi */
              iELogAX = ((((_iml_v2_sp_union_t *)&flT_CQHi)->hex[0] >> 23) & 0xFF);
              /* 7.d) Estimate |y*log2|x|| */
              if (iELogAX + iEYB < 11 + 2 * 0x7F) {
                ;
                /* Here if ex(log2|x|) + ex(y) < 11 */
                /* Here we have 0 < |y*log2|x|| < 4*2^10 */
                if (iELogAX + iEYB > -62 + 2 * 0x7F) {
                  ;
                  /* Path 7.3). Here if                 */
                  /* -62 < ex(y) + ex(log2|x|) < 11     */
                  /* Here we have:                      */
                  /* 2^(-61) <= |y*log2|x|| < 4*2^10    */
                  /* 7.3.a) R := CQ + E */
                  flR = (flCQ + flE);
                  /* 7.3.b) Polynomial */
                  flLog2Poly =
                      ((((((__constant float *)__spow_ep_CoutTab)[364]) * flR +
                         ((__constant float *)__spow_ep_CoutTab)[363]) *
                            flR +
                        ((__constant float *)__spow_ep_CoutTab)[362]) *
                           flR +
                       ((__constant float *)__spow_ep_CoutTab)[361]) *
                      flR;
                  /* 7.3.c) Get 3 parts of log2|x| */
                  /* LogPart3 := CQLo + E + D */
                  flLogPart3 = (flCQLo + flE);
                  flLogPart3 = (flD + flLogPart3);
                  /* Rebreak T_CQHi + Log2Poly */
                  /* into HH + HL */
                  flVTmp1 = ((flT_CQHi) + (flLog2Poly));
                  flTmp1 = ((flT_CQHi)-flVTmp1);
                  flVTmp2 = (flTmp1 + (flLog2Poly));
                  flHH = flVTmp1;
                  flHL = flVTmp2;
                  /* Rebreak HH + LogPart3 */
                  /* into HH + HLL */
                  flVTmp1 = ((flHH) + (flLogPart3));
                  flTmp1 = ((flHH)-flVTmp1);
                  flVTmp2 = (flTmp1 + (flLogPart3));
                  flHH = flVTmp1;
                  flHLL = flVTmp2;
                  /* HLL := HLL + HL */
                  flHLL = (flHLL + flHL);
                  /* Split HH into HH + HL */
                  flVTmp1 =
                      ((flHH) * (((__constant float *)__spow_ep_CoutTab)[375]));
                  flVTmp2 = (flVTmp1 - (flHH));
                  flVTmp1 = (flVTmp1 - flVTmp2);
                  flVTmp2 = ((flHH)-flVTmp1);
                  flHH = flVTmp1;
                  flHL = flVTmp2;
                  /* 7.3.d) Calculation of y*(HH+HL+HLL)*/
                  /* Split y into YHi+YLo */
                  flVTmp1 =
                      ((flBi) * (((__constant float *)__spow_ep_CoutTab)[375]));
                  flVTmp2 = (flVTmp1 - (flBi));
                  flVTmp1 = (flVTmp1 - flVTmp2);
                  flVTmp2 = ((flBi)-flVTmp1);
                  flYHi = flVTmp1;
                  flYLo = flVTmp2;
                  /* Get high PH and medium PL parts of */
                  /* y*log2|x| */
                  flTmp1 = (flYHi * flHH);
                  flTmp2 = ((flYHi * flHL) + (flYLo * flHH));
                  flTmp3 = (flTmp1 + flTmp2);
                  flPL = (((flTmp1 - flTmp3) + flTmp2) + (flYLo * flHL));
                  flPH = flTmp3;
                  /* Get low PLL part of y*log2|x| */
                  flPLL = (flBi * flHLL);
                  /* 7.3.e) Calculation of 2^(PH+PL+PLL)*/
                  /* Break PH into PHH + PHL, */
                  /* where PHH = N + j/2^expK */
                  flVTmp1 =
                      (flPH + ((__constant float *)__spow_ep_CoutTab)[373]);
                  flVPHH =
                      (flVTmp1 - ((__constant float *)__spow_ep_CoutTab)[373]);
                  iN = (((_iml_v2_sp_union_t *)&flVTmp1)->hex[0] & 0x007FFFFF);
                  j = iN & 0x7F;
                  iN = iN << 10;
                  iN = iN >> (7 + 10);
                  flVPHL = (flPH - flVPHH);
                  /* Z = PHL + PL + PLL */
                  flZ = (flPLL + flPL);
                  flZ = (flZ + flVPHL);
                  /* Exponential polynomial */
                  flExp2Poly =
                      (((((__constant float *)__spow_ep_CoutTab)[367]) * flZ +
                        ((__constant float *)__spow_ep_CoutTab)[366]) *
                           flZ +
                       ((__constant float *)__spow_ep_CoutTab)[365]) *
                      flZ;
                  /* Get significand of 2^(PH+PL+PLL) */
                  /* in the form ResHi+ResLo          */
                  flExp2PolyT =
                      (flExp2Poly * ((__constant float *)
                                         __spow_ep_CoutTab)[105 + 2 * (j) + 0]);
                  flResLo = (flExp2PolyT +
                             ((__constant float *)
                                  __spow_ep_CoutTab)[105 + 2 * (j) + 1]);
                  flResHi = ((
                      __constant float *)__spow_ep_CoutTab)[105 + 2 * (j) + 0];
                  /* Get exponent ERes of the result */
                  flRes = (flResHi + flResLo);
                  iERes = ((((_iml_v2_sp_union_t *)&flRes)->hex[0] >> 23) & 0xFF);
                  iERes = (iERes - 0x7F);
                  iERes = (iERes + iN);
                  /* Result is less than overflow */
                  if (iERes < 128) {
                    ;
                    if (iERes >= -126) {
                      ;
                      /* Path 7.3.e.5) Here if */
                      /* -1022 <= ERes <= 1023 */
                      /* Result is normalized */
                      /* Res  := 2^N * Res */
                      (((_iml_v2_sp_union_t *)&flRes)->hex[0] =
                           (((_iml_v2_sp_union_t *)&flRes)->hex[0] & 0x807FFFFF) |
                           (((_iml_uint32_t)(iERes + 0x7F) & 0xFF) << 23));
                      /* r := Res * SignRes */
                      flRes = (flRes * flSignRes);
                      *r = flRes;
                    } else {
                      /* Here if result   */
                      /* is denormalized  */
                      /* or underflowed   */
                      /* Result is big denormal */
                      if (iERes >= -126 - 10) {
                        ;
                        /* Path 7.3.e.4) Here if */
                        /* -126-10 <=           */
                        /*        <= ERes <      */
                        /*               < -126 */
                        /* Rebreak ResHi+ResLo */
                        flVTmp1 = ((flResHi) + (flResLo));
                        flTmp1 = ((flResHi)-flVTmp1);
                        flVTmp2 = (flTmp1 + (flResLo));
                        flResHi = flVTmp1;
                        flResLo = flVTmp2;
                        flVTmp1 =
                            ((flResHi) *
                             (((__constant float *)__spow_ep_CoutTab)[375]));
                        flVTmp2 = (flVTmp1 - (flResHi));
                        flVTmp1 = (flVTmp1 - flVTmp2);
                        flVTmp2 = ((flResHi)-flVTmp1);
                        flResHi = flVTmp1;
                        flTmp2 = flVTmp2;
                        flResLo = (flResLo + flTmp2);
                        /* Unscale result */
                        iN = (iN + 64);
                        /* TwoPowN := 2^N */
                        flTwoPowN =
                            ((__constant float *)__spow_ep_CoutTab)[371];
                        (((_iml_v2_sp_union_t *)&flTwoPowN)->hex[0] =
                             (((_iml_v2_sp_union_t *)&flTwoPowN)->hex[0] &
                              0x807FFFFF) |
                             (((_iml_uint32_t)(iN + 0x7F) & 0xFF) << 23));
                        /* ResHi :=              */
                        /* ResHi*TwoPowN*SignRes  - was */
                        /* ResHi*TwoPowN  - fixed FTZ/DAZ */
                        /* ResLo :=              */
                        /* ResLo*TwoPowN*SignRes - was*/
                        /* ResLo*TwoPowN  - fixed FTZ/DAZ */
                        flResHi = (flResHi * flTwoPowN);
                        flResLo = (flResLo * flTwoPowN);
                        /* Res := ResHi + ResLo - was */
                        /* Res := (ResHi + ResLo)*SignRes - fixed FTZ/DAZ */
                        flRes = (flResHi + flResLo);
                        flRes = (flRes *
                                 ((__constant float *)__spow_ep_CoutTab)[377]);
                        /* r := Res            */
                        /*       + SMALL_VALUE    */
                        /*       * SMALL_VALUE    */
                        flVTmp1 = ((__constant float *)__spow_ep_CoutTab)[369];
                        flVTmp1 = (flVTmp1 * flVTmp1);
                        flRes = (flRes + flVTmp1);
                        *r = flRes * flSignRes;
                      } else {
                        ;
                        /* Result is small denormal     */
                        if (iERes >= -149 - 10) {
                          /* Path 7.3.e.3) Here */
                          /* if                 */
                          /* -149-10 <=        */
                          /*      <= ERes <     */
                          /*         < -126-10 */
                          flSignRes *=
                              ((__constant float *)__spow_ep_CoutTab)[377];
                          iN = iN + 64;
                          /* TwoPowN := 2^N */
                          flTwoPowN =
                              ((__constant float *)__spow_ep_CoutTab)[371];
                          (((_iml_v2_sp_union_t *)&flTwoPowN)->hex[0] =
                               (((_iml_v2_sp_union_t *)&flTwoPowN)->hex[0] &
                                0x807FFFFF) |
                               (((_iml_uint32_t)(iN + 0x7F) & 0xFF) << 23));
                          /* r:= */
                          flRes = (flRes * flTwoPowN);
                          flRes = (flRes * flSignRes);
                          flVTmp1 =
                              ((__constant float *)__spow_ep_CoutTab)[369];
                          flVTmp1 *= flVTmp1;
                          flRes = (flRes - flVTmp1);
                          *r = flRes;
                        }
                        /* Result is 0.0 = Underflow */
                        else {
                          ;
                          /* Path 7.3.e.2) Here */
                          /* if ERes < -149-10 */
                          flVTmp1 =
                              ((__constant float *)__spow_ep_CoutTab)[369];
                          flVTmp1 *= flVTmp1;
                          flRes = (flVTmp1 * flSignRes);
                          *r = flRes;
                        }
                      }
                    }
                  } /* if ( iERes < 128 ) */
                  /* Result is overflow */
                  else /* if ( iERes >= 128 ) */
                  {
                    ;
                    /* Path 7.3.e.1) Here if        */
                    /*               ERes >= 127   */
                    /* Overflow */
                    flVTmp1 = ((__constant float *)__spow_ep_CoutTab)[368];
                    flVTmp1 = (flVTmp1 * flVTmp1);
                    flRes = (flVTmp1 * flSignRes);
                    *r = flRes;
                  } /* if ( iERes >= 128 ) */
                } else {
                  ;
                  /* Path 7.2).                         */
                  /* Here if ex(y) + ex(log2|x|) <= -62 */
                  /* Here we have                       */
                  /* 0 < |y*log2|x|| <= 4*2^(-62)       */
                  flVTmp1 = ((__constant float *)__spow_ep_CoutTab)[371];
                  flVTmp1 =
                      (flVTmp1 + ((__constant float *)__spow_ep_CoutTab)[369]);
                  *r = (flVTmp1 * flSignRes);
                }
              } else {
                ;
                /* Path 7.1).                             */
                /* Here if ex(y) + ex(log2|x|) >= 11      */
                /* Here we have 2^11 <= |y*log2|x|| < Inf */
                /* Get sign of y*log|x| */
                iSign = iSignY ^ (((_iml_v2_sp_union_t *)&flT_CQHi)->hex[0] >> 31);
                /* If y*log|x|>0 then Tmp1=BIG_VALUE   */
                /*               else Tmp1=SMALL_VALUE */
                flTmp1 = ((__constant float *)__spow_ep_CoutTab)[368 + (iSign)];
                /* Tmp1 := Tmp1 * Tmp1 */
                flTmp1 = (flTmp1 * flTmp1);
                /* r := Tmp1 * SignRes */
                flTmp1 = (flTmp1 * flSignRes);
                *r = flTmp1;
                if ((!(((((_iml_v2_sp_union_t *)&flTmp1)->hex[0] >> 23) & 0xFF) !=
                       0xFF) &&
                     ((((_iml_v2_sp_union_t *)&flTmp1)->hex[0] & 0x007FFFFF) ==
                      0))) {
                  nRet = 3;
                }
              }
            } else {
              ;
              /* Path 6). Here if -Inf<x<0, y is finite   */
              /*          non-integer                     */
              flVTmp1 = ((__constant float *)__spow_ep_CoutTab)[370];
              flVTmp1 = (flVTmp1 / flVTmp1);
              *r = flVTmp1;
              nRet = 1;
            }
          } else {
            ;
            /* Path 5). Here if one of arguments is Inf, */
            /* but none of them is zero or NaN, */
            /* and |x|<>1 */
            if (iEXB < 0x7F) {
              ;
              /* Here if y=Inf, |x|<1 */
              if (iSignY) {
                ;
                /* Path 5.1). Here if |x|<1, y=-Inf */
                /* r := +Inf */
                *r = (flBi * flBi);
              } else {
                ;
                /* Path 5.2). Here if |x|<1, y=+Inf */
                /* r := +0 */
                *r = ((__constant float *)__spow_ep_CoutTab)[370];
              }
            } else {
              ;
              /* Here if one of arguments is Inf, */
              /* but none of them is zero or NaN, */
              /* and |x|>1 */
              if (iSignY) {
                ;
                /* Path 5.3). Here if |x|>1, y<0 */
                /* If x<0, y is negative odd integer */
                /* then r := -0 */
                /* else r := +0 */
                flRes = ((__constant float *)
                             __spow_ep_CoutTab)[378 + (iYIsInt & iSignX)];
                *r = flRes;
              } else {
                ;
                /* Path 5.4). Here if |x|>1, y>0 */
                /* If x<0, y is positive odd integer */
                /* then r := -Inf */
                /* else r := +Inf */
                flTmp1 = (flAi * flAi);
                flTmp1 = (flTmp1 * flBi);
                *r = flTmp1 * ((__constant float *)
                                   __spow_ep_CoutTab)[371 + (iYIsInt & iSignX)];
              }
            }
          }
        } else {
          ;
          /* Path 4). Here if x=-1 and y is non-zero integer */
          /*          or Inf                                 */
          /* If y is odd integer */
          /* then r := -1 */
          /* else r := +1 */
          *r = ((__constant float *)__spow_ep_CoutTab)[371 + (iYIsInt & 1)];
        }
      } else {
        ;
        /* Path 3). Here if x=0, y<>NaN, y<>0 */
        flTmp1 = flAi * flAi; /* Tmp1 := +0 */
        if (iSignY) {
          ;
          /* Path 3.1). Here if y<0, x=0, y<>NaN  */
          /* If x=-0, y is negative odd integer   */
          /* then r := -Inf                    */
          /* else r := +Inf                    */
          /* Raise DIVZ                           */
          *r = ((__constant float *)
                    __spow_ep_CoutTab)[371 + (iYIsInt & iSignX)] /
               flTmp1;
          nRet = 1;
        } else {
          ;
          /* Path 3.2). Here if y>0, x=0, y<>NaN  */
          /* If x=-0, y is positive odd integer   */
          /* then r := -0                      */
          /* else r := +0                      */
          *r = ((__constant float *)
                    __spow_ep_CoutTab)[371 + (iYIsInt & iSignX)] *
               flTmp1;
        }
      }
    } else {
      ;
      /* Path 2). Here if one of arguments is NaN */
      /* but x<>1, y<>0 */
      /* r := x + y */
      *r = *a + *b;
    }
  }
  /* x=1 or y=0 */
  else {
    ;
    /* Path 1). Here if x=1 or y=0 */
    /* Raise Invalid exception in case when one of arguments is SNaN */
    flVTmp1 = flAi + flBi;
    iSign = (((_iml_v2_sp_union_t *)&flVTmp1)->hex[0] >> 31);
    flVTmp2 = ((__constant float *)__spow_ep_CoutTab)[371];
    (((_iml_v2_sp_union_t *)&flVTmp2)->hex[0] =
         (((_iml_v2_sp_union_t *)&flVTmp2)->hex[0] & 0x7FFFFFFF) |
         ((_iml_uint32_t)(iSign) << 31));
    /* r := 1 */
    *r = flVTmp2 * flVTmp2;
  }
  return nRet;
}
float __ocl_svml_powf_ep(float x, float y) {
  float r;
  unsigned int vm;
  float va1;
  float va2;
  float vr1;
  va1 = x;
  va2 = y;
  {
    unsigned int iHiDelta;
    unsigned int iLoRange;
    unsigned int iBrkValue;
    unsigned int iOffExpoMask;
    float sOne;
    float sLn2Hi;
    float sLn2Lo;
    float sPoly[7];
    unsigned int iX;
    unsigned int iY;
    unsigned int iXTest;
    float sN;
    unsigned int iN;
    float sR;
    unsigned int iR;
    float sP;
    float sLnResHi;
    float sLnResLo;
    unsigned int iSpecX;
    unsigned int iSpecY;
    unsigned int _NMINNORM;
    unsigned int _NMAXVAL;
    unsigned int _INF;
    unsigned int iRangeMask;
    float sM;
    float s2N;
    unsigned int iAbsX;
    unsigned int iAbsZ;
    unsigned int iRes;
    unsigned int iP;
    unsigned int iM;
    float sInvLn2;
    float sShifter;
    float sLn2hi;
    float sLn2lo;
    unsigned int iBias;
    unsigned int iAbsMask;
    unsigned int iDomainRange;
    float sPC[6];
    iX = as_uint(va1);
    iY = as_uint(va2);
    _NMINNORM = (__ocl_svml_internal_spow_ep_data.NMINNORM);
    _NMAXVAL = (__ocl_svml_internal_spow_ep_data.NMAXVAL);
    _INF = (__ocl_svml_internal_spow_ep_data.INF);
    iAbsMask = (__ocl_svml_internal_spow_ep_data._iAbsMask);
    iSpecX = (iX - _NMINNORM);
    iSpecX = ((unsigned int)(-(signed int)((signed int)iSpecX >=
                                           (signed int)_NMAXVAL)));
    iSpecY = (iY & iAbsMask);
    iSpecY =
        ((unsigned int)(-(signed int)((signed int)iSpecY >= (signed int)_INF)));
    iRangeMask = (iSpecX | iSpecY);
    iBrkValue = (__ocl_svml_internal_spow_ep_data.iBrkValue);
    iOffExpoMask = (__ocl_svml_internal_spow_ep_data.iOffExpoMask);
    /* reduction: compute r,n */
    iX = (iX - iBrkValue);
    iR = (iX & iOffExpoMask);
    iN = ((signed int)iX >> (23));
    iR = (iR + iBrkValue);
    sN = ((float)((int)(iN)));
    sR = as_float(iR);
    sOne = as_float(__ocl_svml_internal_spow_ep_data.sOne);
    sR = (sR - sOne);
    sPoly[6] = as_float(__ocl_svml_internal_spow_ep_data.sPoly[6]);
    sPoly[5] = as_float(__ocl_svml_internal_spow_ep_data.sPoly[5]);
    sPoly[4] = as_float(__ocl_svml_internal_spow_ep_data.sPoly[4]);
    /* polynomial evaluation starts here*/
    sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(sPoly[6], sR, sPoly[5]);
    sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(sP, sR, sPoly[4]);
    sPoly[3] = as_float(__ocl_svml_internal_spow_ep_data.sPoly[3]);
    sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(sP, sR, sPoly[3]);
    sPoly[2] = as_float(__ocl_svml_internal_spow_ep_data.sPoly[2]);
    sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(sP, sR, sPoly[2]);
    sPoly[1] = as_float(__ocl_svml_internal_spow_ep_data.sPoly[1]);
    sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(sP, sR, sPoly[1]);
    sPoly[0] = as_float(__ocl_svml_internal_spow_ep_data.sPoly[0]);
    sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(sP, sR, sPoly[0]);
    sP = (sP * sR);
    sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(sP, sR, sR);
    sLn2Hi = as_float(__ocl_svml_internal_spow_ep_data.sLn2Hi);
    sLn2Lo = as_float(__ocl_svml_internal_spow_ep_data.sLn2Lo);
    sLnResLo = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(sN, sLn2Lo, sP);
    sLnResHi = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(sN, sLn2Hi, sLnResLo);
    sLnResHi = (sLnResHi * va2);
    /* Check for overflow\underflow */
    iAbsZ = as_uint(sLnResHi);
    iAbsZ = (iAbsZ & iAbsMask);
    iDomainRange = (__ocl_svml_internal_spow_ep_data._iDomainRange);
    iAbsZ = ((unsigned int)(-(signed int)((signed int)iAbsZ >
                                          (signed int)iDomainRange)));
    iRangeMask = (iRangeMask | iAbsZ);
    vm = 0;
    vm = iRangeMask;
    sInvLn2 = as_float(__ocl_svml_internal_spow_ep_data._sInvLn2);
    sShifter = as_float(__ocl_svml_internal_spow_ep_data._sShifter);
    sM = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(sLnResHi, sInvLn2, sShifter);
    sN = (sM - sShifter);
    iM = as_uint(sM);
    /* 2^N */
    iM = ((unsigned int)(iM) << (23));
    sLn2hi = as_float(__ocl_svml_internal_spow_ep_data._sLn2hi);
    /* R = LnResHi - N*Ln2Hi*/
    sR = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(-(sN), sLn2hi, sLnResHi);
    /* Polynomial */
    sPC[0] = as_float(__ocl_svml_internal_spow_ep_data._sPC0);
    sPC[1] = as_float(__ocl_svml_internal_spow_ep_data._sPC1);
    sPC[2] = as_float(__ocl_svml_internal_spow_ep_data._sPC2);
    sPC[3] = as_float(__ocl_svml_internal_spow_ep_data._sPC3);
    sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(sPC[3], sR, sPC[2]);
    sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(sP, sR, sPC[1]);
    sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(sP, sR, sPC[0]);
    /* Final reconstruction */
    iP = as_uint(sP);
    iRes = (iM + iP);
    vr1 = as_float(iRes);
  }
  if (__builtin_expect((vm) != 0, 0)) {
    float __cout_a1;
    float __cout_a2;
    float __cout_r1;
    ((float *)&__cout_a1)[0] = va1;
    ((float *)&__cout_a2)[0] = va2;
    ((float *)&__cout_r1)[0] = vr1;
    __ocl_svml_internal_spow_ep(&__cout_a1, &__cout_a2, &__cout_r1);
    vr1 = ((float *)&__cout_r1)[0];
  }
  r = vr1;
  return r;
}

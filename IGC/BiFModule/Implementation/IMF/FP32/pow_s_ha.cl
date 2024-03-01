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
static __constant unsigned char __spow_ha___rcp_tbl[] = {
    0xff, 0xf0, 0xe3, 0xd7, 0xcc, 0xc2, 0xb9, 0xb1, 0xaa,
    0xa3, 0x9d, 0x97, 0x91, 0x8c, 0x88, 0x83, 0x7f,
};
// -log2(_VSTATIC(__rcp_tbl)[i]/2^8))*2^(23+32)
static __constant unsigned long __spow_ha___log2_tbl[] = {
    0x0000000000000000uL, 0x000b2671360338acuL, 0x001563dc29ffacb2uL,
    0x001f5fd8a9063e36uL, 0x002906cbcd2baf2euL, 0x003243001249ba76uL,
    0x003afcd815786af2uL, 0x00431b2abc31565cuL, 0x004a83cf0d01c170uL,
    0x00523bbc64c5e644uL, 0x00591db662b66428uL, 0x006043e946fd97f4uL,
    0x0067b3d42fd0fc50uL, 0x006e232e68aad484uL, 0x007373af48dce654uL,
    0x007a514b229c40a0uL, 0x0080000000000000uL,
};
// polynomial coefficients
// c6*2^31
static __constant int __spow_ha___lc6 = 0xE158260E;
// c5*2^31
static __constant int __spow_ha___lc5 = 0x24F7FD36;
// c4*2^31
static __constant int __spow_ha___lc4 = 0xD1D568F0;
// c3*2^31
static __constant int __spow_ha___lc3 = 0x3D8E12ED;
// c2*2^31
static __constant int __spow_ha___lc2 = 0xA3AAE26C;
// c1*2^(23+32)
static __constant unsigned long __spow_ha___lc1 = 0xB8AA3B295EBB00uL;
// exp2 coefficients
// c7*2^32
static __constant int __spow_ha___sc7 = 0x00016B68;
// c6*2^32
static __constant int __spow_ha___sc6 = 0x00095E83;
// c5*2^32
static __constant int __spow_ha___sc5 = 0x00580436;
// c4*2^32
static __constant int __spow_ha___sc4 = 0x027607DE;
// c3*2^32
static __constant int __spow_ha___sc3 = 0x0E359872;
// c2*2^32
static __constant int __spow_ha___sc2 = 0x3D7F7977;
// c1*2^32
static __constant int __spow_ha___sc1 = 0xB1721817;
static unsigned int __spow_ha_powf_cout(unsigned int xin, unsigned int yin,
                                        int *errcode) {
  int mant, expon, index, sgn_y, R, poly, N;
  int expon_y, is_int, mant_y, mi_y;
  unsigned int rcp, res, shift, abs_y, poly_low, poly_h, sgn_x = 0, p_inf;
  unsigned long poly64, exp64, poly_s1;
  // unpack mantissa, unbiased exponent
  mant = ((xin)&0x7fffff);
  expon = ((xin) >> 23) - 0x7f;
  abs_y = yin & 0x7fffffff;
  sgn_y = (((int)(yin)) >> (31));
  if ((((unsigned int)(abs_y - 1)) >= (0x7F800000 - 1)))
    goto SPECIAL_Y;
  // filter out special and negative cases, as well as denormals
  if ((((unsigned int)(xin - 0x00800000)) >= (0x7F800000 - 0x00800000)))
    goto SPECIAL_X;
LOGF_MAIN:
  // add leading mantissa bit
  mant |= 0x00800000;
  // table index
  index = ((mant + 0x00040000) >> (23 - 4)) - 0x10;
  // rcp ~ 2^8/mant
  rcp = 1 + __spow_ha___rcp_tbl[index];
  // reduced argument R = mant*rcp - 1, scale 2^32
  R = (((unsigned int)mant) * ((unsigned int)rcp)); // scale 2^31
  R = R + R;
  // (c6*R+c5)*2^31
  poly = ((((long)((int)(__spow_ha___lc6))) * ((int)(R))) >> 32);
  poly = poly + __spow_ha___lc5;
  // poly*R+c4, scale 2^31
  poly = ((((long)((int)(poly))) * ((int)(R))) >> 32);
  poly = poly + __spow_ha___lc4;
  // poly*R+c3, scale 2^31
  poly = ((((long)((int)(poly))) * ((int)(R))) >> 32);
  poly = poly + __spow_ha___lc3;
  // poly*R+c2, scale 2^31
  poly = ((((long)((int)(poly))) * ((int)(R))) >> 32);
  poly = poly + __spow_ha___lc2;
  // poly*2^(23+32)
  poly_low = poly << (32 - 8);
  poly_h = (((int)(poly)) >> (8));
  // c1+R*poly, scale 2^(23+32)
  poly64 = (((long)((int)(poly_h))) * ((int)(R))) + __spow_ha___lc1;
  // poly_low to be treated as positive value
  poly_low = (((unsigned int)(poly_low)) >> (1));
  poly_low = ((((long)((int)(poly_low))) * ((int)(R))) >> 32);
  poly_low += poly_low;
  poly64 += (long)((int)poly_low);
  // adjustment for x near 1.0
  shift = 0x7f + 21;
  if (!((expon << 4) + index)) {
    poly64 <<= 7;
    shift = 7 + 0x7f + 21;
    // is x exactly 1.0?
    if (!R)
      return sgn_x | 0x3f800000;
  }
  // T+R*poly, scale 2^(2+32+(shift-bias))
  poly_low = (unsigned int)poly64;
  poly_h = (unsigned int)(poly64 >> 32);
  poly64 = (((long)((int)(poly_h))) * ((int)(R))) + __spow_ha___log2_tbl[index];
  // adjust for sign of poly_low
  poly_low = (((unsigned int)(poly_low)) >> (1));
  poly_low = ((((long)((int)(poly_low))) * ((int)(R))) >> 32);
  poly_low += poly_low;
  poly64 += (long)((int)poly_low);
  // log2(x) ~ expon+T+R*poly, sc 2^(2+32+(shift-bias))
  expon <<= 23;
  exp64 = (unsigned long)expon;
  poly64 += (exp64 << 32);
  poly_s1 = poly64 << 1;
  while (poly_s1 && (((long)(poly_s1 ^ poly64)) >= 0)) {
    poly64 = poly_s1;
    poly_s1 <<= 1;
    shift++;
  }
  // y, sc 2^(30-expon_y)
  // unpack mantissa, biased exponent
  expon_y = shift - ((abs_y) >> 23);
  mant = ((abs_y)&0x7fffff);
  // denormal y?
  if (abs_y < 0x00800000)
    expon_y = shift - 1;
  else
    mant |= 0x00800000;
  // apply sign to mantissa bits
  mant = (mant ^ sgn_y) - sgn_y;
  // mant, scale 2^30
  mant <<= 7;
  // y*log2(x), sc 2^(2+30 + expon_y)
  poly_low = (unsigned int)poly64;
  poly_h = (unsigned int)(poly64 >> 32);
  poly64 = (((long)((int)(poly_h))) * ((int)(mant)));
  // adjust for sign of poly_low
  poly_low = (((unsigned int)(poly_low)) >> (1));
  poly_low = ((((long)((int)(poly_low))) * ((int)(mant))) >> 32);
  poly_low += poly_low;
  poly64 += (long)((int)poly_low);
  if (expon_y < 0) // overflow/underflow
  {
    poly_h = (unsigned int)(poly64 >> 32);
    if (((int)poly_h) < 0)
      goto POWF_UF;
    goto POWF_OF;
  }
  if (expon_y >= 32) {
    expon_y -= 32;
    poly64 = (((long)(poly64)) >> (32));
    if (expon_y >= 32)
      return sgn_x | 0x3f800000;
  }
  // integer part in high 32 bits, fractional bits in low part
  poly64 = (((long)(poly64)) >> (expon_y));
  N = (unsigned int)(poly64 >> 32);
  // reduced exp2 argument, sc 2^32
  R = (unsigned int)poly64;
  // (c7*R+c6)*2^32
  poly = ((((unsigned long)((unsigned int)(__spow_ha___sc7))) *
           ((unsigned int)(R))) >>
          32);
  poly = poly + __spow_ha___sc6;
  // poly*2^32
  poly =
      ((((unsigned long)((unsigned int)(poly))) * ((unsigned int)(R))) >> 32);
  poly = poly + __spow_ha___sc5;
  // poly*2^32
  poly =
      ((((unsigned long)((unsigned int)(poly))) * ((unsigned int)(R))) >> 32);
  poly = poly + __spow_ha___sc4;
  // poly*2^32
  poly =
      ((((unsigned long)((unsigned int)(poly))) * ((unsigned int)(R))) >> 32);
  poly = poly + __spow_ha___sc3;
  // poly*2^32
  poly =
      ((((unsigned long)((unsigned int)(poly))) * ((unsigned int)(R))) >> 32);
  poly = poly + __spow_ha___sc2;
  // poly*2^32
  poly =
      ((((unsigned long)((unsigned int)(poly))) * ((unsigned int)(R))) >> 32);
  poly = poly + __spow_ha___sc1;
  // poly*2^32
  poly =
      ((((unsigned long)((unsigned int)(poly))) * ((unsigned int)(R))) >> 32);
  // rounding and overflow/underflow checking
  // poly*2^31
  poly = (((unsigned int)(poly)) >> (1)) + 128;
  expon = N + 0x7f;
  N = expon + (((unsigned int)(poly)) >> (31));
  // overflow?
  if (N >= 0xff)
    goto POWF_OF;
  // underflow, possibly gradual?
  if (N <= 0)
    goto POWF_GRAD_UF;
  res = sgn_x | ((expon << 23) + (((unsigned int)(poly)) >> (8)));
  return res;
POWF_OF:
  res = sgn_x | 0x7f800000;
  *errcode = 3;
  return res; // goto POWF_ERRCALL;
POWF_GRAD_UF:
  if (N < -24)
    goto POWF_UF;
  // poly*2^31, undo rounding to 24 bits
  poly = poly + 0x80000000 - 128;
  N = expon;
  while (N < 1) {
    poly = (((unsigned int)(poly)) >> (1));
    N++;
  }
  poly = (((unsigned int)(poly + 128)) >> (8));
  if (poly)
    return sgn_x | poly;
POWF_UF:
  res = sgn_x;
  *errcode = 4;
  return res; // goto POWF_ERRCALL;
SPECIAL_Y:
  // 0, Inf, NaN
  // 0?
  if (!abs_y)
    return 0x3f800000;
  // NaN?
  if (abs_y > 0x7f800000)
    return ((xin == 0x3f800000) ? xin : 0xffc00000);
  // +/-Inf
  // x is NaN?
  if (((unsigned int)(xin + xin)) > 0xff000000u)
    return 0xffc00000;
  // |x| == 1?
  R = (xin & 0x7fffffff) - 0x3f800000;
  if (R == 0)
    return 0x3f800000;
  R ^= sgn_y;
  if (((int)R) < 0)
    return 0;
  res = 0x7f800000;
  if (!(xin + xin)) {
    *errcode = 1;
  }
  return res;
SPECIAL_X:
  p_inf = 0x7f800000;
  // +Inf?
  if (xin == p_inf)
    return (sgn_y ? 0 : xin);
  // NaN
  if (((unsigned int)(xin + xin)) > 0xff000000u)
    return 0xffc00000;
  if (((int)xin) > 0) {
  DENORM_X:
    // denormal input, normalize
    expon = 1 - 0x7f;
    while (mant < 0x00800000) {
      expon--;
      mant <<= 1;
    }
    // return to main computation
    goto LOGF_MAIN;
  }
  // is y an integer?
  is_int = 0;
  if (abs_y >= 0x3f800000) {
    if (abs_y >= 0x4b800000)
      is_int = 1; // and even integer (>=2^24)
    else {
      shift = 23 + 0x7f - (((unsigned int)(abs_y)) >> (23));
      mant_y = ((abs_y)&0x7fffff) | 0x00800000;
      mi_y = (((unsigned int)(mant_y)) >> (shift));
      if (mant_y == (mi_y << shift)) {
        is_int = 1;
        // set sign for odd integers
        sgn_x = mi_y << 31;
      }
    }
  }
  // +/-zero?
  if (!(xin + xin)) {
    if (!sgn_y)
      return 0;
    sgn_x &= xin;
    res = sgn_x | 0x7f800000;
    *errcode = 1;
    return res; // goto POWF_ERRCALL;
  }
  // negative?
  if (((int)xin) < 0) {
    if (xin == 0xff800000)
      return (sgn_y ? sgn_x : (sgn_x | 0x7f800000));
    if (!is_int) {
      *errcode = 1;
      res = 0xffc00000;
      return res; // goto POWF_ERRCALL;
    }
    expon -= 0x100;
    if (xin == 0xbf800000)
      return sgn_x | 0x3f800000;
    if (expon >= -126)
      goto LOGF_MAIN;
    goto DENORM_X;
  }
  return xin;
}
static __constant int_float __spow_ha_c4 = {0xbeb8aa69u};
static __constant int_float __spow_ha_c3 = {0x3ef63874u};
static __constant int_float __spow_ha_c2 = {0xbf38aa3bu};
static __constant int_float __spow_ha_c1 = {0x32a56f38u};
static __constant int_float __spow_ha_c1h = {0x3fb8aa3bu};
// Th, Tl
static __constant int_float __spow_ha_log_tbl[] = {
    0xc2fc0000u, 0x00000000u, 0xc2fc02e2u, 0x36c77251u, 0xc2fc05c0u,
    0x3657a488u, 0xc2fc089cu, 0x36e7742bu, 0xc2fc0b74u, 0x35d2f47au,
    0xc2fc0e4au, 0x361c457bu, 0xc2fc111cu, 0xb6d1d513u, 0xc2fc13eeu,
    0x36cf6b98u, 0xc2fc16bau, 0xb6d3758fu, 0xc2fc1986u, 0x3600796du,
    0xc2fc1c4eu, 0x3428deabu, 0xc2fc1f14u, 0x366cf99bu, 0xc2fc21d6u,
    0xb6627e8au, 0xc2fc2496u, 0xb6b6fc27u, 0xc2fc2754u, 0xb63f74a0u,
    0xc2fc2a10u, 0x368f93fau, 0xc2fc2cc8u, 0x3510536fu, 0xc2fc2f7eu,
    0x358b4a3bu, 0xc2fc3232u, 0x36bd2374u, 0xc2fc34e2u, 0xb58ef9a4u,
    0xc2fc3790u, 0xb684815bu, 0xc2fc3a3cu, 0xb6538d6eu, 0xc2fc3ce6u,
    0x359f7403u, 0xc2fc3f8cu, 0xb6d41e16u, 0xc2fc4232u, 0x369dcc96u,
    0xc2fc44d4u, 0x36725bb8u, 0xc2fc4774u, 0x36b9a2abu, 0xc2fc4a10u,
    0xb6a52c1fu, 0xc2fc4cacu, 0x3630ec3bu, 0xc2fc4f44u, 0xb6236cd4u,
    0xc2fc51dau, 0xb6a7e610u, 0xc2fc546eu, 0xb6ae41e9u, 0xc2fc5700u,
    0xb651cfdfu, 0xc2fc5990u, 0x3590536fu, 0xc2fc5c1eu, 0x36f4ac44u,
    0xc2fc5ea8u, 0x341c17d1u, 0xc2fc6130u, 0xb6af40bcu, 0xc2fc63b8u,
    0x36d45f2bu, 0xc2fc663cu, 0x368be206u, 0xc2fc68beu, 0x3666e6fau,
    0xc2fc6b3eu, 0x3687492cu, 0xc2fc6dbcu, 0x36c3a282u, 0xc2fc7036u,
    0xb6db3765u, 0xc2fc72b0u, 0xb631e3b8u, 0xc2fc7528u, 0x360d9e6du,
    0xc2fc779cu, 0xb6ff8ec5u, 0xc2fc7a10u, 0xb5be74e7u, 0xc2fc7c82u,
    0x36b31b7eu, 0xc2fc7ef0u, 0xb635c813u, 0xc2fc815eu, 0x36a2ed98u,
    0xc2fc83c8u, 0xb62dbb03u, 0xc2fc8632u, 0x36b4555fu, 0xc2fc8898u,
    0xb5fb5a61u, 0xc2fc8afeu, 0x36cc4a28u, 0xc2fc8d60u, 0xb5b6523eu,
    0xc2fc8fc2u, 0x36d0493cu, 0xc2fc9220u, 0xb5f561c1u, 0xc2fc947eu,
    0x36a65267u, 0xc2fc96d8u, 0xb687e26du, 0xc2fc9932u, 0x35d3929bu,
    0xc2fc9b8au, 0x36d98adau, 0xc2fc9ddeu, 0xb69d050fu, 0xc2fca032u,
    0xb5c76361u, 0xc2fca284u, 0x354048acu, 0xc2fca4d4u, 0x35f6865du,
    0xc2fca722u, 0x35efe2e1u, 0xc2fca96eu, 0x3500ecdau, 0xc2fcabb8u,
    0xb611b338u, 0xc2fcae00u, 0xb6d1cfdfu, 0xc2fcb048u, 0x3664bd6bu,
    0xc2fcb28cu, 0xb67e4678u, 0xc2fcb4d0u, 0x362db733u, 0xc2fcb712u,
    0x36f19318u, 0xc2fcb950u, 0xb6b1be5eu, 0xc2fcbb8eu, 0xb695d3f8u,
    0xc2fcbdcau, 0xb6bd628du, 0xc2fcc006u, 0x36d4e74fu, 0xc2fcc23eu,
    0x357309e8u, 0xc2fcc474u, 0xb6e39706u, 0xc2fcc6aau, 0xb5ce76b8u,
    0xc2fcc8deu, 0x35aedc1du, 0xc2fccb10u, 0x35df5b18u, 0xc2fccd40u,
    0xb50e4789u, 0xc2fccf6eu, 0xb6b3acd9u, 0xc2fcd19cu, 0x361f580au,
    0xc2fcd3c8u, 0x36f5c842u, 0xc2fcd5f0u, 0xb6c3cbf9u, 0xc2fcd818u,
    0xb6df7f03u, 0xc2fcda40u, 0x36a0463cu, 0xc2fcdc64u, 0xb60dbf88u,
    0xc2fcde88u, 0x36516187u, 0xc2fce0aau, 0x36ac9edau, 0xc2fce2cau,
    0x368296b5u, 0xc2fce4e8u, 0xb53db292u, 0xc2fce706u, 0x36db6e2du,
    0xc2fce920u, 0xb6a64060u, 0xc2fceb3au, 0xb69f0197u, 0xc2fced54u,
    0x36eeefceu, 0xc2fcef6au, 0x332ef727u, 0xc2fcf180u, 0x369617b4u,
    0xc2fcf394u, 0x36aaf0c5u, 0xc2fcf5a6u, 0x35f705d5u, 0xc2fcf7b6u,
    0xb6b39947u, 0xc2fcf9c6u, 0xb5acf799u, 0xc2fcfbd4u, 0xb5ad1961u,
    0xc2fcfde0u, 0xb6b5c813u, 0xc2fcffecu, 0x35ccaf80u, 0xc2fd01f6u,
    0x368d88dau, 0xc2fd03feu, 0x362e8d0du, 0xc2fd0604u, 0xb6633e4au,
    0xc2fd080au, 0x35c36024u, 0xc2fd0a0eu, 0x35f2c1c8u, 0xc2fd0c10u,
    0xb6201ac7u, 0xc2fd0e12u, 0x3688ab28u, 0xc2fd1012u, 0x36c4eac3u,
    0xc2fd1210u, 0x36458c3du, 0xc2fd140cu, 0xb69faa1eu, 0xc2fd1608u,
    0xb60892bcu, 0xc2fd1802u, 0xb68cf729u, 0xc2fd19fcu, 0x368470ceu,
    0xc2fd1bf4u, 0x36ee16a3u, 0xc2fd1deau, 0x36ae2634u, 0xc2fd1fdeu,
    0xb5f4c3a0u, 0xc2fd21d2u, 0x35a9124bu, 0xc2fd23c4u, 0xb56a1394u,
    0xc2fd25b6u, 0x36ea748cu, 0xc2fd27a4u, 0xb6c0585du, 0xc2fd2994u,
    0x36e0986fu, 0xc2fd2b80u, 0xb5d1cfdfu, 0xc2fd2d6cu, 0xb31a809cu,
    0xc2fd2f56u, 0xb6875eb4u, 0xc2fd3140u, 0x35dd7932u, 0xc2fd3328u,
    0x35e552d9u, 0xc2fd350eu, 0xb6832570u, 0xc2fd36f4u, 0x32124dd4u,
    0xc2fd38d8u, 0xb5f80bd7u, 0xc2fd3abcu, 0x36c055feu, 0xc2fd3c9eu,
    0x36f9b6f5u, 0xc2fd3e7eu, 0x36590c12u, 0xc2fd405cu, 0xb6e8d38fu,
    0xc2fd423cu, 0x36f81679u, 0xc2fd4418u, 0x34db37dfu, 0xc2fd45f4u,
    0x362cebbfu, 0xc2fd47ceu, 0xb5bcb409u, 0xc2fd49a8u, 0x3676865du,
    0xc2fd4b80u, 0x3628836du, 0xc2fd4d56u, 0xb6a5b33du, 0xc2fd4f2cu,
    0xb668353eu, 0xc2fd5102u, 0x36e78f75u, 0xc2fd52d4u, 0xb6942e48u,
    0xc2fd54a6u, 0xb6e8ca54u, 0xc2fd5678u, 0xb53dc358u, 0xc2fd5848u,
    0xb589a627u, 0xc2fd5a18u, 0x36f5b407u, 0xc2fd5be4u, 0xb6d0cb51u,
    0xc2fd5db2u, 0x3688af7cu, 0xc2fd5f7cu, 0xb6ff41e1u, 0xc2fd6148u,
    0x3695fce5u, 0xc2fd6310u, 0xb6b8f553u, 0xc2fd64d8u, 0xb6ed771au,
    0xc2fd66a0u, 0xb48e4789u, 0xc2fd6866u, 0xb4c96f70u, 0xc2fd6a2au,
    0xb6f9e5a1u, 0xc2fd6beeu, 0xb6d2250du, 0xc2fd6db2u, 0x3652b13du,
    0xc2fd6f74u, 0x36b743a4u, 0xc2fd7134u, 0x3532754au, 0xc2fd72f4u,
    0x36852eb4u, 0xc2fd74b2u, 0x33a6c7e3u, 0xc2fd7670u, 0x368d4d9cu,
    0xc2fd782cu, 0x35900896u, 0xc2fd79e8u, 0x36c57700u, 0xc2fd7ba2u,
    0x3660d960u, 0xc2fd7d5au, 0xb6dc5a08u, 0xc2fd7f14u, 0x36dde7d3u,
    0xc2fd80cau, 0xb6440d2au, 0xc2fd8280u, 0xb69d5be7u, 0xc2fd8436u,
    0x35aacd72u, 0xc2fd85eau, 0xb4b0b27bu, 0xc2fd879eu, 0x36c0384cu,
    0xc2fd8950u, 0x368b4f91u, 0xc2fd8b00u, 0xb6aaf16du, 0xc2fd8cb0u,
    0xb6e3b4b4u, 0xc2fd8e60u, 0xb58088f6u, 0xc2fd900eu, 0xb642c000u,
    0xc2fd91bcu, 0x362edc1du, 0xc2fd9368u, 0x34925f55u, 0xc2fd9514u,
    0x36b2a7b2u, 0xc2fd96beu, 0x3625aa69u, 0xc2fd9868u, 0x36e8925cu,
    0xc2fd9a10u, 0x366589e4u, 0xc2fd9bb8u, 0x36f054a3u, 0xc2fd9d5eu,
    0x364055feu, 0xc2fd9f04u, 0x36c13371u, 0xc2fda0a8u, 0x3512d9f7u,
    0xc2fda24cu, 0x36252301u, 0xc2fda3eeu, 0xb67e71c3u, 0xc2fda590u,
    0xb6482263u, 0xc2fda732u, 0x36a2fd0eu, 0xc2fda8d2u, 0x3694e81eu,
    0xc2fdaa70u, 0xb68f5801u, 0xc2fdac0eu, 0xb6cac991u, 0xc2fdadacu,
    0xb57388e4u, 0xc2fdaf48u, 0xb68b5179u, 0xc2fdb0e4u, 0xb5135e64u,
    0xc2fdb27eu, 0xb6b4bf8fu, 0xc2fdb418u, 0xb6669520u, 0xc2fdb5b2u,
    0x36b0f707u, 0xc2fdb74au, 0x36b70aadu, 0xc2fdb8e0u, 0xb644110eu,
    0xc2fdba76u, 0xb69b39e5u, 0xc2fdbc0cu, 0x34a80dcau, 0xc2fdbda0u,
    0xb66398d0u, 0xc2fdbf34u, 0xb508a232u, 0xc2fdc0c6u, 0xb6d447c6u,
    0xc2fdc258u, 0xb6bc5665u, 0xc2fdc3eau, 0x35d74798u, 0xc2fdc57au,
    0x3322524du, 0xc2fdc70au, 0x36a516e2u, 0xc2fdc898u, 0x35817c93u,
    0xc2fdca26u, 0x36646adau, 0xc2fdcbb2u, 0xb64c99a0u, 0xc2fdcd3eu,
    0xb6541951u, 0xc2fdcecau, 0x364c2476u, 0xc2fdd054u, 0x3492d9f7u,
    0xc2fdd1deu, 0x367cc968u, 0xc2fdd366u, 0xb5ec9fccu, 0xc2fdd4eeu,
    0xb5915972u, 0xc2fdd676u, 0x36c1fcd1u, 0xc2fdd7fcu, 0x366dec9bu,
    0xc2fdd982u, 0x36f9bc2au, 0xc2fddb06u, 0x3612eba1u, 0xc2fddc8au,
    0x364a9801u, 0xc2fdde0cu, 0xb6b39734u, 0xc2fddf90u, 0x36fdf795u,
    0xc2fde110u, 0xb686daf7u, 0xc2fde292u, 0x36bd1f20u, 0xc2fde412u,
    0x36c9151bu, 0xc2fde590u, 0xb6479104u, 0xc2fde70eu, 0xb6ca47f0u,
    0xc2fde88cu, 0xb6566c4du, 0xc2fdea0au, 0x36b8a11cu, 0xc2fdeb86u,
    0x36a07357u, 0xc2fded00u, 0xb6b488bfu, 0xc2fdee7cu, 0x36b8e4c5u,
    0xc2fdeff6u, 0x36e7f503u, 0xc2fdf16eu, 0xb5a076deu, 0xc2fdf2e6u,
    0xb6702fd7u, 0xc2fdf45eu, 0xb48bcf06u, 0xc2fdf5d4u, 0xb6dacef3u,
    0xc2fdf74au, 0xb6ef0efbu, 0xc2fdf8c0u, 0xb60c79d2u, 0xc2fdfa34u,
    0xb6e1177cu, 0xc2fdfba8u, 0xb6c05c54u, 0xc2fdfd1cu, 0x3559c06bu,
    0xc2fdfe8eu, 0xb61e2b7au, 0xc2fe0000u, 0x80000000u,
};
static __constant int_float __spow_ha_ec6 = {0x39224c80u};
static __constant int_float __spow_ha_ec5 = {0x3aafa463u};
static __constant int_float __spow_ha_ec4 = {0x3c1d94cbu};
static __constant int_float __spow_ha_ec3 = {0x3d635766u};
static __constant int_float __spow_ha_ec2 = {0x3e75fdf1u};
static __constant int_float __spow_ha_ec1 = {0x3e45c862u};
__attribute__((always_inline)) inline int
__ocl_svml_internal_spow_ha(float *pxin, float *pyin, float *pres) {
  int nRet = 0;
  float xin = *pxin, yin = *pyin;
  int_float x, y, mant_x, rcp, iylx, Th, Tl;
  int index;
  float High, High2, ylx_h;
  float R, expon_x, poly, Low, c1hR_h, c1hR_l;
  float ylx_l, poly_h;
  float fN, Re, Rh, Rl;
  int sN;
  unsigned int N, iexpon_x, iexpon_ylx;
  int_float T, res;
  x.f = xin;
  y.f = yin;
  // 2^(-9)*mantissa
  mant_x.w = (x.w & 0x007fffffu) | 0x3b000000u;
  // extract exponent(x)
  iexpon_x = x.w >> 23;
  expon_x = (float)(iexpon_x);
  rcp.f = 1.0f / (mant_x.f);
  // round to rcp to 1+5 mantissa bits
  rcp.f = SPIRV_OCL_BUILTIN(rint, _f32, )(rcp.f);
  // table index
  index = ((rcp.w >> (23 - 8 - 1)) + 0x200) & 0x3fe;
  // reduced argument
  R = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(rcp.f, mant_x.f, (-1.0f));
  // expon_x + Th, exact
  Th.w = __spow_ha_log_tbl[index].w;
  Th.f += expon_x;
  // Tl
  Tl.w = __spow_ha_log_tbl[index + 1].w;
  // polynomial + Tl
  poly =
      SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(__spow_ha_c4.f, R, __spow_ha_c3.f);
  poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(poly, R, __spow_ha_c2.f);
  poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(poly, R, __spow_ha_c1.f);
  poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(poly, R, Tl.f);
  // (Th+expon) + (c1h*R)_high
  High = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(__spow_ha_c1h.f, R, Th.f);
  // (c1h*R)_high
  c1hR_h = High - Th.f;
  // (c1h*R)_low
  c1hR_l =
      SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(__spow_ha_c1h.f, R, (-c1hR_h));
  High2 = poly + High;
  poly_h = High2 - High;
  poly = poly - poly_h;
  Low = poly + c1hR_l;
  // y*log2(x)
  iylx.f = ylx_h = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(y.f, High2, 0.0f);
  ylx_l = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(y.f, High2, (-ylx_h));
  ylx_l += SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(y.f, Low, 0.0f);
  // redirect special cases
  iexpon_x--;
  iylx.w &= 0x7fffffffu;
  if ((iexpon_x >= 0xfe) || (iylx.w >= 0x42FB8000))
    goto POWF_SPECIAL;
  // exp2 computation
  fN = SPIRV_OCL_BUILTIN(rint, _f32, )(ylx_h);
  Re = ylx_h - fN;
  Re = Re + ylx_l;
  sN = (int)fN;
  // exponent
  N = sN;
  poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(__spow_ha_ec6.f, Re,
                                                __spow_ha_ec5.f);
  // 1+0.5*Re
  High = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(Re, 0.5f, 1.0f);
  poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(poly, Re, __spow_ha_ec4.f);
  // (0.5*R)_high
  Rh = High - 1.0f;
  poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(poly, Re, __spow_ha_ec3.f);
  // (0.5*R)_low
  Rl = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(Re, 0.5f, (-Rh));
  poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(poly, Re, __spow_ha_ec2.f);
  poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(poly, Re, __spow_ha_ec1.f);
  poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(poly, Re, Rl);
  res.f = High + poly;
  res.w += (N << 23);
  *pres = res.f;
  return nRet;
POWF_SPECIAL:
  res.w = __spow_ha_powf_cout(x.w, y.w, &nRet);
  *pres = res.f;
  return nRet;
}
float __ocl_svml_powf_ha(float x, float y) {
  float r;
  __ocl_svml_internal_spow_ha(&x, &y, &r);
  return r;
}

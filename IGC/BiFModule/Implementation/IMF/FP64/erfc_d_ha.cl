/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
/*
// ALGORITHM DESCRIPTION:
//
//   CONVENTIONS
//   A  = B denotes that A is equal to B.
//   A := B denotes assignment of a value of the expression B to
//          the variable A. All operations and roundings in B are considered
//          to be in the target precision.
//
//   HIGH LEVEL OVERVIEW
//
//   We use a table lookup method to compute erfc(|a[0]|). We split the input
//   range into a number of subintervals and approximate erfc(|a[0]|) by a
//   polynomial or a polynomial multiplied by exp(-|a[0]|^2) on each of the
//   subintervals. We use special range reduction procedure which allows
//   the use of polynomial approximation for exp(-|a[0]|^2).
//
//   On negative arguments we calculate erfc(a[0]) = 2.0 - erfc(|a[0]|)
//
//   IEEE SPECIAL CONDITIONS:
//   a[0] = +Inf,   r[0] = +0.0
//   a[0] = -Inf,   r[0] = 2.0
//   a[0] = QNaN,   r[0] = QNaN
//   a[0] = SNaN,   r[0] = QNaN
//
//   UNDERFLOW
//
//   erfc(x) = 0.0 in round-to-nearest mode if and only if
//    x >= UNDERFLOW_THRESHOLD.
//
//   erfc(x) produces denormalized result in round-to-nearest mode
//   if and only if
//    GRADUAL_UNDERFLOW_THRESHOLD <= x < UNDERFLOW_THRESHOLD
//
//   ALGORITHM DETAILS
//   A careful algorithm must be used to realize the mathematical ideas
//   accurately. In addition a number of execution paths required to
//   handle special and subtle cases. Below we describe each execution path.
//
//   1) a[0] = [S,Q]NaN or [+,-]Infinity
//
//       1.1) a[0] = [S,Q]NaN
//
//          r[0] := a[0] * a[0]
//
//       1.2) a[0] = -Infinity
//
//          r[0] := 2.0
//
//       1.3) a[0] = +Infinity
//
//          r[0] := +0.0
//
//   2) |a[0]| < 2^(NEAR_ZERO_THRESHOLD_EXP)
//      In this particular implementation NEAR_ZERO_THRESHOLD_EXP = -70
//
//       r[0] := 1.0 + a[0]
//
//      NOTE: here 1.0 + a[0] rounds to 1.0 in round-to-nearest mode,
//            this addition raises an inexact flag
//
//   3) -Infinity < a[0] <= SATURATION_THRESHOLD,
//      In this particular implementation
//      SATURATION_THRESHOLD ~= -5.863584748755168
//
//       r[0] := 2.0 - TINY
//
//      NOTE: TINY ~= +2.225073858507201877156e-308 in this particular
//            implementation. 2.0 - TINY rounds to 2.0 in round-to-nearest
//            mode. This computation raises an inexact flag
//
//   4) UNDERFLOW_THRESHOLD <= a[0] < +Infinity,
//      In this particular implementation
//      UNDERFLOW_THRESHOLD ~= +2.722601711110836575358e+01
//      Here erfc(a[0]) underflows, so
//
//       r[0] := TINY * TINY
//
//      NOTE: TINY ~= +2.225073858507201877156e-308 in this particular
//            implementation. TINY * TINY rounds to 0.0 in round-to-nearest
//            mode. This computation raises inexact and underflow flags.
//            In addition error handling routine is called
//            with IML_STATUS_UNDERFLOW
//
//   5) Main path. SATURATION_THRESHOLD < a[0] < UNDERFLOW_THRESHOLD, and
//      |a[0]| >= 2^(NEAR_ZERO_THRESHOLD_EXP)
//
//      If a[0] is negative we compute 2.0 - erfc(|a[0]|). So we compute
//      erfc(|a[0]|) first.
//
//      The idea of computing erfc(|a[0]|) in this range is to calculate
//      a polynomial of |a[0]| and then multiply it by exp(-|a[0]|^2) if
//      needed.
//
//      We split interval [2^(NEAR_ZERO_THRESHOLD_EXP), UNDERFLOW_THRESHOLD)
//      into 20 subintervals in the way explain below.
//      On the first 6 of them we represent erfc(|a[0]|) as
//
//       erfc(|a[0]|) ~= Poly(|a[0]|),
//
//      on the rest of subintervals we represent erfc(|a[0]|) as
//
//       erfc(|a[0]|) ~= Poly(|a[0]|) * exp(-|a[0]|^2),
//
//      here Poly() is a 15-th degree polynomial. Coefficients are stored
//      in table.
//
//      Let x(j) = -1.0 + 2^(j / 4), j = 0,...19. So we have 20 unequal
//      argument intervals [x(j), x(j + 1)] with length ratio q = 2^(1/4).
//
//      Let |a[0]| fall into interval x(index) <= |a[0]| < x(index + 1).
//      We get index value as the exponent of number (|a[0]| + 1)^4.
//
//      Next we use argument range reduction
//
//       y := |a[0]| + B
//
//      Here B = - ( x(index+1) + x(index) )/2 is also stored in table. This
//      range reduction procedure moves argument closer to zero.
//
//      NOTE: we use multiprecision technique here to perform the exact
//            summation. We store the result as a pair of working precision
//            numbers y and yMid, so that
//
//                y + yMid = |a[0]| + B
//
//      Then we compute the polynomial of |a[0]| + B. Here we also use
//      multiprecision calculations to obtain more accurate result. We get
//
//       resHi + resLo := Poly(|a[0]|)
//
//      If |a[0]| lies outside the first 6 subintervals we proceed with the
//      exp(-|a[0]|^2) computation (see path 5.2 below), else we leave resHi
//      and resLo unchanged till step 5.3, 5.4 or 5.5
//
//      5.1) if index < 6 then we don't need to calculate exp(-|a[0]|^2) so
//           we initialize
//
//            scale := 1.0
//
//      5.2) if index >= 6 we compute exp(-|a[0]|^2) * Poly(|a[0]|)
//           The basic idea is that in order to compute exp(x), we accurately
//           decompose x into
//
//            x = M * ln(2)/(2^K) + R, |R| <= ln(2)/2^(K + 1), M is integer.
//
//           In this particular implementation K = 6.
//
//           Hence exp(x) = 2^(M/2^K) * exp(R).
//           The value 2^(M/2^K) is obtained by simple combinations of values
//           calculated beforehand and stored in table; exp(R) is approximated
//           by a short polynomial because |R| is small.
//
//           We elaborate this method in 3 steps.
//
//           a) Range Reduction:
//              The value 2^K / ln(2.0) is stored in table as a working
//              precision number TWO_TO_THE_K_DIV_LN2.
//
//              w := x * TWO_TO_THE_K_DIV_LN2
//              M := ROUND_TO_NEAREST_INTEGER(w)
//
//              The value ln(2.0) / 2^K is stored as two numbers LOG_HI and
//              LOG_LO so that R can be computed accurately via
//
//              R := (x - RHi) - RLo = (x - M * LOG_HI) - M * LOG_LO
//
//              We pick LOG_HI such that M * LOG_HI is exactly representable in
//              working precision and thus the operation x - M * LOG_HI is error
//              free. In particular, LOG_HI has 17 trailing bits in significand
//              set to 0. 17 is sufficient number since K = 6, and thus M
//              has no more than 17 non-zero digits.
//              LOG_LO is (rounded to working precision) difference between
//              ln(2.0) / 2^K and LOG_HI.
//
//              NOTE: instead of explicit conversions integer <-> double,
//                    we use so called Right Shifter technique as follows:
//
//                     w  = x * TWO_TO_THE_K_DIV_LN2
//                     Nj = w + RS_EXP,
//
//                    where double precision RS_EXP = 2^52 + 2^51 is the value
//                    of "Right Shifter". 32 least significant bits of Nj's
//                    significand contain the value of M. Then the following
//                    operation allows to obtain M in double precision number w:
//
//                     w = Nj - RS_EXP
//
//                    Since some compilers might "optimize away" the sequence
//                    w = (w + RS_EXP) - RS_EXP
//                    by eliminating addition and subtraction, it makes sense
//                    to declare w and Nj with volatile qualifier.
//
//           b) Approximation:
//              exp(R) - 1 is approximated by a short polynomial of the form
//                  p = R + EXP_POLY2 * R^2 + ... + EXP_POLY6 * R^6,
//              Polynomial coefficients EXP_POLY2, ..., EXP_POLY6 are stored
//              in table.
//
//           c) Final reconstruction:
//              exp(x) ~= exp(M * ln(2)/(2^K) + R) = 2^(M/2^K) * exp(R) ~=
//                     ~= 2^(M/2^K) * (1 + p)
//
//              The value 2^(M/2^K) can be composed in the following way.
//              First, express M as two integers N and j as
//
//               M = N * 2^K + j
//
//              where 0 <= j < 2^K and N can be positive or negative. When N is
//              represented in 2's complement form, j is simply the K least
//              significant bits and N is simply M shifted right arithmetically
//              by K bits.
//
//              Now, 2^(M/2^K) is simply 2^N * 2^(j/2^K).
//              2^N (let us call it scale) needs no tabulation and can be easily
//              constructed. 2^(j/2^K) we store accurately with the precision
//              wider than the working one. We use the following method:
//
//               T[j] ~= 2^(j/2^K), we leave only half of the leading bits in
//                       T[j]'s significand, so T[j] * T[j] is exactly
//                       representable in working precision
//               D[j] := 2^(j/2^K) - T[j] rounded to working precision.
//
//              Thus, for each j we tabulate T[j] and D[j]. The sum T[j] + D[j]
//              represents 2^(j/2^K) with the precision wider than working one.
//
//              Then, the reconstruction step looks as follows:
//              exp(x) ~= 2^N * 2^(j/2^K) * (1 + p)
//                      = scale * 2^(j/2^K) * (1 + p)
//                     ~= scale * (T[j] + D[j]) * (1 + p)
//                      = scale * (T[j] + D[j] + p * (T[j] + D[j]))
//
//           So we apply steps a) - c) to x ~= -|a[0]|^2 ~= a2Hi + a2Lo
//           and obtain exp(-|a[0]|^2) in a pair of numbers expHi and expLo. We
//           only do not multiply by scale, leaving this for the later steps.
//           Next we reconstruct Poly(|a[0]|) * (expHi + expLo):
//
//             resHi + resLo ~= (resHi + resLo) * (expHi + expLo)
//
//           The outcome of the step 5.2 is a pair resHi, resLo and the
//           scale = 2^N.
//
//           We proceed with the steps 5.3, 5.4 or 5.5, depending on a[0].
//
//      If a[0] is positive we have to care about gradual underflow
//      ( erfc(a[0]) is denormalized). See path 5.4
//      For negative arguments we proceed with path 5.5
//
//      5.3) a[0] < GRADUAL_UNDERFLOW_THRESHOLD,
//           in this particular implementation
//           GRADUAL_UNDERFLOW_THRESHOLD ~= +2.654325845425098151509e+01
//
//           If here, erfc(a[0]) is normalized and we simply return
//
//           r[0] := (resHi + resLo) * scale
//
//      5.4) a[0] >= GRADUAL_UNDERFLOW_THRESHOLD
//
//           If here, erfc(a[0]) is denormalized and we consider two cases
//           5.4.1 and 5.4.2.
//
//           The idea is to modify the final step
//           r[0] := (resHi + resLo) * scale, since the last multiplication
//           introduces an error, despite the fact that scale is a power of 2.
//
//           We consrtuct new scale multiplier:
//
//            scale = 2^(N + SCALE_EXP),
//
//           where N is defined in 5.2 path (we already performed
//           exp(-a[0]^2) computation if a[0] >= GRADUAL_UNDERFLOW_THRESHOLD
//           since index here equals 19 > 6); and SCALE_EXP = 200 is needed
//           to deal with normalized numbers, since exp(-a[0]^2) may gradually
//           underflow if calculated in working precision.
//           So
//
//            erfc(a[0]) ~= UNSCALE * ((resHi + resLo) * scale),
//
//           here UNSCALE = 2^(-200).
//
//           First we scale up resHi and resLo:
//
//            resHi = resHi * scale
//            resLo = resLo * scale
//
//           NOTE: these are exact computations
//
//           Due to gradual underflow the multiplication on UNSCALE (despite
//           the fact that this value is exact power of 2) introduces an error.
//           The closer final result is to normalized number the bigger that
//           error might be. To address this issue we separate two cases:
//           erfc(a[0]) is either a "small" or "large" denormalized number.
//
//           5.4.1) if a[0] > LARGE_DENORM_THRESHOLD
//                  in this particular implementation
//                  LARGE_DENORM_THRESHOLD ~=+2.669937226834560206612e+01
//
//                  Here erfc(a[0]) is a "small" denormalized number
//                  and we simply return
//
//                  r[0] := (resHi + resLo) * UNSCALE
//
//           5.4.2) if a[0] <= LARGE_DENORM_THRESHOLD
//                  Here erfc(a[0]) is a "large" denormalized number
//
//                  We split the sum resHi + resLo into three numbers
//                  resHi, resMid, resLo, so that resHi * UNSCALE is
//                  computed exactly. Then we sum resLo = resLo + resMid.
//
//                  Next we scale up the two parts:
//
//                   v1 := resHi * UNSCALE
//                   v2 := resLo * UNSCALE
//
//                  and only now accumulate the result in the final summation:
//
//                   r[0] := v1 + v2
//
//                  NOTE: v1 and v2 are recommended to be declared with volatile
//                        qualifier to avoid possible compiler optimisations in
//                        the last computations.
//
//      5.5) if a[0] is negative
//
//           We subtract scaled resHi and resLo from 2.0 and return
//
//            r[0] := 2.0 - scale * resHi - scale * resLo
//
//           NOTE: we use multiprecision summations here to obtain more accurate
//                 result.
//
// --
//
*/
#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF
typedef struct {
  unsigned long _erfc_tbl[3488 * 2];
  unsigned long _AbsMask;
  unsigned long _MaxThreshold;
  unsigned long _SgnMask;
  unsigned long _One;
  unsigned long _TwoM128;
  unsigned long _SRound;
  unsigned long _Exp_X0_Mask;
  unsigned long _ExpMask;
  unsigned long _TwoM9;
  unsigned long _poly1_0;
  unsigned long _poly1_1;
  unsigned long _poly3_0;
  unsigned long _poly3_1;
  unsigned long _poly5_0;
  unsigned long _poly5_1;
  unsigned long _poly7_0;
  unsigned long _poly7_1;
  unsigned long _poly1_2;
  unsigned long _poly3_2;
  unsigned long _poly5_2;
  unsigned long _poly1_3;
  unsigned long _poly3_3;
  unsigned long _poly5_3;
  unsigned long _poly1_4;
  unsigned long _poly3_4;
  unsigned long _poly5_4;
  unsigned long _poly1_5;
  unsigned long _poly3_5;
  unsigned long _poly1_6;
  unsigned long _poly3_6;
  unsigned long _poly3_7;
  unsigned long _poly1_7;
  unsigned long _poly1_8;
  //    VVALUE     ( D, _poly1_9             );
  unsigned long _UF_Threshold;
  unsigned long _SplitMask;
  unsigned long _TwoP128;
  unsigned long _Mask32;
} __ocl_svml_internal_derfc_ha_data_t;
static __ocl_svml_internal_derfc_ha_data_t __ocl_svml_internal_derfc_ha_data = {
    // _erfc_tbl:
    //  first value is erfc_high(x0)*2^128
    //  second value combines erfc_low(x0)/exp_high(x0) as top 9 bits,
    //  and 2.0/sqrt(pi)*exp(-x0*x0)/(2^exponent(erfc(x0))*2^128
    //  in bottom 55 bits (using just 3 bits of the DP exponent)
    {
        0x47f0000000000000uL,
        0x00020dd750429b6duL,
        0x47efb7c9030853b2uL,
        0x71120d8f1975c85duL,
        0x47ef6f9447be0742uL,
        0x71120cb67bd452c7uL,
        0x47ef27640f9853d9uL,
        0x0e120b4d8bac36c1uL,
        0x47eedf3a9ba22dacuL,
        0x391209546ad13ccfuL,
        0x47ee971a2c4436aduL,
        0x4a1206cb4897b148uL,
        0x47ee4f05010eca8buL,
        0x4f9203b261cd0053uL,
        0x47ee06fd58842c7duL,
        0x6992000a00ae3804uL,
        0x47edbf056fe2df34uL,
        0x6d91fbd27cdc72d3uL,
        0x47ed771f82f02f4duL,
        0x4491f70c3b4f2cc8uL,
        0x47ed2f4dcbc2f894uL,
        0x2691f1b7ae44867fuL,
        0x47ece792828eae5buL,
        0x4b91ebd5552f795buL,
        0x47ec9fefdd6eaf18uL,
        0x5691e565bca400d4uL,
        0x47ec58681031eb69uL,
        0x4e91de697e413d29uL,
        0x47ec10fd4c26e895uL,
        0x6f91d6e14099944auL,
        0x47ebc9b1bfe82686uL,
        0x5391cecdb718d61cuL,
        0x47eb82879728f11duL,
        0x4991c62fa1e869b6uL,
        0x47eb3b80fa82a4bauL,
        0x7211bd07cdd189acuL,
        0x47eaf4a00f426da9uL,
        0x5991b357141d95d5uL,
        0x47eaade6f7378a0euL,
        0x3011a91e5a748165uL,
        0x47ea6757d08215d8uL,
        0x20919e5e92b964abuL,
        0x47ea20f4b5626818uL,
        0x34119318bae53a04uL,
        0x47e9dabfbc090900uL,
        0x4f91874ddcdfce24uL,
        0x47e994baf66747acuL,
        0x63917aff0e56ec10uL,
        0x47e94ee8720076b6uL,
        0x31916e2d7093cd8cuL,
        0x47e9094a37bbd66euL,
        0x0c1160da304ed92fuL,
        0x47e8c3e24bb73372uL,
        0x211153068581b781uL,
        0x47e87eb2ad1a4032uL,
        0x301144b3b337c90cuL,
        0x47e839bd55eaafc7uL,
        0x499135e3075d076buL,
        0x47e7f5043ae11861uL,
        0x44912695da8b5bdeuL,
        0x47e7b0894b3ea35cuL,
        0x141116cd8fd67618uL,
        0x47e76c4e70a390e7uL,
        0x0e11068b94962e5euL,
        0x47e728558ee694fbuL,
        0x5690f5d1602f7e41uL,
        0x47e6e4a083ed132fuL,
        0x0c10e4a073dc1b91uL,
        0x47e6a13127843ec0uL,
        0x3e90d2fa5a70c168uL,
        0x47e65e094b3b2413uL,
        0x1f90c0e0a8223359uL,
        0x47e61b2aba3da092uL,
        0x7610ae54fa490723uL,
        0x47e5d89739304dcfuL,
        0x1a909b58f724416buL,
        0x47e59650860d6468uL,
        0x791087ee4d9ad247uL,
        0x47e5545858029b38uL,
        0x5f907416b4fbfe7cuL,
        0x47e512b05f5006e0uL,
        0x42105fd3ecbec298uL,
        0x47e4d15a4527fdc7uL,
        0x20904b27bc403d30uL,
        0x47e49057ab900447uL,
        0x0c103613f2812dafuL,
        0x47e44faa2d42c4a0uL,
        0x3310209a65e29545uL,
        0x47e40f535d93160euL,
        0x0c900abcf3e187a9uL,
        0x47e3cf54c8501620uL,
        0x1c0fe8fb01a47307uL,
        0x47e38faff1aa574auL,
        0x3e0fbbbbef34b4b2uL,
        0x47e35066561a275cuL,
        0x680f8dc092d58ff8uL,
        0x47e311796a46f063uL,
        0x530f5f0cdaf15313uL,
        0x47e2d2ea9aefb635uL,
        0x660f2fa4c16c0019uL,
        0x47e294bb4cd4b2bcuL,
        0x5f0eff8c4b1375dbuL,
        0x47e256ecdca212ccuL,
        0x0e0ecec7870ebca8uL,
        0x47e219809edbd524uL,
        0x158e9d5a8e4c934euL,
        0x47e1dc77dfcacd02uL,
        0x188e6b4982f158b9uL,
        0x47e19fd3e36ac969uL,
        0x5c0e38988fc46e72uL,
        0x47e16395e559e218uL,
        0x020e054be79d3042uL,
        0x47e127bf18c8eadcuL,
        0x1a0dd167c4cf9d2auL,
        0x47e0ec50a86d0dd3uL,
        0x870d9cf06898cdafuL,
        0x47e0b14bb6728cd7uL,
        0x7b0d67ea1a8b5368uL,
        0x47e076b15c70aa27uL,
        0x518d325927fb9d89uL,
        0x47e03c82ab5eb831uL,
        0x2a8cfc41e36c7df9uL,
        0x47e002c0ab8a5017uL,
        0x6b8cc5a8a3fbea40uL,
        0x47df92d8b91d5cc7uL,
        0x209c8e91c4d01368uL,
        0x47df210d6a9a6a31uL,
        0x101c5701a484ef9duL,
        0x47deb02147ce245buL,
        0x301c1efca49a5011uL,
        0x47de40161b701274uL,
        0x2e1be68728e29d5euL,
        0x47ddd0ed9ea4bdd6uL,
        0x1d1bada596f25436uL,
        0x47dd62a978f7c956uL,
        0x429b745c55905bf8uL,
        0x47dcf54b4058455fuL,
        0x109b3aafcc27502euL,
        0x47dc88d479173ccduL,
        0x3f1b00a46237d5beuL,
        0x47dc1d4695e87644uL,
        0x0b1ac63e7ecc1411uL,
        0x47dbb2a2f7e5651fuL,
        0x3a1a8b8287ec6a09uL,
        0x47db48eaee924501uL,
        0x101a5074e2157620uL,
        0x47dae01fb7e55a65uL,
        0x359a1519efaf889euL,
        0x47da78428050527euL,
        0x1219d97610879642uL,
        0x47da115462cbbc17uL,
        0x00999d8da149c13fuL,
        0x47d9ab5668e4930auL,
        0x10196164fafd8de3uL,
        0x47d946498acbd766uL,
        0x1b9925007283d7aauL,
        0x47d8e22eaf68291duL,
        0x3418e86458169af8uL,
        0x47d87f06ac6960c4uL,
        0x1818ab94f6caa71duL,
        0x47d81cd2465e1d96uL,
        0x29186e9694134b9euL,
        0x47d7bb9230cb40b3uL,
        0x3a98316d6f48133duL,
        0x47d75b470e454d34uL,
        0x3a17f41dc12c9e89uL,
        0x47d6fbf1708ba47buL,
        0x2f17b6abbb7aaf19uL,
        0x47d69d91d8a595d9uL,
        0x4297791b886e7403uL,
        0x47d64028b7013867uL,
        0x23173b714a552763uL,
        0x47d5e3b66b9405a8uL,
        0x3116fdb11b1e0c34uL,
        0x47d5883b45fd2b62uL,
        0x3116bfdf0beddaf5uL,
        0x47d52db785a98ac9uL,
        0x379681ff24b4ab04uL,
        0x47d4d42b59f95af9uL,
        0x4496441563c665d4uL,
        0x47d47b96e267647auL,
        0x03960625bd75d07buL,
        0x47d423fa2eb1cb59uL,
        0x2015c8341bb23767uL,
        0x47d3cd553f045d45uL,
        0x13158a445da7c74cuL,
        0x47d377a8042458d1uL,
        0x14954c5a57629db0uL,
        0x47d322f25f9da2fcuL,
        0x3f950e79d1749ac9uL,
        0x47d2cf3423f15fdeuL,
        0x5d94d0a6889dfd9fuL,
        0x47d27c6d14c5e341uL,
        0x0f9492e42d78d2c5uL,
        0x47d22a9ce717edcauL,
        0x4794553664273d24uL,
        0x47d1d9c3416d2b4buL,
        0x131417a0c4049fd0uL,
        0x47d189dfbc07e690uL,
        0x2a93da26d759aef5uL,
        0x47d13af1e11be721uL,
        0x30939ccc1b136d5auL,
        0x47d0ecf92d046d22uL,
        0x22135f93fe7d1b3duL,
        0x47d09ff50e7b3f92uL,
        0x40132281e2fd1a92uL,
        0x47d053e4e6d0c10auL,
        0x5812e5991bd4cbfcuL,
        0x47d008c80a24ff0fuL,
        0x4792a8dcede3673buL,
        0x47cf7d3b7f436012uL,
        0x1da26c508f6bd0ffuL,
        0x47ceeaca836a27ccuL,
        0x0ba22ff727dd6f7buL,
        0x47ce5a3b7c9b56d9uL,
        0x21a1f3d3cf9ffe5auL,
        0x47cdcb8cae2d747euL,
        0x20a1b7e98fe26217uL,
        0x47cd3ebc436b0f25uL,
        0x27a17c3b626c7a12uL,
        0x47ccb3c8500ea348uL,
        0x2ea140cc3173f007uL,
        0x47cc2aaed0bfcfeeuL,
        0x13a1059ed7740313uL,
        0x47cba36dab91c0e9uL,
        0x09a0cab61f084b93uL,
        0x47cb1e02b082b71fuL,
        0x27209014c2ca74dauL,
        0x47ca9a6b99fc973auL,
        0x27a055bd6d32e8d7uL,
        0x47ca18a60d56673duL,
        0x26a01bb2b87c6968uL,
        0x47c998af9b56a3aduL,
        0x329fc3ee5d1524b0uL,
        0x47c91a85c0b65518uL,
        0x351f511a91a67d2auL,
        0x47c89e25e6a4cef8uL,
        0x369edeeee0959518uL,
        0x47c8238d634c0126uL,
        0x3e1e6d6ffaa65a25uL,
        0x47c7aab97a554543uL,
        0x281dfca26f5bbf88uL,
        0x47c733a75d6e91b7uL,
        0x2d9d8c8aace11e63uL,
        0x47c6be542ccffc2fuL,
        0x189d1d2cfff91594uL,
        0x47c64abcf7c175b3uL,
        0x369cae8d93f1d7b7uL,
        0x47c5d8debd20aacduL,
        0x401c40b0729ed548uL,
        0x47c568b66be6f268uL,
        0x1e9bd3998457afdbuL,
        0x47c4fa40e3af3673uL,
        0x2c9b674c8ffc6283uL,
        0x47c48d7af53bc19fuL,
        0x1d9afbcd3afe8ab6uL,
        0x47c4226162fbddd4uL,
        0x451a911f096fbc26uL,
        0x47c3b8f0e1912f6fuL,
        0x2a1a27455e14c93cuL,
        0x47c351261854b991uL,
        0x0b19be437a7de946uL,
        0x47c2eafda1db7849uL,
        0x3199561c7f23a47buL,
        0x47c286740c7a7dabuL,
        0x0b18eed36b886d93uL,
        0x47c22385daca7f46uL,
        0x3898886b1e5ecfd1uL,
        0x47c1c22f842ac1f2uL,
        0x009822e655b417e7uL,
        0x47c1626d7543521fuL,
        0x4b97be47af1f5d89uL,
        0x47c1043c1086777duL,
        0x11175a91a7f4d2eduL,
        0x47c0a797aeb152f1uL,
        0x3716f7c69d7d3ef8uL,
        0x47c04c7c9f4b968fuL,
        0x569695e8cd31867euL,
        0x47bfe5ce524c8ee4uL,
        0x2da634fa54fa285fuL,
        0x47bf35a715b2f3e0uL,
        0x2125d4fd33729015uL,
        0x47be887bf681f217uL,
        0x2ba575f3483021c3uL,
        0x47bdde4553ef94dduL,
        0x1aa517de540ce2a3uL,
        0x47bd36fb7fa50177uL,
        0x1724babff975a04cuL,
        0x47bc9296beb09cf0uL,
        0x2a245e99bcbb7915uL,
        0x47bbf10f4a759888uL,
        0x1e24036d0468a7a2uL,
        0x47bb525d5198cb1buL,
        0x1b23a93b1998736cuL,
        0x47bab678f8eabedbuL,
        0x07a35005285227f1uL,
        0x47ba1d5a5c4edb95uL,
        0x2b22f7cc3fe6f423uL,
        0x47b986f98f9f96c8uL,
        0x0ca2a09153529381uL,
        0x47b8f34e9f8f93a5uL,
        0x2d224a55399ea239uL,
        0x47b8625192879e39uL,
        0x10a1f518ae487dc8uL,
        0x47b7d3fa69816db5uL,
        0x1621a0dc51a9934duL,
        0x47b7484120df1b01uL,
        0x13a14da0a961fd14uL,
        0x47b6bf1db13f3982uL,
        0x3120fb6620c550afuL,
        0x47b63888104d811auL,
        0x0420aa2d09497f2buL,
        0x47b5b478318ff939uL,
        0x152059f59af7a906uL,
        0x47b532e6073095f2uL,
        0x1ca00abff4dec7a3uL,
        0x47b4b3c982c338c7uL,
        0x069f79183b101c5buL,
        0x47b4371a960807f7uL,
        0x361edeb406d9c825uL,
        0x47b3bcd133aa0ffbuL,
        0x421e4652fadcb6b2uL,
        0x47b344e54ffa23b9uL,
        0x1b1daff4969c0b04uL,
        0x47b2cf4ee1a5f0fcuL,
        0x169d1b982c501370uL,
        0x47b25c05e26b3f98uL,
        0x429c893ce1dcbef7uL,
        0x47b1eb024fc75284uL,
        0x309bf8e1b1ca2279uL,
        0x47b17c3c2ba26319uL,
        0x1e9b6a856c3ed54fuL,
        0x47b10fab7cf72f94uL,
        0x1a1ade26b7fbed95uL,
        0x47b0a548507696c0uL,
        0x049a53c4135a6526uL,
        0x47b03d0ab9273b93uL,
        0x4a19cb5bd549b111uL,
        0x47afadd5a20258d3uL,
        0x0aa944ec2e4f5630uL,
        0x47aee5c1730b147buL,
        0x1da8c07329874652uL,
        0x47ae21c938a45a82uL,
        0x22283deeada4d25auL,
        0x47ad61dd57628999uL,
        0x0227bd5c7df3fe9cuL,
        0x47aca5ee4649e31euL,
        0x25273eba3b5b07b7uL,
        0x47abedec8fddb33fuL,
        0x2026c205655be720uL,
        0x47ab39c8d3276d89uL,
        0x2a26473b5b15a7a1uL,
        0x47aa8973c4b5c03euL,
        0x15a5ce595c455b0auL,
        0x47a9dcde2f93a207uL,
        0x1725575c8a468362uL,
        0x47a933f8f6375f2cuL,
        0x0124e241e912c305uL,
        0x47a88eb51369acb8uL,
        0x1aa46f066040a832uL,
        0x47a7ed039b24c96buL,
        0x0323fda6bc016994uL,
        0x47a74ed5bb6bb580uL,
        0x2c238e1fae1d6a9duL,
        0x47a6b41cbd198bc8uL,
        0x01a3206dceef5f87uL,
        0x47a61cca04a90794uL,
        0x28a2b48d9e5dea1cuL,
        0x47a588cf12f4446buL,
        0x11224a7b84d38971uL,
        0x47a4f81d85ecc55auL,
        0x3621e233d434b813uL,
        0x47a46aa7194bd323uL,
        0x27a17bb2c8d41535uL,
        0x47a3e05da73b4158uL,
        0x362116f48a6476ccuL,
        0x47a3593328f6abbeuL,
        0x0320b3f52ce8c383uL,
        0x47a2d519b7653e1duL,
        0x392052b0b1a174eauL,
        0x47a254038bac19d6uL,
        0x0e1fe6460fef4680uL,
        0x47a1d5e2ffb96d40uL,
        0x119f2a901ccafb37uL,
        0x47a15aaa8ec85204uL,
        0x3b9e723726b824a9uL,
        0x47a0e24cd5dd8846uL,
        0x011dbd32ac4c99b0uL,
        0x47a06cbc943d2559uL,
        0x431d0b7a0f921e7cuL,
        0x479ff3d957b29b39uL,
        0x05ac5d0497c09e74uL,
        0x479f13a043742333uL,
        0x04abb1c972f23e50uL,
        0x479e38b43cbd0f0euL,
        0x142b09bfb7d11a84uL,
        0x479d62fbdc2e756auL,
        0x1faa64de673e8837uL,
        0x479c925e02b41668uL,
        0x0d29c31c6df3b1b8uL,
        0x479bc6c1da1f3121uL,
        0x14292470a61b6965uL,
        0x479b000ed5b4a626uL,
        0x00a888d1d8e510a3uL,
        0x479a3e2cb2ae9edauL,
        0x2127f036c0107294uL,
        0x4799810378b1f299uL,
        0x07275a96077274bauL,
        0x4798c87b7a37834euL,
        0x2726c7e64e7281cbuL,
        0x4798147d54e9cc33uL,
        0x0826381e2980956buL,
        0x479764f1f1f6dde9uL,
        0x1ba5ab342383d178uL,
        0x4796b9c286570419uL,
        0x2625211ebf41880buL,
        0x479612d893085124uL,
        0x2d2499d478bca735uL,
        0x4795701de53f4d2euL,
        0x0d24154bc68d75c3uL,
        0x4794d17c968d062buL,
        0x1623937b1b31925auL,
        0x479436df0cfabf1cuL,
        0x32a31458e6542847uL,
        0x4793a02ffb1b7ceduL,
        0x292297db960e4f63uL,
        0x47930d5a6013afc4uL,
        0x32a21df9981f8e53uL,
        0x47927e49879737d2uL,
        0x3021a6a95b1e786fuL,
        0x4791f2e909de04d1uL,
        0x372131e14fa1625duL,
        0x47916b24cb8f8f91uL,
        0x3720bf97e95f2a64uL,
        0x4790e6e8fda56cf6uL,
        0x37204fc3a0481321uL,
        0x479066221d4539d7uL,
        0x381fc4b5e32d6259uL,
        0x478fd179e7243e3buL,
        0x212eeea8c1b1db94uL,
        0x478edd4d2aec5adbuL,
        0x05ae1d4cf1e2450auL,
        0x478def98c6c79efauL,
        0x042d508f9a1ea64fuL,
        0x478d0838121f2418uL,
        0x07ac885df3451a07uL,
        0x478c2706fa45005euL,
        0x0eabc4a54a84e834uL,
        0x478b4be201caa4b3uL,
        0x18ab055303221015uL,
        0x478a76a63fc95c78uL,
        0x22aa4a549829587euL,
        0x4789a7315f1d6a54uL,
        0x1d2993979e14fffeuL,
        0x4788dd619d943ca1uL,
        0x0ca8e109c4622913uL,
        0x47881915cb0e3323uL,
        0x07283298d717210euL,
        0x47875a2d48946eb0uL,
        0x2b278832c03aa2b1uL,
        0x4786a08807632261uL,
        0x1ba6e1c5893c380buL,
        0x4785ec0687e8dcb1uL,
        0x18a63f3f5c4de13buL,
        0x47853c89d8bb3ddauL,
        0x2aa5a08e85af27e0uL,
        0x478491f395818f53uL,
        0x30a505a174e9c929uL,
        0x4783ec25e5d5af11uL,
        0x22246e66be002240uL,
        0x47834b037c1bbfc5uL,
        0x06a3dacd1a8d8cceuL,
        0x4782ae6f94510dd7uL,
        0x20234ac36ad8dafeuL,
        0x4782164df2d29765uL,
        0x07a2be38b6d92415uL,
        0x47818282e31ba3e7uL,
        0x2da2351c2f2d1449uL,
        0x4780f2f3367cd6a9uL,
        0x3121af5d2e04f3f6uL,
        0x4780678442cc256euL,
        0x26212ceb37ff9bc3uL,
        0x477fc037c21c3621uL,
        0x1130adb5fcfa8c75uL,
        0x477eb940d8319830uL,
        0x183031ad58d56279uL,
        0x477db9f17e61c30fuL,
        0x192f7182a851bca2uL,
        0x477cc218694238a1uL,
        0x15ae85c449e377f3uL,
        0x477bd18548996418uL,
        0x1f2da0005e5f28dfuL,
        0x477ae808c479c370uL,
        0x152cc0180af00a8buL,
        0x477a05747a543aa7uL,
        0x102be5ecd2fcb5f9uL,
        0x4779299afa0246a5uL,
        0x142b1160991ff737uL,
        0x4778544fc2c8c1dauL,
        0x03aa4255a00b9f03uL,
        0x477785674053e8b9uL,
        0x02a978ae8b55ce1buL,
        0x4776bcb6c7ad4853uL,
        0x2028b44e6031383euL,
        0x4775fa14942c3d53uL,
        0x26a7f5188610ddc8uL,
        0x47753d57c461a5a6uL,
        0x27273af0c737bb45uL,
        0x4774865856ff6329uL,
        0x2ca685bb5134ef13uL,
        0x4773d4ef27bc49a5uL,
        0x2e25d55cb54cd53auL,
        0x477328f5ec350e66uL,
        0x1aa529b9e8cf9a1euL,
        0x4772824730cacbb4uL,
        0x082482b8455dc491uL,
        0x4771e0be557fa673uL,
        0x0e23e03d891b37deuL,
        0x477144378ad22027uL,
        0x14a3422fd6d12e2buL,
        0x4770ac8fce979b96uL,
        0x10a2a875b5ffab56uL,
        0x477019a4e8d69648uL,
        0x31a212f612dee7fbuL,
        0x476f16aad1422a55uL,
        0x0ab181983e5133dduL,
        0x476e030141df7d25uL,
        0x02b0f443edc5ce49uL,
        0x476cf80d4afc3018uL,
        0x14306ae13b0d3255uL,
        0x476bf5908f50b4a0uL,
        0x0fafcab1483ea7fcuL,
        0x476afb4e269693deuL,
        0x142ec72615a894c4uL,
        0x476a090a974cfebeuL,
        0x01adcaf3691fc448uL,
        0x47691e8bd0830a73uL,
        0x18acd5ec93c12432uL,
        0x47683b9923a85f7buL,
        0x0eabe7e5ac24963buL,
        0x47675ffb3e6519a0uL,
        0x02ab00b38d6b3575uL,
        0x47668b7c2479902duL,
        0x08aa202bd6372dceuL,
        0x4765bde729a6b60fuL,
        0x04294624e78e0fafuL,
        0x4764f708eb9fba62uL,
        0x18a87275e3a6869euL,
        0x476436af4c058acbuL,
        0x0ea7a4f6aca256cbuL,
        0x47637ca96a6cd1d3uL,
        0x28a6dd7fe3358230uL,
        0x4762c8c79e6f04a3uL,
        0x0f261beae53b72b7uL,
        0x47621adb71c70c75uL,
        0x14256011cc3b036duL,
        0x476172b79a7a1181uL,
        0x01a4a9cf6bda3f4cuL,
        0x4760d02ff50ce651uL,
        0x0f23f8ff5042a88euL,
        0x476033197ec68c0duL,
        0x1ea34d7dbc76d7e5uL,
        0x475f3694a0008381uL,
        0x0cb2a727a89a3f14uL,
        0x475e11332d0714c5uL,
        0x043205dac02bd6b9uL,
        0x475cf5bf1fed1e70uL,
        0x0b31697560347b26uL,
        0x475be3eb08ae7c20uL,
        0x0f30d1d69569b82duL,
        0x475adb6b810af9e1uL,
        0x16303ede1a45bfeeuL,
        0x4759dbf721b98df9uL,
        0x1f2f60d8aa2a88f2uL,
        0x4758e54677bb0150uL,
        0x152e4cc4abf7d065uL,
        0x4757f713f9cc9783uL,
        0x182d4143a9dfe965uL,
        0x4757111bfdfb3ce0uL,
        0x032c3e1a5f5c077cuL,
        0x4756331caf57b5dauL,
        0x13ab430ecf4a83a8uL,
        0x47555cd603cc414fuL,
        0x17aa4fe83fb9db25uL,
        0x47548e09b21414bfuL,
        0x0ba9646f35a76624uL,
        0x4753c67b27d50fe7uL,
        0x12a8806d70b2fc36uL,
        0x475305ef7fdbfb94uL,
        0x1e27a3ade6c8b3e5uL,
        0x47524c2d787b9e36uL,
        0x1826cdfcbfc1e263uL,
        0x475198fd6a0ee7bduL,
        0x1125ff2750fe7820uL,
        0x4750ec293d9e6d85uL,
        0x12a536fc18f7ce5cuL,
        0x4750457c63a96690uL,
        0x0724754abacdf1dcuL,
        0x474f49879624a020uL,
        0x11b3b9e3f9d06e3fuL,
        0x474e139bb05eb49euL,
        0x09330499b503957fuL,
        0x474ce8d4b7fd6c6fuL,
        0x11b2553ee2a336bfuL,
        0x474bc8d516fda8b9uL,
        0x11b1aba78ba3af89uL,
        0x474ab341ee553e25uL,
        0x01b107a8c7323a6euL,
        0x4749a7c305336484uL,
        0x0b306918b6355624uL,
        0x4748a602b88919bfuL,
        0x1faf9f9cfd9c3035uL,
        0x4747adadead962eduL,
        0x032e77448fb66bb9uL,
        0x4746be73f45149fbuL,
        0x0c2d58da68fd1170uL,
        0x4745d80693276a6cuL,
        0x19ac4412bf4b8f0buL,
        0x4744fa19dc42d408uL,
        0x1eab38a3af2e55b4uL,
        0x474424642c28ff74uL,
        0x162a3645330550ffuL,
        0x4743569e18328603uL,
        0x21a93cb11a30d765uL,
        0x47429082600643fduL,
        0x10a84ba3004a50d0uL,
        0x4741d1cddf5a82dduL,
        0x1a2762d84469c18fuL,
        0x47411a3f7ffbbfeauL,
        0x08a6821000795a03uL,
        0x474069982c189a9euL,
        0x0625a90b00981d93uL,
        0x473f7f3581a4dc2buL,
        0x14b4d78bba8ca5fduL,
        0x473e381802242163uL,
        0x01340d564548fad7uL,
        0x473cfd6511405b2cuL,
        0x0db34a305080681fuL,
        0x473bcead7f01492fuL,
        0x0c328de11c5031ebuL,
        0x473aab859b20ac9euL,
        0x0b31d83170fbf6fbuL,
        0x473993851cc9779auL,
        0x073128eb96be8798uL,
        0x473886470ad946a7uL,
        0x05b07fdb4dafea5fuL,
        0x47378369a4a2cbd6uL,
        0x09afb99b8b8279e1uL,
        0x47368a8e4b2fc8c1uL,
        0x1cae7f232d9e2630uL,
        0x47359b596b012aa9uL,
        0x18ad4fed7195d7e8uL,
        0x4734b572664bd2dbuL,
        0x20ac2b9cf7f893bfuL,
        0x4733d8837fb08d1duL,
        0x0fab11d702b3deb2uL,
        0x47330439c56dadf6uL,
        0x05aa024365f771bduL,
        0x47323844fd08cb92uL,
        0x23a8fc8c794b03b5uL,
        0x473174578f6efd5cuL,
        0x1ca8005f08d6f1efuL,
        0x4730b826758a086auL,
        0x18270d6a46e07ddauL,
        0x473003692548d98auL,
        0x2b26235fbd7a4345uL,
        0x472eabb2fe335195uL,
        0x17b541f340697987uL,
        0x472d5e6777a83c2auL,
        0x09b468dadf4080abuL,
        0x472c1e6cb6239574uL,
        0x0c3397ced7af2b15uL,
        0x472aeb4423e690e6uL,
        0x15b2ce898809244euL,
        0x4729c47374a0974duL,
        0x13b20cc76202c5fbuL,
        0x4728a98484a1e8d3uL,
        0x0ab15246dda49d47uL,
        0x47279a0538dd4fc6uL,
        0x1e309ec86c75d497uL,
        0x472695875fb574a0uL,
        0x0bafe41cd9bb4eeeuL,
        0x47259ba0929261c5uL,
        0x00ae97ba3b77f306uL,
        0x4724abea183bc470uL,
        0x0c2d57f524723822uL,
        0x4723c600c7f477c5uL,
        0x0c2c245d4b99847auL,
        0x4722e984ed53e777uL,
        0x00aafc85e0f82e12uL,
        0x4722161a2cd9d893uL,
        0x1729e005769dbc1duL,
        0x47214b67693928ceuL,
        0x1928ce75e9f6f8a0uL,
        0x47208916a9561720uL,
        0x07a7c7744d9378f7uL,
        0x471f9da9fde95755uL,
        0x01b6caa0d3582fe9uL,
        0x471e38a4dc27b11auL,
        0x16b5d79eb71e893buL,
        0x471ce283a9e3e330uL,
        0x0a34ee1429bf7cc0uL,
        0x471b9ab1a96e3b3euL,
        0x07340daa3c89f5b6uL,
        0x471a609f7584d32buL,
        0x0cb3360ccd23db3auL,
        0x471933c2d52c56c8uL,
        0x19b266ea71d4f71auL,
        0x4718139690c0d186uL,
        0x11319ff4663ae9dfuL,
        0x4716ff9a4837fa43uL,
        0x0d30e0de78654d1euL,
        0x4715f7524a8e81a1uL,
        0x1330295ef6591848uL,
        0x4714fa476e59f667uL,
        0x142ef25d37f49fe1uL,
        0x47140806eb78e353uL,
        0x0aada01102b5f851uL,
        0x4713202235dada50uL,
        0x052c5b5412dcafaduL,
        0x4712422ed95a3234uL,
        0x17ab23a5a23e4210uL,
        0x47116dc656a14df5uL,
        0x2529f8893d8fd1c1uL,
        0x4710a2860115569cuL,
        0x0928d986a4187285uL,
        0x470fc01dbb80c841uL,
        0x0637c629a822bc9euL,
        0x470e4c0b066a4970uL,
        0x04b6be02102b3520uL,
        0x470ce823f4cc4bacuL,
        0x11b5c0a378c90bcauL,
        0x470b93bf40d5eccbuL,
        0x04b4cda5374ea275uL,
        0x470a4e3a125adc75uL,
        0x17b3e4a23d1f4703uL,
        0x470916f7c5f2f764uL,
        0x05330538fbb77ecduL,
        0x4707ed61b5d3db09uL,
        0x1c322f0b496539beuL,
        0x4706d0e7045988cauL,
        0x18b161be46ad3b50uL,
        0x4705c0fc68335b0cuL,
        0x01b09cfa445b00ffuL,
        0x4704bd1bfa2aba3cuL,
        0x15afc0d55470cf51uL,
        0x4703c4c504792bf7uL,
        0x112e577bbcd49935uL,
        0x4702d77bd3a382bbuL,
        0x1d2cfd4a5adec5c0uL,
        0x4701f4c988d02149uL,
        0x0aabb1a9657ce465uL,
        0x47011c3bed8e7169uL,
        0x1faa740684026555uL,
        0x47004d654905dad0uL,
        0x072943d4a1d1ed39uL,
        0x46ff0fb86d056745uL,
        0x0538208bc334a6a5uL,
        0x46fd9676faafa27fuL,
        0x013709a8db59f25cuL,
        0x46fc2e43d417197buL,
        0x04b5feada379d8b7uL,
        0x46fad664518e771auL,
        0x1434ff207314a102uL,
        0x46f98e25420092dauL,
        0x07340a8c1949f75euL,
        0x46f854daa4a49b0fuL,
        0x0cb3207fb7420eb9uL,
        0x46f729df6503422auL,
        0x01b2408e9ba3327fuL,
        0x46f60c95193c542duL,
        0x02b16a501f0e42cauL,
        0x46f4fc63c27c71aduL,
        0x1b309d5f819c9e29uL,
        0x46f3f8b98f93052auL,
        0x08afb2b792b40a22uL,
        0x46f3010aa198de77uL,
        0x122e3bcf436a1a95uL,
        0x46f214d0d298364fuL,
        0x1bacd55277c18d05uL,
        0x46f1338b7e273194uL,
        0x11ab7e94604479dcuL,
        0x46f05cbf4be650aauL,
        0x15aa36eec00926dduL,
        0x46ef1febf7a916aauL,
        0x04b8fdc1b2dcf7b9uL,
        0x46ed997c68d65935uL,
        0x1137d2737527c3f9uL,
        0x46ec2556a4e7a90fuL,
        0x0536b4702d7d5849uL,
        0x46eac2aa7516ade3uL,
        0x0eb5a329b7d30748uL,
        0x46e970b05888fda1uL,
        0x12b49e17724f4d41uL,
        0x46e82ea92dbc1a27uL,
        0x0333a4b60ba9aa4euL,
        0x46e6fbdddeff308euL,
        0x15b2b6875310f785uL,
        0x46e5d79f11e27f6buL,
        0x06b1d312098e9dbauL,
        0x46e4c144d984e1b8uL,
        0x0130f9e1b4dd36dfuL,
        0x46e3b82e6ba892a4uL,
        0x0c302a8673a94692uL,
        0x46e2bbc1d878d271uL,
        0x13aec929a665b449uL,
        0x46e1cb6bc4eaa678uL,
        0x0d2d4f4b4c8e09eduL,
        0x46e0e69f27a37df3uL,
        0x04abe6abbb10a5aauL,
        0x46e00cd508511266uL,
        0x12aa8e8cc1fadef6uL,
        0x46de7b1882bccac5uL,
        0x04394637d5bacfdbuL,
        0x46dcf09287e48bb8uL,
        0x14b80cfdc72220cfuL,
        0x46db792bbc489b03uL,
        0x11b6e2367dc27f95uL,
        0x46da140206ab9450uL,
        0x0bb5c540b4936fd2uL,
        0x46d8c03d2d39119buL,
        0x05b4b581b8d170fcuL,
        0x46d77d0e6e5bed20uL,
        0x11b3b2652b06c2b2uL,
        0x46d649b01d731109uL,
        0x0fb2bb5cc22e5db6uL,
        0x46d525654343aad1uL,
        0x15b1cfe010e2052duL,
        0x46d40f79420887c7uL,
        0x0130ef6c4c84a0feuL,
        0x46d3073f7cff4a85uL,
        0x02301984165a5f36uL,
        0x46d20c1303550f0duL,
        0x1cae9b5e8d00ce77uL,
        0x46d11d563e54f40euL,
        0x05ad16f5716c6c1auL,
        0x46d03a72a2bbdc06uL,
        0x06aba4f035d60e03uL,
        0x46cec5b0ca2b20f5uL,
        0x053a447b7b03f045uL,
        0x46cd2bfc6210880auL,
        0x0938f4ccca7fc90duL,
        0x46cba6c1c6e87c40uL,
        0x0737b5223dac7336uL,
        0x46ca35068e9c89ceuL,
        0x0c3684c227fcacefuL,
        0x46c8d5dbaa383b98uL,
        0x05b562fac4329b48uL,
        0x46c7885ce9f67cdauL,
        0x13344f21e49054f2uL,
        0x46c64bb0863504dduL,
        0x01b34894a5e24657uL,
        0x46c51f06ad20e4c3uL,
        0x07324eb7254ccf83uL,
        0x46c4019914f0b539uL,
        0x1c3160f438c70913uL,
        0x46c2f2aa92823e80uL,
        0x0f307ebd2a2d2844uL,
        0x46c1f186b432c98auL,
        0x16af4f12e9ab070auL,
        0x46c0fd8160ca94a0uL,
        0x012db5ad0b27805cuL,
        0x46c015f67a552924uL,
        0x082c304efa2c6f4euL,
        0x46be749309831665uL,
        0x11babe09e9144b5euL,
        0x46bcd3caa04cdd1auL,
        0x10b95df988e76644uL,
        0x46bb48774d0f8e45uL,
        0x07b80f439b4ee04buL,
        0x46b9d189f9f85cbeuL,
        0x11b6d11788a69c64uL,
        0x46b86e0050236315uL,
        0x0235a2adfa0b4bc4uL,
        0x46b71ce426a561d3uL,
        0x0134834877429b8fuL,
        0x46b5dd4af79906a8uL,
        0x13b37231085c7d9auL,
        0x46b4ae555af52cdeuL,
        0x14326eb9daed6f7euL,
        0x46b38f2e86f38215uL,
        0x1631783ceac28910uL,
        0x46b27f0bd5d0e6b1uL,
        0x01b08e1badf0fceduL,
        0x46b17d2c50b2bfafuL,
        0x002f5f7d88472604uL,
        0x46b088d83f7e4068uL,
        0x152db92b5212fb8duL,
        0x46af42c17ae0ebf6uL,
        0x083c282cd3957edauL,
        0x46ad8c3ea48f2888uL,
        0x123aab7abace48dcuL,
        0x46abeceb1f9f5b3cuL,
        0x0cb94219bfcb4928uL,
        0x46aa6399674d366auL,
        0x12b7eb1a2075864euL,
        0x46a8ef2a9a18d856uL,
        0x1036a597219a93dauL,
        0x46a78e8dcd2e6bfcuL,
        0x103570b69502f313uL,
        0x46a640bf6745325duL,
        0x11344ba864670882uL,
        0x46a504c882a97423uL,
        0x103335a62115bce2uL,
        0x46a3d9be56279ee9uL,
        0x07322df298214423uL,
        0x46a2bec1a4917edbuL,
        0x033133d96ae7e0dduL,
        0x46a1b2fe32991d5buL,
        0x10b046aeabcfcdecuL,
        0x46a0b5aa42bf5054uL,
        0x0daecb9cfe1d8642uL,
        0x469f8c0c2e2ce8deuL,
        0x07bd21397ead99cbuL,
        0x469dc6b6f1384e17uL,
        0x0b3b8d094c86d374uL,
        0x469c19fa87de37fauL,
        0x113a0df0f0c626dcuL,
        0x469a848df650bea7uL,
        0x0538a2e269750a39uL,
        0x46990538b942ea7cuL,
        0x08b74adc8f4064d3uL,
        0x46979ad1fce5b3d7uL,
        0x15b604ea819f007cuL,
        0x4696443fdcf0c326uL,
        0x15b4d0231928c6f9uL,
        0x46950076ad55cc39uL,
        0x0cb3aba85fe22e20uL,
        0x4693ce784b411930uL,
        0x113296a70f414053uL,
        0x4692ad53760d7286uL,
        0x1831905613b3abf2uL,
        0x46919c232fd50b87uL,
        0x1eb097f6156f32c5uL,
        0x46909a0e254c75e0uL,
        0x09af59a20caf6695uL,
        0x468f4c8c392fb944uL,
        0x01bd9c73698fb1dcuL,
        0x468d800ed59bd025uL,
        0x0fbbf716c6168baeuL,
        0x468bcd30dfbd611buL,
        0x03ba6852c6b58392uL,
        0x468a32923130213fuL,
        0x0438eefd70594a89uL,
        0x4688aee4cd06ec1auL,
        0x143789fb715aae95uL,
        0x468740ebfab80eb3uL,
        0x12b6383f726a8e04uL,
        0x4685e77b6bbd2126uL,
        0x17b4f8c96f26a26auL,
        0x4684a1766b6e5e8auL,
        0x05b3caa61607f920uL,
        0x46836dcf18a6465buL,
        0x1932acee2f5ecdb8uL,
        0x46824b85a8bf0124uL,
        0x01319ec60b1242eduL,
        0x468139a7b37f8475uL,
        0x0ab09f5cf4dd2877uL,
        0x4680374f8792ca97uL,
        0x0eaf5bd95d8730d8uL,
        0x467e87470e4f4245uL,
        0x113d9371e2ff7c35uL,
        0x467cbbab18b73217uL,
        0x01bbe41de54d155auL,
        0x467b0a44aa2f067duL,
        0x0e3a4c89e08ef4f3uL,
        0x467971a1ec0f40c7uL,
        0x03b8cb738399b12cuL,
        0x4677f064a8ba8322uL,
        0x10375fa8dbc84becuL,
        0x467685414c16188duL,
        0x163608078a70dcbcuL,
        0x46752efdf060cd20uL,
        0x0634c37c0394d094uL,
        0x4673ec7176d784b5uL,
        0x08b39100d5687bfeuL,
        0x4672bc82ab9d2302uL,
        0x09326f9df8519bd7uL,
        0x46719e277461403fuL,
        0x1ab15e6827001f18uL,
        0x467090640946d2d5uL,
        0x0ab05c803e4831c1uL,
        0x466f24946f22d5aeuL,
        0x053ed22548cffd35uL,
        0x466d45f15b49b35euL,
        0x04bd06ad6ecdf971uL,
        0x466b83349fd05190uL,
        0x0cbb551c847fbc96uL,
        0x4669dacb2c432ef4uL,
        0x02b9bc09f112b494uL,
        0x46684b37e1cbf8eauL,
        0x0b383a1ff0aa239duL,
        0x4666d3126d74b6cbuL,
        0x1036ce1aa3fd7bdduL,
        0x4665710631158bffuL,
        0x03b576c72b514859uL,
        0x466423d13a3b73e0uL,
        0x18343302cc4a0da8uL,
        0x4662ea43465e3995uL,
        0x033301ba221dc9bbuL,
        0x4661c33cd3c37adcuL,
        0x1ab1e1e857adc568uL,
        0x4660adae3e73c2b4uL,
        0x1130d2966b1746f7uL,
        0x465f512dd15b73b7uL,
        0x01bfa5b4f49cc6b2uL,
        0x465d6608dc942687uL,
        0x063dc3ae30b55c16uL,
        0x465b9823c51276e0uL,
        0x0b3bfd7555a3bd68uL,
        0x4659e5ce2f93dd76uL,
        0x09ba517d9e61628auL,
        0x46584d6fe15b6b93uL,
        0x04b8be4f8f6c951fuL,
        0x4656cd87746bc76buL,
        0x02b74287ded49339uL,
        0x465564a91cd221efuL,
        0x10b5dcd669f2cd34uL,
        0x4654117d7e2c667duL,
        0x07348bfd38302871uL,
        0x4652d2c0909ebeb8uL,
        0x0db34ecf8a3c124auL,
        0x4651a7409475f2f9uL,
        0x0bb22430f521cbcfuL,
        0x46508ddd13bd35e6uL,
        0x13b10b1488aeb235uL,
        0x464f0b0be22d18e7uL,
        0x09c0027c00a263a6uL,
        0x464d1a75065a8c73uL,
        0x103e12ee004efc37uL,
        0x464b48117843c1c7uL,
        0x05bc3e44ae32b16buL,
        0x46499218b8ac7f8euL,
        0x02ba854ea14102a8uL,
        0x4647f6dc6010b4aduL,
        0x08b8e6761569f45duL,
        0x464674c6ae60d851uL,
        0x0eb7603bac345f65uL,
        0x46450a592e3c968duL,
        0x0eb5f1353cdad001uL,
        0x4643b62b6aafb0c7uL,
        0x14b4980cb3c80949uL,
        0x464276e9b681072euL,
        0x1333537f00b6ad4duL,
        0x46414b54042f445auL,
        0x11b2225b12bffc68uL,
        0x4640323ccdc1a3dcuL,
        0x0cb10380e1adb7e9uL,
        0x463e5510173b9a4fuL,
        0x083febc107d5efaauL,
        0x463c6654733b86aduL,
        0x08bdf0f2a0ee6947uL,
        0x463a964ed354f983uL,
        0x0b3c14b2188bcee4uL,
        0x4638e324c651b063uL,
        0x0d3a553644f7f07duL,
        0x46374b179d1eba80uL,
        0x0d38b0cfce0579e0uL,
        0x4635cc82d9070d95uL,
        0x013725e7c5dd20f7uL,
        0x463465daafca8b1duL,
        0x0b35b2fe547a1340uL,
        0x463315aaa46df48euL,
        0x0cb456a974e92e93uL,
        0x4631da9433aebbcfuL,
        0x01b30f93c3699078uL,
        0x4630b34d93135fbfuL,
        0x16b1dc7b5b978cf8uL,
        0x462f3d41033c44ccuL,
        0x06c0bc30c5d52f15uL,
        0x462d36d25268cd2auL,
        0x0bbf5b2be65a0c7fuL,
        0x462b512a1fb1d8fcuL,
        0x03bd5f3a8dea7357uL,
        0x46298a442fc4fc15uL,
        0x04bb82915b03515buL,
        0x4627e03b1cc6d738uL,
        0x02b9c3517e789488uL,
        0x462651468e010b89uL,
        0x11381fb7df06136euL,
        0x4624dbb989001d84uL,
        0x04b6961b8d641d06uL,
        0x46237e00dac4e8b4uL,
        0x13b524ec4d916caeuL,
        0x462236a197bf0b9auL,
        0x04b3cab1343d18d1uL,
        0x46210437b1569d7euL,
        0x05b2860757487a01uL,
        0x461fcae93fb7323cuL,
        0x04c155a09065d4f7uL,
        0x461db23c3f816f91uL,
        0x0fc0384250e4c9fcuL,
        0x461bbc1a022c14d3uL,
        0x0dbe59890b926c78uL,
        0x4619e658108af2e0uL,
        0x05bc642116a8a9e3uL,
        0x46182eedbe410407uL,
        0x03ba8e405e651ab6uL,
        0x461693f22ab61ce8uL,
        0x13b8d5f98114f872uL,
        0x4615139a5f3661fbuL,
        0x0637397c5a66e307uL,
        0x4613ac3788a1b428uL,
        0x1035b71456c5a4c4uL,
        0x46125c354b26cb4duL,
        0x10b44d26de513197uL,
        0x461122182e9a270euL,
        0x1232fa31d6371537uL,
        0x460ff8f84418d510uL,
        0x0641bcca373b7b43uL,
        0x460dd4262aac53e7uL,
        0x0dc0939ab853339fuL,
        0x460bd3474ec16ca5uL,
        0x04befac5187b2863uL,
        0x4609f40fd0082b72uL,
        0x05bcf1e86235d0e7uL,
        0x4608345858c4438duL,
        0x063b0a68a2128babuL,
        0x4606921be96b86b0uL,
        0x12b9423165bc4444uL,
        0x46050b75c536f926uL,
        0x15b7974e743dea3duL,
        0x46039e9f7dcbe478uL,
        0x0db607e9eacd1050uL,
        0x460249ef1c3be817uL,
        0x0234924a74dec729uL,
        0x46010bd565b35392uL,
        0x173334d19e0c2160uL,
        0x45ffc5b8748842b1uL,
        0x09c1edfa3c5f5ccauL,
        0x45fd9b4a18a38642uL,
        0x05c0bc56f1b54701uL,
        0x45fb95cede6d524auL,
        0x09bf3d2185e047d9uL,
        0x45f9b2df77a02224uL,
        0x0e3d26cb87945e87uL,
        0x45f7f03b935e8e39uL,
        0x0a3b334fac4b9f99uL,
        0x45f64bc777824f0duL,
        0x0c396076f7918d1cuL,
        0x45f4c389be9acb82uL,
        0x0fb7ac2d72fc2c63uL,
        0x45f355a9387de78cuL,
        0x07b614801550319euL,
        0x45f2006aeb6bc767uL,
        0x17b4979ac8b28927uL,
        0x45f0c23033e2a375uL,
        0x1a3333c68e2d0548uL,
        0x45ef32ea02b55d22uL,
        0x0c41e767bce37dd7uL,
        0x45ed099c5c770f59uL,
        0x0840b0fc5b6d05a0uL,
        0x45eb05cfe2e99434uL,
        0x0a3f1e3523b41d7duL,
        0x45e92508d0743fc8uL,
        0x0c3d00de6608effeuL,
        0x45e764f46cf19f9buL,
        0x0dbb0778b7b3301buL,
        0x45e5c36679625a00uL,
        0x0c392fb04ec0f6cfuL,
        0x45e43e56c3e340a6uL,
        0x0eb77756ec9f78fauL,
        0x45e2d3dee1869200uL,
        0x0f35dc61922d5a06uL,
        0x45e182380bd2f493uL,
        0x0eb45ce65699ff6duL,
        0x45e047b91fcb6491uL,
        0x02b2f71a5f159970uL,
        0x45de45a9790460c1uL,
        0x0641a94ff571654fuL,
        0x45dc242efeaca75fuL,
        0x0cc071f4bbea09ecuL,
        0x45da284cb82c31cduL,
        0x093e9f1ff8ddd774uL,
        0x45d84f7a1eb7f7f3uL,
        0x03bc818223a202c7uL,
        0x45d697595326d7dbuL,
        0x0d3a887bd2b4404duL,
        0x45d4fdb462549af1uL,
        0x09b8b1a336c5eb6buL,
        0x45d3807ab51436a7uL,
        0x1636fab63324088auL,
        0x45d21dbea9108398uL,
        0x04b56197e30205bauL,
        0x45d0d3b35021d694uL,
        0x15b3e44e45301b92uL,
        0x45cf4154a787cc1auL,
        0x0dc281000bfe4c3fuL,
        0x45cd0623f4f4a28euL,
        0x0c4135f28f2d50b4uL,
        0x45caf2e69a26260fuL,
        0x09400187dded5975uL,
        0x45c904e0b3aa82a3uL,
        0x033dc479de0ef001uL,
        0x45c73985278fa30duL,
        0x0f3bad4fdad3caa1uL,
        0x45c58e7298af87d9uL,
        0x05b9baed3ed27ab8uL,
        0x45c401708b7e64c5uL,
        0x0d37ead9ce4285bbuL,
        0x45c2906cb94eb40cuL,
        0x14b63ac6b4edc88euL,
        0x45c139788f2dd662uL,
        0x0fb4a88be2a6390cuL,
        0x45bff58dab4f2a79uL,
        0x064332259185f1a0uL,
        0x45bda552fdd03043uL,
        0x00c1d5b1f3793044uL,
        0x45bb7f1f31b571b5uL,
        0x0dc0916f04b6e18buL,
        0x45b98006c2117e38uL,
        0x08bec77101de6926uL,
        0x45b7a550f03b145buL,
        0x05bc960bf23153e0uL,
        0x45b5ec74662c5961uL,
        0x093a8bd20fc65ef7uL,
        0x45b4531410823029uL,
        0x0f38a61745ec7d1duL,
        0x45b2d6fc2c9e8bbfuL,
        0x1336e25d0e756261uL,
        0x45b1761f87a6dc3cuL,
        0x0cb53e4f7d1666cbuL,
        0x45b02e94eb4ac8a5uL,
        0x08b3b7c27a7ddb0euL,
        0x45adfd296adef82auL,
        0x05c24caf2c32af14uL,
        0x45abc8ed301215ebuL,
        0x06c0fb3186804d0fuL,
        0x45a9bd5efd2c0f15uL,
        0x05bf830c0bb41fd7uL,
        0x45a7d79f2db2d4a5uL,
        0x003d3c0f1a91c846uL,
        0x45a61500f5293f05uL,
        0x12bb1e5acf351d87uL,
        0x45a47306f04df3d6uL,
        0x04b92712d259ce66uL,
        0x45a2ef5ff0323b28uL,
        0x03b7538c60a04476uL,
        0x45a187e3fb74914duL,
        0x02b5a14b04b47879uL,
        0x45a03a918225a965uL,
        0x0eb40dfd87456f4cuL,
        0x459e0b15822be4e0uL,
        0x06c2977b1172b9d5uL,
        0x459bce26a2fb7175uL,
        0x0d413bc07e891491uL,
        0x4599bb1bc445c3c5uL,
        0x0fbff1dbb4300811uL,
        0x4597cef42e9a617duL,
        0x053d9a880f306bd8uL,
        0x459606e51e0a4962uL,
        0x0b3b6e45220b55e0uL,
        0x459460560e841d78uL,
        0x13b96a0b33f2c4dauL,
        0x4592d8dd47a40ad7uL,
        0x14b78b07e9e924acuL,
        0x45916e3ca3d4393fuL,
        0x0b35ce9ab1670dd2uL,
        0x45901e5e8edda47auL,
        0x1834325167006bb0uL,
        0x458dcea670907819uL,
        0x0342b3e53538ff3fuL,
        0x458b8e9bec48816duL,
        0x07415137a7f44864uL,
        0x45897945aa1c9c34uL,
        0x0940084ff125639duL,
        0x45878b88a4e7107auL,
        0x0fbdaeb0b7311ec7uL,
        0x4585c2827c986b61uL,
        0x113b7937d1c40c53uL,
        0x45841b858361b0fduL,
        0x0c396d082f59ab06uL,
        0x458294150fb19119uL,
        0x00b7872d9fa10aaduL,
        0x458129e20e732adbuL,
        0x1035c4e8e37bc7d0uL,
        0x457fb58fa290d436uL,
        0x05c423ac0df49a40uL,
        0x457d499229819bc5uL,
        0x0b42a117230ad284uL,
        0x457b0c1a759f7738uL,
        0x08c13af4f04f9998uL,
        0x4578f9bb6c075485uL,
        0x0bbfde703724e560uL,
        0x45770f4744735c2buL,
        0x00bd77f0c82e7641uL,
        0x457549cb0f7ef8e2uL,
        0x01bb3ee02611d7dduL,
        0x4573a68a8c1234e1uL,
        0x01392ff33023d5bduL,
        0x457222fc469e8b8cuL,
        0x03b7481a9e69f53fuL,
        0x4570bcc5fd30f1dcuL,
        0x14b5847eda620959uL,
        0x456ee3728761897auL,
        0x07c3e27c1fcc74bduL,
        0x456c7fa0c7e3bac6uL,
        0x0ac25f9ee0b923dcuL,
        0x456a4a56eb132a54uL,
        0x0340f9a068653200uL,
        0x4568401b5336a8a0uL,
        0x013f5cc7718082b0uL,
        0x45665db58e2358c0uL,
        0x103cf7e53d6a2ca5uL,
        0x4564a029a7ea7cd0uL,
        0x0bbac0f5f3229372uL,
        0x456304b3d1961171uL,
        0x00b8b498644847eauL,
        0x456188c45630dc52uL,
        0x11b6cfa9bcca59dcuL,
        0x456029fbd8b92834uL,
        0x12b50f411d4fd2cduL,
        0x455dcc4fabf32f1buL,
        0x08c370ab8327af5euL,
        0x455b767ecb334a7duL,
        0x0b41f167f88c6b6euL,
        0x45594ec06c0ff29euL,
        0x0cc08f24085d4597uL,
        0x455751977e5803d3uL,
        0x01be8f70e181d61auL,
        0x45557bc950253824uL,
        0x123c324c20e337dcuL,
        0x4553ca58b816a87euL,
        0x0bba03261574b54euL,
        0x45523a8197d2607euL,
        0x08b7fe903cdf5855uL,
        0x4550c9b4b0a6a16euL,
        0x0bb6215c58da3450uL,
        0x454eeb27891d2bb2uL,
        0x08446897d4b69fc6uL,
        0x454c77dbfc848865uL,
        0x0c42d1877d731b7buL,
        0x454a357936adf17auL,
        0x084159a386b11517uL,
        0x4548203fa7992553uL,
        0x0bbffd27ae9393ceuL,
        0x454634b7f56b0a5buL,
        0x0e3d7c593130dd0buL,
        0x45446fada7e6a5feuL,
        0x013b2cd607c79bcfuL,
        0x4542ce2a3690576buL,
        0x0a390ae4d3405651uL,
        0x45414d707280e6cfuL,
        0x02371312dd1759e2uL,
        0x453fd5f08ad2b29auL,
        0x0345422ef5d8949duL,
        0x453d48d57f7718b6uL,
        0x08c39544b0ecc957uL,
        0x453aef3ce0add577uL,
        0x0d420997f73e73dduL,
        0x4538c52800f939c8uL,
        0x03c09ca0eaacd277uL,
        0x4536c6e61e57bf9buL,
        0x043e9810295890ecuL,
        0x4534f10e8ebc44a8uL,
        0x0f3c2b45b5aa4a1duL,
        0x4533407b59d72a5auL,
        0x12b9eee068fa7596uL,
        0x4531b2443858c0a1uL,
        0x00b7df2b399c10a8uL,
        0x453043b9f1621ff2uL,
        0x1535f8b87a31bd85uL,
        0x452de4c41eb96b44uL,
        0x0b44385c96e9a2d9uL,
        0x452b77e5cbd5d147uL,
        0x06c29b2933ef4cbcuL,
        0x45293c9fc62bfb11uL,
        0x04c11e68a6378f8auL,
        0x45272f0c4c8e9bffuL,
        0x06bf7f338086a86buL,
        0x45254b92affb11afuL,
        0x003cf8d7d9ce040auL,
        0x45238ee17b150181uL,
        0x0f3aa577251ae485uL,
        0x4521f5e908f70e0buL,
        0x0b38811d739efb5fuL,
        0x45207dd6833bb37fuL,
        0x0f368823e52970beuL,
        0x451e481e7f6ac4bcuL,
        0x0344b72ae68e8b4cuL,
        0x451bcc58edad5558uL,
        0x0b430b14dbe876bcuL,
        0x451983ee9896d581uL,
        0x0d4181012ef86610uL,
        0x45176aca47764426uL,
        0x0fc01647ba798745uL,
        0x45157d287836bd3duL,
        0x053d90e917701675uL,
        0x4513b79118c097a1uL,
        0x093b2a87e86d0c8auL,
        0x451216d1b97279a9uL,
        0x0138f53dcb377293uL,
        0x451097f82fc04024uL,
        0x15b6ed2f2515e933uL,
        0x450e709b415656d0uL,
        0x00c50ecc9ed47f19uL,
        0x450beaa3d6c15504uL,
        0x00c356cd5ce7799euL,
        0x4509996ed9b83966uL,
        0x0cc1c229a587ab78uL,
        0x450778be2bd9795auL,
        0x0f404e15ecc7f3f6uL,
        0x450584a99af8a842uL,
        0x023deffc7e6a6017uL,
        0x4503b99832cbefdduL,
        0x083b7b040832f310uL,
        0x4502143a112d0465uL,
        0x123938e021f36d76uL,
        0x45009182b326b228uL,
        0x1337258610b3b233uL,
        0x44fe5d47637f5db5uL,
        0x01453d3bfc82a909uL,
        0x44fbd20fcc3b76d6uL,
        0x0c437c92babdc2fduL,
        0x44f97c9dda748fc7uL,
        0x0441e06010120f6auL,
        0x44f7589207e91ad1uL,
        0x054065b9616170d4uL,
        0x44f561e669aa7fdauL,
        0x0cbe13dd96b3753buL,
        0x44f394e7a2ac9fc6uL,
        0x0e3b950d32467392uL,
        0x44f1ee2e61eccc99uL,
        0x08394a72263259a5uL,
        0x44f06a996198f06euL,
        0x0fb72fd93e036cdcuL,
        0x44ee0e8fbad2703duL,
        0x08c54164576929abuL,
        0x44eb8328ee330ae9uL,
        0x06c37b83c521fe96uL,
        0x44e92e21013a7670uL,
        0x02c1daf033182e96uL,
        0x44e70aff489136eauL,
        0x08c05ca50205d26auL,
        0x44e515a7c77fab48uL,
        0x01bdfbb6235639fauL,
        0x44e34a53ce0bbb6fuL,
        0x08bb7807e294781fuL,
        0x44e1a58b2b09fdcauL,
        0x0d39298add70a734uL,
        0x44e0241de6c31e5auL,
        0x0eb70beaf9c7ffb6uL,
        0x44dd863cf753825buL,
        0x07451b2cd6709222uL,
        0x44daffb906d0ae09uL,
        0x00c353a6cf7f7fffuL,
        0x44d8afbf9e9520c1uL,
        0x0cc1b1fa8cbe84a7uL,
        0x44d691c7c768becduL,
        0x0a40330f0fd69921uL,
        0x44d4a1a79df39cdduL,
        0x0dbda81670f96f9buL,
        0x44d2db8ca9009091uL,
        0x08bb24a16b4d09aauL,
        0x44d13bf4cb384e49uL,
        0x10b8d6eeb6efdbd6uL,
        0x44cf7f4f88751db4uL,
        0x0146ba91ac734786uL,
        0x44ccc7626bced451uL,
        0x07c4cb7966770ab5uL,
        0x44ca4ab6470c1c5buL,
        0x0cc305e9721d0981uL,
        0x44c80451c2811052uL,
        0x04c1667311fff70auL,
        0x44c5efa4d64f59f6uL,
        0x023fd3de10d62855uL,
        0x44c40880373ed740uL,
        0x013d1aefbcd48d0cuL,
        0x44c24b0d7368076euL,
        0x063a9cc93c25aca9uL,
        0x44c0b3c7b0d960f0uL,
        0x03b85487ee3ea735uL,
        0x44be7eea02e4ed87uL,
        0x07463daf8b4b1e0cuL,
        0x44bbd6408059b695uL,
        0x07445421e69a6ca1uL,
        0x44b96826d9e90341uL,
        0x04c294175802d99auL,
        0x44b72fa4fa12d515uL,
        0x0a40fa17bf41068fuL,
        0x44b5282d2d5803fduL,
        0x093f05e82aae2bb9uL,
        0x44b34d935f1be063uL,
        0x0d3c578101b29058uL,
        0x44b19c050c56d0d6uL,
        0x12b9e39dc5dd2f7cuL,
        0x44b01001dd9c7cceuL,
        0x07b7a553a728bbf2uL,
        0x44ad4ca9b634ecbauL,
        0x00c5982008db1304uL,
        0x44aab81c5c80cf38uL,
        0x0b43b7e00422e51buL,
        0x44a85cfacb7477f1uL,
        0x0bc200c898d9ee3euL,
        0x44a6365862923eb9uL,
        0x03406f5f7eb65a56uL,
        0x44a43fb317b5dc37uL,
        0x083e00e9148a1d25uL,
        0x44a274ea96044bd6uL,
        0x123b623734024e92uL,
        0x44a0d23817479c67uL,
        0x0638fd4e01891bf8uL,
        0x449ea84dd1592590uL,
        0x04c6cd44c7470d89uL,
        0x449bef1b1a12823duL,
        0x0744cd9c04158cd7uL,
        0x4499730edfda64acuL,
        0x0542fa34bf5c8344uL,
        0x44972ede3b7eaa24uL,
        0x0cc14f4890ff2461uL,
        0x44951db1ec3a3086uL,
        0x0abf92c49dfa4df5uL,
        0x44933b1c9d1576ecuL,
        0x063ccaaea71ab0dfuL,
        0x44918311f8a03acauL,
        0x07ba40829f001197uL,
        0x448fe3bcf4629feauL,
        0x0147eef13b59e96cuL,
        0x448d083fda665163uL,
        0x0ac5d11e1a252bf5uL,
        0x448a6d7d18831887uL,
        0x0bc3e296303b2297uL,
        0x44880dcd6603df1auL,
        0x08421f47009f43ceuL,
        0x4485e4062d5b6a4duL,
        0x0f4083768c5e4542uL,
        0x4483eb6ef47c2758uL,
        0x03be1777d831265fuL,
        0x44821fb7a81c5444uL,
        0x013b69f10b0191b5uL,
        0x44807cefb734d68auL,
        0x0bb8f8a3a05b5b53uL,
        0x447dfefbdb19ac7euL,
        0x03c6be573c40c8e7uL,
        0x447b4831fb12343fuL,
        0x0bc4b645ba991fdbuL,
        0x4478cf81557d20b6uL,
        0x01c2dc119095729fuL,
        0x44768f6f0feb4754uL,
        0x0ec12bbcfa4d62deuL,
        0x447482fa78c40635uL,
        0x043f4343c7d504b9uL,
        0x4472a59289a484fbuL,
        0x03bc74d4fe1e0e8buL,
        0x4470f30c4d0be5bfuL,
        0x0e39e614ecbf4af6uL,
        0x446ecf3428c48d4euL,
        0x05c791716475420cuL,
        0x446bff86d9ec8499uL,
        0x014571d34563050auL,
        0x446970bb87f4ae13uL,
        0x09c3829407a207d8uL,
        0x44671d0b55b79b86uL,
        0x01c1bf74244aed5auL,
        0x4464ff315d036fbduL,
        0x06c024924c7520d1uL,
        0x4463125f6a3d2570uL,
        0x083d5cc6ba567f29uL,
        0x44615233ae8815f1uL,
        0x113ab3560167ccaauL,
        0x445f755ea760487cuL,
        0x06c846e9dda7a163uL,
        0x445c905bbd9ab5a6uL,
        0x0346121d7db32bdduL,
        0x4459eebaa0589b4euL,
        0x034410047ead6894uL,
        0x44578a6de0f41b88uL,
        0x0b423c2090cdde78uL,
        0x44555df1790f2f61uL,
        0x03c09257fca001c0uL,
        0x4453643ec463a3cfuL,
        0x023e1dd9ec677783uL,
        0x445198c18435598duL,
        0x053b5ceb5a13221buL,
        0x444fee9bab9f4e13uL,
        0x0648dbaa11de2037uL,
        0x444cf82e0eb6196auL,
        0x06c694680a9a3ee6uL,
        0x444a474e7029a919uL,
        0x05c481f73b3778e8uL,
        0x4447d5af6513e2beuL,
        0x01c29f9e7d8fd094uL,
        0x44459d93e1d8f57cuL,
        0x0bc0e90f64b5b103uL,
        0x444399c279e4699auL,
        0x07beb4b9e47b58c9uL,
        0x4441c579bbca6885uL,
        0x043bdfe62f60dd70uL,
        0x44401c659160612duL,
        0x06b94d1de5c4576fuL,
        0x443d352b1ae2693fuL,
        0x0746f66f6ab90c3cuL,
        0x443a78e8252c204cuL,
        0x08c4d67050b31c2auL,
        0x4437fd7c80f3410duL,
        0x0cc2e8318008cf89uL,
        0x4435bcf92cc55d85uL,
        0x07c1273463a1589buL,
        0x4433b1f876b10da6uL,
        0x09bf1ec20afad0e2uL,
        0x4431d791bb1324a1uL,
        0x063c39fa0d4a5a2buL,
        0x4430294e37abcee8uL,
        0x09b99946bf7e02a1uL,
        0x442d463db5fa3c13uL,
        0x02473679b24aeb9buL,
        0x442a82a5f4047a5buL,
        0x03c50bf2558ab78fuL,
        0x4428011fb05fe08fuL,
        0x084314916abfa1eauL,
        0x4425bb91decf8a58uL,
        0x04c14bad9006f53buL,
        0x4423ac71ce35c1d2uL,
        0x0dbf5a1196b5bb2euL,
        0x4421ceb656955c59uL,
        0x02bc698e001f6d30uL,
        0x44201dcc2acf7754uL,
        0x1039beca74b0f147uL,
        0x441d2b166911c178uL,
        0x04c753637caac6d9uL,
        0x441a6459c5b11342uL,
        0x0345218993857afcuL,
        0x4417e086accc805cuL,
        0x074323f3f19cff3euL,
        0x44159962aef547b2uL,
        0x09c155d47fdb9c94uL,
        0x4413894608650edduL,
        0x08bf6599b70323cauL,
        0x4411ab0e4d284f44uL,
        0x01bc6dc8a4bb3ba6uL,
        0x440ff4248ebb8298uL,
        0x0749bcfd83a431e9uL,
        0x440ce42dd8e4fa23uL,
        0x04474ca889bbacd5uL,
        0x440a1e8aa1400997uL,
        0x01c516d33e26c040uL,
        0x44079c430435a7fcuL,
        0x04c31612a7ef535fuL,
        0x440557046eb39248uL,
        0x07c1457ab75c2489uL,
        0x440349127b59b216uL,
        0x0abf41259c9550c0uL,
        0x44016d392dff5104uL,
        0x013c46969ca99a2euL,
        0x43ff7d80dc993f2fuL,
        0x02c993e82b76e726uL,
        0x43fc72c149cb214buL,
        0x03472267ac1b25a0uL,
        0x43f9b270c24cc8f9uL,
        0x0b44ec0062aeeb78uL,
        0x43f73585df7b6643uL,
        0x0542eb2d18a2081buL,
        0x43f4f59f9910367duL,
        0x0cc11aeb0b11d1a1uL,
        0x43f2ecf5b7f6abe2uL,
        0x0e3eed5c0bbf1061uL,
        0x43f1164ab45aa234uL,
        0x0dbbf4ab21b4f3f0uL,
        0x43eed9bdbc6f1b09uL,
        0x06c944462d4d5991uL,
        0x43ebd8c96533b39auL,
        0x09c6d561de54f6a1uL,
        0x43e921ec84d5860euL,
        0x00c4a1d472804fc8uL,
        0x43e6ae172414ceb9uL,
        0x0d42a406e25fcb44uL,
        0x43e476e3b661be8buL,
        0x0e40d6e7662dda9duL,
        0x43e276873924f0b3uL,
        0x0a3e6bba6770e22duL,
        0x43e0a7c2c9322f58uL,
        0x0b3b797ab2ba22d2uL,
        0x43de0bad18c4e37duL,
        0x01c8cf813910fdcduL,
        0x43db18eba0be4d24uL,
        0x054666f488db6e0auL,
        0x43d86f7884e1caacuL,
        0x0944399f7770045fuL,
        0x43d608484d592327uL,
        0x08c241e1ebbbf4ecuL,
        0x43d3dcfaee52a8f4uL,
        0x09407aa30ce6a5a0uL,
        0x43d1e7cbac093f26uL,
        0x09bdbe8969a24c6fuL,
        0x43d023827dc88ed8uL,
        0x113ad7301258d788uL,
        0x43cd16cd999791c2uL,
        0x08c837a640fa9d3duL,
        0x43ca3666de0788b0uL,
        0x00c5d90f358d61f6uL,
        0x43c79e17816df1e8uL,
        0x0343b5342f7be9c0uL,
        0x43c546e385224d10uL,
        0x00c1c674ecd152d3uL,
        0x43c32a7a483e977buL,
        0x05c007b997a0b531uL,
        0x43c1432649c86c4cuL,
        0x0b3ce8cc007a6432uL,
        0x43bf177ce0bd5835uL,
        0x054a109c0bccbc39uL,
        0x43bbff3166bc36eeuL,
        0x03c77f5624913c3auL,
        0x43b934fc0975fb2fuL,
        0x0a452e251d5d3b1fuL,
        0x43b6b13ebb9a5ad4uL,
        0x054316da780bc4d9uL,
        0x43b46d17a80cc174uL,
        0x014133deb1d35260uL,
        0x43b2624f3a0a886fuL,
        0x08bf00460b24acf8uL,
        0x43b08b47d7733cb5uL,
        0x0fbbee2903d584f9uL,
        0x43adc5de496b1810uL,
        0x01492920a7c80e26uL,
        0x43aac9615b3c9fd7uL,
        0x0446a9b25345c773uL,
        0x43a818d3a356669duL,
        0x0844691b26b9c82fuL,
        0x43a5acbdab2ed713uL,
        0x01c2613e9610f6d1uL,
        0x43a37e61fd4c0fe0uL,
        0x00408c969adf0beauL,
        0x43a187ab3d71db11uL,
        0x003dcc4ac4f59be5uL,
        0x439f8637ea4e52abuL,
        0x094ad2d0a9a18288uL,
        0x439c577fd709b098uL,
        0x0a482498a7cc94b9uL,
        0x43997a3dc62119c8uL,
        0x02c5ba462dee8a02uL,
        0x4396e66137bb7cc9uL,
        0x09438d330d8806a0uL,
        0x439494a3f6a9a70duL,
        0x0c41975e0627306cuL,
        0x43927e767bb79ea1uL,
        0x0d3fa6b5ee8f3088uL,
        0x43909dee32687729uL,
        0x07bc78892308bd90uL,
        0x438ddb6ae2f39380uL,
        0x08499b5ec6741cb3uL,
        0x438ad1f9fba4b2abuL,
        0x0147073c400e10dcuL,
        0x438816dde4c11ca2uL,
        0x0bc4b4ee0b3a84d6uL,
        0x4385a245d5e5289cuL,
        0x03c29df4862ac231uL,
        0x43836d26a686daaeuL,
        0x0c40bc7294e0cbafuL,
        0x438171277cbbce9cuL,
        0x00be163bd8df8640uL,
        0x437f5120b45c00e5uL,
        0x084b0a61bce91993uL,
        0x437c1c74b30d0bb0uL,
        0x03c84cbb00f925f0uL,
        0x43793b02e5cf0324uL,
        0x0445d5841ce6cb73uL,
        0x4376a46f43f3118cuL,
        0x04439dbcd485dd07uL,
        0x43745132973bb79auL,
        0x0b419f153b38a108uL,
        0x43723a85891dc72buL,
        0x043fa7b9159fc471uL,
        0x43705a4dba466c4euL,
        0x033c6de3429e31fauL,
        0x436d561964307dc3uL,
        0x06498769faac8a1buL,
        0x436a4fa0f13737e7uL,
        0x0846ebf82977acf0uL,
        0x4367984b636ad1beuL,
        0x03c4940bc89fa5aauL,
        0x4365281628cb373auL,
        0x014278e135bcf0a4uL,
        0x4362f7cc38bc628duL,
        0x0540946088b6f8eduL,
        0x436100f1aef8eaf5uL,
        0x073dc21972b9e9f4uL,
        0x435e7b62ce66acdeuL,
        0x044ab3e8cfada51auL,
        0x435b5198cf325114uL,
        0x0047f5483f729c27uL,
        0x43587b15da6677aeuL,
        0x0a457e33e2b1c6dauL,
        0x4355ef5de2e68984uL,
        0x0c43477480d89e25uL,
        0x4353a6d00852a688uL,
        0x05414a8b54629fb2uL,
        0x43519a90b14f53aeuL,
        0x0d3f033fa073d520uL,
        0x434f88eba04114cbuL,
        0x034bcede5acc0d40uL,
        0x434c3dea36b87936uL,
        0x07c8ee7b29d0b081uL,
        0x43494a28136fa731uL,
        0x03c659917bbb6632uL,
        0x4346a4b2c9663fa1uL,
        0x02440877b79cd868uL,
        0x43444580945b8451uL,
        0x0b41f44979177348uL,
        0x43422558f1aa9f03uL,
        0x034016d3f0358160uL,
        0x43403dbf8db89298uL,
        0x06bcd508600d0ba8uL,
        0x433d11c2965639f6uL,
        0x0249d4ae77a21604uL,
        0x433a03065db54a4buL,
        0x04c723974e9529d8uL,
        0x433745e6013d8cf2uL,
        0x08c4b9a944f57915uL,
        0x4334d1f2eb853100uL,
        0x03428f9c9b769ee3uL,
        0x43329f9b7c4f56dfuL,
        0x06c09ee66b6e99e9uL,
        0x4330a814a1dfc5eduL,
        0x033dc34b6999ff72uL,
        0x432dca8b63e38fa9uL,
        0x044aa5249b4cca57uL,
        0x432aa36c9242f8bcuL,
        0x02c7d9db080918bauL,
        0x4327d0fbfa6c3c19uL,
        0x044558e88e8945efuL,
        0x43254a6b679dd96euL,
        0x0d431aa564e92066uL,
        0x432307d4e71272d7uL,
        0x05c11831a9c3763duL,
        0x4321022313b11381uL,
        0x083e96c265c21fbfuL,
        0x431e65f78e13edccuL,
        0x09cb5d52c19374feuL,
        0x431b2959e487c93fuL,
        0x02c87a2188252d5fuL,
        0x43184436cf62b6f7uL,
        0x07c5e440cc8caaf9uL,
        0x4315ad66c67f3f63uL,
        0x04c393ad199301deuL,
        0x43135cb549c616eauL,
        0x0d418135a0647102uL,
        0x43114ac7e9322a19uL,
        0x0fbf4ccd98eab06buL,
        0x430ee20fae75a2c5uL,
        0x00cbfaedff2748c1uL,
        0x430b931b883c77f2uL,
        0x0449026a7e3c9538uL,
        0x43089e1f8e1d4be6uL,
        0x02c659f3419269eeuL,
        0x4305f9a24050e89fuL,
        0x0643f92e9472ca4cuL,
        0x43039d2746cbe57euL,
        0x0cc1d89fb6602df9uL,
        0x43018115431b6c49uL,
        0x0abfe32077e095c4uL,
        0x42ff3d3ca19edf64uL,
        0x004c7bf775863df5uL,
        0x42fbdf55dd9bdce0uL,
        0x044970fb0b5580dcuL,
        0x42f8dd8e25d2255cuL,
        0x0646b88087e4af9fuL,
        0x42f62e225ebca190uL,
        0x00c449de67f2c6b2uL,
        0x42f3c855ef212bacuL,
        0x0ac21d51dc348d4duL,
        0x42f1a4576cd5cddcuL,
        0x05402be7023a443euL,
        0x42ef765035c713d8uL,
        0x00ccdec7155697e1uL,
        0x42ec0d0bdeb46ae2uL,
        0x0349c4671c1a6e3cuL,
        0x42e901afbd3819bduL,
        0x08c6feb0af26f865uL,
        0x42e64a386137b955uL,
        0x02c484b1e63b3be4uL,
        0x42e3ddb15521ce48uL,
        0x08424e68a1458bd7uL,
        0x42e1b418ba2217c5uL,
        0x08c054a9a7c2f05auL,
        0x42df8c8bad8e2a1fuL,
        0x06cd2214ad33ca5euL,
        0x42dc1ba4950b8f4fuL,
        0x01c9fb9933adac68uL,
        0x42d90a0b40dd690buL,
        0x0ac72b99eccc462euL,
        0x42d64d860502b278uL,
        0x06c4a8e4dbe3539cuL,
        0x42d3dcf1aadc099cuL,
        0x09426b4018ef81f7uL,
        0x42d1b02414a73357uL,
        0x05406b4fe82cc6aeuL,
        0x42cf7fa3e4bec2aeuL,
        0x044d44feffb34893uL,
        0x42cc0aee6d6b1406uL,
        0x00ca15d86bb23572uL,
        0x42c8f684065398bfuL,
        0x04c73ea5ac0d71a9uL,
        0x42c637ff9397e988uL,
        0x0bc4b5fdd0f567fauL,
        0x42c3c618d3c706ebuL,
        0x0242737769828878uL,
        0x42c1988625955723uL,
        0x06c06f8da87263ceuL,
        0x42bf4fc2f6d50e41uL,
        0x044d4710a9e149eduL,
        0x42bbdb204ff1cda2uL,
        0x08ca12cc7b1bf616uL,
        0x42b8c75a6fa17116uL,
        0x01473793d6253bd7uL,
        0x42b609ec277b8703uL,
        0x0244abd0af44c7f8uL,
        0x42b399725d96eb62uL,
        0x094266f2e981ccfbuL,
        0x42b16d8d1241b86auL,
        0x09406154a07d21a2uL,
        0x42aefd875a51d28cuL,
        0x07cd2842b40e25f0uL,
        0x42ab8cd873c4de71uL,
        0x0949f27fa465d061uL,
        0x42a87d2a89e5ac64uL,
        0x0647167c3937ded9uL,
        0x42a5c3e42539c768uL,
        0x0cc48a7fb96552cauL,
        0x42a35791e04cd29fuL,
        0x05c245dcbaa25b1buL,
        0x42a12fc6cdafd10duL,
        0x06c040d4ab2de626uL,
        0x429e8a0077a1ed46uL,
        0x074ce8fcb8dadc2cuL,
        0x429b2118f75a4eb6uL,
        0x07c9b55e7c11d9e6uL,
        0x429818e8b1c2616fuL,
        0x04c6dbce02ec5c77uL,
        0x429566cdf4525eb0uL,
        0x02c4527acab6dfebuL,
        0x4293014fd204bc70uL,
        0x08c210a3ddcb4706uL,
        0x4290dffe0bfc0c74uL,
        0x06400e7aba6527c9uL,
        0x428df6a8d5e14f10uL,
        0x07cc8a12a152d814uL,
        0x428a9942579915cduL,
        0x01c95c35893651c9uL,
        0x42879bdc576e403auL,
        0x0346884d52cc9914uL,
        0x4284f3d9114d799auL,
        0x0744047ce663f641uL,
        0x428297c4e6eb62fbuL,
        0x09c1c7f9c74f3e7cuL,
        0x42807f35ef1a4fbfuL,
        0x0a3f95dcee779f74uL,
        0x427d455e0a3b0d94uL,
        0x03cc0cc007cc808euL,
        0x4279f70bf04a77ceuL,
        0x00c8e82cd2a6133cuL,
        0x427707990a8defeeuL,
        0x0ac61d0ef76712e4uL,
        0x42746c779ebb14aeuL,
        0x0143a1882865d26euL,
        0x42721c4420bc9879uL,
        0x00416cce86450b20uL,
        0x42700ea48df1e7fauL,
        0x0d3eee1d41e1e516uL,
        0x426c7856a7693627uL,
        0x034b72a1658393d4uL,
        0x42693c7abef59a2cuL,
        0x01c85ac17b553c4fuL,
        0x42665df602b1e0feuL,
        0x0a459b72775450f3uL,
        0x4263d256a5ee461duL,
        0x04432ae03812fc00uL,
        0x42619053bac5f644uL,
        0x0a41004b9cd4bae6uL,
        0x425f1f58fe66e142uL,
        0x044e27d88d5289bfuL,
        0x425b9216793da421uL,
        0x084abdab3fb224ceuL,
        0x42586bd6adace04euL,
        0x0247b5bd9f52a89euL,
        0x4255a104640aeb73uL,
        0x07c5051a941eb130uL,
        0x42532755417b50dcuL,
        0x0842a20366f6a0deuL,
        0x4250f5a5274f5c45uL,
        0x03c083cdb1163405uL,
        0x424e07ab300dc4b9uL,
        0x02cd458a013d18b4uL,
        0x424a956163a49612uL,
        0x08c9f01f97b2e043uL,
        0x4247879eb52380eduL,
        0x02c6fb2eaf7d8102uL,
        0x4244d30488394e18uL,
        0x00c45be480207b14uL,
        0x42426d7af2869fc4uL,
        0x0ac208a2b041836euL,
        0x42404e0c593552f4uL,
        0x093ff1ba8cbc9c8duL,
        0x423cd98a274acae2uL,
        0x074c49f8a8ec4aebuL,
        0x4239852d44d7528auL,
        0x0a490c81ede57558uL,
        0x4236927c2c3e496fuL,
        0x07462d5a948b6358uL,
        0x4233f65a98c177c9uL,
        0x0343a1de0952fd2buL,
        0x4231a6ed66936ee9uL,
        0x0a416098d4b94692uL,
        0x422f36ed3084aa81uL,
        0x00cec24d6a8bc072uL,
        0x422b986ab7ebdd53uL,
        0x094b3828ebcc128buL,
        0x422864933f3c0573uL,
        0x01c8158a3038115euL,
        0x42258f359f0c4e8euL,
        0x0bc54eb3e9a3e72buL,
        0x42230d82cb8a968buL,
        0x08c2d93b0174f61auL,
        0x4220d5e5f59de7c1uL,
        0x01c0abe0d45fd5c2uL,
        0x421dbfc240ab5f81uL,
        0x00cd7ce33a39bd89uL,
        0x421a47db588b15cfuL,
        0x03ca134d30d655e4uL,
        0x421736c0d0a31187uL,
        0x03c70e16f315ef40uL,
        0x421480a1879e8f56uL,
        0x0cc461cda38e2783uL,
        0x42121b0591ce1cfcuL,
        0x0c42044a2faebb7buL,
        0x420ff94e3fca1752uL,
        0x02cfd91813f8cc8cuL,
        0x420c3a9f9558ffa0uL,
        0x034c2530177987feuL,
        0x4208eb738c76b2f1uL,
        0x07c8deb61106f334uL,
        0x4205fee91a43fef0uL,
        0x09c5f91f55e86346uL,
        0x4203699940a6a810uL,
        0x0a43694e7b13691buL,
        0x4201216c07263de0uL,
        0x02c1256a18de488buL,
        0x41fe3ae49fef5534uL,
        0x074e49705a5ebd5fuL,
        0x41faab87fb8e4440uL,
        0x08cabefb3186e784uL,
        0x41f786c3dca158c4uL,
        0x04c79dc285401b7duL,
        0x41f4c036b7451222uL,
        0x09c4d9a4f359ba1euL,
        0x41f24cec8453db03uL,
        0x014267e46fd85893uL,
        0x41f02334e92993b8uL,
        0x08c03efdea0a0506uL,
        0x41ec74fc41217dfbuL,
        0x044cad0afbb569b1uL,
        0x41e9166837399531uL,
        0x06494e0d5e7a8744uL,
        0x41e61d46c11dd915uL,
        0x07c653d077d9eef0uL,
        0x41e37dbe7711fcd3uL,
        0x08c3b2a639494566uL,
        0x41e12d55c1e73c64uL,
        0x0b416038b4af0a0euL,
        0x41de4594b115943auL,
        0x07cea6c598920c48uL,
        0x41daabdabdb93483uL,
        0x094b081aaf25ade1uL,
        0x41d77f073eb945dfuL,
        0x0047d62079a4e4a6uL,
        0x41d4b252d0bc8beauL,
        0x0b45042e1a8664eduL,
        0x41d23a7345c57ccauL,
        0x034287117d29a9e6uL,
        0x41d00d6f8a57f06euL,
        0x064054e44f8ee735uL,
        0x41cc44f136cf3bd8uL,
        0x014cc9cbc5fe04a8uL,
        0x41c8e38df2790b7auL,
        0x03495eb2cb828067uL,
        0x41c5e8f828661e20uL,
        0x06465acfefcd0029uL,
        0x41c3490e7e2bc31cuL,
        0x05c3b20c56ad84f5uL,
        0x41c0f91b7ff9bb2auL,
        0x02c159b917beb87auL,
        0x41bddf56913a541euL,
        0x024e90cb5cac7057uL,
        0x41ba48cc1b8a7bc7uL,
        0x00caeb7659e5f7efuL,
        0x41b71fde01e2ca8cuL,
        0x03c7b4b752e86e5fuL,
        0x41b4578e0b906b32uL,
        0x0244df8ace15322euL,
        0x41b1e4659a2a2155uL,
        0x09c26072a17961a0uL,
        0x41af788fc218597buL,
        0x02502d48c75e7d9buL,
        0x41abac92daac0b9cuL,
        0x074c7a2ecd5f05a0uL,
        0x41a85518c3484796uL,
        0x00490feaede7f2aeuL,
        0x41a56441b55bfff1uL,
        0x05c60dcef1cedc3auL,
        0x41a2cdd203ab43a1uL,
        0x06436787980e7387uL,
        0x41a08700c199ad4euL,
        0x0b4112346e13dd7euL,
        0x419d0c9857c390f3uL,
        0x00ce087915129a98uL,
        0x419986a650394094uL,
        0x05ca6a5096da5b7duL,
        0x41966d6688315ad5uL,
        0x05c73aff07c7874euL,
        0x4193b3d55ebd8547uL,
        0x04c46d572e10e216uL,
        0x41914e7b714e7093uL,
        0x0741f5ba17e5a90buL,
        0x418e667d9a8bcd9euL,
        0x01cf93d0d186fbcduL,
        0x418ab2733e383ad7uL,
        0x07cbc1b22cec72b0uL,
        0x4187712b76c8c7f6uL,
        0x01486529e9df069cuL,
        0x418494d8e1d4fc60uL,
        0x0c45702d052bf73auL,
        0x4182115447c6627cuL,
        0x0d42d65aee08874cuL,
        0x417fb7d503fc65c7uL,
        0x07d08ccb49580d43uL,
        0x417bd660913b938buL,
        0x084d13c32a98512buL,
        0x41786db66e158524uL,
        0x03498a4bfd5a5faduL,
        0x41756f3ed5aa4222uL,
        0x05466e459a7794f4uL,
        0x4172ce2265a96beeuL,
        0x0cc3b28bbce3c1c6uL,
        0x41707f14a8d0c116uL,
        0x00414b8b6b67144euL,
        0x416cf049ebedf60cuL,
        0x06ce5e26dbef0e28uL,
        0x41696129ca292f7duL,
        0x08caa854b5c4f131uL,
        0x4166416763f6b3bcuL,
        0x04c765d329106241uL,
        0x4163837bf030f4a7uL,
        0x094488b9479ee1c4uL,
        0x41611b82880134f9uL,
        0x05c204c8d940530buL,
        0x415dfe0c1b8af1f3uL,
        0x014f9e77238e0031uL,
        0x415a49aa1651cfc9uL,
        0x054bbd2c8fd7e193uL,
        0x415709b5a3a79127uL,
        0x07485502f16a0f8duL,
        0x41542ffa7e9ace3euL,
        0x0b45574ceffe3945uL,
        0x4151affd2eccd615uL,
        0x07c2b72182c97af5uL,
        0x414efd8be43ac9a8uL,
        0x05d06925da53a0fcuL,
        0x414b2564005de7e5uL,
        0x00ccc6bb6d71090duL,
        0x4147c694cd2b4ffduL,
        0x04c93a02d0c97221uL,
        0x4144d23fa69bd813uL,
        0x07461cb1a027e057uL,
        0x41423b556e6e918euL,
        0x01c361358dd1f243uL,
        0x413fecbcf04dca8fuL,
        0x05d0fba0d2660d89uL,
        0x413bf29264dcdc82uL,
        0x01cdc2ef387bd0e0uL,
        0x4138767d7fc43eb6uL,
        0x014a130711aadcdauL,
        0x413568f9937abc78uL,
        0x0ac6d758e1ac9659uL,
        0x4132bc67d8c20136uL,
        0x054401abca024479uL,
        0x413064d4616b0093uL,
        0x0d4185819a7f8c6auL,
        0x412caf8458ad2a11uL,
        0x05ceafc2b00a99b1uL,
        0x412917faff93e540uL,
        0x034ade505ba61e89uL,
        0x4125f2e79283b1c9uL,
        0x094785c00b5cb27euL,
        0x41233220b1da4f59uL,
        0x0444973634932c1auL,
        0x4120c93ac678b0ccuL,
        0x04c205a7d78be568uL,
        0x411d5aa313452daeuL,
        0x03cf8b4440d68221uL,
        0x4119a9b05368c88buL,
        0x024b9a31a7b9868cuL,
        0x41166ede7f0c2d54uL,
        0x064826756e1a42e2uL,
        0x41139b7fc18e5891uL,
        0x0145209676e4b424uL,
        0x411122b662569615uL,
        0x0bc27b019965e362uL,
        0x410df2779ceabfc7uL,
        0x075029ce648133fduL,
        0x410a2a5d2945d2b6uL,
        0x04cc45161cd95fe8uL,
        0x4106dbccf848794auL,
        0x02c8b81d680cdfc5uL,
        0x4103f79bf21caa95uL,
        0x08459ca24a7521dduL,
        0x41017080ae674895uL,
        0x0742e48f266999cfuL,
        0x40fe75b024885f53uL,
        0x04d0838b13324d03uL,
        0x40fa98e26924c6c7uL,
        0x054cdd86b83e679duL,
        0x40f738bf4bc8d296uL,
        0x01493977456406dduL,
        0x40f445a6a9a273c6uL,
        0x00c60a47aca18e96uL,
        0x40f1b1eabeffc3a4uL,
        0x08c341669953fe1cuL,
        0x40eee324e1fde417uL,
        0x03d0d210b765b3d6uL,
        0x40eaf4465e9c5668uL,
        0x01cd622fa53c02ceuL,
        0x40e784e3008fb46buL,
        0x0249a961d6383ef7uL,
        0x40e484eecd2f1383uL,
        0x05c66890cd0bf55fuL,
        0x40e1e65fd1ef2701uL,
        0x024390b73f2a4fb0uL,
        0x40df39dc6baaccd6uL,
        0x04d114ae59581395uL,
        0x40db3bb863d26277uL,
        0x04cdd1e5296953a3uL,
        0x40d7bf89f052b590uL,
        0x07ca06dfa21b6c59uL,
        0x40d4b4e35dbe0cdduL,
        0x0146b6a7a27c9005uL,
        0x40d20d6781986166uL,
        0x0b43d1cca3d4f6d8uL,
        0x40cf790f6877f51euL,
        0x02d14acc164c64feuL,
        0x40cb6e93fa7299b2uL,
        0x054e2ba80b9c3a1buL,
        0x40c7e82cde922832uL,
        0x074a511aa3827999uL,
        0x40c4d515a14a6132uL,
        0x01c6f3d9139319eduL,
        0x40c226a790f97768uL,
        0x01c404113d7d18e6uL,
        0x40bfa02b8ac73415uL,
        0x05d173ed60fcd6fauL,
        0x40bb8c6342337220uL,
        0x044e6ea95e92c624uL,
        0x40b7fe6d7fbcef2buL,
        0x084a8767775dd309uL,
        0x40b4e53acc7531b1uL,
        0x00c71f97a2983044uL,
        0x40b231e547065724uL,
        0x01c42710a88aab19uL,
        0x40afaed5c4559717uL,
        0x01518fb2ded8ebb1uL,
        0x40ab94e0bfb59934uL,
        0x00ce9a4d9b21386euL,
        0x40a80217e57d8a3fuL,
        0x044aa947efe69879uL,
        0x40a4e52d23cf50afuL,
        0x07c7397d8e2bd385uL,
        0x40a22f0652094ae5uL,
        0x0a443a79684f6ef6uL,
        0x409fa4eba730bf5fuL,
        0x03d19ddbd8138a90uL,
        0x409b87f86a26fad6uL,
        0x05ceae2ef93df996uL,
        0x4097f323487ff94auL,
        0x04cab66cfccafb75uL,
        0x4094d4ec8ea8ee66uL,
        0x0747414e5b5ca43cuL,
        0x40921e112e39bf18uL,
        0x04443e1e22ebfdb4uL,
        0x408f8283ec45f116uL,
        0x04d19e4732be2ff0uL,
        0x408b65c7f9f1fbecuL,
        0x064eaa1efb3b003euL,
        0x4087d1b22b6810f5uL,
        0x08caaeb7de6855e2uL,
        0x4084b49e984886e0uL,
        0x01c736f7c0d13f06uL,
        0x4081ff2d0d5a2649uL,
        0x05c431f651be2ff4uL,
        0x407f47ee1cab73dcuL,
        0x06d190f3f39e9af4uL,
        0x407b2e9e76c8d9f8uL,
        0x084e8e2722ca46cfuL,
        0x40779e11d635b9a6uL,
        0x08ca923a9d8d5019uL,
        0x4074848ddf7dfffeuL,
        0x01471a91ee04e82cuL,
        0x4071d2a13fdd2709uL,
        0x04c4161e6298ed3auL,
        0x406ef5b15f732009uL,
        0x075176014201ab17uL,
        0x406ae2fb07705cc2uL,
        0x06ce5a88cbf394e4uL,
        0x406758b92cdfdc63uL,
        0x07ca6137c537bf6duL,
        0x40644528f79b1b50uL,
        0x09c6ec5f2d1367f4uL,
        0x406198d422be3f8cuL,
        0x0243ead7491061afuL,
        0x405e8c8a7276c930uL,
        0x02514dadee76975auL,
        0x405a838b09afcf61uL,
        0x04ce0fbc2ec572b9uL,
        0x40570246e766d2f2uL,
        0x064a1c215fcd0beauL,
        0x4053f700c0d99875uL,
        0x0ac6accae115453euL,
        0x4051524997d01a00uL,
        0x0143b08582357e32uL,
        0x404e0d68d9047f79uL,
        0x075118577f06b2f2uL,
        0x404a11277ca2bd3euL,
        0x054dae6e8d292a1euL,
        0x40469b7f34ec048euL,
        0x02c9c3973d4c9b08uL,
        0x40439ac6410ceb62uL,
        0x08c65c67e684d1e6uL,
        0x4040ffa110b113f0uL,
        0x01c367af901b1370uL,
        0x403d796b4f7aaf7fuL,
        0x03d0d678c614f535uL,
        0x40398cd1cb38dcc0uL,
        0x02cd377f96b9fd62uL,
        0x40362548d6675834uL,
        0x084958648bd60350uL,
        0x403331480815e7ccuL,
        0x0945fbee5e7590f4uL,
        0x4030a19336cc73a0uL,
        0x084310fbf558eca2uL,
        0x402cd1db96a6c6eeuL,
        0x065088a80b837328uL,
        0x4028f7b007e1de49uL,
        0x024cabfe10b3371auL,
        0x4025a0a9c047e3c7uL,
        0x0148db7ccf7600f4uL,
        0x4022bb6f2dd8e253uL,
        0x0b458c38f07b7c3buL,
        0x402038ef3cbdc1c7uL,
        0x0642ad2ebb6268bduL,
        0x401c1829acfb62b3uL,
        0x01d02f94d1fb1ba4uL,
        0x40185308ad209551uL,
        0x03cc0d23d3daadaduL,
        0x40150ec3549a202duL,
        0x01c84df8496cc3aeuL,
        0x40123a3bf963c1ebuL,
        0x03450e4191e1b76cuL,
        0x400f8d2fce0ebb40uL,
        0x06d23d2690dc7344uL,
        0x400b4de68e608347uL,
        0x01cf980a88588961uL,
        0x4007a03df8f9f479uL,
        0x004b5c5135a44acbuL,
        0x400470ce4924af72uL,
        0x0447b10fe1f0aeaauL,
        0x4001aec242758b4euL,
        0x0a44831de32e25bduL,
        0x3ffe9700b697ec95uL,
        0x0651c1d98f1b1f71uL,
        0x3ffa74be9568f921uL,
        0x064ebda6af103d07uL,
        0x3ff6e0c8fadbb04fuL,
        0x09ca9b07f491a273uL,
        0x3ff3c8164e42f29buL,
        0x07c70618a9c019dauL,
        0x3ff11a259faba91duL,
        0x0843ebfb36da371buL,
        0x3fed91518c2acaf6uL,
        0x01513c51b7852ec0uL,
        0x3fe98e739a118b5euL,
        0x034dd1d36683753buL,
        0x3fe616346ca3be0euL,
        0x0249cae5c1f5de61uL,
        0x3fe315f58c13df9buL,
        0x09464e7f0a95542fuL,
        0x3fe07d957435b8c3uL,
        0x09434a1a5595e9cbuL,
        0x3fdc7e35cf4db634uL,
        0x0150ada93ac2688euL,
        0x3fd89cd6ead31b71uL,
        0x014cd680d6a376d2uL,
        0x3fd542176fe1c2b2uL,
        0x0548ed9e84be9bacuL,
        0x3fd25bd00bd97edduL,
        0x03c58bc1beb8e117uL,
        0x3fcfb491e02b7c14uL,
        0x03d29ecb15514182uL,
        0x3fcb5fcd30c7e1f5uL,
        0x065017069c4b54cfuL,
        0x3fc7a1c33cc1922auL,
        0x054bcdb33f7b88f9uL,
        0x3fc46610483f2394uL,
        0x064804f671a7a35cuL,
        0x3fc19b0f23241b87uL,
        0x09c4bf6ca87a4707uL,
        0x3fbe62f62b4555dcuL,
        0x02d1eb67d8a75351uL,
        0x3fba383ca9f98a0fuL,
        0x034ef3318a5788deuL,
        0x3fb69f16aeb3676fuL,
        0x084ab97c2106c4d2uL,
        0x3fb383bf2b37a036uL,
        0x064712bc1550fb6auL,
        0x3fb0d51cf5a16253uL,
        0x09c3eb13a24821e2uL,
        0x3fad08cdac87dce5uL,
        0x045131510c1da6aduL,
        0x3fa909a7c3ac6f98uL,
        0x064dad26311e9ef0uL,
        0x3fa596acfa0bcc8euL,
        0x0a499bf36c7ef068uL,
        0x3fa29cc13bfd539fuL,
        0x06c618c26c1169a6uL,
        0x3fa00b60212cf112uL,
        0x0cc3104d5f799552uL,
        0x3f9ba886ae6e40e0uL,
        0x015071e8b6003b16uL,
        0x3f97d62a282a4850uL,
        0x064c5e5338097f6buL,
        0x3f948a59e9cb1eb0uL,
        0x09c87730de08c821uL,
        0x3f91b2abc895a770uL,
        0x07c518db221cf8bauL,
        0x3f8e7e6f4c33edecuL,
        0x055230ae74a714aauL,
        0x3f8a4480db60fe17uL,
        0x03cf5d1c58fdc6acuL,
        0x3f869fd19aacb90auL,
        0x00cb091a88a72f08uL,
        0x3f837be42e1159e7uL,
        0x02474d459ba38afeuL,
        0x3f80c707db025298uL,
        0x03c414d114bdcde1uL,
        0x3f7ce3ee3757dbe5uL,
        0x01d14dc49cbc0c30uL,
        0x3f78df06bfb34f6cuL,
        0x08cdd13408401cdcuL,
        0x3f7568986affafc5uL,
        0x02c9afd0eca1593duL,
        0x3f726d009f5af049uL,
        0x0246203633a6814auL,
        0x3f6fb69c5d6b524euL,
        0x03530e632b0008c9uL,
        0x3f6b49c67cd1611fuL,
        0x035069124dc6eaefuL,
        0x3f677a47ec4e9fa0uL,
        0x07cc42b48d5cfe42uL,
        0x3f643260788f0a1fuL,
        0x01c854b792c33d4auL,
        0x3f615f4e018a09eduL,
        0x0bc4f1f511f7b2d7uL,
        0x3f5de1c72f739a48uL,
        0x04d2073f996519c0uL,
        0x3f59b25dc6d6642euL,
        0x034f08155c194aaduL,
        0x3f561853cc8eddabuL,
        0x06cab41e011814e5uL,
        0x3f52feeed430b87buL,
        0x0146f9f62ec4193auL,
        0x3f505451535e8102uL,
        0x0143c45d7f9e2fb0uL,
        0x3f4c122bcbda7f8duL,
        0x06d100ffa10ff0f3uL,
        0x3f481ff0b26f3b69uL,
        0x054d401bee3a7787uL,
        0x3f44bb153d2d0728uL,
        0x034927ce5fbbe352uL,
        0x3f41cfe80beb05a4uL,
        0x0245a195c6e2a08euL,
        0x3f3e9ae566e02485uL,
        0x06d2992f3c7d2ce7uL,
        0x3f3a4a3297375461uL,
        0x00cffa47aef63bd2uL,
        0x3f36948e77b6c536uL,
        0x054b7ccca35ce88euL,
        0x3f33644eed5b1126uL,
        0x04c79ffc3cd6bc92uL,
        0x3f30a6cd27d913d6uL,
        0x0a444d7c3dca9cc8uL,
        0x3f2c97f5c053e774uL,
        0x0551720abf01aa9buL,
        0x3f288c0c973b68fcuL,
        0x034dfa22008cf2c8uL,
        0x3f2512157ee1d8bduL,
        0x0749c08a63df00dcuL,
        0x3f2215988e86b085uL,
        0x08461eb258af5a93uL,
        0x3f1f09f2b684fb31uL,
        0x0252ff68a28f7dc4uL,
        0x3f1aa222a98ba953uL,
        0x0250506e21782262uL,
        0x3f16d9b06046eb65uL,
        0x05cc041afe3a1ad2uL,
        0x3f139a30e3030663uL,
        0x08480d8271e40929uL,
        0x3f10d05cd2b64651uL,
        0x0b44a5cc1e67b046uL,
        0x3f0cd740d2318d4duL,
        0x03d1b8f04bdfa1bfuL,
        0x3f08bb7603d9827fuL,
        0x07ce6b65816f0ff1uL,
        0x3f0534d810db5376uL,
        0x074a1a7ec86c94fbuL,
        0x3f022e56de90dc1auL,
        0x054665a9398034f1uL,
        0x3eff2bb06a7069e2uL,
        0x02d336f30c8d3345uL,
        0x3efab79b6edb04e1uL,
        0x04507b7cbf13abf4uL,
        0x3ef6e5b33b150249uL,
        0x024c461717dacbd8uL,
        0x3ef39f005226a7dbuL,
        0x04c83f56253c12f1uL,
        0x3ef0cfc8192e69bduL,
        0x05c4cab82baddd6cuL,
        0x3eecce310b024fd3uL,
        0x06d1d39d04e50424uL,
        0x3ee8acc81455f971uL,
        0x03ce9094beff3587uL,
        0x3ee522570529739euL,
        0x074a3308036822dbuL,
        0x3ee219685023e1bduL,
        0x07c67464f8a36affuL,
        0x3edeff1f945e7f7auL,
        0x06d33e2c9c277148uL,
        0x3eda89fa515a2b44uL,
        0x00d07d0b7bb52fc7uL,
        0x3ed6b83bb4ee4348uL,
        0x034c40cfbd11fd10uL,
        0x3ed372982e2fde1duL,
        0x014833ffa698fa8buL,
        0x3ed0a51297b20ab7uL,
        0x0244bb29dadf3ac0uL,
        0x3ecc7d093fb7e463uL,
        0x0251c147957723bduL,
        0x3ec8607006600008uL,
        0x04ce6896f5762306uL,
        0x3ec4db1c7b733812uL,
        0x00ca096cc3260668uL,
        0x3ec1d76959a6b622uL,
        0x00c64a7647d3f88auL,
        0x3ebe858d8b3acc7fuL,
        0x05d314deba7bab37uL,
        0x3eba1a94b14e3d7fuL,
        0x0050550e92636252uL,
        0x3eb6529df3d1cf1buL,
        0x064bf46cd0f972c3uL,
        0x3eb316449a955429uL,
        0x02c7ebd49fbb30eeuL,
        0x3eb0517b9e1f89dduL,
        0x08c47796af08285buL,
        0x3eabe627dddb55d7uL,
        0x00d1827a73755ec7uL,
        0x3ea7d8a7f2a8a2cfuL,
        0x054df49a10ccc568uL,
        0x3ea4613bf000c71cuL,
        0x06c99ee7037b652buL,
        0x3ea16a45fcb7b881uL,
        0x07c5e9197017791duL,
        0x3e9dc283bcbe780euL,
        0x04d2bc40c543e36buL,
        0x3e996ca751cac37fuL,
        0x025004b34180a4a9uL,
        0x3e95b7cd13179ddeuL,
        0x04cb632d58444faduL,
        0x3e928cb2cb8b4014uL,
        0x07c768f3e13d3bdcuL,
        0x3e8faedd62dabd95uL,
        0x05d401fa7657909euL,
        0x3e8b0de982dbf111uL,
        0x0151190d162109abuL,
        0x3e87195b2becea19uL,
        0x004d3803e22a78e4uL,
        0x3e83b8387eea3f9duL,
        0x01c8f694ad8ac632uL,
        0x3e80d521f8291cd6uL,
        0x00c55326d6aac6fauL,
        0x3e7cbb9be9cbac1duL,
        0x075236e8d3a9e0e7uL,
        0x3e78852e54d26541uL,
        0x064f1ca221c0b98buL,
        0x3e74ec36b8fdf427uL,
        0x054a914b62872bc3uL,
        0x3e71d9d0055d11cfuL,
        0x09c6af2ae42db580uL,
        0x3e6e74cb7ebdea09uL,
        0x05d35dbe86ed95c7uL,
        0x3e69fa735b03463auL,
        0x01d0880cfe68041euL,
        0x3e6627f6220ca6a8uL,
        0x054c3847cbf78a3buL,
        0x3e62e4d9d8b5b22euL,
        0x09c81550cf271bfduL,
        0x3e601c325e8bb3c0uL,
        0x06448cefa0aac509uL,
        0x3e5b783bc148fcefuL,
        0x015188ab9ce5fddduL,
        0x3e576aa8791eba32uL,
        0x06cdea9996bf1c0fuL,
        0x3e53f58d390caeebuL,
        0x064984c7bb9c53ffuL,
        0x3e510299f255a2cauL,
        0x0145c3c6ce5f2f75uL,
        0x3e4cfd7e08a13b20uL,
        0x01528f8faa7c3202uL,
        0x3e48b368e0429dacuL,
        0x034fa73040873530uL,
        0x3e450b2501707be5uL,
        0x08cafca3c464e1d5uL,
        0x3e41ecf2c897b781uL,
        0x05c701780b38d71auL,
        0x3e3e891642306feduL,
        0x04539c08dab159e0uL,
        0x3e3a013c6709bdd4uL,
        0x0450b66dac93672buL,
        0x3e3624c9a2f2f8fbuL,
        0x074c7bde43ebd873uL,
        0x3e32da83d59392f4uL,
        0x09484520ec5eb55auL,
        0x3e300ce3767b77a7uL,
        0x07c4ad54236cf6b4uL,
        0x3e2b5312d520a3f4uL,
        0x02d19d258cf47194uL,
        0x3e274191dcab90bbuL,
        0x084e015665e4efbduL,
        0x3e23ca855a30dad5uL,
        0x01c98dc92b26aea0uL,
        0x3e20d71d1069e44fuL,
        0x01c5c29c3e79c162uL,
        0x3e1ca7c7b61a5357uL,
        0x00d28708aaed4d70uL,
        0x3e186083aaabaf72uL,
        0x07cf8bd2046619b5uL,
        0x3e14bc21b880f9deuL,
        0x044ada636f165959uL,
        0x3e11a28183b0e31fuL,
        0x0646dafa60f704a1uL,
        0x3e0dfe23a6ad4f8buL,
        0x01d37351629c53c0uL,
        0x3e0980956bea8cbfuL,
        0x04d08cff68f5874cuL,
        0x3e05ae767663002duL,
        0x054c29ce58c1fc10uL,
        0x3e026e4fd1165b76uL,
        0x00c7f5772973d16cuL,
        0x3dff54dde2ba8f55uL,
        0x0454612c5674eed9uL,
        0x3dfaa0af3e698b25uL,
        0x07515539e864d70fuL,
        0x3df6a0956d7d1b62uL,
        0x074d7ad5cdc3741euL,
        0x3df339bd6e517d43uL,
        0x0749110bc4b50f8cuL,
        0x3df0554f0943ba8buL,
        0x09454fb970dbe54euL,
        0x3debbfac9007ec07uL,
        0x03521dd98bc7de87uL,
        0x3de791862715d02euL,
        0x04cecc34851c9763uL,
        0x3de403f77382e654uL,
        0x024a2ca34863bfcbuL,
        0x3de0feff2a4fc490uL,
        0x00c63e0d12d4d288uL,
        0x3ddcdc5de1ae8c08uL,
        0x0552e615f0543e41uL,
        0x3dd8804761a993c3uL,
        0x06d00e4ae934cb56uL,
        0x3dd4cc23eb3b5ff9uL,
        0x064b471c42165f4auL,
        0x3dd1a6c6c06ea18auL,
        0x09c72b316e47cc93uL,
        0x3dcdf58ab9ae4fcauL,
        0x0553ad1e7143aa75uL,
        0x3dc96bd0bd6c9a30uL,
        0x06d0b54bd6a9e23fuL,
        0x3dc59163428fb3a5uL,
        0x05cc5f4a785a88d1uL,
        0x3dc24be8d0138112uL,
        0x0a48162809b8dff6uL,
        0x3dbf09f3c1618808uL,
        0x0554721b76389525uL,
        0x3dba53148c3fc482uL,
        0x00515a6678e0082cuL,
        0x3db652d1d62b45e1uL,
        0x00cd73f8da963966uL,
        0x3db2eda549c16ee8uL,
        0x02c8fdeb6a9e8ebcuL,
        0x3db00c2a84aed163uL,
        0x07c5342fe16e83a5uL,
        0x3dab3501c0fdbbceuL,
        0x0551fcdfea216d16uL,
        0x3da70f8998ccf075uL,
        0x034e83eb9bce31c4uL,
        0x3da38b3a7222dd33uL,
        0x0449e170e2dbff8cuL,
        0x3da08fb437656229uL,
        0x00c5f27a9aa5f660uL,
        0x3d9c1085f96d9feduL,
        0x04d29bfa42bc7b76uL,
        0x3d97c6a3cf1c9dcfuL,
        0x00cf8de2739c95a9uL,
        0x3d9423e65b2a3a8buL,
        0x06cabfaa7d4233fauL,
        0x3d910ef40de709bbuL,
        0x06c6ac1833360c58uL,
        0x3d8ce48f9d9e5927uL,
        0x04d336f5ff042b88uL,
        0x3d88773adc5703cduL,
        0x07d0484d7ff5f6bduL,
        0x3d84b6e86a5aa9d8uL,
        0x014b978904649f57uL,
        0x3d8189488e2e9743uL,
        0x02c760249f31a968uL,
        0x3d7db0100ef385d2uL,
        0x0453cd13761f1731uL,
        0x3d79206c1ae9fb29uL,
        0x02d0c569a0b1627cuL,
        0x3d754382e8081942uL,
        0x08cc67fe1e83e910uL,
        0x3d71fe13002859c9uL,
        0x06480dbcff1d72cfuL,
        0x3d6e71fde0c5e217uL,
        0x05545d945dc4844duL,
        0x3d69c159bbc99009uL,
        0x05d13da615eb6c5fuL,
        0x3d65c8fc931c6d94uL,
        0x044d2ffe78d87996uL,
        0x3d626cb8c1920344uL,
        0x04c8b4017551e03buL,
        0x3d5f295714275bc3uL,
        0x00d4e7bd56b77338uL,
        0x3d5a592ca70605e5uL,
        0x02d1b06621cfb60euL,
        0x3d5646a234bddd88uL,
        0x024dee83fc205fc8uL,
        0x3d52d4a498c21371uL,
        0x0149521701d324dauL,
        0x3d4fd5235020e008uL,
        0x03d56ad77d8efe38uL,
        0x3d4ae71657ff542euL,
        0x02d21d11201bfbcfuL,
        0x3d46bbc82f12468auL,
        0x02cea290040397f4uL,
        0x3d43354802504d9duL,
        0x08c9e7295f29cf91uL,
        0x3d403a3b07cf84b0uL,
        0x0345e631fb2a96dbuL,
        0x3d3b6a52af7c7202uL,
        0x03528313d62cbf4fuL,
        0x3d3727cc024d4629uL,
        0x04cf4b2d92a8da6auL,
        0x3d338e1c7590edafuL,
        0x02ca726cda9c5fc4uL,
        0x3d3083385f1e344buL,
        0x0846592390114765uL,
        0x3d2be229b5ed10ebuL,
        0x01d2e1e1bdc1cff3uL,
        0x3d278a15c33bf0d0uL,
        0x07cfe77379b5869auL,
        0x3d23dea49bdca04cuL,
        0x08caf3202215009fuL,
        0x3d20c5225e967ce2uL,
        0x09c6c30c15ee186buL,
        0x3d1c4df14833b32euL,
        0x015338f646703f05uL,
        0x3d17e2197e99732duL,
        0x05503b4338f71d3buL,
        0x3d14266d76b7e9efuL,
        0x034b688e02001605uL,
        0x3d10ff9aa4df55cbuL,
        0x01c72355f261c90fuL,
        0x3d0cad0ea9847217uL,
        0x04d387d609c076c8uL,
        0x3d082f5884a3c4feuL,
        0x05d07bcd8d61f54duL,
        0x3d04650f71159187uL,
        0x024bd20f0d88c869uL,
        0x3d01324c9f973606uL,
        0x07c77977767b819cuL,
        0x3cfcfef7f529f1bfuL,
        0x0253ce0fee10ae91uL,
        0x3cf8716298a66d68uL,
        0x00d0b4fbeda58aa9uL,
        0x3cf49a2f582864b8uL,
        0x034c2f0b2bc85943uL,
        0x3cf15cee56fb8f7fuL,
        0x0647c4f426570458uL,
        0x3ced43356b5d1bc3uL,
        0x01d40b3e347db73auL,
        0x3ce8a7d700826ce3uL,
        0x02d0e67b4f33d066uL,
        0x3ce4c57f38808af8uL,
        0x08cc7efb04c36011uL,
        0x3ce17f41219f6e6duL,
        0x0848055de49eb405uL,
        0x3cdd796294cc09e6uL,
        0x05543f076e4dac86uL,
        0x3cd8d265709c8b81uL,
        0x03d11003322f9f2auL,
        0x3cd4e6bf1c869176uL,
        0x02ccc169496c493buL,
        0x3cd199123dce7f7cuL,
        0x03483a55fe01c77fuL,
        0x3ccda12f38ef6064uL,
        0x0554691f56a0b9d1uL,
        0x3cc8f0ced10d0db4uL,
        0x0051315652423380uL,
        0x3cc4fdbda9c9106buL,
        0x074cf5f3d2534600uL,
        0x3cc1aa3b4e8f3ca9uL,
        0x0748638e1112031duL,
        0x3cbdba6023e12579uL,
        0x05d489478d82c425uL,
        0x3cb902e5d96b5dc6uL,
        0x06514a433d21a4e2uL,
        0x3cb50a589affacc9uL,
        0x034d1c4c912f9acbuL,
        0x3cb1b2a2ba958504uL,
        0x0ac880c8cf6ecf16uL,
        0x3cadc4cfb90a7ce4uL,
        0x05549f5031dc1940uL,
        0x3ca9088f811b7253uL,
        0x07515aa4ccc2f79buL,
        0x3ca50c7d151d73cfuL,
        0x064d343a5202c7c4uL,
        0x3ca1b23bebdcda6cuL,
        0x08c891da95a3a6f5uL,
        0x3c9dc06e50abd948uL,
        0x0454ab18582d9df2uL,
        0x3c9901c34297490fuL,
        0x0451626283914e64uL,
        0x3c950427d64b1c7duL,
        0x034d3d994938f3aduL,
        0x3c91a9076f0d2e24uL,
        0x014896a9d7ab89b1uL,
        0x3c8dad425efa38efuL,
        0x00d4ac8e5c7c8723uL,
        0x3c88ee8b30ca2585uL,
        0x07516170c969f828uL,
        0x3c84f1653e256f40uL,
        0x074d385b6cd88b32uL,
        0x3c819712f23cae3duL,
        0x02488f2f609fe4d3uL,
        0x3c7d8b686448b5afuL,
        0x0154a3b00e506616uL,
        0x3c78cf03de32b405uL,
        0x06d157d10888e2f3uL,
        0x3c74d4512f22a65cuL,
        0x074d2488978a2f74uL,
        0x3c717c7923127a39uL,
        0x00487b7664b4e00cuL,
        0x3c6d5b12a674c803uL,
        0x0354908ab62a09acuL,
        0x3c68a35c1621f2ccuL,
        0x02514591aa0080cauL,
        0x3c64ad16c988b006uL,
        0x07cd023e74fea7e1uL,
        0x3c6159616cbf8a0buL,
        0x09485b9c65443c51uL,
        0x3c5d1c88b489c5c2uL,
        0x05d4733af4601fe1uL,
        0x3c586bd4690c0844uL,
        0x07512acdf1c9738cuL,
        0x3c547bf000e37ae9uL,
        0x03ccd1b037f7490buL,
        0x3c512dff96b26d81uL,
        0x05482fd0e7486194uL,
        0x3c4cd026b64a0ca8uL,
        0x02d44bec79d5416cuL,
        0x3c4828be8d7b2e73uL,
        0x065107adbae7661duL,
        0x3c4441250d6b8cc7uL,
        0x03cc93261af2cd0duL,
        0x3c40fa934555eb5auL,
        0x01c7f854fd47e7d3uL,
        0x3c3c765c89feb632uL,
        0x01d41ad99b7fc9ebuL,
        0x3c37da7c97c8ea4buL,
        0x0350dc65148f57fcuL,
        0x3c33fd0bbb47d67buL,
        0x064c46fcad39a071uL,
        0x3c30bf675e9015a2uL,
        0x08c7b57aa64c1e42uL,
        0x3c2c0facb3969449uL,
        0x0453e04ac23c3f11uL,
        0x3c2781800b4c5861uL,
        0x0550a933c1a65e31uL,
        0x3c23b0069a07f02cuL,
        0x05cbeda3eeb5f0a2uL,
        0x3c207cd154156989uL,
        0x07c767a404101f5auL,
        0x3c1b9cab20b7b4acuL,
        0x00d39c95b8dcd835uL,
        0x3c171e48c82b190auL,
        0x00506e649c54a11duL,
        0x3c135a840f1bb9beuL,
        0x054b879e3daa485duL,
        0x3c10333055f872d0uL,
        0x06c70f426b1f5c67uL,
        0x3c0b1dfbc5f13464uL,
        0x0653501cdad9df5buL,
        0x3c06b163d96b3dd9uL,
        0x01d02c4cdfc5722cuL,
        0x3c02fcfd4e6913c9uL,
        0x05cb157f19f267eauL,
        0x3bffc5d8e0519af2uL,
        0x05d6acd55017e4e2uL,
        0x3bfa945119b38a64uL,
        0x0452fb4e266d3e9fuL,
        0x3bf63b6a2745bde0uL,
        0x074fc696b5025168uL,
        0x3bf297f53c6e927euL,
        0x06ca97e9c202c067uL,
        0x3bef18eb2ba6357euL,
        0x03d640e915b3f3eauL,
        0x3bea006a7219c6a3uL,
        0x05529ea2353deb28uL,
        0x3be5bcff1208eb99uL,
        0x034f278f182d5cceuL,
        0x3be22bf73da1838cuL,
        0x094a0f8fae515880uL,
        0x3bde60853b8b4b65uL,
        0x02d5cc15bf9dbbbbuL,
        0x3bd963124add21c0uL,
        0x01523a9b1f0c9515uL,
        0x3bd536cefa1810b3uL,
        0x074e7c6162103b4euL,
        0x3bd1b995f6e584afuL,
        0x01497d2ef035140auL,
        0x3bcd9da06644bc9duL,
        0x00d54efd8e5e8a15uL,
        0x3bc8bd1c79049ec1uL,
        0x0651cfc34a10ee47uL,
        0x3bc4a98db9bff0e8uL,
        0x014dc5f9803d5324uL,
        0x3bc1416a031bacf2uL,
        0x00c8e1907994f8d3uL,
        0x3bbcd13f7b7c3414uL,
        0x02d4ca4b88f6234cuL,
        0x3bb80f645203dff6uL,
        0x04515eac2ce52257uL,
        0x3bb415f515af2672uL,
        0x00cd054eb8db2ad5uL,
        0x3bb0c410a1d6b3c9uL,
        0x06c83d8652f7235cuL,
        0x3babfc6c8b2d1c94uL,
        0x04d43eb1f8cfdcf1uL,
        0x3ba75acacc068ebeuL,
        0x02d0e7ed05fb3af3uL,
        0x3ba37cc328e513e5uL,
        0x034c3b617ec3cfd6uL,
        0x3ba0422a6340a511uL,
        0x0ac791e9c59e2b42uL,
        0x3b9b2036a988beacuL,
        0x0553ace8dce03fbduL,
        0x3b96a0349d192d19uL,
        0x07506c218ca5f25auL,
        0x3b92deb8d0dae905uL,
        0x01cb69393c895b87uL,
        0x3b8f78b3aa5bebbduL,
        0x03d6df997f6bab1buL,
        0x3b8a3dafb67a96ceuL,
        0x06d315ac58b7d6b7uL,
        0x3b85e0885ebd9cc2uL,
        0x054fd7d13f78002duL,
        0x3b823c981e88b022uL,
        0x014a8fe21d205eb0uL,
        0x3b7e66846a73c925uL,
        0x01d62777b62fde0cuL,
        0x3b7955ea2f392221uL,
        0x025279bb2446baf4uL,
        0x3b751cacbb42476euL,
        0x034ecfc5eb955129uL,
        0x3b719722d0b598a4uL,
        0x0149b06ad8cbcafbuL,
        0x3b6d4f0c5733dbc8uL,
        0x03556a684fe99fcauL,
        0x3b6869f70ffc1fcbuL,
        0x0251d9d500e92622uL,
        0x3b645586a9e82938uL,
        0x034dc163a555fefbuL,
        0x3b60ef18dbc017feuL,
        0x0748cbe28ca7c426uL,
        0x3b5c338d2435fb4buL,
        0x0354a94f1540c9eauL,
        0x3b577ae3cb88b468uL,
        0x055136b93820fc76uL,
        0x3b538bf7be87e680uL,
        0x064cadeb8c3bba05uL,
        0x3b50453702b9a5bauL,
        0x08c7e356a2db5e15uL,
        0x3b4b154294e891dauL,
        0x0253e50df3387f95uL,
        0x3b4689b85dc875b0uL,
        0x07509125281c373auL,
        0x3b42c0dc90fab5b9uL,
        0x07cb969aedac7779uL,
        0x3b3f346b0aa94646uL,
        0x05d6f7d0d10edd84uL,
        0x3b39f5604d9610afuL,
        0x05d31e8350b95daeuL,
        0x3b3597757e14e4e8uL,
        0x03cfd3a5c3ac18bbuL,
        0x3b31f50b401397f7uL,
        0x014a7ca8fa240180uL,
        0x3b2ddd8dcb76e388uL,
        0x00d60a5532471804uL,
        0x3b28d50fcdd2a011uL,
        0x045256887c26e498uL,
        0x3b24a512f5483d31uL,
        0x04ce82efb884fa70uL,
        0x3b2129521372a708uL,
        0x09c961449f1f5f93uL,
        0x3b1c872d91eff745uL,
        0x01551be080b9d49duL,
        0x3b17b56e9895b756uL,
        0x02518df034ba2c47uL,
        0x3b13b37e1b01d1bcuL,
        0x074d31877f1753bauL,
        0x3b105e763ef1c6e1uL,
        0x04c845928aac023duL,
        0x3b0b3291e83a6dd9uL,
        0x04d42d6673958cf7uL,
        0x3b06978c8d7d61b7uL,
        0x04d0c58552d896bduL,
        0x3b02c3987ce2b431uL,
        0x014be0be95f0126euL,
        0x3aff2a6593b4ee39uL,
        0x02572aab5cc51918uL,
        0x3af9e0f0cfd57ab3uL,
        0x05d33fd04413c4e8uL,
        0x3af57c6a75ebbd35uL,
        0x064ffc132424c87auL,
        0x3af1d636b1da2b45uL,
        0x06ca91d6af35687buL,
        0x3aed9c6f3705063buL,
        0x05d6119a09e14fe5uL,
        0x3ae8936d384f421auL,
        0x01d253fb5c838ba6uL,
        0x3ae464f8c7e074fcuL,
        0x004e7068fdcaeb4euL,
        0x3ae0ec1f5aebc21fuL,
        0x01c945fff2eb1b17uL,
        0x3adc14515cb6f8efuL,
        0x0554fb5a7146299auL,
        0x3ad74b15b6eeceb1uL,
        0x02d16ab8334ccb0auL,
        0x3ad352169fa33215uL,
        0x05cce965139dad89uL,
        0x3ad0060a522d6818uL,
        0x0147fe578074e0c8uL,
        0x3aca933ad3e37ea3uL,
        0x02d3e8d828e807b4uL,
        0x3ac608e37fe916b7uL,
        0x025084c9533fea9duL,
        0x3ac24490f08ca22duL,
        0x034b68488148e38cuL,
        0x3abe4940102c0a25uL,
        0x05d6bbe630bdc58cuL,
        0x3ab91a40479b1837uL,
        0x00d2daed7fd23569uL,
        0x3ab4cdb9a0d20ef7uL,
        0x044f45c523b5ec4euL,
        0x3ab13d21ec7ce7a5uL,
        0x0449ee3b5d440d20uL,
        0x3aac90f21d2d475fuL,
        0x01d57f9f997e1f52uL,
        0x3aa7aa5b8d4b4359uL,
        0x01d1d262b74c69e4uL,
        0x3aa39a647b21bed6uL,
        0x03cd8b50e711660auL,
        0x3aa03c70a0dadb1duL,
        0x04c87c4bc616ed3duL,
        0x3a9ae43ba1c85bb0uL,
        0x04d44a615135e868uL,
        0x3a96446b3db12c57uL,
        0x06d0cfed72363bb7uL,
        0x3a926f997cdc041duL,
        0x03cbdb5f7a82d0f4uL,
        0x3a8e86218ea3e6abuL,
        0x0557136d3b897e11uL,
        0x3a89440cec9f5e39uL,
        0x05d31cf2729ac24duL,
        0x3a84e93295651e9buL,
        0x014fa860b2bf75f8uL,
        0x3a814df714b2cc27uL,
        0x01ca36fa64c5b19fuL,
        0x3a7ca3058fde005euL,
        0x0355b478418ed951uL,
        0x3a77b135dc219792uL,
        0x00d1f8035d726d41uL,
        0x3a73995999427ba7uL,
        0x044dbf75e60682c2uL,
        0x3a703604de581435uL,
        0x07c89f0afa1deecauL,
        0x3a6ad067d36fa2c8uL,
        0x0354602a49df0a52uL,
        0x3a662c6642f5d4b8uL,
        0x0450dc2db21eaf21uL,
        0x3a62556d7a42568auL,
        0x044be61355e30a98uL,
        0x3a5e5068065139beuL,
        0x00d7145a7dd1cf8cuL,
        0x3a590efd5cd13c2fuL,
        0x06531725e0702649uL,
        0x3a54b62e9374c452uL,
        0x014f93e90900fd6buL,
        0x3a511de133cc6916uL,
        0x04ca1d0c10ff74dfuL,
        0x3a4c49bf95c5f745uL,
        0x00d597928f3e0c70uL,
        0x3a475f56ab48bd88uL,
        0x04d1d9f316556fccuL,
        0x3a434f00cbd8ea41uL,
        0x064d8389849eaf01uL,
        0x3a3fe61cbe17950duL,
        0x0158650e1db268ebuL,
        0x3a3a589caf82618buL,
        0x03d4293ddcb013c1uL,
        0x3a35c1e107375834uL,
        0x0350a90025fd130cuL,
        0x3a31f7319c565580uL,
        0x04cb87eb911fc5efuL,
        0x3a2daa6c6af5c17fuL,
        0x01d6bea387f6b0a0uL,
        0x3a287d63120a742cuL,
        0x0152c9c915a28ddauL,
        0x3a2436e80df031efuL,
        0x05cf094496a5e827uL,
        0x3a20aef9bffa708duL,
        0x0249a19446f657ccuL,
        0x3a1b890579385cdcuL,
        0x02552a33b4b8094cuL,
        0x3a16b84ffdb5d885uL,
        0x015179841589cd00uL,
        0x3a12be9773700383uL,
        0x05ccda2d93f291abuL,
        0x3a0eecef0206652buL,
        0x0357d0e0e7cac5b0uL,
        0x3a09821029662cceuL,
        0x0453a804f20fd2f4uL,
        0x3a05097c74b3d08duL,
        0x075038a34010e13fuL,
        0x3a0158fcf12f6c8euL,
        0x024ac508371be502uL,
        0x39fc9b60c296975cuL,
        0x03561608ea10db83uL,
        0x39f7958bc88e6005uL,
        0x0452383e3bce3750uL,
        0x39f370dfa8e149d0uL,
        0x084e0e820ef74630uL,
        0x39f0060a594f59c6uL,
        0x0748c9f67fa9c048uL,
        0x39ea6925bee98d74uL,
        0x02d471203b047e85uL,
        0x39e5c351b4996320uL,
        0x0450dae92b938870uL,
        0x39e1ee518d278c58uL,
        0x014bcabf2ba981bfuL,
        0x39dd8b2f8b0b2924uL,
        0x00d6e8f25135d13fuL,
        0x39d855f0a34582a6uL,
        0x0252e219acb023aeuL,
        0x39d40b1881e58e30uL,
        0x004f1fe817902cebuL,
        0x39d0818d80634105uL,
        0x00c9a5d5233d8e13uL,
        0x39cb2ecbb2e8d76buL,
        0x055521d0766f8b85uL,
        0x39c6614d9da549fauL,
        0x065168c985c93c95uL,
        0x39c26c7736a63e7euL,
        0x074cae6809d7d445uL,
        0x39be546a107b57d4uL,
        0x03579f71edd3cb51uL,
        0x39b8f64020effd9buL,
        0x05d37443c37e4835uL,
        0x39b48aa64075b14fuL,
        0x07d004e8297ce819uL,
        0x39b0e6e891142764uL,
        0x01ca60ceba01346auL,
        0x39abcfa525d16889uL,
        0x0255b71dfbe662f9uL,
        0x39a6e0be1ed4e4ccuL,
        0x0351dfe04c5b884auL,
        0x39a2d14568fa3102uL,
        0x06cd6c299b6b03deuL,
        0x399ef39c9c67da70uL,
        0x0158366f8264d161uL,
        0x399973b86e9a718fuL,
        0x0153ec401194be5fuL,
        0x3994ed55e6d4d5dfuL,
        0x0150641ea45be131uL,
        0x3991345b1de4a540uL,
        0x07caf7b06dd7c2fauL,
        0x398c48e8cf8e20ecuL,
        0x04d62e7924beab28uL,
        0x39873f6cd7db5a56uL,
        0x03523e2123cac1dcuL,
        0x39831afb2e91937auL,
        0x054e00be39adba8fuL,
        0x397f6600b76754fbuL,
        0x04d8ab4ee2717624uL,
        0x3979cc2881babaefuL,
        0x055447fa5b4e25feuL,
        0x3975316d5b010b16uL,
        0x0650abf02c055867uL,
        0x3971688993cfebe3uL,
        0x00cb67d9f35f4de8uL,
        0x396c98758b0a4eb9uL,
        0x04d685ccfe1e2ab5uL,
        0x39677baf72da4867uL,
        0x045281e65593d670uL,
        0x3963484c1e2418cbuL,
        0x034e698bd1000fd2uL,
        0x395fa991c211033fuL,
        0x02d8fc0326c87b11uL,
        0x3959fe006460b911uL,
        0x055485d5ed97243euL,
        0x395555b844a27ecduL,
        0x0050db191585c5a2uL,
        0x395182875c9f3983uL,
        0x094baf50ff65044duL,
        0x394cbce2423a80acuL,
        0x0156bb8ebe73c54auL,
        0x394794741d4d28c6uL,
        0x0252a9fd1221e357uL,
        0x3943586a18110b0duL,
        0x04cea4b746dbeae3uL,
        0x393fbd1c1dcb3990uL,
        0x03d9271dfe5687e7uL,
        0x393a085cf5d6c87duL,
        0x0354a4b9ae2c857duL,
        0x393559911f8b7811uL,
        0x0650f0c2d578f06auL,
        0x393181ddd71c27fbuL,
        0x02cbccd0201398bauL,
        0x392cb5889458c00duL,
        0x0356cec95dfef21auL,
        0x392789499da6bff1uL,
        0x02d2b5ae7721763fuL,
        0x39234b0b5ddf82c6uL,
        0x03ceb1327842cc63uL,
        0x391fa04646636ebeuL,
        0x02592bda7bca05b7uL,
        0x3919eb0ea42d451duL,
        0x0554a4186866270auL,
        0x39153ce6234f7db6uL,
        0x0650ec8a57831ec5uL,
        0x3911668fdbb007d5uL,
        0x014bbfd05e1b64f3uL,
        0x390c8289c5fd0186uL,
        0x0556bf24d893426cuL,
        0x39075a62b0407aeeuL,
        0x04d2a4c4fb42b862uL,
        0x3903206cc37b0e49uL,
        0x064e8ec43d273fbauL,
        0x38ff53937c26236euL,
        0x02d90a22ee0d506euL,
        0x38f9a69ad7793258uL,
        0x01d483f4fee6553cuL,
        0x38f50039cbf56e40uL,
        0x04d0ce82f0139653uL,
        0x38f13119a81ee823uL,
        0x064b888d3fea2a71uL,
        0x38ec24cdc6a6909auL,
        0x05568ce8cbb7eaebuL,
        0x38e7089487e1182euL,
        0x0352778e05f0f826uL,
        0x38e2d94fe2dcd5a4uL,
        0x00ce3e0a1bcb7b90uL,
        0x38ded85fe218f014uL,
        0x0458c29185861611uL,
        0x38d93c37ffa2be30uL,
        0x035444e2559eb861uL,
        0x38d4a49efe08b764uL,
        0x01509735c9244f77uL,
        0x38d0e26d33274accuL,
        0x09cb28030446d467uL,
        0x38cb9dfc560135efuL,
        0x05d638fa554a9791uL,
        0x38c6955081ac80b1uL,
        0x06522ed7a20d2031uL,
        0x38c276f565251c72uL,
        0x064dc07399fb9ebduL,
        0x38be30d639687648uL,
        0x0058566bbf3afdccuL,
        0x38b8adc46e842373uL,
        0x0653e7fef514c8f7uL,
        0x38b42bb0eedd3fb2uL,
        0x02d0479dd0162987uL,
        0x38b07beb0edff1b7uL,
        0x094a9fe7272a642buL,
        0x38aaf070915be74duL,
        0x03d5c4d5495043b3uL,
        0x38a602994f04daa5uL,
        0x00d1cbea64272b5fuL,
        0x38a1fb139d7ad130uL,
        0x03cd18375dee0b86uL,
        0x389d5fdfa65dd70cuL,
        0x04d7c798c690caf6uL,
        0x3897fdb85ec65bd4uL,
        0x00d36eec953c25e3uL,
        0x38939787263ebbc9uL,
        0x04cfc2409fc1812euL,
        0x388ffeb0495cc102uL,
        0x0459f29b80329143uL,
        0x388a1f276c1aeb70uL,
        0x04d5328106ecc8f8uL,
        0x388552f40714fe53uL,
        0x05d1507fc4d2f4bauL,
        0x388167c9d827337cuL,
        0x014c484291d11ff0uL,
        0x387c690e28b6a9beuL,
        0x0457189333483e3buL,
        0x38772f13b97db104uL,
        0x0252dbc3e931f24duL,
        0x3872eaa616a9b21cuL,
        0x004ecb050b3055a0uL,
        0x386edda16b7edc87uL,
        0x0259231c8255bcdbuL,
        0x38692da9c9600769uL,
        0x03d4848161f4e509uL,
        0x38648955baf138afuL,
        0x0250beb55467080auL,
        0x3860bf90e157d9d9uL,
        0x07cb542338309321uL,
        0x385b5082a5d8de09uL,
        0x02d64c56b8fb3cecuL,
        0x3856454856772feduL,
        0x02d231052b5f7dd6uL,
        0x385227ecea87251cuL,
        0x064dadb937ed07ebuL,
        0x384d99724acabf71uL,
        0x015834eb55a1d18euL,
        0x38481ff317155699uL,
        0x03d3bdc43dd8955fuL,
        0x3843a90e48619574uL,
        0x01d018fd4cd15479uL,
        0x384005296113b586uL,
        0x004a3fee5158c03fuL,
        0x383a1acf8c750894uL,
        0x0355664a8518a142uL,
        0x38354421936100c1uL,
        0x035171860917e7c8uL,
        0x383152813e135601uL,
        0x07cc6f152728fb8fuL,
        0x382c375a4cba7b23uL,
        0x00d72bf4ab4db677uL,
        0x3826fa5568fa20f3uL,
        0x00d2e18c95c4bfb1uL,
        0x3822b5b13ef0805buL,
        0x064ec41a3d4cf576uL,
        0x381e77117811a7d1uL,
        0x04d91022d83bf8f5uL,
        0x3818ccd934db2cb0uL,
        0x02546a292659269euL,
        0x38142faa33070d29uL,
        0x0750a05da41d6048uL,
        0x38106db98d7f6125uL,
        0x00cb14375f322de2uL,
        0x380abcdbdfcc9f7buL,
        0x05d60c75486158b0uL,
        0x3805c15c23fbb403uL,
        0x01d1f35bc35fb59fuL,
        0x3801b2fdb7cab6dfuL,
        0x01cd39954e0a9d3duL,
        0x37fccb8a64624f6cuL,
        0x0157c98ab66270f5uL,
        0x37f76bb52e82b59auL,
        0x00535be6eb898758uL,
        0x37f30c117f001ac2uL,
        0x07cf819edd38db9cuL,
        0x37eefa0e49e3feccuL,
        0x01d9a2821242ebd0uL,
        0x37e92fa046d58d4duL,
        0x0554dadd528d6ea9uL,
        0x37e479ae4e865feeuL,
        0x0250f6d9e092345cuL,
        0x37e0a4c603089f15uL,
        0x064b987187720ae4uL,
        0x37db0e03e96a5484uL,
        0x03d6711ad9310ce1uL,
        0x37d5fc89a9e03198uL,
        0x06523f97aea9f29fuL,
        0x37d1dd90a3522c75uL,
        0x044dac6b554960ffuL,
        0x37cd07c0b8b30398uL,
        0x01581f77dc55f2bduL,
        0x37c795540ea5dda7uL,
        0x03539bb36d1a51dauL,
        0x37c327f191dd6246uL,
        0x054fdf7c425dfb89uL,
        0x37bf1db008e061d6uL,
        0x0159e6c7f42ee3a0uL,
        0x37b944b7c8850269uL,
        0x00d50bd38f4b0e14uL,
        0x37b4846e1e475567uL,
        0x03511954fcd9d596uL,
        0x37b0a8512d6deeafuL,
        0x064bc7d8a23288e1uL,
        0x37ab0b57b848dfd4uL,
        0x03d69099571fea27uL,
        0x37a5f385601a1095uL,
        0x02525378a9823720uL,
        0x37a1d0aee3f21eaeuL,
        0x034dc36feecfa2bauL,
        0x379ce9ce0f1b56b8uL,
        0x01582a9fb7ad076buL,
        0x379775af322a6fb6uL,
        0x02539ea243c7bf71uL,
        0x3793084e2fb958e5uL,
        0x03cfda4af81b306auL,
        0x378ee0aaff5c7275uL,
        0x01d9da7a2c5ab52cuL,
        0x37890b5b261712acuL,
        0x0254fb44aa933f5cuL,
        0x37844f853ca3d2a0uL,
        0x0451068e39733d5fuL,
        0x37807839b24e2328uL,
        0x064ba0b385a9673fuL,
        0x377ab4ef712ea53buL,
        0x045669cb88b98bb4uL,
        0x3775a6a27edc2aaeuL,
        0x05522e458ff074e2uL,
        0x37718ccfb2383c0duL,
        0x03cd7dccacf16bdfuL,
        0x376c72c7d427b5c6uL,
        0x05d7ea9a57d9c3fduL,
        0x37670debd3477d7buL,
        0x05d364981b4fcaccuL,
        0x3762ae4c8505c4dcuL,
        0x03cf723b60a4c45auL,
        0x375e45347f37826duL,
        0x00597e0b5db827a8uL,
        0x3758859d9d834870uL,
        0x0554a9cae44d02aauL,
        0x3753dcdd6f53a761uL,
        0x0250bf347561e06fuL,
        0x3750163c7a1b8ce3uL,
        0x02cb246ea577dcd5uL,
        0x374a0de9e4d03269uL,
        0x0555fe1a8f2ffd47uL,
        0x374518a7407eb90euL,
        0x00d1d15869af1a46uL,
        0x3741146574533e59uL,
        0x024cde08f63664fduL,
        0x373ba6f77161f191uL,
        0x02d761ba88bf6eeduL,
        0x373661c59f17fae0uL,
        0x01d2efafc89163c3uL,
        0x37321d2894bdd4c6uL,
        0x06ceab12c8aa7e50uL,
        0x372d50e0eba3e44duL,
        0x0058d4d432dee077uL,
        0x3727b84a5753cf1fuL,
        0x02541a589d11cb19uL,
        0x37233091416396dbuL,
        0x035045db9ec2ba81uL,
        0x371f0bb3ff173142uL,
        0x045a57861242277fuL,
        0x37191c3cacc75aaauL,
        0x02d551681b8d3610uL,
        0x37144ea256a84bb0uL,
        0x005140098b38820cuL,
        0x37106bb841410433uL,
        0x074be9e2feb561e0uL,
        0x370a8d98b0d5770fuL,
        0x035694e9fdcb7be5uL,
        0x37057755a2313bdfuL,
        0x02d24419d9ce37ffuL,
        0x37015a03d39bca42uL,
        0x06cd8bf1578b3aacuL,
        0x36fc0c4e9f387792uL,
        0x02d7e4dfe2cee6a2uL,
        0x36f6aa9b63079410uL,
        0x04d3520b0bf08a51uL,
        0x36f250ad98a67e4fuL,
        0x00cf3daa3dd37f3auL,
        0x36ed9842421f4af0uL,
        0x04d94140b3abb78euL,
        0x36e7e859d0226582uL,
        0x025469d2facc66f7uL,
        0x36e34f9e5d4c96d2uL,
        0x07507f7c6b04c092uL,
        0x36df314a5f5af6d7uL,
        0x025aa9f80ec12e52uL,
        0x36d9306ca687d568uL,
        0x03558b5e63278412uL,
        0x36d456b681315daeuL,
        0x065167dcc97a0fd3uL,
        0x36d06b98180e66f0uL,
        0x01cc1ee5bab4ede7uL,
        0x36ca82a4c036e3f2uL,
        0x05d6b69077bfc3c7uL,
        0x36c565cda5d05a6auL,
        0x02d257dcc5bc2717uL,
        0x36c144d77262f022uL,
        0x014d9fdd2296338fuL,
        0x36bbdec7b50a66bfuL,
        0x0557eb427b4ddd71uL,
        0x36b67cb265d8483auL,
        0x00d34f5aee912170uL,
        0x36b224399b226995uL,
        0x06cf2ca4dc8ff69fuL,
        0x36ad448f86c23d11uL,
        0x02d92943634830d2uL,
        0x36a79b2a15ae0fa9uL,
        0x04544e2d8e947442uL,
        0x36a3098d833c2da0uL,
        0x02d0627b1e47c261uL,
        0x369eb3aa595948f2uL,
        0x045a705784809825uL,
        0x3698c0f08dff4e67uL,
        0x03d554226cd542efuL,
        0x3693f49a8880f6acuL,
        0x0451343e7a202e90uL,
        0x369015dd1c62a081uL,
        0x084bc0384ab3550duL,
        0x3689edb80143a704uL,
        0x045660fe966c4e28uL,
        0x3684e52056f2dec4uL,
        0x03520b6b60dae611uL,
        0x3680d62a769875e0uL,
        0x00cd1893fc15ba16uL,
        0x367b2128dd015485uL,
        0x02d7747e31ddd25cuL,
        0x3675dad6d3a16694uL,
        0x0352e7c997078049uL,
        0x36719a81ef58dfc6uL,
        0x00ce790d89e8e564uL,
        0x366c5ae1b79c4ee7uL,
        0x03588e545d12ba57uL,
        0x3666d56e11abc8a6uL,
        0x0553c919aea97870uL,
        0x366262a204b39df0uL,
        0x05cfe13c6f07b6aeuL,
        0x365d9a774b67b183uL,
        0x02d9ae2b16a9550auL,
        0x3657d48e51f6d6eduL,
        0x0254af14f857334euL,
        0x36532e43016e50e3uL,
        0x0650a8564eab8ff5uL,
        0x364edf747f9f14f0uL,
        0x03dad3a333504020uL,
        0x3648d7d80e14b90fuL,
        0x0655996d7e13f467uL,
        0x3643fd1708b687cbuL,
        0x0151636f3d76858auL,
        0x364014ad3fec9ec4uL,
        0x014bfe545fce7a55uL,
        0x3639dee40ecc2982uL,
        0x00d687ce08618977uL,
        0x3634ceca2b27453fuL,
        0x055221a377d62eb4uL,
        0x3630bbd071377b86uL,
        0x07cd2dcd30499eb7uL,
        0x362ae9438e9a5c0auL,
        0x055779da2df7a30cuL,
        0x3625a30285652ad0uL,
        0x02d2e2a7c1fe1c5fuL,
        0x362164daef1c2b14uL,
        0x05ce61933d473856uL,
        0x361bf6806876a634uL,
        0x04d86f2e6e7e582auL,
        0x36167960688424efuL,
        0x0253a62b4892ce6euL,
        0x36120f7f47f404a6uL,
        0x054f99234ed0089euL,
        0x360d061d530972c4uL,
        0x03d9676058974913uL,
        0x3607517e8c57f622uL,
        0x03546bd7c1e28ef0uL,
        0x3602bb6ba79809eduL,
        0x01d069f8cb02119fuL,
        0x35fe179628712470uL,
        0x00da61febb6d574duL,
        0x35f82af24bbe81dduL,
        0x02553351984f5d61uL,
        0x35f3684a09debb18uL,
        0x015108b4faaa8971uL,
        0x35ef2a603a977e7buL,
        0x035b5e91e3ee196duL,
        0x35e9054beadf5a51uL,
        0x01d5fc381e001854uL,
        0x35e415c074fc9065uL,
        0x0151a8782bc000beuL,
        0x35e01ef55a0092e2uL,
        0x06cc5c9be5ba37d4uL,
        0x35d9e016e74801cauL,
        0x03d6c625c9dd5c05uL,
        0x35d4c3713bae315cuL,
        0x055248f08aa2a9f5uL,
        0x35d0a8cf82738469uL,
        0x004d5b98efc2e8d5uL,
        0x35cabada51b7b47euL,
        0x015790b07dcc17dduL,
        0x35c570fb47030aa8uL,
        0x00d2e9c8b4dec3deuL,
        0x35c13270ae279a56uL,
        0x06ce5affac730013uL,
        0x35bb951931589ad6uL,
        0x01d85b69d604d483uL,
        0x35b61dfa678e3295uL,
        0x05538aa7fa8655e3uL,
        0x35b1bb88966006c4uL,
        0x00cf5a41ad29abd6uL,
        0x35ac6e52f00f28e5uL,
        0x055925df815332e1uL,
        0x35a6ca07adb2cabduL,
        0x06542b32a68b6433uL,
        0x35a243c4de072740uL,
        0x05502c65f05a223cuL,
        0x359d4603cf73627euL,
        0x0159ef9ba1f58105uL,
        0x359774b9c8b0651fuL,
        0x0554cb0a4ddc2264uL,
        0x3592cad15ed5f00duL,
        0x03d0ab038a2ddd17uL,
        0x358e1ba565f2f2d9uL,
        0x045ab82536c08c11uL,
        0x35881da56c03901cuL,
        0x02d569ce24f30caduL,
        0x358350587b61e2e7uL,
        0x015128ac3f80b9acuL,
        0x357eeeaf2386ba72uL,
        0x035b7f008c184953uL,
        0x3578c45dba9ebafeuL,
        0x0556071b5b7d5f0buL,
        0x3573d40375ab2fc9uL,
        0x0151a5112ad78884uL,
        0x356fbe96dd52dd29uL,
        0x035c43afb43abf3auL,
        0x35696874b77050b2uL,
        0x0356a28d7dab4750uL,
        0x3564557ac9b8a4ffuL,
        0x02521fe234726979uL,
        0x35604568afbad70auL,
        0x08cd05b30647f5b6uL,
        0x355a097bba9c5bbauL,
        0x01d73bbedaae952fuL,
        0x3554d4668bc3c638uL,
        0x035298ce64edbc52uL,
        0x3550a969821c25d3uL,
        0x054dc489a35fd890uL,
        0x354aa703eac27071uL,
        0x00d7d248efdebaf1uL,
        0x3545506ec96ce1d8uL,
        0x00d30f843b6c62b7uL,
        0x35410b0827e1c59euL,
        0x05ce7fb2011e1175uL,
        0x353b409eb99c2287uL,
        0x015865c4d7ebd336uL,
        0x3535c93bed6568e8uL,
        0x04d383b206d0bb99uL,
        0x353169ff47b694c6uL,
        0x024f36aa78ac249duL,
        0x352bd5de633517f7uL,
        0x0258f5cbbd7e3bd9uL,
        0x35263e7724f64773uL,
        0x0653f5064180659duL,
        0x3521c60a3dd2224duL,
        0x084fe8f1d993bb19uL,
        0x351c66566ef40332uL,
        0x04d981f750955121uL,
        0x3516afcac6c09d19uL,
        0x0654632fef2669ecuL,
        0x35121ee56dbc8c69uL,
        0x08504b03ffb7174auL,
        0x350cf19c31a391abuL,
        0x055a09e23dee12dbuL,
        0x35071ce2ba111a68uL,
        0x01d4cddefbe00daeuL,
        0x3502744e94597df0uL,
        0x02509eb734c1a314uL,
        0x34fd77474fa3c96fuL,
        0x025a8d28a7b21f9euL,
        0x34f7856cde19858auL,
        0x035534c49c3a48a0uL,
        0x34f2c60519b06072uL,
        0x06d0ef5469afe541uL,
        0x34edf6f23e67822duL,
        0x02db0b689ea896f0uL,
        0x34e7e91970609419uL,
        0x05559793ad60d8abuL,
        0x34e313ca61e59762uL,
        0x04513c9ee6b2a529uL,
        0x34de703ac45eb1a4uL,
        0x04db84429b1d33d8uL,
        0x34d8479b71b66ff2uL,
        0x01d5f60114dc317auL,
        0x34d35d621cd7892fuL,
        0x00d1865baa279b03uL,
        0x34cee2c2766d39aeuL,
        0x015bf759f4ae6481uL,
        0x34c8a0a908fbee34uL,
        0x01d64fc41f392bcduL,
        0x34c3a29293d26666uL,
        0x03d1cc51b3533d1buL,
        0x34bf4e2f320ed2f5uL,
        0x00dc645558315ad7uL,
        0x34b8f3fbe30bc1d7uL,
        0x03d6a496dcf46820uL,
        0x34b3e324f4cf0981uL,
        0x02520e4a4b8e031euL,
        0x34afb22b934b992fuL,
        0x045ccadf3adb1af0uL,
        0x34a941518f17ca25uL,
        0x0356f4367d03dbd8uL,
        0x34a41ee59ab3f625uL,
        0x00d24c114d622260uL,
        0x34a00733b2d2d2a6uL,
        0x05cd2aa649df6e65uL,
        0x3499886bd6d1085buL,
        0x01d73e63a45afd4duL,
        0x349455a452136a5fuL,
        0x065285756918be22uL,
        0x3490314c07978175uL,
        0x014d835dd5ba6335uL,
        0x3489c91111b6c15euL,
        0x05d782e2c1c97a81uL,
        0x3484873499e69a71uL,
        0x0352ba486638ab1euL,
        0x3480573c7a800f17uL,
        0x07cdd4be385e9720uL,
        0x347a030c72f0cf33uL,
        0x0257c17c5d99552cuL,
        0x3474b36ddfcc8743uL,
        0x0052ea5f617d321fuL,
        0x347078e5ec28bafduL,
        0x02ce1e853589fe15uL,
        0x346a362e51221b9euL,
        0x0457f9fd64579e1auL,
        0x3464da2bb75a5c64uL,
        0x03d3159306d0abd0uL,
        0x3460962c95c3eb50uL,
        0x014e6076548c0765uL,
        0x345a624c67aa97dfuL,
        0x01582c376c3acddfuL,
        0x3454fb4e0c13d490uL,
        0x00d33bbfc6dd55a6uL,
        0x3450aef82f484486uL,
        0x00ce9a5b32d2ef52uL,
        0x344a874210dbadcfuL,
        0x00d85800f4a2d262uL,
        0x344516b94dabb86duL,
        0x00535cc607ce4fd8uL,
        0x3440c33410fd4c56uL,
        0x00cecc03cea2935duL,
        0x343aa4f078af0320uL,
        0x03d87d359f39448euL,
        0x34352c5696370c9cuL,
        0x0453788a50e33e44uL,
        0x3430d2cf5025ba2duL,
        0x014ef546c9652b0auL,
        0x342abb3ec79d594cuL,
        0x03d89bb66243bfd5uL,
        0x34253c13ca08d951uL,
        0x01d38ef570827673uL,
        0x3420ddbcd68fc942uL,
        0x054f1601a115b514uL,
        0x341aca1a45423b35uL,
        0x0258b369b3c6ec4fuL,
        0x341545e3b0f8838auL,
        0x02539ff49c7fe5e8uL,
        0x3410e3f374dd9d68uL,
        0x01cf2e18e05495b4uL,
        0x340ad1767288e012uL,
        0x03d8c43bad265564uL,
        0x340549be08e15927uL,
        0x0353ab798c59d4c2uL,
        0x3400e56def61fbc3uL,
        0x054f3d7844c8a592uL,
        0x33fad14d1b2f0b5euL,
        0x0458ce1e26fb8214uL,
        0x33f5479f9137160auL,
        0x0653b17a8d383f04uL,
        0x33f0e22b05782283uL,
        0x04cf4412db819edfuL,
        0x33eac99e5e7b9268uL,
        0x03d8d108ccedcd75uL,
        0x33e53f8a0f98a8b7uL,
        0x03d3b1f28f8795cauL,
        0x33e0da2d734853ffuL,
        0x03cf41e3132440dauL,
        0x33daba70af1767afuL,
        0x04d8ccf9296410aeuL,
        0x33d531844d58365duL,
        0x0553ace12e143377uL,
        0x33d0cd7bedf59779uL,
        0x00cf36eac3bc78c2uL,
        0x33caa3d0ca096eeduL,
        0x01d8c1f2a8f92477uL,
        0x33c51d9a0dfd2e92uL,
        0x03d3a24aae988ae7uL,
        0x33c0bc211a3c2858uL,
        0x064f23332c263066uL,
        0x33ba85d1a4e6bedbuL,
        0x04d8affe95ac6f2auL,
        0x33b503dbfed30324uL,
        0x02d39237fbbcfa18uL,
        0x33b0a62b7d92f095uL,
        0x00cf06cce511da3euL,
        0x33aa608c535a2ba0uL,
        0x04d8972c09d7f45cuL,
        0x33a4e45f9fa4adfeuL,
        0x04537cb698950bdauL,
        0x33a08bad69ed20a4uL,
        0x01cee1cfc9be3df9uL,
        0x339a341fe436d2d6uL,
        0x03d8778fdb058321uL,
        0x3394bf3f24d273a5uL,
        0x02d361d88db2b95buL,
        0x33906cbce44363ebuL,
        0x084eb45ad695330auL,
        0x338a00b13659be7buL,
        0x055851447ccc879buL,
        0x3384949952fc2371uL,
        0x025341b44ff4c3c6uL,
        0x3380497386163a38uL,
        0x084e7e93fdecae00uL,
        0x3379c66ac5ae65b3uL,
        0x02d82469dbf1833euL,
        0x337464915486577auL,
        0x04d31c64a141680euL,
        0x337021ee5a248c7fuL,
        0x034e40a7f340982auL,
        0x3369857c70b8b2bbuL,
        0x0357f125320f1e94uL,
        0x33642f4e894cc719uL,
        0x0552f2086b6a5cf4uL,
        0x335fec9b69351b6fuL,
        0x035dfac9ed4c27ceuL,
        0x33593e1b371520a0uL,
        0x04d7b7a0d21f0262uL,
        0x3353f4fc50de840auL,
        0x0352c2c295822108uL,
        0x334f8d6a0e0a9507uL,
        0x035dad335f7aacdbuL,
        0x3348f080f16c57bfuL,
        0x0357780bee4609a1uL,
        0x3343b5c9cfaada16uL,
        0x00528eb9d3f5000auL,
        0x333f269560bdbf91uL,
        0x02dd5823ab37d92euL,
        0x33389cec0363502cuL,
        0x0557329a5753ca24uL,
        0x333371e9af8e6cceuL,
        0x06d2561873c1cc7auL,
        0x332eb86f931c309duL,
        0x015cfbdfc9b64d6euL,
        0x3328439f081b5259uL,
        0x0356e7843670c8d2uL,
        0x33232991dc38028duL,
        0x06d2190c2136fc76uL,
        0x331e434fdd743954uL,
        0x025c98b1eed08258uL,
        0x3317e4e079de1a2euL,
        0x02569705c180d6c1uL,
        0x3312dcfb3be31ebcuL,
        0x05d1d7c5aaa09490uL,
        0x330dc7920bafc5dbuL,
        0x035c2ee925b3e3f6uL,
        0x330780fa5599d558uL,
        0x00d6415eeac7f744uL,
        0x33028c6164ec1235uL,
        0x03519278bf59ff34uL,
        0x32fd459605b63622uL,
        0x045bbed8e8100752uL,
        0x32f71839bad6a45buL,
        0x0255e6d30c67b96buL,
        0x32f2380250c57525uL,
        0x04d1495babbc8d8euL,
        0x32ecbdbf53eed588uL,
        0x00db48d8b08c37b5uL,
        0x32e6aaee88d3a5e6uL,
        0x00d587a8905112ebuL,
        0x32e1e01e0cda0c0duL,
        0x0550fca71267dd26uL,
        0x32dc3074a0c1c67cuL,
        0x03dacd43894c1f06uL,
        0x32d6396af97c5f7euL,
        0x06552428954b7c2fuL,
        0x32d184f669e7e645uL,
        0x02d0ac95a364b406uL,
        0x32cb9e1f37f768c9uL,
        0x015a4c779750fb77uL,
        0x32c5c4033ae88d93uL,
        0x0454bc9e91b546a8uL,
        0x32c126ceaa621095uL,
        0x00505963d1a5105buL,
        0x32bb072a84d6770auL,
        0x04d9c6d5a387a6d7uL,
        0x32b54b0d08180ac5uL,
        0x03545157f4a2e598uL,
        0x32b0c5eb30658610uL,
        0x07d0034f87652744uL,
        0x32aa6c038fdf5aeduL,
        0x00d93cc0a254a9f5uL,
        0x32a4cedf419a9b37uL,
        0x0553e2a3c60327aauL,
        0x32a062912bcc23f9uL,
        0x03cf552fb3e1c70buL,
        0x3299cd187cff951cuL,
        0x0058ae9d3a6eb66fuL,
        0x32944fd186d008c1uL,
        0x045370d2466d3327uL,
        0x328ffa0c91caab55uL,
        0x01de9ef97aa04b46uL,
        0x32892ad80b12a09buL,
        0x01581cd14bd535bbuL,
        0x3283ce3bd0683046uL,
        0x0252fc348f3a8121uL,
        0x327f2b20c0b002aauL,
        0x02dde47d70b3398cuL,
        0x327885b1157e885buL,
        0x05d787c377ac34cduL,
        0x32734a760cc47acauL,
        0x03d2851c338b22e4uL,
        0x326e58ea51580baduL,
        0x01dd263d33512bb6uL,
        0x3267de1218b19541uL,
        0x04d6efdaa9c0e45euL,
        0x3262c4d7bed4d521uL,
        0x06520bdae2cd61c6uL,
        0x325d83f3d3e6d14fuL,
        0x02dc64ba5bdb46deuL,
        0x32573468ba3c29b8uL,
        0x02d6557da47246f7uL,
        0x32523db7a001a935uL,
        0x01d190c20d5b5808uL,
        0x324cacc668087b83uL,
        0x015ba075f0192b60uL,
        0x324689215536317euL,
        0x04d5b9128fb09361uL,
        0x3241b56b45aac06fuL,
        0x02d114228bb99133uL,
        0x323bd3e92f58e3aduL,
        0x035ad9efd6e7e350uL,
        0x3235dca68b92a62fuL,
        0x03551afe8bbb6b6cuL,
        0x32312c46cab86e90uL,
        0x0650964c48f92b05uL,
        0x322af9e0c680145auL,
        0x02da11a652260d00uL,
        0x32252f60dcf5b38fuL,
        0x06547ba5483b6e8fuL,
        0x3220a29c7db10f70uL,
        0x03d0178df0b67157uL,
        0x321a1f2ec5b27de2uL,
        0x00d948157e97fbd7uL,
        0x321481b643932becuL,
        0x00d3db68a0470a4fuL,
        0x321018bc93b8e2e4uL,
        0x044f306942454ae6uL,
        0x3209445149305036uL,
        0x04587db6da6dd3cauL,
        0x3203d409d78b6819uL,
        0x02533aa83bd4deabuL,
        0x31ff1de9c1ab95aauL,
        0x025e311742f9561buL,
        0x31f869c2824b4b6buL,
        0x0157b300d303ed2cuL,
        0x31f326bb792c8c5buL,
        0x03d299c1370fc2d1uL,
        0x31ee0b212b870715uL,
        0x00dd31b83aa1a53buL,
        0x31e78ff85165ac90uL,
        0x0556e8665a634affuL,
        0x31e27a27826da7a4uL,
        0x06d1f90dcff1976euL,
        0x31dcf9b0072f8175uL,
        0x03dc32d9c998168auL,
        0x31d6b763e947db08uL,
        0x02561e5684f4d137uL,
        0x31d1cea67fe8699buL,
        0x065158e51a7ac97euL,
        0x31cbea20cad09b1euL,
        0x02db350464c51c99uL,
        0x31c5e0717c155a1cuL,
        0x00d5553c2fc66728uL,
        0x31c1248cf18568a1uL,
        0x06d0b99abbccdbb1uL,
        0x31badcf760300962uL,
        0x03da38baebfb68e4uL,
        0x31b50b87f214792cuL,
        0x05548d7dafad7ffeuL,
        0x31b07c2b12fe4db9uL,
        0x05d01b7eac5ea688uL,
        0x31a9d2b0d0c4a0b1uL,
        0x02d93e7a4bb07430uL,
        0x31a43908aa677d24uL,
        0x0653c77c897ed254uL,
        0x319fab995891c153uL,
        0x005efdba02e2ceffuL,
        0x3198cbc2fe600107uL,
        0x04d846b92a47c343uL,
        0x3193694f45c1b92fuL,
        0x03530395337f89bbuL,
        0x318e6371d3dc0232uL,
        0x02ddc7fb7bbca8aduL,
        0x3187c89c6867890euL,
        0x005751e7a10e8264uL,
        0x31829cb17b0f706auL,
        0x0452421ee0211f87uL,
        0x317d20647a807a0cuL,
        0x01dc9649548abac7uL,
        0x3176c9a3fd812076uL,
        0x04d6606f00ed6d5duL,
        0x3171d37ef5f490ccuL,
        0x0551836b52067807uL,
        0x316be2ec88ae1478uL,
        0x03db6922692e74d4uL,
        0x3165cf38f9818abeuL,
        0x035572b1a2c0293auL,
        0x31610e013ef486f7uL,
        0x02d0c7c6b93f06a1uL,
        0x315aab7b734b99f6uL,
        0x005a40fcadcdd133uL,
        0x3154d9b2cf546b09uL,
        0x0354890ac32b69b5uL,
        0x31504c7bad04b57buL,
        0x06500f779993bbc1uL,
        0x31497a78d5f1c6dbuL,
        0x01591e450ac30542uL,
        0x3143e9611e8217ffuL,
        0x0653a3ce69b6a143uL,
        0x313f1e56c0773bb7uL,
        0x01deb57d7362f984uL,
        0x313850426f2df55cuL,
        0x0358015f467ddd40uL,
        0x3132fe8bb3e4f4d8uL,
        0x03d2c3495adab7d8uL,
        0x312dac8e8a813f1euL,
        0x02dd53ae35dbfa26uL,
        0x31272d2c2a7422abuL,
        0x0256eaa5fce4af3auL,
        0x31221972950f570duL,
        0x00d1e7c114a57a33uL,
        0x311c44004226dc17uL,
        0x025bf9ebf2ac34cfuL,
        0x3116118037139873uL,
        0x03d5da6aa3adb7a3uL,
        0x31113a4e15d42467uL,
        0x02d11173d5813f4duL,
        0x310ae501496e23f2uL,
        0x015aa895a750e0f6uL,
        0x3104fd7f2b705e63uL,
        0x0554d0f59b16ac32uL,
        0x3100614ef7575b08uL,
        0x06d04098aca1b898uL,
        0x30f98fdb1084fd1buL,
        0x05595ffef5a788b3uL,
        0x30f3f16033b4da16uL,
        0x0653ce864a4f75bbuL,
        0x30ef1d3d20014dd2uL,
        0x03deeabf27142ccbuL,
        0x30e844cb59a101a9uL,
        0x00d82070510e6e91uL,
        0x30e2ed514b22b68buL,
        0x02d2d35346de60f3uL,
        0x30dd84bdf7421498uL,
        0x045d5fe3202b4d44uL,
        0x30d7040489842ad6uL,
        0x0556ea2738b3dbebuL,
        0x30d1f1777f205011uL,
        0x04d1df8a8637ba9cuL,
        0x30cbf956a62adf73uL,
        0x00dbe0e1bcc5bf2buL,
        0x30c5cdae0381ff93uL,
        0x03d5bd567e120a1cuL,
        0x30c0fdef3b187063uL,
        0x03d0f35198b8b7f7uL,
        0x30ba7b2fd5556b69uL,
        0x045a6df243f2c6f4uL,
        0x30b4a1e48fd99b8euL,
        0x01d49a26968a8fd1uL,
        0x30b012cc9c3d1429uL,
        0x05500ec5ed2dbe3euL,
        0x30a90a652d08b6ecuL,
        0x0059073f3afbdfebuL,
        0x30a380bacb3471d9uL,
        0x035380b5f70c487duL,
        0x309e603798765b09uL,
        0x03de63fa380d130buL,
        0x3097a705e88ab4c7uL,
        0x0357ace6e086aab7uL,
        0x30926a399e180e7buL,
        0x04d2711978a97cf7uL,
        0x308cabc2c3d98d7buL,
        0x035cba0a72ae9c08uL,
        0x308651157275ac6fuL,
        0x02d65efbb20adf2duL,
        0x30815e60bb1a2babuL,
        0x07516b5cc5019368uL,
        0x307b08358e30e1b0uL,
        0x035b1fca598944c3uL,
        0x3075088c08941b89uL,
        0x00d51d84fa353951uL,
        0x30705d2722aa0abeuL,
        0x01506f82c9619b90uL,
        0x3069757d44a0d5d0uL,
        0x0459953a1cf16aaduL,
        0x3063cd5765cc7b51uL,
        0x0353e87f66d27bb0uL,
        0x305eccf7568ff3afuL,
        0x015efb0c5f0312cduL,
        0x3057f37a88128932uL,
        0x05581a4d1085cfd1uL,
        0x30529f5b70afae6euL,
        0x0252bfdda4e2b20cuL,
        0x304cf48b1a182cb9uL,
        0x015d2ab3b59164a6uL,
        0x304682022c0d8296uL,
        0x01d6aeea740e7e26uL,
        0x30417e72ed48d1c1uL,
        0x0751a389017ca93cuL,
        0x303b30c9decefa85uL,
        0x045b6dd2d215fccfuL,
        0x303520de188c8ff4uL,
        0x005552ee415230cduL,
        0x30306a7030db71fauL,
        0x04d093620e33d9f9uL,
        0x30298166f02e00a9uL,
        0x0359c4336b720df7uL,
        0x3023cfce2d301755uL,
        0x02540629fd47fda6uL,
        0x301ec63bac9af50auL,
        0x015f1e828f7f1e6euL,
        0x3017e609b497d4bfuL,
        0x00d82d92bd0fbc5buL,
        0x30128e89244647b5uL,
        0x0052c8658b1c7fabuL,
        0x300cd07ee41894f6uL,
        0x015d2def7b6139fbuL,
        0x30065e4eca3c47cduL,
        0x03d6a9a29142865auL,
        0x30015cbd7439af48uL,
        0x0051995fff959855uL,
        0x2ffaf324889fe32euL,
        0x015b549f742691f7uL,
        0x2ff4e9c920d5db04uL,
        0x0555380a4af4c2e9uL,
        0x2ff03a122e1077b6uL,
        0x065078d07375b0b0uL,
        0x2fe92d9bd168c630uL,
        0x00d9921acfd99f39uL,
        0x2fe388030ea8589cuL,
        0x0353d867ecfb60a5uL,
        0x2fde4c4faf832007uL,
        0x035ecccda72dba49uL,
        0x2fd77f4a046c515duL,
        0x04d7e5deef2de87buL,
        0x2fd2387f5f4b712duL,
        0x05d28a511d87ce7duL,
        0x2fcc413282821078uL,
        0x02dcc3995b1e2c40uL,
        0x2fc5e78bc56d0fbauL,
        0x03d64f5f80200f46uL,
        0x2fc0faba5af01355uL,
        0x00514d5424501d7euL,
        0x2fba51f8a6830159uL,
        0x015ad54bef9112d0uL,
        0x2fb465b65a83bdbbuL,
        0x0054ce07b8d50856uL,
        0x2faf9c5589e7201fuL,
        0x00e020f8e226943euL,
        0x2fa87dc5ad8af9ebuL,
        0x04590123a8271991uL,
        0x2fa2f918e4d3f95cuL,
        0x01d3613b89391a8fuL,
        0x2f9d6485a1704139uL,
        0x02de098381b76cd3uL,
        0x2f96c3b66970be3cuL,
        0x0457465697a54c64uL,
        0x2f91a0fd8c3a4e6euL,
        0x05d20858c20a1795uL,
        0x2f8b4ce217bd5e55uL,
        0x02dbf05934cfa1ccuL,
        0x2f8522e259c7017auL,
        0x0155a41409f84e49uL,
        0x2f805caa9cf257c4uL,
        0x0350c2b83023243duL,
        0x2f7954427a430b10uL,
        0x0459f5672cf62a4fuL,
        0x2f739a5d07601e70uL,
        0x04d41985de8f7a14uL,
        0x2f6e56c72cc01fcbuL,
        0x045f1f5d5615d783uL,
        0x2f67797a6e64ddc9uL,
        0x0058179bfb69c631uL,
        0x2f6229374c83805fuL,
        0x0452a5d1d1f1ae5cuL,
        0x2f5c18d454a503aeuL,
        0x01dcdd1c2bddbb9euL,
        0x2f55bb5b3e414ad3uL,
        0x025655e203c78ad0uL,
        0x2f50ce808921de57uL,
        0x01d1481ab5a1469auL,
        0x2f49fdfe587f0569uL,
        0x02dabd4ca4bd8884uL,
        0x2f4418b54bd6a895uL,
        0x00d4af20f59f283duL,
        0x2f3f128f851039d8uL,
        0x045fff032b2dbde7uL,
        0x2f3804c6e03f60cbuL,
        0x0158be8c488684b4uL,
        0x2f3290596a08a94euL,
        0x0553223f2e5be0f0uL,
        0x2f2cb1395c8187f5uL,
        0x03dd964d959533d1uL,
        0x2f262bb1316ec5fbuL,
        0x03d6df780d5ecc43uL,
        0x2f21211a1b47d3aeuL,
        0x0151ae2302fd4bcduL,
        0x2f1a772150026810uL,
        0x035b5455f4e2ce45uL,
        0x2f147143aa78b5feuL,
        0x00d51eade2a24279uL,
        0x2f0f93996ba5e93duL,
        0x016051b3f15282e5uL,
        0x2f08626f2553e203uL,
        0x04593760037df87auL,
        0x2f02d4091cd12adbuL,
        0x03d37ace1ccc1a8duL,
        0x2efd1294db79df79uL,
        0x00de17b7713cf17fuL,
        0x2ef6715149108678uL,
        0x02573db39c4b278buL,
        0x2ef1529206516167uL,
        0x02d1f27cc2724f90uL,
        0x2eeabce28a1f17f2uL,
        0x00dbb70eb3792a1cuL,
        0x2ee4a1fe3e55f964uL,
        0x02d5659e4463ddd1uL,
        0x2edfd6eb54be7325uL,
        0x03e08462ba9624dbuL,
        0x2ed89049c51b8388uL,
        0x02d97f4ffe1284a1uL,
        0x2ed2f2b5e6789756uL,
        0x0153ad748e88c53fuL,
        0x2ecd3aa617478593uL,
        0x045e5e5db98318a5uL,
        0x2ec68a9e9f7b2f99uL,
        0x04576e6798f53e9auL,
        0x2ec161c2a1de488duL,
        0x05521393590da64buL,
        0x2ebacda38e82463buL,
        0x005be32dc731f12cuL,
        0x2eb4a9c33e058099uL,
        0x04d5824d30f3fce1uL,
        0x2eafdaf4969fc44fuL,
        0x02e09660e736b8bduL,
        0x2ea88d45a53c41c5uL,
        0x01d994b0856743cbuL,
        0x2ea2eba8f55fe897uL,
        0x0153b9051c5e7679uL,
        0x2e9d287e1e77c859uL,
        0x02de689bae600601uL,
        0x2e96770239fc87e5uL,
        0x04d77071c1633b26uL,
        0x2e914e513c1b20dbuL,
        0x055210a174166fcduL,
        0x2e8aa90041143186uL,
        0x01dbd7abebe480e6uL,
        0x2e8488642c71cfa6uL,
        0x0255740f6d4ed277uL,
        0x2e7f9f9ce5a157bauL,
        0x02e0874302ee34fduL,
        0x2e785974997b931fuL,
        0x02597701e51a6bfeuL,
        0x2e72bf0c37efc00auL,
        0x03d39d3aac239fe2uL,
        0x2e6cdc89092e43c2uL,
        0x02de36341a88ea0cuL,
        0x2e6636f0e2785c53uL,
        0x055743c5e4db43f9uL,
        0x2e6118b19def65f8uL,
        0x0251e9b8ad36fd99uL,
        0x2e5a4fd2c459c710uL,
        0x00db94cde5e4fc30uL,
        0x2e543ea7a73d5cf0uL,
        0x01d53b3a109a94aeuL,
        0x2e4f26454740b953uL,
        0x01e057635a1ed1dfuL,
        0x2e47f60ab495565cuL,
        0x02d926f55b776f91uL,
        0x2e426de8be09d875uL,
        0x05d35abb1f1cadefuL,
        0x2e3c5889cb51dbb8uL,
        0x025dc853b381e5a0uL,
        0x2e35cbe6a335189cuL,
        0x02d6e96e5d005f5duL,
        0x2e30c22190c33c64uL,
        0x06519fc0dba0e848uL,
        0x2e29c42b0a7816acuL,
        0x015b1c21d6e11086uL,
        0x2e23ce41b9a97542uL,
        0x0254d91f3701143cuL,
        0x2e1e71ba6efe048buL,
        0x01e007de792cfd6euL,
        0x2e176552635a3b26uL,
        0x0458a6663a0ececbuL,
        0x2e11fa1c7f04e719uL,
        0x0152f310e41037d6uL,
        0x2e0b9f88d1e59fb3uL,
        0x00dd2185735c5ad9uL,
        0x2e0538582347c59duL,
        0x04d66381bdd98a02uL,
        0x2e004c9ca3c242acuL,
        0x06d1346f1ba5a69auL,
        0x2df9093a8968bba5uL,
        0x025a706fd9470fb8uL,
        0x2df339c31e0d51b7uL,
        0x03545000f1eec014uL,
        0x2ded8619415342d2uL,
        0x045f3510620184eauL,
        0x2de6aa95f63dd016uL,
        0x02d7f84791f6fdbbuL,
        0x2de16648113f6ec5uL,
        0x0752689bc620188buL,
        0x2ddab5b65b277be6uL,
        0x045c45998d7521aeuL,
        0x2dd47f9aad3382feuL,
        0x0155b50e4b7d6356uL,
        0x2dcf7591b1b1c875uL,
        0x0060aa3508d5db00uL,
        0x2dc82335294ba25fuL,
        0x0359959eb6f64db6uL,
        0x2dc2848053b7dfb1uL,
        0x00d3a2fb2a16d1ccuL,
        0x2dbc68a6f5a8ef61uL,
        0x035e23b370697cbbuL,
        0x2db5c9ffcce7e5fduL,
        0x015720876851d9fbuL,
        0x2db0b5b54d487d34uL,
        0x04d1be79c992aff6uL,
        0x2da9a0421e5c5d71uL,
        0x01db3980569c43a5uL,
        0x2da3a5c4268d4e27uL,
        0x0154e1fc4f822568uL,
        0x2d9e1fba80d34a40uL,
        0x03e0042910b94342uL,
        0x2d97172912ec21f8uL,
        0x01d8908e30f7a1b3uL,
        0x2d91b271db151968uL,
        0x02d2d5e5a1b8288euL,
        0x2d8b1f9ef2d6b135uL,
        0x00dce1b3b9ea6267uL,
        0x2d84c872d1af92bcuL,
        0x01d623e8fb994f23uL,
        0x2d7fd87064e02a6fuL,
        0x00e0f8695160ca38uL,
        0x2d78652a61cdcd3buL,
        0x02da031b186be289uL,
        0x2d72af84a660968cuL,
        0x05d3eee8e04dc3a0uL,
        0x2d6c9f07af149226uL,
        0x01de8bd23cc416f0uL,
        0x2d65eacf76fffc0buL,
        0x045766e8d5583265uL,
        0x2d60c80f3efbbf3fuL,
        0x0051ed2fab014c43uL,
        0x2d59b1f8ffd8f3c7uL,
        0x03db76010ebb6c6auL,
        0x2d53ab5d5023fe4auL,
        0x015507d813502ab7uL,
        0x2d4e1c174ea2aaa5uL,
        0x02e01aa61c90eaccuL,
        0x2d470b05029068dauL,
        0x02d8a90544ab274duL,
        0x2d41a1fba21de5efuL,
        0x0652e0fb0911dd84uL,
        0x2d3afb70654af058uL,
        0x04dce6f24739f7c7uL,
        0x2d34a458b53b2a83uL,
        0x04561eefc532711fuL,
        0x2d2f944d95c81983uL,
        0x0160edb77098a960uL,
        0x2d28272ab43f7156uL,
        0x00d9e82e04d9025fuL,
        0x2d2278886c5a4d73uL,
        0x00d3d237a2e0f859uL,
        0x2d1c3f57b512a1f2uL,
        0x025e5385c7d0efe0uL,
        0x2d1598c52c5d1745uL,
        0x03573258d0b919ebuL,
        0x2d10828ad1da0982uL,
        0x0451bdb57d01ceccuL,
        0x2d093d4935512f53uL,
        0x045b223e5e67d24auL,
        0x2d034a3670d3cd58uL,
        0x0554bf43098a2ef1uL,
        0x2cfd7b67cefff215uL,
        0x045fb93db1e39a21uL,
        0x2cf686e7356020d1uL,
        0x0358402d3eada60auL,
        0x2cf135e695d6d4f7uL,
        0x04d2892e31597360uL,
        0x2cea4b6028e1ae52uL,
        0x005c5502f868f04buL,
        0x2ce415808da66669uL,
        0x00d5a670a5d83e0euL,
        0x2cdead51e60a821duL,
        0x01608ac71830fd4euL,
        0x2cd76cfe88ffbfa6uL,
        0x04d9467d9d3bce7duL,
        0x2cd1e2e61d740a90uL,
        0x05d34ea92731d6f0uL,
        0x2ccb4f6c22875415uL,
        0x005d7e402cf49a21uL,
        0x2cc4d8e03e448997uL,
        0x04d6860e96265ba8uL,
        0x2cbfd2c6816f010buL,
        0x01e132f279000564uL,
        0x2cb8494b75728df0uL,
        0x04da4356bd52863euL,
        0x2cb28836b62851b4uL,
        0x03540cac092d16a6uL,
        0x2cac476ceb4ce0a6uL,
        0x025e9bb8c8c45eaauL,
        0x2ca592d26553a528uL,
        0x04575c6ad9777c96uL,
        0x2ca074be65f60431uL,
        0x06d1d3d889242361uL,
        0x2c991a14719373e4uL,
        0x045b34c7bf3e0108uL,
        0x2c93248b33f78dd8uL,
        0x0654c1bf325b5886uL,
        0x2c8d316bfa6ecf07uL,
        0x015fab351a6d7271uL,
        0x2c8641dc398561eeuL,
        0x035827d8b273a859uL,
        0x2c80f79d08c027e2uL,
        0x03526c35a8453a6euL,
        0x2c79ddabce45ff87uL,
        0x045c18e854f7a653uL,
        0x2c73b6a0443345f0uL,
        0x06556c727238c10euL,
        0x2c6e0b830517633fuL,
        0x00605545196af9e3uL,
        0x2c66e4903f595976uL,
        0x0258e6b62ae03487uL,
        0x2c6170eca4e7a4c9uL,
        0x06d2facf384d3a3buL,
        0x2c5a92756c27d939uL,
        0x045ceddf1e753b81uL,
        0x2c543d40bf74392duL,
        0x02d60b61e0028436uL,
        0x2c4ed3e286c4c0deuL,
        0x0260cbd09b1e5e10uL,
        0x2c477993389df312uL,
        0x035997719e8b73a8uL,
        0x2c41dfa945eaae98uL,
        0x05537e77cf85ca37uL,
        0x2c3b36ec5aa05880uL,
        0x00ddb1e802a6c81fuL,
        0x2c34b749e64b35f5uL,
        0x00d69d3aa6fccfd9uL,
        0x2c2f88d823260c9euL,
        0x0261383f4dd09079uL,
        0x2c27ffa0f1fabb65uL,
        0x01da388f33976b7buL,
        0x2c2242e12375b352uL,
        0x00d3f613589599c6uL,
        0x2c1bc9a844ffd2b4uL,
        0x035e635a66e3ebe7uL,
        0x2c1523af73f84782uL,
        0x03d720bfb4a981d7uL,
        0x2c10146a610e0588uL,
        0x025199a49bcc5100uL,
        0x2c087590d6d36008uL,
        0x01dac8ae259e160cuL,
        0x2c0299b80ea6bb7euL,
        0x03d4609b0c4183cauL,
        0x2bfc496292aa266buL,
        0x015f00af26520f9duL,
        0x2bf5817f72c95e4cuL,
        0x025794ce31e24c7buL,
        0x2bf059392396d037uL,
        0x06d1ef2877dbfcaduL,
        0x2be8da5a346cbb3euL,
        0x02db468dc95cb829uL,
        0x2be2e36a9eb80d32uL,
        0x00d4bd213115ac94uL,
        0x2bdcb4fb203e18a0uL,
        0x00df88862b544527uL,
        0x2bd5cfe5be9615c6uL,
        0x03d7f861b04cbe3auL,
        0x2bd0923c6394f694uL,
        0x0552380a7a548a2fuL,
        0x2bc92d18166ccd50uL,
        0x045bb1122f6e5762uL,
        0x2bc31f510cb3f506uL,
        0x05550ad48dd9b3a6uL,
        0x2bbd0b7c794af437uL,
        0x02dff9ab8e5d6631uL,
        0x2bb60e2f23228decuL,
        0x05584a97f6b3e853uL,
        0x2bb0bef1906dac58uL,
        0x00d273a4b16ba84fuL,
        0x2ba96d0ca88e4fbfuL,
        0x045c07484e1da469uL,
        0x2ba34ce1af3c1b5fuL,
        0x03d549037ceef1feuL,
        0x2b9d4c1f7c67dd18uL,
        0x00e0298e0fc06037uL,
        0x2b963bcc0600e3b1uL,
        0x01d88ab45875f419uL,
        0x2b90def17046c37euL,
        0x02d2a16e161fa35fuL,
        0x2b8999a40ba75f41uL,
        0x035c48699c75f345uL,
        0x2b836bb3093bcf7fuL,
        0x0155771e906a9978uL,
        0x2b7d764e5657aa1fuL,
        0x02604a04a1699caauL,
        0x2b7658528dc53bd4uL,
        0x03d8b822865b44e6uL,
        0x2b70f1f1acd583c0uL,
        0x0352c0fc98ac934cuL,
        0x2b69b2768ee2e27fuL,
        0x03dc73df0b6d4334uL,
        0x2b637b7d60833afbuL,
        0x005594bab8ddacb1uL,
        0x2b5d89a6c43f4c10uL,
        0x02605dee05833b3cuL,
        0x2b5663803afd90e1uL,
        0x02d8d278c9cbfc58uL,
        0x2b50f7c5f2e4264fuL,
        0x06d2d206b997c2ccuL,
        0x2b49b74a41343d69uL,
        0x025c89434d36542fuL,
        0x2b437c1bd3bb9cfeuL,
        0x0255a192e33cf627uL,
        0x2b3d85fb90bdf218uL,
        0x00e0651bc0c61b20uL,
        0x2b365d3aea4b609duL,
        0x0558d9799e5f2521uL,
        0x2b30f0609e7aa673uL,
        0x04d2d464a6b30dc2uL,
        0x2b29a813d2878f73uL,
        0x035c88645e6c88eeuL,
        0x2b236d8ce9d2217auL,
        0x05559d89052b0525uL,
        0x2b1d6b5543d3c940uL,
        0x00605f7d07f3fb02uL,
        0x2b1645913a262a35uL,
        0x0358cd14a1185c8duL,
        0x2b10dbd2f003b6a4uL,
        0x05d2c810d60e767euL,
        0x2b0984f6bfe6777fuL,
        0x02dc714448c370a6uL,
        0x2b034ff297cd534cuL,
        0x045588a691f2cd1fuL,
        0x2afd39f201da2254uL,
        0x04604d1f01416963uL,
        0x2af61cba521cabb4uL,
        0x02d8ad66d03eba59uL,
        0x2af0ba4cc94c45b2uL,
        0x0452ad281b8cc2a0uL,
        0x2ae94e44c9a075e6uL,
        0x02dc44191b160ec2uL,
        0x2ae32391bcecdc02uL,
        0x03d5631c55b5d22cuL,
        0x2adcf2449a3fda4buL,
        0x00602e2c911c7929uL,
        0x2ad5e3150cc8eda3uL,
        0x04587aba1a7120bfuL,
        0x2ad08c1bf3c985f9uL,
        0x06d283e938a586f7uL,
        0x2ac9047cb663bb8cuL,
        0x015c014c17012593uL,
        0x2ac2e8d117dfdd43uL,
        0x05552d41b7968429uL,
        0x2abc94f2cb2815a7uL,
        0x036002edb3674f27uL,
        0x2ab599268900e7bbuL,
        0x045835843f5f0b0cuL,
        0x2ab051aaf415041cuL,
        0x06d24cb3e8b7d756uL,
        0x2aa8a84869fc8267uL,
        0x025ba9781881c8a9uL,
        0x2aa2a037bab743e1uL,
        0x02d4e79366e7a470uL,
        0x2a9c22d2c350e305uL,
        0x045f978cc962d426uL,
        0x2a953f982a03a247uL,
        0x03d7de65083f0e21uL,
        0x2a900b7f70f68971uL,
        0x065208076f18ea30uL,
        0x2a883a7a5a0b9d4cuL,
        0x03db3d6740403453uL,
        0x2a824a6b05eb3edauL,
        0x025492b17a8d9ad4uL,
        0x2a7b9ce7efad864buL,
        0x035f126a42ab2a64uL,
        0x2a74d7351162fad8uL,
        0x01577623e1a3ca2fuL,
        0x2a6f74706d1f613cuL,
        0x0161b680aeae0c3cuL,
        0x2a67bc0a6e57fbc5uL,
        0x00dabe0fed214bcauL,
        0x2a61e82c35430e3cuL,
        0x04d42f5d0cb0afebuL,
        0x2a5b045f25c98b4buL,
        0x01de77a20528f8f5uL,
        0x2a5460e7202036c6uL,
        0x04d6fdace394b03cuL,
        0x2a4ebd15c07c2accuL,
        0x02e158d7d54f1681uL,
        0x2a472e125d540295uL,
        0x025a2c9115542385uL,
        0x2a417a558b9c184euL,
        0x0653be755f8b210cuL,
        0x2a3a5a8a3f3de092uL,
        0x01ddc88f077bd369uL,
        0x2a33ddb38ecb5b52uL,
        0x01d6760d57bb9982uL,
        0x2a2df2826b036577uL,
        0x0460efdda755dbb3uL,
        0x2a2691c997f37f0duL,
        0x03598a2e123c782euL,
        0x2a2101d72c627ff6uL,
        0x04d340f49a722110uL,
        0x2a19a0db3d2b8dacuL,
        0x00dd06b3f65f6fd0uL,
        0x2a134eb72e63e591uL,
        0x04d5e06fcff790f4uL,
        0x2a0d166c8f34fca4uL,
        0x01e07c787991a680uL,
        0x2a05e880d9f1fe43uL,
        0x00d8d849f54265f7uL,
        0x2a007fb3b2ff1602uL,
        0x03d2b7ec30262d2buL,
        0x29f8d8df0cbffd51uL,
        0x03dc33b5a8ad639fuL,
        0x29f2b52265317648uL,
        0x01d53e17e1a8afaduL,
        0x29ec2aa6bd34f17buL,
        0x015fff41d2913dabuL,
        0x29e5339d751ff2a0uL,
        0x035818627da2e9e4uL,
        0x29dfe9f93308c404uL,
        0x0262248100f21115uL,
        0x29d80438073219dduL,
        0x035b515531d535ebuL,
        0x29d21234fbc4a127uL,
        0x0354905d9b84e0cbuL,
        0x29cb31198aa5f8aauL,
        0x025ef4bcc5f71a72uL,
        0x29c474946f304456uL,
        0x00574c0ac8d03b2buL,
        0x29bec59d00f3fe37uL,
        0x02e187e74c209a91uL,
        0x29b7249848679fa8uL,
        0x02da6169b09c4411uL,
        0x29b16739cec78bd4uL,
        0x0053d8a8ccb26cd9uL,
        0x29aa2bbd0795adeeuL,
        0x01dddb87127c2076uL,
        0x29a3ace589cd3351uL,
        0x055674e5d7be735cuL,
        0x299d949ad392f075uL,
        0x00e0e35e84d33d3fuL,
        0x29963bbbf78651cbuL,
        0x035965d9f895d99cuL,
        0x2990b5827a3ba381uL,
        0x03d3186c34406960uL,
        0x29891c922f9ee4bfuL,
        0x03dcb5d51a48d7d4uL,
        0x2982de164c74e725uL,
        0x035594a1039f0199uL,
        0x297c5941f108d9d0uL,
        0x03e0382d1e479246uL,
        0x29754b639c219648uL,
        0x0558609634a384ccuL,
        0x296ffcc624730979uL,
        0x02e25120afe02122uL,
        0x2968059c757355aduL,
        0x03db85e31314f4b4uL,
        0x296209ad26ca18d8uL,
        0x0454acee7c0fcbafuL,
        0x295b15e18d0d2d12uL,
        0x015f0f38c6449ad9uL,
        0x2954554e9983b016uL,
        0x00d753919ff4b182uL,
        0x294e865bf893f8f3uL,
        0x0361844080030d76uL,
        0x2946e8db855aac99uL,
        0x035a4dede3a3eb93uL,
        0x2941312cc0ae5d03uL,
        0x03d3bf7fe7aa33a0uL,
        0x2939ccc1bfbf7ecbuL,
        0x01dda5e8d4d639eduL,
        0x29335b35e7d0088duL,
        0x045640bc7176cda7uL,
        0x292d0a5ff60b92ceuL,
        0x0260b342b640cc13uL,
        0x2925c84558f35d95uL,
        0x0159102c47629cb9uL,
        0x2920560f8bafb2c6uL,
        0x05d2ce013e375d0fuL,
        0x2918801ce509ea26uL,
        0x025c36f07720a932uL,
        0x29125ec7207b3c63uL,
        0x055529fe13854ed9uL,
        0x290b8b58f7c67c36uL,
        0x00dfbf2dc269c35duL,
        0x2904a5c0b3b7424duL,
        0x0157cec854a40ddcuL,
        0x28fef3874e46141auL,
        0x03e1da13f1aaaee6uL,
        0x28f732197e24d857uL,
        0x02dac4c46230c45cuL,
        0x28f1619ff0ea7ec5uL,
        0x04d4112fbeff8a1fuL,
        0x28ea0bb46a0a2c52uL,
        0x045e15420dda8758uL,
        0x28e383201c8ba71auL,
        0x02568bd97eb5b05duL,
        0x28dd3b4e4b894767uL,
        0x02e0e54a78756b6buL,
        0x28d5e4c4aaef0130uL,
        0x015951c14f527745uL,
        0x28d0654a030d3e70uL,
        0x02d2f8178dd14a04uL,
        0x28c88dc03d1ca800uL,
        0x03dc6b6bf9361ee4uL,
        0x28c2621d65152a67uL,
        0x0255495f2949c65euL,
        0x28bb860981f4834auL,
        0x015fe24891c8ca0cuL,
        0x28b49a0d4c97c280uL,
        0x0557e02609a87253uL,
        0x28aed66ed1143992uL,
        0x02e1e064158c947buL,
        0x28a713a5a10cc9afuL,
        0x04dac4304f253262uL,
        0x28a14455cbbff468uL,
        0x0654093bdea6e36fuL,
        0x2899d62205df47a5uL,
        0x045dfe14a435c3c2uL,
        0x289353bfdeb15aa4uL,
        0x01d6720e3d624fdcuL,
        0x288ce97f23783a55uL,
        0x01e0cba8970a9d66uL,
        0x28859f649793ea99uL,
        0x035921e961b81171uL,
        0x28802b46c188f22duL,
        0x03d2cd3135c626d1uL,
        0x28782dcfdba2d59buL,
        0x035c2097f7f7c953uL,
        0x287213830f44d648uL,
        0x0155096e15b063dbuL,
        0x286b0639acae41c6uL,
        0x035f76b39886a20duL,
        0x286432d063e4cc5auL,
        0x02d786c2636e4e2auL,
        0x285e3096b161ade0uL,
        0x02e196dc712e8651uL,
        0x28568f1646f450ccuL,
        0x005a4c39680abb0buL,
        0x2850dad51a121c5euL,
        0x06d3a80eb1934625uL,
        0x28492ed52465cf13uL,
        0x015d6196b3830612uL,
        0x2842cf8cdb32b26duL,
        0x00d5f4b3b930a91auL,
        0x283c1934bb7035c1uL,
        0x016067b3db09279euL,
        0x2834fbc11c19c0b6uL,
        0x04d8832413bcb6f5uL,
        0x282f5613cdc1ad52uL,
        0x01624f8b72bbd6eeuL,
        0x28276547ab0f816auL,
        0x015b5a5bcacf14dduL,
        0x2821770c93ef3135uL,
        0x05d46d8046ba690cuL,
        0x281a128a30d837eauL,
        0x02de8209bd7c6d4duL,
        0x281375630e92b790uL,
        0x0056c744b66f6406uL,
        0x280d0a93cd8add1euL,
        0x0161015024fefc8duL,
        0x2805ab4549d6cf14uL,
        0x03d9631ba1694964uL,
        0x28002a8fed4a1943uL,
        0x06d2f2b3b1ae197duL,
        0x27f81e6d5efc2eceuL,
        0x025c47e5b8f9de0cuL,
        0x27f1fd54f3e20bfbuL,
        0x05551a481761d265uL,
        0x27ead523512d80aeuL,
        0x00df7d2ff106229cuL,
        0x27e4023f854f9c86uL,
        0x02577da522f79ec5uL,
        0x27ddd649c8fad0d4uL,
        0x02e185a192bd02b4uL,
        0x27d63e684c4d4571uL,
        0x02da22ed5ef67f83uL,
        0x27d094b5ecc6e28fuL,
        0x06537d9a85948033uL,
        0x27c8b7643330549duL,
        0x02dd10da89b8212auL,
        0x27c26b65f14cd4dauL,
        0x0155ab7d4224f7e2uL,
        0x27bb734f53e57228uL,
        0x0160276587fa1c20uL,
        0x27b473b9d1931175uL,
        0x015814bdb918424duL,
        0x27ae78d8c6e84fdcuL,
        0x0261f2684f2af658uL,
        0x27a6b2a2c93cd65auL,
        0x015abf540fb4e1a1uL,
        0x27a0e7a7b055d281uL,
        0x01d3eddfeeed0dd2uL,
        0x27992d87cacce695uL,
        0x01ddb1c82f79707duL,
        0x2792bf57b6e0d98cuL,
        0x03d61ea0b7eb4c3cuL,
        0x278bea4f9488e120uL,
        0x03e0799f1fb897d8uL,
        0x2784c7d8bf7bdc41uL,
        0x01d889f21fdb1d69uL,
        0x277eef6b8bfa9224uL,
        0x036245c20ba28a39uL,
        0x277705ed2bbfd521uL,
        0x02db3598a0d59840uL,
        0x277121f1b69882ebuL,
        0x0354418fde75923euL,
        0x27697ec608197c79uL,
        0x015e27e05b6c31f9uL,
        0x2762f7b0edc74f1cuL,
        0x00d671af7f5d8858uL,
        0x275c380c41f75030uL,
        0x0060b3d4442eda68uL,
        0x2754fd20f15083b2uL,
        0x0558db341e4d4306uL,
        0x274f37ea8d01e9c4uL,
        0x02e27e37e3bc73c9uL,
        0x274736cebb19a200uL,
        0x045b83a639f29a80uL,
        0x2741428c012e2c57uL,
        0x00d47730acf38edcuL,
        0x2739a9ae80c06017uL,
        0x035e710d5155d028uL,
        0x27331371c2b63b80uL,
        0x0256a331ab64b688uL,
        0x272c5b240b14f4d5uL,
        0x0360d4fd25f7f52euL,
        0x2725129ffd17a136uL,
        0x01d90712f4e38e37uL,
        0x271f510ba62354a4uL,
        0x03e29ac951c1e60buL,
        0x27174468acd1611cuL,
        0x01dba819d5f14678uL,
        0x271148e1d96c299euL,
        0x02548dce2dc3ecd5uL,
        0x2709ad7d58aaba43uL,
        0x035e8c0193d16d55uL,
        0x2703121b71d77178uL,
        0x04d6b2456938b866uL,
        0x26fc52f68dd90e64uL,
        0x01e0dc826696c76cuL,
        0x26f507f397188495uL,
        0x05590cc63cdbf2a2uL,
        0x26ef3a5bdf92c387uL,
        0x02629af3c144f8c0uL,
        0x26e72e7cbdbb95dbuL,
        0x01dba24cc0f4c8e2uL,
        0x26e134d638b07142uL,
        0x05548500e815d897uL,
        0x26d98a2111174d79uL,
        0x00de7841c45926d0uL,
        0x26d2f3b409e1b7b5uL,
        0x03d69ea5b1b71301uL,
        0x26cc1fa91a869694uL,
        0x02e0ca4195cda6d3uL,
        0x26c4dd4c7d7ec9fauL,
        0x0158ec33daf13649uL,
        0x26bef442d8796794uL,
        0x02e27eb66fea5e85uL,
        0x26b6f56f0c0f22b8uL,
        0x04db72598c77c448uL,
        0x26b106c4a594a046uL,
        0x04545cf12a60cb9auL,
        0x26a9403b0e4bd1b9uL,
        0x01de36284e81b5ffuL,
        0x26a2b8c63e7468c1uL,
        0x025668ac570f2fc8uL,
        0x269bc22598793379uL,
        0x00e09e8e37ef2488uL,
        0x2694936d06178105uL,
        0x04d8a5f0c63b5c24uL,
        0x268e7fffb3b16a7cuL,
        0x0262469273320bdauL,
        0x26869a431ed205a0uL,
        0x015b191b44e70edfuL,
        0x2680bf7e7cce4d06uL,
        0x03d41655d7606103uL,
        0x2678d11ace4d8996uL,
        0x01ddc6e2b76185d5uL,
        0x2672625d4b960a47uL,
        0x0156114f58eab906uL,
        0x266b3c139841a734uL,
        0x03e05a2f4a403a4duL,
        0x26642ba35d81be5buL,
        0x04583b3c9af7ee45uL,
        0x265ddf9fa6fc513auL,
        0x00e1f386e3013e68uL,
        0x26561e943a26f541uL,
        0x03da9826f127d04duL,
        0x26506044c28d2704uL,
        0x00d3b26ef9596f74uL,
        0x26483eb403668f93uL,
        0x03dd2c68adc24dd3uL,
        0x2641f1fd15ed30feuL,
        0x02d59a199b7c8167uL,
        0x263a8fcbdc7eab50uL,
        0x02dffcb2bfa5b8dauL,
        0x2633a7bfb4be9962uL,
        0x01d7adf828472cfduL,
        0x262d15ee90987617uL,
        0x03e1870951a86a79uL,
        0x2625848951944920uL,
        0x0159f1bfa110cbbauL,
        0x261fd57d7b45b3cauL,
        0x00e332fc55367264uL,
        0x26178b8ffae32bf0uL,
        0x005c696d39db75f3uL,
        0x26116996dab0cd1duL,
        0x0455051f4ea04fdfuL,
        0x2609c046dcaa75a4uL,
        0x005f194b2a4cb970uL,
        0x26030a06c462f23duL,
        0x035700975cbb46aauL,
        0x25fc2662350ce7fauL,
        0x01e102fae0ec7794uL,
        0x25f4cec5169fb931uL,
        0x025928c588cfb6d9uL,
        0x25eec1db7d8e44b5uL,
        0x01629a3060c44f3auL,
        0x25e6babae8929705uL,
        0x035b814aa869e0e4uL,
        0x25e0cb7ae5506e7euL,
        0x00d454ee7edd0063uL,
        0x25d8d106f7f4047duL,
        0x03de0e0b72e6ef2euL,
        0x25d255213192c405uL,
        0x0356360f251c2f1fuL,
        0x25cb1500fc71b699uL,
        0x0360699a6631f93fuL,
        0x25c40052c8ba04b4uL,
        0x025840a0d97bb129uL,
        0x25bd8a3d24511c07uL,
        0x00e1eaa023d58a69uL,
        0x25b5cfadd7b9715fuL,
        0x045a77ea01d8b821uL,
        0x25b01a47ddad3ea8uL,
        0x00538c7c7057a652uL,
        0x25a7c5ff3799c35auL,
        0x02dcdf6c504a93e5uL,
        0x25a18c087e86a1f2uL,
        0x045551bff88c1175uL,
        0x2599e64530b957f3uL,
        0x035f7ae8590bb800uL,
        0x25931c908986e1a8uL,
        0x01573d293026bc2auL,
        0x258c33b25da2082euL,
        0x00612730a9790f69uL,
        0x2584ce362055227euL,
        0x02d951a7082f394auL,
        0x257eb1b0ae0a386auL,
        0x00e2af1081b22794uL,
        0x2576a3779e1ff3afuL,
        0x03db925bc48353e0uL,
        0x2570b1f245435ee9uL,
        0x03d4575deb5305a2uL,
        0x25689efddb97fd17uL,
        0x02de029ff0fc8645uL,
        0x256227180cb0a8c9uL,
        0x04d6228a92a17423uL,
        0x255ac39e8a7de061uL,
        0x04605302bb5e3a1auL,
        0x2553ba5b5279aa24uL,
        0x01d81331d3a2cc81uL,
        0x254d145ea8ff6403uL,
        0x00e1c02d69097c72uL,
        0x25456df011e743b8uL,
        0x035a2c1b0ae83a64uL,
        0x253f94750d0f9307uL,
        0x02634ad734ae6135uL,
        0x2537442e71728409uL,
        0x03dc703bfdc748cduL,
        0x253123a683e9b9d5uL,
        0x0054f5290291de6euL,
        0x25293f94a8e393e4uL,
        0x045ee2bb5a2a4470uL,
        0x252298449094a07fuL,
        0x04d6c16f34d9525euL,
        0x251b62c8f87855a8uL,
        0x0160c379a70923bcuL,
        0x25142a02f59d51eeuL,
        0x04d8b21b8919710fuL,
        0x250db09bb0ffb21fuL,
        0x00e2303a1b68b2deuL,
        0x2505daee76f997a7uL,
        0x04dac9c706a79cfcuL,
        0x25001604a662bf4buL,
        0x0553b983b3f72fb5uL,
        0x24f7ad33d50dacd0uL,
        0x015d0b33fd9b6e85uL,
        0x24f16c1e4c8c451auL,
        0x0255615904c6373auL,
        0x24e9a32159dea0d7uL,
        0x02df7950165d693duL,
        0x24e2dc48781056c8uL,
        0x035729dc070c926auL,
        0x24dbbf2871addffauL,
        0x02610b9b38c6e833uL,
        0x24d4684a4152d4e0uL,
        0x0259154f9f73ee5fuL,
        0x24ce03df4eb2c204uL,
        0x00627418ebfd96beuL,
        0x24c6120558a89b11uL,
        0x02db26192fa2f36euL,
        0x24c03a014bcb5351uL,
        0x0553f7df7d25b3e6uL,
        0x24b7db773a6f6623uL,
        0x025d5ec232ba3385uL,
        0x24b1893b9023690cuL,
        0x055598c75ff21ea4uL,
        0x24a9c6ba6a494659uL,
        0x025fc1f9e46a53e2uL,
        0x24a2f125d64e7642uL,
        0x02d758c452444076uL,
        0x249bd607b51aff82uL,
        0x02e1294b791c6529uL,
        0x2494735d5e25dd32uL,
        0x025939e692035be7uL,
        0x248e0bb7795ebab2uL,
        0x01e289cc9b3b4107uL,
        0x248611962fb4b008uL,
        0x015b3e5c199dc217uL,
        0x248035217aa6e0acuL,
        0x04d40415be2c6028uL,
        0x2477cd9c096da3b2uL,
        0x03dd6871e2c76342uL,
        0x24717a22cd2a508fuL,
        0x025599d2a64857abuL,
        0x2469a95351e8c9f1uL,
        0x00dfba952efabe51uL,
        0x2462d63f329a8bcbuL,
        0x01574cc660d4897auL,
        0x245ba6ba0cb47e2auL,
        0x02e11baa6a990cd8uL,
        0x24544ae89d144107uL,
        0x03591ecc31adec4euL,
        0x244dc7e8d1b8f556uL,
        0x006270b14a1f9816uL,
        0x2445d9a42222275cuL,
        0x025b11d883fd3ec1uL,
        0x24400789e350bd19uL,
        0x04d3ddca348b8e79uL,
        0x2437840aaba80c98uL,
        0x015d27f9dd765764uL,
        0x24313f45ccd8c934uL,
        0x04d56472f42babf3uL,
        0x24294bc9a9955f26uL,
        0x01df6359d3980ea5uL,
        0x24228c5f3eaf8edduL,
        0x0257063ccd1b83c6uL,
        0x241b32a3c3e46a35uL,
        0x00e0e31f012ad2b3uL,
        0x2413f01c91fe7f46uL,
        0x04d8c4cd2c02ec2duL,
        0x240d3a718c61d154uL,
        0x00e2298481c2ca0duL,
        0x24056bd3dd5a05c1uL,
        0x00daa1de55237abcuL,
        0x23ff65222fadfbffuL,
        0x0263861db33230b0uL,
        0x23f700eb717cfb77uL,
        0x015c9f401331dbf6uL,
        0x23f0da5e12700c8duL,
        0x00d4fa3a533642f6uL,
        0x23e8b0da54d3c71euL,
        0x045ebed8656f1a7buL,
        0x23e215aeed941b43uL,
        0x0256873a105b43c2uL,
        0x23da7d28bd609e50uL,
        0x0160815216360470uL,
        0x23d3659f3261d18fuL,
        0x05582e8d038330cauL,
        0x23cc6770887b13f6uL,
        0x00e1b65bea6b7e6auL,
        0x23c4cb570f463d9cuL,
        0x03d9f1b427ce89a2uL,
        0x23be715dafe5cd5fuL,
        0x0262ff9fffd4f5f9uL,
        0x23b6480ba9b1723buL,
        0x035bd241d06b6757uL,
        0x23b04e575dd6f2ebuL,
        0x03545e411382662buL,
        0x23a7dcff6d521467uL,
        0x015dd1da1bc7ec85uL,
        0x23a1759a98201ff3uL,
        0x00d5d36e9f7af39cuL,
        0x23998b82586ccf2duL,
        0x005ff233639de02auL,
        0x2392af6afc0ce651uL,
        0x0257606528b3cf28uL,
        0x238b54f244df93deuL,
        0x02e11a8b54a30c34uL,
        0x2383fcc4e4385b18uL,
        0x01d9066e8a3084aduL,
        0x237d3abb2d5b9282uL,
        0x00624e2ffedd9f78uL,
        0x23755eaec016b2b5uL,
        0x025ac6e23cde6ac9uL,
        0x236f3e576e5bfb2cuL,
        0x006394ff72563c26uL,
        0x2366d6394041cb01uL,
        0x015ca3259bb8013euL,
        0x2360b0a8012d71fbuL,
        0x02d4effb58fcce20uL,
        0x2358647f7f3a91deuL,
        0x025e9cac23b8427euL,
        0x2351d29e5c60946auL,
        0x04d6602f707600f3uL,
        0x234a0aa72640fd47uL,
        0x02605a7bd790a4bcuL,
        0x234305e23384e58auL,
        0x01d7e6b1b23c38f4uL,
        0x233bc9e08de1532fuL,
        0x00e176cc55ca9b80uL,
        0x23344b4e89c6a35euL,
        0x04d984a277e8539auL,
        0x232da366d9d2b974uL,
        0x0262a417253e014buL,
        0x2325a3c60cb2c6b0uL,
        0x035b3b2c9b4277c6uL,
        0x231f98800fc076dauL,
        0x0363e333559670c8uL,
        0x23171033226bf0afuL,
        0x01dd0b8591b88278uL,
        0x2310d53e944a7e18uL,
        0x01d534ff7f271b4duL,
        0x23089187f3d75a14uL,
        0x025ef6ed82d51675uL,
        0x2301ed5d0deddfb6uL,
        0x03569a61d0edc9d2uL,
        0x22fa28be72757b85uL,
        0x01e07f57aca805f1uL,
        0x22f3154ef266983cuL,
        0x03d814481a9f253cuL,
        0x22ebd6d859990531uL,
        0x03e1921067277b5duL,
        0x22e44dcd404b4fccuL,
        0x0459a3a7d2712f82uL,
        0x22dd9cdf2aadd6a5uL,
        0x01e2b45137355f77uL,
        0x22d5979672b76b95uL,
        0x03db497e1657b91buL,
        0x22cf7be424410478uL,
        0x01e3e6cfcc06ed27uL,
        0x22c6f36e7903ba4euL,
        0x045d06cfa865bc4euL,
        0x22c0ba8019bd4e85uL,
        0x03d52a47395ed2aeuL,
        0x22b8621eaa755f33uL,
        0x045edca8e605e67auL,
        0x22b1c4a9efdce654uL,
        0x02d67f77ef705254uL,
        0x22a9e475b5aaea97uL,
        0x00e0660edcde1e02uL,
        0x22a2dd03980220abuL,
        0x0357e727aec99554uL,
        0x229b7b478b8fda1cuL,
        0x02616b24c391593buL,
        0x22940424c4fd21f6uL,
        0x02d96221780dfe95uL,
        0x228d276d459f43c6uL,
        0x03627e2788696d86uL,
        0x22853aa8c500f5d0uL,
        0x01daf1357749947cuL,
        0x227ee9c5073f397euL,
        0x01e39fac2bf7a531uL,
        0x2276812e6a2e8fc0uL,
        0x025c9538eaa71fb0uL,
        0x22706198ecffc0e0uL,
        0x0254d04b3a802aeeuL,
        0x2267d857ef6fe55auL,
        0x00de4f0604536408uL,
        0x22615a4dc243cc5euL,
        0x03d610a0b4ec8401uL,
        0x225940cad97ee071uL,
        0x01e00fbde3ac71c6uL,
        0x22525f772e00c70auL,
        0x0257614bf61d6bfauL,
        0x224abb2fd3f529efuL,
        0x00e103beefa07650uL,
        0x2243718d87e8a0afuL,
        0x0158c2ef94786008uL,
        0x223c48328a4346eauL,
        0x026203fa39242793uL,
        0x2234910b37b4de72uL,
        0x015a36313f8e64ecuL,
        0x222de8817c6f33b8uL,
        0x026310e5f6fbfd44uL,
        0x2225be6c950a7e6euL,
        0x04dbbbb999bb060auL,
        0x221f9ccdcf7c94feuL,
        0x01e42afa66f9fdc1uL,
        0x2216fa2fc442a9d2uL,
        0x04dd54340d9c375duL,
        0x2210b2e58cb15f5cuL,
        0x025552b1ae6aeaa2uL,
        0x220844d490056941uL,
        0x045f004e9f45a94buL,
        0x2201a217943b9ac6uL,
        0x03568887b7750462uL,
        0x21f99edc3fa555f4uL,
        0x00e0605cdc8a1e5euL,
        0x21f29c58e31af831uL,
        0x00d7ccfa0b55e3f7uL,
        0x21eb08c96a2d341buL,
        0x03614b13fa04509fuL,
        0x21e3a2063aa9bfc8uL,
        0x02d92087a96ea8f4uL,
        0x21dc831fc61280f7uL,
        0x006240a6edc95f53uL,
        0x21d4b37d15842e1cuL,
        0x045a83b0db0fa5b6uL,
        0x21ce0e63f582488buL,
        0x01634170d65d2fe5uL,
        0x21c5d11b81c3fea7uL,
        0x015bf6f703f6c8b1uL,
        0x21bfab1b4f400c2euL,
        0x00e44dcd884a52dcuL,
        0x21b6fb3ff8ccf41cuL,
        0x015d7adc6f76430fuL,
        0x21b0ace5d20891a2uL,
        0x00d5661968fc8c68uL,
        0x21a8324934a763f4uL,
        0x01df0fe41a3b588buL,
        0x21a18d7d8058e530uL,
        0x03d68ab147365bffuL,
        0x2199769602e7d2c3uL,
        0x04605b48bc57ed71uL,
        0x21927797b62a04a4uL,
        0x02d7bbf2311e9661uL,
        0x218ac8851524d431uL,
        0x016137b41cf9c9a4uL,
        0x21836b7751d5da7fuL,
        0x02d8fa3947e525d9uL,
        0x217c2874cefea298uL,
        0x01621d7603b6e2ccuL,
        0x2174695ee8470b66uL,
        0x005a45e3910021acuL,
        0x216d96c311be3eb3uL,
        0x01630cd0207d04eduL,
        0x216571909f179505uL,
        0x02db9f4dc504a668uL,
        0x215f13cd05945d89uL,
        0x00e40603dadb780auL,
        0x2156844e0504f766uL,
        0x015d06d41c212c13uL,
        0x21504ff770417c7euL,
        0x005509522cc01f2fuL,
        0x2147a1d7e8c27e50uL,
        0x025e7cd2184183ebuL,
        0x21411dc1d57f7df8uL,
        0x015616fb7b910c11uL,
        0x2138ca6e2e342651uL,
        0x006000d1267395e3uL,
        0x2131f372812d1e14uL,
        0x00d72f3f6faafe57uL,
        0x2129fe4fa21e8c98uL,
        0x00e0cacf12619fe1uL,
        0x2122d1356c845fd0uL,
        0x03d8525cca4f244duL,
        0x211b3db9cc5a58f2uL,
        0x03e19c8ed29100e2uL,
        0x2113b7359a6b9390uL,
        0x045980913a0c5f1euL,
        0x210c88e8c09b9bb2uL,
        0x00e2763b979d57b5uL,
        0x2104a59cf5958097uL,
        0x04daba192db244fduL,
        0x20fde016eddfacacuL,
        0x026357ff9fbc97f4uL,
        0x20f59c942db45eaduL,
        0x035bff2fa5de1e9duL,
        0x20ef437cec9632b7uL,
        0x01e44204156d00fcuL,
        0x20e69c4293cefa3euL,
        0x045d500e0534289duL,
        0x20e059a8a5ce0ce7uL,
        0x00d53470ed39dd97uL,
        0x20d7a4cdf5c8de47uL,
        0x01deacebdf5973c2uL,
        0x20d117e42e10afc4uL,
        0x03d62f6cc2a62dbduL,
        0x20c8b65a792fe13fuL,
        0x03600aff63626acfuL,
        0x20c1dc89fe4a5f8auL,
        0x0157331cb44dd6ecuL,
        0x20b9d10a7562f376uL,
        0x03e0c5bd0cbfba30uL,
        0x20b2a7b1b1593291uL,
        0x02583fa43f4f73d5uL,
        0x20aaf4fe4d278bf9uL,
        0x016186c76677c8f7uL,
        0x20a37971726a776euL,
        0x025955251a12574cuL,
        0x209c225447c48b85uL,
        0x00624e359c6528bbuL,
        0x209451dde15504ebuL,
        0x035a73bf0e7dcf7buL,
        0x208d592869bae136uL,
        0x00631c1d70a5a26cuL,
        0x20853109f6b70a01uL,
        0x04db9b8fd3b82aceuL,
        0x207e99944d35a898uL,
        0x00e3f09320694d40uL,
        0x20761706e7ea0b41uL,
        0x02dcccb2e7856e93uL,
        0x206fe3aefa4cdaa2uL,
        0x0164cba948866255uL,
        0x206703e40ae0b132uL,
        0x02de0741675f15a5uL,
        0x20609bc65f9b8063uL,
        0x04d5ad70c9e433d4uL,
        0x2057f7aeba02f7eeuL,
        0x035f4b51e95f89d5uL,
        0x20514a9f8443d057uL,
        0x04d695f8add0a062uL,
        0x2048f272381e3221uL,
        0x02604c7c2a8ead79uL,
        0x2041fe6a1ccca721uL,
        0x0157854e0a5444cfuL,
        0x2039f437947f2742uL,
        0x02e0f822de49bc54uL,
        0x2032b72bc2a1bb28uL,
        0x03587b7be69a8c26uL,
        0x202afd058f4d5cb9uL,
        0x0161a8a41a9a7340uL,
        0x202374e8637e822euL,
        0x03d9788b1f83908euL,
        0x201c0ce07e3f5246uL,
        0x02625e0558a5c077uL,
        0x201437a22e46ffc8uL,
        0x03da7c824c7683f1uL,
        0x200d23ca31c0220buL,
        0x02e3184a6ce13b46uL,
        0x2004ff5980398e02uL,
        0x025b8765a48c0cf1uL,
        0x1ffe41c1da9f8a5euL,
        0x02e3d775743f06aeuL,
        0x1ff5cc0cd28b81e4uL,
        0x035c9936e428a9d9uL,
        0x1fef66c3f065ea05uL,
        0x00e49b86c1b194ceuL,
        0x1fe69db8a882e290uL,
        0x00ddb1f5331fbe71uL,
        0x1fe049650c331274uL,
        0x02d5647ccc18e717uL,
        0x1fd774577e1faf4euL,
        0x03ded19d0b78718cuL,
        0x1fd0e2e586d3df5cuL,
        0x025632541cab3ac0uL,
        0x1fc84fe1b767669buL,
        0x015ff82820edeaabuL,
        0x1fc17fdd44e1dc6buL,
        0x045705073deb552auL,
        0x1fb9304d9065a4b9uL,
        0x026092c6a4a26abfuL,
        0x1fb220449767742auL,
        0x0157dc8eab3ed87auL,
        0x1faa158f0df4c355uL,
        0x02e12ce032c827ceuL,
        0x1fa2c4123936432auL,
        0x0358b8e0c1372c25uL,
        0x1f9aff97ef6163ecuL,
        0x0261ca5926404568uL,
        0x1f936b3b4511d82auL,
        0x055999f1ae9f978buL,
        0x1f8bee57a0fbbbdbuL,
        0x02e26b285aeabdbeuL,
        0x1f8415b32c89327buL,
        0x03da7fb366632c72uL,
        0x1f7ce1bb2fa9523euL,
        0x00630f431387ee69uL,
        0x1f74c36baf8c2284uL,
        0x02db6a15925d0c25uL,
        0x1f6dd9ad3d89a4a4uL,
        0x0263b69cf0bd5608uL,
        0x1f657454d4c97f21uL,
        0x015c590587256b75uL,
        0x1f5ed615f7bfd7d2uL,
        0x01646127e8d37ba7uL,
        0x1f56285ce2e2e29auL,
        0x035d4c6e38ed7f06uL,
        0x1f4fd6db0d73348duL,
        0x03650ed44039bd53uL,
        0x1f46df705a8252f6uL,
        0x045e4438317c2a1euL,
        0x1f406defd40bdb08uL,
        0x03d5bf9082dc8412uL,
        0x1f379979f15ddb0duL,
        0x015f4049875ce630uL,
        0x1f30f2823287afb5uL,
        0x055673497e5a0d03uL,
        0x1f2856628e34ac2cuL,
        0x01602042eb28efefuL,
        0x1f217913a85a33a6uL,
        0x03d729ea3d219a53uL,
        0x1f19161145d0e326uL,
        0x01e0a2671c8cdbeeuL,
        0x1f120191f16dc708uL,
        0x0457e35c0288722euL,
        0x1f09d86b59187f4duL,
        0x03e12680a24c58f5uL,
        0x1f028be97e6e9064uL,
        0x02d89f8647df9662uL,
        0x1efa9d5434377e7auL,
        0x0361ac7d823a316cuL,
        0x1ef31805749922c3uL,
        0x01595e4eba9494cauL,
        0x1eeb64ad6eec66d3uL,
        0x00e2344a7c981006uL,
        0x1ee3a5cfae5998ebuL,
        0x02da1f993b67371duL,
        0x1edc2e56cdffce02uL,
        0x0162bdd30bebc795uL,
        0x1ed43530bcc0ee3auL,
        0x01dae347debd3070uL,
        0x1eccfa2e45eea63duL,
        0x00e3490165a1de50uL,
        0x1ec4c60fe9d5cbc1uL,
        0x025ba93aee1c301fuL,
        0x1ebdc80ffece4451uL,
        0x00e3d5be7b8309a9uL,
        0x1eb558533bc564e3uL,
        0x025c7150ead1fd0euL,
        0x1eae97d659702f92uL,
        0x00e463f1fe01b7dauL,
        0x1ea5ebdf78f85a02uL,
        0x035d3b6691d169e3uL,
        0x1e9f6959f5cadd73uL,
        0x0064f3825f642b00uL,
        0x1e9680982d0eea8auL,
        0x01de0756e0ca137buL,
        0x1e901e38dd55bfc7uL,
        0x02558454d7cf0720uL,
        0x1e87165faec70a10uL,
        0x01ded4fb1c7fef16uL,
        0x1e8088796f5a025fuL,
        0x05d6164d6a338985uL,
        0x1e77ad1726ce2f3buL,
        0x035fa42ad866b600uL,
        0x1e70f3587953aeb4uL,
        0x04d6a94eea23ecd2uL,
        0x1e68449e977fef01uL,
        0x01e03a5dffc21d0duL,
        0x1e615ebef6827c9cuL,
        0x03573d3b028fc2cfuL,
        0x1e58dcd4e591ac75uL,
        0x02e0a3416f4dd0f1uL,
        0x1e51ca951b79a938uL,
        0x01d7d1f23d694b62uL,
        0x1e497597e1aad585uL,
        0x03610ca917d13a59uL,
        0x1e4236c25d3c18a1uL,
        0x045867540c340902uL,
        0x1e3a0ec452e85047uL,
        0x00e1767d933fa0f7uL,
        0x1e32a32d78fe110euL,
        0x0558fd3ed17c059fuL,
        0x1e2aa8360248e3ecuL,
        0x0261e0a6bf884441uL,
        0x1e230fbc7c8ab284uL,
        0x00d9938feb3469d1uL,
        0x1e1b41c7c6ff8cc6uL,
        0x01e24b0bc63cac6buL,
        0x1e137c54cf4ab1fcuL,
        0x015a2a23bdfb3241uL,
        0x1e0bdb5393a7ccd2uL,
        0x0062b59324d7fd9buL,
        0x1e03e8db3be9418buL,
        0x035ac0d5c13ef72auL,
        0x1dfc74b284572b4buL,
        0x02e32022b5a4d882uL,
        0x1df45533fa93710cuL,
        0x00db57808c42df0buL,
        0x1ded0dbced86364buL,
        0x02638a9fb93eb860uL,
        0x1de4c142bbcdb51auL,
        0x045bedfde3fbf9f1uL,
        0x1ddda64a6bca7acfuL,
        0x0263f4eee0ab230duL,
        0x1dd52ceab3daa53buL,
        0x00dc8426c9c266d4uL,
        0x1dce3e31f45a0a96uL,
        0x01645ef458066425uL,
        0x1dc5980ea6ad6692uL,
        0x005d19d38acfc932uL,
        0x1dbed549e6504cf2uL,
        0x00e4c893d1bef1feuL,
        0x1db60290f4619f97uL,
        0x045daedbd083bb8euL,
        0x1daf6b681cab013auL,
        0x02e531b0925a021euL,
        0x1da66c53a6323b06uL,
        0x025e4316b16614afuL,
        0x1da00031007ac3e3uL,
        0x02d59a2d7cbb3c39uL,
        0x1d96d5387be7adf5uL,
        0x025ed65ac2de0264uL,
        0x1d904a064f4bdd37uL,
        0x04d601ed1ee8e719uL,
        0x1d873d20f9b5e73buL,
        0x025f687e2b942e41uL,
        0x1d80931e5b5e6c42uL,
        0x03d668d1bf455ad8uL,
        0x1d77a3ee7681856euL,
        0x045ff956b675583buL,
        0x1d70db636a632668uL,
        0x0256cebd6a35f863uL,
        0x1d6809822a836e1euL,
        0x02e0445cf3250898uL,
        0x1d6122bfb19eafe6uL,
        0x05d73392002f5fc2uL,
        0x1d586dbd3e416492uL,
        0x02608b3e84ebc2b9uL,
        0x1d51691d609b1ec8uL,
        0x04d79731441e1e21uL,
        0x1d48d080d9d1c96cuL,
        0x0360d13aa83e4b01uL,
        0x1d41ae66ac0b0b6auL,
        0x0157f97cea22928buL,
        0x1d3931ae34603f62uL,
        0x00e1163bef9eebc1uL,
        0x1d31f285d8d6c816uL,
        0x04d85a56a6965552uL,
        0x1d299126a3e88ca4uL,
        0x03e15a2cf3193875uL,
        0x1d223565474c154euL,
        0x0258b9a03d510324uL,
        0x1d19eecbad1cb518uL,
        0x02e19cf85b21a11fuL,
        0x1d1276ef7e686adcuL,
        0x0459173b9121e9f7uL,
        0x1d0a4a7f136af77duL,
        0x0361de88eb969b39uL,
        0x1d02b70f3735b79euL,
        0x04d9730ab373bc61uL,
        0x1cfaa422e918100duL,
        0x00e21ec98edb9593uL,
        0x1cf2f5af68314ac2uL,
        0x0259cceff40f1fb1uL,
        0x1ceafb999f61e5d4uL,
        0x01625da56105b758uL,
        0x1ce332bb50b471fauL,
        0x03da24cdf0f0a2e7uL,
        0x1cdb50c6169e961buL,
        0x00629b07bb123c75uL,
        0x1cd36e1e845638bbuL,
        0x00da7a87a6267113uL,
        0x1ccba38bae4baa66uL,
        0x0262d6dc3e1e1b47uL,
        0x1cc3a7c4f63d9d53uL,
        0x00dace007da9e0c8uL,
        0x1cbbf3ce55012ad0uL,
        0x0263110ede9680ceuL,
        0x1cb3df9b045b81fbuL,
        0x045b1f1c5f28dcc9uL,
        0x1cac4172983c2f7duL,
        0x0263498bef599a58uL,
        0x1ca4158d828399aduL,
        0x03db6dbfbfb30836uL,
        0x1c9c8c5db3f49156uL,
        0x036380402cbf1542uL,
        0x1c944989c55b9311uL,
        0x03dbb9cfb13e7262uL,
        0x1c8cd475a1f163eeuL,
        0x00e3b518c77fb7d2uL,
        0x1c847b7dad17cf30uL,
        0x03dc0331f1f7ac71uL,
        0x1c7d19a128cff8a4uL,
        0x0163e8036f737914uL,
        0x1c74ab57affd05a9uL,
        0x00dc49ccfb511d2cuL,
        0x1c6d5bc7eab14dfbuL,
        0x01e418ee5e1d890euL,
        0x1c64d906e49e5535uL,
        0x005c8d8810c585d4uL,
        0x1c5d9ad27381fd3duL,
        0x00e447c860fdcf2cuL,
        0x1c55047b0bcf6526uL,
        0x03dcce4b4e41cdcauL,
        0x1c4dd6aa46d0f45cuL,
        0x01e47480e39f8181uL,
        0x1c452da49a426b15uL,
        0x035d0bffb62a59f5uL,
        0x1c3e0f39ed2991f9uL,
        0x00649f07f95c9d66uL,
        0x1c355474c1ca1f2auL,
        0x035d468f3ef07049uL,
        0x1c2e446d00e60d84uL,
        0x0164c74e66ce3841uL,
        0x1c2578dd7a37e92auL,
        0x03dd7de4e02c6f6fuL,
        0x1c1e76303a6f7572uL,
        0x01e4ed45aae1d60cuL,
        0x1c159ad189ced844uL,
        0x03ddb1ec9f31f5e1uL,
        0x1c0ea4717be0f8c8uL,
        0x006510e0078c325euL,
        0x1c05ba448d444792uL,
        0x00dde2939b1372f7uL,
        0x1bfecf1fdc04a7dauL,
        0x036532108a122ff3uL,
        0x1bf5d72aff4768d9uL,
        0x02de0fc8180b06b8uL,
        0x1beef62bb0a0594auL,
        0x016550cb12e0f1dbuL,
        0x1be5f17a3f894e1cuL,
        0x045e39798a3f0a89uL,
        0x1bdf19869809eb89uL,
        0x03656d045cee7811uL,
        0x1bd60928993f7077uL,
        0x015e5f989fd91caduL,
        0x1bcf392381fab055uL,
        0x026586b2049c7737uL,
        0x1bc61e2d491b1f67uL,
        0x035e82174a67122fuL,
        0x1bbf54f6b79a6d5euL,
        0x02e59dca8e17880fuL,
        0x1bb6308082b0b65cuL,
        0x00dea0e8c77dc629uL,
        0x1baf6cf5e2bb03dcuL,
        0x0065b2456b2d3672uL,
        0x1ba6401b7549eebauL,
        0x03debc01a8965943uL,
        0x1b9f8118143e7eafuL,
        0x01e5c41b0093e8e9uL,
        0x1b964cf8501f223buL,
        0x00ded357da1f18bauL,
        0x1b8f9155c9a1fbd0uL,
        0x02e5d344aaa010f1uL,
        0x1b86571245f3d399uL,
        0x03dee6e2a9b9efd0uL,
        0x1b7f9da8f1a8a0cbuL,
        0x01e5dfbcc1628fd2uL,
        0x1b765e6590135a00uL,
        0x01def69acba2f951uL,
        0x1b6fa60cf0228aacuL,
        0x0365e97e9c2cbc7fuL,
        0x1b6662ef70ab154auL,
        0x025f027a5f3a7f56uL,
        0x1b5faa7ea0cc6ecauL,
        0x01e5f0869476fb64uL,
        0x1b5664ae34801e0euL,
        0x025f0a7cf2ae7563uL,
        0x1b4faafc59456a8buL,
        0x01e5f4d2082760f5uL,
        0x1b4663a133fef350uL,
        0x025f0e9f85c03b41uL,
        0x1b3fa785ea194bf1uL,
        0x01e5f65f5b366281uL,
        0x1b365fc8d3a43882uL,
        0x01df0ee08ba43cd5uL,
        0x1b2fa01c9ede6a16uL,
        0x0165f52df8b025d3uL,
        0x1b26592683be2829uL,
        0x015f0b3febf9cbcduL,
        0x1b1f94c33d66f35auL,
        0x02e5f13e53118eaauL,
        0x1b164fbcbf86f1abuL,
        0x025f03bf02da5a7auL,
        0x1b0f857e040665a0uL,
        0x0165ea91e400b8afuL,
        0x1b06438f0b98caafuL,
        0x02def860a0000a7auL,
        0x1aff7252a6ecb2bauL,
        0x02e5e12b2b611c72uL,
        0x1af634a1f3bd0d7duL,
        0x035ee92905044d53uL,
        0x1aef5b484c995f72uL,
        0x0165d50dadc42d9duL,
        0x1ae622fb08184d55uL,
        0x03ded61de2b81fc4uL,
        0x1adf40678969b4f4uL,
        0x00e5c63df237cf4duL,
        0x1ad60ea0d9b5d710uL,
        0x035ebf4655983167uL,
        0x1acf21ba5a45e2aeuL,
        0x0265b4c17f7488b1uL,
        0x1ac5f79af6759efcuL,
        0x035ea4aae160108auL,
        0x1abeff4c1e71b057uL,
        0x0165a09ed86def16uL,
        0x1ab5ddf1e460242cuL,
        0x025e86556bc034feuL,
        0x1aaed92990861c72uL,
        0x01e589dd784842f0uL,
        0x1aa5c1af1c6454beuL,
        0x015e6451363b8311uL,
        0x1a9eaf60be99fa58uL,
        0x02e57085cdb6c23euL,
        0x1a95a2dd0483fd75uL,
        0x03de3eaad7319948uL,
        0x1a8e820101a05296uL,
        0x016554a135c6b3d2uL,
        0x1a858186e973c8cauL,
        0x03de1570321beee3uL,
        0x1a7e511af403f0e1uL,
        0x01653639f61bab8buL,
        0x1a755db8f7b445c5uL,
        0x03dde8b06f0475d8uL,
        0x1a6e1cc067882b18uL,
        0x0265155b36a1ff17uL,
        0x1a6537803429dd3cuL,
        0x035db87bf13d1856uL,
        0x1a5de5045a77840euL,
        0x0264f210fabcd4feuL,
        0x1a550eea743a03b0uL,
        0x025d84e44d6006fduL,
        0x1a4da9faec295ac0uL,
        0x0364cc6819f5a3a9uL,
        0x1a44e406557456e3uL,
        0x01dd4dfc3ea1615fuL,
        0x1a3d6bb950e85a75uL,
        0x01e4a46e38335bf7uL,
        0x1a34b6e334ceafc2uL,
        0x045d13d79b7b4d75uL,
        0x1a2d2a55c543d97auL,
        0x01e47a31bd7fd98auL,
        0x1a248791257b832duL,
        0x04dcd68b49be13bduL,
        0x1a1ce5e780d6c293uL,
        0x02644dc1cd628aecuL,
        0x1a145620e7623618uL,
        0x035c962d320e4c77uL,
        0x1a0c9e86a88f07ffuL,
        0x01641f2e3dd79383uL,
        0x1a0422a3dd414b5euL,
        0x025c52d432db963cuL,
        0x19fc544c4080f625uL,
        0x0363ee878deaf1c1uL,
        0x19f3ed2c02828af4uL,
        0x045c0c9812daaed1uL,
        0x19ec07521d52071euL,
        0x0163bbdedbff7430uL,
        0x19e3b5cbe0c97301uL,
        0x045bc391730e1bf4uL,
        0x19dbb7b2d547171auL,
        0x00e38745dbc97fd1uL,
        0x19d37c9685446b6auL,
        0x04db77d9c068db21uL,
        0x19cb6589b1020c3euL,
        0x01e350cecc05d9cfuL,
        0x19c3419f75c953bcuL,
        0x025b298b2516cc35uL,
        0x19bb10f29bfb2a68uL,
        0x0163188c6bf4cd49uL,
        0x19b304faa5c619afuL,
        0x01dad8c07976bbc0uL,
        0x19aaba0a14c264cbuL,
        0x0362de91f0a22435uL,
        0x19a2c6bc6b0e1423uL,
        0x045a859534d21642uL,
        0x199a60ed1d150c44uL,
        0x00e2a2f2fa027fc3uL,
        0x199286f9728ce320uL,
        0x035a30255dde65beuL,
        0x198a05b929d439aauL,
        0x036265c387eea954uL,
        0x198245c6b4e79163uL,
        0x0259d88d7b14c6d3uL,
        0x1979a88c12e847c1uL,
        0x02622717ef05792fuL,
        0x197203396b14a770uL,
        0x01d97eea82eb8229uL,
        0x19694984031d9858uL,
        0x0161e704cd7ceb7cuL,
        0x1961bf6702f3caf4uL,
        0x00d92359cbfdea74uL,
        0x1958e8bf6806bcaauL,
        0x0261a59effeaeef1uL,
        0x19517a6513ed67f9uL,
        0x0458c5f8fd2e86f6uL,
        0x1948865ce1efe9b5uL,
        0x02e162fb960e6361uL,
        0x1941344953a2bc16uL,
        0x025866e5fdcf6e5cuL,
        0x1938227b33ef66f4uL,
        0x01611f2fc7a0a0a9uL,
        0x1930ed298ab66e97uL,
        0x0158063ee5dc8676uL,
        0x1927bd39341e60d1uL,
        0x02e0da50e937b941uL,
        0x1920a51b89b5ac37uL,
        0x04d7a421ee53231buL,
        0x191756b5bc0538cfuL,
        0x00e0947461417eb2uL,
        0x19105c351e298146uL,
        0x055740ad61b23997uL,
        0x1906ef0f9946142euL,
        0x00e04daf9d1f19d0uL,
        0x1900128c07d7eac9uL,
        0x0056dbff8cae0f32uL,
        0x18f686657e900798uL,
        0x03e006180668cd93uL,
        0x18ef906bdc779cfcuL,
        0x01667636af21f0cbuL,
        0x18e61cd5f4e4d33cuL,
        0x00df7b85f0c272bbuL,
        0x18defa90ac757636uL,
        0x01e60f70ed4a200euL,
        0x18d5b27f4d3aafafuL,
        0x015ee98b6b3e4f34uL,
        0x18ce63b1303dfbfbuL,
        0x0165a7cc414fb8aauL,
        0x18c5477f92833194uL,
        0x045e566abbe94f87uL,
        0x18bdcbf7abb88523uL,
        0x02653f666d2fde17uL,
        0x18b4dbf47c1fc89fuL,
        0x045dc24dc933bf6duL,
        0x18ad338de3492427uL,
        0x01e4d65ced070949uL,
        0x18a46ffb60cbd760uL,
        0x005d2d5e0d435050uL,
        0x189c9a9d09a6515fuL,
        0x00646ccce9c8cdf5uL,
        0x189403b12a03d499uL,
        0x005c97c4837b573euL,
        0x188c014dae645fc2uL,
        0x02e402d32c6be96duL,
        0x1883973247f05595uL,
        0x02dc01a996aebdb3uL,
        0x187b67c7ad400b85uL,
        0x0363988c1191e211uL,
        0x18732a9aa5db4bb3uL,
        0x025b6b3510058b7auL,
        0x186ace321e309c7buL,
        0x00e32e137db0ef23uL,
        0x1862be059f3526f6uL,
        0x03dad48e069f2207uL,
        0x185a34b346493cc3uL,
        0x0062c384d1c64d5buL,
        0x1852518df52ef491uL,
        0x035a3ddacff96f65uL,
        0x18499b70897047dcuL,
        0x016258fae0968e74uL,
        0x1841e54dc4edf3a3uL,
        0x00d9a740f1248851uL,
        0x1839028e5cf277c7uL,
        0x00e1ee8fe480d92cuL,
        0x1831795e7e5c7cc9uL,
        0x045910e510c93fe1uL,
        0x18286a303af6f698uL,
        0x02e1845d75e974c6uL,
        0x18210dd8db9b7b1fuL,
        0x03d87aeaea087811uL,
        0x1817d27896d87b8duL,
        0x03611a7c823f5ff5uL,
        0x1810a2d4d9171799uL,
        0x03d7e57540380a90uL,
        0x18073b88d266bc5auL,
        0x01e0b10543a01766uL,
        0x18003869ae409b27uL,
        0x015750a5d3814d59uL,
        0x17f6a58134129f17uL,
        0x02e0480f391c14fcuL,
        0x17ef9d5b8ddde220uL,
        0x02e6bc9d56645be6uL,
        0x17e61080de06bfb0uL,
        0x01dfbf623f3bedbauL,
        0x17decb6d7acd34f7uL,
        0x00e6297b642274f2uL,
        0x17d57ca5c62d05dcuL,
        0x035ef001d6eb49dfuL,
        0x17cdfb32aa129cc5uL,
        0x02e5975e7810e700uL,
        0x17c4ea0caf213788uL,
        0x035e222785106b16uL,
        0x17bd2cd2eb59de4buL,
        0x01e50663e5d53392uL,
        0x17b458d1220fa79duL,
        0x005d55fbee497e00uL,
        0x17ac60744f31e197uL,
        0x026476a7d28a437buL,
        0x17a3c90d697e5b5cuL,
        0x045c8ba606fb6833uL,
        0x179b963b20518321uL,
        0x00e3e8452ecdbe84uL,
        0x17933ada8cfe418fuL,
        0x025bc34b0b8bbc60uL,
        0x178ace49de2283aeuL,
        0x00635b55b1b3d652uL,
        0x1782ae504dc15f24uL,
        0x005afd0e79df00ebuL,
        0x177a08c1388db34fuL,
        0x01e2cff1d49f192cuL,
        0x1772238524122580uL,
        0x01da39120c175c51uL,
        0x176945c00d028181uL,
        0x02624630cff92d39uL,
        0x17619a8e3da77fbduL,
        0x04d97775b48ec1aauL,
        0x1758856364b336c5uL,
        0x0161be2898c8a8a4uL,
        0x1751137f7cd08641uL,
        0x04d8b8579b06ca2cuL,
        0x1747c7c673fe436duL,
        0x02e137eddf1f97aeuL,
        0x17408e6b787233b9uL,
        0x04d7fbd41b078795uL,
        0x17370d029afc4471uL,
        0x03e0b3940d5da6fcuL,
        0x17300b637cd0ec0auL,
        0x04574205c365c73euL,
        0x1726552f6729a258uL,
        0x03e0312d48405757uL,
        0x171f14ef1a3e4ac1uL,
        0x01e68b0556e87723uL,
        0x1715a06296220022uL,
        0x02df6194df7630e5uL,
        0x170e176ccb941b52uL,
        0x02e5d6e9ce0425a7uL,
        0x1704eeb0196310ccuL,
        0x035e64f64121563euL,
        0x16fd1e5afef936dauL,
        0x00e525c859a2ea9auL,
        0x16f4402a1b0bd9dfuL,
        0x01dd6c9b6d4d6fc5uL,
        0x16ec29d225a230e2uL,
        0x02e477b466ee6cc1uL,
        0x16e394e1038ce88euL,
        0x015c789ea0183d02uL,
        0x16db39e83951bdaauL,
        0x01e3ccbfa4112a58uL,
        0x16d2ece3803d8d68uL,
        0x005b8917a154498buL,
        0x16ca4eb0c6436cf3uL,
        0x036324fa05e3adc4uL,
        0x16c2483e8ac9d060uL,
        0x035a9e1bcd30af1fuL,
        0x16b9683cf6400111uL,
        0x03628071ce79e917uL,
        0x16b1a6fd716c7c18uL,
        0x01d9b7be1e1550cbuL,
        0x16a8869b9cc95344uL,
        0x0261df33948493fauL,
        0x16a10929dfe85b78uL,
        0x0358d60f37a227b9uL,
        0x1697a9d9444b613euL,
        0x0161414a4b7a1729uL,
        0x16906ecbe9338feauL,
        0x0357f91d72bfd333uL,
        0x1686d2003c3fdf54uL,
        0x0160a6bf4c7a4f95uL,
        0x167fafd4238f8063uL,
        0x00e720f4eaaf4bbbuL,
        0x1675ff18a8317f09uL,
        0x04600f9a5fe04069uL,
        0x166e8912b5139031uL,
        0x00e64d9f8b065b73uL,
        0x166531288f8c01c7uL,
        0x00def7c38ee94e41uL,
        0x165d695a98770e4auL,
        0x02657f251e86550euL,
        0x16546833ee262b10uL,
        0x00ddd73492689d20uL,
        0x164c50b006d4e014uL,
        0x0264b58b5eba6cc7uL,
        0x1643a43cc572b3d3uL,
        0x025cbd8e7539eac7uL,
        0x163b3f14799b1616uL,
        0x0163f0d6044b145duL,
        0x1632e5432e458096uL,
        0x045baad518e7426euL,
        0x162a3486c40b74f0uL,
        0x02633106d7f3cac9uL,
        0x16222b456b1a8db6uL,
        0x04da9f09adee91e3uL,
        0x161931032d667260uL,
        0x0362761dc408f1efuL,
        0x1611763ffacc46abuL,
        0x03d99a2acce5bd7fuL,
        0x160834838ba6fe3duL,
        0x0161c018e67b6eaeuL,
        0x1600c62daba74e7cuL,
        0x02589c349043d67euL,
        0x15f73eff5eb5eca5uL,
        0x01610ef4a3481a29uL,
        0x15f01b07aeca1f41uL,
        0x05d7a520aeb63faeuL,
        0x15e6506bebfc67bduL,
        0x016062abb7415c63uL,
        0x15dee98b577ea7c9uL,
        0x0266b4e695e9099fuL,
        0x15d568bc5a3d72eeuL,
        0x01df766e96435041uL,
        0x15cda6bba883d229uL,
        0x0265cb7b85aa6067uL,
        0x15c487e1cd9f3e43uL,
        0x025e311e0dabf963uL,
        0x15bc6d89f0368fc1uL,
        0x01e4e8d2ab5187d6uL,
        0x15b3adcb83cdccc3uL,
        0x01dcf55249e0172auL,
        0x15ab3ddd3216f86euL,
        0x01640cdd3d52967cuL,
        0x15a2da66f0214306uL,
        0x025bc2f50c60488euL,
        0x159a1799fd5925f4uL,
        0x0063378a96e8e29auL,
        0x15920d9fd7b31257uL,
        0x00da99ed8a2f2e6buL,
        0x1588faa294857a38uL,
        0x02e268c853c2e48duL,
        0x158147606d4e1ee2uL,
        0x04d97a2092e9b19duL,
        0x1577e6d714d6fce6uL,
        0x03e1a0826b9b2f1euL,
        0x157087916d26f37cuL,
        0x01d86370b7b69b46uL,
        0x1566dc159d3dbce3uL,
        0x0160dea34dab05c3uL,
        0x155f9c3470942341uL,
        0x00e755be71f29feauL,
        0x1555da3a74ec8bc7uL,
        0x01e02313fbe40a01uL,
        0x154e35c1df5edf06uL,
        0x02e650e8497f58cduL,
        0x1544e120315adc05uL,
        0x025edb784bbee452uL,
        0x153cdb951dc67cbeuL,
        0x036554cafa9d0c34uL,
        0x1533f09fdba5037duL,
        0x02dd7d0486e476ccuL,
        0x152b8d760c6a3fa9uL,
        0x02e461419b3892c2uL,
        0x152308911536a23duL,
        0x00dc2a975dad9be0uL,
        0x151a4b2aa8c000cauL,
        0x01e37625bf981bdbuL,
        0x151228ca3bac6e06uL,
        0x035ae3f97cbb25ceuL,
        0x150914773f3bbbacuL,
        0x00e2934f9e530baduL,
        0x150151208bdc254euL,
        0x0259a8f1bb2e0d78uL,
        0x14f7e91e9c37a26auL,
        0x02e1b8963382a860uL,
        0x14f0816843f2edd7uL,
        0x035879454bd5bf1auL,
        0x14e6c8e23b87885fuL,
        0x01e0e5cf631ac83buL,
        0x14df72e98937c4f8uL,
        0x016754b7ed21d736uL,
        0x14d5b38276a48e9fuL,
        0x02601ad01a5b2dd0uL,
        0x14cdf23162441e8buL,
        0x01663b0c17c2af00uL,
        0x14c4a8beb16012ecuL,
        0x03deaed8e09770eduL,
        0x14bc804c1d0522ebuL,
        0x01e52c032be62aabuL,
        0x14b3a855850eeeeauL,
        0x025d36ef8a6e08fauL,
        0x14ab1cdcc2ca0213uL,
        0x02e4275d9d00481duL,
        0x14a2b204ea20186euL,
        0x015bcd89c2310d59uL,
        0x1499c78595e362cduL,
        0x02632cdb1c10f0eeuL,
        0x1491c58a6013aaeduL,
        0x02da724c21e93002uL,
        0x14887fe848fd6bffuL,
        0x01623c3ac05a8c19uL,
        0x1480e2a313c94bb4uL,
        0x045924da86249080uL,
        0x147745a6341bd9d2uL,
        0x03e1553b2e7eba16uL,
        0x1470090c041eb55fuL,
        0x0057e4d844204d5fuL,
        0x14661860872f36c7uL,
        0x01e0779abdf88654uL,
        0x145e710449b20327uL,
        0x00e6b1e85d9cfdc3uL,
        0x1454f7b87a3ccd22uL,
        0x00df462f39da55f5uL,
        0x144ce184ffaa0274uL,
        0x02e58badb2559681uL,
        0x1443e34f7b15484duL,
        0x00ddaedfe49c8a9fuL,
        0x143b6314a8f93440uL,
        0x02e471cb2f12adecuL,
        0x1432dac758984610uL,
        0x005c28c3fc94131buL,
        0x1429f52e6b0168fbuL,
        0x006363e3fa566830uL,
        0x1421ddc26b854421uL,
        0x04dab358720f461fuL,
        0x1418974e49b18480uL,
        0x02e2619b9e9f9276uL,
        0x1410ebe3bcdc6652uL,
        0x00594e1adf5ef17auL,
        0x140748f15c14a98fuL,
        0x03e16a96324493c1uL,
        0x140004cf29d383aeuL,
        0x04d7f889bf8109c7uL,
        0x13f60995fd7916b3uL,
        0x03607e787ce8decbuL,
        0x13ee50530acb7a2buL,
        0x0066b224a16aa4e0uL,
        0x13e4d8bbfb38c980uL,
        0x005f39d03522ee6euL,
        0x13dcab316f0b29dduL,
        0x02657a6c57f8fed2uL,
        0x13d3b5e4bf3051bauL,
        0x045d8b1738bdcb74uL,
        0x13cb1987b3f62cd2uL,
        0x016450e32693ba8duL,
        0x13c2a09376f26715uL,
        0x035bf0154de94403uL,
        0x13b99aa6a5f22416uL,
        0x00e3350cea8cd61auL,
        0x13b1984d37c8d150uL,
        0x04da681c1d2f0b94uL,
        0x13a82de1daeb9c47uL,
        0x00e2266f414ce57buL,
        0x13a09c991f950457uL,
        0x0258f27fe21c9591uL,
        0x1396d28fdea98719uL,
        0x02e12491ab5c17d9uL,
        0x138f5a00e548f085uL,
        0x01678e979aa0c9beuL,
        0x1385880a5ae03597uL,
        0x03602efdac5a4ff4uL,
        0x137d921d6d1c821auL,
        0x01e63bbd32217718uL,
        0x13744dae3b23367buL,
        0x025e8a7dcff4677cuL,
        0x136be0a394617720uL,
        0x02e4f94da865b2a3uL,
        0x136322dbccd73cabuL,
        0x005ccdc67829105buL,
        0x135a44b3f5ce9c8buL,
        0x0163c6a934743c05uL,
        0x135206f6db46b930uL,
        0x015b26f5afd4ebc9uL,
        0x1348bd742e227a37uL,
        0x0262a3336386b4d7uL,
        0x1340f966c7fd2396uL,
        0x00599530a15ce61auL,
        0x13374a0efc06d36euL,
        0x01e18e533433f227uL,
        0x132ff32d3f1c0a49uL,
        0x016817a166d90dbduL,
        0x1325e9b45aff1bdfuL,
        0x046087732df4f3abuL,
        0x131e0dea55db81c4uL,
        0x0066ad7728d6db01uL,
        0x13149b9999981d6buL,
        0x03df1c02ea5235f3uL,
        0x130c41e9fb058b1euL,
        0x016555e63841a093uL,
        0x13035ef96b0fe654uL,
        0x03dd42dfb77e321euL,
        0x12fa8e19002cb47fuL,
        0x01e4102823a6a0a2uL,
        0x12f23313f4adb099uL,
        0x025b8267dd51660duL,
        0x12e8f16bf19917abuL,
        0x02e2db7bc80b123euL,
        0x12e1172ed701cd40uL,
        0x0259d98e007ff597uL,
        0x12d76adf2095d808uL,
        0x0161b7255d8af1ceuL,
        0x12d00a953345bce3uL,
        0x03d8474c5f89cf1fuL,
        0x12c5f976a86ba7a3uL,
        0x00e0a26e7ff7c8a0uL,
        0x12be192f5a290a0duL,
        0x01e6caa4dc34bcc6uL,
        0x12b49c3e6e576cf7uL,
        0x035f394c675d5da1uL,
        0x12ac3918d16606aeuL,
        0x01e562a0ffd36fefuL,
        0x12a3524a1ccb90ceuL,
        0x025d4a41cdb95576uL,
        0x129a739e0c3f00b2uL,
        0x03640e51faa74ee4uL,
        0x12921ab51a49a640uL,
        0x025b7670ded07be7uL,
        0x1288c781323e2b8auL,
        0x0362ccd09eaa3410uL,
        0x1280f4a27c210b83uL,
        0x0259bc980b6cd88buL,
        0x1277338f3cfd4b18uL,
        0x00e19d3d560c7458uL,
        0x126fbe79eabbab8buL,
        0x00e81b807901b2dduL,
        0x1265b69fdd784131uL,
        0x01e07ec015b26bbfuL,
        0x125db36d8463b3e0uL,
        0x02e691fdebe382beuL,
        0x12544f955c9776f6uL,
        0x005ee11097f70374uL,
        0x124bc693203fe92buL,
        0x02e51eeeac7320beuL,
        0x1242fd5c7756dd24uL,
        0x01dce39998362bf9uL,
        0x1239f66cc65fb2cauL,
        0x0263c13b67a17ff2uL,
        0x1231beec36eb8501uL,
        0x035b03976c943068uL,
        0x1228418af0dd65ecuL,
        0x02e277d70b2ebc6fuL,
        0x12209345c546e7ccuL,
        0x04593f94ba2c6b6auL,
        0x1216a68c4bfd764auL,
        0x03e141be9e049453uL,
        0x120ef2e87ca7b716uL,
        0x0267962a50231832uL,
        0x1205241d71eb6e19uL,
        0x02601df915097b64uL,
        0x11fce118fc8beee9uL,
        0x02e605fee84767f0uL,
        0x11f3b8f8a28fd848uL,
        0x015e172e498cd2fcuL,
        0x11eaef59daa19c92uL,
        0x02648dc6e3757e71uL,
        0x11e263e577f574cfuL,
        0x045c1366206ca036uL,
        0x11d91bfa9231de5cuL,
        0x00632c440230ef3auL,
        0x11d123b897af1af4uL,
        0x00da2ee0ea25a216uL,
        0x11c7655cd85a2773uL,
        0x0261e04519eb8f87uL,
        0x11bfeea6c3554148uL,
        0x026867f82bdccb8fuL,
        0x11b5c9f427a491a4uL,
        0x0260a8a5c7678dffuL,
        0x11adbb4739afff2duL,
        0x02e6bd1744d1513euL,
        0x11a4484548d479a3uL,
        0x025f089c3d3d8b6fuL,
        0x119bab46440d8e4auL,
        0x02e52cbafb8bc99fuL,
        0x1192dee5d96e696duL,
        0x02dce464b1286c0duL,
        0x1189bcaf0aad775buL,
        0x0263b571085ef9dbuL,
        0x11818c7bd07b007euL,
        0x045ae2a4fedee59cuL,
        0x1177eda37d26ae66uL,
        0x016255d79dbe3905uL,
        0x11704fbd01fd3b99uL,
        0x0559017432798e26uL,
        0x11663c5ba199716fuL,
        0x00e10c9ceee61d28uL,
        0x115e4edd431a7a40uL,
        0x01e73effa34f57abuL,
        0x1154a724e2f6eadeuL,
        0x00dfb0fd6a99ec28uL,
        0x114c24c9890314ccuL,
        0x0265998a4600495buL,
        0x11432c615eef6a3duL,
        0x025d70936a92f04auL,
        0x113a1f03c81340fduL,
        0x01640f6bfdad1f14uL,
        0x1131ca87340e1c38uL,
        0x045b55b284add8c1uL,
        0x11283b6cbf2ba29fuL,
        0x01629f10ece9036euL,
        0x1120801fd07f7284uL,
        0x01595e2d86ae92c8uL,
        0x111677ffffc31b92uL,
        0x01e146f8c6e8dc57uL,
        0x110e978e83ebd95duL,
        0x00e787f26e598ebbuL,
        0x1104d2d2f5dd4095uL,
        0x03e005b6216a17eauL,
        0x10fc58570e2f641cuL,
        0x01e5d10973fbab06uL,
        0x10f34a13f272cdf9uL,
        0x035db3db8f832a58uL,
        0x10ea4017c5ace0deuL,
        0x0064379416dfac63uL,
        0x10e1dc0938cfb931uL,
        0x045b84ac1ef46255uL,
        0x10d84c7064147f80uL,
        0x0262b9cc2c3d6738uL,
        0x10d087100f5e6429uL,
        0x02d97b6c5dc3637auL,
        0x10c67b20873fc995uL,
        0x01e15602f1227af8uL,
        0x10be9337a8979da0uL,
        0x006795cb2bb480b6uL,
        0x10b4ca0667456eb7uL,
        0x04600aa01fc8a73euL,
        0x10ac446a2ccade1buL,
        0x01e5d196927cdaccuL,
        0x10a3371d92c55c69uL,
        0x01ddac421184af19uL,
        0x109a1ef1650d3561uL,
        0x02642cba823b93cbuL,
        0x1091c07db1df4cf5uL,
        0x04db6e2f60b615c1uL,
        0x1088202debc2593cuL,
        0x00e2a53f94211ba9uL,
        0x108064595037ce7auL,
        0x04595853e0fd75aduL,
        0x107645a58ac6913buL,
        0x02613949d3b2fbd2uL,
        0x106e41f95cc492ceuL,
        0x016768213ee2ba9cuL,
        0x10648d0194e5b153uL,
        0x00dfce2f1e195a7auL,
        0x105be99935f38c41uL,
        0x02659b2d772c1b04uL,
        0x1052f40d4a5d2870uL,
        0x015d5a005ce1b15duL,
        0x1049bc8aa74c3804uL,
        0x02e3ef3138f8ae58uL,
        0x104178b448b82b15uL,
        0x045b12e626e3c8a1uL,
        0x1037b7f2dc7fa065uL,
        0x02e2620652c3102cuL,
        0x1030190106456395uL,
        0x0358f5ecffd9c995uL,
        0x1025d92194746ef1uL,
        0x03e0f1a62a97a48euL,
        0x101da636b2add639uL,
        0x02e7004d0a0dd3fcuL,
        0x10141d8f14e2d234uL,
        0x02df38508375a815uL,
        0x100b4a8e16df3a2euL,
        0x00e52f67f4a45dbduL,
        0x100282da2ee06e9fuL,
        0x005cbf8187da9700uL,
        0x0ff91bc4f0e82a0fuL,
        0x02e380c6fa6ddd1buL,
        0x0ff106c65473611buL,
        0x005a757e44dde4fbuL,
        0x0fe716ca73d3a1dbuL,
        0x03e1f218f165083cuL,
        0x0fdf4e737e667fe5uL,
        0x0268571975a9ba0cuL,
        0x0fd538bdbc88034fuL,
        0x026081306aee058buL,
        0x0fccc4774fe05a13uL,
        0x00e661571375ee31uL,
        0x0fc37eeb586702afuL,
        0x01de5803c9b677c0uL,
        0x0fba6be51e94d2c2uL,
        0x02e49169d29f057fuL,
        0x0fb1e6cae3cc5ce4uL,
        0x025be144165bfdaduL,
        0x0fa841452e30c6ecuL,
        0x00e2e4b0b7596d86uL,
        0x0fa06dfcc0330324uL,
        0x01599a8814f82396uL,
        0x0f964157d8dbcaa0uL,
        0x026158b4c1d7aa61uL,
        0x0f8e248fc3725278uL,
        0x00e7806fe5adc0deuL,
        0x0f84691284199248uL,
        0x015fd64d63539ac4uL,
        0x0f7ba32f675bcca0uL,
        0x03658fd2560c98e3uL,
        0x0f72b59cb5fcd06fuL,
        0x035d33b9c01b8858uL,
        0x0f6953f4278d9770uL,
        0x0363c5b9e7be019euL,
        0x0f61244d4a198783uL,
        0x01dac5a261b57bd2uL,
        0x0f57333ac721d352uL,
        0x03621f61f6e6a3a5uL,
        0x0f4f654f8b2c9938uL,
        0x0168883e334bf813uL,
        0x0f453d9d5f4e3889uL,
        0x01e09a33ffab8174uL,
        0x0f3cbcb3935e8706uL,
        0x02e678037d69a88auL,
        0x0f336fefd85e37f7uL,
        0x015e678a0474dd4duL,
        0x0f2a4a7147e53789uL,
        0x01e491a44a8cc267uL,
        0x0f21c73c8c2f3142uL,
        0x03dbd3a60953bab8uL,
        0x0f180a7df6e9e4abuL,
        0x0162d20af56e98e4uL,
        0x0f1040c111171b21uL,
        0x00d9748563f2a02cuL,
        0x0f05f9153468350cuL,
        0x02e13656dff66048uL,
        0x0efdb3d65827b6f1uL,
        0x0167463a2ae57157uL,
        0x0ef412b4a3b0b6bbuL,
        0x015f77b2a384d071uL,
        0x0eeb20abd232bd71uL,
        0x02e5451ae34b02aeuL,
        0x0ee25417f5fe18a9uL,
        0x045cc024fa52d21euL,
        0x0ed8c38db09c3d67uL,
        0x02636dbe645ba702uL,
        0x0ed0ba351c6b2c43uL,
        0x04da415d531b6e85uL,
        0x0ec69856de023170uL,
        0x01e1bcf7eeeba2f5uL,
        0x0ebe847157246bfcuL,
        0x0167f70703ac5558uL,
        0x0eb49b2d16422141uL,
        0x01e02fd377359b10uL,
        0x0eabd304de355d85uL,
        0x00e5dd1b0bb84b26uL,
        0x0ea2c87c2ff697dcuL,
        0x00dd87243e77ecaduL,
        0x0e995b4456f24a66uL,
        0x01e3efdb3b369292uL,
        0x0e911cf1a60f1d84uL,
        0x00daeb4dc01a4631uL,
        0x0e8718a9184a8678uL,
        0x00e22bcd99dbdb06uL,
        0x0e7f2af0be1fde49uL,
        0x01688766c06b0833uL,
        0x0e7507007917e3d8uL,
        0x02e08db80d427d79uL,
        0x0e6c5e695f15072buL,
        0x01e65709eb54bf5euL,
        0x0e632266540e08c1uL,
        0x02de253876b38aceuL,
        0x0e59cf012acb820auL,
        0x02645623a2f6a451uL,
        0x0e51673fda512b45uL,
        0x035b6f674d703273uL,
        0x0e4777d05328bd25uL,
        0x03e280eca736b4b1uL,
        0x0e3fa46d62b8e57cuL,
        0x01e8f4d804e3ad6fuL,
        0x0e35544c8bc23e1cuL,
        0x01e0d3e50a2eecdcuL,
        0x0e2cc068b1dc8ab1uL,
        0x0266b0c7763ce52buL,
        0x0e236042b9065710uL,
        0x005e979edc5b3767uL,
        0x0e1a1cbbab815b4cuL,
        0x01649ecd657d5dd6uL,
        0x0e1197d0fe71564buL,
        0x02dbcb59141dc715uL,
        0x0e07b41f3bcb1868uL,
        0x02e2bad65a82bb23uL,
        0x0dffeec24eca8005uL,
        0x02e93d6de18ac6bfuL,
        0x0df581b387627669uL,
        0x01e1011dd6dfecf6uL,
        0x0decf746ccaba031uL,
        0x01e6e8be31f2fe24uL,
        0x0de380f8b864e1abuL,
        0x035edc51c8649aaauL,
        0x0dda4312cc2f816auL,
        0x00e4c88f43732a10uL,
        0x0dd1adc83c96acceuL,
        0x02dbfd81ed74f1cduL,
        0x0dc7cc835281bbf3uL,
        0x00e2d883a292df3buL,
        0x0dc0044e6f2b903euL,
        0x04d95fde403b5724uL,
        0x0db58e66674c0f82uL,
        0x01611494966870b7uL,
        0x0dad0209514d613duL,
        0x0166fdef1ca550b3uL,
        0x0da383f2f4495aecuL,
        0x02def217eb67d36duL,
        0x0d9a41575f0363d5uL,
        0x0264d2aaa5b8e28auL,
        0x0d91a8c12a0cae90uL,
        0x045c04fcbf1fddd8uL,
        0x0d87c08d08f2ccbbuL,
        0x00e2d96cdd2a30b8uL,
        0x0d7ff186c5b90603uL,
        0x02e95b8ba50a2687uL,
        0x0d757a2b0b1c4c86uL,
        0x00610df03cd711e3uL,
        0x0d6ce07ef98af2aduL,
        0x01e6eff939f51c8fuL,
        0x0d636923c5eb270auL,
        0x025ed88d96607fb4uL,
        0x0d5a1791489717beuL,
        0x02e4bcf1445c1d61uL,
        0x0d5188d2c2d680a2uL,
        0x02dbe1a747b458c8uL,
        0x0d47907312c7e254uL,
        0x0362bd8dde16ba8auL,
        0x0d3fa9e995f4c413uL,
        0x02693089dc23e417uL,
        0x0d35455df149c7b4uL,
        0x0360ed4f34d6e965uL,
        0x0d2c93410e8142f7uL,
        0x01e6bf1c754a3325uL,
        0x0d233105a5b594f7uL,
        0x01de9027b1c5a4abuL,
        0x0d19c67f441e11b3uL,
        0x006487c687197597uL,
        0x0d114e8ebae7496euL,
        0x02db942323a72767uL,
        0x0d073d10c597b773uL,
        0x026285660efb3e9auL,
        0x0cff330b99c7f9e7uL,
        0x00e8df9d62fb9c5euL,
        0x0cf4f0ef77c81a6fuL,
        0x00e0b34677fe9486uL,
        0x0cec1baedb5f2e64uL,
        0x01e66c37bb05de1euL,
        0x0ce2dc9788ad9863uL,
        0x045e1a30436bcde5uL,
        0x0cd94f913add4907uL,
        0x00e4341c90c553e7uL,
        0x0cd0fafd2c40ba26uL,
        0x04db1dd0ffc5d04buL,
        0x0cc6c7df995241d1uL,
        0x00e231f4a6757469uL,
        0x0cbe8f062cc963cduL,
        0x01e86a35930ed5e1uL,
        0x0cb47e5cbff0d92euL,
        0x016060dd236f49a3uL,
        0x0cab7be34be4e18cuL,
        0x02e5f8c25cd122d7uL,
        0x0ca26d5559b935e6uL,
        0x045d78bca82e9f37uL,
        0x0c98b4dd6af9c05cuL,
        0x0363c36d15093021uL,
        0x0c908f94cfc79157uL,
        0x035a80c62c44a65buL,
        0x0c8632ec0e0d009buL,
        0x03e1c4b11ed6627auL,
        0x0c7dc0b5f2e40ea4uL,
        0x00e7d261cc2edf72uL,
        0x0c73efa480ea698buL,
        0x025fef096f5252f0uL,
        0x0c6ab6a5245de9e4uL,
        0x036566c107178d1fuL,
        0x0c61e52cde409267uL,
        0x01dcae9de8f00c0buL,
        0x0c57f910d0084829uL,
        0x006337ae444bd293uL,
        0x0c500e3012bd4170uL,
        0x0559bfbcfe9dc1e8uL,
        0x0c4580c66bfc7cf5uL,
        0x01e13f803c0631d9uL,
        0x0c3ccba595fe34b5uL,
        0x00671ac2109d33c9uL,
        0x0c3347383dcf4a9buL,
        0x025ef21caa7d80c3uL,
        0x0c29cf52785fcd1fuL,
        0x0064b8b6bbdb7a4fuL,
        0x0c21466f7a4ba4b2uL,
        0x04dbbf4bcf8ca0c3uL,
        0x0c171f5b701cb666uL,
        0x0362934441fdae8buL,
        0x0c0ef1fef5338f87uL,
        0x0168de00a5d4cff3uL,
        0x0c04b46ffc2e70ccuL,
        0x00e0a4a61359d63auL,
        0x0bfbb3f3e667d5e5uL,
        0x01e64673b39bdd54uL,
        0x0bf287ea78b8278fuL,
        0x01ddcf3acd0cc1f4uL,
        0x0be8c9c8347a2862uL,
        0x0263f1926f0c2aa4uL,
        0x0be093c166d47d8fuL,
        0x03daaecb94ca24e1uL,
        0x0bd62b5957e6b821uL,
        0x02e1d8efbbc88d6cuL,
        0x0bcda4f3c5b8c56fuL,
        0x00e7df554174928cuL,
        0x0bc3d1457a1afdaeuL,
        0x01dfed6b4a9440a8uL,
        0x0bba7e3665ffae25uL,
        0x016558fae0fed7aauL,
        0x0bb1b4da97b89112uL,
        0x03dc8b307e047613uL,
        0x0ba7aa46b2ec675cuL,
        0x01e3149a005e5984uL,
        0x0b9fa00e080e5360uL,
        0x00e9819329634547uL,
        0x0b9520f92dcad4a2uL,
        0x01e10bba52994e8euL,
        0x0b8c3a9666328faauL,
        0x00e6c7dd2d93c0f9uL,
        0x0b82dae795ce73b5uL,
        0x02de70fd5d6d806duL,
        0x0b792f5963d343ceuL,
        0x02645629dffe1fa7uL,
        0x0b70d15f439254beuL,
        0x005b2b2e959996b0uL,
        0x0b6675546ac2c966uL,
        0x0262255364dfcfd7uL,
        0x0b5dfca1ff236f01uL,
        0x01e83c6a3841fccauL,
        0x0b54046155930cfauL,
        0x04602ee197efc99duL,
        0x0b4ab8846c89a496uL,
        0x01659bfc8bdbfffeuL,
        0x0b41d5226b496f7euL,
        0x005cd9f4c9733040uL,
        0x0b37cc7edd2bedd1uL,
        0x01e3420703d360eauL,
        0x0b2fc1e021531b11uL,
        0x0069b4a6e4580455uL,
        0x0b252f9fd29afa7auL,
        0x02e1276cde31355euL,
        0x0b1c439018f9e7afuL,
        0x02e6e44a0da72deduL,
        0x0b12d9d4a3bfacf9uL,
        0x03de8b82d35e9882uL,
        0x0b09247c7d6b7108uL,
        0x02e4603c1a2de688uL,
        0x0b00c3d4d5746631uL,
        0x045b2e6fa531d555uL,
        0x0af65add59367765uL,
        0x01e220b241172407uL,
        0x0aedce1e8301e6eeuL,
        0x01e82d28ae825549uL,
        0x0ae3dde18cb97a8duL,
        0x01601ea51e3f541cuL,
        0x0ada7b31ccb0b2f3uL,
        0x02e57e3d8e31e749uL,
        0x0ad1a59798dd7aa1uL,
        0x035ca77ce984ce61uL,
        0x0ac7843a7981f8e3uL,
        0x01e3192c63185ef2uL,
        0x0abf55b0f3ffe462uL,
        0x026974911a73b1a7uL,
        0x0ab4df9fe655b0fbuL,
        0x0160f64b579273f6uL,
        0x0aabce68ce6bcfecuL,
        0x02e69a3e1bad13dauL,
        0x0aa284bfe1cdea23uL,
        0x035e1d6859c11527uL,
        0x0a98a9c29acbf47duL,
        0x01640f425a16dca3uL,
        0x0a906bd70b72892buL,
        0x025ab8633790b1e2uL,
        0x0a85dd55c1a48477uL,
        0x00e1cb4a43b9229fuL,
        0x0a7d1bd6b173b9f1uL,
        0x02e7b25cc6523c3buL,
        0x0a735fc8451ff49duL,
        0x03df8db2dc70232buL,
        0x0a69c9712232f548uL,
        0x00e5014bc06e7f91uL,
        0x0a6128b47439dcd4uL,
        0x035bf66ba3b9066cuL,
        0x0a56d53d2be0a0b5uL,
        0x02e29c2c1dc958dbuL,
        0x0a4e6122171333dfuL,
        0x0068c4a9d76af90fuL,
        0x0a4435229d0cc680uL,
        0x03607ae5a7347d0buL,
        0x0a3ae1371b74ea2cuL,
        0x02e5ed9539dfd0c9uL,
        0x0a31e01427183001uL,
        0x015d2c69c7599edcuL,
        0x0a27c589442700ebuL,
        0x0363677341a98a13uL,
        0x0a1f9be9e1d7b4e4uL,
        0x00e9cf2c5625685euL,
        0x0a15033c96eb7570uL,
        0x01e1298aebe8af0fuL,
        0x0a0bef014f36ffa8uL,
        0x01e6d2655c8560ebuL,
        0x0a0290979be09b3auL,
        0x03de58166789d0bcuL,
        0x09f8ac6ba86dcc3buL,
        0x02642b9e90b536b6uL,
        0x09f064e638fb2516uL,
        0x02dacfe7e64002b1uL,
        0x09e5c884857d8adeuL,
        0x0161d179e12ade6euL,
        0x09dcf0beaeb1b319uL,
        0x00e7ae01eb0f55cbuL,
        0x09d338e29511ffccuL,
        0x045f772a9e0423a1uL,
        0x09c9881a23b2ff9auL,
        0x02e4e72e15f0f016uL,
        0x09c0f43798c4f845uL,
        0x015bc4e2f5a8c9afuL,
        0x09b6836e63bd7d87uL,
        0x02627165d875ec78uL,
        0x09ade466f9c32fdauL,
        0x00687eb54ae1860duL,
        0x09a3d79f883687beuL,
        0x036043b38d103ec9uL,
        0x099a56d48500b8a3uL,
        0x006598a7d65e3b67uL,
        0x09917ac327f9b5e5uL,
        0x015cac2d1ee89db1uL,
        0x09873278f241bb95uL,
        0x016308090afcd9f3uL,
        0x097ec801820c3f3cuL,
        0x01e942d41e7bf2a3uL,
        0x09746b841565ab3euL,
        0x01e0c34dc595f4bfuL,
        0x096b16ea850bfa34uL,
        0x00e63e9cb83e74b2uL,
        0x0961f76e44abf0ebuL,
        0x045d83e5a3ffd7aduL,
        0x0957d432d7dd0ca0uL,
        0x03639428e0fd00c5uL,
        0x094f99abec00b682uL,
        0x0069f8c2eadfb109uL,
        0x0944f35579392d4buL,
        0x01613957092e7741uL,
        0x093bc6c19eee10e8uL,
        0x00e6d7ad6ac744f9uL,
        0x0932692d6adc530euL,
        0x03de4a41e3c393c2uL,
        0x0928673fad41c336uL,
        0x0364149a31665d1euL,
        0x09202bd066e6e445uL,
        0x045a9efbad7c9909uL,
        0x09156dece3f159c2uL,
        0x0361a4d14ca40e60uL,
        0x090c64dabfd6babcuL,
        0x02e7628f37011dc7uL,
        0x0902cf07ed3ac7c9uL,
        0x02defd93aae49244uL,
        0x08f8ea5cdb1b77f7uL,
        0x0264884565714d83uL,
        0x08f0801f05da3babuL,
        0x005b341347ab9d2euL,
        0x08e5da3ba0723cbcuL,
        0x006204d0f497ca7duL,
        0x08dcefd7b19fc691uL,
        0x0167de10a24a9be3uL,
        0x08d3281b7ca3d770uL,
        0x03df9c4f419d97b9uL,
        0x08c95c663259c5d8uL,
        0x0064ee2a6bb63f1duL,
        0x08c0c90568fe453buL,
        0x005bb6bea4d790c6uL,
        0x08b6374ef6370a23uL,
        0x01e258802fee3a1buL,
        0x08ad668024e6e773uL,
        0x0168491dcb50d650uL,
        0x08a3739f6c74a991uL,
        0x036012888bcf5e1buL,
        0x0899bc5a2748238fuL,
        0x0365456466d99824uL,
        0x089105de86fb726euL,
        0x025c25d7813e5a28uL,
        0x08868453b252f9aeuL,
        0x03e29f220ff323bduL,
        0x087dc7c640bf856euL,
        0x01e8a2c46b36447duL,
        0x0873b0e7a2d8004cuL,
        0x02e04b5178932d9euL,
        0x086a095d99893be9uL,
        0x02658d2d04dcdef9uL,
        0x0861361f24d04a1euL,
        0x00dc8060b8a624d8uL,
        0x0856c0994513d45buL,
        0x0162d8154e3020f5uL,
        0x084e12caa0268707uL,
        0x0168ea37661d565fuL,
        0x0843df6725a60cf5uL,
        0x006078003d294269uL,
        0x083a42bf15180a08uL,
        0x02e5c4df6da1a5f0uL,
        0x08315957e82800c6uL,
        0x015cc58a0676d26euL,
        0x0826eb9463d29a0duL,
        0x01e302d6b1661ef0uL,
        0x081e46dfa81a2018uL,
        0x01691ed1d851d1dduL,
        0x0813feb236502137uL,
        0x02e0982d94421652uL,
        0x080a67f97b02e025uL,
        0x0365ebfab91b4a2buL,
        0x08016f37032d6084uL,
        0x045cf4b3235443f5uL,
        0x07f704e120e656fduL,
        0x01e31f0304f01ddbuL,
        0x07ee638c247f445cuL,
        0x026940198fd0e1c2uL,
        0x07e40e7ff18c854cuL,
        0x01e0ab8eaa8fae67uL,
        0x07da78b6039c7038uL,
        0x02e60223e0067b2cuL,
        0x07d1778970df4481uL,
        0x005d0d6e2f89dd66uL,
        0x07c70c446e7535ccuL,
        0x00e32c589802b4bauL,
        0x07be688d1dc06741uL,
        0x01e94dc0e4e3bd62uL,
        0x07b40eab69ffb356uL,
        0x03e0b1f64079cf15uL,
        0x07aa74cd8f49285buL,
        0x016607271cb1c230uL,
        0x07a1723bbb37e710uL,
        0x005d0f815d3e30e4uL,
        0x079701ad03f5aba1uL,
        0x03632ab83cb1b9aauL,
        0x078e55d6dd34aeb5uL,
        0x016947a7e7d08e62uL,
        0x0783ff3437e5e591uL,
        0x0360ab555a059592uL,
        0x077a5c493ec4b75buL,
        0x0165faf8b45ee11cuL,
        0x07715f5a46f2a8c5uL,
        0x01dcfae7d166a387uL,
        0x0766e533a1804da4uL,
        0x03631a25c153692fuL,
        0x075e2b951ac76b4buL,
        0x00692ddcdd3a585auL,
        0x0753e03e7aaf4a22uL,
        0x036097bb793410b5uL,
        0x074a2f624fa2da40uL,
        0x02e5ddb524f58124uL,
        0x07413f112353b2e1uL,
        0x045ccfd1b6b2b0d1uL,
        0x0736b71aaf8395abuL,
        0x0362fac7e1ac1a55uL,
        0x072dea2a52e6f8d5uL,
        0x01e9009c068a7447uL,
        0x0723b2124c85eb7duL,
        0x026077566199da13uL,
        0x0719ee813dcc82f4uL,
        0x00e5afa0b60e30aduL,
        0x071111ab5ef7d9ceuL,
        0x025c8ea38207b48cuL,
        0x070677cd3ce598a2uL,
        0x00e2cce7b0334e93uL,
        0x06fd922e485849deuL,
        0x01e8c04eb792831buL,
        0x06f3751aaab95802uL,
        0x02e04a716678c7d9uL,
        0x06e99a3c2eb312deuL,
        0x036571266fb205e7uL,
        0x06e0d791e54efc95uL,
        0x015c37f46c8a36ceuL,
        0x06d627dd610c1f2euL,
        0x036290ef7aa6784euL,
        0x06cd246bba093ddcuL,
        0x01e86d89be61c44fuL,
        0x06c329e3d8fc35e5uL,
        0x016011744722e8f8uL,
        0x06b93354aecb0f90uL,
        0x02e522d67c700dd9uL,
        0x06b09149eae599f4uL,
        0x015bcc8c2b79e5e6uL,
        0x06a5c8020a89d6a7uL,
        0x016247692feaf7c7uL,
        0x069ca1dd59404577uL,
        0x02e8090b25f1fb1cuL,
        0x0692d1194826d1d8uL,
        0x03df99c33fa36826uL,
        0x0688bab4cd7bc185uL,
        0x0164c563ff8738eduL,
        0x06803f72f0fa181cuL,
        0x00db4d5ff233ee8buL,
        0x067559144638d7d1uL,
        0x0361f0fc4fe41aefuL,
        0x066c0baa10766978uL,
        0x026793b75fbd2367uL,
        0x06626b830bbc4f32uL,
        0x035efaa9eeaa4992uL,
        0x0658316ba6f8ef74uL,
        0x016459a26ac43fcfuL,
        0x064fc588d5eeb2ffuL,
        0x01eabb8ece685efeuL,
        0x0644dc0c0d42f862uL,
        0x02e18e6b704952c1uL,
        0x063b6320aea70779uL,
        0x02670e95e366ca95uL,
        0x0631fa02ebad6484uL,
        0x03de4700e7fab75euL,
        0x062798a96e59845auL,
        0x0263e0826243926duL,
        0x061ef81624855ca4uL,
        0x01ea185d71d9ae78uL,
        0x061451fcaaed5e70uL,
        0x00e1209163a43d8auL,
        0x060aa9b30dd7b332uL,
        0x01e67acd56555624uL,
        0x06017d9121b4ff43uL,
        0x01dd805487b20ec2uL,
        0x05f6f1bb0c9eff17uL,
        0x02e35b0e3e76f72auL,
        0x05ee184bec96bcc5uL,
        0x006965317fc3f8ebuL,
        0x05e3bc10ccdff1d6uL,
        0x0260a85e11600392uL,
        0x05d9e0f0cdf83a75uL,
        0x0365d99f4f4fa7a2uL,
        0x05d0f738d3253e75uL,
        0x00dca8538b911cc2uL,
        0x05c63e056b37b485uL,
        0x02e2ca663e8f6c6euL,
        0x05bd2806afda0511uL,
        0x01e8a38c763ae500uL,
        0x05b31b865207923auL,
        0x046026d30f31261euL,
        0x05a90a81bef15366uL,
        0x02652c63cbe5201duL,
        0x05a068145905badcuL,
        0x03dbc0c903e2dd51uL,
        0x05957f0081c7461buL,
        0x01e22fbc7eb40c8euL,
        0x058c293abfeb81c1uL,
        0x0167d5064d5d2e6auL,
        0x058271a9ed146425uL,
        0x015f3a001a1da12auL,
        0x0578282015bfd092uL,
        0x026474846e880b80uL,
        0x056fa292d1f4b615uL,
        0x016acb96019278e3uL,
        0x0564b6323fa7fafbuL,
        0x03618c50c637e437uL,
        0x055b1ded81f6cf48uL,
        0x0166fb47e7243b10uL,
        0x0551bfd2aff12d22uL,
        0x045e17fe4af1cdcduL,
        0x05473b9288cf980auL,
        0x0363b3779cd081bcuL,
        0x053e680a6315c8f9uL,
        0x00e9caab20737c4buL,
        0x0533e52969a46a03uL,
        0x0160e16c42489121uL,
        0x052a082ea93d471fuL,
        0x01e618056ad2fa0duL,
        0x0521075d9566cab2uL,
        0x00dce9e247afa7efuL,
        0x051646a66f6fb196uL,
        0x0362eabb9557e4c3uL,
        0x050d22f0f82317a8uL,
        0x0068c0020c90fd02uL,
        0x05030d7883df3e06uL,
        0x0460305d4157bdecuL,
        0x04f8ea1187daf8b2uL,
        0x01e52cf8a69cbdeeuL,
        0x04f049a91d747c01uL,
        0x04dbb1f3a4ce848cuL,
        0x04e54b29ff375e83uL,
        0x00621bd19407d3a8uL,
        0x04dbd5a7cbaf896duL,
        0x01e7ad97206eb3e9uL,
        0x04d230b0dec754dauL,
        0x015ef4e6059f1fe4uL,
        0x04c7c5a693980a40uL,
        0x01e43bdb9112e65buL,
        0x04bf10221f87a1c9uL,
        0x026a7278c0b2c815uL,
        0x04b44ae6c097e3b7uL,
        0x036148391a9b5b70uL,
        0x0uL,
        0x0uL, // VHEX(D, 04aa8288818abb3f), VHEX(D, 02669563388e87ee),
    },
    0x7fffffffffffffffuL, /* _AbsMask */
    0x403b3e0000000000uL, /* _MaxThreshold=3487.0/128.0 */
    0x8000000000000000uL, /* sign mask */
    0x3ff0000000000000uL, /* 1.0, used when _VLANG_FMA_AVAILABLE is defined */
    0x37f0000000000000uL, /* 2^(-128) */
    0x42c0000000000000uL, /* SRound */
    0x007fffffffffffffuL, /* Exp_x0_Mask */
    0x7ff0000000000000uL, /* ExpMask */
    0x3f60000000000000uL, /* TwoM9 = 2^(-9) */
                          // polynomial coefficients
    0x3efaec468d02a083uL, // poly1[0]
    0xbf2282607c399dbfuL, // poly1[1]
    0x3f64d516d3db4588uL, // poly3[0]
    0xbf844103d0fa9378uL, // poly3[1]
    0x3fa30107685fc1efuL, // poly5[0]
    0xbfb561b8921c8bc7uL, // poly5[1]
    0x3fa5766603986421uL, // poly7[0]
    0xbf9898a47706cfa8uL, // poly7[1]
    0x3f471de30b781626uL, // poly1[2]
    0x3fa1110ffd4fa97fuL, // poly3[2]
    0x3fc24921081babf0uL, // poly5[2]
    0xbf6a019f74e36970uL, // poly1[3]
    0xbfb861848a3ac063uL, // poly3[3]
    0xbfc55550d7299c73uL, // poly5[3]
    0x3f8a01a01a0f4f8cuL, // poly1[4]
    0x3fcc71c71cb27ab6uL, // poly3[4]
    0x3fb999999b2e0d33uL, // poly5[4]
    0xbfa6c16c16cc1d1duL, // poly1[5]
    0xbfd9999999d359a6uL, // poly3[5]
    0x3fc111111111108fuL, // poly1[6]
    0x3fdffffffffff778uL, // poly3[6]
    0xbfd5555555555019uL, // poly3[7]
    0xbfd55555555554d9uL, // poly1[7]
    0x3fe5555555555555uL, // poly1[8]
                          // VHEX_BROADCAST(D, bff0000000000000),  // poly1[9]
    0x403B39DC41E48BFDuL, /* UF_Threshold */
    0xfffff80000000000uL, /* SplitMask */
    0x47f0000000000000uL, /* 2^128 */
    0x00000000ffffffffuL, /* _Mask32 */
};                        /*dErf_Table*/
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
static __constant _iml_v2_dp_union_t __derfc_ha__imldErfcTab[608] = {
    0x00000000, 0xBFC00000, /* B   = -.125    */
    /* Polynomial coefficients */
    0x9728F11D, 0x3FEB8287, /* PH0 = +8.596837951986661119363e-01 */
    0x536ED695, 0x3C945E92, /* PL0 = +7.067062302091855131811e-17 */
    0xA1E869B6, 0xBFF1C62F, /* PH1 = -1.110885269596662450198e+00 */
    0x95E80C85, 0xBC8CE690, /* PL1 = -5.013462509852387052237e-17 */
    0xA1E869B6, 0x3FC1C62F, /* PH2 = +1.388606586995828062747e-01 */
    0x858F185B, 0x3C5CE68B, /* PL2 = +6.266811382417062127439e-18 */
    0xDBCC3336, 0x3FD6F552, /* PH3 = +3.587233683072555967541e-01 */
    0x2FEAA61E, 0x3C454B62, /* PL3 = +2.308750734795408017028e-18 */
    0xCD8DFDF1, 0xBFB196C9, /* PH4 = -6.870709675239773661648e-02 */
    0x2CB4DE57, 0xBC651327, /* PL4 = -9.139744736861412325091e-18 */
    0x23E123F4, 0xBFBAABA6, /* PH5 = -1.041816556545567817871e-01 */
    0x5CB92BAB, 0xBC5C9863, /* PL5 = -6.200609935844905132073e-18 */
    0x69C583CA, 0x3F9734EA, /* P6  = +2.266279478624592327297e-02 */
    0xE78941A7, 0x3F989258, /* P7  = +2.399577058014448549900e-02 */
    0xC0BF4823, 0xBF76F681, /* P8  = -5.606180999125273920259e-03 */
    0x5236DCCD, 0xBF727933, /* P9  = -4.510117028465998712941e-03 */
    0x0E76245C, 0x3F522D31, /* P10 = +1.109407335976328963348e-03 */
    0x879DDBF7, 0x3F475B71, /* P11 = +7.128052086348137456888e-04 */
    0x0F2D1F46, 0xBF27FA9A, /* P12 = -1.829445896765985010537e-04 */
    0x91E1CC4B, 0xBF196E07, /* P13 = -9.700699999530650874170e-05 */
    0x2406CB15, 0x3EFB4DED, /* P14 = +2.603950474773093312892e-05 */
    0x030024C9, 0x3EEB0E91, /* P15 = +1.290173540270874688622e-05 */
    /* index = 1 , 0.18921 <= |a[0]| < 0.41421 */
    /* Range reduction coefficient */
    0x00000000, 0xBFD40000, /* B   = -.3125   */
    /* Polynomial coefficients */
    0x5F5006E0, 0x3FE512B0, /* PH0 = +6.585313664984049353279e-01 */
    0x290813ED, 0x3C90D397, /* PL0 = +5.837873681975455930877e-17 */
    0xECBEC297, 0xBFF05FD3, /* PH1 = -1.023395466600197201146e+00 */
    0xD471CDC0, 0xBCA07415, /* PL1 = -1.141688000892149012402e-16 */
    0xE7EE733D, 0x3FD477C8, /* PH2 = +3.198110833125616392358e-01 */
    0xCF78C3FA, 0x3C792228, /* PL2 = +2.179978005216628762566e-17 */
    0x60ACAB72, 0x3FD1917B, /* PH3 = +2.745045131766153945208e-01 */
    0xEF9D960A, 0x3C82032C, /* PL3 = +3.124654080428842049883e-17 */
    0x28D4ED12, 0xBFC322A7, /* PH4 = -1.494950246213667122142e-01 */
    0xCFF6D5C7, 0xBC3A22F5, /* PL4 = -1.416865935501462800178e-18 */
    0xA9CD2C14, 0xBFB04C50, /* PH5 = -6.366447587531381957504e-02 */
    0xE04266F7, 0xBC6DD20E, /* PL5 = -1.293259747987646078442e-17 */
    0x4EEDDD63, 0x3FA7CE76, /* P6  = +4.649705613604273532831e-02 */
    0x58014213, 0x3F868AAC, /* P7  = +1.100668567247090568143e-02 */
    0x95F44231, 0xBF862AA8, /* P8  = -1.082355220435812954169e-02 */
    0xC4FAA60A, 0xBF56C003, /* P9  = -1.388553315261000723620e-03 */
    0x2AB9768A, 0x3F607950, /* P10 = +2.010971618615913158283e-03 */
    0x8822F759, 0x3F1D9C85, /* P11 = +1.129585744221428079474e-04 */
    0x873A7172, 0xBF345A8D, /* P12 = -3.105731513899568300881e-04 */
    0x3C59D237, 0xBEB147EC, /* P13 = -1.030024792040023479620e-06 */
    0x384E82AC, 0x3F0566C6, /* P14 = +4.082004946970771709144e-05 */
    0x731D5B88, 0xBEAAB979, /* P15 = -7.964524455322637084093e-07 */
    /* index = 2 , 0.41421 <= |a[0]| < 0.68179 */
    /* Range reduction coefficient */
    0x00000000, 0xBFE40000, /* B   = -.625    */
    /* Polynomial coefficients */
    0x465E1D96, 0x3FD81CD2, /* PH0 = +3.767591178115820005345e-01 */
    0xF52A128A, 0x3C7F25F4, /* PL0 = +2.701681674164514034641e-17 */
    0x94134B9D, 0xBFE86E96, /* PH1 = -7.634995357606048083099e-01 */
    0x3936E22C, 0xBC96212F, /* PL1 = -7.677756633007059633634e-17 */
    0x39181E85, 0x3FDE8A3C, /* PH2 = +4.771872098503780468270e-01 */
    0x1A5B5E31, 0x3C5D4D0C, /* PL2 = +6.353634182678935129854e-18 */
    0x021682E2, 0x3FAC8105, /* PH3 = +5.567184114921076842908e-02 */
    0x3A3888DC, 0x3C545A50, /* PL3 = +4.413307105477620098508e-18 */
    0x8A39D692, 0xBFC6963C, /* PH4 = -1.764598536425877051137e-01 */
    0x00A3FDFD, 0xBC709E91, /* PL4 = -1.441503172261824687232e-17 */
    0xDFFFC507, 0x3F9C1242, /* PH5 = +2.741341106588371032138e-02 */
    0x431E3C82, 0x3C2F7DDA, /* PL5 = +8.535818445990388799716e-19 */
    0x68E86346, 0x3FA52B26, /* P6  = +4.134483366596567958410e-02 */
    0xC1CA648D, 0xBF8C7CD9, /* P7  = -1.391000864650695545081e-02 */
    0xA74EEC9D, 0xBF7B62F4, /* P8  = -6.686168363496805689745e-03 */
    0x6F24D8C1, 0x3F6DC3B4, /* P9  = +3.633358393155012325403e-03 */
    0xDED2AD38, 0x3F481148, /* P10 = +7.344823658476445221948e-04 */
    0x2F21B169, 0xBF463795, /* P11 = -6.780126989390069777853e-04 */
    0x73D9A95C, 0xBF055041, /* P12 = -4.065227265644358015802e-05 */
    0xADB9086F, 0x3F1A1868, /* P13 = +9.954584987162546683263e-05 */
    0x996CB45C, 0xBECDF5A5, /* P14 = -3.571457635762179750353e-06 */
    0x4AA022D9, 0xBEEA45E3, /* P15 = -1.252794239194178541308e-05 */
    /* index = 3 , 0.68179 <= |a[0]| < 1.00000 */
    /* Range reduction coefficient */
    0x00000000, 0xBFF40000, /* B   = -1.25     */
    /* Polynomial coefficients */
    0x33A8F0D0, 0x3FB3BCD1, /* PH0 = +7.709987174252153074860e-02 */
    0xAB7C6ED8, 0x3C6211F2, /* PL0 = +7.836660616579099566352e-18 */
    0xFAF492D0, 0xBFCE4652, /* PH1 = -2.365211224906915710164e-01 */
    0x56F99C83, 0xBC7C1AA3, /* PL1 = -2.437638187373169284427e-17 */
    0xDBDD8551, 0x3FD2EBF3, /* PH2 = +2.956514021990033769249e-01 */
    0xA66EFC84, 0x3C54172E, /* PL2 = +4.356444800064670909222e-18 */
    0x32FE6C0D, 0xBFC571D0, /* PH3 = -1.675358056062069833647e-01 */
    0x2D4B5CA8, 0xBC65FE84, /* PL3 = -9.538465659642625354576e-18 */
    0x7F08D3E1, 0x3F793A82, /* PH4 = +6.159314871685390470335e-03 */
    0x9AD1202F, 0x3C26DFD6, /* PL4 = +6.200109967174716744523e-19 */
    0x3AF60574, 0x3FA8280A, /* PH5 = +4.718048066844007792930e-02 */
    0xC5CF7802, 0x3C1E360E, /* PL5 = +4.094376058011800904351e-19 */
    0x29AE2D6A, 0xBF95D0B0, /* P6  = -2.130389456173756853666e-02 */
    0x3B3037F7, 0xBF6DC84B, /* P7  = -3.635546623178771827981e-03 */
    0x5CF50410, 0x3F7739E0, /* P8  = +5.670429628765752622315e-03 */
    0x288ADD9C, 0xBF4EBFC6, /* P9  = -9.383885927353622020772e-04 */
    0xBEC91AAE, 0xBF4D74C6, /* P10 = -8.989305911846956119118e-04 */
    0x58F25E18, 0x3F281227, /* P11 = +1.836464911988279134414e-04 */
    0xF4E067D6, 0xBF16B13D, /* P12 = -8.656445172086578795083e-05 */
    0x82520A7C, 0xBF24261B, /* P13 = -1.537235813523113798776e-04 */
    0xA71582A5, 0xBF053AFA, /* P14 = -4.049374991655266949697e-05 */
    0x3CDFDF16, 0xBEC0E5EC, /* P15 = -2.014414781644530969243e-06 */
    /* index = 4 , 1.00000 <= |a[0]| < 1.37841 */
    /* Range reduction coefficient */
    0x00000000, 0xBFF80000, /* B   = -1.5      */
    /* Polynomial coefficients */
    0x8EC85290, 0x3FA15AAA, /* PH0 = +3.389485352469023826671e-02 */
    0x8BEF42FC, 0x3C3E0237, /* PL0 = +1.626772722288704126088e-18 */
    0x26B8128C, 0xBFBE7237, /* PH1 = -1.189302892235650221942e-01 */
    0xDD778E1A, 0xBC669E18, /* PL1 = -9.808806072670492044012e-18 */
    0x5D0B3179, 0x3FC6D5A9, /* PH2 = +1.783954338374192094552e-01 */
    0xB59EE035, 0x3C73EF9D, /* PL2 = +1.729172378661890528872e-17 */
    0x2BD6FF02, 0xBFC1C2A0, /* PH3 = -1.387520040570891510789e-01 */
    0x5099B1AC, 0xBC752CD8, /* PL3 = -1.836653719961153971163e-17 */
    0x6127BEBA, 0x3FA6D5A9, /* PH4 = +4.459885893799993283881e-02 */
    0x33B9CB18, 0x3C5A2510, /* PL4 = +5.669245124071952489034e-18 */
    0xC13F055A, 0x3F8E7237, /* PH5 = +1.486629065027738130778e-02 */
    0x57CDF6D4, 0x3C3DD26F, /* PL5 = +1.616654480594620215759e-18 */
    0x508685F5, 0xBF93CA3B, /* P6  = -1.932614020336660937250e-02 */
    0x0CA9FF89, 0x3F736DA2, /* P7  = +4.743226056802892559802e-03 */
    0x5E40AA2E, 0x3F635C6D, /* P8  = +2.363408670153852593658e-03 */
    0x3CC61F20, 0xBF5BF95E, /* P9  = -1.707403210362508339339e-03 */
    0x1376790B, 0x3F19FD8B, /* P10 = +9.914552071286027178235e-05 */
    0x9898123E, 0x3F317D7B, /* P11 = +2.668787716120535842388e-04 */
    0x3F2BBB11, 0xBF0ED920, /* P12 = -5.883817347817320720075e-05 */
    0x045EF193, 0x3ECA2831, /* P13 = +3.118157141350321079087e-06 */
    0xD1CA113B, 0x3EFFFACA, /* P14 = +3.049817779522785432100e-05 */
    0x111EDCE2, 0x3EDE896D, /* P15 = +7.280545348756929991407e-06 */
    /* index = 5 , 1.37841 <= |a[0]| < 1.82843 */
    /* Range reduction coefficient */
    0x00000000, 0xBFF80000, /* B   = -1.5      */
    /* Polynomial coefficients */
    0x8EC85204, 0x3FA15AAA, /* PH0 = +3.389485352468926682157e-02 */
    0xDB819C2F, 0x3C5C2E6C, /* PL0 = +6.110855860569441550177e-18 */
    0x26B824A8, 0xBFBE7237, /* PH1 = -1.189302892236293596184e-01 */
    0xCB511678, 0xBC6B3551, /* PL1 = -1.179971022390412119299e-17 */
    0x5D0A1B7E, 0x3FC6D5A9, /* PH2 = +1.783954338354440394276e-01 */
    0x163F1666, 0x3C761E30, /* PL2 = +1.918423861423959499597e-17 */
    0x2BEB6ABC, 0xBFC1C2A0, /* PH3 = -1.387520040942343824142e-01 */
    0x6F3FA623, 0xBC5EC068, /* PL3 = -6.668188905775501085753e-18 */
    0x5D0A1B72, 0x3FA6D5A9, /* PH4 = +4.459885845886092659018e-02 */
    0xD52A7843, 0x3C28CEF8, /* PL4 = +6.724353166671389331120e-19 */
    0x26B878A3, 0x3F8E7237, /* PH5 = +1.486628615299096477231e-02 */
    0x2B3EC189, 0x3C038AE6, /* PL5 = +1.324256364768719153782e-19 */
    0x72C535CF, 0xBF93CA3D, /* P6  = -1.932617199900282515464e-02 */
    0xA5D2DAD8, 0x3F736D73, /* P7  = +4.743053196924328349949e-03 */
    0xE89C6962, 0x3F635AE4, /* P8  = +2.362677657299958832241e-03 */
    0xA1577172, 0xBF5C0380, /* P9  = -1.709819426984085262802e-03 */
    0x6C5BB8A4, 0x3F185B33, /* P10 = +9.291173327216851248583e-05 */
    0x4C4B028B, 0x3F30AD10, /* P11 = +2.544560232245271699007e-04 */
    0x56CDC10D, 0xBF145B1C, /* P12 = -7.765160053252815866141e-05 */
    0xBFEB29DB, 0xBEF33433, /* P13 = -1.831428016422520340714e-05 */
    0x50C21429, 0x3EF00348, /* P14 = +1.527101726185947526620e-05 */
    0x091F6A29, 0xBEC55D31, /* P15 = -2.546790775587578767002e-06 */
    /* index = 6 , 1.82843 <= |a[0]| < 2.36359 */
    /* Range reduction coefficient */
    0x00000000, 0xC0040000, /* B   = -2.5      */
    /* Polynomial coefficients */
    0xF3B73438, 0x3FCAFBB3, /* PH0 = +2.108063640611435030081e-01 */
    0x81B79938, 0x3C7CDCC5, /* PL0 = +2.503413163677096344178e-17 */
    0x7F01AD4C, 0xBFB3086D, /* PH1 = -7.434734678979743049965e-02 */
    0x42C601A1, 0xBC6C7F8A, /* PL1 = -1.235912563389004715233e-17 */
    0xA7A8994F, 0x3F998958, /* PH2 = +2.493799708658999900179e-02 */
    0xEBA7CEDB, 0x3C2AE70F, /* PL2 = +7.291960730441339672571e-19 */
    0x76894030, 0xBF806320, /* PH3 = -8.001569383083134701096e-03 */
    0xFE6A86F7, 0xBBF7D08F, /* PL3 = -8.068733253162344956003e-20 */
    0x4CC7C3F8, 0x3F6435C0, /* PH4 = +2.467036805903873725176e-03 */
    0xAE30B51F, 0x3C1145FB, /* PL4 = +2.340978375314159972301e-19 */
    0xB1805CBD, 0xBF4809CE, /* PH5 = -7.335910077043355593043e-04 */
    0xDF90CAC7, 0xBB922E8E, /* PL5 = -9.625400548405819444466e-22 */
    0x3ABE7C07, 0x3F2BA8A3, /* P6  = +2.110194449160553717519e-04 */
    0x96277800, 0xBF0EDD77, /* P7  = -5.887051772267282193020e-05 */
    0x0414252C, 0x3EF0BB6A, /* P8  = +1.595696108317339439705e-05 */
    0xD541E9CE, 0xBED1B9D5, /* P9  = -4.226188441087313660768e-06 */
    0x98DCCD9C, 0x3EB1D593, /* P10 = +1.063006127077814124174e-06 */
    0xAF30A268, 0xBE946E66, /* P11 = -3.044494142922876706915e-07 */
    0xFB726A7D, 0x3E624822, /* P12 = +3.405247588871700464376e-08 */
    0x5215DD02, 0xBE65EE8E, /* P13 = -4.085127104207537014336e-08 */
    0x8A90A20D, 0xBE41B60D, /* P14 = -8.247394172183139582756e-09 */
    0x6DA61F56, 0xBE319781, /* P15 = -4.095914463832445816757e-09 */
    /* index = 7 , 2.36359 <= |a[0]| < 3.00000 */
    /* Range reduction coefficient */
    0x00000000, 0xC0080000, /* B   = -3        */
    /* Polynomial coefficients */
    0x7D229D2C, 0x3FC6E982, /* PH0 = +1.790011511813899280909e-01 */
    0xA71024B3, 0x3C79BE28, /* PL0 = +2.232832652217315924449e-17 */
    0x4D14B16E, 0xBFABD6AE, /* PH1 = -5.437226000717286511499e-02 */
    0x2CF2AA37, 0xBC5D02EB, /* PL1 = -6.290844797024351953136e-18 */
    0x1A98C0CC, 0x3F9043FE, /* PH2 = +1.588437115987133274597e-02 */
    0xF58D6DC6, 0x3C028926, /* PL2 = +1.256031324958086062712e-19 */
    0x1BA85761, 0xBF725906, /* PH3 = -4.479431018372754570722e-03 */
    0x39418C57, 0xBC2236E2, /* PL3 = -4.937020139237959615699e-19 */
    0x2ED3A51E, 0x3F5409CC, /* PH4 = +1.223039052371816936410e-03 */
    0xC713AB01, 0x3BE35D3F, /* PL4 = +3.280432298485645887207e-20 */
    0x9D2011E0, 0xBF353DEC, /* PH5 = -3.241255445803042228858e-04 */
    0xB291AD69, 0xBBDF61EE, /* PL5 = -2.658205253589004723770e-20 */
    0x2C42EF76, 0x3F15E739, /* P6  = +8.355413871446816938815e-05 */
    0x7D3BC702, 0xBEF60251, /* P7  = -2.098947136536852204430e-05 */
    0x8208F37A, 0x3ED595E7, /* P8  = +5.146399445762954462053e-06 */
    0x347FA123, 0xBEB4B1E5, /* P9  = -1.233512380233578921585e-06 */
    0xC08000DA, 0x3E93627C, /* P10 = +2.888547787627375511624e-07 */
    0xA57A3907, 0xBE721246, /* P11 = -6.732117562911647243075e-08 */
    0x66EA33A2, 0x3E4D6FB4, /* P12 = +1.370736698910330723738e-08 */
    0xAECE5167, 0xBE33D83A, /* P13 = -4.620441568319444136471e-09 */
    0x39B9DE97, 0xBDE456E0, /* P14 = -1.479883133251195577334e-10 */
    0x32098FCA, 0xBE001D0B, /* P15 = -4.689631773839406720911e-10 */
    /* index = 8 , 3.00000 <= |a[0]| < 3.75683 */
    /* Range reduction coefficient */
    0x00000000, 0xC0080000, /* B   = -3        */
    /* Polynomial coefficients */
    0x7D229D2C, 0x3FC6E982, /* PH0 = +1.790011511813899280909e-01 */
    0xECBD772B, 0x3C79BE26, /* PL0 = +2.232830365461207732574e-17 */
    0x4D14B16E, 0xBFABD6AE, /* PH1 = -5.437226000717286511499e-02 */
    0x4561D938, 0xBC5CB99D, /* PL1 = -6.228753627329738947833e-18 */
    0x1A98C0CB, 0x3F9043FE, /* PH2 = +1.588437115987132927653e-02 */
    0x2CD91206, 0x3C4AAFA5, /* PL2 = +2.893314290772168246331e-18 */
    0x1BA855BF, 0xBF725906, /* PH3 = -4.479431018372392013516e-03 */
    0x3DD664F8, 0xBC282676, /* PL3 = -6.545936135507353246969e-19 */
    0x2ED3B247, 0x3F5409CC, /* PH4 = +1.223039052372547471834e-03 */
    0xA58D9A1E, 0x3BFA707E, /* PL4 = +8.958028165265505037128e-20 */
    0x9CF7A1A8, 0xBF353DEC, /* PH5 = -3.241255444366383277333e-04 */
    0x0D619CA0, 0xBBED3B0E, /* PL5 = -4.951870343255805989112e-20 */
    0x2DD18C72, 0x3F15E739, /* P6  = +8.355413906850783266166e-05 */
    0xC442F162, 0xBEF60250, /* P7  = -2.098946085092710162349e-05 */
    0x2275B994, 0x3ED595ED, /* P8  = +5.146419915431905541076e-06 */
    0x790DCEE8, 0xBEB4B108, /* P9  = -1.233311625462144392179e-06 */
    0x29EA4F7B, 0x3E93672A, /* P10 = +2.891270386798041615380e-07 */
    0xC4DAE4D7, 0xBE71C5E4, /* P11 = -6.620966638394383103212e-08 */
    0x71614B23, 0x3E4F6AAA, /* P12 = +1.462952385289519746746e-08 */
    0xF14CE3ED, 0xBE29880D, /* P13 = -2.972253452390389255211e-09 */
    0x1A00B97F, 0x3E00B1D4, /* P14 = +4.858780496046739194158e-10 */
    0xAA3D0C51, 0xBDC91460, /* P15 = -4.561952662834934469701e-11 */
    /* index = 9 , 3.75683 <= |a[0]| < 4.65685 */
    /* Range reduction coefficient */
    0x00000000, 0xC0140000, /* B   = -5        */
    /* Polynomial coefficients */
    0x9E943C2C, 0x3FBC5723, /* PH0 = +1.107046377330653252891e-01 */
    0xE90EA447, 0x3C3F9083, /* PL0 = +1.711115667162339866661e-18 */
    0x497DA9B2, 0xBF95D843, /* PH1 = -2.133278976490666362098e-02 */
    0xC5DB327D, 0xBC4958F1, /* PL1 = -2.748174860564178119366e-18 */
    0x2B69BCAA, 0x3F708CF8, /* PH2 = +4.040688908033699192068e-03 */
    0xB0481CEE, 0x3C1F9CFF, /* PL2 = +4.284397882371289611826e-19 */
    0x9BDC1F59, 0xBF48ABC1, /* PH3 = -7.528968196490556279502e-04 */
    0xE8F54A82, 0xBBE3AAD6, /* PL3 = -3.331777292184567868949e-20 */
    0x829B3F7A, 0x3F2219F2, /* PH4 = +1.381023915230058835175e-04 */
    0x90E66C36, 0x3BDA838C, /* PL4 = +2.245811604276452230644e-20 */
    0xBFD9D18A, 0xBEFA2A88, /* PH5 = -2.495398438502738613096e-05 */
    0xB2D9B160, 0xBBA7BA7D, /* PL5 = -2.512350597419754267052e-21 */
    0xD8B722C8, 0x3ED2A3C9, /* P6  = +4.444074316941374438703e-06 */
    0x1FF47694, 0xBEAA35FE, /* P7  = -7.811459567273970640998e-07 */
    0x2E08AA26, 0x3E820DA2, /* P8  = +1.345072382514117585994e-07 */
    0x1B23B0BD, 0xBE59DB4B, /* P9  = -2.408084904478389023895e-08 */
    0xAAEEE61C, 0x3E2A40D5, /* P10 = +3.056281748051161490004e-09 */
    0x9A7A2B2F, 0xBE1575E3, /* P11 = -1.249165751661470590471e-09 */
    0xDCF656C1, 0xBDEB97BD, /* P12 = -2.007636127492143902008e-10 */
    0x47038278, 0xBDE1F7F8, /* P13 = -1.307390059987260112212e-10 */
    0x019B6639, 0xBDBA8853, /* P14 = -2.413118324991824340890e-11 */
    0xCA9B4A15, 0xBD90EA22, /* P15 = -3.845933264329345997788e-12 */
    /* index = 10, 4.65685 <= |a[0]| < 5.72717 */
    /* Range reduction coefficient */
    0x00000000, 0xC0180000, /* B   = -6        */
    /* Polynomial coefficients */
    0x8489D716, 0x3FB7C034, /* PH0 = +9.277656780053819551846e-02 */
    0x56FC28AC, 0x3C60C0E9, /* PL0 = +7.265698668824114372571e-18 */
    0x6D9D12F8, 0xBF8ED7F6, /* PH1 = -1.506035348905630788519e-02 */
    0x7B4FB124, 0xBC3AA6DB, /* PL1 = -1.444796242536647170241e-18 */
    0x4A8043B6, 0x3F63C776, /* PH2 = +2.414446866176390808778e-03 */
    0x395EA4ED, 0x3C1A1333, /* PL2 = +3.533821505783413573156e-19 */
    0x7D3A97C8, 0xBF39106A, /* PH3 = -3.824481948221350631478e-04 */
    0xE409F413, 0xBBE65DE0, /* PL3 = -3.789068534525912662357e-20 */
    0x8CA11CD1, 0x3F0F64CD, /* PH4 = +5.987884794982914553026e-05 */
    0x2C56B6EC, 0x3BA56A85, /* PL4 = +2.267517211913373633307e-21 */
    0x4157A0EF, 0xBEE370D1, /* PH5 = -9.270044794854453634801e-06 */
    0xECEF52D3, 0xBB89660E, /* PL5 = -6.722970712247342732869e-22 */
    0xB79002F5, 0x3EB7D0CD, /* P6  = +1.419522702273017638515e-06 */
    0x3DD02B8C, 0xBE8CDF7B, /* P7  = -2.151204218373179321673e-07 */
    0xE011892E, 0x3E6149CB, /* P8  = +3.220190692637674657821e-08 */
    0xBEC759FA, 0xBE34D966, /* P9  = -4.854338247743904137811e-09 */
    0xF8086603, 0x3E061DD2, /* P10 = +6.436748773444691992105e-10 */
    0xAEE46EB0, 0xBDE3F16F, /* P11 = -1.451052259652364958956e-10 */
    0xECB23213, 0xBDA01108, /* P12 = -7.306217607264472270351e-12 */
    0x1514D73E, 0xBDA682BA, /* P13 = -1.023665931430311422136e-11 */
    0xB5A8249E, 0xBD7CCB3A, /* P14 = -1.636741705360417497371e-12 */
    0xE58B6B89, 0xBD5440D7, /* P15 = -2.878166231081046406140e-13 */
    /* index = 11, 5.72717 <= |a[0]| < 7.00000 */
    /* Range reduction coefficient */
    0x00000000, 0xC0180000, /* B   = -6        */
    /* Polynomial coefficients */
    0x8489D721, 0x3FB7C034, /* PH0 = +9.277656780053834817412e-02 */
    0x52C58503, 0x3C5CA9E0, /* PL0 = +6.215422957630999163349e-18 */
    0x6D9D09FD, 0xBF8ED7F6, /* PH1 = -1.506035348905231975591e-02 */
    0x30978609, 0xBC3B0A30, /* PL1 = -1.465830376901830526211e-18 */
    0x4A81F461, 0x3F63C776, /* PH2 = +2.414446866224426602870e-03 */
    0x4CC7DD34, 0x3C1F5AF1, /* PL2 = +4.249428018213156350008e-19 */
    0x7CD79E3C, 0xBF39106A, /* PH3 = -3.824481944705059640460e-04 */
    0x128A0C55, 0xBBCAB024, /* PL3 = -1.130282900246833283649e-20 */
    0x9C07B7FA, 0x3F0F64CD, /* PH4 = +5.987884970069797072479e-05 */
    0x973656B9, 0x3BB70FC3, /* PL4 = +4.883479143449795236453e-21 */
    0x64177A9D, 0xBEE370D0, /* PH5 = -9.270038506534623371489e-06 */
    0xCA3C8D09, 0xBB8C48CE, /* PL5 = -7.486819761708650728098e-22 */
    0x3ED67FB5, 0x3EB7D0E0, /* P6  = +1.419539553772204058214e-06 */
    0xEC6A97F8, 0xBE8CDE4C, /* P7  = -2.150860522648929071972e-07 */
    0x6E8EBF33, 0x3E615134, /* P8  = +3.225581030692536713387e-08 */
    0xE35119EA, 0xBE3491BB, /* P9  = -4.789157116946741883298e-09 */
    0x178DFC80, 0x3E083208, /* P10 = +7.041798664221452071869e-10 */
    0x926FF2F9, 0xBDDC3068, /* P11 = -1.025513325381537290929e-10 */
    0xF3C04744, 0x3DB03B2A, /* P12 = +1.476212141627693514970e-11 */
    0xDC6BC5F5, 0xBD821E5C, /* P13 = -2.059846843475102042800e-12 */
    0x424D11CE, 0x3D51CC56, /* P14 = +2.529275094461536537033e-13 */
    0x0CFFEFCE, 0xBD161650, /* P15 = -1.961733795671940839456e-14 */
    /* index = 12, 7.00000 <= |a[0]| < 8.51366 */
    /* Range reduction coefficient */
    0x00000000, 0xC0240000, /* B   = -10        */
    /* Polynomial coefficients */
    0x1F599E07, 0x3FACBE83, /* PH0 = +5.614099271479849190269e-02 */
    0x9B5D5D11, 0x3C566FD2, /* PL0 = +4.865207055837691637626e-18 */
    0x91A3878F, 0xBF76C55C, /* PH1 = -5.559312436362775187992e-03 */
    0x29307AEA, 0xBC15305D, /* PL1 = -2.871634343409622442297e-19 */
    0x6D081EA1, 0x3F41F3DD, /* PH2 = +5.478697949578733824086e-04 */
    0x2F85F6BF, 0x3BEECD83, /* PL2 = +5.218194410910425324723e-20 */
    0xACB9D989, 0xBF0C2CC8, /* PH3 = -5.373942766777595046084e-05 */
    0xDE4DFD19, 0xBBB8D35D, /* PL3 = -5.257036097860468190305e-21 */
    0xD1A2E75D, 0x3ED5FE91, /* PH4 = +5.243876581123783232462e-06 */
    0x7CF2841A, 0x3B821389, /* PL4 = +4.784761177768180772867e-22 */
    0x2BBC15D1, 0xBEA13271, /* PH5 = -5.125117108212647066897e-07 */
    0xA57ACBF7, 0xBB2E706C, /* PL5 = -1.258934060250243913466e-23 */
    0x1299800A, 0x3E694950, /* P6  = +4.709954943833527374290e-08 */
    0x6A098B46, 0xBE3A8530, /* P7  = -6.174731532155882725521e-09 */
    0xCA91617F, 0xBDF1AAA9, /* P8  = -2.570836413264466811035e-10 */
    0xEA5E5F9B, 0xBDF61B23, /* P9  = -3.216848821768813405453e-10 */
    0x96986E30, 0xBDD607F9, /* P10 = -8.014886467085944074100e-11 */
    0x3EBC5946, 0xBDB5E068, /* P11 = -1.989664329212594525251e-11 */
    0x49C5E229, 0xBD8D2840, /* P12 = -3.314793384528502069933e-12 */
    0x6C66E9CF, 0xBD5CD6D4, /* P13 = -4.098293803813285568237e-13 */
    0xA52D5517, 0xBD218B8B, /* P14 = -3.116635761380032184627e-14 */
    0x6A58BFA0, 0xBCD67CC2, /* P15 = -1.248298217228707722088e-15 */
    /* index = 13, 8.51366 <= |a[0]| < 10.31371 */
    /* Range reduction coefficient */
    0x00000000, 0xC0280000, /* B   = -12        */
    /* Polynomial coefficients */
    0xC5C1371A, 0x3FA7FD46, /* PH0 = +4.685422100065590733653e-02 */
    0xAE3BFE44, 0x3C5BBF78, /* PL0 = +6.016874322967494971121e-18 */
    0xC1306384, 0xBF6FC477, /* PH1 = -3.877862830915681027766e-03 */
    0x025A9643, 0xBBFBEF63, /* PL1 = -9.464781562126468091969e-20 */
    0x1102C8BB, 0x3F34F67E, /* PH2 = +3.198678759971475336725e-04 */
    0x977B1858, 0x3BE19BB7, /* PL2 = +2.982956990099603274065e-20 */
    0x4914D09B, 0xBEFB9312, /* PH3 = -2.629709030550816730993e-05 */
    0x592540A0, 0xBBAA84F0, /* PL3 = -2.807839302504418260391e-21 */
    0xE6AFB869, 0x3EC211BD, /* PH4 = +2.154028881921076004426e-06 */
    0x398F93E4, 0x3B70AB01, /* PL4 = +2.205989769661318421396e-22 */
    0xD02886AE, 0xBE87B520, /* PH5 = -1.766348774615811518552e-07 */
    0x960DC01C, 0xBB3B5949, /* PL5 = -2.262237983652115629603e-23 */
    0x51E6C042, 0x3E4DBF0D, /* P6  = +1.385169895177704424437e-08 */
    0x1633DBA4, 0xBE1883B0, /* P7  = -1.426926209838787884619e-09 */
    0xE352A9E8, 0xBDB70BDF, /* P8  = -2.096056506112467303796e-11 */
    0xE87E3636, 0xBDC9A05D, /* P9  = -4.661420994133798885138e-11 */
    0x74D5438C, 0xBDA542CB, /* P10 = -9.668345682245144615529e-12 */
    0x08D290A5, 0xBD829CEC, /* P11 = -2.116050450249746927441e-12 */
    0xCEEA5BA5, 0xBD5572AC, /* P12 = -3.047936920785516594522e-13 */
    0x03E8D3A2, 0xBD227CC3, /* P13 = -3.284013185281208657656e-14 */
    0xBB1794E9, 0xBCE384F9, /* P14 = -2.167092681974533161384e-15 */
    0x986C2EAA, 0xBC95D838, /* P15 = -7.578872799230437417738e-17 */
    /* index = 14, 10.31371 <= |a[0]| < 12.45434 */
    /* Range reduction coefficient */
    0x00000000, 0xC0280000, /* B   = -12        */
    /* Polynomial coefficients */
    0xC5E0864D, 0x3FA7FD46, /* PH0 = +4.685422101489376128791e-02 */
    0x09247FFC, 0x3C3890CE, /* PL0 = +1.331706221831208465061e-18 */
    0xB46D6659, 0xBF6FC477, /* PH1 = -3.877862738062270737266e-03 */
    0x917D1A19, 0xBC1552DF, /* PL1 = -2.889903469760923727329e-19 */
    0x473CC4F8, 0x3F34F67F, /* PH2 = +3.198681581465102723161e-04 */
    0x67AA7311, 0x3BE19CD7, /* PL2 = +2.983700969842561996877e-20 */
    0xDB6A4C39, 0xBEFB92ED, /* PH3 = -2.629656020276488365161e-05 */
    0x9323A265, 0xBB9BA755, /* PL3 = -1.463972015808350754232e-21 */
    0xAB42F442, 0x3EC21338, /* PH4 = +2.154717856665936739360e-06 */
    0x50EFE1AA, 0x3B7A3B2B, /* PL4 = +3.471661109597578359561e-22 */
    0x1AC65B1B, 0xBE879E92, /* PH5 = -1.759783691101313870009e-07 */
    0x6DDAC65A, 0xBB33F149, /* PL5 = -1.649607144283048541004e-23 */
    0x64ABB83D, 0x3E4EC3B2, /* P6  = +1.432580911316871873360e-08 */
    0x9B569EE9, 0xBE13F89D, /* P7  = -1.162474211815902746231e-09 */
    0xA9783AD5, 0x3DD9D8BF, /* P8  = +9.402965417353367412827e-11 */
    0xBFD67D2D, 0xBDA0AC34, /* P9  = -7.581857012818548345008e-12 */
    0x77383011, 0x3D657163, /* P10 = +6.094445545884397147187e-13 */
    0x93DD687F, 0xBD2B811E, /* P11 = -4.885758078484284813761e-14 */
    0x72299765, 0x3CF1666E, /* P12 = +3.863603386424802983607e-15 */
    0x24EEB59F, 0xBCB87D6D, /* P13 = -3.398662831632079393599e-16 */
    0xE71C414F, 0x3C6CF84A, /* P14 = +1.256368833866149779235e-17 */
    0x53A34013, 0xBC549B9C, /* P15 = -4.468616038460557301918e-18 */
    /* index = 15, 12.45434 <= |a[0]| < 15.00000 */
    /* Range reduction coefficient */
    0x00000000, 0xC0280000, /* B   = -12        */
    /* Polynomial coefficients */
    0xC5E0864C, 0x3FA7FD46, /* PH0 = +4.685422101489375434902e-02 */
    0x78BFBA45, 0x3C272668, /* PL0 = +6.274828641509031623674e-19 */
    0xB46D6566, 0xBF6FC477, /* PH1 = -3.877862738062165352815e-03 */
    0x1B925347, 0xBC1899A9, /* PL1 = -3.333953749567984913256e-19 */
    0x473C9571, 0x3F34F67F, /* PH2 = +3.198681581458506979244e-04 */
    0x78CC1D91, 0x3BDA98D8, /* PL2 = +2.252858027431785150500e-20 */
    0xDB5F131A, 0xBEFB92ED, /* PH3 = -2.629656020027284834628e-05 */
    0x4C4D07B3, 0xBB7BFB8A, /* PL3 = -3.703463495511892670191e-22 */
    0xAA5DA40A, 0x3EC21338, /* PH4 = +2.154717850301212589078e-06 */
    0xD9C2C294, 0x3B7533EB, /* PL4 = +2.806169536753554072221e-22 */
    0x008190E3, 0xBE879E92, /* PH5 = -1.759783574444804469825e-07 */
    0x7797666E, 0xBAF46AC1, /* PL5 = -1.055534913889543642325e-24 */
    0x27E315C3, 0x3E4EC3B0, /* P6  = +1.432579321522661311114e-08 */
    0x0F940503, 0xBE13F88B, /* P7  = -1.162457739713884263078e-09 */
    0xE2660617, 0x3DD9D7D2, /* P8  = +9.401651037517552911559e-11 */
    0xC1E29C3A, 0xBDA0A7A3, /* P9  = -7.573745501338381321947e-12 */
    0xB2008C20, 0x3D654E35, /* P10 = +6.055389242639684594076e-13 */
    0x8763F87C, 0xBD2AAD54, /* P11 = -4.738799762937255621770e-14 */
    0xF5AB6E9D, 0x3CEF6DAE, /* P12 = +3.489258984229413691354e-15 */
    0x3D984DA9, 0xBCAFFD2B, /* P13 = -2.219678680392544065728e-16 */
    0x1C245BF0, 0x3C68091A, /* P14 = +1.042376022993320165406e-17 */
    0xB29EA9F2, 0xBC132241, /* P15 = -2.593115469510864474516e-19 */
    /* index = 16, 15.00000 <= |a[0]| < 18.02731 */
    /* Range reduction coefficient */
    0x00000000, 0xC0340000, /* B   = -20        */
    /* Polynomial coefficients */
    0x89B687FC, 0x3F9CD9BC, /* PH0 = +2.817434874089740082237e-02 */
    0x71E519FA, 0x3C43EF0A, /* PL0 = +2.161221844560317095442e-18 */
    0xC09F2766, 0xBF5705E8, /* PH1 = -1.405217454236462517464e-03 */
    0x0408FB08, 0xBBF36417, /* PL1 = -6.569918285842021897652e-20 */
    0x2C9281BC, 0x3F125999, /* PH2 = +6.999967003418807722989e-05 */
    0xE50A9084, 0x3BCE8040, /* PL2 = +1.291767181774881166965e-20 */
    0xB0711D41, 0xBECD36FF, /* PH3 = -3.482680202479945060537e-06 */
    0x9D97DD8F, 0xBB3AB20B, /* PL3 = -2.208199155550967230155e-23 */
    0xD17F7509, 0x3E873A36, /* PH4 = +1.730576080207233701290e-07 */
    0xBACC70D5, 0x3B2F76DE, /* PL4 = +1.301334439827987179720e-23 */
    0xACE5737D, 0xBE4272FC, /* PH5 = -8.591063330073796559049e-09 */
    0x690E2795, 0xBAE097D4, /* PL5 = -4.289211909718843135394e-25 */
    0xEA72BD9D, 0x3DFD2F97, /* P6  = +4.247109143958435726067e-10 */
    0x60C32295, 0xBDB7AC02, /* P7  = -2.152947789459854206463e-11 */
    0x85BA536E, 0x3D6F7204, /* P8  = +8.937314960904922884953e-13 */
    0xB994B501, 0xBD394723, /* P9  = -8.980510155645059579267e-14 */
    0xFF013997, 0xBCF7042C, /* P10 = -5.110647812986618592825e-15 */
    0xF7824A22, 0xBCD7415D, /* P11 = -1.290930699563734837436e-15 */
    0x565B14CB, 0xBCA21F26, /* P12 = -1.257444060764218380104e-16 */
    0x7A013753, 0xBC6906EE, /* P13 = -1.085376422585727739728e-17 */
    0xF66BE081, 0xBC237987, /* P14 = -5.278636380644175701162e-19 */
    0xA3778EB5, 0xBBD1A67F, /* P15 = -1.495045771956186467450e-20 */
    /* index = 17, 18.02731 <= |a[0]| < 21.62742 */
    /* Range reduction coefficient */
    0x00000000, 0xC0380000, /* B   = -24        */
    /* Polynomial coefficients */
    0x88D32FC8, 0x3F980D1E, /* PH0 = +2.348754606354910845489e-02 */
    0xE4C13CC8, 0x3BC11F8F, /* PL0 = +7.251995140064273100374e-21 */
    0x8F42E24F, 0xBF5001A6, /* PH1 = -9.769560393079362078245e-04 */
    0xB639FF78, 0xBBA831A6, /* PL1 = -2.561634104245913607173e-21 */
    0x4BE3330C, 0x3F054964, /* PH2 = +4.060113268309324479309e-05 */
    0x124EB567, 0x3B89AFBF, /* PL2 = +6.799162391117347981195e-22 */
    0xF4DDDC7D, 0xBEBC48D3, /* PH3 = -1.685886633377171627666e-06 */
    0x33B51BE1, 0xBB4C0CDF, /* PL3 = -4.640529689556493556424e-23 */
    0x8F719C61, 0x3E72C662, /* PH4 = +6.994210709349966198374e-08 */
    0xD6A25309, 0x3B065850, /* PL4 = +2.310417141995942018020e-24 */
    0x2635F3B0, 0xBE28E916, /* PH5 = -2.899963201593786136787e-09 */
    0x3827E21E, 0xBAD3ECE5, /* PL5 = -2.575294050118122612377e-25 */
    0x1C143D97, 0x3DE07603, /* P6  = +1.197694287839830919046e-10 */
    0xE19784CD, 0xBD965664, /* P7  = -5.078954205880457368179e-12 */
    0x7A8C4AEA, 0x3D48A4FC, /* P8  = +1.751095447792382875218e-13 */
    0x3A29F48F, 0xBD10E82A, /* P9  = -1.501633869336638589763e-14 */
    0xE73A7DE2, 0xBCCA9669, /* P10 = -7.379528503588020861739e-16 */
    0xA720E1CA, 0xBCA6096A, /* P11 = -1.529109036840242579387e-16 */
    0x2422CC17, 0xBC6CCA29, /* P12 = -1.248553789186111546830e-17 */
    0x701FFB5E, 0xBC30916F, /* P13 = -8.981588616695516325451e-19 */
    0x27F92100, 0xBBE58CF8, /* P14 = -3.650824075674495663740e-20 */
    0x7F7B2262, 0xBB90474C, /* P15 = -8.617771976972918012715e-22 */
    /* index = 18, 21.62742 <= |a[0]| < 25.90869 */
    /* Range reduction coefficient */
    0x00000000, 0xC0380000, /* B   = -24        */
    /* Polynomial coefficients */
    0x88D3C61F, 0x3F980D1E, /* PH0 = +2.348754606368263705973e-02 */
    0xDB6AA98B, 0x3C4FE707, /* PL0 = +3.458872038588374884904e-18 */
    0x8F1B5449, 0xBF5001A6, /* PH1 = -9.769560387458289894141e-04 */
    0x4C6DC256, 0xBBD02F74, /* PL1 = -1.370954023475464061342e-20 */
    0x558F64CD, 0x3F054964, /* PH2 = +4.060113378274443765104e-05 */
    0x1431C594, 0x3BB1C937, /* PL2 = +3.766331298428091548922e-21 */
    0x7F6C7D0E, 0xBEBC48D2, /* PH3 = -1.685885306641605947312e-06 */
    0xA6995905, 0xBB5F73CA, /* PL3 = -1.040669673694017132041e-22 */
    0xFE0705C2, 0x3E72C675, /* PH4 = +6.994321167295849567189e-08 */
    0xC28EE575, 0x3B13D8D8, /* PL4 = +4.104275294024223009813e-24 */
    0x8190518C, 0xBE28E79B, /* PH5 = -2.899290596240391333390e-09 */
    0xE322D84D, 0xBAD83BDB, /* PL5 = -3.132148335993573511713e-25 */
    0x91CFEB8F, 0x3DE080E8, /* P6  = +1.200791210441542747239e-10 */
    0x7BCD7CA2, 0xBD95DAA8, /* P7  = -4.969054624729652332190e-12 */
    0x1E193B99, 0x3D4CEA35, /* P8  = +2.054525297256377804560e-13 */
    0xE06B785D, 0xBD031CBA, /* P9  = -8.487533569846840815219e-15 */
    0x7BEB0D76, 0x3CB93E90, /* P10 = +3.503363176188788286412e-16 */
    0x343307E0, 0xBC70A8A0, /* P11 = -1.444911422938005329496e-17 */
    0xF4E32036, 0x3C25F673, /* P12 = +5.953003622048074693286e-19 */
    0xF15BB353, 0xBBDCCF3A, /* P13 = -2.440258988821977760917e-20 */
    0xE0C2BD31, 0x3B936C1C, /* P14 = +1.028208828843002843297e-21 */
    0xA2439E70, 0xBB4E2303, /* P15 = -4.985711068305271137101e-23 */
    /* index = 19, 25.90869 <= |a[0]| < 27.22602 */
    /* Range reduction coefficient */
    0x00000000, 0xC0380000, /* B   = -24        */
    /* Polynomial coefficients */
    0x88D3C61F, 0x3F980D1E, /* PH0 = +2.348754606368263705973e-02 */
    0x8017808A, 0x3C474AB8, /* PL0 = +2.525310446004377934367e-18 */
    0x8F1B542D, 0xBF5001A6, /* PH1 = -9.769560387458229178820e-04 */
    0x3F6D744D, 0xBBE03775, /* PL1 = -2.747204456532203879708e-20 */
    0x558F5A32, 0x3F054964, /* PH2 = +4.060113378272604009542e-05 */
    0xFE249593, 0x3B8E9207, /* PL2 = +8.091926624742914852517e-22 */
    0x7F69FC49, 0xBEBC48D2, /* PH3 = -1.685885306606869761420e-06 */
    0x5834345E, 0xBB5A8B2A, /* PL3 = -8.782545445167356372591e-23 */
    0xFDD26BF9, 0x3E72C675, /* PH4 = +6.994321162733464091993e-08 */
    0xD6E5CF88, 0x3B2FC152, /* PL4 = +1.313363060180429806309e-23 */
    0x7B30AFBB, 0xBE28E79B, /* PH5 = -2.899290552014914771856e-09 */
    0xBEB0A7A8, 0xBAB50F09, /* PL5 = -6.804446670057210557773e-26 */
    0x464BF716, 0x3DE080E8, /* P6  = +1.200790882945486774254e-10 */
    0x08585C62, 0xBD95DAA3, /* P7  = -4.969035712753671439053e-12 */
    0xAEA55A61, 0x3D4CE9E5, /* P8  = +2.054439173264395762229e-13 */
    0x434D6E9E, 0xBD031AF0, /* P9  = -8.484425882266839469087e-15 */
    0xBD415341, 0x3CB92E28, /* P10 = +3.494469870697635224097e-16 */
    0x16CA3688, 0xBC706D61, /* P11 = -1.424837913533794877032e-17 */
    0xE7F2FDCD, 0x3C24AC97, /* P12 = +5.603751214790867572401e-19 */
    0x5A4713CF, 0xBBD76D72, /* P13 = -1.984388650724449395076e-20 */
    0x11680385, 0x3B849DF3, /* P14 = +5.457271859359798402112e-22 */
    0x36082C94, 0xBB242EE9, /* P15 = -8.347594902406587689065e-24 */
    /* Coefficients for exp(R) - 1 polynomial approximation */
    0x00000000, 0x3FE00000, /* EXP_POLY2 = .500000000000000 */
    0x555548F8, 0x3FC55555, /* EXP_POLY3 = .166666666666579 */
    0x55558FCC, 0x3FA55555, /* EXP_POLY4 = .041666666666771 */
    0x3AAF20D3, 0x3F811112, /* EXP_POLY5 = .008333341995140 */
    0x1C2A3FFD, 0x3F56C16A, /* EXP_POLY6 = .001388887045923 */
                            /* T(j) and D(j) entries, j goes from 0 to 63 */
    0x00000000, 0x3FF00000, /* T( 0) = +1.000000000000000000000e+00 */
    0x00000000, 0x00000000, /* D( 0) = +0.000000000000000000000e-01 */
    0x38000000, 0x3FF02C9A, /* T( 1) = +1.010889261960983276367e+00 */
    0x83B9BDF3, 0x3E59DE01, /* D( 1) = +2.409071718365322229056e-08 */
    0xD0000000, 0x3FF059B0, /* T( 2) = +1.021897137165069580078e+00 */
    0xA1D73E2A, 0x3E48AC2B, /* D( 2) = +1.148904709815635513478e-08 */
    0x18000000, 0x3FF08745, /* T( 3) = +1.033024877309799194336e+00 */
    0x0230D7C9, 0x3E1D66F2, /* D( 3) = +1.711429228164170783970e-09 */
    0x68000000, 0x3FF0B558, /* T( 4) = +1.044273763895034790039e+00 */
    0x3D8A62E5, 0x3E53E624, /* D( 4) = +1.853237905028290397874e-08 */
    0x30000000, 0x3FF0E3EC, /* T( 5) = +1.055645167827606201172e+00 */
    0x10103A17, 0x3E469E8D, /* D( 5) = +1.053295095763646632515e-08 */
    0xD0000000, 0x3FF11301, /* T( 6) = +1.067140400409698486328e+00 */
    0xA4EBBF1B, 0x3DF25B50, /* D( 6) = +2.671251318413961209928e-10 */
    0xA8000000, 0x3FF1429A, /* T( 7) = +1.078760772943496704102e+00 */
    0x7ECD0406, 0x3E5AA4B7, /* D( 7) = +2.481362308963911753744e-08 */
    0x38000000, 0x3FF172B8, /* T( 8) = +1.090507715940475463867e+00 */
    0xEB737DF2, 0x3E51F545, /* D( 8) = +1.672478219533982315576e-08 */
    0xE8000000, 0x3FF1A35B, /* T( 9) = +1.102382570505142211914e+00 */
    0xA9E5B4C8, 0x3E4B7E5B, /* D( 9) = +1.280269873164235170943e-08 */
    0x30000000, 0x3FF1D487, /* T(10) = +1.114386737346649169922e+00 */
    0xA7805B80, 0x3E368B9A, /* D(10) = +5.249243366386937956920e-09 */
    0x88000000, 0x3FF2063B, /* T(11) = +1.126521617174148559570e+00 */
    0x8EE3BAC1, 0x3E18A335, /* D(11) = +1.434093340224486143787e-09 */
    0x68000000, 0x3FF2387A, /* T(12) = +1.138788610696792602539e+00 */
    0xE19B07EB, 0x3E59D588, /* D(12) = +2.405989905116476778384e-08 */
    0x60000000, 0x3FF26B45, /* T(13) = +1.151189208030700683594e+00 */
    0x7495E99D, 0x3E5789F3, /* D(13) = +2.192228202222400963520e-08 */
    0xF0000000, 0x3FF29E9D, /* T(14) = +1.163724839687347412109e+00 */
    0x84B09745, 0x3E547F7B, /* D(14) = +1.909023010170419859909e-08 */
    0xA0000000, 0x3FF2D285, /* T(15) = +1.176396965980529785156e+00 */
    0x2D002475, 0x3E5B900C, /* D(15) = +2.566975149112839572848e-08 */
    0x08000000, 0x3FF306FE, /* T(16) = +1.189207106828689575195e+00 */
    0xA96F46AD, 0x3E418DB8, /* D(16) = +8.174031491522187470560e-09 */
    0xB0000000, 0x3FF33C08, /* T(17) = +1.202156722545623779297e+00 */
    0xFA64E431, 0x3E4320B7, /* D(17) = +8.907079362799521957498e-09 */
    0x30000000, 0x3FF371A7, /* T(18) = +1.215247333049774169922e+00 */
    0x2A9C5154, 0x3E5CEAA7, /* D(18) = +2.693069470819464525134e-08 */
    0x30000000, 0x3FF3A7DB, /* T(19) = +1.228480517864227294922e+00 */
    0xDBA86F25, 0x3E53967F, /* D(19) = +1.824264271077213395779e-08 */
    0x48000000, 0x3FF3DEA6, /* T(20) = +1.241857796907424926758e+00 */
    0x88D6D049, 0x3E5048D0, /* D(20) = +1.516605912183586496873e-08 */
    0x20000000, 0x3FF4160A, /* T(21) = +1.255380749702453613281e+00 */
    0x9F84325C, 0x3E3F72E2, /* D(21) = +7.322237476298140657442e-09 */
    0x60000000, 0x3FF44E08, /* T(22) = +1.269050955772399902344e+00 */
    0x40C4DBD0, 0x3E18624B, /* D(22) = +1.419333320210669081032e-09 */
    0xB0000000, 0x3FF486A2, /* T(23) = +1.282869994640350341797e+00 */
    0x404F068F, 0x3E5704F3, /* D(23) = +2.143842793892979478102e-08 */
    0xD0000000, 0x3FF4BFDA, /* T(24) = +1.296839535236358642578e+00 */
    0x9C750E5F, 0x3E54D8A8, /* D(24) = +1.941465102335562911779e-08 */
    0x70000000, 0x3FF4F9B2, /* T(25) = +1.310961186885833740234e+00 */
    0x9AB4CF63, 0x3E5A74B2, /* D(25) = +2.463893060168861678633e-08 */
    0x50000000, 0x3FF5342B, /* T(26) = +1.325236618518829345703e+00 */
    0x077C2A0F, 0x3E5A753E, /* D(26) = +2.464091194892641209550e-08 */
    0x30000000, 0x3FF56F47, /* T(27) = +1.339667499065399169922e+00 */
    0x699BB2C0, 0x3E5AD49F, /* D(27) = +2.498790383543815566972e-08 */
    0xD8000000, 0x3FF5AB07, /* T(28) = +1.354255527257919311523e+00 */
    0xA56324C0, 0x3E552150, /* D(28) = +1.967897341677457724014e-08 */
    0x10000000, 0x3FF5E76F, /* T(29) = +1.369002401828765869141e+00 */
    0x21BA6F93, 0x3E56B485, /* D(29) = +2.114582474278897613298e-08 */
    0xB0000000, 0x3FF6247E, /* T(30) = +1.383909881114959716797e+00 */
    0x58F87D03, 0x3E0D2AC2, /* D(30) = +8.488722380757845272652e-10 */
    0x80000000, 0x3FF66238, /* T(31) = +1.398979663848876953125e+00 */
    0x24893ECF, 0x3E42A911, /* D(31) = +8.689434187084528136715e-09 */
    0x60000000, 0x3FF6A09E, /* T(32) = +1.414213538169860839844e+00 */
    0x32422CBF, 0x3E59FCEF, /* D(32) = +2.420323420895793872421e-08 */
    0x38000000, 0x3FF6DFB2, /* T(33) = +1.429613322019577026367e+00 */
    0xBBC8838B, 0x3E519468, /* D(33) = +1.637239298486787827828e-08 */
    0xE8000000, 0x3FF71F75, /* T(34) = +1.445180803537368774414e+00 */
    0x7BA46E1E, 0x3E2D8BEE, /* D(34) = +3.439677845622943741472e-09 */
    0x50000000, 0x3FF75FEB, /* T(35) = +1.460917770862579345703e+00 */
    0x22FDBA6B, 0x3E59099F, /* D(35) = +2.331806764294817789031e-08 */
    0x70000000, 0x3FF7A114, /* T(36) = +1.476826131343841552734e+00 */
    0x36BEA881, 0x3E4F580C, /* D(36) = +1.459565775865253248037e-08 */
    0x30000000, 0x3FF7E2F3, /* T(37) = +1.492907702922821044922e+00 */
    0x8841740B, 0x3E5B3D39, /* D(37) = +2.536844380427876853149e-08 */
    0x98000000, 0x3FF82589, /* T(38) = +1.509164422750473022461e+00 */
    0x28ACF88B, 0x3E34CCE1, /* D(38) = +4.842949717305082051033e-09 */
    0x98000000, 0x3FF868D9, /* T(39) = +1.525598138570785522461e+00 */
    0x640720ED, 0x3E4A2497, /* D(39) = +1.217375278439031618952e-08 */
    0x40000000, 0x3FF8ACE5, /* T(40) = +1.542210817337036132813e+00 */
    0xDADD3E2B, 0x3E415506, /* D(40) = +8.070904690799791862091e-09 */
    0x98000000, 0x3FF8F1AE, /* T(41) = +1.559004396200180053711e+00 */
    0x62B98274, 0x3E315773, /* D(41) = +4.037656913322790589475e-09 */
    0xB0000000, 0x3FF93737, /* T(42) = +1.575980842113494873047e+00 */
    0x9E8A0388, 0x3E29B8BC, /* D(42) = +2.994391613408395160182e-09 */
    0x98000000, 0x3FF97D82, /* T(43) = +1.593142122030258178711e+00 */
    0x3E2E7A48, 0x3E5F7939, /* D(43) = +2.931200871922631114312e-08 */
    0x80000000, 0x3FF9C491, /* T(44) = +1.610490322113037109375e+00 */
    0x80E3E236, 0x3E451F84, /* D(44) = +9.836217198804520667357e-09 */
    0x78000000, 0x3FFA0C66, /* T(45) = +1.628027409315109252930e+00 */
    0x2594D6D4, 0x3E4AEF2B, /* D(45) = +1.254223851391853102201e-08 */
    0xB0000000, 0x3FFA5503, /* T(46) = +1.645755469799041748047e+00 */
    0xE45A1225, 0x3E41F12A, /* D(46) = +8.354923096471881724726e-09 */
    0x50000000, 0x3FFA9E6B, /* T(47) = +1.663676559925079345703e+00 */
    0xFD0FAC91, 0x3E55E7F6, /* D(47) = +2.040165708934321145698e-08 */
    0x98000000, 0x3FFAE89F, /* T(48) = +1.681792825460433959961e+00 */
    0xD5E8734D, 0x3E35AD3A, /* D(48) = +5.046995126101313452466e-09 */
    0xB8000000, 0x3FFB33A2, /* T(49) = +1.700106352567672729492e+00 */
    0xBDAFF43A, 0x3E13C57E, /* D(49) = +1.150850740009175073498e-09 */
    0xF0000000, 0x3FFB7F76, /* T(50) = +1.718619287014007568359e+00 */
    0x37553D84, 0x3E47DAF2, /* D(50) = +1.110847034726996937646e-08 */
    0x90000000, 0x3FFBCC1E, /* T(51) = +1.737333834171295166016e+00 */
    0x891EE83D, 0x3E12F074, /* D(51) = +1.102411082978577081872e-09 */
    0xD8000000, 0x3FFC199B, /* T(52) = +1.756252139806747436523e+00 */
    0x7088832C, 0x3E56154A, /* D(52) = +2.056655204658872311938e-08 */
    0x28000000, 0x3FFC67F1, /* T(53) = +1.775376468896865844727e+00 */
    0x2D2884E0, 0x3E595F45, /* D(53) = +2.362965540782399670020e-08 */
    0xD8000000, 0x3FFCB720, /* T(54) = +1.794709056615829467773e+00 */
    0xA4540F2F, 0x3E53BE41, /* D(54) = +1.838727771865426574213e-08 */
    0x48000000, 0x3FFD072D, /* T(55) = +1.814252167940139770508e+00 */
    0xDC687918, 0x3E403C4B, /* D(55) = +7.560258985742022100362e-09 */
    0xD8000000, 0x3FFD5818, /* T(56) = +1.834008067846298217773e+00 */
    0x1C976817, 0x3E53EE92, /* D(56) = +1.856304424571364568959e-08 */
    0x00000000, 0x3FFDA9E6, /* T(57) = +1.853979110717773437500e+00 */
    0x2B84600D, 0x3E4ED994, /* D(57) = +1.436561213089245307034e-08 */
    0x30000000, 0x3FFDFC97, /* T(58) = +1.874167621135711669922e+00 */
    0xF5CB4656, 0x3E4BDCDA, /* D(58) = +1.297458823140812394995e-08 */
    0xE0000000, 0x3FFE502E, /* T(59) = +1.894575953483581542969e+00 */
    0xD89CF44C, 0x3E5E2CFF, /* D(59) = +2.810338409837146865343e-08 */
    0xA0000000, 0x3FFEA4AF, /* T(60) = +1.915206551551818847656e+00 */
    0xCC2C7B9D, 0x3E452486, /* D(60) = +9.845328446216361270296e-09 */
    0xE8000000, 0x3FFEFA1B, /* T(61) = +1.936061769723892211914e+00 */
    0x9DDC7F48, 0x3E598568, /* D(61) = +2.376840223868399340457e-08 */
    0x58000000, 0x3FFF5076, /* T(62) = +1.957144111394882202148e+00 */
    0x033A7C26, 0x3E4B722A, /* D(62) = +1.278051806686988475163e-08 */
    0x80000000, 0x3FFFA7C1, /* T(63) = +1.978456020355224609375e+00 */
    0x82E90A7E, 0x3E39E90D, /* D(63) = +6.032726358883249918131e-09 */
    /* Double precision constants */
    /* Two parts of ln(2.0)/64 */
    0xFEFA0000, 0x3F862E42, /* LOG_HI = .010830424696223 */
    0xBC9E3B3A, 0x3D1CF79A, /* LOG_LO = 2.572804622327669e-14 */
    /* TWO_TO_THE_K_DIV_LN2 = 2^6/ln(2.0) rounded to double */
    0x652B82FE, 0x40571547, /* 92.332482616893658 */
    0x00000000, 0x33700000, /* UNSCALE       = 2^(-200) */
    0x00000000, 0x43380000, /* RS_EXP = 2^52 + 2^51 */
    0x02000000, 0x41A00000, /* RS_MuL = 2^27 + 1 */
    0x00000000, 0x00000000, /* ZERO          =  0.0    */
    0x00000001, 0x00100000, /* TINY = +2.225073858507201877156e-308 */
    0x00000000, 0x3FF00000, /* ONES(0)       =  1.0    */
    0x00000000, 0xBFF00000, /* ONES(1)       = -1.0    */
    0x00000000, 0x40000000, /* TWO           =  2.0    */
    /* UNDERFLOW_THRESHOLD = +2.722601711110836575358e+01 */
    0x41E48BFD, 0x403B39DC,
    /* LARGE_DENORM_THRESHOLD = +2.669937226834560206612e+01 */
    0x0F9C4611, 0x403AB30A,
    /* GRADUAL_UNDERFLOW_THRESHOLD = +2.654325845425098151509e+01 */
    0xFC6E4892, 0x403A8B12,
    /* SATURATION_THRESHOLD = -5.863584748755168 */
    0x8F74E94A, 0xC017744F};
__attribute__((always_inline)) inline int
__ocl_svml_internal_derfc_ha(double *a, double *r) {
  int nRet = 0;
  double absAi;
  double rHi, rLo;
  double sHi, sMid, sLo;
  double p, pHi, pMid, pLo;
  double y, y4, yHi, yMid, yLo;
  double res, resHi, resMid, resLo;
  double aHi, aLo, a2Hi, a2Lo;
  double R, RMid, RLo;
  double scale, w, wLog;
  double expHi, expMid, expLo;
  double t1, t2;
  double Nj, v1, v2, v3;
  int iSign, expnt;
  _iml_uint32_t N = 0, j, index;
  /* Filter out INFs and NaNs */
  if ((((((_iml_v2_dp_union_t *)&a[0])->dwords.hi_dword >> 20) & 0x7FF) !=
       0x7FF)) {
    /* Here if argument is finite */
    /* Get the biased exponent of a[0] */
    expnt = ((((_iml_v2_dp_union_t *)&a[0])->dwords.hi_dword >> 20) & 0x7FF);
    /* Check whether |a[0]| >= 2^(NEAR_ZERO_THRESHOLD_EXP) */
    if (expnt >= -70 + 0x3FF) {
      /* Here if argument is not within "Near zero" interval */
      /* Check if saturation doesn't occur */
      if (a[0] > ((__constant double *)__derfc_ha__imldErfcTab)[607]) {
        /* Here if no saturtion: erfc(a[0]) < 2.0 */
        /* Check if erfc(a[0]) underflows */
        if (a[0] < ((__constant double *)__derfc_ha__imldErfcTab)[604]) {
          /* Path 5) No underflow. Main path */
          absAi = a[0];
          (((_iml_v2_dp_union_t *)&absAi)->dwords.hi_dword =
               (((_iml_v2_dp_union_t *)&absAi)->dwords.hi_dword & 0x7FFFFFFF) |
               ((_iml_uint32_t)(0) << 31));
          /* Obtain index */
          y4 =
              absAi + ((__constant double *)__derfc_ha__imldErfcTab)[601 + (0)];
          y4 = y4 * y4;
          y4 = y4 * y4;
          index = ((((_iml_v2_dp_union_t *)&y4)->dwords.hi_dword >> 20) & 0x7FF) -
                  0x3FF;
          /* y + yMid = |a[0]| + B */
          v1 = ((absAi) + (((__constant double *)
                                __derfc_ha__imldErfcTab)[index * 23 + 0]));
          v2 = ((absAi)-v1);
          v3 = (v1 + v2);
          v2 = ((((__constant double *)
                      __derfc_ha__imldErfcTab)[index * 23 + 0]) +
                v2);
          v3 = ((absAi)-v3);
          v3 = (v2 + v3);
          y = v1;
          yMid = v3;
          ;
          /* Compute the high order part of the polynomial */
          res = ((((__constant double *)
                       __derfc_ha__imldErfcTab)[index * 23 + 22] *
                      y +
                  ((__constant double *)
                       __derfc_ha__imldErfcTab)[index * 23 + 21]) *
                     y +
                 ((__constant double *)
                      __derfc_ha__imldErfcTab)[index * 23 + 20]) *
                    y +
                ((__constant double *)__derfc_ha__imldErfcTab)[index * 23 + 19];
          res = ((res * y + ((__constant double *)
                                 __derfc_ha__imldErfcTab)[index * 23 + 18]) *
                     y +
                 ((__constant double *)
                      __derfc_ha__imldErfcTab)[index * 23 + 17]) *
                    y +
                ((__constant double *)__derfc_ha__imldErfcTab)[index * 23 + 16];
          res = ((res * y + ((__constant double *)
                                 __derfc_ha__imldErfcTab)[index * 23 + 15]) *
                     y +
                 ((__constant double *)
                      __derfc_ha__imldErfcTab)[index * 23 + 14]) *
                    y +
                ((__constant double *)__derfc_ha__imldErfcTab)[index * 23 + 13];
          res = res * y;
          /* Add the lower terms of the polynomial */
          /* using multiprecision technique        */
          /* Re-split y + yMid into yHi + yLo */
          v1 = ((y) * (((__constant double *)__derfc_ha__imldErfcTab)[598]));
          v2 = (v1 - (y));
          v1 = (v1 - v2);
          v2 = ((y)-v1);
          yHi = v1;
          yLo = v2;
          ;
          yLo += yMid;
          /* sHi + sLo ~=  PH5 + PL5 + res */
          v1 = ((((__constant double *)
                      __derfc_ha__imldErfcTab)[index * 23 + 11]) +
                (res));
          t1 = ((((__constant double *)
                      __derfc_ha__imldErfcTab)[index * 23 + 11]) -
                v1);
          v2 = (t1 + (res));
          sHi = v1;
          sLo = v2;
          ;
          sLo +=
              ((__constant double *)__derfc_ha__imldErfcTab)[index * 23 + 12];
          /* Split sHi into rHi + rLo */
          v1 = ((sHi) * (((__constant double *)__derfc_ha__imldErfcTab)[598]));
          v2 = (v1 - (sHi));
          v1 = (v1 - v2);
          v2 = ((sHi)-v1);
          rHi = v1;
          rLo = v2;
          ;
          rLo += sLo;
          /* rHi + rLo ~= (rHi + rLo) * (yHi + yLo) */
          t1 = ((rHi) * (yHi));
          t2 = ((rLo) * (yLo));
          t2 = (t2 + (rHi) * (yLo));
          v1 = (t2 + (rLo) * (yHi));
          rHi = t1;
          rLo = v1;
          ;
          /* sHi + sLo ~=  PH4 + PL4 + rHi + rLo */
          v1 = ((((__constant double *)
                      __derfc_ha__imldErfcTab)[index * 23 + 9]) +
                (rHi));
          t1 = ((((__constant double *)
                      __derfc_ha__imldErfcTab)[index * 23 + 9]) -
                v1);
          v2 = (t1 + (rHi));
          sHi = v1;
          sLo = v2;
          ;
          sLo += rLo;
          sLo +=
              ((__constant double *)__derfc_ha__imldErfcTab)[index * 23 + 10];
          /* Split sHi into rHi + rLo */
          v1 = ((sHi) * (((__constant double *)__derfc_ha__imldErfcTab)[598]));
          v2 = (v1 - (sHi));
          v1 = (v1 - v2);
          v2 = ((sHi)-v1);
          rHi = v1;
          rLo = v2;
          ;
          rLo += sLo;
          /* rHi + rLo ~= (rHi + rLo) * (yHi + yLo) */
          t1 = ((rHi) * (yHi));
          t2 = ((rLo) * (yLo));
          t2 = (t2 + (rHi) * (yLo));
          v1 = (t2 + (rLo) * (yHi));
          rHi = t1;
          rLo = v1;
          ;
          /* sHi + sLo ~=  PH3 + PL3 + rHi + rLo */
          v1 = ((((__constant double *)
                      __derfc_ha__imldErfcTab)[index * 23 + 7]) +
                (rHi));
          t1 = ((((__constant double *)
                      __derfc_ha__imldErfcTab)[index * 23 + 7]) -
                v1);
          v2 = (t1 + (rHi));
          sHi = v1;
          sLo = v2;
          ;
          sLo += rLo;
          sLo += ((__constant double *)__derfc_ha__imldErfcTab)[index * 23 + 8];
          /* Split sHi into rHi + rLo */
          v1 = ((sHi) * (((__constant double *)__derfc_ha__imldErfcTab)[598]));
          v2 = (v1 - (sHi));
          v1 = (v1 - v2);
          v2 = ((sHi)-v1);
          rHi = v1;
          rLo = v2;
          ;
          rLo += sLo;
          /* rHi + rLo ~= (rHi + rLo) * (yHi + yLo) */
          t1 = ((rHi) * (yHi));
          t2 = ((rLo) * (yLo));
          t2 = (t2 + (rHi) * (yLo));
          v1 = (t2 + (rLo) * (yHi));
          rHi = t1;
          rLo = v1;
          ;
          /* sHi + sLo ~=  PH2 + PL2 + rHi + rLo */
          v1 = ((((__constant double *)
                      __derfc_ha__imldErfcTab)[index * 23 + 5]) +
                (rHi));
          t1 = ((((__constant double *)
                      __derfc_ha__imldErfcTab)[index * 23 + 5]) -
                v1);
          v2 = (t1 + (rHi));
          sHi = v1;
          sLo = v2;
          ;
          sLo += rLo;
          sLo += ((__constant double *)__derfc_ha__imldErfcTab)[index * 23 + 6];
          /* Split sHi into rHi + rLo */
          v1 = ((sHi) * (((__constant double *)__derfc_ha__imldErfcTab)[598]));
          v2 = (v1 - (sHi));
          v1 = (v1 - v2);
          v2 = ((sHi)-v1);
          rHi = v1;
          rLo = v2;
          ;
          rLo += sLo;
          /* rHi + rLo ~= (rHi + rLo) * (yHi + yLo) */
          t1 = ((rHi) * (yHi));
          t2 = ((rLo) * (yLo));
          t2 = (t2 + (rHi) * (yLo));
          v1 = (t2 + (rLo) * (yHi));
          rHi = t1;
          rLo = v1;
          ;
          /* sHi + sLo ~=  PH1 + PL1 + rHi + rLo */
          v1 = ((((__constant double *)
                      __derfc_ha__imldErfcTab)[index * 23 + 3]) +
                (rHi));
          t1 = ((((__constant double *)
                      __derfc_ha__imldErfcTab)[index * 23 + 3]) -
                v1);
          v2 = (t1 + (rHi));
          sHi = v1;
          sLo = v2;
          ;
          sLo += rLo;
          sLo += ((__constant double *)__derfc_ha__imldErfcTab)[index * 23 + 4];
          /* Split sHi into rHi + rLo */
          v1 = ((sHi) * (((__constant double *)__derfc_ha__imldErfcTab)[598]));
          v2 = (v1 - (sHi));
          v1 = (v1 - v2);
          v2 = ((sHi)-v1);
          rHi = v1;
          rLo = v2;
          ;
          rLo += sLo;
          /* rHi + rLo ~= (rHi + rLo) * (yHi + yLo) */
          t1 = ((rHi) * (yHi));
          t2 = ((rLo) * (yLo));
          t2 = (t2 + (rHi) * (yLo));
          v1 = (t2 + (rLo) * (yHi));
          rHi = t1;
          rLo = v1;
          ;
          /* sHi + sLo ~=  PH0 + PL0 + rHi + rLo */
          v1 = ((((__constant double *)
                      __derfc_ha__imldErfcTab)[index * 23 + 1]) +
                (rHi));
          t1 = ((((__constant double *)
                      __derfc_ha__imldErfcTab)[index * 23 + 1]) -
                v1);
          v2 = (t1 + (rHi));
          sHi = v1;
          sLo = v2;
          ;
          sLo += rLo;
          sLo += ((__constant double *)__derfc_ha__imldErfcTab)[index * 23 + 2];
          /* resHi + resLo ~= sHi + sLo */
          v1 = ((sHi) + (sLo));
          t1 = ((sHi)-v1);
          v2 = (t1 + (sLo));
          resHi = v1;
          resLo = v2;
          ;
          /* Now resHi + resLo represents the value of the */
          /* polynomial P(|a[0]|). Next we check whether   */
          /* we need to multiply P(|a[0]|) by the          */
          /* exp(-|a[0]|^2)                                */
          if (index < 6) {
            /* Path 5.1)                           */
            /* No multiplication by exp(-|a[0]|^2) */
            scale = ((__constant double *)__derfc_ha__imldErfcTab)[601 + (0)];
          } else {
            /* Path 5.2) */
            /* Split resHi into resHi + resMid */
            v1 = ((resHi) *
                  (((__constant double *)__derfc_ha__imldErfcTab)[598]));
            v2 = (v1 - (resHi));
            v1 = (v1 - v2);
            v2 = ((resHi)-v1);
            resHi = v1;
            resMid = v2;
            ;
            /* Accumulate resMid + resLo in resLo */
            resLo += resMid;
            /*----------------- Exp Section -------------*/
            /* Split a[0] into aHi + aLo */
            v1 = ((a[0]) *
                  (((__constant double *)__derfc_ha__imldErfcTab)[598]));
            v2 = (v1 - (a[0]));
            v1 = (v1 - v2);
            v2 = ((a[0]) - v1);
            aHi = v1;
            aLo = v2;
            ;
            /* a2Hi + a2Lo ~= (aHi+aLo)^2 */
            t1 = ((aHi) * (aHi));
            t2 = ((aLo) * (aLo));
            t2 = (t2 + (aHi) * (aLo));
            v1 = (t2 + (aLo) * (aHi));
            a2Hi = t1;
            a2Lo = v1;
            ;
            /* Change sign */
            /* a2Hi + a2Lo ~= -(a[0])^2 */
            a2Hi *= ((__constant double *)__derfc_ha__imldErfcTab)[601 + (1)];
            a2Lo *= ((__constant double *)__derfc_ha__imldErfcTab)[601 + (1)];
            /* Range Reduction part */
            w = a2Hi * ((__constant double *)__derfc_ha__imldErfcTab)[595];
            Nj = w + ((__constant double *)__derfc_ha__imldErfcTab)[597];
            w = Nj - ((__constant double *)__derfc_ha__imldErfcTab)[597];
            /* R + RLo ~=                                */
            /*  ~= a2Hi + a2Lo - w * LOG_HI - w * LOG_LO */
            R = a2Hi - w * ((__constant double *)__derfc_ha__imldErfcTab)[593];
            wLog = -w * ((__constant double *)__derfc_ha__imldErfcTab)[594];
            v1 = ((R) + (wLog));
            v2 = ((R)-v1);
            v3 = (v1 + v2);
            v2 = ((wLog) + v2);
            v3 = ((R)-v3);
            v3 = (v2 + v3);
            R = v1;
            RLo = v3;
            ;
            v1 = ((R) + (a2Lo));
            v2 = ((R)-v1);
            v3 = (v1 + v2);
            v2 = ((a2Lo) + v2);
            v3 = ((R)-v3);
            v3 = (v2 + v3);
            R = v1;
            RMid = v3;
            ;
            RLo += RMid;
            /* get N and j from Nj's significand */
            N = (((_iml_v2_dp_union_t *)&Nj)->dwords.lo_dword);
            j = N & ((1 << 6) - 1);
            N = N >> 6;
            N = N + 0x3FF;
            /* Approximation part: polynomial series */
            p = ((((((__constant double *)__derfc_ha__imldErfcTab)[464] * R +
                    ((__constant double *)__derfc_ha__imldErfcTab)[463]) *
                       R +
                   ((__constant double *)__derfc_ha__imldErfcTab)[462]) *
                      R +
                  ((__constant double *)__derfc_ha__imldErfcTab)[461]) *
                     R +
                 ((__constant double *)__derfc_ha__imldErfcTab)[460]) *
                R * R;
            /* pHi + pLo ~= p + R */
            v2 = ((p) + (R));
            t1 = ((p)-v2);
            v3 = (t1 + (R));
            pHi = v2;
            pLo = v3;
            ;
            /* Split pHi into pHi + pLo */
            v1 =
                ((pHi) * (((__constant double *)__derfc_ha__imldErfcTab)[598]));
            v2 = (v1 - (pHi));
            v1 = (v1 - v2);
            v2 = ((pHi)-v1);
            pHi = v1;
            pMid = v2;
            ;
            pLo += pMid;
            pLo += RLo;
            /* expHi + expLo ~=                         */
            /*              (T(j) + D(j)) * (pHi + pLo) */
            t1 =
                ((((__constant double *)__derfc_ha__imldErfcTab)[465 + 2 * j]) *
                 (pHi));
            t2 =
                ((((__constant double *)__derfc_ha__imldErfcTab)[466 + 2 * j]) *
                 (pLo));
            t2 =
                (t2 +
                 (((__constant double *)__derfc_ha__imldErfcTab)[465 + 2 * j]) *
                     (pLo));
            v1 =
                (t2 +
                 (((__constant double *)__derfc_ha__imldErfcTab)[466 + 2 * j]) *
                     (pHi));
            expHi = t1;
            expLo = v1;
            ;
            /* expHi + expMid ~= expHi + T(j) + D(j)    */
            v1 =
                ((expHi) +
                 (((__constant double *)__derfc_ha__imldErfcTab)[465 + 2 * j]));
            v2 = ((expHi)-v1);
            v3 = (v1 + v2);
            v2 =
                ((((__constant double *)__derfc_ha__imldErfcTab)[465 + 2 * j]) +
                 v2);
            v3 = ((expHi)-v3);
            v3 = (v2 + v3);
            expHi = v1;
            expMid = v3;
            ;
            expMid +=
                ((__constant double *)__derfc_ha__imldErfcTab)[466 + 2 * j];
            /* Accumulate expLo + expMid in expLo       */
            expLo += expMid;
            /* Re-split expHi into expHi + expMid       */
            v1 = ((expHi) *
                  (((__constant double *)__derfc_ha__imldErfcTab)[598]));
            v2 = (v1 - (expHi));
            v1 = (v1 - v2);
            v2 = ((expHi)-v1);
            expHi = v1;
            expMid = v2;
            ;
            /* Accumulate expLo + expMid in expLo       */
            expLo += expMid;
            /*-------------- End of Exp section --------*/
            /* Now expHi + expLo represents not scaled  */
            /* value of exp(-|a[0]|^2)                  */
            /* Multiply polynomial P(|a[0]|) in resHi + */
            /* resLo by not scaled exp in expHi + expLo */
            /* resHi + resLo ~=                         */
            /*        (resHi + resLo) * (expHi + expLo) */
            t1 = ((resHi) * (expHi));
            t2 = ((resLo) * (expLo));
            t2 = (t2 + (resHi) * (expLo));
            v1 = (t2 + (resLo) * (expHi));
            resHi = t1;
            resLo = v1;
            ;
            /* Construct the proper scale value         */
            scale = ((__constant double *)__derfc_ha__imldErfcTab)[599];
            N = N & 0x7FF;
            (((_iml_v2_dp_union_t *)&scale)->dwords.hi_dword =
                 (((_iml_v2_dp_union_t *)&scale)->dwords.hi_dword & 0x800FFFFF) |
                 (((_iml_uint32_t)(N)&0x7FF) << 20));
          }
          /* Check if a[0] is positive */
          if (((((_iml_v2_dp_union_t *)&a[0])->dwords.hi_dword >> 31) == 1) == 0) {
            /* Here if a[0] is positive */
            /* Path 5.3) or 5.4) */
            /* Check whether the result is normalized */
            if (a[0] < ((__constant double *)__derfc_ha__imldErfcTab)[606]) {
              /* Path 5.3) */
              /* Here if erfc(a[0]) is normalized */
              res = (resHi + resLo) * scale;
            } else {
              /* Path 5.4) */
              /* Here if erfc(a[0]) gradually underflows */
              /* i.e. result is denormalized number      */
              /* Construct new scale */
              scale = ((__constant double *)__derfc_ha__imldErfcTab)[599];
              N = (N + 200) & 0x7FF;
              (((_iml_v2_dp_union_t *)&scale)->dwords.hi_dword =
                   (((_iml_v2_dp_union_t *)&scale)->dwords.hi_dword & 0x800FFFFF) |
                   (((_iml_uint32_t)(N)&0x7FF) << 20));
              /* Scale up */
              resHi *= scale;
              resLo *= scale;
              /* Check if "small" or "large"             */
              /* denormalized result path should follow  */
              if (a[0] > ((__constant double *)__derfc_ha__imldErfcTab)[605]) {
                /* Path 5.4.1) */
                /* Here if "small" denormalized result */
                /* Scaling back the result             */
                res = (resHi + resLo) *
                      ((__constant double *)__derfc_ha__imldErfcTab)[596];
                /* Raising Underflow and Inexact flags */
                /* NOTE: res*res rounds to 0.0 if in   */
                /*       round-to-nearest mode         */
                v1 = res * res;
                res += v1;
              } else {
                /* Path 5.4.2) */
                /* Here if "large" denormalized result */
                /* Accumulate the most significant     */
                /* part of the sum resHi + resLo in    */
                /* resHi. resLo will contain the       */
                /* remainder                           */
                v1 = ((resHi) + (resLo));
                t1 = ((resHi)-v1);
                v2 = (t1 + (resLo));
                resHi = v1;
                resLo = v2;
                ;
                /* Split resHi into resHi + resMid     */
                v1 = ((resHi) *
                      (((__constant double *)__derfc_ha__imldErfcTab)[598]));
                v2 = (v1 - (resHi));
                v1 = (v1 - v2);
                v2 = ((resHi)-v1);
                resHi = v1;
                resMid = v2;
                ;
                /* Accumulate resLo + resMid in resLo  */
                resLo += resMid;
                /* Scale back the two result parts     */
                v1 =
                    resHi * ((__constant double *)__derfc_ha__imldErfcTab)[596];
                v2 =
                    resLo * ((__constant double *)__derfc_ha__imldErfcTab)[596];
                /* Obtain the final result             */
                res = v1 + v2;
              }
            }
          } else {
            /* Path 5.5). Here if a[0] is negative  */
            /* Here erfc(a[0]) = 2.0 - erfc(|a[0]|) */
            /* Get resHi + resLo = -erfc(|a[0]|)    */
            resHi *= -scale;
            resLo *= -scale;
            /* sHi + sMid = 2.0 + resHi             */
            v1 = ((((__constant double *)__derfc_ha__imldErfcTab)[603]) +
                  (resHi));
            t1 = ((((__constant double *)__derfc_ha__imldErfcTab)[603]) - v1);
            v2 = (t1 + (resHi));
            sHi = v1;
            sMid = v2;
            ;
            /* sHi + sLo = sHi + sMid + resLo       */
            v1 = ((sHi) + (resLo));
            v2 = ((sHi)-v1);
            v3 = (v1 + v2);
            v2 = ((resLo) + v2);
            v3 = ((sHi)-v3);
            v3 = (v2 + v3);
            sHi = v1;
            sLo = v3;
            ;
            sLo += sMid;
            res = sHi + sLo;
          }
          r[0] = res;
        } else {
          /* Path 4) Underflow                               */
          /* Here if UNDERFLOW_THRESHOLD <= a[0] < +Infinity */
          r[0] = (((__constant double *)__derfc_ha__imldErfcTab)[600]) *
                 ((__constant double *)__derfc_ha__imldErfcTab)[600];
          nRet = 4;
          //_IML_FUNC_NAME_CALL_EM(dError,(_IML_SCODE_IN_C() =
          //  IML_STATUS_UNDERFLOW,i,a,a,r,r, _IML_THISFUNC_NAME));
        }
      } else {
        /* Path 3) Saturation                                */
        /* Here if -Infinity < a[0] <= SATURATION_THRESHOLD  */
        /* erfc(a[0]) rounds to 2.0 in round-to-nearest mode */
        r[0] = ((__constant double *)__derfc_ha__imldErfcTab)[603] -
               (((__constant double *)__derfc_ha__imldErfcTab)[600]);
      }
    } else {
      /* Path 2). Here if argument is "near zero" */
      r[0] = ((__constant double *)__derfc_ha__imldErfcTab)[601 + (0)] + a[0];
    }
  } else {
    /* Path 1). Here if argument is NaN or Infinity */
    if ((((((_iml_v2_dp_union_t *)&a[0])->dwords.hi_dword & 0x000FFFFF) == 0) &&
         ((((_iml_v2_dp_union_t *)&a[0])->dwords.lo_dword) == 0)) == 0) {
      /* Path 1.1). Here if argument is NaN */
      r[0] = a[0] * a[0];
    } else {
      /* Here if argument is [+,-]Infinity */
      if (((((_iml_v2_dp_union_t *)&a[0])->dwords.hi_dword >> 31) == 1)) {
        /* Path 1.2). Here if argument is -Infinity */
        r[0] = ((__constant double *)__derfc_ha__imldErfcTab)[603];
      } else {
        /* Path 1.3). Here if argument is +Infinity */
        r[0] = ((__constant double *)__derfc_ha__imldErfcTab)[599];
      }
    }
  }
  return nRet;
}
double __ocl_svml_erfc_ha(double x) {
  double r;
  unsigned int vm;
  double va1;
  double vr1;
  va1 = x;
  {
    double _AbsMask;
    double _MaxThreshold;
    double _SRound;
    double _ExpMask;
    double _Exp_X0_Mask;
    double _TwoM9;
    double _poly1_0;
    double _poly1_1;
    double _poly3_0;
    double _poly3_1;
    double _poly5_0;
    double _poly5_1;
    double _poly7_0;
    double _poly7_1;
    double _poly1_2;
    double _poly3_2;
    double _poly5_2;
    double _poly1_3;
    double _poly3_3;
    double _poly5_3;
    double _poly1_4;
    double _poly3_4;
    double _poly5_4;
    double _poly1_5;
    double _poly3_5;
    double _poly1_6;
    double _poly3_6;
    double _poly3_7;
    double _poly1_7;
    double _poly1_8;
    double _TwoM128;
    double _One;
    double _UF_Threshold;
    double _SplitMask;
    double X;
    double X0;
    double T;
    double Diff;
    unsigned long Index;
    double P1;
    double P3;
    double P5;
    double P07;
    double D2;
    double D3;
    double T2;
    double THL[2];
    double Erfc_E0H;
    double Exp_X0H;
    double Erfc_L;
    double Exp_X0H_Low;
    double Exp_X0H_High;
    double HighRes;
    unsigned long lErfc_E0H;
    unsigned long lExp_X0H;
    double Ph;
    double Phh;
    double LowRes;
    double Sgn;
    double _SgnMask;
    double NegConst;
    double MOne;
    double RangeMask;
    unsigned long iRangeMask;
    unsigned long _Mask32;
    _AbsMask = as_double(__ocl_svml_internal_derfc_ha_data._AbsMask);
    X = as_double((as_ulong(va1) & as_ulong(_AbsMask)));
    // erfc(27.25) underflows to 0
    // can compute all results in the main path
    _MaxThreshold = as_double(__ocl_svml_internal_derfc_ha_data._MaxThreshold);
    X = ((X < _MaxThreshold) ? X : _MaxThreshold);
    _SgnMask = as_double(__ocl_svml_internal_derfc_ha_data._SgnMask);
    _One = as_double(__ocl_svml_internal_derfc_ha_data._One);
    _TwoM128 = as_double(__ocl_svml_internal_derfc_ha_data._TwoM128);
    Sgn = as_double((as_ulong(va1) & as_ulong(_SgnMask)));
    MOne = as_double((as_ulong(_One) | as_ulong(Sgn)));
    // 2.0 if x<0, 0.0 otherwise
    NegConst = (_One - MOne);
    {
      double dIndex;
      _SRound = as_double(__ocl_svml_internal_derfc_ha_data._SRound);
      dIndex = (X + _SRound);
      X = ((X > _TwoM128) ? X : _TwoM128);
      X0 = (dIndex - _SRound);
      Diff = (X - X0);
      T = (X0 * Diff);
      Index = as_ulong(dIndex);
      Index = ((unsigned long)(Index) << (4));
    };
    // 2^(-128) with sign of input
    _TwoM128 = as_double((as_ulong(_TwoM128) | as_ulong(Sgn)));
    // Start polynomial evaluation
    _poly1_0 = as_double(__ocl_svml_internal_derfc_ha_data._poly1_0);
    _poly1_1 = as_double(__ocl_svml_internal_derfc_ha_data._poly1_1);
    P1 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(_poly1_0, T, _poly1_1);
    _poly3_0 = as_double(__ocl_svml_internal_derfc_ha_data._poly3_0);
    _poly3_1 = as_double(__ocl_svml_internal_derfc_ha_data._poly3_1);
    P3 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(_poly3_0, T, _poly3_1);
    _poly5_0 = as_double(__ocl_svml_internal_derfc_ha_data._poly5_0);
    _poly5_1 = as_double(__ocl_svml_internal_derfc_ha_data._poly5_1);
    P5 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(_poly5_0, T, _poly5_1);
    _poly7_0 = as_double(__ocl_svml_internal_derfc_ha_data._poly7_0);
    _poly7_1 = as_double(__ocl_svml_internal_derfc_ha_data._poly7_1);
    P07 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(_poly7_0, T, _poly7_1);
    _poly1_2 = as_double(__ocl_svml_internal_derfc_ha_data._poly1_2);
    P1 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(P1, T, _poly1_2);
    _poly3_2 = as_double(__ocl_svml_internal_derfc_ha_data._poly3_2);
    P3 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(P3, T, _poly3_2);
    _poly5_2 = as_double(__ocl_svml_internal_derfc_ha_data._poly5_2);
    P5 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(P5, T, _poly5_2);
    // Diff^2
    D2 = (Diff * Diff);
    _poly1_3 = as_double(__ocl_svml_internal_derfc_ha_data._poly1_3);
    P1 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(P1, T, _poly1_3);
    _poly3_3 = as_double(__ocl_svml_internal_derfc_ha_data._poly3_3);
    P3 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(P3, T, _poly3_3);
    _poly5_3 = as_double(__ocl_svml_internal_derfc_ha_data._poly5_3);
    P5 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(P5, T, _poly5_3);
    _Mask32 = (__ocl_svml_internal_derfc_ha_data._Mask32);
    Index = (Index & _Mask32);
    // vector gather: erfc_h(x0), (erfc_l(x0), 2/sqrt(pi)*exp(-x0^2))
    THL[0] = as_double(
        ((unsigned long *)((double *)(&__ocl_svml_internal_derfc_ha_data
                                           ._erfc_tbl[0])))[Index >> 3]);
    THL[1] = as_double(
        ((unsigned long *)((double *)(&__ocl_svml_internal_derfc_ha_data
                                           ._erfc_tbl[0])))[(Index >> 3) + 1]);
    _poly1_4 = as_double(__ocl_svml_internal_derfc_ha_data._poly1_4);
    P1 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(P1, T, _poly1_4);
    _poly3_4 = as_double(__ocl_svml_internal_derfc_ha_data._poly3_4);
    P3 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(P3, T, _poly3_4);
    _poly5_4 = as_double(__ocl_svml_internal_derfc_ha_data._poly5_4);
    P5 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(P5, T, _poly5_4);
    // Form Erfc_L = erfc_low(x0)/(2/sqrt(pi)*exp(-x0^2))
    _Exp_X0_Mask = as_double(__ocl_svml_internal_derfc_ha_data._Exp_X0_Mask);
    Exp_X0H = as_double((as_ulong(THL[1]) & as_ulong(_Exp_X0_Mask)));
    {
      _TwoM9 = as_double(__ocl_svml_internal_derfc_ha_data._TwoM9);
      Erfc_L = as_double(((unsigned long)as_ulong(THL[1]) >> ((64 - 9))));
      Erfc_L = as_double((as_ulong(Erfc_L) | as_ulong(_TwoM9)));
      Erfc_L = (Erfc_L - _TwoM9);
    };
    _poly1_5 = as_double(__ocl_svml_internal_derfc_ha_data._poly1_5);
    P1 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(P1, T, _poly1_5);
    _poly3_5 = as_double(__ocl_svml_internal_derfc_ha_data._poly3_5);
    P3 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(P3, T, _poly3_5);
    // P5 = P5 + D2*P07
    P5 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(D2, P07, P5);
    // form EXP_X0H = 2/sqrt(pi)*exp(-x0^2);
    _ExpMask = as_double(__ocl_svml_internal_derfc_ha_data._ExpMask);
    Erfc_E0H = as_double(
        (as_ulong(THL[0]) & as_ulong(_ExpMask))); // exponent of erfc_h
    lExp_X0H = as_ulong(Exp_X0H);
    lErfc_E0H = as_ulong(Erfc_E0H);
    lExp_X0H = (lExp_X0H + lErfc_E0H);
    Exp_X0H = as_double(lExp_X0H);
    _poly1_6 = as_double(__ocl_svml_internal_derfc_ha_data._poly1_6);
    P1 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(P1, T, _poly1_6);
    _poly3_6 = as_double(__ocl_svml_internal_derfc_ha_data._poly3_6);
    P3 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(P3, T, _poly3_6);
    _poly3_7 = as_double(__ocl_svml_internal_derfc_ha_data._poly3_7);
    _poly3_7 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(D2, P5, _poly3_7);
    // Diff^3
    D3 = (D2 * Diff);
    _poly1_7 = as_double(__ocl_svml_internal_derfc_ha_data._poly1_7);
    P1 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(P1, T, _poly1_7);
    P3 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(P3, T, _poly3_7);
    // T^2
    T2 = (T * T);
    // get high part of erfc_high(x0)-Diff*Exp_X0H
    HighRes = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(-(Diff), Exp_X0H, THL[0]);
    ;
    _poly1_8 = as_double(__ocl_svml_internal_derfc_ha_data._poly1_8);
    P1 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(P1, T, _poly1_8);
    // P3*D3 - Erfc_L
    P3 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(P3, D3, -(Erfc_L));
    // Phh
    Phh = (THL[0] - HighRes);
    // P1 = P1*T2 - T
    P1 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(P1, T2, -(T));
    // Ph = Diff*Exp_X0H - Phh
    Ph = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(Diff, Exp_X0H, -(Phh));
    ;
    // P1 = Diff*P1 + P3
    P1 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(P1, Diff, P3);
    // low part of result
    LowRes = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(P1, Exp_X0H, Ph);
    ;
    // Special arguments (for flags only)
    _UF_Threshold = as_double(__ocl_svml_internal_derfc_ha_data._UF_Threshold);
    RangeMask = as_double(
        (unsigned long)((va1 < _UF_Threshold) ? 0xffffffffffffffff : 0x0));
    HighRes = (HighRes - LowRes);
    // combine and get argument value range mask
    iRangeMask = as_ulong(RangeMask);
    vm = 0;
    vm = (iRangeMask == 0);
    vr1 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(HighRes, _TwoM128, NegConst);
  }
  if (__builtin_expect((vm) != 0, 0)) {
    double __cout_a1;
    double __cout_r1;
    ((double *)&__cout_a1)[0] = va1;
    ((double *)&__cout_r1)[0] = vr1;
    __ocl_svml_internal_derfc_ha(&__cout_a1, &__cout_r1);
    vr1 = ((double *)&__cout_r1)[0];
  }
  r = vr1;
  return r;
}

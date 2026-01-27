/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCBIF_INTRINSICS_DPAS_CL
#define IGCBIF_INTRINSICS_DPAS_CL
// This file is to be included into IGCBiF_Intrinsics.cl!

#pragma OPENCL EXTENSION cl_khr_fp16 : enable

// Names of SIMD8 dpas builtin functions are in the form:
//    __builtin_IB_idpas_<a's precision>_<b's precision>_<depth>_<repeatCount>  // experimental, non-subgroup
//    __builtin_IB_fdpas_<a's precision>_<b's precision>_<depth>_<repeatCount>  // experimental, non-subgroup
//    __builtin_IB_sub_group_idpas_<a's precision>_<b's precision>_<depth>_<repeatCount>
//    __builtin_IB_sub_group_fdpas_<a's precision>_<b's precision>_<depth>_<repeatCount>
//        Note that for fdpas, a and b must have the same precision!
//
//  retty, accty:
//     f
//  Precision:
//     idpas
//         u8/s8 : unsigned/signed 8 bits
//         u4/s4 : unsigned/signed 4 bits
//         u2/s2 : unsigned/signed 2 bits
//     fdpas
//         bf    : bfloat16
//         hf    : fp16 (half)
//  depth : 8
//  repeatCount: 1|2|4|8
//     No official support 3|5|6|7 even though igc handles these repeatCount already,
//     as other related features (types for 5/6/7, block read/write for vector3/5/6/7)
//     are not supported yet.
//
// Note that dpasw has the same format as dpas.
//


// Macro to generate signed/unsiged precisions
//   prefix : function's prefix, such as __builtin_IB_idpas
//   retty  : function's return type, also acc type.
//   aty    : type of argument a
//   bty    : type of argument b
//   abits  : the number of bits of argument a's precision
//   bbits  : the number of bits of argument b's precision
//   rcount : repeat count
//
#define DPAS_DEPTH_8(prefix, retty, aty, bty, abits, bbits, rcount) \
retty prefix##_s##abits##_s##bbits##_8_##rcount ( retty acc, aty a, bty b ) __attribute__((const)); \
retty prefix##_s##abits##_u##bbits##_8_##rcount ( retty acc, aty a, bty b ) __attribute__((const)); \
retty prefix##_u##abits##_s##bbits##_8_##rcount ( retty acc, aty a, bty b ) __attribute__((const)); \
retty prefix##_u##abits##_u##bbits##_8_##rcount ( retty acc, aty a, bty b ) __attribute__((const));

//
// WI version of dpas (suffix: <pa-bits>_<pb-bits>_<depth>_<rcount>)
//    Argument 'a' must be uniform. These functions can be used
//    in SIMD8 (preferred), SIMD16, etc.
// This is for experiment.
//
DPAS_DEPTH_8( __builtin_IB_idpas,  int,  int8,    int8,  8, 8, 1 )
DPAS_DEPTH_8( __builtin_IB_idpas,  int,  short8,  int8,  4, 8, 1 )
DPAS_DEPTH_8( __builtin_IB_idpas,  int,  char8,   int8,  2, 8, 1 )
DPAS_DEPTH_8( __builtin_IB_idpas,  int,  int8,    int4,  8, 4, 1 )
DPAS_DEPTH_8( __builtin_IB_idpas,  int,  int8,    int2,  8, 2, 1 )

// 2xint8 (double throughput, a & b are doubled in size)
DPAS_DEPTH_8( __builtin_IB_idpas,  int,  int8,    int8,  4, 4, 1 )
DPAS_DEPTH_8( __builtin_IB_idpas,  int,  short8,  int8,  2, 4, 1 )
DPAS_DEPTH_8( __builtin_IB_idpas,  int,  int8,    int4,  4, 2, 1 )
DPAS_DEPTH_8( __builtin_IB_idpas,  int,  short8,  int4,  2, 2, 1 )

float __builtin_IB_fdpas_bf_bf_8_1 (float acc, int8 a, int8 b) __attribute__((const));
float __builtin_IB_fdpas_hf_hf_8_1 (float acc, int8 a, int8 b) __attribute__((const));

//
// Sub group version of dpas. (suffix: <pa-bits>_<pb-bits>_<depth>_<rcount>)
//    The reason for naming them with "sub_group" is because that each work-item
//    needs to read data from other simd lanes. The single work-item's data alone
//    is not enough to do computation.
//
//    Note that these functions work for SIMD8 only.
//
DPAS_DEPTH_8( __builtin_IB_sub_group_idpas,  int,   int,     int8,  8, 8, 1 )
DPAS_DEPTH_8( __builtin_IB_sub_group_idpas,  int,   short,   int8,  4, 8, 1 )
DPAS_DEPTH_8( __builtin_IB_sub_group_idpas,  int,   char,    int8,  2, 8, 1 )
DPAS_DEPTH_8( __builtin_IB_sub_group_idpas,  int,   int,     int4,  8, 4, 1 )
DPAS_DEPTH_8( __builtin_IB_sub_group_idpas,  int,   int,     int2,  8, 2, 1 )

DPAS_DEPTH_8( __builtin_IB_sub_group_idpas,  int2,  int2,    int8,  8, 8, 2 )
DPAS_DEPTH_8( __builtin_IB_sub_group_idpas,  int2,  short2,  int8,  4, 8, 2 )
DPAS_DEPTH_8( __builtin_IB_sub_group_idpas,  int2,  char2,   int8,  2, 8, 2 )
DPAS_DEPTH_8( __builtin_IB_sub_group_idpas,  int2,  int2,    int4,  8, 4, 2 )
DPAS_DEPTH_8( __builtin_IB_sub_group_idpas,  int2,  int2,    int2,  8, 2, 2 )

DPAS_DEPTH_8( __builtin_IB_sub_group_idpas,  int4,  int4,    int8,  8, 8, 4 )
DPAS_DEPTH_8( __builtin_IB_sub_group_idpas,  int4,  short4,  int8,  4, 8, 4 )
DPAS_DEPTH_8( __builtin_IB_sub_group_idpas,  int4,  char4,   int8,  2, 8, 4 )
DPAS_DEPTH_8( __builtin_IB_sub_group_idpas,  int4,  int4,    int4,  8, 4, 4 )
DPAS_DEPTH_8( __builtin_IB_sub_group_idpas,  int4,  int4,    int2,  8, 2, 4 )

DPAS_DEPTH_8( __builtin_IB_sub_group_idpas,  int8,  int8,    int8,  8, 8, 8 )
DPAS_DEPTH_8( __builtin_IB_sub_group_idpas,  int8,  short8,  int8,  4, 8, 8 )
DPAS_DEPTH_8( __builtin_IB_sub_group_idpas,  int8,  char8,   int8,  2, 8, 8 )
DPAS_DEPTH_8( __builtin_IB_sub_group_idpas,  int8,  int8,    int4,  8, 4, 8 )
DPAS_DEPTH_8( __builtin_IB_sub_group_idpas,  int8,  int8,    int2,  8, 2, 8 )

// 2xint8 (double throughput, a & b are doubled in size)
DPAS_DEPTH_8( __builtin_IB_sub_group_idpas,  int,   int,     int8,  4, 4, 1 )
DPAS_DEPTH_8( __builtin_IB_sub_group_idpas,  int,   short,   int8,  2, 4, 1 )
DPAS_DEPTH_8( __builtin_IB_sub_group_idpas,  int,   int,     int4,  4, 2, 1 )
DPAS_DEPTH_8( __builtin_IB_sub_group_idpas,  int,   short,   int4,  2, 2, 1 )

DPAS_DEPTH_8( __builtin_IB_sub_group_idpas,  int2,  int2,    int8,  4, 4, 2 )
DPAS_DEPTH_8( __builtin_IB_sub_group_idpas,  int2,  short2,  int8,  2, 4, 2 )
DPAS_DEPTH_8( __builtin_IB_sub_group_idpas,  int2,  int2,    int4,  4, 2, 2 )
DPAS_DEPTH_8( __builtin_IB_sub_group_idpas,  int2,  short2,  int4,  2, 2, 2 )

DPAS_DEPTH_8( __builtin_IB_sub_group_idpas,  int4,  int4,    int8,  4, 4, 4 )
DPAS_DEPTH_8( __builtin_IB_sub_group_idpas,  int4,  short4,  int8,  2, 4, 4 )
DPAS_DEPTH_8( __builtin_IB_sub_group_idpas,  int4,  int4,    int4,  4, 2, 4 )
DPAS_DEPTH_8( __builtin_IB_sub_group_idpas,  int4,  short4,  int4,  2, 2, 4 )

DPAS_DEPTH_8( __builtin_IB_sub_group_idpas,  int8,  int8,    int8,  4, 4, 8 )
DPAS_DEPTH_8( __builtin_IB_sub_group_idpas,  int8,  short8,  int8,  2, 4, 8 )
DPAS_DEPTH_8( __builtin_IB_sub_group_idpas,  int8,  int8,    int4,  4, 2, 8 )
DPAS_DEPTH_8( __builtin_IB_sub_group_idpas,  int8,  short8,  int4,  2, 2, 8 )

// bfloat16 (deprecated)
float  __builtin_IB_sub_group_fdpas_8_1 (float  acc, int  a, int8 b) __attribute__((const)); // deprecated
float2 __builtin_IB_sub_group_fdpas_8_2 (float2 acc, int2 a, int8 b) __attribute__((const)); // deprecated
float4 __builtin_IB_sub_group_fdpas_8_4 (float4 acc, int4 a, int8 b) __attribute__((const)); // deprecated
float8 __builtin_IB_sub_group_fdpas_8_8 (float8 acc, int8 a, int8 b) __attribute__((const)); // deprecated

// bfloat16
float  __builtin_IB_sub_group_fdpas_bf_bf_8_1 (float  acc, int  a, int8 b) __attribute__((const));
float2 __builtin_IB_sub_group_fdpas_bf_bf_8_2 (float2 acc, int2 a, int8 b) __attribute__((const));
float4 __builtin_IB_sub_group_fdpas_bf_bf_8_4 (float4 acc, int4 a, int8 b) __attribute__((const));
float8 __builtin_IB_sub_group_fdpas_bf_bf_8_8 (float8 acc, int8 a, int8 b) __attribute__((const));

// half
float  __builtin_IB_sub_group_fdpas_hf_hf_8_1 (float  acc, int  a, int8 b) __attribute__((const));
float2 __builtin_IB_sub_group_fdpas_hf_hf_8_2 (float2 acc, int2 a, int8 b) __attribute__((const));
float4 __builtin_IB_sub_group_fdpas_hf_hf_8_4 (float4 acc, int4 a, int8 b) __attribute__((const));
float8 __builtin_IB_sub_group_fdpas_hf_hf_8_8 (float8 acc, int8 a, int8 b) __attribute__((const));

//
// dpasw: 'a' size is the half of the dpas version.
//    Not all combination of precisions are supported. For a dpas builin in which 'a'
//    is either 2, 4, 8 GRFs (simd8), its corresponding dpasw is supported. In this way,
//    'a' is fetched evenly from both EU0 and EU1.
//
//    The following shows what are supported and what are not supported:
//      1. r = 1 : no builtin supported.
//      2. r = 2 : (int8)   8-bit precision of 'a', and
//                 (2xint8) 4-bit precision of 'a'.
//      3. r = 4 : all combinations of a and b except a=2-bit and b=8-bit.
//      4. r = 8 : all combinations
//
DPAS_DEPTH_8( __builtin_IB_sub_group_idpasw,  int2,  int,     int8,  8, 8, 2 )
DPAS_DEPTH_8( __builtin_IB_sub_group_idpasw,  int2,  int,     int4,  8, 4, 2 )
DPAS_DEPTH_8( __builtin_IB_sub_group_idpasw,  int2,  int,     int2,  8, 2, 2 )

DPAS_DEPTH_8( __builtin_IB_sub_group_idpasw,  int4,  int2,    int8,  8, 8, 4 )
DPAS_DEPTH_8( __builtin_IB_sub_group_idpasw,  int4,  short2,  int8,  4, 8, 4 )
DPAS_DEPTH_8( __builtin_IB_sub_group_idpasw,  int4,  int2,    int4,  8, 4, 4 )
DPAS_DEPTH_8( __builtin_IB_sub_group_idpasw,  int4,  int2,    int2,  8, 2, 4 )

DPAS_DEPTH_8( __builtin_IB_sub_group_idpasw,  int8,  int4,    int8,  8, 8, 8 )
DPAS_DEPTH_8( __builtin_IB_sub_group_idpasw,  int8,  short4,  int8,  4, 8, 8 )
DPAS_DEPTH_8( __builtin_IB_sub_group_idpasw,  int8,  char4,   int8,  2, 8, 8 )
DPAS_DEPTH_8( __builtin_IB_sub_group_idpasw,  int8,  int4,    int4,  8, 4, 8 )
DPAS_DEPTH_8( __builtin_IB_sub_group_idpasw,  int8,  int4,    int2,  8, 2, 8 )

// 2xint8 (double throughput)
DPAS_DEPTH_8( __builtin_IB_sub_group_idpasw,  int2,  int,     int8,  4, 4, 2 )
DPAS_DEPTH_8( __builtin_IB_sub_group_idpasw,  int2,  int,     int4,  4, 2, 2 )

DPAS_DEPTH_8( __builtin_IB_sub_group_idpasw,  int4,  int2,    int8,  4, 4, 4 )
DPAS_DEPTH_8( __builtin_IB_sub_group_idpasw,  int4,  short2,  int8,  2, 4, 4 )
DPAS_DEPTH_8( __builtin_IB_sub_group_idpasw,  int4,  int2,    int4,  4, 2, 4 )
DPAS_DEPTH_8( __builtin_IB_sub_group_idpasw,  int4,  short2,  int4,  2, 2, 4 )

DPAS_DEPTH_8( __builtin_IB_sub_group_idpasw,  int8,  int4,    int8,  4, 4, 8 )
DPAS_DEPTH_8( __builtin_IB_sub_group_idpasw,  int8,  short4,  int8,  2, 4, 8 )
DPAS_DEPTH_8( __builtin_IB_sub_group_idpasw,  int8,  int4,    int4,  4, 2, 8 )
DPAS_DEPTH_8( __builtin_IB_sub_group_idpasw,  int8,  short4,  int4,  2, 2, 8 )

// bfloat16
float2 __builtin_IB_sub_group_fdpasw_bf_bf_8_2 (float2 acc, int  a, int8 b) __attribute__((const));
float4 __builtin_IB_sub_group_fdpasw_bf_bf_8_4 (float4 acc, int2 a, int8 b) __attribute__((const));
float8 __builtin_IB_sub_group_fdpasw_bf_bf_8_8 (float8 acc, int4 a, int8 b) __attribute__((const));

// half
float2 __builtin_IB_sub_group_fdpasw_hf_hf_8_2 (float2 acc, int  a, int8 b) __attribute__((const));
float4 __builtin_IB_sub_group_fdpasw_hf_hf_8_4 (float4 acc, int2 a, int8 b) __attribute__((const));
float8 __builtin_IB_sub_group_fdpasw_hf_hf_8_8 (float8 acc, int4 a, int8 b) __attribute__((const));

// Names of SIMD16 dpas builtin functions are in the form:
//    __builtin_IB_sub_group16_idpas_<a's precision>_<b's precision>_<depth>_<repeatCount>
//    __builtin_IB_sub_group16_fdpas_<retty>_<accty>_<a's precision>_<b's precision>_<depth>_<repeatCount>
//        Note that for fdpas, a and b must have the same precision!
//
// In addition to the operand types supported on XeHP_SDV, PVC has supported additional types. here are all
// supported types:
//
// retty, accty:
//    f, bf, hf
//
//  Precision:
//     idpas
//         u8/s8 : unsigned/signed 8 bits
//         u4/s4 : unsigned/signed 4 bits
//         u2/s2 : unsigned/signed 2 bits
//     fdpas
//         bf    : bfloat16
//         hf    : fp16 (half)
//         bf8   : bfloat8
//         tf32  : tensorFloat
//  depth : 8
//  repeatCount: 1|2|4|8
//     No official support 3|5|6|7 even though igc handles these repeatCount already,
//     as other related features (types for 5/6/7, block read/write for vector3/5/6/7)
//     are not supported yet.
//
//   SIMD16 DPAS, the base type of 'a' will be halfed for each work-item compared
//   with XeHP_SDV SIMD8 dpas.  Thus, a's type is changed from int to short,  short to
//   char.  As there is no 4-bit integer type,  SIMD16 version of simd8 dpas with char
//   as a's base type will not be provided, thus no support via intrinsics for now. Those
//   are for src1-src2 pair [2-bit, 8-bit precision] only.
//
//   Also, the intrinsic is not overloaded, substring "sub_group16" is used to
//   distinguish with XeHP_SDV's simd8 intrinsic "sub_group". And for fdpas, the
//   return type and acc's type are encoded in the names. See format defined
//   right before XeHP_SDV intrinsics in this file.
//
DPAS_DEPTH_8( __builtin_IB_sub_group16_idpas,  int,   short,   int8,  8, 8, 1 )
DPAS_DEPTH_8( __builtin_IB_sub_group16_idpas,  int,   char,    int8,  4, 8, 1 )
DPAS_DEPTH_8( __builtin_IB_sub_group16_idpas,  int,   short,   int4,  8, 4, 1 )
DPAS_DEPTH_8( __builtin_IB_sub_group16_idpas,  int,   short,   int2,  8, 2, 1 )

DPAS_DEPTH_8( __builtin_IB_sub_group16_idpas,  int2,  short2,  int8,  8, 8, 2 )
DPAS_DEPTH_8( __builtin_IB_sub_group16_idpas,  int2,  char2,   int8,  4, 8, 2 )
DPAS_DEPTH_8( __builtin_IB_sub_group16_idpas,  int2,  short2,  int4,  8, 4, 2 )
DPAS_DEPTH_8( __builtin_IB_sub_group16_idpas,  int2,  short2,  int2,  8, 2, 2 )

DPAS_DEPTH_8( __builtin_IB_sub_group16_idpas,  int4,  short4,  int8,  8, 8, 4 )
DPAS_DEPTH_8( __builtin_IB_sub_group16_idpas,  int4,  char4,   int8,  4, 8, 4 )
DPAS_DEPTH_8( __builtin_IB_sub_group16_idpas,  int4,  short4,  int4,  8, 4, 4 )
DPAS_DEPTH_8( __builtin_IB_sub_group16_idpas,  int4,  short4,  int2,  8, 2, 4 )

DPAS_DEPTH_8( __builtin_IB_sub_group16_idpas,  int8,  short8,  int8,  8, 8, 8 )
DPAS_DEPTH_8( __builtin_IB_sub_group16_idpas,  int8,  char8,   int8,  4, 8, 8 )
DPAS_DEPTH_8( __builtin_IB_sub_group16_idpas,  int8,  short8,  int4,  8, 4, 8 )
DPAS_DEPTH_8( __builtin_IB_sub_group16_idpas,  int8,  short8,  int2,  8, 2, 8 )

// 2xint8 (double throughput, a & b are doubled in size)
DPAS_DEPTH_8( __builtin_IB_sub_group16_idpas,  int,   short,   int8,  4, 4, 1 )
DPAS_DEPTH_8( __builtin_IB_sub_group16_idpas,  int,   char,    int8,  2, 4, 1 )
DPAS_DEPTH_8( __builtin_IB_sub_group16_idpas,  int,   short,   int4,  4, 2, 1 )
DPAS_DEPTH_8( __builtin_IB_sub_group16_idpas,  int,   char,    int4,  2, 2, 1 )

DPAS_DEPTH_8( __builtin_IB_sub_group16_idpas,  int2,  short2,  int8,  4, 4, 2 )
DPAS_DEPTH_8( __builtin_IB_sub_group16_idpas,  int2,  char2,   int8,  2, 4, 2 )
DPAS_DEPTH_8( __builtin_IB_sub_group16_idpas,  int2,  short2,  int4,  4, 2, 2 )
DPAS_DEPTH_8( __builtin_IB_sub_group16_idpas,  int2,  char2,   int4,  2, 2, 2 )

DPAS_DEPTH_8( __builtin_IB_sub_group16_idpas,  int4,  short4,  int8,  4, 4, 4 )
DPAS_DEPTH_8( __builtin_IB_sub_group16_idpas,  int4,  char4,   int8,  2, 4, 4 )
DPAS_DEPTH_8( __builtin_IB_sub_group16_idpas,  int4,  short4,  int4,  4, 2, 4 )
DPAS_DEPTH_8( __builtin_IB_sub_group16_idpas,  int4,  char4,   int4,  2, 2, 4 )

DPAS_DEPTH_8( __builtin_IB_sub_group16_idpas,  int8,  short8,  int8,  4, 4, 8 )
DPAS_DEPTH_8( __builtin_IB_sub_group16_idpas,  int8,  char8,   int8,  2, 4, 8 )
DPAS_DEPTH_8( __builtin_IB_sub_group16_idpas,  int8,  short8,  int4,  4, 2, 8 )
DPAS_DEPTH_8( __builtin_IB_sub_group16_idpas,  int8,  char8,   int4,  2, 2, 8 )

//
//  float/bfloat sub_group dpas naming convention (PVC, simd16 only)
//    __builtin_IB_sub_group16_fdpas_<rty>_<accty>_<aty>_<bty>_<depth>_<rcount>
//        rty:  return base type:  f (float32), hf(half), bf(bfloat)
//      accty:  acc's base type:   f (float32), hf(half), bf(bfloat)
//    aty/bty:  a/b's base type:  bf (bfloat),  hf(half)
//                                bf8, tf32(tensorFloat)
//
//  Note that as OCL has no bfloat type, use short instead.
//

// bfloat16, rcount = 1, simd16
float  __builtin_IB_sub_group16_fdpas_f_f_bf_bf_8_1   (float  acc, short  a, int8 b) __attribute__((const));
short  __builtin_IB_sub_group16_fdpas_bf_f_bf_bf_8_1  (float  acc, short  a, int8 b) __attribute__((const));
float  __builtin_IB_sub_group16_fdpas_f_bf_bf_bf_8_1  (short  acc, short  a, int8 b) __attribute__((const));
short  __builtin_IB_sub_group16_fdpas_bf_bf_bf_bf_8_1 (short  acc, short  a, int8 b) __attribute__((const));

// bfloat16, rcount = 2, simd16
float2  __builtin_IB_sub_group16_fdpas_f_f_bf_bf_8_2   (float2  acc, short2  a, int8 b) __attribute__((const));
short2  __builtin_IB_sub_group16_fdpas_bf_f_bf_bf_8_2  (float2  acc, short2  a, int8 b) __attribute__((const));
float2  __builtin_IB_sub_group16_fdpas_f_bf_bf_bf_8_2  (short2  acc, short2  a, int8 b) __attribute__((const));
short2  __builtin_IB_sub_group16_fdpas_bf_bf_bf_bf_8_2 (short2  acc, short2  a, int8 b) __attribute__((const));

// bfloat16, rcount = 4, simd16
float4  __builtin_IB_sub_group16_fdpas_f_f_bf_bf_8_4   (float4  acc, short4  a, int8 b) __attribute__((const));
short4  __builtin_IB_sub_group16_fdpas_bf_f_bf_bf_8_4  (float4  acc, short4  a, int8 b) __attribute__((const));
float4  __builtin_IB_sub_group16_fdpas_f_bf_bf_bf_8_4  (short4  acc, short4  a, int8 b) __attribute__((const));
short4  __builtin_IB_sub_group16_fdpas_bf_bf_bf_bf_8_4 (short4  acc, short4  a, int8 b) __attribute__((const));

// bfloat16, rcount = 8, simd16
float8  __builtin_IB_sub_group16_fdpas_f_f_bf_bf_8_8   (float8  acc, short8  a, int8 b) __attribute__((const));
short8  __builtin_IB_sub_group16_fdpas_bf_f_bf_bf_8_8  (float8  acc, short8  a, int8 b) __attribute__((const));
float8  __builtin_IB_sub_group16_fdpas_f_bf_bf_bf_8_8  (short8  acc, short8  a, int8 b) __attribute__((const));
short8  __builtin_IB_sub_group16_fdpas_bf_bf_bf_bf_8_8 (short8  acc, short8  a, int8 b) __attribute__((const));

// half, rcount = 1, simd16
float  __builtin_IB_sub_group16_fdpas_f_f_hf_hf_8_1   (float acc,  short  a, int8 b) __attribute__((const));
half   __builtin_IB_sub_group16_fdpas_hf_f_hf_hf_8_1  (float acc,  short  a, int8 b) __attribute__((const));
float  __builtin_IB_sub_group16_fdpas_f_hf_hf_hf_8_1  (half  acc,  short  a, int8 b) __attribute__((const));
half   __builtin_IB_sub_group16_fdpas_hf_hf_hf_hf_8_1 (half  acc,  short  a, int8 b) __attribute__((const));

// half, rcount = 2, simd16
float2  __builtin_IB_sub_group16_fdpas_f_f_hf_hf_8_2   (float2 acc,  short2  a, int8 b) __attribute__((const));
half2   __builtin_IB_sub_group16_fdpas_hf_f_hf_hf_8_2  (float2 acc,  short2  a, int8 b) __attribute__((const));
float2  __builtin_IB_sub_group16_fdpas_f_hf_hf_hf_8_2  (half2  acc,  short2  a, int8 b) __attribute__((const));
half2   __builtin_IB_sub_group16_fdpas_hf_hf_hf_hf_8_2 (half2  acc,  short2  a, int8 b) __attribute__((const));

// half, rcount = 4, simd16
float4  __builtin_IB_sub_group16_fdpas_f_f_hf_hf_8_4   (float4 acc,  short4  a, int8 b) __attribute__((const));
half4   __builtin_IB_sub_group16_fdpas_hf_f_hf_hf_8_4  (float4 acc,  short4  a, int8 b) __attribute__((const));
float4  __builtin_IB_sub_group16_fdpas_f_hf_hf_hf_8_4  (half4  acc,  short4  a, int8 b) __attribute__((const));
half4   __builtin_IB_sub_group16_fdpas_hf_hf_hf_hf_8_4 (half4  acc,  short4  a, int8 b) __attribute__((const));

// half, rcount = 8, simd16
float8  __builtin_IB_sub_group16_fdpas_f_f_hf_hf_8_8   (float8 acc,  short8  a, int8 b) __attribute__((const));
half8   __builtin_IB_sub_group16_fdpas_hf_f_hf_hf_8_8  (float8 acc,  short8  a, int8 b) __attribute__((const));
float8  __builtin_IB_sub_group16_fdpas_f_hf_hf_hf_8_8  (half8  acc,  short8  a, int8 b) __attribute__((const));
half8   __builtin_IB_sub_group16_fdpas_hf_hf_hf_hf_8_8 (half8  acc,  short8  a, int8 b) __attribute__((const));

//
// Only F dst/acc are supported. All combinations of BF8/HF8 in a and b are supported.
//
// bf8, rcount = 1, simd16
float   __builtin_IB_sub_group16_fdpas_f_f_bf8_bf8_8_1   (float  acc,  short  a, int8 b) __attribute__((const));
float   __builtin_IB_sub_group16_fdpas_f_f_bf8_hf8_8_1   (float  acc,  short  a, int8 b) __attribute__((const));
float   __builtin_IB_sub_group16_fdpas_f_f_hf8_bf8_8_1   (float  acc,  short  a, int8 b) __attribute__((const));
float   __builtin_IB_sub_group16_fdpas_f_f_hf8_hf8_8_1   (float  acc,  short  a, int8 b) __attribute__((const));

// bf8, rcount = 2, simd16
float2  __builtin_IB_sub_group16_fdpas_f_f_bf8_bf8_8_2   (float2 acc,  short2 a, int8 b) __attribute__((const));
float2  __builtin_IB_sub_group16_fdpas_f_f_bf8_hf8_8_2   (float2 acc,  short2 a, int8 b) __attribute__((const));
float2  __builtin_IB_sub_group16_fdpas_f_f_hf8_bf8_8_2   (float2 acc,  short2 a, int8 b) __attribute__((const));
float2  __builtin_IB_sub_group16_fdpas_f_f_hf8_hf8_8_2   (float2 acc,  short2 a, int8 b) __attribute__((const));

// bf8, rcount = 4, simd16
float4  __builtin_IB_sub_group16_fdpas_f_f_bf8_bf8_8_4   (float4 acc,  short4 a, int8 b) __attribute__((const));
float4  __builtin_IB_sub_group16_fdpas_f_f_bf8_hf8_8_4   (float4 acc,  short4 a, int8 b) __attribute__((const));
float4  __builtin_IB_sub_group16_fdpas_f_f_hf8_bf8_8_4   (float4 acc,  short4 a, int8 b) __attribute__((const));
float4  __builtin_IB_sub_group16_fdpas_f_f_hf8_hf8_8_4   (float4 acc,  short4 a, int8 b) __attribute__((const));

// bf8, rcount = 8, simd16
float8  __builtin_IB_sub_group16_fdpas_f_f_bf8_bf8_8_8   (float8 acc,  short8 a, int8 b) __attribute__((const));
float8  __builtin_IB_sub_group16_fdpas_f_f_bf8_hf8_8_8   (float8 acc,  short8 a, int8 b) __attribute__((const));
float8  __builtin_IB_sub_group16_fdpas_f_f_hf8_bf8_8_8   (float8 acc,  short8 a, int8 b) __attribute__((const));
float8  __builtin_IB_sub_group16_fdpas_f_f_hf8_hf8_8_8   (float8 acc,  short8 a, int8 b) __attribute__((const));

// 8
float8  __builtin_IB_sub_group16_fdpas_f_f_e2m1_e2m1_8_8(float8 acc, short8 a, int8 b) __attribute__((const));
short8  __builtin_IB_sub_group16_fdpas_bf_bf_e2m1_e2m1_8_8(short8 acc, short8 a, int8 b) __attribute__((const));
float8  __builtin_IB_sub_group16_fdpas_f_bf_e2m1_e2m1_8_8(short8 acc, short8 a, int8 b) __attribute__((const));
short8  __builtin_IB_sub_group16_fdpas_bf_f_e2m1_e2m1_8_8(float8 acc, short8 a, int8 b) __attribute__((const));
// 4
float4  __builtin_IB_sub_group16_fdpas_f_f_e2m1_e2m1_8_4(float4 acc, short4 a, int8 b) __attribute__((const));
short4  __builtin_IB_sub_group16_fdpas_bf_bf_e2m1_e2m1_8_4(short4 acc, short4 a, int8 b) __attribute__((const));
float4  __builtin_IB_sub_group16_fdpas_f_bf_e2m1_e2m1_8_4(short4 acc, short4 a, int8 b) __attribute__((const));
short4  __builtin_IB_sub_group16_fdpas_bf_f_e2m1_e2m1_8_4(float4 acc, short4 a, int8 b) __attribute__((const));
// 2
float2  __builtin_IB_sub_group16_fdpas_f_f_e2m1_e2m1_8_2(float2 acc, short2 a, int8 b) __attribute__((const));
short2  __builtin_IB_sub_group16_fdpas_bf_bf_e2m1_e2m1_8_2(short2 acc, short2 a, int8 b) __attribute__((const));
float2  __builtin_IB_sub_group16_fdpas_f_bf_e2m1_e2m1_8_2(short2 acc, short2 a, int8 b) __attribute__((const));
short2  __builtin_IB_sub_group16_fdpas_bf_f_e2m1_e2m1_8_2(float2 acc, short2 a, int8 b) __attribute__((const));
// 1
float  __builtin_IB_sub_group16_fdpas_f_f_e2m1_e2m1_8_1(float acc, short a, int8 b) __attribute__((const));
short  __builtin_IB_sub_group16_fdpas_bf_bf_e2m1_e2m1_8_1(short acc, short a, int8 b) __attribute__((const));
float  __builtin_IB_sub_group16_fdpas_f_bf_e2m1_e2m1_8_1(short acc, short a, int8 b) __attribute__((const));
short  __builtin_IB_sub_group16_fdpas_bf_f_e2m1_e2m1_8_1(float acc, short a, int8 b) __attribute__((const));


// tf32, rcount = 1, simd16
float   __builtin_IB_sub_group16_fdpas_f_f_tf32_tf32_8_1  (float  acc, float  a, float8 b) __attribute__((const));

// tf32, rcount = 2, simd16
float2  __builtin_IB_sub_group16_fdpas_f_f_tf32_tf32_8_2  (float2 acc, float  a, float8 b) __attribute__((const));

// tf32, rcount = 4, simd16
float4  __builtin_IB_sub_group16_fdpas_f_f_tf32_tf32_8_4  (float4 acc, float2 a, float8 b) __attribute__((const));

// tf32, rcount = 8, simd16
float8  __builtin_IB_sub_group16_fdpas_f_f_tf32_tf32_8_8  (float8 acc, float4 a, float8 b) __attribute__((const));


//
// Pure bloat16/half float DPAS (acc and return type are also bloat16/half)
short __builtin_IB_bfdpas_bf_bf_8_1 (short acc, int8 a, int8 b) __attribute__((const));  // deprecated
half  __builtin_IB_hfdpas_hf_hf_8_1 (half acc, int8 a, int8 b) __attribute__((const));   // deprecated
// pure sub group version of fdpas. (suffix: <pa-bits>_<pb-bits>_<depth>_<rcount>)
// pure bfloat16  -- deprecated
short  __builtin_IB_sub_group_bfdpas_bf_bf_8_1 (short  acc, int  a, int8 b) __attribute__((const));
short2 __builtin_IB_sub_group_bfdpas_bf_bf_8_2 (short2 acc, int2 a, int8 b) __attribute__((const));
short4 __builtin_IB_sub_group_bfdpas_bf_bf_8_4 (short4 acc, int4 a, int8 b) __attribute__((const));
short8 __builtin_IB_sub_group_bfdpas_bf_bf_8_8 (short8 acc, int8 a, int8 b) __attribute__((const));
// pure half -- deprecated
half  __builtin_IB_sub_group_hfdpas_hf_hf_8_1 (half  acc, int  a, int8 b) __attribute__((const));
half2 __builtin_IB_sub_group_hfdpas_hf_hf_8_2 (half2 acc, int2 a, int8 b) __attribute__((const));
half4 __builtin_IB_sub_group_hfdpas_hf_hf_8_4 (half4 acc, int4 a, int8 b) __attribute__((const));
half8 __builtin_IB_sub_group_hfdpas_hf_hf_8_8 (half8 acc, int8 a, int8 b) __attribute__((const));

// bf16 precision, f32/bf16 acc
float8  __builtin_IB_sub_group16_bdpas_f_f_bf_bf_8_8(float8 acc, short8 a, int8 b, uchar scale_a, uchar scale_b) __attribute__((const));
short8  __builtin_IB_sub_group16_bdpas_bf_bf_bf_bf_8_8(short8 acc, short8 a, int8 b, uchar scale_a, uchar scale_b) __attribute__((const));
float8  __builtin_IB_sub_group16_bdpas_f_bf_bf_bf_8_8(short8 acc, short8 a, int8 b, uchar scale_a, uchar scale_b) __attribute__((const));
short8  __builtin_IB_sub_group16_bdpas_bf_f_bf_bf_8_8(float8 acc, short8 a, int8 b, uchar scale_a, uchar scale_b) __attribute__((const));

// f16 precision, f32/f16 acc
float8  __builtin_IB_sub_group16_bdpas_f_f_hf_hf_8_8(float8 acc, short8 a, int8 b, uchar scale_a, uchar scale_b) __attribute__((const));
half8  __builtin_IB_sub_group16_bdpas_hf_hf_hf_hf_8_8(half8 acc, short8 a, int8 b, uchar scale_a, uchar scale_b) __attribute__((const));
float8  __builtin_IB_sub_group16_bdpas_f_hf_hf_hf_8_8(half8 acc, short8 a, int8 b, uchar scale_a, uchar scale_b) __attribute__((const));
half8  __builtin_IB_sub_group16_bdpas_hf_f_hf_hf_8_8(float8 acc, short8 a, int8 b, uchar scale_a, uchar scale_b) __attribute__((const));

// bf8/hf8 precision, f32/bf16 acc
float8  __builtin_IB_sub_group16_bdpas_f_f_hf8_hf8_8_8(float8 acc, short8 a, int8 b, uchar scale_a, uchar scale_b) __attribute__((const));
short8  __builtin_IB_sub_group16_bdpas_bf_bf_hf8_hf8_8_8(short8 acc, short8 a, int8 b, uchar scale_a, uchar scale_b) __attribute__((const));
float8  __builtin_IB_sub_group16_bdpas_f_bf_hf8_hf8_8_8(short8 acc, short8 a, int8 b, uchar scale_a, uchar scale_b) __attribute__((const));
short8  __builtin_IB_sub_group16_bdpas_bf_f_hf8_hf8_8_8(float8 acc, short8 a, int8 b, uchar scale_a, uchar scale_b) __attribute__((const));
float8  __builtin_IB_sub_group16_bdpas_f_f_bf8_hf8_8_8(float8 acc, short8 a, int8 b, uchar scale_a, uchar scale_b) __attribute__((const));
short8  __builtin_IB_sub_group16_bdpas_bf_bf_bf8_hf8_8_8(short8 acc, short8 a, int8 b, uchar scale_a, uchar scale_b) __attribute__((const));
float8  __builtin_IB_sub_group16_bdpas_f_bf_bf8_hf8_8_8(short8 acc, short8 a, int8 b, uchar scale_a, uchar scale_b) __attribute__((const));
short8  __builtin_IB_sub_group16_bdpas_bf_f_bf8_hf8_8_8(float8 acc, short8 a, int8 b, uchar scale_a, uchar scale_b) __attribute__((const));
float8  __builtin_IB_sub_group16_bdpas_f_f_hf8_bf8_8_8(float8 acc, short8 a, int8 b, uchar scale_a, uchar scale_b) __attribute__((const));
short8  __builtin_IB_sub_group16_bdpas_bf_bf_hf8_bf8_8_8(short8 acc, short8 a, int8 b, uchar scale_a, uchar scale_b) __attribute__((const));
float8  __builtin_IB_sub_group16_bdpas_f_bf_hf8_bf8_8_8(short8 acc, short8 a, int8 b, uchar scale_a, uchar scale_b) __attribute__((const));
short8  __builtin_IB_sub_group16_bdpas_bf_f_hf8_bf8_8_8(float8 acc, short8 a, int8 b, uchar scale_a, uchar scale_b) __attribute__((const));
float8  __builtin_IB_sub_group16_bdpas_f_f_bf8_bf8_8_8(float8 acc, short8 a, int8 b, uchar scale_a, uchar scale_b) __attribute__((const));
short8  __builtin_IB_sub_group16_bdpas_bf_bf_bf8_bf8_8_8(short8 acc, short8 a, int8 b, uchar scale_a, uchar scale_b) __attribute__((const));
float8  __builtin_IB_sub_group16_bdpas_f_bf_bf8_bf8_8_8(short8 acc, short8 a, int8 b, uchar scale_a, uchar scale_b) __attribute__((const));
short8  __builtin_IB_sub_group16_bdpas_bf_f_bf8_bf8_8_8(float8 acc, short8 a, int8 b, uchar scale_a, uchar scale_b) __attribute__((const));
// fp4 precision, f32/bf16 acc
float8  __builtin_IB_sub_group16_bdpas_f_f_e2m1_e2m1_8_8(float8 acc, short8 a, int8 b, uchar2 scale_a, uchar2 scale_b) __attribute__((const));
short8  __builtin_IB_sub_group16_bdpas_bf_bf_e2m1_e2m1_8_8(short8 acc, short8 a, int8 b, uchar2 scale_a, uchar2 scale_b) __attribute__((const));
float8  __builtin_IB_sub_group16_bdpas_f_bf_e2m1_e2m1_8_8(short8 acc, short8 a, int8 b, uchar2 scale_a, uchar2 scale_b) __attribute__((const));
short8  __builtin_IB_sub_group16_bdpas_bf_f_e2m1_e2m1_8_8(float8 acc, short8 a, int8 b, uchar2 scale_a, uchar2 scale_b) __attribute__((const));

// bf <--> float conversion
//    bf : no igc type for bf yet. Use short as *opaque* type for it.
//
// float -> bf conversion builtins (rte rounding mode)
short   __builtin_IB_ftobf_1 (float   a) __attribute__((const));
short2  __builtin_IB_ftobf_2 (float2  a) __attribute__((const));
short3  __builtin_IB_ftobf_3 (float3  a) __attribute__((const));
short4  __builtin_IB_ftobf_4 (float4  a) __attribute__((const));
short8  __builtin_IB_ftobf_8 (float8  a) __attribute__((const));
short16 __builtin_IB_ftobf_16(float16 a) __attribute__((const));

// bf -> float conversion builtins (precise conversion)
float   __builtin_IB_bftof_1 (short   a) __attribute__((const));
float2  __builtin_IB_bftof_2 (short2  a) __attribute__((const));
float3  __builtin_IB_bftof_3 (short3  a) __attribute__((const));
float4  __builtin_IB_bftof_4 (short4  a) __attribute__((const));
float8  __builtin_IB_bftof_8 (short8  a) __attribute__((const));
float16 __builtin_IB_bftof_16(short16 a) __attribute__((const));

// 2 floats --> packed 2 bf (rte rounding mode)
int   __builtin_IB_2fto2bf_1 (float   a, float   b) __attribute__((const));
int2  __builtin_IB_2fto2bf_2 (float2  a, float2  b) __attribute__((const));
int3  __builtin_IB_2fto2bf_3 (float3  a, float3  b) __attribute__((const));
int4  __builtin_IB_2fto2bf_4 (float4  a, float4  b) __attribute__((const));
int8  __builtin_IB_2fto2bf_8 (float8  a, float8  b) __attribute__((const));
int16 __builtin_IB_2fto2bf_16(float16 a, float16 b) __attribute__((const));


// bf8 <--> half float conversion
//    bf8 : no igc type for bf8 yet. Use char as *opaque* type for it.
//    Rounding: RTNE
//
// hf -> bf8 conversion builtins (rte rounding mode)
char   __builtin_IB_hftobf8_1 (half   a) __attribute__((const));
char2  __builtin_IB_hftobf8_2 (half2  a) __attribute__((const));
char3  __builtin_IB_hftobf8_3 (half3  a) __attribute__((const));
char4  __builtin_IB_hftobf8_4 (half4  a) __attribute__((const));
char8  __builtin_IB_hftobf8_8 (half8  a) __attribute__((const));
char16 __builtin_IB_hftobf8_16(half16 a) __attribute__((const));

// bf8 -> hf conversion builtins (precise conversion)
half   __builtin_IB_bf8tohf_1 (char   a) __attribute__((const));
half2  __builtin_IB_bf8tohf_2 (char2  a) __attribute__((const));
half3  __builtin_IB_bf8tohf_3 (char3  a) __attribute__((const));
half4  __builtin_IB_bf8tohf_4 (char4  a) __attribute__((const));
half8  __builtin_IB_bf8tohf_8 (char8  a) __attribute__((const));
half16 __builtin_IB_bf8tohf_16(char16 a) __attribute__((const));

// hf -> bf8 conversion with saturation.
char   __builtin_IB_hftobf8_1_sat (half   a) __attribute__((const));
char2  __builtin_IB_hftobf8_2_sat (half2  a) __attribute__((const));
char3  __builtin_IB_hftobf8_3_sat (half3  a) __attribute__((const));
char4  __builtin_IB_hftobf8_4_sat (half4  a) __attribute__((const));
char8  __builtin_IB_hftobf8_8_sat (half8  a) __attribute__((const));
char16 __builtin_IB_hftobf8_16_sat(half16 a) __attribute__((const));

// hf -> hf8 conversion builtins (rte rounding mode)
char   __builtin_IB_hftohf8_1 (half   a) __attribute__((const));
char2  __builtin_IB_hftohf8_2 (half2  a) __attribute__((const));
char3  __builtin_IB_hftohf8_3 (half3  a) __attribute__((const));
char4  __builtin_IB_hftohf8_4 (half4  a) __attribute__((const));
char8  __builtin_IB_hftohf8_8 (half8  a) __attribute__((const));
char16 __builtin_IB_hftohf8_16(half16 a) __attribute__((const));

// hf -> hf8 conversion with saturation
char   __builtin_IB_hftohf8_1_sat (half   a) __attribute__((const));
char2  __builtin_IB_hftohf8_2_sat (half2  a) __attribute__((const));
char3  __builtin_IB_hftohf8_3_sat (half3  a) __attribute__((const));
char4  __builtin_IB_hftohf8_4_sat (half4  a) __attribute__((const));
char8  __builtin_IB_hftohf8_8_sat (half8  a) __attribute__((const));
char16 __builtin_IB_hftohf8_16_sat(half16 a) __attribute__((const));

// hf8 -> hf conversion builtins (precise conversion)
half   __builtin_IB_hf8tohf_1 (char   a) __attribute__((const));
half2  __builtin_IB_hf8tohf_2 (char2  a) __attribute__((const));
half3  __builtin_IB_hf8tohf_3 (char3  a) __attribute__((const));
half4  __builtin_IB_hf8tohf_4 (char4  a) __attribute__((const));
half8  __builtin_IB_hf8tohf_8 (char8  a) __attribute__((const));
half16 __builtin_IB_hf8tohf_16(char16 a) __attribute__((const));

// tf32 <--> float conversion
//    tf32 : no igc type for tf32. Use float as *opaque* type for it.
//           (tf32: 19 bits, taking 32bit storage)
//
// f -> tf32 conversion builtins (rte rounding mode)
float   __builtin_IB_ftotf32_1 (float   a) __attribute__((const));
float2  __builtin_IB_ftotf32_2 (float2  a) __attribute__((const));
float3  __builtin_IB_ftotf32_3 (float3  a) __attribute__((const));
float4  __builtin_IB_ftotf32_4 (float4  a) __attribute__((const));
float8  __builtin_IB_ftotf32_8 (float8  a) __attribute__((const));
float16 __builtin_IB_ftotf32_16(float16 a) __attribute__((const));

// tf32 -> f conversion is not needed, since every tf32 is already a float

// Stochastic rounding : srnd d  a  r
//      d: bf8 | hf
//      a: hf | f
//      r: random number, has the same type as a's
//  HF -> BF8
char   __builtin_IB_srnd_hftobf8_1 (half   a, char   r) __attribute__((const));
char2  __builtin_IB_srnd_hftobf8_2 (half2  a, char2  r) __attribute__((const));
char3  __builtin_IB_srnd_hftobf8_3 (half3  a, char3  r) __attribute__((const));
char4  __builtin_IB_srnd_hftobf8_4 (half4  a, char4  r) __attribute__((const));
char8  __builtin_IB_srnd_hftobf8_8 (half8  a, char8  r) __attribute__((const));
char16 __builtin_IB_srnd_hftobf8_16(half16 a, char16 r) __attribute__((const));

char   __builtin_IB_srnd_hftobf8_1_sat (half   a, char   r) __attribute__((const));
char2  __builtin_IB_srnd_hftobf8_2_sat (half2  a, char2  r) __attribute__((const));
char3  __builtin_IB_srnd_hftobf8_3_sat (half3  a, char3  r) __attribute__((const));
char4  __builtin_IB_srnd_hftobf8_4_sat (half4  a, char4  r) __attribute__((const));
char8  __builtin_IB_srnd_hftobf8_8_sat (half8  a, char8  r) __attribute__((const));
char16 __builtin_IB_srnd_hftobf8_16_sat(half16 a, char16 r) __attribute__((const));

// F -> HF
half   __builtin_IB_srnd_ftohf_1 (float   a, short   r) __attribute__((const));
half2  __builtin_IB_srnd_ftohf_2 (float2  a, short2  r) __attribute__((const));
half3  __builtin_IB_srnd_ftohf_3 (float3  a, short3  r) __attribute__((const));
half4  __builtin_IB_srnd_ftohf_4 (float4  a, short4  r) __attribute__((const));
half8  __builtin_IB_srnd_ftohf_8 (float8  a, short8  r) __attribute__((const));
half16 __builtin_IB_srnd_ftohf_16(float16 a, short16 r) __attribute__((const));

uint __builtin_IB_lfsr_b32(uint seed, uint polynomial) __attribute__((const)); // as 1 int
uint __builtin_IB_lfsr_b16v2(uint seed, uint polynomial) __attribute__((const)); // as 2 shorts
uint __builtin_IB_lfsr_b8v4(uint seed, uint polynomial) __attribute__((const)); // as 4 chars
// for sources smaller than 32 bits we have special variants that operate in lower simd
ushort __builtin_IB_lfsr_b16v2_ushort(ushort seed, ushort polynomial) __attribute__((const));
ushort __builtin_IB_lfsr_b8v4_ushort(ushort seed, ushort polynomial) __attribute__((const));
uchar __builtin_IB_lfsr_b8v4_uchar(uchar seed, uchar polynomial) __attribute__((const));

enum DNSCL_CONVERT_TO_TYPE {
    DNSCL_CONVERT_TO_E2M1 = 1,  // conversion to E2M1
    DNSCL_CONVERT_TO_INT4 = 2,  // conversion to INT4
};
uint __builtin_IB_dnscl_bf16(uint, uint, enum DNSCL_CONVERT_TO_TYPE convert_to, uint mode);
uint __builtin_IB_dnscl_hf16(uint,  uint,  enum DNSCL_CONVERT_TO_TYPE convert_to, uint mode);
uint __builtin_IB_dnscl_bf16_srnd(uint, uint, uint, enum DNSCL_CONVERT_TO_TYPE convert_to, uint mode);
uint __builtin_IB_dnscl_hf16_srnd(uint,  uint,  uint, enum DNSCL_CONVERT_TO_TYPE convert_to, uint mode);

// HF -> HF8 - stochastic rounding (a: to be rounded; r: random number used for rounding)
char   __builtin_IB_srnd_hftohf8_1 (half   a, char   r) __attribute__((const));
char2  __builtin_IB_srnd_hftohf8_2 (half2  a, char2  r) __attribute__((const));
char3  __builtin_IB_srnd_hftohf8_3 (half3  a, char3  r) __attribute__((const));
char4  __builtin_IB_srnd_hftohf8_4 (half4  a, char4  r) __attribute__((const));
char8  __builtin_IB_srnd_hftohf8_8 (half8  a, char8  r) __attribute__((const));
char16 __builtin_IB_srnd_hftohf8_16(half16 a, char16 r) __attribute__((const));
// HF -> HF8 - saturated stochastic rounding (a: to be rounded; r: random number used for rounding)
char   __builtin_IB_srnd_hftohf8_1_sat (half   a, char   r) __attribute__((const));
char2  __builtin_IB_srnd_hftohf8_2_sat (half2  a, char2  r) __attribute__((const));
char3  __builtin_IB_srnd_hftohf8_3_sat (half3  a, char3  r) __attribute__((const));
char4  __builtin_IB_srnd_hftohf8_4_sat (half4  a, char4  r) __attribute__((const));
char8  __builtin_IB_srnd_hftohf8_8_sat (half8  a, char8  r) __attribute__((const));
char16 __builtin_IB_srnd_hftohf8_16_sat(half16 a, char16 r) __attribute__((const));

// BF -> BF8 - stochastic rounding (a: to be rounded; r: random number used for rounding)
char   __builtin_IB_srnd_bftobf8_1 (short   a, char   r) __attribute__((const));
char2  __builtin_IB_srnd_bftobf8_2 (short2  a, char2  r) __attribute__((const));
char3  __builtin_IB_srnd_bftobf8_3 (short3  a, char3  r) __attribute__((const));
char4  __builtin_IB_srnd_bftobf8_4 (short4  a, char4  r) __attribute__((const));
char8  __builtin_IB_srnd_bftobf8_8 (short8  a, char8  r) __attribute__((const));
char16 __builtin_IB_srnd_bftobf8_16(short16 a, char16 r) __attribute__((const));
// BF -> BF8 - saturated stochastic rounding (a: to be rounded; r: random number used for rounding)
char   __builtin_IB_srnd_bftobf8_1_sat (short   a, char   r) __attribute__((const));
char2  __builtin_IB_srnd_bftobf8_2_sat (short2  a, char2  r) __attribute__((const));
char3  __builtin_IB_srnd_bftobf8_3_sat (short3  a, char3  r) __attribute__((const));
char4  __builtin_IB_srnd_bftobf8_4_sat (short4  a, char4  r) __attribute__((const));
char8  __builtin_IB_srnd_bftobf8_8_sat (short8  a, char8  r) __attribute__((const));
char16 __builtin_IB_srnd_bftobf8_16_sat(short16 a, char16 r) __attribute__((const));

// BF -> HF8 - stochastic rounding (a: to be rounded; r: random number used for rounding)
char   __builtin_IB_srnd_bftohf8_1 (short   a, char   r) __attribute__((const));
char2  __builtin_IB_srnd_bftohf8_2 (short2  a, char2  r) __attribute__((const));
char3  __builtin_IB_srnd_bftohf8_3 (short3  a, char3  r) __attribute__((const));
char4  __builtin_IB_srnd_bftohf8_4 (short4  a, char4  r) __attribute__((const));
char8  __builtin_IB_srnd_bftohf8_8 (short8  a, char8  r) __attribute__((const));
char16 __builtin_IB_srnd_bftohf8_16(short16 a, char16 r) __attribute__((const));
// BF -> HF8 - saturated stochastic rounding (a: to be rounded; r: random number used for rounding)
char   __builtin_IB_srnd_bftohf8_1_sat (short   a, char   r) __attribute__((const));
char2  __builtin_IB_srnd_bftohf8_2_sat (short2  a, char2  r) __attribute__((const));
char3  __builtin_IB_srnd_bftohf8_3_sat (short3  a, char3  r) __attribute__((const));
char4  __builtin_IB_srnd_bftohf8_4_sat (short4  a, char4  r) __attribute__((const));
char8  __builtin_IB_srnd_bftohf8_8_sat (short8  a, char8  r) __attribute__((const));
char16 __builtin_IB_srnd_bftohf8_16_sat(short16 a, char16 r) __attribute__((const));
#endif // IGCBIF_INTRINSICS_DPAS_CL

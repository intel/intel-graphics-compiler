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



#endif // IGCBIF_INTRINSICS_DPAS_CL

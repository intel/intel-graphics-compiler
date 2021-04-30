/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../../Headers/spirv.h"

#ifndef __BIF_DEFINITIONS_CL__
#define __BIF_DEFINITIONS_CL__

#define FLOAT_SIGN_MASK         (0x80000000)  // used to be FLT_SIGN_MASK
#define FLOAT_EXPONENT_MASK     (0x7F800000)  // used to be EXPONENT_MASK
#define FLOAT_MANTISSA_MASK     (0x007FFFFF)  // used to be MANTISSA_MASK
#define FLOAT_QUIET_NAN         (0x7FFFFFFF)  // used to be QUIET_NAN
#define FLOAT_NEG_ONE_EXP_MASK  (0x3F000000)
#define FLOAT_IMPLICIT_BIT      (0x00800000)
#define FLOAT_BIAS              (127)
#define FLOAT_MANTISSA_BITS     (23)

#define HALF_SIGN_MASK          ((short)(0x8000))
#define HALF_EXPONENT_MASK      ((short)(0x7C00))
#define HALF_MANTISSA_MASK      ((short)(0x03FF))
#define HALF_QUIET_NAN          ((short)(0x7FFF))
#define HALF_NEG_ONE_EXP_MASK   ((short)(0x3800))
#define HALF_IMPLICIT_BIT       ((short)(0x0800))

#define DOUBLE_SIGN_MASK        (0x8000000000000000L)
#define DOUBLE_EXPONENT_MASK    (0x7FF0000000000000L)
#define DOUBLE_MANTISSA_MASK    (0x000FFFFFFFFFFFFFL)
#define DOUBLE_QUIET_NAN        (0x7FFFFFFFFFFFFFFFL)
#define DOUBLE_NEG_ONE_EXP_MASK (0x3FE0000000000000L)
#define DOUBLE_BIAS             (1023)
#define DOUBLE_MANTISSA_BITS    (52)

#define HALF_BITS               (16)
#define HALF_SIGN_BITS          (1)
#define HALF_EXPONENT_BITS      (5)
#define HALF_MANTISSA_BITS      (10)
#define HALF_BIAS               (15)
#define HALF_EXPONENT_RANGE     (31)

#define FLOAT_BITS              (32)
#define FLOAT_SIGN_BITS         (1)
#define FLOAT_EXPONENT_BITS     (8)
#define FLOAT_MANTISSA_BITS     (23)
#define FLOAT_BIAS              (127)
#define FLOAT_EXPONENT_RANGE    (255)

#define DOUBLE_BITS             (64)
#define DOUBLE_SIGN_BITS        (1)
#define DOUBLE_EXPONENT_BITS    (11)
#define DOUBLE_MANTISSA_BITS    (52)
#define DOUBLE_BIAS             (1023)

#define as_char(x) __builtin_astype((x), char)
#define as_char2(x) __builtin_astype((x), char2)
#define as_char3(x) __builtin_astype((x), char3)
#define as_char4(x) __builtin_astype((x), char4)
#define as_char8(x) __builtin_astype((x), char8)
#define as_char16(x) __builtin_astype((x), char16)

#define as_uchar(x) __builtin_astype((x), uchar)
#define as_uchar2(x) __builtin_astype((x), uchar2)
#define as_uchar3(x) __builtin_astype((x), uchar3)
#define as_uchar4(x) __builtin_astype((x), uchar4)
#define as_uchar8(x) __builtin_astype((x), uchar8)
#define as_uchar16(x) __builtin_astype((x), uchar16)

#define as_short(x) __builtin_astype((x), short)
#define as_short2(x) __builtin_astype((x), short2)
#define as_short3(x) __builtin_astype((x), short3)
#define as_short4(x) __builtin_astype((x), short4)
#define as_short8(x) __builtin_astype((x), short8)
#define as_short16(x) __builtin_astype((x), short16)

#define as_ushort(x) __builtin_astype((x), ushort)
#define as_ushort2(x) __builtin_astype((x), ushort2)
#define as_ushort3(x) __builtin_astype((x), ushort3)
#define as_ushort4(x) __builtin_astype((x), ushort4)
#define as_ushort8(x) __builtin_astype((x), ushort8)
#define as_ushort16(x) __builtin_astype((x), ushort16)

#define as_int(x) __builtin_astype((x), int)
#define as_int2(x) __builtin_astype((x), int2)
#define as_int3(x) __builtin_astype((x), int3)
#define as_int4(x) __builtin_astype((x), int4)
#define as_int8(x) __builtin_astype((x), int8)
#define as_int16(x) __builtin_astype((x), int16)

#define as_uint(x) __builtin_astype((x), uint)
#define as_uint2(x) __builtin_astype((x), uint2)
#define as_uint3(x) __builtin_astype((x), uint3)
#define as_uint4(x) __builtin_astype((x), uint4)
#define as_uint8(x) __builtin_astype((x), uint8)
#define as_uint16(x) __builtin_astype((x), uint16)

#define as_long(x) __builtin_astype((x), long)
#define as_long2(x) __builtin_astype((x), long2)
#define as_long3(x) __builtin_astype((x), long3)
#define as_long4(x) __builtin_astype((x), long4)
#define as_long8(x) __builtin_astype((x), long8)
#define as_long16(x) __builtin_astype((x), long16)

#define as_ulong(x) __builtin_astype((x), ulong)
#define as_ulong2(x) __builtin_astype((x), ulong2)
#define as_ulong3(x) __builtin_astype((x), ulong3)
#define as_ulong4(x) __builtin_astype((x), ulong4)
#define as_ulong8(x) __builtin_astype((x), ulong8)
#define as_ulong16(x) __builtin_astype((x), ulong16)

#define as_half(x) __builtin_astype((x), half)
#define as_half2(x) __builtin_astype((x), half2)
#define as_half3(x) __builtin_astype((x), half3)
#define as_half4(x) __builtin_astype((x), half4)
#define as_half8(x) __builtin_astype((x), half8)
#define as_half16(x) __builtin_astype((x), half16)

#define as_float(x) __builtin_astype((x), float)
#define as_float2(x) __builtin_astype((x), float2)
#define as_float3(x) __builtin_astype((x), float3)
#define as_float4(x) __builtin_astype((x), float4)
#define as_float8(x) __builtin_astype((x), float8)
#define as_float16(x) __builtin_astype((x), float16)

#define as_double(x) __builtin_astype((x), double)
#define as_double2(x) __builtin_astype((x), double2)
#define as_double3(x) __builtin_astype((x), double3)
#define as_double4(x) __builtin_astype((x), double4)
#define as_double8(x) __builtin_astype((x), double8)
#define as_double16(x) __builtin_astype((x), double16)

#define ONE_EIGHTY_OVER_PI_DBL   (as_double(0x404CA5DC1A63C1F8)) // 57.295779513082320876798154814105
#define ONE_EIGHTY_OVER_PI_FLT   (as_float(0x42652EE1))          // 57.295779513082320876798154814105f
#define ONE_EIGHTY_OVER_PI_HLF   (as_half((ushort)0x5329))       // 57.295779513082320876798154814105h

#define PI_OVER_ONE_EIGHTY_DBL   (as_double(0x3F91DF46A2529D39)) // 0.01745329251994329576923690768489
#define PI_OVER_ONE_EIGHTY_FLT   (as_float(0x3C8EFA35))          // 0.01745329251994329576923690768489f
#define PI_OVER_ONE_EIGHTY_HLF   (as_half((ushort)0x2478))       // 0.01745329251994329576923690768489h

#define MINNORM                 (0x00800000)
#define MAXNORM                 (0x7f7fffff)

#endif // __BIF_DEFINITIONS_CL__

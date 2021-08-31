/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef OPENCL_DEF
#define OPENCL_DEF

#include <opencl_type_traits.h>

using int8_t = char;
using uint8_t = unsigned char;
using int16_t = short;
using uint16_t = unsigned short;
using int32_t = int;
using uint32_t = unsigned;
using int64_t = long;
using uint64_t = unsigned long;
using int128_t = long long;
using uint128_t = unsigned long long;

using intptr_t =
    typename cl::conditional<sizeof(void *) == 4, int32_t, int64_t>::type;
using uintptr_t =
    typename cl::conditional<sizeof(void *) == 4, uint32_t, uint64_t>::type;

using uchar = unsigned char;
using ushort = unsigned short;
using uint = unsigned int;
using ulong = unsigned long;

using char2 = char __attribute__((ext_vector_type(2)));
using char3 = char __attribute__((ext_vector_type(3)));
using char4 = char __attribute__((ext_vector_type(4)));
using char8 = char __attribute__((ext_vector_type(8)));
using char16 = char __attribute__((ext_vector_type(16)));
using uchar2 = uchar __attribute__((ext_vector_type(2)));
using uchar3 = uchar __attribute__((ext_vector_type(3)));
using uchar4 = uchar __attribute__((ext_vector_type(4)));
using uchar8 = uchar __attribute__((ext_vector_type(8)));
using uchar16 = uchar __attribute__((ext_vector_type(16)));
using short2 = short __attribute__((ext_vector_type(2)));
using short3 = short __attribute__((ext_vector_type(3)));
using short4 = short __attribute__((ext_vector_type(4)));
using short8 = short __attribute__((ext_vector_type(8)));
using short16 = short __attribute__((ext_vector_type(16)));
using ushort2 = ushort __attribute__((ext_vector_type(2)));
using ushort3 = ushort __attribute__((ext_vector_type(3)));
using ushort4 = ushort __attribute__((ext_vector_type(4)));
using ushort8 = ushort __attribute__((ext_vector_type(8)));
using ushort16 = ushort __attribute__((ext_vector_type(16)));
using int2 = int __attribute__((ext_vector_type(2)));
using int3 = int __attribute__((ext_vector_type(3)));
using int4 = int __attribute__((ext_vector_type(4)));
using int8 = int __attribute__((ext_vector_type(8)));
using int16 = int __attribute__((ext_vector_type(16)));
using uint2 = uint __attribute__((ext_vector_type(2)));
using uint3 = uint __attribute__((ext_vector_type(3)));
using uint4 = uint __attribute__((ext_vector_type(4)));
using uint8 = uint __attribute__((ext_vector_type(8)));
using uint16 = uint __attribute__((ext_vector_type(16)));
using long2 = long __attribute__((ext_vector_type(2)));
using long3 = long __attribute__((ext_vector_type(3)));
using long4 = long __attribute__((ext_vector_type(4)));
using long8 = long __attribute__((ext_vector_type(8)));
using long16 = long __attribute__((ext_vector_type(16)));
using ulong2 = ulong __attribute__((ext_vector_type(2)));
using ulong3 = ulong __attribute__((ext_vector_type(3)));
using ulong4 = ulong __attribute__((ext_vector_type(4)));
using ulong8 = ulong __attribute__((ext_vector_type(8)));
using ulong16 = ulong __attribute__((ext_vector_type(16)));
using float2 = float __attribute__((ext_vector_type(2)));
using float3 = float __attribute__((ext_vector_type(3)));
using float4 = float __attribute__((ext_vector_type(4)));
using float8 = float __attribute__((ext_vector_type(8)));
using float16 = float __attribute__((ext_vector_type(16)));
#ifdef cl_khr_fp16
#pragma OPENCL EXTENSION cl_khr_fp16 : enable
using half2 = half __attribute__((ext_vector_type(2)));
using half3 = half __attribute__((ext_vector_type(3)));
using half4 = half __attribute__((ext_vector_type(4)));
using half8 = half __attribute__((ext_vector_type(8)));
using half16 = half __attribute__((ext_vector_type(16)));
#endif
using double2 = double __attribute__((ext_vector_type(2)));
using double3 = double __attribute__((ext_vector_type(3)));
using double4 = double __attribute__((ext_vector_type(4)));
using double8 = double __attribute__((ext_vector_type(8)));
using double16 = double __attribute__((ext_vector_type(16)));
#endif // OPENCL_DEF

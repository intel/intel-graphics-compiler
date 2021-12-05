/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef OPENCL_DEF
#define OPENCL_DEF

#include <opencl_detail.h>

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
    typename cl::detail::conditional<sizeof(void *) == 4, int32_t, int64_t>::type;
using uintptr_t =
    typename cl::detail::conditional<sizeof(void *) == 4, uint32_t, uint64_t>::type;

using uchar = unsigned char;
using ushort = unsigned short;
using uint = unsigned int;
using ulong = unsigned long;

using char2 = cl::detail::vector_impl<char, 2>;
using char3 = cl::detail::vector_impl<char, 3>;
using char4 = cl::detail::vector_impl<char, 4>;
using char8 = cl::detail::vector_impl<char, 8>;
using char16 = cl::detail::vector_impl<char, 16>;
using uchar2 = cl::detail::vector_impl<uchar, 2>;
using uchar3 = cl::detail::vector_impl<uchar, 3>;
using uchar4 = cl::detail::vector_impl<uchar, 4>;
using uchar8 = cl::detail::vector_impl<uchar, 8>;
using uchar16 = cl::detail::vector_impl<uchar, 16>;
using short2 = cl::detail::vector_impl<short, 2>;
using short3 = cl::detail::vector_impl<short, 3>;
using short4 = cl::detail::vector_impl<short, 4>;
using short8 = cl::detail::vector_impl<short, 8>;
using short16 = cl::detail::vector_impl<short, 16>;
using ushort2 = cl::detail::vector_impl<ushort, 2>;
using ushort3 = cl::detail::vector_impl<ushort, 3>;
using ushort4 = cl::detail::vector_impl<ushort, 4>;
using ushort8 = cl::detail::vector_impl<ushort, 8>;
using ushort16 = cl::detail::vector_impl<ushort, 16>;
using int2 = cl::detail::vector_impl<int, 2>;
using int3 = cl::detail::vector_impl<int, 3>;
using int4 = cl::detail::vector_impl<int, 4>;
using int8 = cl::detail::vector_impl<int, 8>;
using int16 = cl::detail::vector_impl<int, 16>;
using uint2 = cl::detail::vector_impl<uint, 2>;
using uint3 = cl::detail::vector_impl<uint, 3>;
using uint4 = cl::detail::vector_impl<uint, 4>;
using uint8 = cl::detail::vector_impl<uint, 8>;
using uint16 = cl::detail::vector_impl<uint, 16>;
using long2 = cl::detail::vector_impl<long, 2>;
using long3 = cl::detail::vector_impl<long, 3>;
using long4 = cl::detail::vector_impl<long, 4>;
using long8 = cl::detail::vector_impl<long, 8>;
using long16 = cl::detail::vector_impl<long, 16>;
using ulong2 = cl::detail::vector_impl<ulong, 2>;
using ulong3 = cl::detail::vector_impl<ulong, 3>;
using ulong4 = cl::detail::vector_impl<ulong, 4>;
using ulong8 = cl::detail::vector_impl<ulong, 8>;
using ulong16 = cl::detail::vector_impl<ulong, 16>;
using float2 = cl::detail::vector_impl<float, 2>;
using float3 = cl::detail::vector_impl<float, 3>;
using float4 = cl::detail::vector_impl<float, 4>;
using float8 = cl::detail::vector_impl<float, 8>;
using float16 = cl::detail::vector_impl<float, 16>;
#ifdef cl_khr_fp16
#pragma OPENCL EXTENSION cl_khr_fp16 : enable
using half2 = cl::detail::vector_impl<half, 2>;
using half3 = cl::detail::vector_impl<half, 3>;
using half4 = cl::detail::vector_impl<half, 4>;
using half8 = cl::detail::vector_impl<half, 8>;
using half16 = cl::detail::vector_impl<half, 16>;
#endif
using double2 = cl::detail::vector_impl<double, 2>;
using double3 = cl::detail::vector_impl<double, 3>;
using double4 = cl::detail::vector_impl<double, 4>;
using double8 = cl::detail::vector_impl<double, 8>;
using double16 = cl::detail::vector_impl<double, 16>;

enum memory_order {
  memory_order_relaxed = 0,
  memory_order_acquire = 1,
  memory_order_release = 2,
  memory_order_acq_rel = 3,
  memory_order_seq_cst = 4
};

enum memory_scope {
  memory_scope_work_item = 0,
  memory_scope_work_group = 1,
  memory_scope_device = 2,
  memory_scope_all_svm_devices = 3,
  memory_scope_all_devices = memory_scope_all_svm_devices,
  memory_scope_sub_group = 4
};

#define as_double(x) __builtin_astype((x), double)
#define as_float(x) __builtin_astype((x), float)
#define as_int(x) __builtin_astype((x), int)
#define as_long(x) __builtin_astype((x), long)

#endif // OPENCL_DEF

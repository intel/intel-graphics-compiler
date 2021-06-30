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

#endif // OPENCL_DEF

/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "spirv.h"

INLINE
uchar OVERLOADABLE popcount(uchar x) { return __builtin_IB_popcount_1u8(x); }

INLINE
ushort OVERLOADABLE popcount(ushort x) { return __builtin_IB_popcount_1u16(x); }

INLINE
uint OVERLOADABLE popcount(uint x) { return __builtin_IB_popcount_1u32(x); }

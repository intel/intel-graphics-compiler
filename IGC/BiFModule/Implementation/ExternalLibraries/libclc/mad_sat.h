/*========================== begin_copyright_notice ============================

Copyright (C) 2021-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../../include/BiF_Definitions.h"
#include "../../../Headers/spirv.h"
#include "mul_hi.h"

INLINE OVERLOADABLE long libclc_mad_sat(long x, long y, long z) {
  long hi = libclc_mul_hi(x, y);
  ulong lo = (ulong)x * (ulong)y;
  ulong r = lo + (ulong)z;
  hi += (z >> 63) + (r < lo);
  if (hi != ((long)r >> 63))
    return hi < 0 ? LONG_MIN : LONG_MAX;
  return (long)r;
}

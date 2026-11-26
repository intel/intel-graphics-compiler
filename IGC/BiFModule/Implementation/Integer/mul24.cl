/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.h"
#include "../../Headers/spirv.h"

// Valid 24-bit operands give products wider than 32 bits; signed overflow would be UB.
INLINE
int __attribute__((overloadable)) __spirv_ocl_s_mul24(int x, int y) { return as_int(as_uint(x) * as_uint(y)); }

INLINE
int2 __attribute__((overloadable)) __spirv_ocl_s_mul24(int2 x, int2 y) { return as_int2(as_uint2(x) * as_uint2(y)); }

INLINE
int3 __attribute__((overloadable)) __spirv_ocl_s_mul24(int3 x, int3 y) { return as_int3(as_uint3(x) * as_uint3(y)); }

INLINE
int4 __attribute__((overloadable)) __spirv_ocl_s_mul24(int4 x, int4 y) { return as_int4(as_uint4(x) * as_uint4(y)); }

INLINE
int8 __attribute__((overloadable)) __spirv_ocl_s_mul24(int8 x, int8 y) { return as_int8(as_uint8(x) * as_uint8(y)); }

INLINE
int16 __attribute__((overloadable)) __spirv_ocl_s_mul24(int16 x, int16 y) {
  return as_int16(as_uint16(x) * as_uint16(y));
}

INLINE
uint __attribute__((overloadable)) __spirv_ocl_u_mul24(uint x, uint y) { return x * y; }

INLINE
uint2 __attribute__((overloadable)) __spirv_ocl_u_mul24(uint2 x, uint2 y) { return x * y; }

INLINE
uint3 __attribute__((overloadable)) __spirv_ocl_u_mul24(uint3 x, uint3 y) { return x * y; }

INLINE
uint4 __attribute__((overloadable)) __spirv_ocl_u_mul24(uint4 x, uint4 y) { return x * y; }

INLINE
uint8 __attribute__((overloadable)) __spirv_ocl_u_mul24(uint8 x, uint8 y) { return x * y; }

INLINE
uint16 __attribute__((overloadable)) __spirv_ocl_u_mul24(uint16 x, uint16 y) { return x * y; }

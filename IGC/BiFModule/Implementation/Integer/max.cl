/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.h"
#include "../../Headers/spirv.h"

INLINE
char __attribute__((overloadable)) __spirv_ocl_s_max(char x, char y) { return (x >= y) ? x : y; }

INLINE
uchar __attribute__((overloadable)) __spirv_ocl_u_max(uchar x, uchar y) { return (x >= y) ? x : y; }

INLINE
short __attribute__((overloadable)) __spirv_ocl_s_max(short x, short y) { return (x >= y) ? x : y; }

INLINE
ushort __attribute__((overloadable)) __spirv_ocl_u_max(ushort x, ushort y) { return (x >= y) ? x : y; }

INLINE
int __attribute__((overloadable)) __spirv_ocl_s_max(int x, int y) { return (x >= y) ? x : y; }

INLINE
uint __attribute__((overloadable)) __spirv_ocl_u_max(uint x, uint y) { return (x >= y) ? x : y; }

INLINE
long __attribute__((overloadable)) __spirv_ocl_s_max(long x, long y) { return (x >= y) ? x : y; }

INLINE
ulong __attribute__((overloadable)) __spirv_ocl_u_max(ulong x, ulong y) { return (x >= y) ? x : y; }

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS(s_max, char, char, i8)
GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS(u_max, uchar, uchar, i8)
GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS(s_max, short, short, i16)
GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS(u_max, ushort, ushort, i16)
GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS(s_max, int, int, i32)
GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS(u_max, uint, uint, i32)
GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS(s_max, long, long, i64)
GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS(u_max, ulong, ulong, i64)

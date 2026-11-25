/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.h"
#include "../../Headers/spirv.h"

INLINE
char __attribute__((overloadable)) __spirv_ocl_s_min(char x, char y) { return (x <= y) ? x : y; }

INLINE
uchar __attribute__((overloadable)) __spirv_ocl_u_min(uchar x, uchar y) { return (x <= y) ? x : y; }

INLINE
short __attribute__((overloadable)) __spirv_ocl_s_min(short x, short y) { return (x <= y) ? x : y; }

INLINE
ushort __attribute__((overloadable)) __spirv_ocl_u_min(ushort x, ushort y) { return (x <= y) ? x : y; }

INLINE
int __attribute__((overloadable)) __spirv_ocl_s_min(int x, int y) { return (x <= y) ? x : y; }

INLINE
uint __attribute__((overloadable)) __spirv_ocl_u_min(uint x, uint y) { return (x <= y) ? x : y; }

INLINE
long __attribute__((overloadable)) __spirv_ocl_s_min(long x, long y) { return (x < y) ? x : y; }

INLINE
ulong __attribute__((overloadable)) __spirv_ocl_u_min(ulong x, ulong y) { return (x < y) ? x : y; }

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS(s_min, char, char, i8)
GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS(u_min, uchar, uchar, i8)
GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS(s_min, short, short, i16)
GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS(u_min, ushort, ushort, i16)
GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS(s_min, int, int, i32)
GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS(u_min, uint, uint, i32)
GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS(s_min, long, long, i64)
GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS(u_min, ulong, ulong, i64)

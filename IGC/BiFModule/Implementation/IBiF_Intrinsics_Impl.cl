/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "spirv.h"

INLINE
char __attribute__((overloadable)) __spirv_ocl_popcount(char x) { return __builtin_IB_popcount_1u8(x); }

INLINE
short __attribute__((overloadable)) __spirv_ocl_popcount(short x) { return __builtin_IB_popcount_1u16(x); }

INLINE
int __attribute__((overloadable)) __spirv_ocl_popcount(int x) { return __builtin_IB_popcount_1u32(x); }

INLINE
float FDIV_IEEE(float a, float b) { return __builtin_IB_ieee_divide(a, b); }

INLINE
double FDIV_IEEE_DOUBLE(double a, double b) { return __builtin_IB_ieee_divide_f64(a, b); }

INLINE
float FSQRT_IEEE(float a) { return __builtin_IB_ieee_sqrt(a); }

/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
// REQUIRES: regkeys, pvc-supported, llvm-16-plus

/// Check if correct result values are inlined into the kernel
/// when out-of-dest-range inputs are known at compile time

// RUN: ocloc compile -file %s -device pvc \
// RUN: -options "-cl-std=CL3.0 -igc_opts 'EnableOpaquePointersBackend=1 PrintToConsole=1 PrintBefore=EmitPass'" \
// RUN: -out_dir /dev/null 2>&1 | FileCheck %s

#pragma OPENCL EXTENSION cl_khr_fp16 : enable
#define POS_INF_half (half)INFINITY;
#define NEG_INF_half (half)-INFINITY;
#define NAN_half (half)NAN;
#define POS_INF_float (float)INFINITY;
#define NEG_INF_float (float)-INFINITY;
#define NAN_float (float)NAN;
#define POS_INF_double (double)INFINITY;
#define NEG_INF_double (double)-INFINITY;
#define NAN_double (double)NAN;

#define test_intty_with_fp_out_of_range(INTTY, FPTY, VALTY)       \
kernel void test_##INTTY##_##FPTY##_##VALTY(global INTTY *dst) {  \
  FPTY source_fp = VALTY##_##FPTY;                                \
  *dst = convert_##INTTY##_sat(source_fp);                        \
}

/*////////////////////
/// Positive infinity
*/////////////////////

// CHECK-LABEL: define spir_kernel void @test_char_half_POS_INF
// CHECK: store i8 127, ptr addrspace(1) %dst
test_intty_with_fp_out_of_range(char, half, POS_INF)
// CHECK-LABEL: define spir_kernel void @test_uchar_half_POS_INF
// CHECK: store i8 -1, ptr addrspace(1) %dst
test_intty_with_fp_out_of_range(uchar, half, POS_INF)
// CHECK-LABEL: define spir_kernel void @test_short_half_POS_INF
// CHECK: store i16 32767, ptr addrspace(1) %dst
test_intty_with_fp_out_of_range(short, half, POS_INF)
// CHECK-LABEL: define spir_kernel void @test_ushort_half_POS_INF
// CHECK: store i16 -1, ptr addrspace(1) %dst
test_intty_with_fp_out_of_range(ushort, half, POS_INF)
// CHECK-LABEL: define spir_kernel void @test_int_half_POS_INF
// CHECK: store i32 2147483647, ptr addrspace(1) %dst
test_intty_with_fp_out_of_range(int, half, POS_INF)
// CHECK-LABEL: define spir_kernel void @test_uint_half_POS_INF
// CHECK: store i32 -1, ptr addrspace(1) %dst
test_intty_with_fp_out_of_range(uint, half, POS_INF)
/* begin i64 */
// CHECK-LABEL: define spir_kernel void @test_long_half_POS_INF
// CHECK: store <2 x i32> <i32 -1, i32 2147483647>, ptr addrspace(1) %dst
test_intty_with_fp_out_of_range(long, half, POS_INF)
// CHECK-LABEL: define spir_kernel void @test_long_float_POS_INF
// CHECK: store <2 x i32> <i32 -1, i32 2147483647>, ptr addrspace(1) %dst
test_intty_with_fp_out_of_range(long, float, POS_INF)
// CHECK-LABEL: define spir_kernel void @test_long_double_POS_INF
// CHECK: store <2 x i32> <i32 -1, i32 2147483647>, ptr addrspace(1) %dst
test_intty_with_fp_out_of_range(long, double, POS_INF)
// CHECK-LABEL: define spir_kernel void @test_ulong_half_POS_INF
// CHECK: store <2 x i32> <i32 -1, i32 -1>, ptr addrspace(1) %dst
test_intty_with_fp_out_of_range(ulong, half, POS_INF)
// CHECK-LABEL: define spir_kernel void @test_ulong_float_POS_INF
// CHECK: store <2 x i32> <i32 -1, i32 -1>, ptr addrspace(1) %dst
test_intty_with_fp_out_of_range(ulong, float, POS_INF)
// CHECK-LABEL: define spir_kernel void @test_ulong_double_POS_INF
// CHECK: store <2 x i32> <i32 -1, i32 -1>, ptr addrspace(1) %dst
test_intty_with_fp_out_of_range(ulong, double, POS_INF)

/*////////////////////
/// Negative infinity
*/////////////////////

// CHECK-LABEL: define spir_kernel void @test_char_half_NEG_INF
// CHECK: store i8 -128, ptr addrspace(1) %dst
test_intty_with_fp_out_of_range(char, half, NEG_INF)
// CHECK-LABEL: define spir_kernel void @test_uchar_half_NEG_INF
// CHECK: store i8 0, ptr addrspace(1) %dst
test_intty_with_fp_out_of_range(uchar, half, NEG_INF)
// CHECK-LABEL: define spir_kernel void @test_short_half_NEG_INF
// CHECK: store i16 -32768, ptr addrspace(1) %dst
test_intty_with_fp_out_of_range(short, half, NEG_INF)
// CHECK-LABEL: define spir_kernel void @test_ushort_half_NEG_INF
// CHECK: store i16 0, ptr addrspace(1) %dst
test_intty_with_fp_out_of_range(ushort, half, NEG_INF)
// CHECK-LABEL: define spir_kernel void @test_int_half_NEG_INF
// CHECK: store i32 -2147483648, ptr addrspace(1) %dst
test_intty_with_fp_out_of_range(int, half, NEG_INF)
// CHECK-LABEL: define spir_kernel void @test_uint_half_NEG_INF
// CHECK: store i32 0, ptr addrspace(1) %dst
test_intty_with_fp_out_of_range(uint, half, NEG_INF)
/* begin i64 */
// CHECK-LABEL: define spir_kernel void @test_long_half_NEG_INF
// CHECK: store <2 x i32> <i32 0, i32 -2147483648>, ptr addrspace(1) %dst
test_intty_with_fp_out_of_range(long, half, NEG_INF)
// CHECK-LABEL: define spir_kernel void @test_long_float_NEG_INF
// CHECK: store <2 x i32> <i32 0, i32 -2147483648>, ptr addrspace(1) %dst
test_intty_with_fp_out_of_range(long, float, NEG_INF)
// CHECK-LABEL: define spir_kernel void @test_long_double_NEG_INF
// CHECK: store <2 x i32> <i32 0, i32 -2147483648>, ptr addrspace(1) %dst
test_intty_with_fp_out_of_range(long, double, NEG_INF)
// CHECK-LABEL: define spir_kernel void @test_ulong_half_NEG_INF
// CHECK: store <2 x i32> zeroinitializer, ptr addrspace(1) %dst
test_intty_with_fp_out_of_range(ulong, half, NEG_INF)
// CHECK-LABEL: define spir_kernel void @test_ulong_float_NEG_INF
// CHECK: store <2 x i32> zeroinitializer, ptr addrspace(1) %dst
test_intty_with_fp_out_of_range(ulong, float, NEG_INF)
// CHECK-LABEL: define spir_kernel void @test_ulong_double_NEG_INF
// CHECK: store <2 x i32> zeroinitializer, ptr addrspace(1) %dst
test_intty_with_fp_out_of_range(ulong, double, NEG_INF)

/*////////////////////
/// Quiet NaN
*/////////////////////

// CHECK-LABEL: define spir_kernel void @test_char_half_NAN
// CHECK: store i8 0, ptr addrspace(1) %dst
test_intty_with_fp_out_of_range(char, half, NAN)
// CHECK-LABEL: define spir_kernel void @test_uchar_half_NAN
// CHECK: store i8 0, ptr addrspace(1) %dst
test_intty_with_fp_out_of_range(uchar, half, NAN)
// CHECK-LABEL: define spir_kernel void @test_short_half_NAN
// CHECK: store i16 0, ptr addrspace(1) %dst
test_intty_with_fp_out_of_range(short, half, NAN)
// CHECK-LABEL: define spir_kernel void @test_ushort_half_NAN
// CHECK: store i16 0, ptr addrspace(1) %dst
test_intty_with_fp_out_of_range(ushort, half, NAN)
// CHECK-LABEL: define spir_kernel void @test_int_half_NAN
// CHECK: store i32 0, ptr addrspace(1) %dst
test_intty_with_fp_out_of_range(int, half, NAN)
// CHECK-LABEL: define spir_kernel void @test_uint_half_NAN
// CHECK: store i32 0, ptr addrspace(1) %dst
test_intty_with_fp_out_of_range(uint, half, NAN)
/* begin i64 */
// CHECK-LABEL: define spir_kernel void @test_long_half_NAN
// CHECK: store <2 x i32> zeroinitializer, ptr addrspace(1) %dst
test_intty_with_fp_out_of_range(long, half, NAN)
// CHECK-LABEL: define spir_kernel void @test_long_float_NAN
// CHECK: store <2 x i32> zeroinitializer, ptr addrspace(1) %dst
test_intty_with_fp_out_of_range(long, float, NAN)
// CHECK-LABEL: define spir_kernel void @test_long_double_NAN
// CHECK: store <2 x i32> zeroinitializer, ptr addrspace(1) %dst
test_intty_with_fp_out_of_range(long, double, NAN)
// CHECK-LABEL: define spir_kernel void @test_ulong_half_NAN
// CHECK: store <2 x i32> zeroinitializer, ptr addrspace(1) %dst
test_intty_with_fp_out_of_range(ulong, half, NAN)
// CHECK-LABEL: define spir_kernel void @test_ulong_float_NAN
// CHECK: store <2 x i32> zeroinitializer, ptr addrspace(1) %dst
test_intty_with_fp_out_of_range(ulong, float, NAN)
// CHECK-LABEL: define spir_kernel void @test_ulong_double_NAN
// CHECK: store <2 x i32> zeroinitializer, ptr addrspace(1) %dst
test_intty_with_fp_out_of_range(ulong, double, NAN)

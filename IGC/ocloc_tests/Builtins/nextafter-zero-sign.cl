/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys, pvc-supported, llvm-16-plus
// RUN: ocloc compile -file %s -device pvc \
// RUN: -options "-cl-std=CL3.0 -igc_opts 'EnableOpaquePointersBackend=1 PrintToConsole=1 PrintBefore=EmitPass'" \
// RUN: -out_dir /dev/null 2>&1 | FileCheck %s

#pragma OPENCL EXTENSION cl_khr_fp16 : enable
#pragma OPENCL EXTENSION cl_khr_fp64 : enable

// Compare bit patterns: floating-point equality cannot distinguish signed zeros.
// Construct subnormal inputs from their bits to avoid literal underflow.
#define TEST_NEXTAFTER(TYPE, UINT, SIGN)                                                                               \
  kernel void test_##TYPE(global int *out) {                                                                           \
    TYPE neg_min = as_##TYPE((UINT)((SIGN) | 1));                                                                      \
    TYPE pos_min = as_##TYPE((UINT)1);                                                                                 \
    TYPE neg_zero = as_##TYPE((UINT)(SIGN));                                                                           \
    *out = as_##UINT(nextafter(neg_min, (TYPE)1)) == (UINT)(SIGN) &&                                                   \
           as_##UINT(nextafter(neg_min, (TYPE)0)) == (UINT)(SIGN) &&                                                   \
           as_##UINT(nextafter(neg_min, neg_zero)) == (UINT)(SIGN) &&                                                  \
           as_##UINT(nextafter(pos_min, (TYPE) - 1)) == (UINT)0 &&                                                     \
           as_##UINT(nextafter((TYPE)0, neg_zero)) == (UINT)(SIGN) &&                                                  \
           as_##UINT(nextafter(neg_zero, (TYPE)0)) == (UINT)0 &&                                                       \
           as_##UINT(nextafter((TYPE)0, (TYPE) - 1)) == (UINT)((SIGN) | 1) &&                                          \
           as_##UINT(nextafter(neg_zero, (TYPE)1)) == (UINT)1 &&                                                       \
           as_##UINT(nextafter(as_##TYPE((UINT)((SIGN) | 2)), (TYPE)1)) == (UINT)((SIGN) | 1);                         \
  }                                                                                                                    \
  kernel void test_##TYPE##_vector(global int *out) {                                                                  \
    TYPE##4 x = as_##TYPE##4((UINT##4)((SIGN) | 1, 1, 0, (SIGN)));                                                     \
    TYPE##4 y = (TYPE##4)(1, -1, as_##TYPE((UINT)(SIGN)), 0);                                                          \
    UINT##4 expected = (UINT##4)((SIGN), 0, (SIGN), 0);                                                                \
    *out = all(as_##UINT##4(nextafter(x, y)) == expected);                                                             \
  }

// CHECK-LABEL: define {{.*}}spir_kernel void @test_float(
// CHECK: store i32 1,
// CHECK-LABEL: define {{.*}}spir_kernel void @test_float_vector(
// CHECK: store i32 1,
TEST_NEXTAFTER(float, uint, 0x80000000U)

// CHECK-LABEL: define {{.*}}spir_kernel void @test_double(
// CHECK: store i32 1,
// CHECK-LABEL: define {{.*}}spir_kernel void @test_double_vector(
// CHECK: store i32 1,
TEST_NEXTAFTER(double, ulong, 0x8000000000000000UL)

// CHECK-LABEL: define {{.*}}spir_kernel void @test_half(
// CHECK: store i32 1,
// CHECK-LABEL: define {{.*}}spir_kernel void @test_half_vector(
// CHECK: store i32 1,
TEST_NEXTAFTER(half, ushort, 0x8000U)

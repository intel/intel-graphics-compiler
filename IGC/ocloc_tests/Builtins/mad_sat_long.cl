/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: pvc-supported
// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'PrintToConsole=1 PrintAfter=EmitPass'" 2>&1 | FileCheck %s

// Results are stored as <2 x i32> <low, high>.

// CHECK-LABEL: define spir_kernel void @zero(
// CHECK: store <2 x i32> <i32 7, i32 0>
kernel void zero(global long *r) { *r = mad_sat(0L, -5L, 7L); }

// CHECK-LABEL: define spir_kernel void @rescued(
// CHECK: store <2 x i32> <i32 -2, i32 2147483647>
kernel void rescued(global long *r) { *r = mad_sat(LONG_MAX, 2L, LONG_MIN); }

// CHECK-LABEL: define spir_kernel void @mul_max(
// CHECK: store <2 x i32> <i32 -1, i32 2147483647>
kernel void mul_max(global long *r) { *r = mad_sat(LONG_MAX, 2L, 0L); }

// CHECK-LABEL: define spir_kernel void @mul_min(
// CHECK: store <2 x i32> <i32 0, i32 -2147483648>
kernel void mul_min(global long *r) { *r = mad_sat(LONG_MIN, 2L, 0L); }

// CHECK-LABEL: define spir_kernel void @add_min(
// CHECK: store <2 x i32> <i32 0, i32 -2147483648>
kernel void add_min(global long *r) { *r = mad_sat(-1L, 1L, LONG_MIN); }

// CHECK-LABEL: define spir_kernel void @add_max(
// CHECK: store <2 x i32> <i32 -1, i32 2147483647>
kernel void add_max(global long *r) { *r = mad_sat(1L, 1L, LONG_MAX); }

// CHECK-LABEL: define spir_kernel void @exact_max(
// CHECK: store <2 x i32> <i32 -1, i32 2147483647>
kernel void exact_max(global long *r) { *r = mad_sat(LONG_MIN, -1L, -1L); }

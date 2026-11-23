/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys, bmg-supported

// RUN: ocloc compile -file %s -device bmg \
// RUN: -options "-igc_opts 'PrintToConsole=1 PrintBefore=igc-scalarize'" \
// RUN: -out_dir /dev/null 2>&1 | FileCheck %s

// The 64-bit sub_sat() BiF implementation handled only x > 0 and x < 0 when looking
// for overflow, so sub_sat(0, LONG_MIN) returned the wrapped 0 - LONG_MIN == LONG_MIN
// instead of LONG_MAX. The BiF now resolves to llvm.ssub.sat.i64 (via
// __builtin_IB_ssub_sat_i64), which is constant folded here, so the returned values are
// visible in the IR. Without that mapping the call would stay opaque and not fold.

// CHECK-LABEL: define spir_kernel void @test(
// CHECK: store i64 9223372036854775807, ptr addrspace(1) %out,
// CHECK: store i64 9223372036854775807,
// CHECK: store i64 9223372036854775807,
// CHECK: store i64 -9223372036854775807,
// CHECK: store i64 -9223372036854775808,
// CHECK: store i64 -9223372036854775808,

kernel void test(global long *out) {
  out[0] = sub_sat(0L, LONG_MIN);  // LONG_MAX (saturated)
  out[1] = sub_sat(1L, LONG_MIN);  // LONG_MAX (saturated)
  out[2] = sub_sat(-1L, LONG_MIN); // LONG_MAX (exact)
  out[3] = sub_sat(0L, LONG_MAX);  // -LONG_MAX (exact)
  out[4] = sub_sat(-1L, LONG_MAX); // LONG_MIN (exact)
  out[5] = sub_sat(-2L, LONG_MAX); // LONG_MIN (saturated)
}

// The vector overloads call the scalar one per element.
// CHECK-LABEL: define spir_kernel void @test_vector(
// CHECK: store <2 x i64> <i64 9223372036854775807, i64 -9223372036854775808>, ptr addrspace(1) %out,

kernel void test_vector(global long2 *out) { out[0] = sub_sat((long2)(0L, LONG_MIN), (long2)(LONG_MIN, 1L)); }

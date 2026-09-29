/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys,pvc-supported

// Verify that __builtin_expect (llvm.expect) is lowered to branch_weights
// metadata.

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'PrintToConsole=1,PrintAfter=EmitPass'" 2>&1 | FileCheck %s

// CHECK-LABEL: define spir_kernel void @test_expect
// CHECK-NOT:   call i64 @llvm.expect.i64
// CHECK:       br i1 {{.*}}, !prof [[PROF:![0-9]+]]
// CHECK-NOT:   call i64 @llvm.expect.i64
// CHECK:       [[PROF]] = !{!"branch_weights"{{(, !"expected")?}}, i32 {{[0-9]+}}, i32 {{[0-9]+}}}

double test_branch(double x) {
  if (__builtin_expect(x >= 0, 1))
    return 1.0 + (double)(1.0f / (float)x);
  x = x * x;
  x = x * x;
  return 1.0 + (double)(1.0f / (float)x);
}

kernel void test_expect(global double *a, global double *y) {
  size_t gid = get_global_id(0);
  y[gid] = test_branch(a[gid]);
}

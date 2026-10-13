/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys, dg2-supported, opaque-pointers
// RUN: ocloc compile -file %s -device dg2 -options "-cl-std=CL2.0 -cl-opt-disable -igc_opts 'EnableOpaquePointersBackend=1 PrintToConsole=1 PrintAfter=LowerByValAttribute'" 2>&1 | FileCheck %s

// Frontends don't emit a byval temp when the actual argument already lives in the private address space,
// so IGC has to make the copy implied by `byval` also when optimizations are disabled. Otherwise f1
// modifies the caller's object through f2's parameter.

typedef struct {
  int x, y[1000], z;
} S;

__attribute__((noinline)) void f1(__generic S *s1) {
  s1->x = 222;
  s1->z = 222;
}

__attribute__((noinline)) void f2(S s2) { f1(&s2); }

// CHECK-LABEL: define {{.*}} @f3(
// CHECK: [[S3:%[0-9]+]] = load ptr, ptr %s3.addr
// CHECK-NEXT: [[COPY:%[0-9]+]] = alloca %struct.S, align 4
// CHECK-NEXT: call void @llvm.memcpy.p0.p0.i64(ptr align 4 [[COPY]], ptr align 4 [[S3]], i64 4008, i1 false)
// CHECK-NEXT: call spir_func void @f2(ptr byval(%struct.S) align 4 [[COPY]])
__attribute__((noinline)) void f3(__private S *s3, __global int *out) {
  f2(*s3);
  out[0] = s3->x;
  out[1] = s3->z;
}

__kernel void test(__global int *out) {
  S s4;
  s4.x = s4.z = 111;
  f3(&s4, out);
}

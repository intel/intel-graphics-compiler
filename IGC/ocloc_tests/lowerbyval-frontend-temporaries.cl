/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys, dg2-supported, opaque-pointers
// RUN: ocloc compile -file %s -device dg2 -options "-cl-std=CL2.0 -cl-opt-disable -igc_opts 'EnableOpaquePointersBackend=1 PrintToConsole=1 PrintAfter=LowerByValAttribute'" 2>&1 | FileCheck %s --check-prefix=CHECK-O0
// RUN: ocloc compile -file %s -device dg2 -options "-cl-std=CL2.0 -igc_opts 'EnableOpaquePointersBackend=1 PrintToConsole=1 PrintAfter=LowerByValAttribute'" 2>&1 | FileCheck %s --check-prefix=CHECK-O2
// RUN: ocloc compile -file %s -device dg2 -options "-cl-std=CL2.0 -igc_opts 'EnableOpaquePointersBackend=1 DisableMergeAllocasPrivateMemory=1 PrintToConsole=1 PrintBefore=PrivateMemoryResolution'" > %t 2>&1
// RUN: FileCheck %s --input-file %t --check-prefix=PMR-PRIVATE
// RUN: FileCheck %s --input-file %t --check-prefix=PMR-LOOP
// RUN: FileCheck %s --input-file %t --check-prefix=PMR-READ
// RUN: FileCheck %s --input-file %t --check-prefix=PMR-USER-COPY
// RUN: FileCheck %s --input-file %t --check-prefix=PMR-TWICE

// Frontends make their own byval temporary when the argument isn't already a private object, e.g. when it
// lives in another address space or is a call result. LowerByValAttribute must not copy such a temporary
// again, but it still has to copy every object the caller could observe after the call.
//
// At O2 the optimizer removes many of these copies on its own, so only the cases that must keep a copy
// are checked there. The PMR-* prefixes check that each of these copies still reaches the call right before
// PrivateMemoryResolution, i.e. that no pass scheduled after LowerByValAttribute removes it. Functions are
// reordered by then, so each one is checked by a separate FileCheck run. MergeAllocasOCL is disabled for these
// checks: it may legally give a copy the slot of an object that is dead by then, and the order in which it
// considers allocas of the same size isn't fixed.

typedef struct {
  int x, y[100], z;
} S;

__attribute__((noinline)) void modify(__generic S *s) {
  s->x = 222;
  s->z = 222;
}

// Returns the value seen on entry and after the modification, so that neither the copy's contents nor the
// write into it can be optimized away.
__attribute__((noinline)) int callee(S s) {
  int before = s.x;
  modify(&s);
  return before * 1000 + s.x;
}

__attribute__((noinline)) S make(int v) {
  S s;
  s.x = v;
  return s;
}

// CHECK-O0-LABEL: define {{.*}} @from_private(
// CHECK-O0: [[COPY:%[0-9]+]] = alloca %struct.S, align 4
// CHECK-O0-NEXT: call void @llvm.memcpy.p0.p0.i64(ptr align 4 [[COPY]], ptr align 4 %{{.*}}, i64 408, i1 false)
// CHECK-O0-NEXT: call spir_func i32 @callee(ptr byval(%struct.S) align 4 [[COPY]])
// CHECK-O2-LABEL: define {{.*}} @from_private(
// CHECK-O2: [[COPY:%[0-9]+]] = alloca %struct.S, align 4
// CHECK-O2-NEXT: call void @llvm.memcpy.p0.p0.i64(ptr align 4 [[COPY]], ptr align 4 %p, i64 408, i1 false)
// CHECK-O2-NEXT: call spir_func i32 @callee(ptr {{.*}}[[COPY]],
// PMR-PRIVATE-LABEL: define {{.*}} @from_private(
// PMR-PRIVATE: [[COPY:%[0-9]+]] = alloca %struct.S
// PMR-PRIVATE-NOT: {{^}}define
// PMR-PRIVATE: getelementptr {{.*}}, ptr [[COPY]],
// PMR-PRIVATE-NOT: {{^}}define
// PMR-PRIVATE: call spir_func i32 @callee(ptr {{(nonnull )?}}[[COPY]],
__attribute__((noinline)) int from_private(__private S *p) { return callee(*p); }

// CHECK-O0-LABEL: define {{.*}} @from_generic(
// CHECK-O0: [[TMP:%[A-Za-z0-9._-]+]] = alloca %struct.S
// CHECK-O0-NOT: alloca %struct.S
// CHECK-O0: call void @llvm.memcpy.p0.p4.i{{32|64}}(
// CHECK-O0-NEXT: call spir_func i32 @callee(ptr byval(%struct.S) align 4 [[TMP]])
__attribute__((noinline)) int from_generic(__generic S *p) { return callee(*p); }

// CHECK-O0-LABEL: define {{.*}} @from_global(
// CHECK-O0: [[TMP:%[A-Za-z0-9._-]+]] = alloca %struct.S
// CHECK-O0-NOT: alloca %struct.S
// CHECK-O0: call void @llvm.memcpy.p0.p1.i{{32|64}}(
// CHECK-O0-NEXT: call spir_func i32 @callee(ptr byval(%struct.S) align 4 [[TMP]])
__attribute__((noinline)) int from_global(__global S *p) { return callee(*p); }

// CHECK-O0-LABEL: define {{.*}} @from_constant(
// CHECK-O0: [[TMP:%[A-Za-z0-9._-]+]] = alloca %struct.S
// CHECK-O0-NOT: alloca %struct.S
// CHECK-O0: call void @llvm.memcpy.p0.p2.i{{32|64}}(
// CHECK-O0-NEXT: call spir_func i32 @callee(ptr byval(%struct.S) align 4 [[TMP]])
__attribute__((noinline)) int from_constant(__constant S *p) { return callee(*p); }

// CHECK-O0-LABEL: define {{.*}} @from_call(
// CHECK-O0: [[TMP:%[A-Za-z0-9._-]+]] = alloca %struct.S
// CHECK-O0-NOT: alloca %struct.S
// CHECK-O0: call spir_func void @make(ptr {{.*}}sret(%struct.S) align 4 [[TMP]],
// CHECK-O0-NEXT: call spir_func i32 @callee(ptr byval(%struct.S) align 4 [[TMP]])
__attribute__((noinline)) int from_call(int v) { return callee(make(v)); }

// CHECK-O0-LABEL: define {{.*}} @read_after_call(
// CHECK-O0: [[COPY:%[0-9]+]] = alloca %struct.S, align 4
// CHECK-O0-NEXT: call void @llvm.memcpy.p0.p0.i64(ptr align 4 [[COPY]], ptr align 4 %s, i64 408, i1 false)
// CHECK-O0-NEXT: call spir_func i32 @callee(ptr byval(%struct.S) align 4 [[COPY]])
// PMR-READ-LABEL: define {{.*}} @read_after_call(
// PMR-READ: [[COPY:%[0-9]+]] = alloca %struct.S
// PMR-READ-NOT: {{^}}define
// PMR-READ: getelementptr {{.*}}, ptr [[COPY]],
// PMR-READ-NOT: {{^}}define
// PMR-READ: call spir_func i32 @callee(ptr {{(nonnull )?}}[[COPY]],
__attribute__((noinline)) int read_after_call(int v) {
  S s;
  s.x = v;
  callee(s);
  return s.x;
}

// A user's copy looks like a frontend temporary (alloca + memcpy), but it's read after the call.
// CHECK-O0-LABEL: define {{.*}} @user_copy_read_after_call(
// CHECK-O0: call void @llvm.memcpy.p0.p0.i{{32|64}}(
// CHECK-O0: [[COPY:%[0-9]+]] = alloca %struct.S, align 4
// CHECK-O0-NEXT: call void @llvm.memcpy.p0.p0.i64(ptr align 4 [[COPY]], ptr align 4 %s2, i64 408, i1 false)
// CHECK-O0-NEXT: call spir_func i32 @callee(ptr byval(%struct.S) align 4 [[COPY]])
// PMR-USER-COPY-LABEL: define {{.*}} @user_copy_read_after_call(
// PMR-USER-COPY: [[COPY:%[0-9]+]] = alloca %struct.S
// PMR-USER-COPY-NOT: {{^}}define
// PMR-USER-COPY: getelementptr {{.*}}, ptr [[COPY]],
// PMR-USER-COPY-NOT: {{^}}define
// PMR-USER-COPY: call spir_func i32 @callee(ptr {{(nonnull )?}}[[COPY]],
__attribute__((noinline)) int user_copy_read_after_call(int v) {
  S s1;
  s1.x = v;
  S s2 = s1;
  callee(s2);
  return s2.x;
}

// CHECK-O0-LABEL: define {{.*}} @passed_twice(
// CHECK-O0: [[COPY0:%[0-9]+]] = alloca %struct.S, align 4
// CHECK-O0-NEXT: call void @llvm.memcpy.p0.p0.i64(ptr align 4 [[COPY0]], ptr align 4 %s, i64 408, i1 false)
// CHECK-O0-NEXT: call spir_func i32 @callee(ptr byval(%struct.S) align 4 [[COPY0]])
// CHECK-O0: [[COPY1:%[0-9]+]] = alloca %struct.S, align 4
// CHECK-O0-NEXT: call void @llvm.memcpy.p0.p0.i64(ptr align 4 [[COPY1]], ptr align 4 %s, i64 408, i1 false)
// CHECK-O0-NEXT: call spir_func i32 @callee(ptr byval(%struct.S) align 4 [[COPY1]])
// PMR-TWICE-LABEL: define {{.*}} @passed_twice(
// PMR-TWICE-NOT: {{^}}define
// PMR-TWICE: call spir_func i32 @callee(ptr {{(nonnull )?}}%{{[0-9]+}},
// PMR-TWICE-NOT: {{^}}define
// PMR-TWICE: call spir_func i32 @callee(ptr {{(nonnull )?}}%{{[0-9]+}},
__attribute__((noinline)) int passed_twice(int v) {
  S s;
  s.x = v;
  int r = callee(s);
  return r + callee(s);
}

// The frontend refills its temporary right before every call in the loop, so it isn't copied again.
// CHECK-O0-LABEL: define {{.*}} @generic_in_loop(
// CHECK-O0: [[TMP:%[A-Za-z0-9._-]+]] = alloca %struct.S
// CHECK-O0-NOT: alloca %struct.S
// CHECK-O0: for.body:
// CHECK-O0: call void @llvm.memcpy.p0.p4.i{{32|64}}(
// CHECK-O0-NEXT: call spir_func i32 @callee(ptr byval(%struct.S) align 4 [[TMP]])
__attribute__((noinline)) int generic_in_loop(__generic S *p, int n) {
  int r = 0;
  for (int i = 0; i < n; ++i)
    r += callee(*p);
  return r;
}

// The caller's object is passed directly, so every iteration needs its own copy.
// CHECK-O0-LABEL: define {{.*}} @private_in_loop(
// CHECK-O0: for.body:
// CHECK-O0: [[COPY:%[0-9]+]] = alloca %struct.S, align 4
// CHECK-O0-NEXT: call void @llvm.memcpy.p0.p0.i64(ptr align 4 [[COPY]], ptr align 4 %{{.*}}, i64 408, i1 false)
// CHECK-O0-NEXT: call spir_func i32 @callee(ptr byval(%struct.S) align 4 [[COPY]])
// CHECK-O2-LABEL: define {{.*}} @private_in_loop(
// CHECK-O2: [[COPY:%[0-9]+]] = alloca %struct.S, align 4
// CHECK-O2-NEXT: call void @llvm.memcpy.p0.p0.i64(ptr align 4 [[COPY]], ptr align 4 %p, i64 408, i1 false)
// CHECK-O2-NEXT: call spir_func i32 @callee(ptr {{.*}}[[COPY]],
// PMR-LOOP-LABEL: define {{.*}} @private_in_loop(
// PMR-LOOP: [[COPY:%[0-9]+]] = alloca %struct.S
// PMR-LOOP-NOT: {{^}}define
// PMR-LOOP: getelementptr {{.*}}, ptr [[COPY]],
// PMR-LOOP-NOT: {{^}}define
// PMR-LOOP: call spir_func i32 @callee(ptr {{(nonnull )?}}[[COPY]],
__attribute__((noinline)) int private_in_loop(__private S *p, int n) {
  int r = 0;
  for (int i = 0; i < n; ++i)
    r += callee(*p);
  return r;
}

__constant S cs = {1};

__kernel void test(__global S *g, __global int *out, int v, int n) {
  S s;
  s.x = v;
  out[0] = from_private(&s);
  out[1] = from_generic(&s);
  out[2] = from_global(g);
  out[3] = from_constant(&cs);
  out[4] = from_call(v);
  out[5] = read_after_call(v);
  out[6] = user_copy_read_after_call(v);
  out[7] = passed_twice(v);
  out[8] = generic_in_loop(&s, n);
  out[9] = private_in_loop(&s, n);
  out[10] = s.x;
}

/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys, dg2-supported, llvm-16-plus
// RUN: ocloc compile -file %s -device dg2 -options "-igc_opts 'EnableOpaquePointersBackend=1 PrintToConsole=1 PrintAfter=StatelessToStateful'" 2>&1 | FileCheck %s

// Accesses through a kernel argument without any getelementptr are promoted to stateful.
// The char pointers aren't known to be DW-aligned, so the buffer offset is added for them.

// CHECK-LABEL: define spir_kernel void @test(
// CHECK: [[BUF:%[0-9]+]] = inttoptr i32 %bindlessOffset to ptr addrspace([[AS:[0-9]+]])
// CHECK-NEXT: call void @llvm.genx.GenISA.storeraw.indexed.p[[AS]].i32(ptr addrspace([[AS]]) [[BUF]], i32 0, i32 -1, i32 4, i1 false)
// CHECK-NEXT: [[IN:%[0-9]+]] = inttoptr i32 %bindlessOffset{{[0-9]+}} to ptr addrspace([[AS]])
// CHECK-NEXT: [[VAL:%[0-9]+]] = call i8 @llvm.genx.GenISA.ldraw.indexed.i8.p[[AS]](ptr addrspace([[AS]]) [[IN]], i32 %bufferOffset{{[0-9]+}}, i32 1, i1 false)
// CHECK-NEXT: [[OUT:%[0-9]+]] = inttoptr i32 %bindlessOffset{{[0-9]+}} to ptr addrspace([[AS]])
// CHECK-NEXT: call void @llvm.genx.GenISA.storeraw.indexed.p[[AS]].i8(ptr addrspace([[AS]]) [[OUT]], i32 %bufferOffset{{[0-9]+}}, i8 [[VAL]], i32 1, i1 false)
// CHECK-NEXT: ret void

kernel void test(global int *buffer, global char *in, global char *out) {
  buffer[0] = 0xffffffff;
  out[0] = in[0];
}

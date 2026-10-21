/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys, pvc-supported, llvm-16-plus
// UNSUPPORTED: sys32
// RUN: ocloc compile -file %s -device pvc -out_dir %t.pvc -options "-cl-std=CL2.0 -igc_opts 'PrintToConsole=1 PrintAfter=Layout'" > %t.log 2>&1
// RUN: FileCheck %s < %t.log
// RUN: FileCheck %s --check-prefix=I64 < %t.log
// RUN: FileCheck %s --check-prefix=I32 < %t.log
// RUN: ocloc compile -file %s -device pvc -out_dir %t.pvc.o0 -options "-cl-std=CL2.0 -cl-opt-disable -igc_opts 'PrintToConsole=1 PrintAfter=Layout'" > %t.o0.log 2>&1
// RUN: FileCheck %s < %t.o0.log
// RUN: FileCheck %s --check-prefix=I64 < %t.o0.log
// RUN: FileCheck %s --check-prefix=I32 < %t.o0.log

// Clustered reduce with cluster size 1 returns its input:
// - i64 on PVC (no full int64) must not generate the emulated reduction loop (WaveBallot),
// - i32 must not reach WaveClustered with cluster size 1,
// - the loaded value is stored unchanged.
// Kernel order differs between optimization levels, so each kernel is checked separately.
// CHECK-NOT: GenISA.WaveBallot
// CHECK-NOT: GenISA.WaveClustered
// CHECK: Build succeeded.

// I64: [[VAL:%.*]] = load i64, ptr addrspace(1)
// I64: [[VEC:%.*]] = bitcast i64 [[VAL]] to <2 x i32>
// I64: store <2 x i32> [[VEC]], ptr addrspace(1)

// I32: [[VAL:%.*]] = load i32, ptr addrspace(1)
// I32: store i32 [[VAL]], ptr addrspace(1)

__attribute__((intel_reqd_sub_group_size(16))) kernel void size1_i64(global const long *input, global long *output) {
  size_t id = get_global_id(0);
  output[id] = sub_group_clustered_reduce_add(input[id], 1);
}

__attribute__((intel_reqd_sub_group_size(16))) kernel void size1_i32(global const int *input, global int *output) {
  size_t id = get_global_id(0);
  output[id] = sub_group_clustered_reduce_add(input[id], 1);
}

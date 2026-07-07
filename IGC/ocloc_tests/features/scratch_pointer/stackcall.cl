/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// Verify that scratch_pointer is not removed if kernel uses stackcalls.

// REQUIRES: cri-supported
// RUN: ocloc compile -file %s -options "-igc_opts 'DumpZEInfoToConsole=1, RemoveUnusedIdImplicitArguments=1, RemoveImplicitScratchPointer=1, EnableStackCallFuncCall=1'" -device cri | FileCheck %s --check-prefix=CHECK

// CHECK:      has_stack_calls: true
// CHECK:      payload_arguments:
// CHECK-NEXT: - arg_type:        indirect_data_pointer
// CHECK-NEXT:   offset:          0
// CHECK-NEXT:   size:            8
// CHECK-NEXT: - arg_type:        scratch_pointer
// CHECK-NEXT:   offset:          8
// CHECK-NEXT:   size:            8
// CHECK:      per_thread_memory_buffers:
// CHECK-NEXT: - type:            scratch
//
// CHECK-NOT:      Start recompilation of the kernel

__attribute__((noinline))
double no_inline_func(double a) {
    return a + 39.0f;
}

kernel void foo(global double *a1, global double *a2) {
    int tid = get_global_id(0);
    a1[tid] = no_inline_func(a1[tid]);
}

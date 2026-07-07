/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// Verify that scratch_pointer is not removed if kernel uses private memory.
// Disable loop unroll so that the private memory is not optimized out.

// REQUIRES: cri-supported
// RUN: ocloc compile -file %s -options "-igc_opts 'DumpZEInfoToConsole=1, RemoveUnusedIdImplicitArguments=1, RemoveImplicitScratchPointer=1, DisableLoopUnroll=1'" -device cri | FileCheck %s --check-prefix=CHECK

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

void kernel test(global float* a1, global float* a2) {

    float b[1024];

    int tid = get_global_id(0);

    for (int i = 0; i < 1024; i++) {
        b[i] = a2[i];
    }

    float sum = 0.0f;
    for (int i = 0; i < 1024; i++) {
        sum += b[i];
    }

    a1[tid] = sum;
}

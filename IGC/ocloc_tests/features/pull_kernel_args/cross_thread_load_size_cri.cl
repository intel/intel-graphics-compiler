/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// Checks zeinfo execution_env.cross_thread_load_size for kernels with pulled arguments.
// It is the end offset of the pushed arguments (inline data included). Pulled arguments start at or after it.
// Kernels without pulled arguments don't emit it (0 means the whole payload is in GRFs at kernel entry).

// REQUIRES: regkeys, cri-supported

// RUN: ocloc compile -file %s -device cri -64 -options "-igc_opts 'PullKernelArgs=1,DumpZEInfoToConsole=1'" | FileCheck %s

// 4000 B, over the 2048 B limit of pushed arguments
typedef struct {
  int data[1000];
} Big;

// CRI has 64 B of inline data. The last pushed argument is a (offset 48, 4 B), so pushed arguments end at 52, inside
// inline data. The value is the inline data size and the prologue loads no arguments.
// big does not fit in the limit and is pulled, b is pulled as it comes after big.
kernel void k_pulled(global int *out, int a, Big big, int b) {
  size_t gid = get_global_id(0);
  out[gid] = a + big.data[gid] + b;
}

// CHECK-LABEL: - name: k_pulled
// CHECK:       execution_env:
// CHECK:         cross_thread_load_size: 64
// CHECK:         inline_data_payload_size: 64
// CHECK:       payload_arguments:
// CHECK:           offset: 64{{$}}
// CHECK-NEXT:      size: 4000


// k_pushed has only pushed arguments, so cross_thread_load_size is not emitted
kernel void k_pushed(global int *out, int a) {
  out[get_global_id(0)] = a;
}

// CHECK-LABEL: - name: k_pushed
// CHECK-NOT:     cross_thread_load_size
// CHECK:       payload_arguments:

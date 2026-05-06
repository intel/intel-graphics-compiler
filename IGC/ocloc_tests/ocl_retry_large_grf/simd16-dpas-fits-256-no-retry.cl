/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys, cri-supported

// A SIMD16 DPAS kernel with low register pressure that fits comfortably in 256
// GRF.  With EnableOCLRetry512GRF=1 the gate is armed but the first attempt
// succeeds without spilling, so the retry mechanism must NOT fire and no _1_
// retry asm should appear in the dump directory.
//
// RUN: ocloc compile -file %s -device cri \
// RUN: -options "-igc_opts 'EnableOCLRetry512GRF=1,DumpASMToConsole=1'" \
// RUN: 2>&1 | FileCheck %s \
// RUN:   --implicit-check-not="[RetryManager] Start recompilation of the kernel"

// COMPILE: Build succeeded.

// The first-attempt asm must exist and declare its GRF allocation.
// CHECK: numGRF=
// CHECK: Build succeeded.

// Low-pressure DPAS kernel: single int8 accumulator, 4-iteration loop.
// Identical to the baseline-default-off pattern — easily within 256 GRF.
__attribute__((intel_reqd_sub_group_size(16))) __kernel void small_dpas(__global short *A, __global int *B,
                                                                        __global int8 *C) {
  int i = get_global_id(0);
  int8 acc = (int8)(0);
  for (int k = 0; k < 4; ++k) {
    short8 a = (short8)(A[i + k]);
    int8 b = (int8)(B[i + k]);
    acc = intel_sub_group_i8_i8_matrix_mad_k32(a, b, acc);
  }
  C[i] = acc;
}

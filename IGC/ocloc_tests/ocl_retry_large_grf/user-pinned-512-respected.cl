/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys, cri-supported

// The first-attempt asm should show -maxGRFNum 512 (the user's pin) and the
// build must succeed without any gate-induced changes.

// RUN: ocloc compile -file %s -device cri \
// RUN: -options "-cl-intel-512-GRF-per-thread -igc_opts 'EnableOCLRetry512GRF=1,DumpASMToConsole=1'" \
// RUN: 2>&1 | FileCheck %s --check-prefix=COMPILE --implicit-check-not "error:"

// The asm must show the user's 512-GRF pin: numGRF=512 in .thread_config and
// TotalGRFNum 512 in .full_options, both printed by DumpASMToConsole.
// COMPILE: numGRF=512
// COMPILE: TotalGRFNum 512
// COMPILE: Build succeeded.

__attribute__((intel_reqd_sub_group_size(16))) __kernel void user512(__global short *A, __global int *B,
                                                                     __global int8 *C) {
  int i = get_global_id(0);
  int8 acc = (int8)(0);
  short8 a = (short8)(A[i]);
  int8 b = (int8)(B[i]);
  acc = intel_sub_group_i8_i8_matrix_mad_k32(a, b, acc);
  C[i] = acc;
}

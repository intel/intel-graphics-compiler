/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys, cri-supported

// A spilling kernel with a required sub-group size must still be retried when
// its only wave intrinsics are a reduction (WaveAll) and shuffle/broadcast with
// a constant lane: a constant lane is a direct region, not an indirect operand.

// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'TotalGRFNum=128'" 2>&1 | FileCheck %s
// CHECK: [RetryManager] Start recompilation of the kernel
// CHECK: Build succeeded.

#define CHAIN(N)                                                                                                       \
  int16 v##N = vload16(0, in + 16 * N + i);                                                                            \
  int16 w##N = vload16(0, in + 16 * N + i + 1024)

#define ITER(N)                                                                                                        \
  v##N = v##N * w##N + (int16)(k);                                                                                     \
  w##N = w##N + v##N

#define STORE(N) vstore16(v##N + w##N, 0, out + 16 * N + i)

__attribute__((intel_reqd_sub_group_size(16))) __kernel void heavy_alu_wave(__global int *in, __global int *out,
                                                                            __global int *res) {
  int i = get_global_id(0);
  CHAIN(0);
  CHAIN(1);
  CHAIN(2);
  CHAIN(3);
  CHAIN(4);
  CHAIN(5);
  CHAIN(6);
  CHAIN(7);

  for (int k = 0; k < 8; ++k) {
    ITER(0);
    ITER(1);
    ITER(2);
    ITER(3);
    ITER(4);
    ITER(5);
    ITER(6);
    ITER(7);
  }
  int red = sub_group_reduce_add(v0.s0);
  int shf = intel_sub_group_shuffle(w1.s0, 0);
  int bc = sub_group_broadcast(v2.s0, 0);
  res[i] = red + shf + bc;
  STORE(0);
  STORE(1);
  STORE(2);
  STORE(3);
  STORE(4);
  STORE(5);
  STORE(6);
  STORE(7);
}

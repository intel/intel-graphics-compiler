/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys, cri-supported

// Per-kernel 512-GRF lift: two SIMD16 high-pressure kernels in one module that
// both spill at 256 GRF on the first attempt, so the module recompiles. With
// only the DPAS category enabled (the default), the lift is granted per-kernel:
//   - kA_dpas    (DPAS)    qualifies -> ceiling lifted to 512 on retry,
//   - kB_no_dpas (no DPAS) does NOT qualify -> stays at 256 and keeps spilling.
// This proves the decision is taken per-kernel rather than all-or-nothing for
// the module.
//
// RUN: ocloc compile -file %s -device cri \
// RUN: -options "-igc_opts 'EnableOCL512GRFForDPAS=1,EnableOCL512GRFForSIMD16=0,DumpASMToConsole=1'" \
// RUN: 2>&1 | FileCheck %s

// kA_dpas is the DPAS kernel: its retry is compiled with the 512-GRF ceiling.
// CHECK: //.kernel kA_dpas
// CHECK: -TotalGRFNum 512 -maxGRFNum 512

// kB_no_dpas has no DPAS and the SIMD16 category is off, so it is never lifted:
// it is compiled at 256 GRF and no 512 dump appears for it through end of output.
// CHECK: //.kernel kB_no_dpas
// CHECK: numGRF=256
// CHECK-NOT: numGRF=512

// CHECK: [RetryManager] Start recompilation of the kernel
// CHECK: Build succeeded.

// clang-format off
#define MM(N) acc##N = intel_sub_group_i8_i8_matrix_mad_k32(a##N, b##N, acc##N);
__attribute__((intel_reqd_sub_group_size(16)))
__kernel void kA_dpas(__global short *in_a, __global int *in_b, __global int8 *out) {
  int i = get_global_id(0);
#define LD(N) short8 a##N = (short8)(in_a[i + 8 * N]); int8 b##N = (int8)(in_b[i + 8 * N + 1]);
  LD(0)  LD(1)  LD(2)  LD(3)  LD(4)  LD(5)  LD(6)  LD(7)
  LD(8)  LD(9)  LD(10) LD(11) LD(12) LD(13) LD(14) LD(15)
  LD(16) LD(17) LD(18) LD(19) LD(20) LD(21) LD(22) LD(23)
#define INIT(N) int8 acc##N = (int8)(in_b[i + 1000 + N]);
  INIT(0)  INIT(1)  INIT(2)  INIT(3)  INIT(4)  INIT(5)  INIT(6)  INIT(7)
  INIT(8)  INIT(9)  INIT(10) INIT(11) INIT(12) INIT(13) INIT(14) INIT(15)
  INIT(16) INIT(17) INIT(18) INIT(19) INIT(20) INIT(21) INIT(22) INIT(23)
  for (int k = 0; k < 8; ++k) {
    MM(0)  MM(1)  MM(2)  MM(3)  MM(4)  MM(5)  MM(6)  MM(7)
    MM(8)  MM(9)  MM(10) MM(11) MM(12) MM(13) MM(14) MM(15)
    MM(16) MM(17) MM(18) MM(19) MM(20) MM(21) MM(22) MM(23)
  }
  out[i +  0] = acc0  + acc1  + acc2  + acc3  + acc4  + acc5  + acc6  + acc7;
  out[i +  8] = acc8  + acc9  + acc10 + acc11 + acc12 + acc13 + acc14 + acc15;
  out[i + 16] = acc16 + acc17 + acc18 + acc19 + acc20 + acc21 + acc22 + acc23;
}

#define CHAIN(N) \
  int16 v##N = vload16(0, in + 16 * N + i); \
  int16 w##N = vload16(0, in + 16 * N + i + 1024);
#define ITER(N) \
  v##N = v##N * w##N + (int16)(k); \
  w##N = w##N + v##N;
#define STORE(N) \
  vstore16(v##N + w##N, 0, out + 16 * N + i);

__attribute__((intel_reqd_sub_group_size(16)))
__kernel void kB_no_dpas(__global int *in, __global int *out) {
  int i = get_global_id(0);
  CHAIN(0)  CHAIN(1)  CHAIN(2)  CHAIN(3)  CHAIN(4)  CHAIN(5)  CHAIN(6)  CHAIN(7)
  CHAIN(8)  CHAIN(9)  CHAIN(10) CHAIN(11) CHAIN(12) CHAIN(13) CHAIN(14) CHAIN(15)
  CHAIN(16) CHAIN(17) CHAIN(18) CHAIN(19) CHAIN(20) CHAIN(21) CHAIN(22) CHAIN(23)
  CHAIN(24) CHAIN(25) CHAIN(26) CHAIN(27) CHAIN(28) CHAIN(29) CHAIN(30) CHAIN(31)
  CHAIN(32) CHAIN(33) CHAIN(34) CHAIN(35) CHAIN(36) CHAIN(37) CHAIN(38) CHAIN(39)
  CHAIN(40) CHAIN(41) CHAIN(42) CHAIN(43) CHAIN(44) CHAIN(45) CHAIN(46) CHAIN(47)
  for (int k = 0; k < 8; ++k) {
    ITER(0)  ITER(1)  ITER(2)  ITER(3)  ITER(4)  ITER(5)  ITER(6)  ITER(7)
    ITER(8)  ITER(9)  ITER(10) ITER(11) ITER(12) ITER(13) ITER(14) ITER(15)
    ITER(16) ITER(17) ITER(18) ITER(19) ITER(20) ITER(21) ITER(22) ITER(23)
    ITER(24) ITER(25) ITER(26) ITER(27) ITER(28) ITER(29) ITER(30) ITER(31)
    ITER(32) ITER(33) ITER(34) ITER(35) ITER(36) ITER(37) ITER(38) ITER(39)
    ITER(40) ITER(41) ITER(42) ITER(43) ITER(44) ITER(45) ITER(46) ITER(47)
  }
  STORE(0)  STORE(1)  STORE(2)  STORE(3)  STORE(4)  STORE(5)  STORE(6)  STORE(7)
  STORE(8)  STORE(9)  STORE(10) STORE(11) STORE(12) STORE(13) STORE(14) STORE(15)
  STORE(16) STORE(17) STORE(18) STORE(19) STORE(20) STORE(21) STORE(22) STORE(23)
  STORE(24) STORE(25) STORE(26) STORE(27) STORE(28) STORE(29) STORE(30) STORE(31)
  STORE(32) STORE(33) STORE(34) STORE(35) STORE(36) STORE(37) STORE(38) STORE(39)
  STORE(40) STORE(41) STORE(42) STORE(43) STORE(44) STORE(45) STORE(46) STORE(47)
}
// clang-format on

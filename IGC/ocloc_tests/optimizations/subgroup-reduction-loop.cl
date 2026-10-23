/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// Sub-group reduction written as a shuffle-xor loop. The pattern is visible
// only after the loop is unrolled, so SubGroupReductionPattern must run after
// loop unroll.

// REQUIRES: regkeys, bmg-supported

// RUN: ocloc compile -file %s -device bmg -options "-igc_opts 'PrintToConsole=1, PrintAfter=EmitPass'" 2>&1 | FileCheck %s

// CHECK: define spir_kernel void @test
// CHECK-NOT: WaveShuffleIndex
// CHECK-NOT: simdShuffleXor
// CHECK: call float @llvm.genx.GenISA.WaveAll.f32
// CHECK-NOT: WaveShuffleIndex
// CHECK-NOT: simdShuffleXor
// CHECK: ret void

__attribute__((intel_reqd_sub_group_size(32)))
kernel void test(global float *in, global float *out) {
  float v = in[get_global_id(0)];
  for (int mask = 16; mask > 0; mask >>= 1)
    v += sub_group_shuffle_xor(v, mask);
  out[get_global_id(0)] = v;
}

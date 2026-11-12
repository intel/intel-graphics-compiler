/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// SIMD32 shuffle xor with bit 4 set moves data between the SIMD16 halves.
// It's lowered to simdShuffleXor only where SIMD32 is a single instance.
// DG2 emits SIMD32 as two SIMD16 instances and keeps the generic shuffle.

// REQUIRES: regkeys, bmg-supported, dg2-supported

// RUN: ocloc compile -file %s -device bmg -options "-igc_opts 'PrintToConsole=1, PrintAfter=EmitPass'" 2>&1 | FileCheck %s --check-prefix=BMG
// RUN: ocloc compile -file %s -device dg2 -options "-igc_opts 'PrintToConsole=1, PrintAfter=EmitPass'" 2>&1 | FileCheck %s --check-prefix=DG2

// BMG: define spir_kernel void @test
// BMG-NOT: call {{.*}}WaveShuffleIndex
// BMG: call i32 @llvm.genx.GenISA.simdShuffleXor.i32(i32 {{.*}}, i32 17)
// BMG-NOT: call {{.*}}WaveShuffleIndex
// BMG: ret void

// DG2: define spir_kernel void @test
// DG2-NOT: call {{.*}}simdShuffleXor
// DG2: call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32
// DG2-NOT: call {{.*}}simdShuffleXor
// DG2: ret void

__attribute__((intel_reqd_sub_group_size(32)))
kernel void test(global int *out) {
  int lane = get_sub_group_local_id();
  out[get_global_id(0)] = sub_group_shuffle_xor(lane, 17u);
}

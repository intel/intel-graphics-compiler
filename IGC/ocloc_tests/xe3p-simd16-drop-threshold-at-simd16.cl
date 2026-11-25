/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This test checks that on xe3p the EarlySIMD16DropForXE3Threshold fallback of the SIMD16 drop
// compares the register pressure at SIMD16, not at SIMD32. The kernel keeps 40 float4 values
// live across a loop: about 170 GRFs at SIMD16 and 340 at SIMD32.
// With the threshold at 256 the kernel stays SIMD32, although its SIMD32 pressure is above 256.
// With the threshold at 128 it drops to SIMD16.
// AllowSIMD16DropForXE2Plus and EarlySIMD16DropForXE3Threshold are unavailable in Release builds.
// UNSUPPORTED: system-windows, release
// REQUIRES: regkeys, cri-supported

// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'AllowSIMD16DropForXE2Plus=1, EarlySIMD16DropForXE3Threshold=256, DumpZEInfoToConsole=1'" 2>&1 | FileCheck %s --check-prefix=CHECK-SIMD32
// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'AllowSIMD16DropForXE2Plus=1, EarlySIMD16DropForXE3Threshold=128, DumpZEInfoToConsole=1'" 2>&1 | FileCheck %s --check-prefix=CHECK-SIMD16

// CHECK-SIMD32-LABEL: - name: foo
// CHECK-SIMD32: simd_size: 32

// CHECK-SIMD16-LABEL: - name: foo
// CHECK-SIMD16: simd_size: 16

#define def(N) float4 float_var_##N = {1 + tid, 2 + tid, 3 + tid, 4 + tid};
#define incf4(N) float_var_##N += (float4){4, 3, 2, 1};
#define wrt(N) result[N] = float_var_##N;
// clang-format off
#define ALL(M) \
    M(1); M(2); M(3); M(4); M(5); M(6); M(7); M(8); M(9); M(10); \
    M(11); M(12); M(13); M(14); M(15); M(16); M(17); M(18); M(19); M(20); \
    M(21); M(22); M(23); M(24); M(25); M(26); M(27); M(28); M(29); M(30); \
    M(31); M(32); M(33); M(34); M(35); M(36); M(37); M(38); M(39); M(40);
// clang-format on

__kernel void foo(float4 __global *result) {
  int tid = get_global_id(0);

  ALL(def)

#pragma nounroll
  for (int i = 0; i < 1000; i++) {
    ALL(incf4)
  }

  ALL(wrt)
}

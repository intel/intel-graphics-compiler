/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys, cri-supported

// On CRI the default VRT GRF ceiling is 512, so a high-pressure SIMD16 DPAS
// kernel with 24 independent accumulator chains reaches 512 GRF by default with
// no 512-GRF regkey set. An explicit ForceVRTGRFCeiling override is honored over
// the raised default, in either direction.

// Default (no override): VRT picks the raised 512 ceiling.
// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'DumpASMToConsole=1'" \
// RUN: 2>&1 | FileCheck %s --check-prefix=DEFAULT

// DEFAULT: numGRF=512
// DEFAULT: Build succeeded.

// Forcing the ceiling to 256 wins over the raised default: held at 256 GRF.
// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'ForceVRTGRFCeiling=256,DumpASMToConsole=1'" \
// RUN: 2>&1 | FileCheck %s --check-prefix=FORCE256 --implicit-check-not "numGRF=512"

// FORCE256: numGRF=256
// FORCE256: Build succeeded.

// Forcing the ceiling to 512 is honored.
// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'ForceVRTGRFCeiling=512,DumpASMToConsole=1'" \
// RUN: 2>&1 | FileCheck %s --check-prefix=FORCE512

// FORCE512: numGRF=512
// FORCE512: Build succeeded.

#define MM(N) acc##N = intel_sub_group_i8_i8_matrix_mad_k32(a##N, b##N, acc##N);

// clang-format off
__attribute__((intel_reqd_sub_group_size(16)))
__kernel void heavy_dpas(__global short *in_a, __global int *in_b, __global int8 *out) {
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
// clang-format on

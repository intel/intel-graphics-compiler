/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys, cri-supported

// The kernel is high-RP enough that it would otherwise be promoted on retry
// (DPAS + intel_reqd_sub_group_size(16) satisfies every other gate). With
// -cl-intel-256-GRF-per-thread set, both the first attempt and the retry must
// stay at 256 GRF.

// RUN: ocloc compile -file %s -device cri \
// RUN: -options "-cl-intel-256-GRF-per-thread -igc_opts 'EnableOCLRetry512GRF=1,DumpASMToConsole=1'" \
// RUN: 2>&1 | FileCheck %s --check-prefix=COMPILE --implicit-check-not "numGRF=512"

// COMPILE: numGRF=256
// COMPILE: Build succeeded.

// clang-format off
#define CHAIN(N) \
  int16 v##N = vload16(0, in + 16 * N + i); \
  int16 w##N = vload16(0, in + 16 * N + i + 1024);

#define ITER(N) \
  v##N = v##N * w##N + (int16)(k); \
  w##N = w##N + v##N;

#define STORE(N) \
  vstore16(v##N + w##N, 0, out + 16 * N + i);

__attribute__((intel_reqd_sub_group_size(16)))
__kernel void high_rp_pinned_256(__global int *in, __global int *out) {
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
;

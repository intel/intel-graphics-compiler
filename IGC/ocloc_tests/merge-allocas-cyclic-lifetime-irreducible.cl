/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: cri-supported

// RUN: ocloc compile -file %s -device cri 2>&1 | FileCheck %s

// The cycle below has two entries, so LoopInfo does not report it as a loop at all. An allocation
// escaping inside it makes the live range cyclic, which used to make MergeAllocas assert with
// "No lifetime start block?", or crash in release builds.

// CHECK: Build succeeded

kernel void irreducible_cycle(__global ulong *out, int n, int m) {
  int i = 0;
  if (n & 1)
    goto B;
A: {
  ulong buf[4];
  ulong k = (n + i) & 3;
  buf[k] = i;
  out[k] = (ulong)(__private ulong *)&buf[k];
}
B:
  i++;
  if (i < m)
    goto A;
}

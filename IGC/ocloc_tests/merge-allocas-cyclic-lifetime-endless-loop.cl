/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: cri-supported, regkeys

// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'EnableOpaquePointersBackend=1'" 2>&1 | FileCheck %s

// An allocation escaping inside an endless loop makes its live range cyclic, which used to make
// MergeAllocas assert with "No lifetime start block?", or crash in release builds.
//
// Opaque pointers are forced on purpose, hence the regkeys requirement: with typed pointers LICM
// hoists the lifetime-marker bitcast into the entry block, which then dominates all the uses and
// hides the problem.

// CHECK: Build succeeded

kernel void endless_loop(__global ulong *out, int n) {
  ulong i = 0;
  while (1) {
    ulong buf[4];
    ulong k = (n + i) & 3;
    buf[k] = i;
    out[i & 7] = (ulong)(__private ulong *)&buf[k];
    i++;
  }
}

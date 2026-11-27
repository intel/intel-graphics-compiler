/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// The private array `p` takes 80 KB per thread at SIMD32, at least 60% of the
// range of HWord scratch messages, so the spills of the live vectors use OWord
// block messages. Their spill/fill intrinsics carry no offset, which the post-RA
// spill/fill propagation must not mistake for a single spill slot: it used to
// drop all but one spill, replace the fills with copies of unrelated registers,
// and shrink the spill size to 0.

// REQUIRES: regkeys, tgllp-supported

// RUN: ocloc compile -file %s -device tgllp -options "-igc_opts 'VISAOptions=-asmToConsole'" | FileCheck %s

// CHECK: //.spill size {{[1-9][0-9]*}}
// CHECK: //.private memory size 81920
// CHECK-COUNT-16: oword block write
// CHECK-COUNT-32: oword block read

#define P 640
#define R8(X) X(0) X(1) X(2) X(3) X(4) X(5) X(6) X(7)
#define LOAD(a) uint16 v##a = vload16(a * S + g, in);
#define STORE(a) vstore16(v##a + p[(a * s) % P], (7 - a) * S + g, out);

__attribute__((intel_reqd_sub_group_size(32)))
kernel void k(global uint *out, global const uint *in, uint s) {
  uint g = get_global_id(0), S = get_global_size(0);
  R8(LOAD)
  uint p[P];
  for (uint j = 0; j < P; j++)
    p[(j * s) % P] = j;
  R8(STORE)
}

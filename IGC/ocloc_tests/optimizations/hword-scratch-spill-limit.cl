/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// The 64 vectors loaded here all stay live until the stores, because `out` may
// alias `in` and the vectors are stored to reversed positions. At SIMD32 their
// spills, on top of the private array `p`, need more than the 128 KB that the
// 12-bit offset of a HWord scratch block message can address from the scratch
// base in r0.5. Without LSC, a spill or fill starting past that limit must
// move r0.5 to the next 128 KB window for the duration of the message, and
// encode its offset relative to that window, instead of wrapping around into
// private memory.

// REQUIRES: regkeys, tgllp-supported

// RUN: ocloc compile -file %s -device tgllp -options "-igc_opts 'VISAOptions=-asmToConsole'" | FileCheck %s

// The first message past the limit is bracketed by the window adjustment:
// CHECK-NOT: add (1|M0) {{r[0-9]+}}.5<1>:ud
// CHECK: add (1|M0) [[R0:r[0-9]+]].5<1>:ud [[R0]].5<0;1,0>:ud 131072:d
// CHECK-NOT: scratch block
// CHECK-NOT: [[R0]].5<1>:ud
// CHECK: send.dc0 (16|M0) null [[R0]] {{.*}} hword scratch block write
// CHECK-NOT: scratch block
// CHECK-NOT: [[R0]].5<1>:ud
// CHECK: add (1|M0) [[R0]].5<1>:ud [[R0]].5<0;1,0>:ud -131072:d

// A 4-GRF fill from HWord 4112 encodes offset 0x10 and block size 4 (0x2000):
// CHECK: send.dc0 (16|M0) {{r[0-9]+}} [[R0]] null 0x0 0x024C2010 {{.*}} hword scratch block read x4 {{.*}}offset{{\[}}4112x32]

#define P 256
#define R8(X, a) X(a, 0) X(a, 1) X(a, 2) X(a, 3) X(a, 4) X(a, 5) X(a, 6) X(a, 7)
#define R64(X) R8(X, 0) R8(X, 1) R8(X, 2) R8(X, 3) R8(X, 4) R8(X, 5) R8(X, 6) R8(X, 7)
#define LOAD(a, b) uint16 v##a##b = vload16((a * 8 + b) * S + g, in);
#define STORE(a, b) vstore16(v##a##b + p[((a * 8 + b) * s) % P], (63 - (a * 8 + b)) * S + g, out);

__attribute__((intel_reqd_sub_group_size(32)))
kernel void k(global uint *out, global const uint *in, uint s) {
  uint g = get_global_id(0), S = get_global_size(0);
  R64(LOAD)
  uint p[P];
  for (uint j = 0; j < P; j++)
    p[(j * s) % P] = j;
  R64(STORE)
}

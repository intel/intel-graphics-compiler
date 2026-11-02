/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// localDefHoisting folds the moves of each packed char into its definition, a
// mov from a dword shift into byte 1 of a word-aligned 2-byte variable. Gen12LP
// only supports a byte dst at byte 0 or 1 of a dword channel, so HWConformity
// must align that variable to a dword; otherwise RA may place it at byte 2,
// which puts the char at byte 3, and the hardware doesn't write it.

// REQUIRES: regkeys, tgllp-supported

// RUN: ocloc compile -file %s -device tgllp -options "-igc_opts 'VISAOptions=-asmToConsole'" | FileCheck %s
// RUN: ocloc compile -file %s -device tgllp -options "-igc_opts 'VISAOptions=-asmToConsole'" | FileCheck %s --check-prefix=CHECK-MOV

// No byte dst of a dword channel may start at byte 2 or 3 of it:
// CHECK-NOT: {{r[0-9]+\.(2|3|6|7|10|11|14|15|18|19|22|23|26|27|30|31)<4>:b}}
// CHECK: Build succeeded

// CHECK-MOV: mov (1|M0) {{r[0-9]+\.[0-9]+}}<4>:b {{r[0-9]+\.[0-9]+}}<0;1,0>:d
// CHECK-MOV: mov (1|M0) {{r[0-9]+\.[0-9]+}}<4>:b {{r[0-9]+\.[0-9]+}}<0;1,0>:d

typedef struct { uchar a; char b; ushort c; } T;

kernel void k(global T *out, uint s, uint t) {
  size_t i = get_global_id(0);
  out[2*i].a = s & 1;
  out[2*i].b = s >> 8;
  out[2*i].c = s >> 16;
  out[2*i+1].a = t & 1;
  out[2*i+1].b = t >> 8;
  out[2*i+1].c = t >> 3;
}

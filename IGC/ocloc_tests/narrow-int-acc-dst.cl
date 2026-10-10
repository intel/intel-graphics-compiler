/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// The accumulator keeps integer results at more than 16 bits, so a word result
// that overflowed is not wrapped when it is read back from acc. The signed max
// below must compare the wrapped mad result, so the mad cannot write to acc.

// REQUIRES: regkeys
// RUN: %if tgllp-supported %{ ocloc compile -file %s -options " -igc_opts 'VISAOptions=-asmToConsole'" -device tgllp | FileCheck %s %}
// RUN: %if dg2-supported %{ ocloc compile -file %s -options " -igc_opts 'VISAOptions=-asmToConsole'" -device dg2 | FileCheck %s %}

// CHECK-LABEL: //.kernel test
// CHECK: mad (16|M0) [[MAD:r[0-9]+\.[0-9]+]]<1>:w
// CHECK: sel (16|M0) (ge)f0.0 {{[^ ]+}}<1>:w [[MAD]]<{{[^>]+}}>:w
__attribute__((intel_reqd_sub_group_size(16)))
kernel void test(global const short2 *in, global short *out) {
  int i = get_global_id(0);
  short2 v = in[i];
  out[i] = max((short)(v.x * v.x + v.y * v.y), out[i]);
}

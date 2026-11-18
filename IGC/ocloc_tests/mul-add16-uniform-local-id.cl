/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys, pvc-supported
// RUN: ocloc compile -file %s -options " -igc_opts 'DumpVISAASMToConsole=1'" -device pvc | FileCheck %s

// With a work group of 32 x 4 x 8 at SIMD32, a thread covers all of X, so get_local_id(2) is uniform, but it is still
// stored per lane in the local ID payload. The uniform add that reads its zext as a word must read it as a scalar: the
// strided region of a half of a dword (<16;8,2>) is not valid at execution size 1.
// CHECK-NOT: (M1_NM, 1){{.*}}<16;8,2>
// CHECK: shl (M1_NM, 1) [[MUL:[^ (]+]](0,0)<1> {{[^ ]+}} 0x3:d
// CHECK-NEXT: add (M1_NM, 1) {{[^ (]+}}(0,0)<1> [[MUL]](0,0)<0;1,0> {{[^ (]+}}(0,0)<0;1,0>
// CHECK-NOT: (M1_NM, 1){{.*}}<16;8,2>

__attribute__((reqd_work_group_size(32, 4, 8))) __attribute__((intel_reqd_sub_group_size(32))) kernel void
uniform_local_id(global int *out) {
  int z = (int)get_group_id(2) * 8 + (int)get_local_id(2);
  out[z * 32 + get_local_id(0)] = z;
}

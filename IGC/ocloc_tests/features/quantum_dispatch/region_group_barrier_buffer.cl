/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys, cri-supported
// RUN: ocloc compile -64 -file %s -options "-igc_opts 'DumpVISAASMToConsole=1'" -device cri | FileCheck %s --check-prefix=CHECK-VISA

// CHECK-VISA: .input regionGroupBarrierBuffer offset={{.*}} size=8

__global volatile uchar* __builtin_IB_get_region_group_barrier_buffer();
kernel void test(__global volatile uchar* out) {
  out = __builtin_IB_get_region_group_barrier_buffer();
}
